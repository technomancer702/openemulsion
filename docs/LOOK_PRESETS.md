# Look Presets

Added in v0.24. The top-level Preset dropdown sits directly below Film Gauge and remains available in every mode, including Bypass. New and existing instances default to Custom without changing the existing look.

## Library

| Preset | Creative target |
| --- | --- |
| 50D Daylight (Inspired) | Fine, restrained grain; cleaner color and gentle highlight shaping. |
| 250D Daylight (Inspired) | Balanced daytime starting point with moderate texture. |
| 200T Tungsten (Inspired) | Restrained texture and softer highlights. |
| 500T Tungsten (Inspired) | Stronger shadow grain, shoulder, and highlight halation. |
| B&W Reversal (Inspired) | Monochrome contrast with grain, without colored diffusion. |
| Classic Cinema | Richer color and stronger print character with restrained halation. |
| Soft Portrait | Gentler contrast, saturation, and texture, with subtle bloom. |
| Neon Nights | Cool shadow/warm highlight split, stronger halation, and saturated-color compression. |
| Seventies Print | Vintage palette, warmer print balance, lifted blacks, and stronger texture. |
| Bleach Bypass | Bleach-retained family, lower saturation, harder contrast, and mostly monochrome grain. |
| Super 8 Home Movie | Super 8 gauge, warm print balance, softer response, and coarse texture. |
| Silver Noir | Monochrome, harder contrast, deeper blacks, and pronounced grain. |

These are original artistic recipes using existing controls, not calibrated reproductions of manufacturer stocks or particular movie grades. Stock-inspired names describe creative targets only. Tungsten/daylight names do not apply camera white balance: input footage is interpreted using the selected Input Color Space and the user's camera balance. There is no hidden stock transform.

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

Automated checks cover complete recipe application, valid control ranges, preserved encoding/balance/seed, Custom inheritance, repeated selection, finite/monotonic gray ramps, and edit/restore/recursion policy. The render harness exercises every recipe across all supported input/output spaces, mode overrides, alpha, and all-disabled bypass on CPU/OpenCL. Synthetic appearance checks are not validation against actual film scans. Dropdown placement, grouped undo/redo, project reload, keyframe replacement, and final appearance should also be checked in Resolve on real footage.
