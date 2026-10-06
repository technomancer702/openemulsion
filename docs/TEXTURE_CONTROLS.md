# Texture Controls (v0.18)

## Film Gauge

Film Gauge is an original creative approximation, not a measured calibration of film stocks, camera gates, or scanned grain. It multiplies the existing sliders rather than overwriting them:

| Gauge | Grain size and halo spread | Grain strength |
| --- | ---: | ---: |
| Custom | 1.00x | 1.00x |
| 8 mm | 2.60x | 1.35x |
| 16 mm | 1.65x | 1.15x |
| 35 mm | 1.00x | 1.00x |
| 65 mm | 0.70x | 0.80x |

Custom and 35 mm currently render identically. Larger-format settings give finer, gentler grain and tighter halo spread; smaller-format settings give stronger, coarser texture. Fine/Classic/Rough grain styles, color, softness, roughness, and tonal weighting remain independently adjustable. Active full-strength Mono Negative automatically uses monochrome grain regardless of Grain Color; texture-only modes keep the requested grain color. Zero grain, halation, or aura strength stays zero under every preset, and module switches still take precedence.

## Advanced Grain

`Horizontal Stretch` is a horizontal desqueeze ratio from 0.5 to 2.0, default 1. A value of 2 doubles the horizontal scale of both noise layers before their rotations; vertical scale and image dimensions remain unchanged. There is no resampling of footage. Noise variance normalization remains unchanged, so stretch primarily changes structure, not strength. It is independent of Film Gauge and resolution scaling.

`Red Grain`, `Green Grain`, and `Blue Grain` multiply each channel's noise delta by 0-2, default 1. Zero removes grain from that working-space channel; source color is not multiplied. Grain Color at zero uses a shared monochrome noise field, but unequal channel multipliers can tint that field. Active full-strength Mono Negative still finishes the final composite monochrome, including unequal channel gains. The channel controls operate in the managed Rec.709-primary working space, not individual camera-gamut channels.

Enabled Film Development in Full mode scales grain size by `2^(0.12 * Push/Pull)` and strength by `2^(0.22 * Push/Pull)`. Zero Grain and the Grain Enable toggle still win. Grain Only and Halation, Bloom & Grain Only ignore development entirely, preserving their independent texture workflow. Grain remains additive after print/halo, keyed by final working luminance; negative-density-driven, pre-print grain is still future work.

## Highlight Selection

Halation and Aura share a source-highlight key derived before camera balance, negative response, print response, or grain. Film color, print exposure, and their strength controls do not change which sources produce a halo.

- `Highlight Threshold`: the lower cutoff. Higher values restrict the effect to brighter sources. The units are managed perceptual working-space values, not camera log code values, linear stops, or output nits. The default is 0.48.
- `Highlight Transition`: the width above that cutoff over which contribution rises smoothly to full strength. The default is 0.52; a smaller width makes selection more abrupt. A larger width includes the same starting level but gives intermediate highlights less contribution.
- `Red / Amber`: 0 is redder, 1 more amber, with constant working-space luminance. This affects both Halation and Aura. The default 0.5 matches the earlier red/amber balance. An active full-strength Mono Negative removes the final tint, retaining neutral halo intensity/spread.

The source key is a bounded smoothstep of `max(max(R,G,B), 1.2*luminance)` in the managed sRGB-transfer/Rec.709-primary working domain. Bright saturated colors can therefore produce a halo even if their luminance is relatively low. The new smooth transition differs from the earlier linear key; old threshold/amount combinations are not guaranteed to match visually.

`Halation Matte` shows the combined selected-and-spread signal, not just the unblurred key. Its orange diagnostic tint remains fixed, so the Red / Amber control does not recolor the matte.

## Spread and Resolution

`Halation Radius` controls the tight Gaussian spread. `Aura Radius` independently controls the broader Gaussian spread. Neither changes source selection or the other radius. The corresponding amounts control intensity; raising a radius spreads energy over a wider area rather than adding energy. Increasing radius can therefore lower the peak brightness around an isolated light. Aura is a shared-key broad halo, not neutral/source-colored bloom. The independent [Bloom module](BLOOM.md) uses its own source selection, linear RGB spread, and destination highlight protection.

Grain size and halo radii use a 1080-line reference and scale with rendered image height. The highlight working grid is area-averaged at 2-pixel steps for HD, 4-pixel steps for 4K, and 8-pixel steps for 8K. Dense separable Gaussian filtering and bilinear reconstruction avoid sparse rings of repeated light sources. The grid step is bounded between 2 and 16 pixels; very small images also use a minimum filter width.

Scaling preserves the approximate relative spread between HD and 4K, not identical pixels across host proxy/preview resolutions. Downsampling and resampling can alter the appearance of tiny lights. Full-resolution Resolve renders remain the final appearance check. The broadest 8 mm settings require more filtering work than 35 mm; disable unused modules or set their amounts to zero to skip their work.

## Your Own LUT

Use Grain Only or Halation, Bloom & Grain Only, or disable Film Color, Film Development, and Print in Full mode. Choose the input color space actually entering this node. Texture-only rendering returns that same encoding, regardless of the output selector. Turning color/tone strengths to zero alone is not equivalent to disabling those modules: camera/print exposure and color-space conversion still operate.

## Verification

Tests cover smooth bounded highlight selection, threshold/transition activity, hue luminance, independent radii, gauge size/strength multipliers, zero-strength behavior, HD/4K normalized halo spread, odd-sized work grids through 8K height, CPU/OpenCL parity, and texture/color isolation. These are synthetic numerical checks, not comparison against measured film scans. OpenCL out-of-order queue checks run only when the device supports them.

An optional synthetic GPU preview compares all gauges across isolated lights and grain-only patches:

```powershell
.\build\ofx\HalationTests.exe analysis\grain-preview.bmp analysis\response-preview.bmp analysis\texture-preview.bmp
```

Columns are Custom, 8 mm, 16 mm, 35 mm, and 65 mm. This deliberately strong test texture is not a recommended grading preset. No texture preview is produced without an OpenCL device.
