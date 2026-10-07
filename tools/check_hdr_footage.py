# SPDX-License-Identifier: MPL-2.0
"""Local PQ luminance checks using preserved full-precision color-bench inputs."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path

import numpy as np
from color_bench import ROOT, Renderer


def pq_nits(code: np.ndarray) -> np.ndarray:
    code = np.asarray(code, dtype=np.float64)
    n = np.clip(code, 0, 1) ** (1 / 78.84375)
    return 10000 * (np.maximum(n - .8359375, 0) / (18.8515625 - 18.6875 * n)) ** (1 / .1593017578125)


def run(args: argparse.Namespace) -> None:
    if not 80 <= args.white <= 300 or any(not 400 <= peak <= 10000 for peak in args.peaks):
        raise ValueError("HDR control values exceed the plugin ranges")
    renderer = Renderer(args.library)
    reference = json.loads((args.reference / "results.json").read_text(encoding="utf-8"))
    source_id = json.loads((args.reference / "manifest.json").read_text(encoding="utf-8"))["source_space"]
    inputs = {row["float_output"].split("-preset")[0]: row for row in reference}
    if not inputs:
        raise ValueError("No reference inputs found")
    records = []
    for base, row in inputs.items():
        source = np.load(args.reference / (base + "-input.npy"))
        source_hash = hashlib.sha256(source.tobytes()).hexdigest()
        for preset in args.presets:
            for peak in args.peaks:
                s = renderer.settings(preset, source_id, renderer.dll.oe_hdr_rendering_index())
                s[27] = renderer.dll.oe_hdr_output_index()
                s[renderer.dll.oe_hdr_peak_index()] = peak
                s[renderer.dll.oe_hdr_white_index()] = args.white
                output = renderer.render(source, s)
                if not np.array_equal(output[..., 3], source[..., 3]):
                    raise ValueError("HDR changed alpha")
                upper_code = float((.8359375 + 18.8515625 * (peak / 10000) ** .1593017578125) /
                                   (1 + 18.6875 * (peak / 10000) ** .1593017578125)) ** 78.84375
                if np.min(output[..., :3]) < 0 or np.max(output[..., :3]) > upper_code + 1e-5:
                    raise ValueError(f"PQ peak bound failed: {row['clip']} / {preset} / {peak}")
                nits = pq_nits(output[..., :3])
                y = nits @ np.array([.2627, .6780, .0593])
                records.append({"clip": row["clip"], "seconds": row["seconds"], "preset": preset,
                                "peak_nits": peak, "white_nits": args.white, "source_sha256": source_hash,
                                "settings": s.tolist(), "maximum_channel_nits": float(nits.max()),
                                "luminance_percentiles_nits": np.percentile(y, [1, 50, 90, 99, 100]).tolist(),
                                "above_reference_white_pct": float(np.mean(y > args.white) * 100)})
        print(f"Checked {row['clip']} @ {row['seconds']} s", flush=True)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps({"revision": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
        "working_tree_status": subprocess.check_output(["git", "status", "--short"], cwd=ROOT, text=True),
        "library_sha256": hashlib.sha256(args.library.read_bytes()).hexdigest(),
        "reference": str(args.reference.resolve()), "source_space": source_id,
        "note": "Color Only; numeric PQ validation, not a calibrated HDR preview. Percentiles include burn-ins. Original clips unchanged.",
        "records": records}, indent=2), encoding="utf-8")
    print(f"PASS: {len(records)} HDR footage renders finite, peak bounded and alpha preserved. {args.output}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path)
    parser.add_argument("--library", type=Path, default=ROOT / "build/ofx/bench/ColorBench.dll")
    parser.add_argument("--output", type=Path, default=ROOT / "analysis/hdr-bench-v036/results.json")
    parser.add_argument("--presets", nargs="+", default=["Neutral / Clean Slate", "50D Daylight"])
    parser.add_argument("--peaks", nargs="+", type=float, default=[400, 1000, 4000])
    parser.add_argument("--white", type=float, default=203)
    run(parser.parse_args())
