# SPDX-License-Identifier: MPL-2.0
"""Render licensed, display-ready stock clips through the production OpenCL pipeline."""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import tempfile
from pathlib import Path

import color_bench as bench
import numpy as np
from PIL import Image, ImageDraw
from render_readme_examples import font, resize

ROOT = Path(__file__).resolve().parents[1]
CLIPS = {
    "headlights": ("pexels-headlights-16815342.mp4", 4.0,
                   "https://www.pexels.com/video/close-up-view-of-a-car-with-lights-on-in-the-pouring-rain-16815342/",
                   "Erik Mclean / Pexels"),
    "forest": ("mixkit-forest-50861.mp4", 4.0,
               "https://mixkit.co/free-stock-video/a-spooky-looking-forest-surrounded-by-the-morning-fog-50861/",
               "Mixkit"),
    "desk": ("pexels-desk-5095957.mp4", 6.0,
             "https://www.pexels.com/video/a-man-sitting-at-a-desk-in-a-room-with-windows-5095957/",
             "cottonbro studio / Pexels"),
}
EXAMPLES = {
    "halation": ("headlights", "Neutral / Clean Slate"),
    "folk-dread": ("forest", "Folk Dread (The Witch)"),
    "archive-thriller": ("desk", "Archive Thriller (Zodiac)"),
}
REC709_GAMMA24, CONVERSION_ONLY = 5, 1
HALATION, AURA, GRAIN, BLOOM = 4, 8, 16, 64


class FullRenderer(bench.Renderer):
    def __init__(self, library: Path, executable: Path):
        super().__init__(library)
        self.executable = executable.resolve()

    def render(self, rgba: np.ndarray, settings: np.ndarray, time: float = 0) -> np.ndarray:
        self.validate_frame(rgba)
        if rgba.dtype != np.float32 or not rgba.flags.c_contiguous:
            raise ValueError("Expected contiguous float32 RGBA")
        if (settings.shape != (self.count,) or settings.dtype != np.float32 or
                not settings.flags.c_contiguous or not np.isfinite(settings).all() or not np.isfinite(time)):
            raise ValueError("Invalid settings or frame time")
        with tempfile.TemporaryDirectory(prefix="oe-preview-") as folder:
            path = Path(folder)
            rgba.tofile(path / "input.f32")
            settings.tofile(path / "settings.f32")
            result = subprocess.run([str(self.executable), str(path / "input.f32"), str(path / "output.f32"),
                                     str(rgba.shape[1]), str(rgba.shape[0]), str(path / "settings.f32"), str(time)],
                                    capture_output=True, text=True, timeout=120)
            if result.returncode:
                raise RuntimeError(f"Production OpenCL render failed: {result.stderr.strip()}")
            out = np.fromfile(path / "output.f32", dtype=np.float32).reshape(rgba.shape)
        if not np.isfinite(out).all() or not np.array_equal(out[..., 3], rgba[..., 3]):
            raise ValueError("Invalid output or altered alpha")
        return out


def decode_stock(path: Path, seconds: float) -> tuple[np.ndarray, float]:
    """Use libswscale for chroma reconstruction; preserve Rec.709 video code values."""
    metadata = bench.probe(path)
    if any(metadata[key] != 1 for key in ("range_tag", "matrix_tag", "transfer_tag", "primaries_tag")):
        raise ValueError("Stock renderer expects explicitly tagged limited-range BT.709 SDR footage")
    if not 0 <= seconds < metadata["duration"]:
        raise ValueError("Requested time is outside the clip")
    with bench.av.open(str(path)) as container:
        stream = container.streams.video[0]
        start = stream.start_time or 0
        target = start + int(seconds / float(stream.time_base))
        container.seek(target, stream=stream, backward=True)
        for frame in container.decode(stream):
            if frame.pts is not None and frame.pts >= target:
                break
        else:
            raise ValueError("No frame at requested time")
        timestamp = float((frame.pts - start) * stream.time_base)
        converted = bench.av.video.reformatter.VideoReformatter().reformat(
            frame, format="yuv444p16le", src_colorspace="ITU709", dst_colorspace="ITU709",
            src_color_range="MPEG", dst_color_range="MPEG", interpolation="BICUBIC")
        rgb = bench.unpack_yuv444(converted, "ITU709", "MPEG")
        return bench.rgba(rgb), timestamp


def settings_for_example(renderer: bench.Renderer, slug: str) -> np.ndarray:
    preset = EXAMPLES[slug][1]
    settings = renderer.settings(preset, REC709_GAMMA24, CONVERSION_ONLY)
    if slug == "halation":
        settings[0], settings[bench.MASK] = 2, HALATION
        settings[13], settings[14], settings[15], settings[16] = .45, 1.0, 0, 0
        settings[46], settings[47], settings[48] = .65, .25, .2
    return settings


def comparison(left: Image.Image, right: Image.Image, left_label: str, right_label: str) -> Image.Image:
    if left.size != right.size:
        raise ValueError("Comparison dimensions differ")
    left, right = resize(left, 960), resize(right, 960)
    image = Image.new("RGB", (1920, left.height + 56), "#181818")
    image.paste(left, (0, 56)); image.paste(right, (960, 56))
    draw = ImageDraw.Draw(image)
    for x, label in ((20, left_label), (980, right_label)):
        if draw.textbbox((0, 0), label, font=font(23))[2] > 920:
            raise ValueError("Comparison label is too wide")
        draw.text((x, 13), label, font=font(23), fill="white")
    return image


def contacts(destination: Path) -> None:
    tiles = []
    for name, (filename, _, _, _) in CLIPS.items():
        path = ROOT / "test footage" / filename
        duration = bench.probe(path)["duration"]
        for fraction in (.2, .5, .8):
            frame, seconds = decode_stock(path, duration * fraction)
            image = resize(bench.display_preview(frame[..., :3]), 480)
            tile = Image.new("RGB", (480, 310), "#181818")
            tile.paste(image, (0, 40))
            ImageDraw.Draw(tile).text((12, 10), f"{name} / {seconds:.2f}s", font=font(18), fill="white")
            tiles.append(tile)
    sheet = Image.new("RGB", (1440, 930), "#181818")
    for i, tile in enumerate(tiles):
        sheet.paste(tile, (i % 3 * 480, i // 3 * 310))
    destination.mkdir(parents=True, exist_ok=True)
    sheet.save(destination / "stock-contact-sheet.jpg", quality=94, subsampling=0, icc_profile=bench.ICC)


def examples(renderer: FullRenderer, destination: Path, only: list[str] | None = None) -> None:
    destination.mkdir(parents=True, exist_ok=True)
    records = []
    for slug, (clip, preset) in EXAMPLES.items():
        if only and slug not in only:
            continue
        filename, seconds, source_url, credit = CLIPS[clip]
        path = ROOT / "test footage" / filename
        frame, timestamp = decode_stock(path, seconds)
        settings = settings_for_example(renderer, slug)
        output = renderer.render(frame, settings, timestamp)
        before, after = bench.display_preview(frame[..., :3]), bench.display_preview(output[..., :3])
        if slug == "halation":
            off = settings.copy(); off[13] = 0
            baseline = renderer.render(frame, off, timestamp)
            if not np.array_equal(baseline, frame):
                raise ValueError("Halation-off reference is not exact source pass-through")
            left_label, right_label = "Halation Off", "Halation On / no bloom or aura"
        else:
            left_label, right_label = "Before / Original Rec.709", f"After / {preset}"
        difference = float(np.mean(np.abs(np.asarray(after, dtype=float) - np.asarray(before, dtype=float))) / 255)
        if difference < .001:
            raise ValueError(f"Indistinguishable comparison: {slug}")
        image = comparison(before, after, left_label, right_label)
        image.save(destination / f"{slug}.jpg", quality=94, subsampling=0, icc_profile=bench.ICC)
        records.append({"image": f"{slug}.jpg", "clip": filename, "seconds": timestamp,
                        "source_url": source_url, "credit": credit, "preset": preset,
                        "source_sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
                        "mean_preview_difference": difference, "settings": settings.tolist(),
                        "metadata": bench.probe(path)})
        print(f"Saved {slug}.jpg: {filename} @ {timestamp:.3f}s / {difference:.4f}", flush=True)
    report = ROOT / "analysis/readme-examples"
    report.mkdir(parents=True, exist_ok=True)
    manifest = {"pipeline": "Production OpenCL; source treated as display-ready Rec.709 / Gamma 2.4; Conversion Only viewing; sRGB browser previews; no extra grade or crop",
                "decoder": "PyAV/libswscale bicubic chroma reconstruction to limited-range 16-bit YUV444, then float BT.709 matrix/levels",
                "bridge_sha256": hashlib.sha256(renderer.executable.read_bytes()).hexdigest(),
                "examples": records}
    (report / "stock-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--contact-only", action="store_true")
    parser.add_argument("--only", nargs="+", choices=list(EXAMPLES))
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.contact_only:
        contacts(args.output or ROOT / "analysis/readme-examples")
    else:
        renderer = FullRenderer(ROOT / "build/ofx/bench/ColorBench.dll", ROOT / "build/ofx/bench/FullFrameBench.exe")
        examples(renderer, args.output or ROOT / "docs/media", args.only)


if __name__ == "__main__":
    main()
