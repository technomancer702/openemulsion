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

    def test_sdr_controls_gray_double_reference(self):
        x = np.exp2(np.linspace(-16, 14, 2000))
        frame = rgba(np.repeat(x[None, :, None], 3, axis=2).astype(np.float32))
        indices = self.renderer.sdr_indices
        self.assertEqual(set(indices), {"contrast", "rolloff", "gamut"})
        for contrast in [-1, 0, 1]:
            for rolloff in [-1, 0, 1]:
                settings = self.renderer.settings("Neutral / Clean Slate", 14)
                settings[indices["contrast"]], settings[indices["rolloff"]] = contrast, rolloff
                slope = (13/15)*(1+.25*contrast)
                toe = 1.3*(1+.25*contrast)
                join = .6-.25*rolloff
                headroom = 1-(.12+slope*(join-.18))
                expected = np.empty_like(x)
                low, high = x <= .18, x > join
                mid = ~(low | high)
                expected[low] = .12*x[low]/(.18*toe-(toe-1)*x[low])
                expected[mid] = .12+slope*(x[mid]-.18)
                expected[high] = 1-headroom**2/(headroom+slope*(x[high]-join))
                actual = self.renderer.render(frame, settings)
                np.testing.assert_allclose(actual[0, :, 0], expected**(1/2.4), atol=3e-6)

    def test_sdr_controls_inactive_and_invalid(self):
        frame = rgba(np.array([[[.18, .18, .18], [32, .1, .3]]], np.float32))
        for rendering, output in [(1, 1), (3, self.renderer.dll.oe_hdr_output_index()), (0, 2)]:
            settings = self.renderer.settings("50D Daylight", 14, rendering)
            settings[27] = output
            baseline = self.renderer.render(frame, settings)
            for index in self.renderer.sdr_indices.values():
                settings[index] = 1
            np.testing.assert_array_equal(baseline, self.renderer.render(frame, settings))
        for index in self.renderer.sdr_indices.values():
            settings = self.renderer.settings("Neutral / Clean Slate", 14)
            settings[index] = 1.01
            with self.assertRaises(ValueError):
                self.renderer.render(frame, settings)

    def test_candidate_preserves_grays_and_darker_colors(self):
        frame = rgba(np.array([[[.38, .2, .12], [0, 0, 0], [.18, .18, .18], [4, 4, 4]]], np.float32))
        s = self.renderer.settings("Neutral / Clean Slate", 14)
        a = self.renderer.render(frame, s)
        b = self.renderer.render(frame, s, 1)
        np.testing.assert_allclose(a, b, atol=2e-6)
        np.testing.assert_array_equal(a, self.renderer.render(frame, s, 0))
        s[self.renderer.dll.oe_rendering_index()] = 1
        np.testing.assert_array_equal(self.renderer.render(frame, s), self.renderer.render(frame, s, 1))

    def test_hdr_viewing_double_reference(self):
        x = np.exp2(np.linspace(-16, 14, 2000))
        frame = rgba(np.repeat(x[None, :, None], 3, axis=2).astype(np.float32))
        self.assertEqual(set(self.renderer.hdr_indices), {"exposure", "rolloff"})
        for peak, white in [(400, 300), (1000, 203), (10000, 80)]:
            for ev in [-4, 0, 4]:
                for rolloff in [-1, 0, 1]:
                    settings = self.renderer.settings("Neutral / Clean Slate", 14, 3)
                    settings[27] = self.renderer.dll.oe_hdr_output_index()
                    settings[self.renderer.dll.oe_hdr_peak_index()] = peak
                    settings[self.renderer.dll.oe_hdr_white_index()] = white
                    settings[self.renderer.hdr_indices["exposure"]] = ev
                    settings[self.renderer.hdr_indices["rolloff"]] = rolloff
                    light = x*2**ev
                    expected = np.empty_like(light)
                    low, high = light <= .18, light > 1
                    mid = ~(low | high)
                    expected[low] = .12*light[low]/(.234-.30*light[low])
                    t = (light[mid]-.18)/.82
                    expected[mid] = .12+.88*t+.82*(13/15-.88/.82)*t*(1-t)**2
                    headroom = peak/white-1
                    u = (.88/.82)*(light[high]-1)/headroom
                    progress = u/(1+u)
                    expected[high] = 1+headroom*progress/(1+.9*rolloff*progress*(1-progress))
                    n = (expected*white/10000)**(2610/16384)
                    pq = ((3424/4096+(2413/128)*n)/(1+(2392/128)*n))**(2523/32)
                    actual = self.renderer.render(frame, settings)
                    np.testing.assert_allclose(actual[0, :, 0], pq, atol=1e-5)
                    np.testing.assert_allclose(actual[0, :, 0], actual[0, :, 1], atol=1e-6)

    def test_hdr_controls_inactive_and_invalid(self):
        frame = rgba(np.array([[[.18, .18, .18], [32, .1, .3]]], np.float32))
        for rendering, output in [(0, 1), (1, self.renderer.dll.oe_hdr_output_index()), (0, 2)]:
            settings = self.renderer.settings("50D Daylight", 14, rendering)
            settings[27] = output
            baseline = self.renderer.render(frame, settings)
            settings[self.renderer.hdr_indices["exposure"]] = 4
            settings[self.renderer.hdr_indices["rolloff"]] = 1
            np.testing.assert_array_equal(baseline, self.renderer.render(frame, settings))
        for control, value in [("exposure", 4.01), ("rolloff", -1.01)]:
            settings = self.renderer.settings("Neutral / Clean Slate", 14)
            settings[self.renderer.hdr_indices[control]] = value
            with self.assertRaises(ValueError):
                self.renderer.render(frame, settings)

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

    def test_emitter_shoulder_independent_double_reference(self):
        rgb = np.array([[[32, .1, .3], [.01, .03, 32], [.38, .2, .12], [4, 4, 4], [2, -.02, .2]]], np.float64)
        source = rgba(rgb.astype(np.float32))
        def tone(x):
            return np.where(x <= .18, .12*x/(.234-.3*x), np.where(x <= .6,
                .12+(13/15)*(x-.18), 1-.516**2/(.516+(13/15)*(x-.6))))
        def slope(x):
            return np.where(x <= .18, .02808/(.234-.3*x)**2, np.where(x <= .6,
                13/15, (13/15)*.516**2/(.516+(13/15)*(x-.6))**2))
        def gamut(c, mapped):
            distance = np.maximum((c.max(-1)-mapped)/np.maximum(1-mapped, 1e-7),
                                  (mapped-c.min(-1))/np.maximum(mapped, 1e-7))
            excess = np.maximum(distance-.8, 0)
            scale = np.where(distance > .8, (.8+.2*excess/(.2+excess))/np.maximum(distance, 1e-7), 1)
            return np.clip(mapped[..., None]+(c-mapped[..., None])*scale[..., None], 0, 1)
        peak, low = rgb.max(-1), rgb.min(-1)
        t = np.clip(((peak-low)/peak-.15)/.6,0,1)
        gate = t*t*(3-2*t)
        strength = 2*gate
        detail_peak = np.where((peak>1)&(gate>0),
            1+np.maximum(peak-1,0)/np.sqrt(1+strength*np.maximum(peak-1,0)),peak)
        rgb = rgb*(detail_peak/peak)[...,None]
        peak = detail_peak
        y = rgb @ np.array([.2126, .7152, .0722])
        mapped = tone(y)
        baseline = gamut(rgb*(mapped/y)[..., None], mapped)
        ratio = y/peak
        knee = tone(ratio)
        headroom = ratio-knee
        high = slope(ratio)*np.maximum(y-ratio, 0)
        retained_y = np.where(peak > 1, knee+headroom*high/(headroom+high), mapped)
        retained = gamut(rgb*(retained_y/y)[..., None], retained_y)
        for amount in [0, .25, .5, 1]:
            expected = (baseline+(retained-baseline)*((.5+.3*amount)*gate)[..., None])**(1/2.4)
            actual = self.renderer.render(source,
                self.renderer.settings("Neutral / Clean Slate", 14, retention=amount))
            np.testing.assert_allclose(actual[..., :3], expected, atol=3e-6)

    def test_emitter_detail_and_exposure_order(self):
        exposure = np.exp2(np.linspace(-14, 14, 2400)).astype(np.float32)
        for chip in [[1, .01, .025], [.01, 1, .02], [.01, .03, 1], [1, -.05, .25], [1, 1, .01]]:
            frame = rgba((exposure[:, None]*np.array(chip, np.float32))[None, ...])
            for amount in [0, .5, 1]:
                actual = self.renderer.render(frame, self.renderer.settings("Neutral / Clean Slate", 14, retention=amount))
                linear_y = actual[0, :, :3].astype(np.float64)**2.4 @ np.array([.2126, .7152, .0722])
                self.assertGreaterEqual(np.diff(linear_y).min(), -2e-6)
                self.assertTrue(((actual[..., :3] >= 0) & (actual[..., :3] <= 1)).all())
        frame = rgba(np.array([[[16, .05, .15], [32, .1, .3]]], np.float32))
        actual = self.renderer.render(frame, self.renderer.settings("Neutral / Clean Slate", 14))
        self.assertLess(actual[0, 1, 1], .8)
        self.assertGreater(actual[0, 1, 0], actual[0, 0, 0])

    def test_bright_emitter_intensity_contrast(self):
        for ray in [[1,.003125,.009375], [1,-.015,.02], [.01,.03,1], [1,1,.01]]:
            for peak in [8,16,32,64]:
                frame = rgba(np.array([[np.array(ray)*peak, np.array(ray)*peak*1.25]],np.float32))
                for amount in [0,1]:
                    rendered = self.renderer.render(frame,self.renderer.settings("Neutral / Clean Slate",14,retention=amount))
                    y = rendered[0,:,:3].astype(np.float64)**2.4 @ [.2126,.7152,.0722]
                    self.assertGreater((y[1]-y[0])/y.mean(),.0025 if ray[1]==1 else .01)

    def test_retention_keeps_low_intensity_and_pale_colors(self):
        frame = rgba(np.array([[[1, .01, .025], [.38, .2, .12], [8, 7.8, 7.6], [4, 4, 4]]], np.float32))
        baseline = self.renderer.render(frame, self.renderer.settings("Neutral / Clean Slate", 14))
        maximum = self.renderer.render(frame, self.renderer.settings("Neutral / Clean Slate", 14, retention=1))
        np.testing.assert_array_equal(baseline, maximum)

    def test_all_recipes_are_finite(self):
        frame = rgba(np.array([[[.18, .18, .18], [4, -.05, .25], [.01, .03, 4], [0, 0, 0]]], np.float32))
        for name in self.renderer.presets:
            for amount in [0, .35, 1]:
                self.renderer.render(frame, self.renderer.settings(name, 14), amount)

    def test_hdr_pq_reference_and_headroom(self):
        from check_hdr_footage import pq_nits
        frame = rgba(np.array([[[.18] * 3, [1] * 3, [4] * 3, [64] * 3]], np.float32))
        s = self.renderer.settings("Neutral / Clean Slate", 14, self.renderer.dll.oe_hdr_rendering_index())
        s[27] = self.renderer.dll.oe_hdr_output_index()
        s[self.renderer.dll.oe_hdr_peak_index()] = 1000
        s[self.renderer.dll.oe_hdr_white_index()] = 203
        actual = pq_nits(self.renderer.render(frame, s)[..., :3])
        np.testing.assert_allclose(actual[0, 0], 24.36, atol=.02)
        np.testing.assert_allclose(actual[0, 1], 203, atol=.05)
        self.assertTrue((actual[0, 2] > 203).all())
        self.assertTrue((actual[0, 3] > actual[0, 2]).all())
        self.assertTrue((actual < 1000).all())
        stock = self.renderer.settings("50D Daylight",14,self.renderer.dll.oe_hdr_rendering_index())
        stock[27] = self.renderer.dll.oe_hdr_output_index()
        bright = pq_nits(self.renderer.render(frame,stock)[...,:3])
        self.assertTrue((bright[0,3] > 300).all())

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
