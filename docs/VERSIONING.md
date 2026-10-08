# Release Versioning

Starting with v0.44.0, OpenEmulsion uses `major.minor.patch` release numbers.

- Patch: fixes and small maintenance changes, such as `0.44.1`.
- Minor: features or substantial improvements, such as `0.45.0`; reset patch to zero.
- Major: incompatible changes after a stable release; reset minor and patch to zero.

The `0.x` series remains experimental. Minor versions can change rendering or compatibility; three-part numbering does not imply production stability. Historical versions, tags and release notes keep their original two-part numbers. Do not reset to `0.0.0` or rename published releases.

Edit the three numeric definitions in `ofx/src/PluginVersion.h` when releasing. The plugin descriptor's version label, exported preset `createdWith`, package filename, source archive and manifest all derive from this header. Update current-version documentation and release notes, build and run regressions, then commit and push. Do not create Git tags or GitHub releases automatically.

The OFX export struct has only major and minor fields. From v0.44.0, its major is the release major and its minor is `minor * 1000 + patch`. For example, `0.44.0` exports `0 / 44000` and `0.44.1` exports `0 / 44001`. This keeps host version ordering increasing, including relative to v0.43 (`0 / 43`). Patch values must be below 1000. Descriptor properties expose the actual three-part components and readable label; keep the plugin identifier unchanged.

New release manifests record the encoded OFX fields separately from the human-readable version. The archive verifier checks their correspondence against the binary. It also accepts historical two-part manifests using their original OFX encoding. Preset format versions are independent: v0.44.0 still uses format 7 and reads the same older presets. Rendering and controls are unchanged.
