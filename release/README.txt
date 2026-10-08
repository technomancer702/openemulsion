OpenEmulsion v0.44.0 - Windows x64
Experimental development release

INSTALL
1. Close DaVinci Resolve.
2. Extract this ZIP (do not install from inside the ZIP).
3. Copy the entire OpenEmulsion.ofx.bundle folder to:
   C:\Program Files\Common Files\OFX\Plugins
   Create that folder if necessary. Windows may request administrator approval.
4. Restart Resolve. Look for OpenFX > OpenEmulsion > OpenEmulsion.

Keep the bundle's Contents folder and all its files intact. You do not need
a compiler, Python, DCTL, or this project's development folder to install it.
Read INSTALL.md for updates, uninstalling, quick-start settings, and caveats.

WHAT IS INCLUDED
Film Color, Film Development, Print, Halation, Aura, Bloom, Grain, Selective Color.
Modules are independently switchable. Texture-only modes work with your own LUT.
Bloom defaults to zero; raise its strength to use it.
Selective Color defaults to disabled; only Graphic Noir presets enable it.
Output Rendering defaults to Auto for log/linear input sent to Rec.709 or sRGB.
Display-ready input, managed log output and texture-only modes are unaffected.
Experimental Rec.2100 PQ / Rec.2020 output adds post-look HDR rendering, with
1000-nit peak and 203-nit reference-white defaults. Auto selects HDR for scene
input to PQ; Standard HDR explicitly enables it. Set Resolve HDR monitoring
and export metadata separately. HLG/PQ input and HDR display certification are
not included. Strong film/print shoulders can still reduce highlight headroom.
HDR Viewing sits below SDR Viewing and groups peak/reference white with post-look
Exposure Trim and Highlight Rolloff. Both adjustments default to zero, retaining
v0.41 HDR output; they are inactive outside HDR rendering. Format-7 user presets
capture them and complete older files load neutral adjustments.
Selecting Standard HDR directly also selects Rec.2100 PQ output. Other rendering
choices and preset/project restoration do not change the output-space menu.
The SDR foundation now preserves more color/gradation in bright saturated
emitters automatically. Highlight Color Retention adds to this response with a
brightness tradeoff. Zero uses the updated default, not the pre-v0.38 look.
User Presets can save/load portable look snapshots. Grain Response optionally
adds negative-driven grain before Print; Post Print remains the default.

REQUIREMENTS AND STATUS
Windows 10/11 x64, DaVinci Resolve, and a supported display/GPU driver.
Development testing: Resolve 21.1.1 on Windows x64, CPU and AMD OpenCL.
Other Resolve versions and NVIDIA/Intel GPU paths are not certified.
OpenCL acceleration requires Resolve to provide OpenCL buffers; otherwise
the CPU fallback is used and can be slower. CUDA and Metal are not implemented.
macOS, Linux, and native Windows ARM builds are not included.

This is an unsigned experimental build, not a stable production release.
Test on copies of projects before using it for important work.

SOURCE AND LICENSE
Free and open source, MPL-2.0. See LICENSE and THIRD_PARTY_NOTICES.md.
Compiler runtime notices travel with the plugin in Contents/Licenses.
The versioned ZIP in Source contains the matching project source;
external OpenFX SDK files are not included. See manifest.json for the exact
source revision and SHA-256 hashes.

Project: https://github.com/technomancer702/openemulsion
Report problems: https://github.com/technomancer702/openemulsion/issues
Include Resolve version, Windows version, GPU/driver, input/output spaces,
active modules, and steps to reproduce. Never upload private footage without
permission.
