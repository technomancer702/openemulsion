# Slider Tuning (v0.25)

This is a targeted usability update, not a new film model. Defaults and built-in recipes retain their v0.24 values and response. Expanding the exposed ranges makes the same percentage of slider travel more expressive while preserving precise numeric adjustments.

The names below document the v0.25 release. v0.26 renames Negative Density to Color Density and Neutralize Print to Neutralize Balance, without changing their values or behavior. See [Control Names](CONTROL_NAMES.md).

## Expanded Ranges

| Control | Previous range | v0.25 range |
| --- | --- | --- |
| Temperature / Tint | -1 to 1 | -3 to 3 |
| Skin Hue | -1 to 1 | -3 to 3 |
| Color Crosstalk | 0 to 1 | 0 to 3 |
| Negative Density | -0.6 to 0.9 | -1.2 to 1.5 |
| Split Tone | 0 to 1 | 0 to 3 |
| Grain Softness | 0 to 1 | 0 to 2 |

Existing values do not receive a hidden multiplier or require migration. The Film Color/Development preparation clamps were expanded along with the UI so new endpoints actually render. Temperature/Tint remain creative linear-light balance units, not kelvin or calibrated chromatic adaptation.

Color Crosstalk at one is the original family's mixing matrix. Above one, it extrapolates that palette away from identity while retaining the palette stage's working-luminance correction. Different families still have different palette character/strength. Skin Hue retains its warm-color mask; it is not a global hue rotation or face detector. Negative Density still leaves neutral grays unchanged. Split Tone retains its neutral pivot and opposing shadow/highlight directions.

Higher creative settings can generate out-of-gamut colors. Negative/print compression and downstream color management still determine the final range. These expanded controls are not physical stock calibration.

## Grain Softness

Zero to one retains the original response: progressively remove the fine-detail noise layer. One to two additionally blends the primary lattice's interpolation weights toward a quadratic B-spline footprint. This softens granule edges without changing lattice pitch, seed, stretch, or nominal strength. The effective spatial footprint/correlation increases, as expected from smoothing; this is not a new grain-size multiplier.

Separable squared-weight normalization keeps the field variance approximately stable rather than merely fading grain away. The calculation is shared by CPU/OpenCL. It adds no image buffers, blur passes, GPU readbacks, or full-frame convolution. The upper range evaluates nine lattice corners, skips the unused detail field, and costs slightly more than the original maximum-softness endpoint; actual host/hardware timings vary.

The join at one is continuous. Built-in presets use softness values below one and retain their texture. Grain Amount zero, disabled modules, bypass, Mono finishing, and Push/Pull's stable geometry remain unchanged.

## Contextual Greying

When Film Color is active and Film System is Mono Negative, Color Crosstalk and Skin Hue are unavailable because the Mono family never uses its palette matrix or skin adjustment. At Film Color Strength exactly one, Saturation, Negative Density, Gamut Compression, and Grain Color are also unavailable because they cannot change the result. Stored settings are preserved. Partial strength restores those latter controls; a different system, disabling Film Color, or selecting a texture-only mode restores availability according to the normal module policy. Film Color Strength itself remains editable.

## Deliberately Unchanged

Exposure, contrast, saturation transfer functions, Toe, Negative Shoulder, gamut-compression mapping, print styles, halation/Aura, bloom, and grain amount/size/roughness were not amplified. Toe/shoulder remain selective tone controls; Print can reduce visible negative highlight differences. Neutralize Print remains a correction of the print cast, not a global palette bypass.

## Verification

- v0.24 color/grain numerical anchors for defaults and every built-in recipe.
- Expanded endpoint and finite color sweeps, skin/cool/neutral isolation, camera-gain positivity, split luminance/pivot checks, soft-lattice joins, grain pitch/seed preservation, and RMS/spatial correlation.
- Semantic UI policy in all modes, Enable masks, Film Systems, and partial/full strengths.
- CPU/OpenCL parity at new color endpoints in every input space/system; all grain styles, upper Softness values, stretched RGB grain, odd grids, alpha, disabled modules, and bypass.

These are synthetic tests, not real-film measurement or a substitute for Resolve footage/host UI checks. GPU-resident timing excludes Resolve, transfers, other nodes, and timeline decoding.
