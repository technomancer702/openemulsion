# SPDX-License-Identifier: MPL-2.0
"""Offline color-only footage evaluation. PNGs are previews; metrics use float RGB."""

from __future__ import annotations

import argparse
import ctypes
import hashlib
import html
import json
import subprocess
from pathlib import Path

import numpy as np
from PIL import Image, ImageCms

ROOT = Path(__file__).resolve().parents[1]
LOCAL_DEPS = ROOT / "build" / "tuning-python-deps"
if LOCAL_DEPS.exists():
    import sys
    sys.path.insert(0, str(LOCAL_DEPS))
import av

FLOATS = np.ctypeslib.ndpointer(dtype=np.float32, flags="C_CONTIGUOUS")
MASK = 19
NEGATIVE, PRINT, DEVELOPMENT, SELECTIVE = 1, 2, 32, 128
COLOR_MODULES = NEGATIVE | PRINT | DEVELOPMENT | SELECTIVE
ICC = ImageCms.ImageCmsProfile(ImageCms.createProfile("sRGB")).tobytes()


class Renderer:
    def __init__(self, path: Path):
        self.dll = ctypes.CDLL(str(path.resolve()))
        self.dll.oe_settings_count.restype = ctypes.c_int
        self.dll.oe_rendering_index.restype = ctypes.c_int
        self.dll.oe_retention_index.restype = ctypes.c_int
        for name in ["oe_hdr_peak_index","oe_hdr_white_index","oe_hdr_output_index","oe_hdr_rendering_index"]:
            getattr(self.dll,name).restype = ctypes.c_int
        self.dll.oe_preset_count.restype = ctypes.c_int
        self.dll.oe_preset_label.argtypes = [ctypes.c_int]
        self.dll.oe_preset_label.restype = ctypes.c_char_p
        self.dll.oe_preset_settings.argtypes = [ctypes.c_int, FLOATS, ctypes.c_size_t]
        self.dll.oe_render.argtypes = [FLOATS, FLOATS, ctypes.c_size_t, FLOATS, ctypes.c_size_t, ctypes.c_float]
        self.dll.oe_decode_linear.argtypes = [FLOATS, FLOATS, ctypes.c_size_t, ctypes.c_int]
        self.count = self.dll.oe_settings_count()
        self.sdr_indices = {}
        for control in ["contrast", "rolloff", "gamut"]:
            function = getattr(self.dll, f"oe_sdr_{control}_index", None)
            if function is not None:
                function.restype = ctypes.c_int
                self.sdr_indices[control] = function()
        self.presets = {self.dll.oe_preset_label(i).decode(): i for i in range(self.dll.oe_preset_count())}
        self.hdr_indices = {}
        for control in ["exposure", "rolloff"]:
            function = getattr(self.dll, f"oe_hdr_{control}_index", None)
            if function is not None:
                function.restype = ctypes.c_int
                self.hdr_indices[control] = function()

    def settings(self, label: str, source: int, rendering: int = 0, retention: float = 0) -> np.ndarray:
        s = np.zeros(self.count, dtype=np.float32)
        if self.dll.oe_preset_settings(self.presets[label], s, self.count):
            raise ValueError("Invalid native recipe")
        s[26], s[27], s[self.dll.oe_rendering_index()] = source, 1, rendering
        s[self.dll.oe_retention_index()] = retention
        s[MASK] = int(s[MASK]) & COLOR_MODULES
        return s

    def render(self, rgba: np.ndarray, settings: np.ndarray, blend: float = 0) -> np.ndarray:
        self.validate_frame(rgba)
        if settings.shape != (self.count,) or not np.isfinite(settings).all():
            raise ValueError("Invalid settings array")
        out = np.empty_like(rgba)
        if self.dll.oe_render(rgba, out, rgba.shape[0] * rgba.shape[1], settings, self.count, blend):
            raise ValueError("Native color renderer rejected the request")
        if not np.isfinite(out).all():
            raise ValueError("Native renderer produced nonfinite output")
        if not np.array_equal(out[..., 3], rgba[..., 3]):
            raise ValueError("Renderer altered alpha")
        return out

    def linear(self, rgba: np.ndarray, source: int) -> np.ndarray:
        self.validate_frame(rgba)
        out = np.empty_like(rgba)
        if self.dll.oe_decode_linear(rgba, out, rgba.shape[0] * rgba.shape[1], source):
            raise ValueError("Invalid source space")
        if not np.isfinite(out).all():
            raise ValueError("Nonfinite source conversion")
        return out

    @staticmethod
    def validate_frame(frame: np.ndarray) -> None:
        if frame.ndim != 3 or frame.shape[2] != 4 or not frame.size or not np.isfinite(frame).all():
            raise ValueError("Expected nonempty finite HxWx4 float RGBA")


def rgba(rgb: np.ndarray) -> np.ndarray:
    out = np.ones((*rgb.shape[:2], 4), dtype=np.float32)
    out[..., :3] = rgb
    return out


def probe(path: Path) -> dict:
    with av.open(str(path)) as container:
        stream = container.streams.video[0]
        c = stream.codec_context
        if stream.duration is None:
            raise ValueError(f"Missing video duration: {path}")
        return {"file": path.name, "width": stream.width, "height": stream.height,
                "duration": float(stream.duration * stream.time_base), "fps": str(stream.average_rate),
                "frames": stream.frames, "codec": c.name, "pixel_format": c.format.name,
                "range_tag": c.color_range, "matrix_tag": c.colorspace,
                "transfer_tag": c.color_trc, "primaries_tag": c.color_primaries}


def decode(path: Path, seconds: float, matrix: str, range_override: str) -> tuple[np.ndarray, float]:
    with av.open(str(path)) as container:
        stream = container.streams.video[0]
        start = stream.start_time or 0
        target = start + int(seconds / float(stream.time_base))
        container.seek(target, stream=stream, backward=True)
        for frame in container.decode(stream):
            if frame.pts is not None and frame.pts >= target:
                break
        else:
            raise ValueError(f"Cannot decode requested frame: {path.name} @ {seconds}")
        color_range = range_override
        if color_range == "auto":
            tag = frame.color_range or stream.codec_context.color_range
            color_range = {1: "MPEG", 2: "JPEG"}.get(tag)
            if color_range is None:
                raise ValueError(f"Unspecified data levels in {path.name}; explicitly use --range MPEG or JPEG")
        rgb = unpack_yuv444(frame, matrix, color_range)
        if rgb.dtype != np.float32 or rgb.shape != (frame.height, frame.width, 3) or not np.isfinite(rgb).all():
            raise ValueError("Unexpected float RGB decode")
        return rgba(rgb), float((frame.pts - start) * stream.time_base)


def unpack_yuv444(frame: av.VideoFrame, matrix: str, levels: str) -> np.ndarray:
    """Unpack decoded planar YCbCr without an intermediate quantizer or clamp."""
    fmt = frame.format
    components = fmt.components
    if (not fmt.is_planar or fmt.is_rgb or len(components) != 3 or
            any(c.width != frame.width or c.height != frame.height or c.plane != i
                for i, c in enumerate(components)) or
            len({c.bits for c in components}) != 1 or not 8 <= components[0].bits <= 16):
        raise ValueError(f"Bench requires planar integer YUV 4:4:4; got {fmt.name}")
    bits = components[0].bits
    dtype = np.dtype('u1' if bits == 8 else ('>u2' if fmt.is_big_endian else '<u2'))
    planes = [np.ndarray((p.height, p.width), dtype=dtype, buffer=p,
                         strides=(p.line_size, dtype.itemsize)).astype(np.float32) for p in frame.planes]
    scale, center = float(1 << (bits - 8)), float(1 << (bits - 1))
    if levels == "MPEG":
        y = (planes[0] - 16 * scale) / (219 * scale)
        cb, cr = [(v - center) / (224 * scale) for v in planes[1:]]
    elif levels == "JPEG":
        maximum = float((1 << bits) - 1)
        y = planes[0] / maximum
        cb, cr = [(v - center) / maximum for v in planes[1:]]
    else:
        raise ValueError("Explicit MPEG or JPEG levels required")
    # ITU non-constant-luminance packing coefficients; camera primaries/log are separate.
    kr, kb = {"ITU709": (.2126, .0722), "ITU601": (.299, .114), "BT2020": (.2627, .0593)}[matrix]
    r, b = y + 2 * (1 - kr) * cr, y + 2 * (1 - kb) * cb
    g = (y - kr * r - kb * b) / (1 - kr - kb)
    return np.stack((r, g, b), axis=-1)


def display_preview(rgb: np.ndarray) -> Image.Image:
    # A browser expects sRGB PNGs, not our Rec.709/Gamma 2.4 output code values.
    linear = np.clip(rgb, 0, 1).astype(np.float64) ** 2.4
    srgb = np.where(linear <= .0031308, linear * 12.92, 1.055 * linear ** (1 / 2.4) - .055)
    return Image.fromarray(np.rint(np.clip(srgb, 0, 1) * 255).astype(np.uint8))


def metrics(rgb: np.ndarray, mask: np.ndarray) -> dict:
    values = rgb[mask]
    lo, hi = values.min(axis=1), values.max(axis=1)
    positive = np.maximum(values, 0)
    positive_lo, positive_hi = positive.min(axis=1), positive.max(axis=1)
    y = values @ np.array([.2126, .7152, .0722])
    return {"pixels": int(values.shape[0]), "minimum": float(values.min()), "maximum": float(values.max()),
            "channel_outside_0_1_pct": float(np.mean((values < 0) | (values > 1)) * 100),
            "any_channel_above_1_pct": float(np.mean(hi > 1) * 100),
            "all_channels_above_097_pct": float(np.mean(lo > .97) * 100),
            "luma_percentiles": np.percentile(y, [1, 10, 50, 90, 99]).tolist(),
            "mean_rgb": values.mean(axis=0).tolist(),
            "mean_code_saturation": float(np.mean((positive_hi - positive_lo) / np.maximum(positive_hi, 1e-6)))}


def content_mask(height: int, width: int, exclusions: list[list[float]] = ()) -> np.ndarray:
    mask = np.ones((height, width), dtype=bool)
    for box in exclusions:
        mask &= ~roi_mask(height, width, box)
    if not mask.any():
        raise ValueError("Exclusions cover the entire frame")
    return mask


def roi_mask(height: int, width: int, box: list[float]) -> np.ndarray:
    x0, y0, x1, y1 = box
    if not 0 <= x0 < x1 <= 1 or not 0 <= y0 < y1 <= 1:
        raise ValueError("ROI coordinates must be normalized rectangles")
    mask = np.zeros((height, width), dtype=bool)
    mask[int(y0 * height):max(int(y0 * height) + 1, int(y1 * height)),
         int(x0 * width):max(int(x0 * width) + 1, int(x1 * width))] = True
    return mask


def plot_scopes(rgb: np.ndarray, path: Path) -> None:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    data = rgb[::3, ::3]
    fig, axes = plt.subplots(1, 2, figsize=(9, 2.8), facecolor="#181818")
    for ax in axes:
        ax.set_facecolor("#181818"); ax.tick_params(colors="#bbbbbb")
        for spine in ax.spines.values(): spine.set_color("#555555")
    for channel, color in enumerate(("#ee7777", "#77cc99", "#7799ee")):
        axes[0].hist(data[..., channel].ravel(), bins=160, range=(-.05, 1.05), histtype="step", color=color)
    axes[0].set_yscale("log"); axes[0].set_xlim(-.05, 1.05); axes[0].set_title("RGB Code Distribution", color="white")
    y = data @ np.array([.2126, .7152, .0722])
    x = np.broadcast_to(np.arange(y.shape[1]), y.shape)
    axes[1].hist2d(x.ravel(), y.ravel(), bins=(150, 150), range=((0, y.shape[1]), (-.05, 1.05)), cmap="Greys_r")
    axes[1].set_ylim(-.05, 1.05); axes[1].set_title("Gamma 2.4 Luma Waveform", color="white")
    fig.tight_layout(); fig.savefig(path, dpi=120); plt.close(fig)


def build_report(out: Path, records: list[dict], manifest: dict) -> None:
    payload = json.dumps(records).replace("<", "\\u003c")
    info = html.escape(f"{manifest['revision'][:12]} | Source ID {manifest['source_space']} -> Rec.709 / Gamma 2.4 | Color Only | PyAV {av.__version__}")
    document = """<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>OpenEmulsion Color Bench</title><style>
*{box-sizing:border-box}body{margin:0;background:#171717;color:#eee;font:14px system-ui;letter-spacing:0}header,main{padding:20px 24px;max-width:1800px;margin:auto}header{border-bottom:1px solid #444}h1{font-size:22px;margin:0 0 8px}small{color:#aaa}nav{display:flex;flex-wrap:wrap;gap:12px;margin:0 0 18px}label{display:grid;gap:5px;font-size:12px;color:#bbb}select{background:#282828;border:1px solid #555;color:white;padding:7px;max-width:100%;border-radius:3px}section{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:18px}figure{margin:0;min-width:0}figcaption{padding:8px 0;color:#a2d8bc}img.frame{display:block;width:100%;aspect-ratio:16/9;object-fit:contain;background:#000}img.scope{width:100%;display:block}dl{display:grid;grid-template-columns:1fr auto;gap:6px;margin:12px 0;font-size:12px}dt{color:#bbb}dd{margin:0;font-variant-numeric:tabular-nums}footer{padding:14px 0;color:#aaa;font-size:12px}@media(max-width:800px){section{grid-template-columns:1fr}header,main{padding:16px}}
</style><header><h1>OpenEmulsion Color Bench</h1><small>__INFO__</small></header>
<main><nav><label>Clip<select id="clip"></select></label><label>Frame<select id="frame"></select></label><label>Preset<select id="preset"></select></label><label>A<select id="left"></select></label><label>B<select id="right"></select></label><label>Region<select id="region"><option value="">Full Frame</option></select></label></nav>
<section><figure><figcaption id="titleA"></figcaption><a id="linkA"><img id="imageA" class="frame"></a><dl id="statsA"></dl><img id="scopeA" class="scope"></figure><figure><figcaption id="titleB"></figcaption><a id="linkB"><img id="imageB" class="frame"></a><dl id="statsB"></dl><img id="scopeB" class="scope"></figure></section><footer>Production = compiled plugin math. Retention = plugin Highlight Color Retention. Peak blend = bench-only experiment. No grain/glow. Float measurements precede preview clipping; PNG previews are sRGB encoded. Footage and results stay local.</footer></main>
<script>const rows=__DATA__;const ids=['clip','frame','preset','left','right','region'];const el=Object.fromEntries(ids.map(id=>[id,document.getElementById(id)]));
function fill(select,values){let old=select.value;select.replaceChildren(...values.map(v=>new Option(v,v)));if(values.includes(old))select.value=old;}
fill(el.clip,[...new Set(rows.map(r=>r.clip))]);fill(el.preset,[...new Set(rows.map(r=>r.preset))]);const models=[...new Set(rows.map(r=>r.model))];fill(el.left,models);fill(el.right,models);el.left.value='Production SDR';el.right.value=models.includes('Peak blend 0.35')?'Peak blend 0.35':models[models.length-1];
function update(){const available=rows.filter(r=>r.clip===el.clip.value);fill(el.frame,[...new Set(available.map(r=>String(r.seconds)))]);const rois=[...new Set(available.flatMap(r=>Object.keys(r.regions)))];const old=el.region.value;el.region.replaceChildren(new Option('Full Frame',''),...rois.map(v=>new Option(v,v)));if(rois.includes(old))el.region.value=old;
for(const [side,key] of [['A','left'],['B','right']]){let r=available.find(r=>String(r.seconds)===el.frame.value&&r.preset===el.preset.value&&r.model===el[key].value);if(!r)continue;const selected=el.region.value?r.regions[el.region.value]:r;const image=document.getElementById('image'+side);image.src=selected.image;image.alt=r.clip+' / '+r.preset+' / '+r.model;image.style.aspectRatio=el.region.value?'auto':String(r.aspect);document.getElementById('link'+side).href=selected.image;document.getElementById('title'+side).textContent=r.model+' / '+r.seconds+' s';const m=selected.metrics;const items=[['Outside [0,1] channels',m.channel_outside_0_1_pct.toFixed(4)+'%'],['Near-white pixels (RGB > .97)',m.all_channels_above_097_pct.toFixed(4)+'%'],['Median / 99th-percentile luma',m.luma_percentiles[2].toFixed(4)+' / '+m.luma_percentiles[4].toFixed(4)],['Mean code saturation',m.mean_code_saturation.toFixed(4)]];document.getElementById('stats'+side).replaceChildren(...items.flatMap(([name,value])=>{const a=document.createElement('dt'),b=document.createElement('dd');a.textContent=name;b.textContent=value;return[a,b]}));document.getElementById('scope'+side).src=r.scope;}}
for(const select of Object.values(el))select.addEventListener('change',update);update();</script></html>"""
    (out / "index.html").write_text(document.replace("__INFO__", info).replace("__DATA__", payload), encoding="utf-8")


def run(args: argparse.Namespace) -> None:
    renderer = Renderer(args.library)
    files = sorted(args.footage.glob("*.mov"))
    if not files:
        raise ValueError("No .mov test clips found")
    out = args.output.resolve(); out.mkdir(parents=True, exist_ok=True)
    crops = json.loads(args.rois.read_text(encoding="utf-8")) if args.rois else {}
    revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
    source_paths = [*sorted((ROOT / "ofx" / "src").glob("*.h")), ROOT / "ofx" / "tools" / "ColorBench.cpp", Path(__file__)]
    manifest = {"revision": revision, "library_sha256": hashlib.sha256(args.library.read_bytes()).hexdigest(),
                "working_tree_status": subprocess.check_output(["git", "status", "--short"], cwd=ROOT, text=True),
                "source_sha256": {p.relative_to(ROOT).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in source_paths},
                "source_space": args.source, "packing_matrix": args.matrix, "range_override": args.range,
                "decode": "PyAV integer planar YUV444 -> float RGB; exact nominal packing matrix/levels, no clamp or transfer/primary conversion",
                "preview": "Gamma 2.4 decoded then sRGB encoded, embedded sRGB ICC profile",
                "code_saturation": "(max-min)/max of nonnegative RGB; diagnostic only, not perceptual accuracy",
                "sampling": "integer-stride sampling, not rescaling; no original media modified",
                "regions_and_exclusions": crops,
                "decoder_version": av.__version__, "stride": args.stride, "sample_fractions": args.samples,
                "models": [{"name": "Production SDR", "blend": 0, "rendering": 0},
                           {"name": "Conversion Only", "blend": 0, "rendering": 1}] +
                          [{"name": f"Retention {v:g}", "retention": v, "blend": 0, "rendering": 0} for v in args.retentions] +
                          [{"name": f"Peak blend {v:g}", "blend": v, "rendering": 0} for v in args.blends],
                "clips": [probe(path) for path in files]}
    records = []
    for clip_index, path in enumerate(files):
        meta = manifest["clips"][clip_index]
        for sample_index, fraction in enumerate(args.samples):
            frame, seconds = decode(path, meta["duration"] * fraction, args.matrix, args.range)
            frame = np.ascontiguousarray(frame[::args.stride, ::args.stride])
            height, width = frame.shape[:2]
            clip_config = crops.get(path.name, {})
            content = content_mask(height, width, clip_config.get("exclude", []))
            linear = renderer.linear(frame, args.source)
            base = f"clip{clip_index}-frame{sample_index}"
            np.save(out / f"{base}-input.npy", frame)
            np.save(out / f"{base}-linear.npy", linear)
            source_stats = metrics(frame[..., :3], content)
            print(f"{path.name} @ {seconds:.3f}s ({width}x{height})", flush=True)
            for preset_index, preset in enumerate(args.presets):
                for model_index, model in enumerate(manifest["models"]):
                    s = renderer.settings(preset, args.source, model["rendering"], model.get("retention",0))
                    rendered = renderer.render(frame, s, model["blend"])
                    filename = f"{base}-preset{preset_index}-model{model_index}"
                    image = display_preview(rendered[..., :3])
                    image.save(out / f"{filename}.png", icc_profile=ICC)
                    np.save(out / f"{filename}.npy", rendered)
                    plot_scopes(rendered[..., :3], out / f"{filename}-scope.png")
                    region_records = {}
                    for name, box in clip_config.get("regions", {}).items():
                        mask = roi_mask(height, width, box)
                        ys, xs = np.nonzero(mask)
                        crop = image.crop((int(xs.min()), int(ys.min()), int(xs.max()) + 1, int(ys.max()) + 1))
                        crop_name = f"{filename}-roi{len(region_records)}.png"
                        crop.save(out / crop_name, icc_profile=ICC)
                        region_records[name] = {"image": crop_name, "box": box,
                                                "metrics": metrics(rendered[..., :3], mask),
                                                "source_linear_metrics": metrics(linear[..., :3], mask)}
                    records.append({"clip": path.name, "seconds": round(seconds, 3), "preset": preset,
                                    "model": model["name"], "image": f"{filename}.png", "aspect": width / height,
                                    "scope": f"{filename}-scope.png", "float_output": f"{filename}.npy",
                                    "settings": s.tolist(), "metrics": metrics(rendered[..., :3], content),
                                    "source_code_metrics": source_stats, "regions": region_records})
        (out / "results.json").write_text(json.dumps(records, indent=2), encoding="utf-8")
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    (out / "results.json").write_text(json.dumps(records, indent=2), encoding="utf-8")
    build_report(out, records, manifest)
    print(out / "index.html", flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--footage", type=Path, default=ROOT / "test footage")
    parser.add_argument("--output", type=Path, default=ROOT / "analysis" / "color-bench")
    parser.add_argument("--library", type=Path, default=ROOT / "build" / "ofx" / "bench" / "ColorBench.dll")
    parser.add_argument("--source", type=int, required=True, help="Native color space ID; Alexa LogC3 is 0")
    parser.add_argument("--matrix", choices=["ITU709", "ITU601", "BT2020"], default="ITU709")
    parser.add_argument("--range", choices=["auto", "MPEG", "JPEG"], default="auto")
    parser.add_argument("--stride", type=int, default=2)
    parser.add_argument("--samples", nargs="+", type=float, default=[.2, .5, .8])
    parser.add_argument("--blends", nargs="*", type=float, default=[.15, .35, .6, 1.0])
    parser.add_argument("--retentions", nargs="*", type=float, default=[])
    parser.add_argument("--presets", nargs="+", default=["Neutral / Clean Slate", "50D Daylight"])
    parser.add_argument("--rois", type=Path)
    args = parser.parse_args()
    if args.stride < 1 or not all(0 <= v < 1 for v in args.samples) or not all(0 < v <= 1 for v in args.blends + args.retentions):
        parser.error("Invalid stride, sample fraction or peak blend")
    run(args)
