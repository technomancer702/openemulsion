# OFX Color Spaces (v0.12)

## Resolve Workflow

`Input Color Space` describes the RGB entering this node. It is not inferred from camera metadata or project settings.

- Unconverted Alexa LogC3 in a manually managed project: select `ARRI Alexa LogC3 / Wide Gamut 3 (EI 800)`. With Film Color/Print enabled, select output `Rec.709 / Gamma 2.4` for an SDR look. Do not apply another LogC3-to-709 conversion afterward.
- Alexa already converted to DaVinci Wide Gamut/Intermediate by a CST or Resolve Color Management: select `DaVinci Wide Gamut / Intermediate` and output `Same as Input`. Keep the project's normal output transform.
- ACEScct timeline: select `ACEScct / AP1` with output `Same as Input`. This effect is a look, not an ACES Input/Output Transform.
- Your own camera LUT after this node: choose its camera input and `Halation & Grain Only` or `Grain Only`. Those modes retain the camera encoding for the LUT.
- Your own LUT before this node: select the LUT's output space, not the original camera space.

Texture-only processing always returns the input gamut and encoding, regardless of the output dropdown. This also applies to Full mode with Film Color, Film Development, and Print disabled. Texture still changes pixels, but no film color or print curves are applied. Bypass, all-disabled processing, and zero-strength texture preserve incoming RGBA exactly. Halation Matte is a direct diagnostic image, not a camera-log image.

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

Outputs: Same as Input, Rec.709/Gamma 2.4, DaVinci Wide Gamut/Intermediate, ACEScct/AP1, sRGB, and Linear/Rec.709. Output conversion runs only while Film Color, non-neutral Film Development, or Print is active. This is not a standalone CST: disabling all effects means pass-through, not input-to-output conversion.

## Math and Limits

Managed processing decodes the source, converts linear primaries to Rec.709, and applies the original artistic effects in a common perceptual sRGB domain. Its linear toe avoids amplifying floating-point cancellation at black. Halation extraction and grain weighting use that same domain across managed inputs. Exposure and temperature/tint balance operate in linear light before Film Color; Exposure +1 doubles linear RGB. Results are converted to the destination space.

Matrices are derived from published chromaticities in double precision once. Bradford adaptation aligns ACES D60 and Blackmagic's published D65 variant with Rec.709 D65. Small float matrices are passed to OpenCL kernels; there are no frame downloads or extra host transfers. C++ and OpenCL share transfer/balance math, including equivalent base-2 expressions for base-10 logs/powers.

Negative/HDR values survive texture processing. Gamma 2.4 and sRGB use symmetric negative extensions; camera curves use linear/signed extensions. Rec.709/Gamma 2.4 is a zero-black display power function, not the scene Rec.709 OETF. ACEScct decoding follows its specified 65504 upper limit. Other ordinary photographic values are not clamped to 0..1 by conversions. Film Color/Print still intentionally reshape tone and clamp negative effect-domain values.

These are encoding/gamut conversions plus our original look, not manufacturer viewing LUTs, an ACES rendering transform, measured film spectral response, or a complete HDR rendering pipeline. PQ/HLG, non-800 LogC3 EI curves, alternate Canon gamuts, and sensor-specific IDTs are not included. LogC3 uses the SUP 3.x exposure-value EI-800 curve, not the sensor-value curve.

Resolve must supply float RGB in the selected encoding. The plugin does not apply video/full-range remapping; Resolve handles media data levels before OFX. Incorrect clip levels cannot be corrected by changing this dropdown.

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

The harness reports 4K GPU-resident timings for Rec.709/Gamma 2.4, Alexa LogC3, DaVinci Intermediate, and ACEScct. They exclude Resolve, transfers, warmup, and other timeline effects, and are not guaranteed playback rates. UI layout and footage appearance still require testing in Resolve.
