# OpenEmulsion v0.26

Experimental Windows x64 terminology update. Control labels and tooltips are clearer and aligned with comparable Filmbox terminology. Rendering, performance, parameter identifiers, values, animation, and built-in preset recipes are unchanged from v0.25.

## Changes in v0.26

- Renamed Negative Density to Color Density, Color Richness to Richness, Neutralize Print to Neutralize Balance, Neutral Width to Dead Zone Width, and Print Tone to Print Tone Curve.
- Retained Skin Hue and Print Color: comparable Filmbox controls use the same terminology and direction. Print Color's tooltip now explains that zero is more print-like and one is more neutral/telecine-like, distinct from Print Color Strength.
- Added clearer tooltips for density, richness, print curve/balance, and the split-tone dead zone. No processing changes or additional GPU work.
- Retained the broader slider ranges and extended grain softness from v0.25. See `docs/SLIDER_TUNING.md` and `docs/CONTROL_NAMES.md`.

## Included

- Twelve editable stock-inspired and creative looks in a top-level Preset dropdown.
- Six original creative negative families, including monochrome finishing.
- Independent Film Color, Film Development, Print, Halation, Aura, Bloom, and Grain modules, with mode-driven toggles and disabled-control greying.
- Film Color's continuous gamut-compression amount, with the near-zero jump fixed.
- Full, Standard, Extended, and Custom print styles, with inherited and locked preset recipes.
- LogC3 EI-800, LogC4, Sony, Blackmagic, RED, Canon, Panasonic, ACES, DaVinci Wide Gamut, and standard display/linear input choices.
- Smooth resolution-aware halation/Aura, separate linear-light bloom, and diagnostic mattes.
- Procedural grain styles, size/softness/roughness, desqueeze, channel intensity, and gauge presets.
- Push/Pull, Richness, and Split Tone; Push/Pull preserves grain geometry.
- OpenCL acceleration, multithreaded CPU fallback, and an original plugin icon.

## Limitations

Windows x64 only; no CUDA, Metal, macOS, Linux, or native Windows ARM build. OpenCL needs host-provided OpenCL buffers. Testing uses Resolve 21.1.1 and an AMD GPU; other systems need testing. The binary is unsigned and the release is experimental, not production-certified. Profiles are artistic approximations, not measured film stocks. See `docs/COLOR_SPACES.md` for unsupported encodings, metadata assumptions, and workflow limitations.

Project source, documentation, build scripts, and tests are in the matching source ZIP. External Resolve/OpenFX developer files are required to rebuild and are not redistributed. Compiler runtime notices remain inside the installed plugin bundle.
