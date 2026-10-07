# SPDX-License-Identifier: MPL-2.0
"""Compare revised SDR highlights against a retained pre-v0.38 native bridge.

Uses local full-precision source arrays; does not modify or package source media.
ROI color statistics do not establish spatial lens detail or highlight recovery.
"""

import argparse
import hashlib
import json
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw
from color_bench import ICC, ROOT, Renderer, display_preview, metrics, roi_mask


def emitter_metrics(rgb, mask):
    selected = rgb[mask]
    return {"pixels": int(len(selected)), "mean_rgb": selected.mean(0).tolist(),
            "near_white_09_percent": float(np.mean(selected.min(-1) > .9)*100),
            "red_code_std": float(selected[:, 0].std()),
            "code_std_rgb": selected.std(0).tolist()}


def compare(reference, updated, source, settings):
    old, new = reference.render(source, settings), updated.render(source, settings)
    if not np.array_equal(old, new):
        raise ValueError(f"Isolated output changed; max delta {np.max(np.abs(old-new))}")


def run(args):
    root = args.reference.resolve()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    reference = Renderer(args.reference_bridge)
    updated = Renderer(ROOT / "build/ofx/bench/ColorBench.dll")
    if reference.count != updated.count:
        raise ValueError("Compare bridges with the same settings layout")
    rows = json.loads((root / "results.json").read_text(encoding="utf-8"))
    results, isolated, taillights = [], 0, 0
    for row in rows:
        if row["model"] != "Production SDR":
            continue
        stem = row["float_output"].split("-preset", 1)[0]
        source = np.load(root / f"{stem}-input.npy")
        linear = np.load(root / f"{stem}-linear.npy")[..., :3]
        settings = np.array(row["settings"], np.float32)
        if len(settings) == updated.count-2:
            settings = np.concatenate([settings, np.array([1000, 203], np.float32)])
        if len(settings) != updated.count:
            raise ValueError("Unsupported retained settings layout")
        old = reference.render(source, settings)
        retained = np.load(root / row["float_output"])
        if not np.array_equal(old, retained):
            raise ValueError("Reference bridge does not reproduce the retained baseline")
        new = updated.render(source, settings)
        maximum_settings = settings.copy()
        maximum_settings[updated.dll.oe_retention_index()] = 1
        maximum = updated.render(source, maximum_settings)
        frame_mask = np.ones(source.shape[:2], bool)
        record = {"clip": row["clip"], "seconds": row["seconds"], "preset": row["preset"],
                  "source": f"{stem}-input.npy", "regions": {},
                  "old": metrics(old[..., :3], frame_mask),
                  "new": metrics(new[..., :3], frame_mask),
                  "maximum": metrics(maximum[..., :3], frame_mask)}
        for name, region in row["regions"].items():
            roi = roi_mask(*source.shape[:2], region["box"])
            entry = {"box": region["box"], "old": metrics(old[..., :3], roi),
                     "new": metrics(new[..., :3], roi), "maximum": metrics(maximum[..., :3], roi)}
            if "taillight" in name.lower():
                # Select highly red, bright source pixels, not output-dependent pixels.
                mask = roi & (linear[..., 0] > 4) & (linear[..., 0] > 2*np.maximum(linear[..., 1], linear[..., 2]))
                if mask.any():
                    entry["source_keyed_emitters"] = {
                        "old": emitter_metrics(old[..., :3], mask),
                        "new": emitter_metrics(new[..., :3], mask),
                        "maximum": emitter_metrics(maximum[..., :3], mask)}
                    if row["preset"] == "Neutral / Clean Slate":
                        before = entry["source_keyed_emitters"]["old"]["near_white_09_percent"]
                        after = entry["source_keyed_emitters"]["new"]["near_white_09_percent"]
                        if before > 1 and after >= before*.25:
                            raise ValueError("Taillight near-white washout was not reduced sufficiently")
                    taillights += 1
                ys, xs = np.nonzero(roi)
                images = [display_preview(c[ys.min():ys.max()+1, xs.min():xs.max()+1, :3]) for c in [old, new, maximum]]
                canvas = Image.new("RGB", (images[0].width*3, images[0].height+24), "#202020")
                draw = ImageDraw.Draw(canvas)
                for i, (label, image) in enumerate(zip([args.reference_label, args.updated_label+" default", "Retention 1"], images)):
                    canvas.paste(image, (i*image.width, 24))
                    draw.text((i*image.width+3, 5), label, fill="white")
                canvas.save(output / f"{stem}-{updated.presets[row['preset']]}-{name.replace(' ', '-')}.png", icc_profile=ICC)
            record["regions"][name] = entry
        if "tail lights" in row["clip"].lower():
            display_preview(new[..., :3]).save(output / f"{stem}-{updated.presets[row['preset']]}-new.png", icc_profile=ICC)
        results.append(record)
        conversion = settings.copy()
        conversion[updated.dll.oe_rendering_index()] = 1
        compare(reference, updated, source, conversion)
        hdr = settings.copy()
        hdr[27] = updated.dll.oe_hdr_output_index()
        hdr[updated.dll.oe_rendering_index()] = updated.dll.oe_hdr_rendering_index()
        compare(reference, updated, source, hdr)
        isolated += 2
        print(f"Checked {row['clip']} / {row['seconds']}s / {row['preset']}", flush=True)
    report = {"reference_bridge_sha256": hashlib.sha256(args.reference_bridge.read_bytes()).hexdigest(),
              "new_bridge_sha256": hashlib.sha256((ROOT / "build/ofx/bench/ColorBench.dll").read_bytes()).hexdigest(),
              "sdr_comparisons": len(results), "unchanged_conversion_hdr_pairs": isolated,
              "taillight_emitter_regions": taillights, "results": results}
    (output / "results.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(f"PASS: {len(results)} SDR comparisons, {taillights} taillight regions, {isolated} bit-exact Conversion Only/HDR pairs.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path, help="Retained pre-v0.38 footage bench directory")
    parser.add_argument("--reference-bridge", type=Path, required=True, help="Retained pre-v0.38 ColorBench.dll")
    parser.add_argument("--reference-label", default="v0.37")
    parser.add_argument("--updated-label", default="v0.39")
    parser.add_argument("--output", type=Path, default=ROOT / "analysis/highlight-detail-v039")
    run(parser.parse_args())
