# Upgrade Roadmap

## Completed Through v0.17

- Independent film/print color and tone strengths and in-module Enable toggles.
- Resolution-aware smooth halation, refined highlight selection, tint, and independent Aura radius.
- Creative film-gauge presets.
- Fixed print recipes with editable Custom inheritance.
- Monochrome finishing that includes print and texture.
- Independently enabled Film Development: Push/Pull with grain coupling, Color Richness, and Split Tone.
- Advanced grain: horizontal desqueeze and independent RGB intensity.

## Next: Separate Bloom

Implement neutral/source-colored highlight diffusion, independently enabled and distinct from warm Aura. Reuse the established resolution-aware filtering patterns, but budget and benchmark its additional spatial work. Check isolated highlights, colored sources, edge normalization, exact zero/disabled isolation, Mono, CPU/OpenCL parity, and combined 4K playback cost before installing.

## Further Work

- Investigate negative-response-driven, pre-print grain rather than the current additive post-print grain. Preserve independent texture-only operation and predictable print interaction.
- Reference-based profiles using properly licensed original scans and measured charts. Current film families remain original creative approximations; do not claim measured stock calibration without measurements.
- Ready-to-install Windows release ZIPs with complete compiler/runtime redistribution notices and clean-machine installation checks.
- CUDA backend for NVIDIA hosts after suitable hardware/testing becomes available. GPU ownership/event handling and existing CPU/OpenCL parity must remain intact.

macOS/Metal is postponed because no Mac is available for verification. Dust, scratches, borders, gate weave, and other damage effects are lower priority than core response, texture, and performance.
