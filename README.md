# OpenEmulsion

Free, open-source film emulation for DaVinci Resolve: adjustable negative and print response, procedural grain, smooth halation, aura, and bloom in a native OpenFX plugin.

Film Color, Film Development, Print, Halation, Aura, Bloom, and Grain can be enabled independently. Use the whole pipeline or keep your own grade and LUTs with texture-only processing.

## Status

Experimental, Windows x64. Current development version: **v0.30**. The current implementation supports OpenCL acceleration and a multithreaded CPU fallback. CUDA, Metal, macOS, and Linux builds are not implemented.

OpenEmulsion is an original artistic approximation, not a measured film-stock calibration or a complete HDR rendering transform. There is no DCTL dependency. Rendering has been tested in Resolve, but this is not yet a stable production release.

## Build

For the prebuilt Windows ZIP, follow `INSTALL.md` inside the release instead of the development build/install steps below. The first experimental binary release is v0.22; it includes a complete OFX bundle, installation and usage guides, runtime notices, matching project source, and a SHA-256 checksum. macOS/Linux binaries are not available.

Prerequisites:

- Windows x64 and DaVinci Resolve's OpenFX developer files.
- CMake 3.20 or newer, Ninja, and a C++17 compiler on PATH.
- The current development build is tested with MinGW-w64 GCC. Other compiler configurations need verification.
- An OpenCL-capable GPU/driver for GPU rendering. No OpenCL SDK is needed to compile.

The default developer-file location is:

```text
C:\ProgramData\Blackmagic Design\DaVinci Resolve\Support\Developer\OpenFX
```

It must contain both `OpenFX-1.4` and `Support`. These external SDK files are not vendored in this repository.

From the project root:

```powershell
.\tools\build_ofx.ps1
ctest --test-dir build/ofx --output-on-failure
```

For an alternate SDK location:

```powershell
.\tools\build_ofx.ps1 -ResolveOpenFXRoot "D:\SDKs\OpenFX"
```

The bundle is generated at `dist/OpenEmulsion.ofx.bundle`. Build output is excluded from Git; release binaries should be distributed separately.

The original film-frame icon is packaged in `Contents/Resources`, using the plugin identifier as its filename according to the [OpenFX icon convention](https://openfx.readthedocs.io/en/main/Reference/ofxPackaging.html#plug-in-icons). Its editable SVG and 256x256 RGBA PNG are tracked in `ofx/resources`. Builds copy both assets even when no binary relink is needed. No additional asset-generation dependency is needed to build or install the plugin. Icon display and sizing are controlled by the host; restart Resolve after installation.

## Install

Close Resolve, then run:

```powershell
.\tools\install_ofx.ps1
```

The installer copies the bundle into this project's `ofx-plugins` directory and adds that directory to the user `OFX_PLUGIN_PATH`. Keep the project folder in place while using this installation.

Restart Resolve from the Start Menu or a new shell. Look for **OpenFX > OpenEmulsion > OpenEmulsion**. Alternatively, manually copy the bundle to the standard Windows OFX directory, `C:\Program Files\Common Files\OFX\Plugins` (administrator access required).

Earlier development builds appeared as Cleanroom Film. OpenEmulsion uses a new plugin identifier; existing Cleanroom nodes are not migrated. The installer does not remove an older plugin.

## Color Workflow

Set **Input Color Space** to the RGB space entering the node, which may differ from the camera recording space. It is not auto-detected.

- Unconverted Alexa footage: choose **ARRI Alexa LogC3 / Wide Gamut 3 (EI 800)**. For an SDR look with Film Color or Print enabled, choose **Rec.709 / Gamma 2.4** output.
- A DaVinci Wide Gamut/Intermediate timeline: choose that input and **Same as Input** output, keeping the project's normal output transform.
- Your own LUT: use **Grain Only** or **Halation, Bloom & Grain Only** and select the space entering this node. Texture-only processing preserves its input encoding for a downstream LUT.

New instances default to Rec.709/Gamma 2.4 input and Same as Input output. Supported choices also include ARRI LogC4, Sony S-Log3, Blackmagic Film Gen 5, RED Log3G10, Canon Log 2/3, Panasonic V-Log, ACEScct, ACEScg, sRGB, and linear Rec.709.

See [Color Spaces](docs/COLOR_SPACES.md) for exact gamut pairs, workflow details, references, and limitations.

## Modes and Controls

Modes: **Full**, **Color Only**, **Halation, Bloom & Grain Only**, **Grain Only**, **Bypass**, **Halation Matte**, and **Bloom Matte**.

Each module's first control is an **Enable** toggle that independently bypasses that stage without resetting its settings. In Full mode, disabling Film Color, Film Development, and Print leaves texture only. Bypass and all-disabled processing preserve RGBA exactly, including negative RGB and values above 1.

Selecting a mode switches its applicable modules on and the others off, without resetting sliders or print recipes. Disabled modules grey out their options; toggles excluded by the mode are also unavailable. Returning to Full enables all modules, after which you can disable individual ones again. Print recipe knobs require both an enabled Print module and Custom style. See [Module Controls](docs/MODULE_CONTROLS.md) for the mode mapping and animation limitations.

- **Film Color:** independent color/tone strengths, six creative families, linear-light exposure/balance, density, saturation, toe, contrast, shoulder, crosstalk, gamut compression, and Skin Hue.
- **Film Development:** Push/Pull changes tone and grain strength without moving or resizing the grain pattern; Richness favors muted colors; Split Tone has hue, pivot, Dead Zone Width, and separate shadow/highlight intensities. Neutral defaults preserve the previous look.
- **Print:** Full (Film Print), Standard, Extended (Telecine), and Custom. Named presets load and lock their tone/color recipe; Custom unlocks the last recipe without changing its look. Color/tone strengths, exposure, and RGB balance remain editable in every style.
- **Grain:** Fine, Classic, Rough, and Debug styles; strength, size, softness, roughness, color, horizontal stretch, independent RGB intensity, tonal weighting, and repeatable frame/seed variation. Grain Response optionally adds grain before Print and keys it from the developed negative; Post Print remains the default and texture-only behavior is preserved.
- **Halation and Aura:** smooth source-highlight selection with adjustable threshold/transition, red-to-amber tint, independent tight-halo and broad-aura radii, and resolution-aware continuous Gaussian spread. Matte mode exposes the signal.
- **Bloom:** separate neutral/source-colored linear-light diffusion with its own strength, radius, threshold/transition, highlight protection, and Bloom Matte. It defaults to zero and is independent of Aura and Film Gauge.
- **Film Gauge:** Custom, 8 mm, Super 8, 16 mm, Super 16, 35 mm, Super 35, 65 mm, and 70 mm (15-perf) creative presets coordinate grain size/strength and halo spread without resetting sliders. Custom and 35 mm use the unscaled settings. Formats do not crop/resize the image or affect Bloom or film/print color. Older development nodes need their gauge reselected after the dropdown expansion; see [Texture Controls](docs/TEXTURE_CONTROLS.md).
- **Preset Category / Preset:** below Film Gauge, 37 editable recipes plus Custom, filtered into starting points, cinema negatives, still negatives, reversal film, monochrome, print looks, and creative looks, with an All Presets view. Category browsing does not change the image. Neutral / Clean Slate removes creative effects while retaining color-space conversion and camera balance; its Film/Print strengths are zero. Other looks cover stock-inspired targets and creative grades, not measured film reproductions. Recipes load Full mode, gauge, module switches and creative settings, preserving encoding, camera balance and grain seed. Editing marks Custom; selecting Custom alone does nothing. Existing saved values remain authoritative. See [Look Presets](docs/LOOK_PRESETS.md).
- **User Presets:** native Save/Load dialogs export portable `.oepreset` JSON snapshots of all current controls before opening the dialog, including module switches and Grain Response. Loading restores everything by default; optional Preserve switches retain destination color spaces, camera balance, or grain seed. Existing v0.27 nodes keep their stored Preserve switches; uncheck all three for a complete restore. See [User Presets](docs/USER_PRESETS.md).
- **Slider tuning:** expanded balance, Skin Hue, Crosstalk, Density, and Split Tone ranges retain existing values/defaults/preset looks. Grain Softness above 1 smooths the primary field with normalized variance and no extra image pass. Full-strength Mono greys out ineffective color controls. See [Slider Tuning](docs/SLIDER_TUNING.md).

With **Mono Negative** and Film Color Strength at 1, the final print and texture composite stays monochrome, including source-colored bloom. Halation/Aura remain visible as neutral glow, and grain automatically becomes monochrome. This does not affect texture-only modes, bypass, or a disabled Film Color module.

Grain Size uses a 1080-line reference, scaling granules with image height. Grain Color at zero uses one monochrome noise field; RGB intensity controls at equal values give equal RGB grain. Horizontal Stretch uses a 0.5-2.0 desqueeze ratio and does not resize the image. Grain is procedural, not sampled from film scans.

Strengths default to 1 (full response). Zero Color Strength removes that stage's color character; zero Tone Strength removes its tone shaping. Camera and print exposure/balance remain independent while their modules are enabled. Disable Film Color, Film Development, and Print completely when using your own LUT with texture only.

Selecting a named print preset replaces Custom recipe adjustments and their keyframes. Print strength, exposure/balance, and other modules are not reset. The style selector itself is not animated; recipe knobs can be animated in Custom.

See [Negative and Print Response](docs/FILM_RESPONSE.md), [Film Development](docs/FILM_DEVELOPMENT.md), [Texture Controls](docs/TEXTURE_CONTROLS.md), and [Bloom](docs/BLOOM.md) for exact semantics and limitations.

## Performance and Tests

OpenCL runs when Resolve supplies OpenCL image buffers. Otherwise the plugin uses CPU rendering; a machine's general GPU capability alone does not guarantee OpenCL execution in the host.

Working buffers are reused on the GPU. The highlight blur uses a resolution-scaled work grid with dense filtering and area-averaged source extraction. Disabled texture stages skip their work. Keep Halation, Aura, and Bloom at zero when unused; Grain Only skips the blur entirely.

The regression harness covers color-space reference values, tone curves, grain statistics, continuous halation, module isolation, partial/zero color-tone strengths, gauge presets, HD/4K halo scaling, odd-sized higher-resolution grids, bypass, alpha, development tone/split isolation, grain stretch/channel gains, linear bloom extraction/spread/protection, and CPU/OpenCL parity. GPU checks are skipped explicitly when no device is available. GPU-resident 4K timings exclude Resolve, transfers, and other effects; they are not timeline playback guarantees.

Optional synthetic previews:

```powershell
New-Item -ItemType Directory -Path analysis -Force
.\build\ofx\HalationTests.exe analysis\grain-preview.bmp analysis\response-preview.bmp analysis\texture-preview.bmp
```

The optional sixth preview path produces a five-panel bloom comparison; see [Bloom](docs/BLOOM.md). The optional third preview uses OpenCL and compares all nine gauge choices across isolated lights and grain-only patches; it is skipped when no OpenCL device is available.

## Release Packaging

Commit release files first, then run `.\tools\package_release.ps1`. It builds and tests Release, verifies the supported compiler/runtime notices and imported DLLs, and creates `dist/releases/OpenEmulsion-v0.30-Windows-x64.zip` plus `.zip.sha256`. Use `-Force` only when deliberately replacing a generated archive. Release output is ignored by Git; upload the ZIP and checksum as release assets, not repository files.

The packager includes only the bundle, release guides, documentation, licenses, and a `git archive` source snapshot of the exact revision in `manifest.json`. It validates the extracted files and hashes, loads the extracted OFX binary using only its directory and Windows system DLL search, and checks its exported identifier/version. To recheck an archive, run `.\tools\test_release.ps1 -Archive "dist/releases/OpenEmulsion-v0.30-Windows-x64.zip"` in 64-bit PowerShell. This is not a clean-machine Resolve installation test or a code-signing/security certification; separate-machine and other-GPU validation remain outstanding.

Optional Resolve test charts:

```powershell
python -m pip install -r tools/requirements.txt
python tools/generate_test_charts.py
```

Generated charts and diagnostics stay local under `analysis/`.

See the [Upgrade Roadmap](docs/ROADMAP.md) for remaining work.

## Contributing and License

See [Contributing](CONTRIBUTING.md). OpenEmulsion source is licensed under [MPL-2.0](LICENSE). External OpenFX components retain their own notices in [Third-Party Notices](THIRD_PARTY_NOTICES.md).

No proprietary film-plugin binaries, shaders, LUTs, or stock profiles are included.
