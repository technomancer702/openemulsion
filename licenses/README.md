# Compiler Runtime Notices

These unmodified upstream texts accompany the Windows binary built with WinLibs GCC 16.1.0, MinGW-w64 14.0.0, UCRT, and POSIX threads (WinLibs r4). GCC/libstdc++ and Winpthreads are statically linked. The compiler itself is not distributed. Windows system libraries and the GPU driver's OpenCL runtime are not bundled.

- `GCC-COPYING3.txt` and `GCC-RUNTIME-EXCEPTION.txt`: [GCC 16.1.0](https://github.com/gcc-mirror/gcc/tree/releases/gcc-16.1.0), `COPYING3` and `COPYING.RUNTIME`.
- `MinGW-w64-COPYING.txt`, `MinGW-w64-RUNTIME.txt`, `MinGW-w64-DISCLAIMER.txt`, `MinGW-w64-PUBLIC-DOMAIN.txt`, and `MinGW-w64-AUTHORS.txt`: [MinGW-w64 v14.0.0](https://github.com/mingw-w64/mingw-w64/tree/v14.0.0), including `COPYING.MinGW-w64-runtime/COPYING.MinGW-w64-runtime.txt`.
- `Winpthreads-COPYING.txt`: the same MinGW-w64 tag, `mingw-w64-libraries/winpthreads/COPYING`.

OpenEmulsion's own source and icon remain MPL-2.0. OpenFX notices are in `THIRD_PARTY_NOTICES.md`. These runtime texts do not change the project's license. Reassess the included notices when changing toolchains. The current release packager checks the supported GCC/MinGW versions rather than silently applying these notices to a different compiler.
