# SPDX-License-Identifier: MPL-2.0
"""Audit SDR controls and stacked tone response using native production math.

Local footage and renders stay under ignored analysis/. Comparisons establish
engineering behavior, not film-stock fidelity or agreement with SpektraFilm.
"""

import argparse
import hashlib
import json
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

from color_bench import ICC, MASK, NEGATIVE, PRINT, ROOT, Renderer, display_preview, rgba


VARIANTS = {"Default": {}, "Softer contrast": {"contrast": -1},
            "Stronger contrast": {"contrast": 1}, "Later rolloff": {"rolloff": -1},
            "Earlier rolloff": {"rolloff": 1}, "Less gamut compression": {"gamut": -1},
            "More gamut compression": {"gamut": 1}}


def tuned(renderer, settings, values):
    result = settings.copy()
    for name, value in values.items():
        result[renderer.sdr_indices[name]] = value
    return result


def luminance(output):
    return np.maximum(output[..., :3].astype(np.float64), 0)**2.4 @ np.array([.2126, .7152, .0722])


def run(args):
    old, new = Renderer(args.reference_bridge), Renderer(args.bridge)
    if new.count != old.count+3 or set(new.sdr_indices) != {"contrast", "rolloff", "gamut"}:
        raise ValueError("Expected v0.39 reference and three appended SDR controls")
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    stages = []
    exposure = np.exp2(np.linspace(-14, 14, 281)).astype(np.float32)
    ramp = rgba(np.repeat(exposure[None, :, None], 3, axis=2))
    for label in new.presets:
        if label == "Custom / Current Settings":
            continue
        base = new.settings(label, 14)
        for stage in ["Viewing only", "Negative only", "Print only", "Combined"]:
            settings = base.copy()
            # Native strengths occupy stable indices 41..44. Keep profile tone
            # values intact; disable stages rather than changing their curves.
            settings[MASK] = NEGATIVE | PRINT
            if stage in ["Viewing only", "Print only"]:
                settings[41:43] = 0
            if stage in ["Viewing only", "Negative only"]:
                settings[43:45] = 0
            for variant, values in VARIANTS.items():
                result = new.render(ramp, tuned(new, settings, values))
                y = luminance(result)[0]
                stages.append({"preset": label, "stage": stage, "variant": variant,
                               "scene_linear": exposure.tolist(), "display_linear_y": y.tolist(),
                               "local_ev_slope": np.diff(y).tolist()})
    rows = json.loads((args.footage / "results.json").read_text(encoding="utf-8"))
    comparisons, isolated, previews = 0, 0, []
    seen = set()
    for row in rows:
        if row["model"] != "Production SDR":
            continue
        stem = row["float_output"].split("-preset", 1)[0]
        source = np.load(args.footage / f"{stem}-input.npy")
        original = np.asarray(row["settings"], np.float32)
        if len(original) != old.count-2:
            raise ValueError("Expected retained pre-HDR footage settings")
        old_settings = np.concatenate([original, np.array([1000, 203], np.float32)])
        settings = np.concatenate([old_settings, np.zeros(3, np.float32)])
        baseline, current = old.render(source, old_settings), new.render(source, settings)
        np.testing.assert_array_equal(baseline, current, err_msg="Zero controls alter v0.39 footage")
        comparisons += 1
        for hdr in [False, True]:
            inactive = settings.copy()
            inactive[new.dll.oe_rendering_index()] = new.dll.oe_hdr_rendering_index() if hdr else 1
            if hdr:
                inactive[27] = new.dll.oe_hdr_output_index()
            neutral = new.render(source, inactive)
            changed = new.render(source, tuned(new, inactive, {"contrast": 1, "rolloff": -1, "gamut": 1}))
            np.testing.assert_array_equal(neutral, changed, err_msg="SDR controls affect inactive rendering")
            np.testing.assert_array_equal(old.render(source, inactive[:old.count]), neutral,
                                          err_msg="Inactive output changed from v0.39")
            isolated += 1
        if row["clip"] not in seen and row["preset"] == "Neutral / Clean Slate":
            seen.add(row["clip"])
            images, responses = [], {}
            base_y = luminance(current)
            for label, values in VARIANTS.items():
                rendered = new.render(source, tuned(new, settings, values))
                delta = luminance(rendered)-base_y
                responses[label] = {"max_linear_y_delta": float(np.abs(delta).max()),
                                    "rms_linear_y_delta": float(np.sqrt(np.mean(delta**2)))}
                image = display_preview(rendered[..., :3])
                image.thumbnail((480, 300))
                images.append((label, image))
            width, height = 480, 328
            canvas = Image.new("RGB", (width*3, height*3), "#202020")
            draw = ImageDraw.Draw(canvas)
            for i, (label, image) in enumerate(images):
                x, y = i % 3*width, i // 3*height
                draw.text((x+8, y+6), label, fill="white")
                canvas.paste(image, (x, y+28))
            path = output / f"{stem}-controls.png"
            canvas.save(path, icc_profile=ICC)
            previews.append({"clip": row["clip"], "preview": path.name, "responses": responses})
        print(f"Checked {row['clip']} / {row['seconds']}s / {row['preset']}", flush=True)
    if not comparisons or len(seen) != 5:
        raise ValueError("Expected coverage of all five local clips")
    report = {"reference_bridge_sha256": hashlib.sha256(args.reference_bridge.read_bytes()).hexdigest(),
              "bridge_sha256": hashlib.sha256(args.bridge.read_bytes()).hexdigest(),
              "bit_exact_default_frames": comparisons, "bit_exact_inactive_pairs": isolated,
              "stage_ramps": stages, "previews": previews}
    (output / "results.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(f"PASS: {comparisons} exact v0.39 frames, {isolated} exact inactive pairs, {len(stages)} stage ramps, five footage previews.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference-bridge", type=Path, required=True)
    parser.add_argument("--bridge", type=Path, default=ROOT / "build/ofx/bench/ColorBench.dll")
    parser.add_argument("--footage", type=Path, default=ROOT / "analysis/color-bench-v035")
    parser.add_argument("--output", type=Path, default=ROOT / "analysis/sdr-viewing-v040")
    run(parser.parse_args())
