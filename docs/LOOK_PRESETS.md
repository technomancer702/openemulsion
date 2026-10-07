# Look Presets

Added in v0.24. The top-level Preset dropdown sits directly below Film Gauge and remains available in every mode, including Bypass. New and existing instances default to Custom without changing the existing look.

## Library

| Preset | Creative target |
| --- | --- |
| 50D Daylight (Inspired) | Fine, restrained grain; cleaner color and gentle highlight shaping. |
| 250D Daylight (Inspired) | Balanced daytime starting point with moderate texture and near-neutral print balance. |
| 200T Tungsten (Inspired) | Fine texture, gentler contrast, and softer highlights; no automatic warm cast. |
| 500T Tungsten (Inspired) | More texture than 200T, stronger highlight shoulder and halation, with restrained shadow weighting. |
| B&W Reversal (Inspired) | Monochrome contrast with grain, without colored diffusion. |
| Classic Cinema | Richer color, denser print contrast, deeper shadows, and restrained halation. |
| Soft Portrait | Gentler contrast, saturation, and texture, with subtle bloom. |
| Neon Nights | Visible cool shadow/warm highlight split, cleaner print palette, stronger halation, and saturated-color compression. |
| Seventies Print | Muted vintage palette, warmer print balance, softer lifted shadows, and stronger texture. |
| Bleach Bypass | Bleach-retained family, lower saturation, harder contrast, and mostly monochrome grain. |
| Super 8 Home Movie | Brighter color-reversal-inspired palette, gently warm print balance, Super 8 gauge, and coarse texture. |
| Silver Noir | Monochrome, harder contrast, deeper blacks, and pronounced grain. |

These are original artistic recipes using existing controls, not calibrated reproductions of manufacturer stocks or particular movie grades. Stock-inspired names describe creative targets only. Tungsten/daylight names do not apply camera white balance: input footage is interpreted using the selected Input Color Space and the user's camera balance. There is no hidden stock transform.

## v0.29 Tuning

Classic Cinema, Neon Nights, Seventies Print, and Super 8 Home Movie now have clearer tone/color identities, not just different grain or glow. 250D, 200T, and 500T have modest toe/shoulder/print refinements and retain a consistent near-neutral family appearance. 50D, Soft Portrait, Bleach Bypass, B&W Reversal, and Silver Noir are unchanged. Default Custom controls and renderer math are unchanged.

This is recipe tuning, not a stock-calibration upgrade. Public [50D technical data](https://www.kodak.com/content/products-brochures/motion-picture/KODAK-VISION3-50D-5203-7203-technical-information.pdf), [200T information](https://www.kodak.com/content/products-brochures/motion-picture/KODAK-VISION3-200T-5213-7213-brochure.pdf), and [Kodak processing guidance](https://www.kodak.com/en/motion/page/processing-techniques/) inform qualitative targets: family consistency, fine grain, highlight latitude, and silver-retained contrast/desaturation. Numeric recipe values are original artistic choices, not fits to these documents. Super 8 is a format, not a single stock; Home Movie now explicitly targets a colorful reversal-inspired presentation rather than the same faded-negative palette as Seventies Print.

Saved projects and user preset files store ordinary parameter values. They do not change on upgrade, even if their Preset label still names a built-in look. To try a revised recipe on an existing node, select Custom, then the named preset again. This replaces creative tuning/keys but retains camera balance, encoding, and grain seed as before. No choice order, parameter identifiers, file format, or ownership policy changed.

On nine synthetic linear-light color/skin patches at five exposure scales (45 samples), mean absolute display-RGB difference (8-bit-equivalent codes) rose from 1.065 to 3.992 for Classic/Neon and from 5.245 to 14.460 for Seventies/Super 8. 200T/500T remains restrained at 2.045; 50D/250D at 1.339. These values exclude grain/glow, combine tone and color differences, and are neither perceptual DeltaE nor stock-accuracy scores. Representative midtone skin hues retain warm channel ordering; this is not comprehensive skin validation.

Preset application still adds no image passes, buffers, or readbacks. Glow amounts/radii and active blur stages were not increased by this pass. Seventies Print now enables nonneutral Richness in the existing per-pixel development stage; per-look 4K benchmarks exercise every complete recipe. Timing depends on hardware/host and is not a playback guarantee.

## Selection And Editing

- Selecting a named preset loads Full mode, Film Gauge, all seven module switches, and a complete set of creative module settings. It can reactivate processing from Bypass or Grain Only.
- Input Color Space, Output Color Space, Film Color Exposure/Temperature/Tint, and Grain Seed are preserved, including their keyframes. Print exposure and RGB balance are part of the creative recipe and are replaced.
- Selecting a named preset replaces all keyframes on recipe-owned settings and module switches. The writes are grouped in one host edit block.
- Each preset loads Print Style = Custom, so the print recipe remains editable. Normal disabled-module greying still applies. A manually selected fixed print style retains its existing locking behavior.
- Editing recipe-owned settings, Film Gauge, Mode, Print Style, or a module switch changes Preset to Custom without resetting anything. Camera balance, color-space, and grain-seed edits do not change the preset label.
- Selecting Custom does nothing to the settings. It is not a reset/default recipe.
- Loading a project, undo/redo, host-generated parameter notifications, or moving the playhead never reapplies a recipe. Rendering reads the ordinary module settings, not the preset selector; saved edits remain authoritative.

Preset application only writes parameters. It adds no rendering passes, buffers, per-pixel branching, or external LUT files. Individual looks can enable existing diffusion stages and thus cost more than a color/grain-only recipe.

## Source And Validation

Recipes and their parameter ownership are defined in `ofx/src/LookPresetConfig.h`. Contributors can propose new recipes without changing the renderer. v0.27 adds [User Presets](USER_PRESETS.md) for portable file save/load. Built-in recipes retain Post Print grain and reset Grain Response along with other creative settings; user files capture the selected response. Resolve's normal saved effect settings remain available.

The stock families are informed by public [Kodak camera-film references](https://www.kodak.com/en/motion/products/camera-films/), but no manufacturer graphs, datasets, proprietary presets, or third-party implementation code are incorporated.

Automated checks cover complete recipe application, valid control ranges, preserved encoding/balance/seed, Custom inheritance, repeated selection, finite/monotonic gray ramps, and edit/restore/recursion policy. v0.29 adds synthetic creative-separation guards, stock-family restraint/texture hierarchy/neutral pivot checks, representative midtone skin channel ordering, Neon cool-shadow/warm-highlight identity, and vintage-shadow softness. The render harness exercises every recipe across all supported input/output spaces, mode overrides, alpha, and all-disabled bypass on CPU/OpenCL, and times every full recipe at 4K. Synthetic appearance checks are not validation against actual film scans. Dropdown placement, grouped undo/redo, project reload, keyframe replacement, and final appearance should also be checked in Resolve on real footage.
