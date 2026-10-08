# Contributing to OpenEmulsion

Bug reports, footage-based feedback, documentation, and focused pull requests are welcome.

## Build and Test

Follow the prerequisites and build instructions in [README.md](README.md), then run:

```powershell
ctest --test-dir build/ofx --output-on-failure
```

Windows x64 is the currently implemented platform. C++ and OpenCL compile the same grain, color, and film-response math headers. Keep those headers compatible with both languages.

GPU tests need an OpenCL device and driver. The harness reports skipped GPU and out-of-order queue checks explicitly; a CPU-only pass is not GPU validation.

## Changes

- Keep edits focused and describe their visible effect.
- Include regression tests for rendering or parameter changes.
- Preserve exact bypass and alpha, negative/HDR texture values, independent modules, and texture-only output encoding.
- Avoid GPU readbacks, per-frame program compilation, and unnecessary image passes.
- Report performance with resolution, mode, color space, device, and timing method. Harness timings exclude Resolve and transfers.
- Include appropriate attribution and compatible licenses for any new dependencies or assets.
- Do not include private footage, credentials, build outputs, or local installation files.

For visual bugs, include Resolve version, operating system, GPU/driver, project color management, input/output choices, relevant parameters, and a minimal reproduction. Only share media you have permission to publish.

Contributions to project code use MPL-2.0. Keep third-party notices separate and intact. This is an experimental development project; parameter and plugin identifiers may change before a stable release.
