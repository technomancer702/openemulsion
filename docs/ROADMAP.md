# Upgrade Roadmap

## Completed Through v0.43

- Selecting Standard HDR directly now selects Rec.2100 PQ output and refreshes the viewing controls. Existing output-space animation is preserved outside the edit time; preset/undo/timeline notifications and non-HDR choices do not rewrite output context. Shipping-binary transition tests cover all output choices, animation and inactive callbacks. Rendering math and preset format are unchanged.

- Added a collapsed HDR Viewing group below SDR Viewing, including existing peak/white settings and new post-look Exposure Trim / Highlight Rolloff. Zero controls preserve v0.41; fixed gray/white anchors, C1 joins, bounded monotonic shoulders and shared CPU/OpenCL math. Format-7 capture and strict neutral legacy migration, output-context preservation, inactive-path greying/isolation, HDR emitter checks and local footage diagnostics. See [Color Spaces](COLOR_SPACES.md), [Color Bench](COLOR_BENCH.md) and the [upstream HDR review](SPEKTRAFILM_REVIEW.md#hdr-follow-up-2026-10-07).

- Moved the collapsed SDR Viewing group to the bottom below Selective Color, with shipping-plugin page-order regression checks. Rendering, defaults and preset format are unchanged.

- Exposed SDR Viewing Contrast, Highlight Rolloff and Gamut Compression, with exact v0.39 zero defaults, fixed middle-gray placement, shared CPU/OpenCL math and inactive-path greying/isolation. Format-6 output-context capture and strict legacy zero migration. Stage-isolated exposure ramps and five-clip comparisons document combined SDR/negative/print compression without changing existing creative recipes. See [Color Spaces](COLOR_SPACES.md) and [Color Bench](COLOR_BENCH.md).

- SDR bright-colored intensity shoulder to retain more lens texture, not merely reduce whitening. Full-resolution source/exposure comparisons, fixed source-keyed spatial contrast diagnostics, independent references and synthetic emitter-contrast checks. Neutral/sub-threshold colors and non-SDR paths are unchanged; no source reconstruction or spatial sharpening. See [Color Spaces](COLOR_SPACES.md) and [Color Bench](COLOR_BENCH.md).

- SDR colored-highlight shoulder to reduce taillight washout automatically, preserving neutral/low-intensity response and ordered exposure ramps. Retention adds to the updated default. Shared CPU/OpenCL arithmetic, independent references and local footage/isolated-path comparisons; no source highlight reconstruction. See [Color Spaces](COLOR_SPACES.md).

- Fixed v0.36 descriptor startup crash for root-level HDR controls. Added shipping-binary OFX descriptor/instance lifecycle checks for Filter/General contexts, including initial SDR/HDR control states. Rendering math and preset format are unchanged.

- Experimental Rec.2100 PQ / Rec.2020 output, post-look HDR viewing response, peak/reference-white controls, shared CPU/OpenCL math and format-5 output-context snapshots. Independent transfer/matrix anchors, all-recipe parity and local footage luminance checks. HLG/metadata and calibrated HDR monitoring remain outstanding. See [Color Spaces](COLOR_SPACES.md).

- Restrained Highlight Color Retention in Film Color, neutral at zero and isolated from disabled/managed/texture/matte/Mono workflows. Shared CPU/OpenCL arithmetic, smooth slider and exposure-ramp checks, format-4 snapshots with zero-retention legacy migration. See [Color Spaces](COLOR_SPACES.md).
- Optional [Offline Color Bench](COLOR_BENCH.md), using native production color stages, high-precision local ProRes 4:4:4 decode, float output/region statistics, tagged browser previews and experimental highlight variants. Original media and results are ignored and never packaged. Five-clip evaluation informed the retention range; this is not calibrated stock ground truth.

- Automatic SDR viewing response for scene-log/linear input going to display output, before creative film/print processing. Denser shadows without an added pedestal, smooth highlight shoulder and linear-light radial gamut compression. Explicit Conversion Only and Standard SDR policies, exact texture/bypass isolation and portable format-3 capture with legacy conversion-only migration. See [Color Spaces](COLOR_SPACES.md).

- Independent film/print color and tone strengths and in-module Enable toggles.
- Disabled-module control greying, mode-driven Enable toggles, and mode-aware print-preset locks.
- Selective Color is explicitly disabled on new instances and all ordinary presets; only Graphic Noir opts in. Full/Color Only preserve an explicit choice instead of activating it, while excluded modes disable it. Saved projects and user-file switches remain authoritative.
- Continuous negative gamut-compression amount, without the full-strength jump immediately above zero; consistent negative-channel flooring before later stages.
- Original plugin icon with editable SVG, RGBA PNG, and incremental resource packaging checks.
- Experimental Windows release ZIP packaging with instructions, matching source, compiler runtime notices, hashes, and extracted OFX load checks.
- Resolution-aware smooth halation, refined highlight selection, tint, and independent Aura radius.
- Creative film-gauge presets, expanded in v0.23 to Custom/8/Super 8/16/Super 16/35/Super 35/65/70 mm (15-perf), with the original recipes retained.
- Fixed print recipes with editable Custom inheritance.
- Monochrome finishing that includes print and texture.
- Independently enabled Film Development: Push/Pull with grain-strength coupling and stable grain geometry, Richness, and Split Tone.
- Advanced grain: horizontal desqueeze and independent RGB intensity.
- Categorized top-level preset browser with 52 complete editable recipes, including Neutral, stock and creative interpretations. Non-destructive category/Custom browsing, stable stored IDs, preserved context and category/recipe/edit-policy plus CPU/OpenCL checks.
- Fargo, Terminator 2, Alien and three Sin City graphic-noir interpretations, with primary production research. Independent source-keyed Selective Color with raw matte, smooth hue wrap/feather and final texture-neutralizing desaturation, in the existing CPU/OpenCL composite pass. Neutral by default, with portable format-version-2 save/load and strict version-1 migration. See [Selective Color](SELECTIVE_COLOR.md).
- Nine researched genre interpretations in Thriller, Horror and Sci-Fi; simplified stock labels without changing their artistic-approximation status. Synthetic color-only separation and shadow/palette intent checks, unchanged renderer math and existing recipes. See [Creative Look Research](CREATIVE_LOOK_RESEARCH.md).
- Revised creative recipes with clearer cinema/neon/vintage/reversal-home-movie separation, restrained modern stock-family refinements, synthetic intent/skin/neutral checks, and per-look 4K timings. Existing saved tuning and Custom defaults remain unchanged. See [Look Presets](LOOK_PRESETS.md).
- Targeted slider range expansion with unchanged defaults/presets, variance-normalized primary grain smoothing above Softness one, full-strength Mono semantic greying, historical response anchors, and expanded CPU/OpenCL/performance checks.
- Comparable Filmbox-style control terminology and clearer directional tooltips, without changing parameter identifiers or rendering. See [Control Names](CONTROL_NAMES.md).
- Portable user preset save/load, current-control capture before dialogs, complete restoration by default, strict JSON validation, atomic file replacement, and optional context preservation. See [User Presets](USER_PRESETS.md).
- Optional Negative & Print grain keyed by the developed negative before Print, with unchanged default/texture-only behavior and CPU/OpenCL parity. See [Texture Controls](TEXTURE_CONTROLS.md).

## Bloom Added in v0.18

Separate neutral/source-colored linear-light diffusion with an in-module Enable toggle, independent source selection/radius/protection, and Bloom Matte. Bloom is off by default (strength zero), uses cached OpenCL working buffers, and retains Mono finishing and texture-only encoding.

Synthetic checks cover bounded extraction, colored/white sources, continuous conserved spread, HD/4K scaling, edge normalization, every mode/mask/input, exact zero/disabled isolation, bypass/resizing, and CPU/OpenCL parity. Appearance and host workflow still need real-footage validation in Resolve.

## Further Work

- [SpektraFilm source review](SPEKTRAFILM_REVIEW.md): prioritize independent per-stock palette/response refinement and real-footage comparison over adding expensive spectral processing. Review print-only and reversal workflow with the new library.

- Real-footage refinement of the look library and the new optional Negative & Print grain response. Compare motion, shadow bias, print interaction, and proxy/full-resolution consistency.
- Community preset sharing and versioned preset-format evolution. Current built-in recipes live in `ofx/src/LookPresetConfig.h`; user files use complete validated snapshots, not animation curves.
- Reference-based profiles using properly licensed original scans and measured charts. Current film families remain original creative approximations; do not claim measured stock calibration without measurements.
- Clean-machine Resolve installation checks, other-GPU/host testing, and signed release binaries. Current ZIP checks validate extracted payloads and native OFX loading, not a second-machine Resolve session.
- CUDA backend for NVIDIA hosts after suitable hardware/testing becomes available. GPU ownership/event handling and existing CPU/OpenCL parity must remain intact.

macOS/Metal is postponed because no Mac is available for verification. Dust, scratches, borders, gate weave, and other damage effects are lower priority than core response, texture, and performance.
