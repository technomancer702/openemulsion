# Development and Release Packaging

See the [README](../README.md#build-from-source) for build prerequisites and the basic build/test commands.

## Rendering and Performance

C++ and OpenCL share the grain, color, and film-response math headers. Keep them compatible with both languages. OpenCL runs when Resolve supplies GPU image buffers; otherwise the plugin uses its multithreaded CPU renderer.

GPU working buffers are reused. Disabled texture stages skip their work, and Grain Only skips diffusion entirely. Halation, Aura, and Bloom at zero avoid their unused processing. See [Bloom](BLOOM.md) and [Texture Controls](TEXTURE_CONTROLS.md) for work-grid and filtering details.

The harness covers color-space references, tone curves, grain statistics, highlight diffusion, module isolation, gauge scaling, alpha/bypass, presets, descriptor lifecycle, version metadata, and CPU/OpenCL parity. GPU tests report skips when no device is available. Harness timings exclude Resolve, transfers, and other nodes; compare them separately from timeline playback.

The minimal OFX host loads the shipping binary, describes Filter/General controls, creates and destroys instances, and checks control states and parameter-change transitions. Also test UI interaction, undo/redo, saved-project reload, and playback in Resolve.

## Diagnostic Tools

Generate synthetic previews with the native rendering harness:

```powershell
New-Item -ItemType Directory -Path analysis -Force
.\build\ofx\HalationTests.exe analysis\grain.bmp analysis\response.bmp analysis\texture.bmp analysis\mono.bmp analysis\development.bmp analysis\bloom.bmp
```

The third preview uses OpenCL and compares film gauges; it is skipped without a device. The fifth and sixth previews show development and bloom diagnostics. See their module guides for panel descriptions.

Generate Resolve test charts:

```powershell
python -m pip install -r tools/requirements.txt
python tools/generate_test_charts.py
```

The [Offline Color Bench](COLOR_BENCH.md) renders float comparisons through production color math with scopes, region statistics, and highlight experiments. It is separate from the installed plugin. Media and results stay in ignored local directories.

## README Comparisons

`tools/render_readme_examples.py` uses the native ColorBench bridge and the local LogC3 sample clips to reproduce the color-only examples in `docs/media`. Each pair uses the same decoded frame and preserves the full image. The left displays untreated LogC3 code values directly, without a viewing transform. The right applies the named preset with neutral camera exposure/balance and SDR rendering, then converts Gamma 2.4 output to tagged sRGB for browsers. Grain and diffusion are not included. The comparison shows both scene-to-display rendering and creative styling.

With the [Color Bench dependencies](COLOR_BENCH.md#setup) installed and `BUILD_COLOR_BENCH=ON`:

```powershell
cmake --build build/ofx --target ColorBench
python tools/render_readme_examples.py --contact-only
python tools/render_readme_examples.py
```

The contact sheet samples multiple scenes in `Skin Colour.mov` and the three other professional clips. It stays local in `analysis/readme-examples`; the gas-station clip is excluded. The script's `EXAMPLES` list selects the published frames and presets. Rendering settings, timestamps and the bridge hash stay in an ignored local manifest. Only approved comparison JPEGs belong in `docs/media`; original clips remain ignored.

The Graphic Noir example uses a shot-specific Selective Color key: Keep Hue 355 degrees, Hue Range 8, Hue Feather 3, Minimum Saturation 0.80. This rejects the amber edge haze while retaining the taillights; the built-in preset is unchanged. The README discloses this adjustment. Hue selection cannot distinguish two objects with identical colors.

## Plugin Icon

The editable SVG and 256x256 RGBA PNG live in `ofx/resources`. Builds copy both into `Contents/Resources` even without a binary relink. Filenames match the plugin identifier, following the [OpenFX icon convention](https://openfx.readthedocs.io/en/main/Reference/ofxPackaging.html#plug-in-icons). Resolve controls icon display and sizing; restart it after installation.

## Release Packaging

Update the shared version and current release documentation as described in [Versioning](VERSIONING.md). Commit source and release files before packaging so the included source snapshot matches the binary.

```powershell
.\tools\package_release.ps1
```

The script builds Release, runs regressions, checks compiler/runtime notices and imported DLLs, then creates `dist/releases/OpenEmulsion-v<version>-Windows-x64.zip` and `.zip.sha256`. Use `-Force` only to deliberately replace a generated archive. Release output is ignored by Git; upload the ZIP and checksum as release assets.

The package contains the complete bundle, guides, documentation, licenses, and a `git archive` source snapshot. `manifest.json` records the exact source revision, human-readable and encoded OFX versions, file sizes, and hashes. Validation checks extracted payloads and loads the binary with plugin-directory/system DLL search only.

To check an existing archive in 64-bit PowerShell:

```powershell
.\tools\test_release.ps1 -Archive "dist/releases/OpenEmulsion-v0.44.0-Windows-x64.zip"
```

Archive validation does not replace clean-machine Resolve installation, other-GPU tests, or code signing. See the [Roadmap](ROADMAP.md) for remaining release work.
