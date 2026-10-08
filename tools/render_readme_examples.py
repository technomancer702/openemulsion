# SPDX-License-Identifier: MPL-2.0
"""Render color-only README comparisons with the native production color bench."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFont

import color_bench as bench


ROOT = Path(__file__).resolve().parents[1]
SAMPLES = [
    ("Skin Colour.mov", seconds) for seconds in (1, 3, 5, 7, 9, 11)
] + [
    ("interior hallway with window.mov", 2.5),
    ("Nigh Shot car tail lights.mov", 5.9),
    ("two men sitting in front of window.mov", 4.5),
]
EXAMPLES = [
    ("portrait", "Skin Colour.mov", 1, "Portra 400"),
    ("street", "Skin Colour.mov", 9, "Kodachrome 64"),
    ("night", "Nigh Shot car tail lights.mov", 5.9, "Classic Cinema"),
    ("print-2383", "interior hallway with window.mov", 2.5, "2383 Print"),
    ("daylight-50d", "Skin Colour.mov", 7, "50D Daylight"),
    ("selective-color", "Nigh Shot car tail lights.mov", 5.9, "Graphic Noir / Red (Sin City)"),
]
SELECTIVE_HUE = 70
SELECTIVE_RANGE = 71
SELECTIVE_FEATHER = 72
SELECTIVE_MINIMUM_SATURATION = 73


def settings_for_example(renderer: bench.Renderer, slug: str, preset: str) -> np.ndarray:
    settings = renderer.settings(preset, source=0)
    if slug == "selective-color":
        settings[SELECTIVE_HUE] = 355
        settings[SELECTIVE_RANGE] = 8
        settings[SELECTIVE_FEATHER] = 3
        settings[SELECTIVE_MINIMUM_SATURATION] = .8
    return settings


def font(size: int) -> ImageFont.FreeTypeFont:
    return ImageFont.truetype("C:/Windows/Fonts/segoeui.ttf", size)


def resize(image: Image.Image, width: int) -> Image.Image:
    return image.resize((width, round(image.height * width / image.width)), Image.Resampling.LANCZOS)


def log_preview(frame: np.ndarray) -> Image.Image:
    bench.Renderer.validate_frame(frame)
    # Show camera-log code values directly, without a viewing transform.
    return Image.fromarray(np.rint(np.clip(frame[..., :3], 0, 1) * 255).astype(np.uint8))


def native_frame(renderer: bench.Renderer, path: Path, seconds: float, preset: str):
    frame, timestamp = bench.decode(path, seconds, "ITU709", "auto")
    settings = renderer.settings(preset, source=0)
    output = renderer.render(frame, settings)
    return frame, timestamp, settings, output


def contact_sheet(renderer: bench.Renderer, footage: Path, destination: Path) -> None:
    width, header, columns = 400, 38, 3
    tiles = []
    for name, seconds in SAMPLES:
        _, timestamp, _, output = native_frame(renderer, footage / name, seconds, "Neutral / Clean Slate")
        image = resize(bench.display_preview(output[..., :3]), width)
        tile = Image.new("RGB", (width, image.height + header), "#181818")
        tile.paste(image, (0, header))
        ImageDraw.Draw(tile).text((10, 9), f"{name} / {timestamp:.2f}s", font=font(15), fill="white")
        tiles.append(tile)
    cell_height = max(tile.height for tile in tiles)
    sheet = Image.new("RGB", (width * columns, cell_height * ((len(tiles) + columns - 1) // columns)), "#181818")
    for index, tile in enumerate(tiles):
        sheet.paste(tile, ((index % columns) * width, (index // columns) * cell_height))
    destination.mkdir(parents=True, exist_ok=True)
    sheet.save(destination / "contact-sheet.jpg", quality=94, subsampling=0, icc_profile=bench.ICC)


def examples(renderer: bench.Renderer, footage: Path, destination: Path, only: list[str] | None = None) -> None:
    destination.mkdir(parents=True, exist_ok=True)
    records = []
    for slug, name, seconds, preset in EXAMPLES:
        if only and slug not in only:
            continue
        frame, timestamp = bench.decode(footage / name, seconds, "ITU709", "auto")
        after_settings = settings_for_example(renderer, slug, preset)
        if not np.array_equal(after_settings[[4, 5, 6]], [1, 1, 1]):
            raise ValueError("Comparison changes camera exposure or balance")
        after = renderer.render(frame, after_settings)
        before_image = log_preview(frame)
        after_image = bench.display_preview(after[..., :3])
        difference = float(np.mean(np.abs(np.asarray(after_image, dtype=np.float32) -
                                          np.asarray(before_image, dtype=np.float32))) / 255)
        if difference < .005:
            raise ValueError(f"Comparison is indistinguishable: {name} / {preset}")
        left = resize(before_image, 960)
        right = resize(after_image, 960)
        header = 56
        comparison = Image.new("RGB", (1920, left.height + header), "#181818")
        comparison.paste(left, (0, header))
        comparison.paste(right, (960, header))
        draw = ImageDraw.Draw(comparison)
        draw.text((20, 13), "Before / Ungraded LogC3", font=font(23), fill="white")
        label = f"{preset} / tuned key" if slug == "selective-color" else preset
        draw.text((980, 13), f"After / {label}", font=font(23), fill="white")
        path = destination / f"{slug}.jpg"
        comparison.save(path, quality=94, subsampling=0, icc_profile=bench.ICC)
        records.append({"image": path.name, "clip": name, "seconds": timestamp, "preset": preset,
                        "mean_preview_difference": difference, "before": "Untreated LogC3 code values",
                        "after_settings": after_settings.tolist()})
        print(f"Saved {path.name}: {name} @ {timestamp:.3f}s, {preset}")
    # Rendering settings stay local; only the selected comparison JPEGs are published.
    manifest = {"source": "LogC3 / ARRI Wide Gamut 3 (EI 800)", "output": "Rec.709 / Gamma 2.4",
                "presentation": "Before: direct LogC3 codes. After: SDR converted to sRGB. Full frame, unchanged camera balance; color-only, no grain or diffusion.",
                "bridge_sha256": hashlib.sha256((ROOT / "build/ofx/bench/ColorBench.dll").read_bytes()).hexdigest(),
                "examples": records}
    report = ROOT / "analysis/readme-examples"
    report.mkdir(parents=True, exist_ok=True)
    (report / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--contact-only", action="store_true")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--only", nargs="+", choices=[example[0] for example in EXAMPLES])
    args = parser.parse_args()
    renderer = bench.Renderer(ROOT / "build/ofx/bench/ColorBench.dll")
    footage = ROOT / "test footage"
    if args.contact_only:
        contact_sheet(renderer, footage, args.output or ROOT / "analysis/readme-examples")
    else:
        examples(renderer, footage, args.output or ROOT / "docs/media", args.only)


if __name__ == "__main__":
    main()
