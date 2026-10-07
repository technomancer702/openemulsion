# User Presets (v0.27)

The User Presets group sits below the built-in Preset dropdown. Save Preset and Load Preset open native Windows dialogs for portable `.oepreset` files. The files contain UTF-8 JSON, not executable code or external LUT references. They are separate from Filmbox/Dehancer preset formats.

## Save and Load

Save captures the current frame's complete creative settings, module switches, Grain Response, camera balance, input/output spaces, and grain seed. The file's name supplies its metadata name. It saves numeric snapshots, not keyframe curves, footage, project data, or the current built-in preset label. Saved fixed Print Styles remain fixed; selecting Custom afterward uses the normal inherited recipe behavior.

Load validates the complete document before modifying parameters. Successful import is grouped into one host edit block, sets the built-in Preset dropdown to Custom, and refreshes module/print/semantic greying. Imported creative controls and module toggles replace their keyframes, as built-in looks do. Their stored values are authoritative; undo/redo, project reload, and time notifications do not read a preset file or reapply its recipe.

Three non-animated import options default to on:

- Preserve Color Spaces: keep this node's Input and Output Color Space settings and keys.
- Preserve Camera Balance: keep Film Color Exposure, Temperature, and Tint settings and keys. Print Exposure/Red/Green/Blue remain part of the imported creative look.
- Preserve Grain Seed: keep this node's grain seed and keys.

Disable an option to import its saved context instead, replacing that group's keyframes. These options affect file loading only; export always captures all groups and built-in looks retain their existing preservation policy. Invalid context is rejected even if its group is preserved, so a malformed file cannot become valid merely by changing import options.

Mode and all seven Enable switches are imported together without automatic mode-driven rewriting, preserving the exact saved module choices. A loaded Grain Only/Bypass/matte snapshot retains that mode. Grain Response remains independent of Mode; texture-only rendering always retains the original post-print workflow.

Canceling a dialog changes nothing. Filesystem failures are reported through the host's OFX message suite. Invalid files never partially apply. Host errors during parameter writes may require Undo; an edit block is not a transactional rollback of arbitrary host failures. Final dialog behavior, undo/redo, animated-node imports, and project reload should be verified in Resolve.

## File Format and Validation

Format version 1 stores `plugin`, `createdWith`, `name`, `controls`, `modules`, and `context`, alongside `formatVersion`. Creative settings use existing stable parameter identifiers, not display labels. Choices are numeric indices in the v0.27 control vocabulary. This is an initial format, not a guarantee that future control additions or choice reordering will load without a schema migration.

All known controls and switches are required. Parsing rejects unsupported versions/plugins, missing or unknown controls, wrong types, fractional choices, nonfinite/out-of-range numbers, duplicate fields, excessive nesting, trailing data, and files over 64 KiB. Names are limited to 256 UTF-8 bytes; paths use the native Unicode Windows APIs. JSON parsing/serialization use vendored MIT-licensed nlohmann/json v3.12.0, whose release checksum was verified upstream.

Saving first serializes a validated snapshot. It creates a uniquely named temporary file beside the destination, writes and flushes it, then replaces the destination on the same filesystem. A failed write/replacement cleans up its temporary file and leaves the previous preset in place. The native Save dialog asks before overwriting. Do not save over an unrelated file.

File access occurs only when Save/Load is clicked. Rendering never parses JSON, opens dialogs, reads presets, or performs filesystem work.

## Checks

Tests cover exact round-trips for all built-in recipes, every context-preservation combination, complete creative/toggle import, strict malformed-input rejection, Unicode filenames, replacing an existing file, locked-destination failures, previous-file preservation, and temporary-file cleanup. Render tests cover the new Grain Response independently. These are not a substitute for native dialog/host undo checks or real-footage look evaluation.
