# SPDX-License-Identifier: MPL-2.0

import unittest

import numpy as np

import render_readme_examples as examples


class LogPreviewTests(unittest.TestCase):
    def test_camera_codes_are_not_transformed(self):
        frame = np.array([[[.1, .4, .7, 1], [-.1, .5, 1.2, 1]]], dtype=np.float32)
        original = frame.copy()
        preview = np.asarray(examples.log_preview(frame))
        np.testing.assert_array_equal(preview, np.rint(np.clip(frame[..., :3], 0, 1) * 255).astype(np.uint8))
        np.testing.assert_array_equal(frame, original)

    def test_invalid_frame_is_rejected(self):
        with self.assertRaises(ValueError):
            examples.log_preview(np.full((1, 1, 4), np.nan, dtype=np.float32))

    def test_gas_station_is_excluded(self):
        self.assertTrue(all('gas station' not in name.lower() for name, _ in examples.SAMPLES))
        self.assertTrue(all('gas station' not in name.lower() for _, name, _, _ in examples.EXAMPLES))

    def test_key_tuning_only_changes_the_graphic_noir_example(self):
        class Renderer:
            def settings(self, label, source):
                return np.zeros(84, dtype=np.float32)

        renderer = Renderer()
        baseline = renderer.settings('', 0)
        np.testing.assert_array_equal(examples.settings_for_example(renderer, 'night', ''), baseline)
        tuned = examples.settings_for_example(renderer, 'selective-color', '')
        expected = baseline.copy()
        expected[[examples.SELECTIVE_HUE, examples.SELECTIVE_RANGE, examples.SELECTIVE_FEATHER,
                  examples.SELECTIVE_MINIMUM_SATURATION]] = [355, 8, 3, .8]
        np.testing.assert_array_equal(tuned, expected)


if __name__ == '__main__':
    unittest.main()
