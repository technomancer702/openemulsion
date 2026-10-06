# SPDX-License-Identifier: MPL-2.0

from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageDraw


ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "analysis" / "input"
W, H = 1920, 1080


def srgb(v: float) -> int:
    return max(0, min(255, int(round(v * 255.0))))


def save(img: Image.Image, name: str) -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    img.save(OUT / name)
    print(OUT / name)


def make_neutral_ramp() -> None:
    img = Image.new("RGB", (W, H))
    px = img.load()
    for x in range(W):
        v = x / (W - 1)
        c = srgb(v)
        for y in range(H):
            px[x, y] = (c, c, c)
    save(img, "neutral_ramp.png")


def make_rgb_ramps() -> None:
    img = Image.new("RGB", (W, H), (0, 0, 0))
    px = img.load()
    bands = [(1, 0, 0), (0, 1, 0), (0, 0, 1), (1, 1, 1)]
    band_h = H // len(bands)
    for i, mask in enumerate(bands):
        y0 = i * band_h
        y1 = H if i == len(bands) - 1 else (i + 1) * band_h
        for x in range(W):
            v = srgb(x / (W - 1))
            color = (v * mask[0], v * mask[1], v * mask[2])
            for y in range(y0, y1):
                px[x, y] = color
    save(img, "rgb_ramps.png")


def make_color_chips() -> None:
    colors = [
        (0.90, 0.90, 0.90), (0.18, 0.18, 0.18), (0.90, 0.08, 0.06), (0.05, 0.65, 0.18),
        (0.05, 0.20, 0.90), (0.95, 0.85, 0.05), (0.85, 0.08, 0.80), (0.04, 0.78, 0.80),
        (0.70, 0.35, 0.18), (0.95, 0.55, 0.25), (0.25, 0.12, 0.06), (0.12, 0.08, 0.04),
        (0.95, 0.72, 0.62), (0.76, 0.50, 0.39), (0.55, 0.34, 0.27), (0.34, 0.22, 0.18),
        (0.03, 0.03, 0.03), (0.08, 0.08, 0.08), (0.18, 0.18, 0.18), (0.35, 0.35, 0.35),
        (0.55, 0.55, 0.55), (0.72, 0.72, 0.72), (0.86, 0.86, 0.86), (1.00, 1.00, 1.00),
    ]
    img = Image.new("RGB", (W, H), (26, 26, 26))
    draw = ImageDraw.Draw(img)
    cols, rows = 6, 4
    margin_x, margin_y = 120, 120
    gap = 22
    chip_w = (W - 2 * margin_x - (cols - 1) * gap) // cols
    chip_h = (H - 2 * margin_y - (rows - 1) * gap) // rows
    for i, color in enumerate(colors):
        col, row = i % cols, i // cols
        x0 = margin_x + col * (chip_w + gap)
        y0 = margin_y + row * (chip_h + gap)
        x1 = x0 + chip_w
        y1 = y0 + chip_h
        rgb = tuple(srgb(c) for c in color)
        draw.rectangle((x0, y0, x1, y1), fill=rgb)
    save(img, "color_chips.png")


def make_halation_edges() -> None:
    img = Image.new("RGB", (W, H), (0, 0, 0))
    draw = ImageDraw.Draw(img)
    draw.rectangle((120, 120, W - 120, H - 120), fill=(8, 8, 8))
    draw.rectangle((260, 250, 820, 830), fill=(255, 255, 255))
    draw.rectangle((1060, 250, 1620, 830), fill=(255, 235, 195))
    for x in range(0, W, 120):
        draw.line((x, 0, x, H), fill=(32, 32, 32), width=1)
    for y in range(0, H, 120):
        draw.line((0, y, W, y), fill=(32, 32, 32), width=1)
    save(img, "halation_edges.png")


def make_grain_flats() -> None:
    img = Image.new("RGB", (W, H), (0, 0, 0))
    draw = ImageDraw.Draw(img)
    values = [0.05, 0.10, 0.18, 0.35, 0.55, 0.75, 0.90]
    strip_w = W // len(values)
    for i, v in enumerate(values):
        x0 = i * strip_w
        x1 = W if i == len(values) - 1 else (i + 1) * strip_w
        c = srgb(v)
        draw.rectangle((x0, 0, x1, H), fill=(c, c, c))
    save(img, "grain_flats.png")


def main() -> None:
    make_neutral_ramp()
    make_rgb_ramps()
    make_color_chips()
    make_halation_edges()
    make_grain_flats()


if __name__ == "__main__":
    main()
