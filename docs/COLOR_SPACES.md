# OFX Color Spaces (v0.41)

## Resolve Workflow

`Input Color Space` describes the RGB entering this node. It is not inferred from camera metadata or project settings.

- Unconverted Alexa LogC3 in a manually managed project: select `ARRI Alexa LogC3 / Wide Gamut 3 (EI 800)`. With Film Color/Print enabled, select output `Rec.709 / Gamma 2.4` and `Output Rendering: Auto` for an SDR look. Do not apply another LogC3-to-709 viewing transform afterward.
- Alexa already converted to DaVinci Wide Gamut/Intermediate by a CST or Resolve Color Management: select `DaVinci Wide Gamut / Intermediate` and output `Same as Input`. Keep the project's normal output transform.
- ACEScct timeline: select `ACEScct / AP1` with output `Same as Input`. This effect is a look, not an ACES Input/Output Transform.
- Your own camera LUT after this node: choose its camera input and `Halation, Bloom & Grain Only` or `Grain Only`. Those modes retain the camera encoding for the LUT.
- Your own LUT before this node: select the LUT's output space, not the original camera space.

Texture-only processing always returns the input gamut and encoding, regardless of the output dropdown. This also applies to Full mode with Film Color, Film Development, Print and Selective Color inactive. Texture still changes pixels, but no film color or print curves are applied. Bypass, all-disabled processing, and zero-strength texture preserve incoming RGBA exactly. Halation Matte, Bloom Matte and Selection Matte are direct diagnostic images, not camera-log images.

New instances default to input `Rec.709 / Gamma 2.4` and output `Same as Input`. Select the actual space entering the node; the plugin does not auto-detect it. There is no unmanaged input path. Input choice indices changed in this development update: recreate older test instances and select their input explicitly.

## Choices

The input dropdown contains these 15 combinations:

| Gamut | Encoding |
| --- | --- |
| ARRI Wide Gamut 3 | Alexa LogC3, EI 800 |
| ARRI Wide Gamut 4 | LogC4 |
| Sony S-Gamut3.Cine | S-Log3 |
| Sony S-Gamut3 | S-Log3 |
| DaVinci Wide Gamut | Intermediate |
| Rec.709 | Gamma 2.4 |
| Rec.709 primaries | sRGB |
| Blackmagic Wide Gamut | Film Gen 5 |
| REDWideGamutRGB | Log3G10 |
| Canon Cinema Gamut | Canon Log 2 |
| Canon Cinema Gamut | Canon Log 3 |
| Panasonic V-Gamut | V-Log |
| ACES AP1 | ACEScct |
| ACES AP1 | Linear (ACEScg) |
| Rec.709 | Linear |

Outputs: Same as Input, Rec.709/Gamma 2.4, DaVinci Wide Gamut/Intermediate, ACEScct/AP1, sRGB, Linear/Rec.709, and Rec.2100/PQ (Rec.2020). PQ is output-only; the 15 input choices are unchanged. Output conversion runs only while Film Color, non-neutral Film Development, Print, or non-neutral Selective Color is active. Diagnostic mattes are not output-encoded. This is not a standalone CST: disabling all effects means pass-through, not input-to-output conversion.

## Math and Limits

### SDR Viewing Response

`Output Rendering` sits below Output Color Space and is preserved when switching built-in recipes. It is independent of the Film/Print strength sliders:

- **Auto** (default): apply an original SDR viewing response for camera-log, DaVinci Intermediate, ACEScct or scene-linear input going to Rec.709/Gamma 2.4 or sRGB. Do not apply it to display-ready Rec.709/Gamma 2.4 or sRGB input, or to log/linear output. Same as Input therefore does not add rendering automatically.
- **Conversion Only**: retain the pre-v0.34 encoding/gamut conversion. Use when another stage supplies the viewing transform. This does not make an enabled creative film/print response an identity.
- **Standard SDR**: explicitly enable the viewing response for display output, including display-ready input if deliberately desired. It still does nothing for log/linear output, texture-only processing, bypass, or diagnostic mattes.
- **Standard HDR (PQ)**: explicitly render only Rec.2100/PQ output. Auto selects this HDR path for scene-log/linear input sent to PQ; neither applies SDR first. Standard HDR does nothing to SDR or managed log/linear destinations. Standard SDR does not tone-map PQ output.

The viewing response runs after enabled Film Color camera balance and before the creative negative, development and print stages. It does not replace or scale the creative recipe sliders. Clean Slate removes those creative effects but retains the selected viewing response. Auto thus provides a finished neutral SDR starting point for log input, not just a mathematical conversion.

The independent rational luminance curve maps scene-linear 18% gray to display-linear 0.12 (approximately 0.4134 in Gamma 2.4). Scene white 1 maps to approximately 0.69136 (0.85745 encoded); the shoulder starts at scene-linear 0.6, and brighter values approach display white continuously. Shadows remain monotonic down to zero, with no added pedestal or positive-shadow clipping. The old conversion mapped gray to 0.4894 and did not provide a display shoulder. This difference addresses washed-out direct log-to-display rendering; it is not automatic dehazing or exposure correction.

Chroma is rescaled with linear luminance, then compressed radially toward the neutral axis at the display-gamut boundary. In-gamut colors below the compression knee retain their chroma direction. From v0.38, bright saturated emitters blend toward a peak-aware shoulder rather than being forced too quickly toward neutral white. This trades highlight brightness for retained color/channel gradation, not a global saturation or exposure change. No stock-data import, ARRI LUT reproduction, or ACES rendering-transform equivalence is claimed. Creative print lift/casts and texture can still intentionally change the final black/white values.

v0.38 improved highlight color but still compressed intensity differences enough
to hide lens texture. From v0.39, SDR first reserves more contrast for very bright
colored emitters with a square-root input shoulder. For source peak `p > 1`,
`g = smoothstep(0.15, 0.75, (max-min)/max)` and `s = 2*g`, the compressed peak is
`p' = 1 + (p-1)/sqrt(1+s*(p-1))`. All three linear channels scale by `p'/p` before
the existing viewing/gamut shoulders. A zero gate or peak at/below one returns
the original signal exactly. The join has matching value and slope at peak one;
the chroma-only gate stays constant along an exposure ray, preserving exposure
ordering. Neutral/pale highlights, ordinary-intensity colors, and shadow/gray
anchors are unchanged. Saturated lights/reflections can be dimmer; this reserves
tonal separation rather than adding sharpening, local contrast, or invented
detail. This automatic SDR operation is independent of the retention slider.
HDR, Conversion Only, managed output, display-ready Auto and texture/bypass/matte
policies are unchanged. Source keys and grain coordinates are not compressed.

For positive scene luminance `y` and peak RGB `p > 1`, let `q = y/p`,
`k = tone(q)`, `h = q-k`, and `d = toneDerivative(q)*(y-q)`.
The alternate luminance is `k + h*d/(h+d)`. At peak one it matches the
original value and slope; along a fixed chromatic ray, `q` stays constant and
the shoulder is monotonic. Its luminance ceiling is `q`, keeping the
alternate's pre-gamut peak RGB below one, rather than requiring a red emitter
to occupy the same near-white
brightness as an achromatic light. Negative wide-gamut channels are handled by
the same bounded radial mapping at both endpoints.
Here `y` and `p` refer to the post-input-shoulder signal in v0.39.

The original and alternate display-linear RGB outputs are blended with a
chroma-only smooth gate: `smoothstep(0.15, 0.75, (max-min)/max)`. At default,
the maximum blend is 0.5. Peak RGB at or below one, neutral grays and pale colors
with relative range at or below 0.15 keep the original response. The gate is
exposure invariant: a simple intensity-ramped stronger blend can reverse
brightness on an exposure ramp and is deliberately not used. This response
applies wherever SDR rendering is active, even without Film Color; the extra
retention adjustment below still requires that module.

Source highlight extraction and selective-color keys still use the original converted scene signal (camera-balanced for selection when Film Color is enabled). Rendering does not change glow extraction thresholds, source hue selection or procedural grain coordinates. Grain tonal weighting follows the image at its selected insertion point, so its amplitude can legitimately change with the improved tone response.

### SDR Viewing Controls

The collapsed **SDR Viewing** group sits at the bottom below Selective Color
and exposes three centered adjustments, each
from -1 to +1. Zero preserves v0.39, including its colored-emitter detail response.
They are output context, not stock-recipe values: built-in looks preserve them,
and user presets capture them in format 6. Preserve Color Spaces also preserves
these settings when loading a file. Older files receive zero adjustments.

- **Viewing Contrast:** negative softens shadow/midtone contrast; positive deepens
  shadows and increases midtone separation. Scene gray 0.18 remains display-linear
  0.12. This is separate from Negative/Print Contrast and does not adjust exposure.
- **Highlight Rolloff:** positive starts the shoulder earlier, giving darker/softer
  highlights; negative delays it, giving brighter highlights. Gray and its slope
  remain fixed. Negative/Print tone curves can add further compression.
- **Gamut Compression:** positive begins SDR display-boundary softening earlier;
  negative reduces it. Separate from Film Color Gamut Compression. Minus one
  removes in-boundary softening, not boundary safety: out-of-gamut RGB is still
  radially limited to the display boundary.

For contrast adjustment `c` and rolloff `r`, the midtone slope is
`m = (13/15)*(1+0.25*c)` and shoulder join is `j = 0.6-0.25*r`.
Below gray the curve is `0.12*x / (0.18*a-(a-1)*x)`, with
`a = 1.3*(1+0.25*c)`. Between gray and `j` it is `0.12+m*(x-0.18)`.
Above `j`, let `h = 1-(0.12+m*(j-0.18))`; the curve is
`1-h*h/(h+m*(x-j))`. These joins retain matching value/slope, bounded monotonic
output and no black pedestal across the allowed settings. The peak-aware emitter
shoulder uses the adjusted tone and its derivative too, avoiding a separate,
inconsistent highlight curve. The automatic emitter input shoulder is unchanged.

For gamut adjustment `g`, the normalized boundary knee is `0.8-0.2*g`.
The curve and gamut helpers explicitly retain the original arithmetic at zero.
The controls use the existing shared CPU/OpenCL composite pass, with no new
image buffers, passes, readbacks or per-pixel iterative gamut searches.

They are greyed out and ignored in Conversion Only, HDR, log/linear destinations,
display-ready Auto, texture-only/bypass and diagnostic matte paths. They remain
available in SDR with Film Color, Print, non-neutral Development or Selective Color
active; Film Color itself need not be enabled. No source reconstruction or
calibrated ARRI/ACES/SpektraFilm transform match is claimed.

### Highlight Color Retention

Film Color's **Highlight Color Retention** defaults to zero, using the current
updated automatic colored-highlight response. Its 0-1 range increases the
fully gated display-linear RGB blend from 0.5 to 0.8. At each fixed source pixel
the slider is an affine RGB blend between fixed, bounded endpoints, after
camera balance. Zero no longer reproduces pre-v0.38 SDR highlights; saved SDR
projects/presets intentionally receive the improved foundation. The preset
file format and recipes are unchanged. This is not a global saturation
increase or guaranteed recovery of clipped source channels.

Neutral/pale colors and colors whose largest linear channel is at most one retain
their original response. Bright colored windows and reflections can change too,
not just neon/LEDs. Start around 0.5 when a luminous colored source turns too
white, and compare its brightness as well as its hue.

It requires enabled Film Color and active SDR output rendering. Conversion
Only, Auto with display-ready input, log/linear output, disabled Film Color,
texture-only, bypass, full-strength Mono Negative and Selection Matte ignore
it. The UI greys it out in those cases without discarding its stored value.
Film Color/Tone Strength do not scale it: it belongs to the SDR foundation, so
it works on Clean Slate while Film Color is enabled even at zero creative
strengths. Built-in looks reset it to zero; portable user files capture it.

Runs in the shared CPU/OpenCL composite pass without extra buffers or image
passes. Source glow/selection keys and grain geometry remain unchanged;
image-dependent grain amplitude may follow changed highlight tones. This is an
original RGB rendering control, not spectral processing or stock calibration.

The C++/OpenCL operation is in the existing composite pass with no new image buffers, passes, transfers or dependencies. It adds transfer-function and rational arithmetic when active. The original conversions below remain separately testable and retain their signed/HDR round trips in Conversion Only.

Managed processing decodes the source, converts linear primaries to Rec.709, and applies the original artistic effects in a common perceptual sRGB domain. Its linear toe avoids amplifying floating-point cancellation at black. Halation extraction and grain weighting use that same domain across managed inputs. Exposure and temperature/tint balance operate in linear light before Film Color; Exposure +1 doubles linear RGB. Results are converted to the destination space.

Matrices are derived from published chromaticities in double precision once. Bradford adaptation aligns ACES D60 and Blackmagic's published D65 variant with Rec.709 D65. Small float matrices are passed to OpenCL kernels; there are no frame downloads or extra host transfers. C++ and OpenCL share transfer/balance math, including equivalent base-2 expressions for base-10 logs/powers.

Negative/HDR values survive texture processing. Gamma 2.4 and sRGB use symmetric negative extensions; camera curves use linear/signed extensions. Rec.709/Gamma 2.4 is a zero-black display power function, not the scene Rec.709 OETF. ACEScct decoding follows its specified 65504 upper limit. Other ordinary photographic values are not clamped to 0..1 by conversions. Film Color/Print still intentionally reshape tone and clamp negative effect-domain values.

These are encoding/gamut conversions plus original viewing responses and artistic looks, not manufacturer viewing LUTs, an ACES rendering transform or measured film spectral response. HLG, PQ input, non-800 LogC3 EI curves, alternate Canon gamuts, and sensor-specific IDTs are not included. LogC3 uses the SUP 3.x exposure-value EI-800 curve, not the sensor-value curve. ARRI's VFX document explicitly distinguishes direct colorimetric conversion from its tone-mapped viewing LUTs; neither a correct LogC inverse nor matching input/output labels implies a matching display rendering.

Resolve must supply float RGB in the selected encoding. The plugin does not apply video/full-range remapping; Resolve handles media data levels before OFX. Incorrect clip levels cannot be corrected by changing this dropdown.

### HDR PQ Output

For manually managed direct HDR output, choose the actual input space, output
**Rec.2100 / PQ (Rec.2020)** and **Standard HDR (PQ)**. Auto also renders HDR
for log/linear input to PQ; display-ready Rec.709/sRGB input requires explicit
Standard HDR to reinterpret its decoded values with our HDR tone scale.
This is not inverse tone mapping or recovery of already clipped SDR highlights.

**HDR Peak Luminance** ranges from 400 to 10000 nits (default 1000).
**HDR Reference White** ranges from 80 to 300 nits (default 203). Their ranges
ensure white is below peak. Both are top-level output context, preserved when
switching built-in looks and by Preserve Color Spaces during user-preset loading.
Peak is enabled only during HDR rendering; reference white is enabled for all
active PQ output, including Conversion Only. Texture-only, bypass and diagnostic
mattes ignore both and keep the existing pass-through/matte behavior.

The existing creative/texture pipeline works in its original perceptual Rec.709
domain. Only during HDR rendering, negative and print shoulder ceilings receive
extra perceptual headroom derived from peak/reference white; print gamut mapping
uses the adapted print ceiling too. Toe, pivot, knee, contrast and palette settings
remain the same. This lets print-heavy looks carry above-white highlights rather
than stretching an SDR-limited result. SDR/Conversion Only/managed/texture
parameters are unchanged. HDR does not squeeze the signal through the SDR viewing curve. After
film, development, print, grain, glow and selective finishing, we decode to linear,
convert to Rec.2020 and apply an original luminance tone scale. This does not
expand the creative engine itself to wide-gamut spectral processing.

The scale anchors linear gray 0.18 at 0.12 times reference white, and linear
white 1 at reference white. A cubic segment smoothly joins the shadow toe to
white; a rational shoulder approaches peak with matching first derivatives.
At defaults, neutral gray is 24.36 nits and white is 203 nits. Above-white scene
values can occupy HDR headroom. Peak-normalized radial gamut mapping preserves
Rec.2020 linear luminance/chroma direction, then ST 2084 inverse EOTF encodes
absolute luminance (code 1 means 10000 nits, not the selected peak).
This is our artistic HDR rendering, not the BT.2100 reference OOTF or an ACES ODT.

PQ transfer constants/Rec.2020 primaries and the 203-nit reference-white convention
come from [ITU-R BT.2100-3](https://www.itu.int/dms_pubrec/itu-r/rec/bt/R-REC-BT.2100-3-202502-I!!PDF-E.pdf).
The algebraically equivalent float PQ evaluation avoids independently rounded
numerator/denominator terms reversing tiny highlight steps. The HDR-only primary conversion uses a neutral-anchored matrix evaluation to
avoid amplifying float rounding into near-neutral PQ chroma. Existing SDR and
managed encodings retain their previous math. Highlight Color Retention remains
an SDR-only control and greys out during PQ output.

Conversion Only to PQ skips tone and radial gamut mapping, scales decoded linear
white by HDR Reference White, and applies PQ encoding. Negative channels and
channels above 10000 nits clip to the PQ transfer's physical endpoints. The peak
control is ignored. This is not the usual RCM/ACES configuration: in a managed
timeline, use the space entering the plugin, Same as Input and Conversion Only,
letting Resolve's chosen output transform handle SDR/HDR delivery.

Strong creative negative/print shoulders can intentionally compress values near
or below reference white. A 1000-nit target does not guarantee a 1000-nit image.
Start with Clean Slate to evaluate the HDR foundation; reduce Film Tone/Print
Tone Strength if their look suppresses specular headroom. No source highlight
restoration is performed. Glow/grain still run before output rendering; source
keys and grain geometry are unchanged, although HDR maps their visible amplitude.

Configure Resolve monitoring, project/output tagging, export data levels and
mastering metadata separately. The OFX cannot set HDR10/MaxCLL/MaxFALL metadata
or configure an HDR display. Avoid a second viewing transform after rendered PQ.
There is no calibrated HDR viewer certification; SDR browser previews must not
be used to judge PQ appearance. CPU/OpenCL numerical tests and original-footage
luminance checks are not substitutes for a reference HDR display.

## References

Original implementation based on numerical specifications; no Filmbox binaries or private implementation were used.

- [ARRI Alexa LogC3 in VFX](https://www.arri.com/resource/blob/31918/66f56e6abb6e5b6553929edf9aa7483e/2017-03-alexa-logc-curve-in-vfx-data.pdf): SUP 3.x coefficients and AWG3 primaries.
- [ARRI LogC4 Specification](https://www.arri.com/resource/blob/278790/bea879ac0d041a925bed27a096ab3ec2/2022-05-arri-logc4-specification-data.pdf): software curve and AWG4.
- [Sony S-Gamut3/S-Log3 Technical Summary](https://download.pro.sony/FNGP/protein/1237494271390/1237494271406.pdf): curve and both Sony gamuts.
- [Blackmagic DaVinci Wide Gamut/Intermediate](https://documents.blackmagicdesign.com/InformationNotes/DaVinci_Resolve_17_Wide_Gamut_Intermediate.pdf): working-space curve and primaries.
- [REDWideGamutRGB/Log3G10, Rev C](https://docs.red.com/955-0187/PDF/915-0187%20Rev-C%20%20%20RED%20OPS,%20White%20Paper%20on%20REDWideGamutRGB%20and%20Log3G10.pdf): shifted curve, negative extension, and gamut.
- [Panasonic V-Log/V-Gamut](https://pro-av.panasonic.net/en/cinema_camera_varicam_eva/support/pdf/VARICAM_V-Log_V-Gamut.pdf): transfer, primaries, and reference matrix.
- [Canon Log Gamma Curves](https://www.usa.canon.com/content/dam/canon-assets/white-papers/pro/white-paper-canon-log-gamma-curves.pdf), [Colour's Canon reference implementation](https://github.com/colour-science/colour/blob/develop/colour/models/rgb/transfer_functions/canon.py), and [Cinema Gamut dataset](https://github.com/colour-science/colour/blob/develop/colour/models/rgb/datasets/canon_cinema_gamut.py): v1.2 normalized-code coefficients and reflection scaling.
- [Colour's Blackmagic Gen 5 reference implementation](https://github.com/colour-science/colour/blob/develop/colour/models/rgb/transfer_functions/blackmagic_design.py) and [gamut dataset](https://github.com/colour-science/colour/blob/develop/colour/models/rgb/datasets/blackmagic_design.py): reference for Blackmagic's Gen 5 specification. The linked manufacturer document could not be retrieved; these numerical references supply its coefficients and distinct white point. No library code was copied.
- [ACEScct Specification](https://docs.acescentral.com/encodings/acescct/): AP1, D60, piecewise curve, and decoding ceiling.
- [ICC sRGB Specification](https://registry.color.org/rgb-registry/files/sRGB.pdf) and [ITU-R BT.1886](https://www.itu.int/rec/R-REC-BT.1886/en): display conventions.

## Verification

`ctest --test-dir build/ofx --output-on-failure` covers manufacturer gray anchors, monotonic curves, negative/HDR round trips, reference matrices, adapted neutrals, CPU/OpenCL textures, every managed input/output with individual/combined film stages, linear exposure, alpha, exact bypass/zero texture, and texture-only output preservation. Existing grain/halation/module regressions remain enabled.

SDR checks additionally cover independent gray/white/black anchors, continuous slopes, shadow and HDR gradation, skin ordering, neutral axes, linear-luminance/chroma preservation in gamut mapping, overbright red emitters, all rendering/input/output policies, all-recipe module masks, camera-exposure ordering, Auto/Conversion Only equality for display input, CPU/OpenCL parity and exact disabled/bypass/texture/matte isolation. Synthetic checks cannot establish a match to a footage screenshot or another plugin's undocumented preset.

The harness reports 4K GPU-resident timings for Rec.709/Gamma 2.4, Alexa LogC3, DaVinci Intermediate, and ACEScct. They exclude Resolve, transfers, warmup, and other timeline effects, and are not guaranteed playback rates. UI layout and footage appearance still require testing in Resolve.
