# OpenEmulsion v0.27

Experimental Windows x64 update with portable user presets and optional negative-driven, pre-print grain. Post Print remains the default; existing preset recipes and texture-only workflows retain their appearance.

## Changes in v0.27

- Added native Save Preset / Load Preset dialogs and portable `.oepreset` JSON snapshots. Import optionally preserves camera balance, color spaces, and grain seed; all three are preserved by default.
- Added complete preset validation, Unicode file paths, atomic replacement, and malformed/oversize/duplicate-field rejection before applying any settings. Snapshot imports replace creative keyframes in one undoable edit; animation curves are not exported.
- Added Grain Response: Post Print (original/default) or Negative & Print. The new optional response keys grain after Film Color/Development and adds it before Print, so print tone/color shapes the texture. It does not add buffers, readbacks, blur passes, or extra grain evaluations.
- Retained original texture-only grain, source-keyed halation/bloom, exact zero/disabled behavior, Mono finishing, stable grain coordinates, and all built-in looks. The new selector is unavailable in texture-only modes.
- Added preset I/O/preservation tests and CPU/OpenCL grain-stage parity, interaction, isolation, and 4K benchmark cases. See `docs/USER_PRESETS.md` and `docs/TEXTURE_CONTROLS.md` for limits and host validation still needed.

## Included

- Twelve editable stock-inspired and creative looks in a top-level Preset dropdown.
- Portable user preset snapshots with selective context preservation.
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
