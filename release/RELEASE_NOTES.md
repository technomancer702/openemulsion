# OpenEmulsion v0.25

Experimental Windows x64 update with more expressive slider ranges and an extended grain-softness response. The plugin identifier, defaults, and built-in preset looks are retained.

## Changes in v0.25

- Expanded Temperature/Tint and Skin Hue to -3..3, Color Crosstalk and Split Tone to 0..3, and Negative Density to -1.2..1.5. Existing values keep their effect; no preset retuning or value migration is needed.
- Extended Grain Softness to 0..2. Above one, the main grain field is smoothed with approximately stable variance, without extra image passes or buffers. Zero to one retains the previous texture.
- Greyed out ineffective color controls in active full-strength Mono Negative, including forced-monochrome Grain Color. Partial strength and texture-only modes retain the applicable controls.
- Added v0.24 response/grain anchors for defaults and all twelve presets, expanded endpoint/semantic UI checks, and CPU/OpenCL parity/performance checks for new ranges and smoothing.
- Kept selective toe/shoulder and gamut behavior, existing print styles, halation/Aura/bloom, exposure/contrast/saturation, and grain amount/size/roughness unchanged. See `docs/SLIDER_TUNING.md`.

## Included

- Twelve editable stock-inspired and creative looks in a top-level Preset dropdown.
- Six original creative negative families, including monochrome finishing.
- Independent Film Color, Film Development, Print, Halation, Aura, Bloom, and Grain modules, with mode-driven toggles and disabled-control greying.
- Film Color's continuous gamut-compression amount, with the near-zero jump fixed.
- Full, Standard, Extended, and Custom print styles, with inherited and locked preset recipes.
- LogC3 EI-800, LogC4, Sony, Blackmagic, RED, Canon, Panasonic, ACES, DaVinci Wide Gamut, and standard display/linear input choices.
- Smooth resolution-aware halation/Aura, separate linear-light bloom, and diagnostic mattes.
- Procedural grain styles, size/softness/roughness, desqueeze, channel intensity, and gauge presets.
- Push/Pull, Color Richness, and Split Tone; Push/Pull preserves grain geometry.
- OpenCL acceleration, multithreaded CPU fallback, and an original plugin icon.

## Limitations

Windows x64 only; no CUDA, Metal, macOS, Linux, or native Windows ARM build. OpenCL needs host-provided OpenCL buffers. Testing uses Resolve 21.1.1 and an AMD GPU; other systems need testing. The binary is unsigned and the release is experimental, not production-certified. Profiles are artistic approximations, not measured film stocks. See `docs/COLOR_SPACES.md` for unsupported encodings, metadata assumptions, and workflow limitations.

Project source, documentation, build scripts, and tests are in the matching source ZIP. External Resolve/OpenFX developer files are required to rebuild and are not redistributed. Compiler runtime notices remain inside the installed plugin bundle.
