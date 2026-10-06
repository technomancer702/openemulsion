# OpenEmulsion v0.23

Experimental Windows x64 update with an expanded Film Gauge dropdown. The plugin identifier and original gauge recipes are unchanged.

## Changes in v0.23

- Added Super 8, Super 16, Super 35, and 70 mm (15-perf) creative texture presets.
- Retained Custom, 8 mm, 16 mm, 35 mm, and 65 mm rendering recipes, with all formats in size order.
- Expanded CPU/OpenCL coverage and the optional texture preview to all nine choices. No extra rendering passes or buffers are added.
- Existing development nodes store numeric gauge indices: reselect your gauge after updating if it was 16, 35, or 65 mm. Other settings are not reset.

## Included

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
