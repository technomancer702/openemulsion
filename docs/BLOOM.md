# Bloom (v0.18)

Independent neutral/source-colored highlight diffusion, separate from the red/amber Halation and Aura. Bloom has its own source key and radius. It is available in Full and Halation, Bloom & Grain Only; Color Only, Grain Only, Bypass, and Halation Matte exclude it.

The first control is Enable. Strength defaults to zero, preserving the previous look and skipping bloom extraction, blur, and composite. No bloom working textures are allocated until it is used. Turning it off retains all settings.

## Controls

- `Bloom`: 0-2, default 0. Strength of the diffused light added to the image. Try 0.2-0.5 as a starting point on footage with bright lamps, windows, or reflections.
- `Bloom Radius`: 0-2, default 1. Scales the spread, using a 1080-line reference. Zero is the smallest nonzero diffusion width, not a bypass. A larger radius redistributes the same extracted energy, so the peak can become softer/dimmer. Independent of Halation/Aura radii and Film Gauge.
- `Highlight Threshold`: 0-2, default 0.65. Lower values include darker sources; higher values restrict extraction to brighter sources. This uses perceptual sRGB-transfer/Rec.709-primary working-domain units, not camera log code values, stops, or output nits. The threshold is converted to linear light before selection.
- `Highlight Transition`: 0.01-2, default 0.35. Width above the threshold over which source contribution rises smoothly. Its endpoints are converted to linear light, and the smoothstep is evaluated there.
- `Source Color`: 0-1, default 1. One retains source-highlight color; zero neutralizes the extracted light at the same linear luminance. Intermediate values mix the two. White sources stay neutral. This is independent of Halation's Red / Amber control.
- `Protect Highlights`: 0-1, default 0.8. Attenuates light added to already bright destination pixels while retaining diffusion into dark surroundings. At one, destination pixels with a linear RGB peak of at least one receive no additional bloom. This is not a hard output clamp or a complete highlight-recovery tool.

Full-strength Mono Negative neutralizes the final composite after bloom and grain. Bloom remains visible as neutral diffusion; it does not leak colored halos into Mono.

## Matte and Your Own LUT

`Bloom Matte` displays the source-colored/neutral diffused contribution over black, at the selected Bloom strength. It is a direct sRGB-transfer diagnostic image, not a camera-log image. It excludes the image/grade, Halation, Aura, Grain, output conversion, Mono finishing, and destination highlight protection. Alpha is preserved. Zero strength or a disabled Bloom module produces a black matte.

For bloom alone with your own grade/LUT, choose Halation, Bloom & Grain Only and disable Halation, Aura, and Grain. Select the actual Input Color Space. Texture-only rendering returns the input gamut/encoding regardless of the output dropdown. Alternatively disable Film Color, Film Development, and Print in Full mode.

Bloom selection uses the original source, before camera balance, negative response, development, print, halo, or grain. Those adjustments do not change which source pixels are extracted. Destination highlight protection does depend on the processed image, so changes to the grade can change where added bloom is attenuated. Composite order is Film Color, Development, Print, Halation/Aura, Bloom, Grain, then Mono finishing and output encoding.

## Original Model and Limits

The source is decoded and converted to linear Rec.709 RGB. Negative source components are excluded from extraction. A smoothstep of the largest positive linear channel selects highlights. Extracted RGB is scaled by `key / (1 + peak)`, bounding extreme source energy while preserving its chromatic direction. Source Color blends toward linear luminance before filtering.

Area-averaged extraction preserves small lights on a resolution-scaled work grid. A dense normalized separable Gaussian and bilinear reconstruction give continuous diffusion without displaced copies of the source. The Gaussian sigma in source pixels is approximately `6 + 20 * Radius` at 1080 lines. The kernel is truncated at three sigma. Default work-grid steps are 4 pixels at HD, 8 at 4K, and 16 at 8K, bounded between 4 and 32. Tiny frames have a minimum filter width. Edge clamping preserves uniform fields; lights right at the image boundary do not have the same energy distribution as centered lights.

The diffused RGB is added in linear light with gain `0.35 * Bloom`, then returned to the perceptual working domain. Highlight protection modulates that gain by `1 - ProtectHighlights * smoothstep(0.25, 1, destinationLinearPeak)`. Zero contribution bypasses this decode/encode exactly.

This is an original artistic approximation, not measured optical scattering or a reconstruction of Dehancer's private model. It is a bounded source-colored diffusion layer, not an energy-conserving lens simulation: the original image is retained and additional light is added. It has no object tracking, calibrated lens model, or temporal accumulation. Film Gauge deliberately does not scale it. Proxy resizing and work-grid alignment can slightly change very small lights.

Bloom may raise values above one and cannot restore clipped source detail. Highlight protection reduces added light but does not guarantee an output gamut or SDR white limit. Existing enabled film/print finishing and downstream output management remain responsible for the final range. Bloom-only operation preserves negative/HDR source values away from its contribution.

## Performance and Verification

Nonzero bloom adds three OpenCL passes: extraction and horizontal/vertical blur. Composition remains in the existing image kernel. Two working RGB buffers are reused per context/device/queue; at ordinary HD/4K dimensions they use approximately 4 MiB total. Only filter coefficients are uploaded per render. There is no GPU image readback, frame-texture upload, or full-resolution blur allocation. Queue dependencies also cover bypass transitions and resizing. CPU fallback uses the same math and a multithreaded separable blur.

Tests cover bounded extraction, threshold/transition endpoints, source/neutral color and luminance, every control's activity, exact zero/disabled behavior, highlight protection, conserved centered-light energy, symmetric continuous halos, radius activity, uniform edges/tiny/odd grids, normalized HD/4K spread, Mono, alpha, every input/mode/module mask, texture-only encoding, CPU/OpenCL parity, and queued bypass/resizing. Out-of-order queue tests are skipped explicitly if the GPU driver does not support them. Resolve UI, saved-project reload/animation, and real-footage appearance still need host validation.

The benchmark reports paired bloom-off/on cases in Full and texture-only modes. GPU-resident timings exclude Resolve, transfers, and other nodes and are not timeline playback guarantees.

Optional synthetic preview (sixth path argument):

```powershell
.\build\ofx\HalationTests.exe analysis\grain.bmp analysis\response.bmp analysis\texture.bmp analysis\mono.bmp analysis\development.bmp analysis\bloom.bmp
```

Bloom columns are Original, Tight, Broad, Neutral, and Matte; rows contain white, red, and blue sources. These deliberately strong settings are diagnostics, not recommended grading presets.

[Dehancer's public bloom guide](https://www.dehancer.com/learn/articles/bloom-how-it-works) discusses local light diffusion, source selection, spread, and highlight protection. Those workflow ideas inform our controls; our extraction, blur, and composition equations are original.
