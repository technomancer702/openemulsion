OpenEmulsion v0.31 - Windows x64
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
Film Color, Film Development, Print, Halation, Aura, Bloom, and Grain.
Modules are independently switchable. Texture-only modes work with your own LUT.
Bloom defaults to zero; raise its strength to use it.
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
The film profiles are original creative approximations, not measured stocks.

SOURCE AND LICENSE
Free and open source, MPL-2.0. See LICENSE and THIRD_PARTY_NOTICES.md.
Compiler runtime notices travel with the plugin in Contents/Licenses.
Source/OpenEmulsion-v0.31-source.zip contains the matching project source;
external OpenFX SDK files are not included. See manifest.json for the exact
source revision and SHA-256 hashes.

Project: https://github.com/technomancer702/openemulsion
Report problems: https://github.com/technomancer702/openemulsion/issues
Include Resolve version, Windows version, GPU/driver, input/output spaces,
active modules, and steps to reproduce. Never upload private footage without
permission.
