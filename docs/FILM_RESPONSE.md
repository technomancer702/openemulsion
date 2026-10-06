# Negative and Print Response (v0.12)

## Current Engine

Film Response is the only negative/print engine. There is no response-model selector, unmanaged input, or compatibility path. All response controls are available immediately. New instances default to input `Rec.709 / Gamma 2.4` and output `Same as Input`; select the actual space entering the node before grading. This development update changes input choice indices, so recreate old test instances rather than relying on their stored selections.

Film Color and Print remain independent. Full can use both, either, or neither. Grain Only, Halation & Grain Only, Bypass, and Halation Matte never use the negative/print response controls. Disabling Film Color also disables camera exposure/white balance; disabling Print disables all its tone and balance adjustments. Turning both off retains the texture-only input/output behavior documented in [Color Spaces](COLOR_SPACES.md).

The new engine is an original artistic model, not a measured stock calibration, physical emulsion simulation, or reproduction of Filmbox's private implementation. The six film families are creative profiles. There is no new LUT, reference-image dependency, extra image pass, GPU readback, or per-frame texture allocation.

## Negative

The negative uses a shared luminance response, palette mixing, saturation/density, and optional soft gamut compression. Unlike independent RGB tone curves, luminance shaping does not itself rotate hue. Each film family has its own contrast, saturation, and color-mixing profile; the monochrome family uses its own RGB weighting.

- `Exposure`, `Temperature`, `Tint`: camera controls that balance linear light before the negative. For log footage choose its actual encoding.
- `Contrast`: adjusts slope around a fixed middle-gray pivot. It no longer moves middle gray simply because contrast changes. The pivot corresponds to scene-linear 0.18 encoded in the managed perceptual work domain.
- `Toe`: rounds the lower shadows while retaining nonzero detail. Maximum Toe does not impose a hard black clipping threshold.
- `Negative Shoulder`: higher values begin highlight compression earlier and reduce its upper headroom. It remains smooth at the transition.
- `Color Crosstalk`: blends each film family's color-mixing matrix with identity. Zero removes the matrix's palette mixing, not its contrast/saturation profile. It is separate from Grain Color.
- `Negative Density`: higher values darken saturated colors to add density, leaving neutral grays unchanged. Negative values brighten those colors.
- `Saturation`: overall negative colorfulness, combined with the selected film-family profile.
- `Gamut Compression`: softens colors approaching or exceeding the effect's working RGB boundary. Higher values begin compression earlier; zero disables this negative-stage compression. Hue/luminance are retained by radial chroma scaling. This is not a standardized ACES gamut compressor or an output-gamut mapping guarantee.
- `Skin Hue`: positive shifts selected warm midtone colors toward magenta; negative shifts toward green. Selection is a soft RGB color region, not face detection, and can also affect similarly colored objects. Neutral, blue, and green colors are excluded. Mono ignores it.

The neutral axis is preserved by the new negative palette and density logic. Palette changes, saturation, input gamut conversion, and intentional tone mapping can still change the appearance of colored objects. Saturation/compression set to extreme values is a creative override, not a colorimetric correction.

## Print

Contact is the strongest print-like response; Standard balances that with softer tone and broader color; Telecine is gentler and more neutral. Custom is an adjustable middle starting point. Styles select a base profile, while sliders modify it: selecting a style does not reset or silently override slider values.

- `Print Tone`: -1 emphasizes the print-like toe/contrast character; +1 makes it gentler and cleaner. Zero uses the selected style's base profile.
- `Print Contrast`: a multiplier on the style's midtone contrast; 1 uses its base contrast.
- `Highlight Rolloff`: higher values start the shoulder earlier. Highlights approach the upper level smoothly instead of folding downward at large inputs.
- `Print Color`: 0 applies the full style palette/cold-shadow/warm-highlight character; 1 removes that palette/cast character. Tone and output-gamut compression still operate.
- `Neutralize Print`: removes only the tone-dependent shadow/highlight cast. It does not undo the film-family palette, print matrix, saturation, toe, or shoulder. At 1, neutral input remains neutral with default RGB print balance.
- `Print Saturation`: multiplies the style's colorfulness independently of Film Color saturation. A value of 0 removes chroma before any enabled print cast; set Neutralize Print to 1 for a fully monochrome print result.
- `Black Point`: adjusts a neutral lifted floor; 0 sets the floor to zero. Contact lifts more than Standard, and Telecine lifts less.
- `Print Exposure`: creative print-side exposure in linear-light stops before the print tone response; positive values brighten. It does not change the negative or source-keyed halation.
- `Print Red`, `Print Green`, `Print Blue`: independent linear-light channel exposure offsets in stops, added to Print Exposure. These are creative RGB balance controls, not calibrated physical printer-point units.

Black Point and Print Color work with every print style.

New print response approaches a bounded SDR-like perceptual white and uses hue-preserving radial compression. It is not a full HDR rendering transform; the existing output-space conversion remains responsible for encoding the result for the rest of your pipeline.

## GPU and Tests

The frame-level configuration prepares profile matrices, curve coefficients, and print gains once. A 152-byte response structure is passed to the existing OpenCL image kernel. The default tone/color response uses rational/polynomial arithmetic, not per-pixel logs or a sampled LUT. Nonzero print RGB/exposure adjustments additionally use the existing shared linear/perceptual conversion functions.

C++ and OpenCL compile the same response header. Tests cover monotonic HDR ramps through 1,000,000 work-domain units, continuous curve slopes, stable gray pivots, neutral axes/floors, hue/luminance-preserving gamut compression, skin-region isolation, density, slider activity with every print style, CPU/GPU parity across all 15 input choices and 24 film/print combinations, all module masks, independent composition, and exact texture/matte/bypass isolation.

Run `ctest --test-dir build/ofx --output-on-failure`. To generate grain and response comparison charts:

```powershell
.\build\ofx\HalationTests.exe analysis\grain-preview.bmp analysis\response-preview.bmp
```

Response-chart columns are Contact, Standard, Telecine, and Custom. Rows are neutral gray, warm/skin-like colors, saturated color ramps, and a 16-stop gray exposure ramp. These are synthetic diagnostics, not validation against real skin, film scans, or Resolve footage. UI layout and final appearance should be checked in Resolve.

## Public Design Context

[Filmbox's public negative controls](https://videovillage.com/learn/filmbox/full-guide/negative/color-and-tone) separate color/tone selection, skin rendition, and saturated-value handling. Its [public print guide](https://videovillage.com/learn/filmbox/full-guide/print-module) distinguishes printed and digitally transferred responses and independently exposes tone, color, balance neutralization, and black level. Those workflow ideas inform the control surface; our curves, matrices, profiles, and compression are original approximations, not inferred proprietary equations.
