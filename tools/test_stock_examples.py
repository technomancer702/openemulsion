# SPDX-License-Identifier: MPL-2.0
"""Stock-preview policies and optional production OpenCL integration checks."""

from __future__ import annotations

import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

import render_stock_examples as stock
import color_bench as bench
import numpy as np
from PIL import Image

GPU = "--gpu" in sys.argv
if GPU:
    sys.argv.remove("--gpu")


class StockExamplesTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.renderer = bench.Renderer(stock.ROOT / "build/ofx/bench/ColorBench.dll")

    def test_creative_settings_use_display_ready_color_only(self):
        for slug in ("folk-dread", "archive-thriller"):
            settings = stock.settings_for_example(self.renderer, slug)
            self.assertEqual(settings[0], 1)
            self.assertEqual(settings[26], 5)
            self.assertEqual(settings[27], 1)
            self.assertEqual(settings[self.renderer.dll.oe_rendering_index()], 1)
            self.assertFalse(int(settings[bench.MASK]) & (stock.HALATION | stock.AURA | stock.GRAIN | stock.BLOOM))
            np.testing.assert_array_equal(settings[[4, 5, 6]], [1, 1, 1])

    def test_halation_is_isolated_and_disclosed(self):
        settings = stock.settings_for_example(self.renderer, "halation")
        self.assertEqual(settings[0], 2)
        self.assertEqual(settings[bench.MASK], stock.HALATION)
        np.testing.assert_allclose(settings[[13, 14, 15, 16, 45, 46, 47, 48]],
                                   [.45, 1, 0, 0, 0, .65, .25, .2])

    def test_shot_adjustments_are_isolated_to_archive_example(self):
        for slug in ("folk-dread", "archive-thriller"):
            baseline = self.renderer.settings(stock.EXAMPLES[slug][1], stock.REC709_GAMMA24,
                                              stock.CONVERSION_ONLY)
            expected = baseline.copy()
            if slug == "archive-thriller":
                for index, value in stock.ARCHIVE_ADJUSTMENTS.items():
                    expected[index] = value
                self.assertFalse(np.array_equal(expected, baseline))
                self.assertEqual(expected[2], 3)
            np.testing.assert_array_equal(stock.settings_for_example(self.renderer, slug), expected)
            np.testing.assert_array_equal(self.renderer.settings(stock.EXAMPLES[slug][1],
                                          stock.REC709_GAMMA24, stock.CONVERSION_ONLY), baseline)

    def test_sources_are_explicit_and_exclude_gas_station(self):
        for filename, _, url, credit in stock.CLIPS.values():
            self.assertTrue(filename.endswith(".mp4"))
            self.assertNotIn("gas", filename.lower())
            self.assertTrue(url.startswith("https://"))
            self.assertTrue(credit)

    def test_comparison_layout_and_labels(self):
        image = Image.new("RGB", (1920, 1080), "white")
        pair = stock.comparison(image, image, "Original", "Preset")
        self.assertEqual(pair.size, (1920, 596))
        self.assertEqual(pair.getpixel((0, 56)), (255, 255, 255))
        with self.assertRaises(ValueError):
            stock.comparison(image, Image.new("RGB", (100, 100)), "A", "B")
        with self.assertRaises(ValueError):
            stock.comparison(image, image, "Label " * 100, "B")

    def test_decoder_rejects_unknown_or_hdr_tags(self):
        metadata = {"range_tag": 1, "matrix_tag": 1, "transfer_tag": 1, "primaries_tag": 1, "duration": 1}
        for key in ("range_tag", "matrix_tag", "transfer_tag", "primaries_tag"):
            with patch.object(bench, "probe", return_value={**metadata, key: 0}):
                with self.assertRaises(ValueError):
                    stock.decode_stock(Path("unused.mp4"), 0)
        with patch.object(bench, "probe", return_value=metadata):
            for time in (-1, 1, 2):
                with self.assertRaises(ValueError):
                    stock.decode_stock(Path("unused.mp4"), time)

    def test_stock_decoder_preserves_black_white_and_dimensions(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "levels.mp4"
            with bench.av.open(str(path), "w") as container:
                stream = container.add_stream("libx264", rate=24)
                stream.width, stream.height, stream.pix_fmt = 48, 16, "yuv420p"
                stream.options = {"crf": "0"}
                context = stream.codec_context
                context.color_range = context.colorspace = context.color_trc = context.color_primaries = 1
                frame = bench.av.VideoFrame(48, 16, "yuv420p")
                frame.color_range = frame.colorspace = 1
                for i, plane in enumerate(frame.planes):
                    data = np.ndarray((plane.height, plane.width), dtype=np.uint8, buffer=plane,
                                      strides=(plane.line_size, 1))
                    data[:] = 128 if i else 16
                    if i == 0:
                        data[:, 24:] = 235
                for packet in stream.encode(frame):
                    container.mux(packet)
                for packet in stream.encode():
                    container.mux(packet)
            decoded, time = stock.decode_stock(path, 0)
            self.assertEqual(decoded.shape, (16, 48, 4))
            self.assertEqual(time, 0)
            np.testing.assert_allclose(decoded[:, :16, :3], 0, atol=2e-4)
            np.testing.assert_allclose(decoded[:, 32:, :3], 1, atol=2e-4)
            np.testing.assert_array_equal(decoded[..., 3], 1)


@unittest.skipUnless(GPU, "Use --gpu for production OpenCL integration checks")
class FullFrameTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.renderer = stock.FullRenderer(stock.ROOT / "build/ofx/bench/ColorBench.dll",
                                          stock.ROOT / "build/ofx/bench/FullFrameBench.exe")
        rng = np.random.default_rng(41)
        cls.frame = np.ascontiguousarray(rng.uniform(.01, .9, (33, 65, 4)), dtype=np.float32)

    def test_gpu_color_only_matches_native_cpu(self):
        for slug in ("folk-dread", "archive-thriller"):
            settings = stock.settings_for_example(self.renderer, slug)
            cpu = bench.Renderer.render(self.renderer, self.frame, settings)
            gpu = self.renderer.render(self.frame, settings)
            np.testing.assert_allclose(gpu, cpu, atol=3e-5, rtol=3e-5)

    def test_halation_off_identity_on_visible_and_alpha_preserved(self):
        settings = stock.settings_for_example(self.renderer, "halation")
        frame = self.frame.copy()
        frame[..., :3] = .02
        frame[12:21, 28:37, :3] = 1
        off = settings.copy(); off[13] = 0
        np.testing.assert_array_equal(self.renderer.render(frame, off), frame)
        rendered = self.renderer.render(frame, settings)
        self.assertGreater(float(np.max(np.abs(rendered[..., :3] - frame[..., :3]))), .01)
        self.assertGreater(float(rendered[11, 32, 0]), float(frame[11, 32, 0]))
        np.testing.assert_array_equal(rendered[..., 3], frame[..., 3])
        np.testing.assert_array_equal(self.renderer.render(frame, off), frame)

    def test_invalid_data_and_dimensions_fail_explicitly(self):
        settings = stock.settings_for_example(self.renderer, "halation")
        with self.assertRaises(ValueError):
            self.renderer.render(self.frame.astype(np.float64), settings)
        with self.assertRaises(ValueError):
            self.renderer.render(self.frame, settings, time=float("nan"))
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder)
            self.frame.tofile(path / "input.f32")
            settings.tofile(path / "settings.f32")
            for width in (0, 16385, 64):
                result = subprocess.run([str(self.renderer.executable), str(path / "input.f32"),
                                         str(path / "output.f32"), str(width), "33", str(path / "settings.f32"), "0"],
                                        capture_output=True, text=True, timeout=120)
                self.assertNotEqual(result.returncode, 0)
                self.assertFalse((path / "output.f32").exists())


if __name__ == "__main__":
    unittest.main()
