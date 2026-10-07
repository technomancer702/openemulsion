# Upgrade Roadmap

## Completed Through v0.33

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
