# OpenEmulsion v0.24

Experimental Windows x64 update with a top-level look-preset library. The plugin identifier and renderer are unchanged.

## Changes in v0.24

- Added Preset directly below Film Gauge with twelve stock-inspired/creative looks and Custom.
- Named presets load Full mode, gauge, module switches, and complete editable recipes. All print controls start in Custom style.
- Preserved input/output color spaces, camera exposure/temperature/tint, and grain seed. Selecting a preset replaces creative tuning and its keyframes, including print exposure/balance.
- Editing a recipe marks it Custom without resetting settings. Selecting Custom alone retains the current look; undo/reload/time notifications do not reapply presets.
- Added recipe ownership/range/edit-policy tests and CPU/OpenCL checks across every supported input/output space. Presets add no renderer passes or buffers; their chosen effects use the existing pipeline.
- See `docs/LOOK_PRESETS.md` for the library and selection behavior. New/existing instances default to Custom without automatic look changes.

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
