# SPDX-License-Identifier: MPL-2.0
"""Check zero-retention rendering against a previous local bench's float outputs."""
import argparse
import json
from pathlib import Path

import numpy as np
from color_bench import ROOT, Renderer


def check(reference: Path, renderer: Renderer) -> int:
    records = json.loads((reference / "results.json").read_text(encoding="utf-8"))
    checked = 0
    for row in records:
        if row["model"] not in ["Production SDR", "Conversion Only"]:
            continue
        settings = np.array(row["settings"], dtype=np.float32)
        if settings.size in [renderer.count - 1,renderer.count - 2,renderer.count - 3]:
            settings = np.pad(settings, (0,renderer.count-settings.size))
        if settings.size != renderer.count:
            raise ValueError("Unsupported reference settings layout")
        settings[renderer.dll.oe_retention_index()] = 0
        base = row["float_output"].split("-preset")[0]
        source = np.load(reference / (base + "-input.npy"))
        expected = np.load(reference / row["float_output"])
        actual = renderer.render(source, settings)
        if not np.array_equal(actual, expected):
            raise ValueError(f"Zero-retention output changed: {row['float_output']}; max delta {np.max(np.abs(actual-expected))}")
        checked += 1
    if not checked:
        raise ValueError("Reference has no baseline float outputs")
    print(f"PASS: {checked} original-footage baseline outputs are bit-exact at zero retention.")
    return checked


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path)
    parser.add_argument("--library", type=Path, default=ROOT / "build/ofx/bench/ColorBench.dll")
    args = parser.parse_args()
    check(args.reference, Renderer(args.library))
