# Negative and Print Response (v0.26)

## Current Engine

Film Response is the only negative/print engine. There is no response-model selector, unmanaged input, or compatibility path. Film controls are available immediately; print-recipe controls are editable in Custom. New instances default to input `Rec.709 / Gamma 2.4` and output `Same as Input`; select the actual space entering the node before grading. Earlier development builds changed input choice indices; check stored input/output selections when using old test nodes. Recreate pre-v0.14 print test nodes or reselect a named print style to populate its displayed recipe under the new workflow.

Film Color and Print remain independent. Full can use both, either, or neither. Grain Only, Halation, Bloom & Grain Only, Bypass, Halation Matte, and Bloom Matte never use the negative/print response controls. Disabling Film Color also disables camera exposure/white balance; disabling Print disables all its tone and balance adjustments. With Film Development also disabled or neutral, turning both off retains the texture-only input/output behavior documented in [Color Spaces](COLOR_SPACES.md).

The engine is an original artistic model, not a measured stock calibration, physical emulsion simulation, or reproduction of Filmbox's private implementation. The six film families are creative profiles. There is no new LUT, reference-image dependency, extra image pass, GPU readback, or per-frame texture allocation.

## Negative

v0.25 expands Temperature/Tint and Skin Hue to -3..3, Color Crosstalk to 0..3, and Color Density (previously Negative Density) to -1.2..1.5. Existing values/defaults/presets retain their response; above-one Crosstalk intensifies the original matrix rather than changing the meaning of one. See [Slider Tuning](SLIDER_TUNING.md). v0.26 updates labels and tooltips only; see [Control Names](CONTROL_NAMES.md).

The negative uses a shared luminance response, palette mixing, saturation/density, and optional soft gamut compression. Unlike independent RGB tone curves, luminance shaping does not itself rotate hue. Each film family has its own contrast, saturation, and color-mixing profile; the monochrome family uses its own RGB weighting.

- `Film Color Strength`: 0 removes the film palette, monochrome conversion, Skin Hue, saturation, density, and negative gamut compression. 1 applies them fully; intermediate values blend the color operations. Intentional color density can still alter luminance. Tone shaping and camera exposure/balance remain independent.
- `Film Tone Strength`: 0 removes negative contrast, toe, and shoulder; 1 applies the selected response fully. Intermediate values interpolate the unshaped and shaped luminance response. Color operations remain independent.
- `Exposure`, `Temperature`, `Tint`: camera controls that balance linear light before the negative. For log footage choose its actual encoding.
- `Contrast`: adjusts slope around a fixed middle-gray pivot. It no longer moves middle gray simply because contrast changes. The pivot corresponds to scene-linear 0.18 encoded in the managed perceptual work domain.
- `Toe`: rounds the lower shadows while retaining nonzero detail. Maximum Toe does not impose a hard black clipping threshold.
- `Negative Shoulder`: higher values begin highlight compression earlier and reduce its upper headroom. It remains smooth at the transition.
- `Color Crosstalk`: blends each film family's color-mixing matrix with identity. Zero removes the matrix's palette mixing, not its contrast/saturation profile. It is separate from Grain Color.
- `Color Density`: higher values darken saturated colors to add density, leaving neutral grays unchanged. Negative values brighten those colors. This is a creative color-density adjustment, not calibrated photographic optical density.
- `Saturation`: overall negative colorfulness, combined with the selected film-family profile.
- `Gamut Compression`: an amount control that blends toward a fixed soft radial compression target in the perceptual working RGB domain. Zero disables this negative-stage compression, 0.5 applies half its RGB change, and 1 applies the full target (knee 0.45). Working-space weighted brightness and chroma direction are retained, not physical scene-linear luminance or perceptual hue in a color appearance model. Partial amounts can retain negative or above-boundary values; final negative-channel clamping, Print, and downstream color management still affect the result. This is not a standardized ACES gamut compressor or an output-gamut mapping guarantee.
- `Skin Hue`: positive shifts selected warm midtone colors toward magenta; negative shifts toward green. Selection is a soft RGB color region, not face detection, and can also affect similarly colored objects. Neutral, blue, and green colors are excluded. Mono ignores it.

### Mono Negative

At Film Color Strength 1, Mono Negative now finishes the entire print/texture composite on the neutral axis before output encoding. Halation and Aura become neutral glow rather than red/amber tint. Grain automatically uses monochrome noise, retaining its amplitude rather than averaging away colored noise. Print color and RGB balance can change brightness but cannot leave a colored final image.

Lower Film Color Strength progressively restores color both in the negative and in the final composite; it is not a single global crossfade between two fully rendered endpoints. At zero, the extra monochrome finishing is disabled. The Mono system choice has no effect when Film Color is disabled or in Grain Only, Halation, Bloom & Grain Only, Bypass, or Halation Matte. The diagnostic matte remains orange.

The finishing calculation is shared by C++ and OpenCL and runs inside the existing image pass. Tests cover strong halation, Aura, colored-grain settings, every print style, all input/output spaces, partial strengths, alpha, preserved texture activity, and exact disabled-stage isolation.

The neutral axis is preserved by the new negative palette and density logic. Palette changes, saturation, input gamut conversion, and intentional tone mapping can still change the appearance of colored objects. Saturation/compression set to extreme values is a creative override, not a colorimetric correction.

### v0.21 Compression Fix

Earlier versions bypassed compression at exactly zero, but applied the full radial operation at every positive value. The slider only moved its knee from 0.98 toward 0.45, causing an abrupt change near zero on out-of-gamut colors and much smaller changes afterward. It now blends toward the fixed maximum-compression target, so equal slider steps produce equal working-RGB changes before clamping/encoding. The default 0.5 is genuinely half strength; synthetic out-of-gamut red chips retain more red-channel intensity than with the old default. Full strength retains the previous maximum target, and zero retains the negative-only disabled result. Input conversion, palette/density/tone equations, and Print equations are unchanged. This fixes a confirmed control discontinuity, not a claim of a viewing-LUT match or a complete diagnosis of any specific source clip.

Partial compression can leave negative channels. An active negative response now applies its existing negative-channel floor at the stage boundary, before Development/Print/texture, as well as retaining the final film/print floor. This makes negative-then-print composition consistent with two separate nodes instead of allowing channels that a negative-only node would discard to influence Print. Zero color and tone strengths skip the new stage floor and retain signed conversion-only values. Compression remains continuous, but channel-floor crossings and display encoding mean the visible change need not be perfectly linear. Texture-only processing is unchanged.

## Film Development

The independent development stage runs between Film Color and Print. Neutral controls skip the stage completely; disabling it also removes Push/Pull's grain coupling. It does not depend on Film Color/Print strengths. See [Film Development](FILM_DEVELOPMENT.md) for controls, equations, and limitations. Full-strength Mono finishes the complete composite after development, so Split Tone cannot recolor Mono.

## Print

Full (Film Print) is the strongest print-like response. Standard balances that with softer tone and broader color. Extended (Telecine) is gentler, with a neutral gray axis and lower black point. These are original artistic presets, not Filmbox's numerical recipes.

Selecting Full, Standard, or Extended loads and grays out seven recipe knobs: Print Tone Curve, Print Contrast, Highlight Rolloff, Print Color, Neutralize Balance, Print Saturation, and Black Point. Custom unlocks the last selected recipe without changing its rendered result. There is one underlying response, with no hidden style-specific multipliers after switching to Custom.

| Recipe Knob | Full | Standard | Extended |
| --- | ---: | ---: | ---: |
| Print Tone Curve | -1 | 0 | 1 |
| Print Contrast | 1 | 1 | 1 |
| Highlight Rolloff | 0.55 | 0.55 | 0.55 |
| Print Color | 0.25 | 0.43 | 0.79 |
| Neutralize Balance | 0 | 0 | 1 |
| Print Saturation | 1 | 1 | 1 |
| Black Point | 0.585 | 0.45 | 0.135 |

Standard retains the earlier default print appearance within floating-point precision. Extended now neutralizes its print cast explicitly. Custom initially starts from Standard on a fresh instance, or from the last named style after a selection.

Named presets are enforced by frame-level configuration, so stale stored values cannot secretly alter their response. Selecting a named preset replaces Custom recipe tweaks and their animation; these knob changes are grouped in a host edit block. The Print Style selector is not animated, while the seven recipe knobs can be animated in Custom. Print Color/Tone Strength, Print Exposure, and Print RGB balance stay editable and are never reset by selecting a preset. Film Color and texture controls are also unaffected.

The following seven recipe controls apply only in Custom; the strength and exposure/balance controls apply in every style.

- `Print Color Strength`: 0 removes print palette, saturation, cast, and print gamut compression; 1 applies them fully. Intermediate values blend the color operations. Print tone and linear-light exposure/balance remain independent.
- `Print Tone Strength`: 0 removes print contrast, toe, shoulder, and lifted black point; 1 applies them fully. Intermediate values interpolate the tone response and scale the black lift. Print color and exposure/balance remain independent.
- `Print Tone Curve`: -1 emphasizes the print-like toe/contrast character; +1 makes it gentler and cleaner. Zero uses the common middle tone response.
- `Print Contrast`: a multiplier on the contrast selected by Print Tone Curve; 1 uses that response.
- `Highlight Rolloff`: higher values start the shoulder earlier. Highlights approach the upper level smoothly instead of folding downward at large inputs.
- `Print Color`: 0 applies the full print palette/cold-shadow/warm-highlight character; 1 removes that palette/cast character. Tone and output-gamut compression still operate.
- `Neutralize Balance`: removes only the tone-dependent shadow/highlight cast. It does not undo the film-family palette, print matrix, saturation, toe, or shoulder. At 1, neutral input remains neutral with default RGB print balance.
- `Print Saturation`: multiplies print colorfulness independently of Film Color saturation. A value of 0 removes chroma before any enabled print cast; set Neutralize Balance to 1 for a fully monochrome print result.
- `Black Point`: adjusts a neutral lifted floor; 0 sets the floor to zero. Full selects a higher value than Standard, while Extended selects a lower one.
- `Print Exposure`: creative print-side exposure in linear-light stops before the print tone response; positive values brighten. It does not change the negative or source-keyed halation.
- `Print Red`, `Print Green`, `Print Blue`: independent linear-light channel exposure offsets in stops, added to Print Exposure. These are creative RGB balance controls, not calibrated physical printer-point units.

Black Point and Print Color are part of every preset recipe and can be adjusted in Custom. `Print Color` adjusts palette/cast character only, while `Print Color Strength` also scales saturation and gamut compression.

Both strengths at zero remove the corresponding negative/print response, but do not bypass its exposure/balance or input/output conversion. Disable the module itself to bypass all its controls. With zero tone strength, HDR values are not forced through that stage's tone shoulder; enabled color operations may still compress chroma and intentionally change density.

New print response approaches a bounded SDR-like perceptual white and uses hue-preserving radial compression. It is not a full HDR rendering transform; the existing output-space conversion remains responsible for encoding the result for the rest of your pipeline.

## GPU and Tests

The frame-level configuration prepares profile matrices, curve coefficients, and print gains once. A 212-byte response structure is passed to the existing OpenCL image kernel. The default tone/color response uses rational/polynomial arithmetic, not per-pixel logs or a sampled LUT. Nonzero print RGB/exposure adjustments additionally use the existing shared linear/perceptual conversion functions.

C++ and OpenCL compile the same response header. Tests cover monotonic HDR ramps through 1,000,000 work-domain units, continuous curve slopes, stable gray pivots, neutral axes/floors, hue/luminance-preserving gamut compression, skin-region isolation, density, locked-recipe immunity in named styles and slider activity in Custom, CPU/GPU parity across all 15 input choices and 24 film/print combinations, all module masks, independent composition, and exact texture/matte/bypass isolation. Additional tests check independent color/tone endpoints, partial tone interpolation, retained exposure with strengths at zero, and CPU/OpenCL response parity at partial strengths in all input spaces using linear output. The existing managed output tests cover all encodings; linear comparisons avoid amplifying float cancellation at Gamma 2.4's near-black singularity for extreme synthetic wide-gamut colors.

Negative compression additionally has a 1001-step amount sweep across all film systems and zero/partial/full color and tone strengths, including negative-channel/HDR red chips. Tests assert continuous behavior at zero, uniform working-RGB interpolation, unchanged working brightness/neutral axes, full-target equality, and Print isolation. GPU-rendered negative-only checks cover endpoints, near-zero and intermediate amounts in every input space, preserved alpha, final negative-channel clamping, and CPU parity. No source clip or viewing LUT is used in these synthetic checks.

Run `ctest --test-dir build/ofx --output-on-failure`. To generate grain and response comparison charts:

```powershell
.\build\ofx\HalationTests.exe analysis\grain-preview.bmp analysis\response-preview.bmp
```

Response-chart columns are Full, Standard, Extended, and Custom (starting from Standard). Rows are neutral gray, warm/skin-like colors, saturated color ramps, and a 16-stop gray exposure ramp. These are synthetic diagnostics, not validation against real skin, film scans, or Resolve footage. UI layout, gray-out behavior, host undo/redo, saved-project reload, and final appearance should be checked in Resolve. Tests additionally verify exact preset-to-Custom rendered equality in all 15 input spaces and every mode, unchanged independent strength/exposure settings, and that stored locked-knob edits have no effect.

## Public Design Context

[Filmbox's public negative controls](https://videovillage.com/learn/filmbox/full-guide/negative/color-and-tone) separate color/tone selection, skin rendition, and saturated-value handling. Its [public print guide](https://videovillage.com/learn/filmbox/full-guide/print-module) distinguishes printed and digitally transferred responses and independently exposes tone, color, balance neutralization, and black level. The documented fixed-style-to-Custom workflow also informs our preset selection and inheritance behavior. Those workflow ideas inform the control surface; our curves, matrices, profiles, and compression are original approximations, not inferred proprietary equations.
