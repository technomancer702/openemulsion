# SPDX-License-Identifier: MPL-2.0
"""Native HDR controls audit; numeric results only, not an SDR preview of PQ."""

import argparse
import hashlib
import json
import subprocess
from pathlib import Path

import numpy as np

from check_hdr_footage import pq_nits
from color_bench import ROOT, Renderer
from inspect_lens_detail import source_pairs


VARIANTS = {"Default": (0, 0), "Darker trim": (-1, 0), "Brighter trim": (1, 0),
            "Less rolloff": (0, -1), "More rolloff": (0, 1)}
PEAKS = [400, 1000, 4000]
PRESETS = ["Neutral / Clean Slate", "50D Daylight"]


def hdr_settings(renderer, label, source, peak):
    settings = renderer.settings(label, source, renderer.dll.oe_hdr_rendering_index())
    settings[27] = renderer.dll.oe_hdr_output_index()
    settings[renderer.dll.oe_hdr_peak_index()] = peak
    settings[renderer.dll.oe_hdr_white_index()] = 203
    return settings


def measure(rendered, peak, pairs=None):
    nits = pq_nits(rendered[..., :3])
    if not np.isfinite(nits).all() or nits.min() < 0 or nits.max() > peak+.25:
        raise ValueError("HDR decoded channel peak bound failed")
    y = nits @ np.array([.2627, .6780, .0593])
    result = {"maximum_channel_nits": float(nits.max()),
              "luminance_percentiles_nits": np.percentile(y, [1, 50, 90, 99, 100]).tolist()}
    if pairs is not None:
        a, b = y[:, :-4][pairs], y[:, 4:][pairs]
        contrast = np.abs(b-a)/np.maximum((a+b)*.5, 1e-9)
        result["source_keyed_pairs"] = len(a)
        if len(a):
            result["relative_luminance_contrast_percentiles"] = np.percentile(contrast, [10, 50, 90]).tolist()
    return result


def run(args):
    old, new = Renderer(args.reference_bridge), Renderer(args.bridge)
    if new.count != old.count+2 or set(new.hdr_indices) != {"exposure", "rolloff"}:
        raise ValueError("Expected v0.41 reference and two appended HDR controls")
    manifest = json.loads((args.footage / "manifest.json").read_text(encoding="utf-8"))
    rows = json.loads((args.footage / "results.json").read_text(encoding="utf-8"))
    inputs = {row["float_output"].split("-preset")[0]: row for row in rows}
    source_id = manifest["source_space"]
    exact, isolated, records, lenses, seen, source_hashes = 0, 0, [], [], set(), {}
    for stem, row in inputs.items():
        source = np.load(args.footage / f"{stem}-input.npy")
        source_hashes[stem] = hashlib.sha256(source.tobytes()).hexdigest()
        first = row["clip"] not in seen
        for label in PRESETS:
            for peak in PEAKS:
                settings = hdr_settings(new, label, source_id, peak)
                baseline = new.render(source, settings)
                np.testing.assert_array_equal(old.render(source, settings[:old.count]), baseline,
                                              err_msg="Zero HDR controls change v0.41 footage")
                exact += 1
                records.append({"clip": row["clip"], "seconds": row["seconds"], "preset": label,
                                "peak_nits": peak, "variant": "Default", **measure(baseline, peak)})
                if first:
                    for variant, (ev, rolloff) in VARIANTS.items():
                        if variant == "Default":
                            continue
                        changed = settings.copy()
                        changed[new.hdr_indices["exposure"]] = ev
                        changed[new.hdr_indices["rolloff"]] = rolloff
                        rendered = new.render(source, changed)
                        records.append({"clip": row["clip"], "seconds": row["seconds"], "preset": label,
                                        "peak_nits": peak, "variant": variant, **measure(rendered, peak)})
            for rendering, output in [(0, 1), (1, new.dll.oe_hdr_output_index()), (0, 2)]:
                settings = new.settings(label, source_id, rendering)
                settings[27] = output
                baseline = new.render(source, settings)
                np.testing.assert_array_equal(old.render(source, settings[:old.count]), baseline,
                                              err_msg="Inactive output changes v0.41 footage")
                settings[new.hdr_indices["exposure"]] = 4
                settings[new.hdr_indices["rolloff"]] = 1
                np.testing.assert_array_equal(baseline, new.render(source, settings),
                                              err_msg="HDR controls alter inactive output")
                isolated += 1
        regions = manifest.get("regions_and_exclusions", {}).get(row["clip"], {}).get("regions", {})
        for name, box in regions.items():
            if "taillight" not in name.lower():
                continue
            h, w = source.shape[:2]
            x0, y0, x1, y1 = box
            crop = np.ascontiguousarray(source[int(y0*h):int(y1*h), int(x0*w):int(x1*w)])
            pairs = source_pairs(new.linear(crop, source_id))
            for label in PRESETS:
                for peak in PEAKS:
                    for variant, (ev, rolloff) in VARIANTS.items():
                        settings = hdr_settings(new, label, source_id, peak)
                        settings[new.hdr_indices["exposure"]] = ev
                        settings[new.hdr_indices["rolloff"]] = rolloff
                        lenses.append({"clip": row["clip"], "seconds": row["seconds"], "region": name,
                                       "preset": label, "peak_nits": peak, "variant": variant,
                                       **measure(new.render(crop, settings), peak, pairs)})
        seen.add(row["clip"])
        print(f"Checked {row['clip']} @ {row['seconds']}s", flush=True)
    if not exact or len(seen) != 5:
        raise ValueError("Expected five retained clips")
    report = {"reference_bridge_sha256": hashlib.sha256(args.reference_bridge.read_bytes()).hexdigest(),
              "bridge_sha256": hashlib.sha256(args.bridge.read_bytes()).hexdigest(),
              "source_space": source_id, "footage": str(args.footage.resolve()),
              "source_sha256": source_hashes,
              "revision": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
              "working_tree_status": subprocess.check_output(["git", "status", "--short"], cwd=ROOT, text=True),
              "bit_exact_default_hdr_frames": exact, "bit_exact_inactive_pairs": isolated,
              "records": records, "lens_records": lenses,
              "note": "Color Only, retained float inputs, 203-nit white. Whole-frame percentiles include burn-ins. "
                      "Fixed source-keyed four-pixel pairs at full retained resolution are diagnostic, not perceptual "
                      "ground truth, clipping recovery, an HDR-monitor evaluation or a SpektraFilm A/B."}
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / "results.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(f"PASS: {exact} exact HDR defaults, {isolated} exact inactive pairs, "
          f"{len(records)} frame measurements, {len(lenses)} taillight-region measurements.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference-bridge", type=Path, required=True)
    parser.add_argument("--bridge", type=Path, default=ROOT / "build/ofx/bench/ColorBench.dll")
    parser.add_argument("--footage", type=Path, default=ROOT / "analysis/color-bench-v035")
    parser.add_argument("--output", type=Path, default=ROOT / "analysis/hdr-viewing-v042")
    run(parser.parse_args())
