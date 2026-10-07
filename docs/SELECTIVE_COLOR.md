# Selective Color (v0.33)

An independent finishing module for a monochrome image with one retained hue
range. Available in Full and Color Only, disabled by texture-only and diagnostic
glow modes. Enable is the first control; disabling it greys out its options and
retains their values. Enable defaults to off and Amount to zero. Only the three
Graphic Noir recipes enable it automatically; all other built-in recipes disable
it and reset Amount/View. Switching Full/Color Only preserves an explicit Enable
choice; texture-only, bypass and glow-matte modes disable it, without automatic
re-enabling when returning. Saved projects and user preset files retain their
stored Enable choice; the new defaults do not rewrite existing nodes.

## Controls

| Control | Behavior |
| --- | --- |
| Amount | 0-1. Zero is unchanged; one removes all chroma outside the selection. Intermediate values retain partial background color. |
| Keep Hue | 0-360 degrees. Red 0/360, yellow 60, green 120, cyan 180, blue 240, magenta 300. |
| Hue Range | 0-180 degrees on either side of Keep Hue at full retention. Red wraps continuously across zero. |
| Hue Feather | 0-90 degrees of smooth falloff outside the retained range, capped at the opposite hue. Zero is a hard cutoff. |
| Minimum Saturation | 0-1 source HSV saturation threshold, with a smooth 0.05 transition below the cutoff. Exact neutral/black sources have no hue and are rejected. |
| View | Image or Selection Matte. White retains chroma, black rejects it, gray partially retains it. Matte is independent of Amount and output encoding, with unchanged alpha. |

Select Graphic Noir / Red, Blue or Yellow (Sin City) for complete high-contrast
recipes, or enable the module on your own look and raise Amount to one. For a red
accent, narrow Hue Range and increase Minimum Saturation if skin is also retained.
Use Selection Matte to inspect the key, then return to Image. It is possible for
skin and a desired red object to share both hue and saturation; these controls
cannot separate identical colors based on object identity.

## Pipeline And Limits

The key is computed from the source converted into the plugin's perceptual
Rec.709-primary sRGB working domain, after camera balance when Film Color is
enabled, but before negative, development, print, glow or grain. Hue/saturation
are not evaluated directly in camera-log code values. Negative source channels
are floored only for evaluating the key; HDR colors are not capped at one.

The mask controls final chroma removal after Print, glow, grain and the normal
Mono finishing stage, before output conversion. Unselected pixels become neutral
at the same working-domain weighted luminance; this is not a linear-light energy
preservation claim. Source keying prevents added texture or print balance from
changing the mask. Glow and colored grain cannot recolor fully rejected pixels.
Selected pixels retain the processed color, not a reconstruction of the original
source color.

Full-strength Mono Negative or an upstream black-and-white node has already
removed accent color. Selective Color cannot restore it. The Graphic Noir recipes
therefore use a color negative family and do their monochrome conversion here.
Only one hue interval is supported; no object recognition, tracked regional mask,
arbitrary multiple hue bands, recoloring or comic-book edge extraction is added.
Source noise can still affect the selection, especially with narrow hard keys.

Selective-only processing honors Output Color Space. Amount zero with Image view
skips the stage and preserves the previous pipeline; if it is the sole active
effect, that means exact RGBA pass-through, not standalone color conversion.
Selection Matte returns the direct mask rather than gamma/log-encoding it. An
inactive module or excluded mode ignores the matte setting.

CPU and OpenCL use the same math in the existing composite pass. There are no
additional image buffers, blur passes, downloads or preset file reads. The stage
is skipped at zero Amount/Image view, so old recipe RGB math is unchanged.

## Presets And Verification

All built-in recipes reset these six controls, preventing a prior selective look
from contaminating another recipe. Portable files use format version 2 to capture
the new controls and eighth module switch. Complete format-version-1 files still
load: existing values are retained, new controls receive neutral defaults and
the new module is disabled. No missing older field is silently accepted.

Tests cover primary hue centers and the red seam, continuous feathering, exact
zero/fully retained color, neutral and negative/HDR sources, working luminance,
raw matte/alpha, standalone output conversion, colored grain/glow finishing,
every input/output space, modes, independent/disabled switches, both queue types,
recipe round-trips and strict legacy migration. Native UI, Undo/reload and noisy
moving footage still need evaluation in Resolve. Color-only synthetic checks are
not proof of a shot-for-shot match to Sin City.
