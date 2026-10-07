# Texture Controls (v0.27)

## Film Gauge

Film Gauge is an original creative approximation, not a measured calibration of film stocks, camera gates, or scanned grain. It multiplies the existing sliders rather than overwriting them:

| Gauge | Grain size and halo spread | Grain strength |
| --- | ---: | ---: |
| Custom | 1.00x | 1.00x |
| 8 mm | 2.60x | 1.35x |
| Super 8 | 2.35x | 1.28x |
| 16 mm | 1.65x | 1.15x |
| Super 16 | 1.45x | 1.10x |
| 35 mm | 1.00x | 1.00x |
| Super 35 | 0.90x | 0.95x |
| 65 mm | 0.70x | 0.80x |
| 70 mm (15-perf) | 0.50x | 0.70x |

Custom and 35 mm currently render identically. Larger-format settings give finer, gentler grain and tighter halo spread; smaller-format settings give stronger, coarser texture. Fine/Classic/Rough grain styles, color, softness, roughness, and tonal weighting remain independently adjustable. Active full-strength Mono Negative automatically uses monochrome grain regardless of Grain Color; texture-only modes keep the requested grain color. Zero grain, halation, or aura strength stays zero under every preset, and module switches still take precedence.

Super formats use modestly finer/gentler texture than their standard counterparts. The 70 mm choice represents a 15-perf large-frame look, not a 5-perf 70 mm print of the same 65 mm negative. Kodak distinguishes [65 mm 5-perf and 15-perf capture and their 70 mm prints](https://www.kodak.com/en/motion/blog-post/dunkirk-imax/). Our multipliers are artistic recipes, not gate-area ratios or measured stock granularity. No preset crops, resizes, changes aspect ratio, or adds borders; Bloom and film/print color response are unchanged.

v0.23 orders the expanded dropdown by format size. Older development projects store numeric gauge choices, so indices after 8 mm are not migrated; reselect the intended gauge when opening an older node. The original Custom/8/16/35/65 mm rendering recipes are retained.

## Advanced Grain

Grain Softness now extends to two: 0..1 retains the original fine-detail reduction; 1..2 also smooths the primary grain field, with normalized variance and unchanged lattice pitch/seed. See [Slider Tuning](SLIDER_TUNING.md).

`Horizontal Stretch` is a horizontal desqueeze ratio from 0.5 to 2.0, default 1. A value of 2 doubles the horizontal scale of both noise layers before their rotations; vertical scale and image dimensions remain unchanged. There is no resampling of footage. Noise variance normalization remains unchanged, so stretch primarily changes structure, not strength. It is independent of Film Gauge and resolution scaling.

`Red Grain`, `Green Grain`, and `Blue Grain` multiply each channel's noise delta by 0-2, default 1. Zero removes grain from that working-space channel; source color is not multiplied. Grain Color at zero uses a shared monochrome noise field, but unequal channel multipliers can tint that field. Active full-strength Mono Negative still finishes the final composite monochrome, including unequal channel gains. The channel controls operate in the managed Rec.709-primary working space, not individual camera-gamut channels.

Enabled Film Development in Full mode scales grain strength by `2^(0.22 * Push/Pull)` without changing grain size or sampling coordinates. At a fixed frame/seed, adjusting Push/Pull preserves the noise field; tonal weighting follows the selected Grain Response stage. Zero Grain and the Grain Enable toggle still win. Grain Only and Halation, Bloom & Grain Only ignore development entirely, preserving their independent texture workflow.

### Grain Response

- `Post Print` (default): the original additive grain after Print, Halation/Aura, and Bloom, keyed by final working luminance. Existing defaults and built-in look recipes retain this path.
- `Negative & Print`: in Full mode with an active color stage, key grain from luminance after Film Color and Film Development, then add it before Print. Print exposure/balance, palette, tone, black lift, and gamut compression process the textured image. Color Density and development tone can affect the pre-print grain's tonal weighting without shifting its coordinates. Print changes affect the rendered texture but do not change the underlying noise field or pre-print tonal key.

This optional response uses the same procedural grain model and amplitude controls in the managed perceptual domain. It is not optical-density-domain chemistry or calibrated physical emulsion simulation. Without Print, it remains keyed before glow; Halation/Aura and Bloom are still composited afterward and their source keys are unchanged. Added glow does not receive an additional grain pass in this response.

Texture-only modes and Full with no active color stage always retain Post Print behavior, regardless of the stored choice. The selector is unavailable outside Full or when all color-stage module switches are off. Neutral Development alone also falls back to Post Print until it actually processes color. No-print/no-glow composition is identical between responses. Grain zero/disabled and bypass remain exact; Mono finishing still neutralizes the final composite.

The placement selector adds no GPU buffers, readbacks, blur passes, extra grain field evaluations, or extra per-pixel print evaluations. Native file preset operations also run only on button clicks, never in rendering. Real-footage appearance and GPU/host timing still need testing beyond the synthetic harness.

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

Tests cover all nine gauge choices and their recipes, gauge index bounds, seed/highlight/bloom isolation, smooth bounded highlight selection, threshold/transition activity, hue luminance, independent radii, gauge size/strength multipliers, zero-strength behavior, HD/4K normalized halo spread, odd-sized work grids through 8K height, CPU/OpenCL parity, and texture/color isolation. These are synthetic numerical checks, not comparison against measured film scans. OpenCL out-of-order queue checks run only when the device supports them.

An optional synthetic GPU preview compares all gauges across isolated lights and grain-only patches:

```powershell
.\build\ofx\HalationTests.exe analysis\grain-preview.bmp analysis\response-preview.bmp analysis\texture-preview.bmp
```

Columns are Custom, 8 mm, Super 8, 16 mm, Super 16, 35 mm, Super 35, 65 mm, and 70 mm (15-perf). This deliberately strong test texture is not a recommended grading preset. No texture preview is produced without an OpenCL device.
