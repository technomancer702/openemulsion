# SPDX-License-Identifier: MPL-2.0
"""Optional bench checks; run after building ColorBench (no footage required)."""
import unittest

import numpy as np
from color_bench import ROOT, Renderer, av, content_mask, display_preview, metrics, rgba, roi_mask, unpack_yuv444


class BenchTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.renderer = Renderer(ROOT / "build/ofx/bench/ColorBench.dll")

    def test_gray_anchors_and_alpha(self):
        frame = rgba(np.array([[[.18, .18, .18], [1, 1, 1], [0, 0, 0]]], np.float32))
        frame[..., 3] = [.31, .63, .89]
        s = self.renderer.settings("Neutral / Clean Slate", 14)
        actual = self.renderer.render(frame, s)
        np.testing.assert_allclose(actual[0, 0, :3], .12 ** (1 / 2.4), atol=2e-6)
        np.testing.assert_allclose(actual[0, 1, :3], .69135703 ** (1 / 2.4), atol=2e-6)
        np.testing.assert_array_equal(actual[0, 2, :3], 0)
        np.testing.assert_array_equal(frame[..., 3], actual[..., 3])
        converted = self.renderer.render(frame, self.renderer.settings("Neutral / Clean Slate", 14, 1))
        np.testing.assert_allclose(converted[0, 0, :3], .18 ** (1 / 2.4), atol=2e-6)

    def test_candidate_preserves_grays_and_darker_colors(self):
        frame = rgba(np.array([[[.38, .2, .12], [0, 0, 0], [.18, .18, .18], [4, 4, 4]]], np.float32))
        s = self.renderer.settings("Neutral / Clean Slate", 14)
        a = self.renderer.render(frame, s)
        b = self.renderer.render(frame, s, 1)
        np.testing.assert_allclose(a, b, atol=2e-6)
        np.testing.assert_array_equal(a, self.renderer.render(frame, s, 0))
        s[self.renderer.dll.oe_rendering_index()] = 1
        np.testing.assert_array_equal(self.renderer.render(frame, s), self.renderer.render(frame, s, 1))

    def test_logc3_reference_black_and_gray(self):
        frame = rgba(np.array([[[.092809] * 3, [.39100683] * 3]], np.float32))
        linear = self.renderer.linear(frame, 0)
        np.testing.assert_allclose(linear[0, 0, :3], 0, atol=1e-7)
        np.testing.assert_allclose(linear[0, 1, :3], .18, atol=2e-6)
        actual = self.renderer.render(frame, self.renderer.settings("Neutral / Clean Slate", 0))
        np.testing.assert_allclose(actual[0, 1, :3], .12 ** (1 / 2.4), atol=2e-6)

    def test_display_input_not_double_rendered(self):
        frame = rgba(np.array([[[.3, .5, .7]]], np.float32))
        np.testing.assert_array_equal(self.renderer.render(frame, self.renderer.settings("Neutral / Clean Slate", 5)),
                                      self.renderer.render(frame, self.renderer.settings("Neutral / Clean Slate", 5, 1)))

    def test_production_retention_matches_restrained_experiment(self):
        frame = rgba(np.array([[[32, .1, .3], [.01, .03, 32], [.38, .2, .12], [4, 4, 4]]], np.float32))
        s = self.renderer.settings("Neutral / Clean Slate", 14)
        expected = self.renderer.render(frame, s, .35)
        actual = self.renderer.render(frame, self.renderer.settings("Neutral / Clean Slate", 14, retention=1))
        np.testing.assert_allclose(actual, expected, atol=2e-6)

    def test_all_recipes_are_finite(self):
        frame = rgba(np.array([[[.18, .18, .18], [4, -.05, .25], [.01, .03, 4], [0, 0, 0]]], np.float32))
        for name in self.renderer.presets:
            for amount in [0, .35, 1]:
                self.renderer.render(frame, self.renderer.settings(name, 14), amount)

    def test_threaded_path_matches_small_render(self):
        pixel = rgba(np.array([[[4, .05, .25]]], np.float32))
        frame = np.tile(pixel, (192, 192, 1))
        s = self.renderer.settings("50D Daylight", 14)
        expected = np.tile(self.renderer.render(pixel, s, .35), (192, 192, 1))
        np.testing.assert_array_equal(self.renderer.render(frame, s, .35), expected)

    def test_invalid_arrays_rejected(self):
        s = self.renderer.settings("Neutral / Clean Slate", 14)
        for bad in [np.zeros((2, 2, 3), np.float32), np.zeros((0, 2, 4), np.float32), np.full((2, 2, 4), np.nan, np.float32)]:
            with self.assertRaises(ValueError):
                self.renderer.render(bad, s)
        frame = rgba(np.zeros((2, 2, 3), np.float32))
        for blend in [-1, 2, float("nan")]:
            with self.assertRaises(ValueError):
                self.renderer.render(frame, s, blend)
        for index in [0, 1, 2, 19, 26, 27, self.renderer.dll.oe_rendering_index()]:
            bad = s.copy()
            bad[index] = 1e30
            with self.assertRaises(ValueError):
                self.renderer.render(frame, bad)

    def test_float_channel_order_and_precision(self):
        values = np.zeros((8, 8, 3), np.float32)
        values[..., 0], values[..., 1], values[..., 2] = .123456, .654321, .234567
        frame = av.VideoFrame.from_ndarray(values, format="gbrpf32le", channel_last=True)
        actual = frame.to_ndarray(format="gbrpf32le", channel_last=True)
        np.testing.assert_array_equal(actual, values)
        self.assertGreater(abs(float(actual[0, 0, 0]) * 255 - round(float(actual[0, 0, 0]) * 255)), .1)

    def test_yuv_legal_range_to_float(self):
        frame = av.VideoFrame(8, 8, "yuv444p12le")
        for plane, value in zip(frame.planes, [256, 2048, 2048]):
            data = np.full((plane.height, plane.line_size // 2), value, np.uint16)
            plane.update(data.tobytes())
        black = unpack_yuv444(frame, "ITU709", "MPEG")
        np.testing.assert_allclose(black, 0, atol=1e-5)
        plane = frame.planes[0]
        plane.update(np.full((plane.height, plane.line_size // 2), 3760, np.uint16).tobytes())
        white = unpack_yuv444(frame, "ITU709", "MPEG")
        np.testing.assert_allclose(white, 1, atol=1e-5)
        plane.update(np.full((plane.height, plane.line_size // 2), 4000, np.uint16).tobytes())
        self.assertGreater(float(unpack_yuv444(frame, "ITU709", "MPEG").min()), 1)
        # A one-code 12-bit step survives decoding; values are not reduced to 8 bits.
        plane.update(np.full((plane.height, plane.line_size // 2), 256 + 1, np.uint16).tobytes())
        np.testing.assert_allclose(unpack_yuv444(frame, "ITU709", "MPEG"), 1 / 3504, atol=1e-7)

    def test_masks_previews_and_float_metrics(self):
        self.assertTrue(content_mask(100, 100).all())
        self.assertFalse(content_mask(100, 100, [[0, .7, .3, 1]])[99, 0])
        with self.assertRaises(ValueError):
            content_mask(2, 2, [[0, 0, 1, 1]])
        with self.assertRaises(ValueError):
            roi_mask(10, 10, [-.1, 0, 1, 1])
        rgb = np.full((2, 2, 3), .5, np.float32)
        self.assertEqual(np.asarray(display_preview(rgb))[0, 0, 0], 120)
        rgb[0, 0] = [1.2, -.1, .99]
        result = metrics(rgb, content_mask(2, 2))
        self.assertGreater(result["maximum"], 1)
        self.assertGreater(result["channel_outside_0_1_pct"], 0)
        self.assertEqual(result["all_channels_above_097_pct"], 0)
        self.assertLessEqual(result["mean_code_saturation"], 1)
        rgb[0, 0] = [-.1, -.2, -.3]
        self.assertLessEqual(metrics(rgb, content_mask(2, 2))["mean_code_saturation"], 1)

    def test_yuv_red_channel_order_and_full_range(self):
        frame = av.VideoFrame(8, 8, "yuv444p12le")
        y = .2126
        codes = [round((16 + 219 * y) * 16), round(2048 - y / 1.8556 * 3584), 3840]
        for plane, value in zip(frame.planes, codes):
            plane.update(np.full((plane.height, plane.line_size // 2), value, np.uint16).tobytes())
        np.testing.assert_allclose(unpack_yuv444(frame, "ITU709", "MPEG"),
                                   np.broadcast_to([1, 0, 0], (8, 8, 3)), atol=5e-4)
        for y in [0, 4095]:
            for plane, value in zip(frame.planes, [y, 2048, 2048]):
                plane.update(np.full((plane.height, plane.line_size // 2), value, np.uint16).tobytes())
            np.testing.assert_allclose(unpack_yuv444(frame, "ITU709", "JPEG"), y / 4095, atol=1e-6)

    def test_subsampled_formats_rejected(self):
        with self.assertRaises(ValueError):
            unpack_yuv444(av.VideoFrame(8, 8, "yuv422p10le"), "ITU709", "MPEG")


if __name__ == "__main__":
    unittest.main()
