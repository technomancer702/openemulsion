# User Presets (v0.36)

The User Presets group sits below the built-in Preset dropdown. Save Preset and Load Preset open native Windows dialogs for portable `.oepreset` files. The files contain UTF-8 JSON, not executable code or external LUT references. They are separate from Filmbox/Dehancer preset formats.

## Save and Load

Save captures all current host/UI control values immediately when clicked, before opening the file dialog: complete creative settings, module switches, Grain Response, camera balance, input/output spaces, and grain seed. Export does not sample the push-button callback's time or rebuild the selected built-in recipe. The file's name supplies its metadata name. It saves numeric snapshots, not keyframe curves, footage, project data, or the current built-in preset label. Saved fixed Print Styles remain fixed; selecting Custom afterward uses the normal inherited recipe behavior.

Load validates the complete document before modifying parameters. Successful import is grouped into one host edit block, sets the built-in Preset dropdown to Custom, and refreshes module/print/semantic greying. Imported creative controls and module toggles replace their keyframes, as built-in looks do. Their stored values are authoritative; undo/redo, project reload, and time notifications do not read a preset file or reapply its recipe.

Loading restores every saved rendering setting by default, including color spaces, camera balance, and grain seed. Three optional non-animated preservation switches default to off:

- Preserve Color Spaces: keep this node's Input/Output Color Space, Output Rendering, HDR Peak Luminance and HDR Reference White settings and keys.
- Preserve Camera Balance: keep Film Color Exposure, Temperature, and Tint settings and keys. Print Exposure/Red/Green/Blue remain part of the imported creative look.
- Preserve Grain Seed: keep this node's grain seed and keys.

Enable an option only to retain the destination node's context instead of importing that group's values and replacing its keyframes. Existing v0.27 nodes retain their stored Preserve settings: uncheck all three for a complete restore. These options affect file loading only; export always captures all groups and built-in looks retain their existing preservation policy. Invalid context is rejected even if its group is preserved, so a malformed file cannot become valid merely by changing import options.

Reusing a look requires the same incoming image/grade, compatible color management, and (for exactly matching animated grain) the same frame time and resolution. Presets do not include Resolve's external OFX blend/bypass controls, other nodes, project color management, or media. The v0.28 save fix cannot recover adjustments absent from a v0.27 file; resave from the tuned node.

Mode and all eight Enable switches are imported together without automatic mode-driven rewriting, preserving the exact saved module choices. A loaded Grain Only/Bypass/matte snapshot retains that mode. Grain Response remains independent of Mode; texture-only rendering always retains the original post-print workflow. Selective Color's controls and Image/Selection Matte view are also captured.

Canceling a dialog changes nothing. Filesystem failures are reported through the host's OFX message suite. Invalid files never partially apply. Host errors during parameter writes may require Undo; an edit block is not a transactional rollback of arbitrary host failures. Final dialog behavior, undo/redo, animated-node imports, and project reload should be verified in Resolve.

## File Format and Validation

Format version 5 stores `plugin`, `createdWith`, `name`, `controls`, `modules`, and `context`, alongside `formatVersion`. It adds HDR peak/reference white to output context, and permits PQ output/Standard HDR choices. Format 4 introduced Highlight Color Retention; format 3 introduced Output Rendering. Complete format-1/2/3/4 files remain readable and receive HDR defaults of 1000/203 nits, without changing existing output choices. Versions 1/2 migrate rendering to Conversion Only; 3/4 retain the stored policy. Versions 1/2/3 receive retention zero; version 1 also receives neutral, disabled Selective Color. Mixed/incomplete schemas and legacy files containing new HDR choices are rejected. New files require v0.36 or later; older builds cannot read format 5. Parameter identifiers and numeric choice IDs are stable. Highlight retention is creative, so is restored even with Preserve Color Spaces; HDR context is preserved by that option. New nodes still default to Auto.

All known controls and switches are required. Parsing rejects unsupported versions/plugins, missing or unknown controls, wrong types, fractional choices, nonfinite/out-of-range numbers, duplicate fields, excessive nesting, trailing data, and files over 64 KiB. Names are limited to 256 UTF-8 bytes; paths use the native Unicode Windows APIs. JSON parsing/serialization use vendored MIT-licensed nlohmann/json v3.12.0, whose release checksum was verified upstream.

Saving first serializes a validated snapshot. It creates a uniquely named temporary file beside the destination, writes and flushes it, then replaces the destination on the same filesystem. A failed write/replacement cleans up its temporary file and leaves the previous preset in place. The native Save dialog asks before overwriting. Do not save over an unrelated file.

File access occurs only when Save/Load is clicked. Rendering never parses JSON, opens dialogs, reads presets, or performs filesystem work.

## Checks

Tests exercise the same current-value capture helper used by the plugin with typed host parameter doubles, reject accidental time-sampled reads, verify all settings are captured once, retain the snapshot across simulated dialog-time changes, and restore all groups by default. They also cover exact round-trips for all built-in recipes, every context-preservation combination, complete creative/toggle import, strict malformed-input rejection, Unicode filenames, replacing an existing file, locked-destination failures, previous-file preservation, and temporary-file cleanup. Render tests cover Grain Response independently. These are not a substitute for native dialog/host undo checks or real-footage look evaluation.
