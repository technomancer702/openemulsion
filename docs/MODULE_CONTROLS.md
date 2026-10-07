# Module Controls (v0.32)

Disabled modules retain their settings but grey out every option inside the module. The Enable toggle remains editable if the current mode permits that module. Modules excluded by the mode have unavailable toggles and controls, regardless of any stored Enable value.

In active Mono Negative, Color Crosstalk and Skin Hue are unavailable at any strength because this family does not use them. At full Film Color Strength, Saturation, Color Density, Gamut Compression, and Grain Color also become unavailable. Their stored values are retained. Lowering Film Color Strength below one, changing Film System, disabling Film Color, or entering texture-only modes restores the applicable controls. Strength and system edits refresh this policy alongside mode/module/time changes. See [Slider Tuning](SLIDER_TUNING.md).

Grain Response additionally requires Full mode and at least one enabled color-stage module switch. Neutral Development alone may remain editable but falls back to Post Print until its processing is non-neutral. User preset Save/Load and import-preservation options remain available in every mode; they are workflow controls, not processing modules.

A user edit to Mode sets the Enable toggles as follows:

| Mode | Enabled modules |
| --- | --- |
| Full | Film Color, Film Development, Print, Halation, Aura, Bloom, Grain, Selective Color |
| Color Only | Film Color, Film Development, Print, Selective Color |
| Halation, Bloom & Grain Only | Halation, Aura, Bloom, Grain |
| Grain Only | Grain |
| Bypass | None |
| Halation Matte | Halation, Aura |
| Bloom Matte | Bloom |

Selecting a different mode replaces manual Enable choices with that mode's module set. Returning to Full enables all modules; individual modules can then be disabled again. The operation does not alter strengths, numeric controls, seeds, selected styles, or print recipes. An enabled effect with zero strength stays at zero; mode selection does not invent an effect amount.

Print's recipe controls require both an active Print module and Custom style. Re-enabling Print does not unlock a named preset, and selecting Custom does not unlock a disabled Print module. Strength and exposure/balance controls remain subject only to the Print module's state.

Input Color Space is unavailable when no module applies. Output Color Space is unavailable without an enabled color-processing module. Film Gauge is unavailable without enabled Halation, Aura, or Grain; Bloom is independent of gauge. Neutral Film Development remains editable even though rendering skips its neutral math. These UI rules use the configured module switches, not effect strengths.

Halation's highlight threshold, transition, and tint still also influence Aura. Disabling Halation greys out those controls with the rest of that module; Aura retains their stored values and its own editable strength/radius.

## Notifications and Animation

Mode changes are grouped into an OFX edit block. Existing Enable animation is preserved: a mode edit changes the current-time key when that toggle already has keys, otherwise it changes the constant value. Other keys are not deleted. Plugin-generated callbacks are guarded against recursive mode updates.

Construction, plugin-edit notifications, and timeline notifications refresh control availability without reapplying mode presets or changing stored values. Animated Mode values still mask the stored Enable values during rendering; moving the playhead is not treated as a fresh mode-preset selection. To configure different module sets over time, animate the Enable toggles as well.

## Verification

The standalone ModuleControlState regression checks all seven modes, all 256 Enable masks, toggle synchronization, control mappings, global-selector availability, neutral Development, print locking, sequential mode transitions, and Mono semantic availability in every system and at partial/full strengths. The CPU/OpenCL regression separately covers slider endpoints, grain smoothing and Selective Color.

Actual greying, edit-block undo/redo, animated controls, and save/reload still require checks inside Resolve. The original UI-policy change added no processing work; v0.32's Selective Color uses the existing composite pass and is neutral by default. See [Selective Color](SELECTIVE_COLOR.md).
