# Upgrade Roadmap

## Completed Through v0.24

- Independent film/print color and tone strengths and in-module Enable toggles.
- Disabled-module control greying, mode-driven Enable toggles, and mode-aware print-preset locks.
- Continuous negative gamut-compression amount, without the full-strength jump immediately above zero; consistent negative-channel flooring before later stages.
- Original plugin icon with editable SVG, RGBA PNG, and incremental resource packaging checks.
- Experimental Windows release ZIP packaging with instructions, matching source, compiler runtime notices, hashes, and extracted OFX load checks.
- Resolution-aware smooth halation, refined highlight selection, tint, and independent Aura radius.
- Creative film-gauge presets, expanded in v0.23 to Custom/8/Super 8/16/Super 16/35/Super 35/65/70 mm (15-perf), with the original recipes retained.
- Fixed print recipes with editable Custom inheritance.
- Monochrome finishing that includes print and texture.
- Independently enabled Film Development: Push/Pull with grain-strength coupling and stable grain geometry, Color Richness, and Split Tone.
- Advanced grain: horizontal desqueeze and independent RGB intensity.
- Top-level Preset dropdown with twelve complete, editable stock-inspired/creative recipes, Custom inheritance, preserved camera balance/encoding/seed, and recipe/edit-policy plus CPU/OpenCL checks.

## Bloom Added in v0.18

Separate neutral/source-colored linear-light diffusion with an in-module Enable toggle, independent source selection/radius/protection, and Bloom Matte. Bloom is off by default (strength zero), uses cached OpenCL working buffers, and retains Mono finishing and texture-only encoding.

Synthetic checks cover bounded extraction, colored/white sources, continuous conserved spread, HD/4K scaling, edge normalization, every mode/mask/input, exact zero/disabled isolation, bypass/resizing, and CPU/OpenCL parity. Appearance and host workflow still need real-footage validation in Resolve.

## Further Work

- Real-footage refinement of the look library and user preset save/load for community sharing. Current built-in recipes live in `ofx/src/LookPresetConfig.h`; there is no preset file import/export yet.
- Investigate negative-response-driven, pre-print grain rather than the current additive post-print grain. Preserve independent texture-only operation and predictable print interaction.
- Reference-based profiles using properly licensed original scans and measured charts. Current film families remain original creative approximations; do not claim measured stock calibration without measurements.
- Clean-machine Resolve installation checks, other-GPU/host testing, and signed release binaries. Current ZIP checks validate extracted payloads and native OFX loading, not a second-machine Resolve session.
- CUDA backend for NVIDIA hosts after suitable hardware/testing becomes available. GPU ownership/event handling and existing CPU/OpenCL parity must remain intact.

macOS/Metal is postponed because no Mac is available for verification. Dust, scratches, borders, gate weave, and other damage effects are lower priority than core response, texture, and performance.
