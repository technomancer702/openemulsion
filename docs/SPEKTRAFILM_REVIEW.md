# SpektraFilm Source Review

Reviewed the local checkout after `fetch` and `pull --ff-only` on 2026-10-06. Upstream `main` was current at `86476af` (`simplify readme`). This review covers that source revision rather than other branches or installed binaries. The checkout is under ignored `analysis/spektrafilm-ofx`.

## What It Does Differently

- [Profile interface](https://github.com/chaert-s/spektrafilm-ofx/blob/86476af/src/SpektraProfileCurves.h): film/paper entries contain wavelength-dependent sensitivity, exposure/density curves, base/layer densities, illumination/scanning transforms, halation defaults and stock-calibration fields. This is profile-backed spectral simulation, not a collection of artistic slider recipes. Its presence alone does not establish accuracy against every actual stock/scan.
- [OFX controls and file workflow](https://github.com/chaert-s/spektrafilm-ofx/blob/86476af/src/SpektraFilmPlugin.cpp): independent film Stock and print Paper selectors, process/output roles, optional stock-calibrated development, HDR controls, film formats, diffusion, user preset snapshots and LUT export. In this revision, user presets are file-backed and the stock menu is separate; there is no top-level creative preset-category control named in this source.
- [Build-time table generator](https://github.com/chaert-s/spektrafilm-ofx/blob/86476af/tools/generate_profile_curves.py): transforms profile data into compiled tables. It avoids requiring Python or loose JSON profiles for normal rendering, though its scientific build pipeline adds dependencies.
- [Parity/performance approach](https://github.com/chaert-s/spektrafilm-ofx/tree/86476af/PnP): structured images, stage outputs and timing comparisons against the reference Python implementation. This is a useful validation model. We should not infer relative speed from unmatched published/harness timings.
- [Platform architecture](https://github.com/chaert-s/spektrafilm-ofx/blob/86476af/README.md): Metal/Vulkan rather than our OpenCL/CPU implementation, with macOS/Windows and a Linux developer build. This is broader platform coverage, not automatically a better fit for our current tested Windows workflow.

Their stock inventory includes twenty camera-film targets (modern cinema negatives, still negatives including pushed Portra, and color reversal), plus print-film/paper profiles. Our v0.30 covers those camera-stock names as inspired recipes, adds two print-film-inspired looks, Neutral, B&W targets and creative grades. It does not offer their photographic-paper simulation or spectral profile fidelity.

## What To Improve Next

1. **Organized discovery and a clean start:** implemented in v0.30, with a non-destructive category browser, Neutral and a broader original recipe library. Native host workflow and real footage remain the next validation step.
2. **Distinct stock character:** the main remaining limitation. Our six negative-family matrices are shared by many looks; new names mostly vary tone, saturation, density, viewing balance and texture. Per-stock palette/curve refinements could improve separation without a spectral pipeline. Reference scans and charts would support quantitative calibration.
3. **Negative/print pairing:** our new print-inspired looks are whole recipes, not independently selectable stock profiles. A separate future print-profile choice could make combinations easier, but should preserve editable Custom print behavior and avoid stacking two viewing transforms.
4. **Better appearance validation:** use a repeatable real-footage set with skins, foliage, mixed lighting, saturated emitters, gray ramps and motion; compare proxy/full-resolution grain, stage isolation and highlight rolloff. Existing synthetic CPU/OpenCL tests are valuable but cannot establish stock fidelity or pleasing motion.
5. **Keep performance boundaries explicit:** preserve GPU residency, skip inactive stages, reuse scratch buffers and benchmark complete looks. Do not add spectral reconstruction, HDR roles, multiple scattering models, or platform backends merely to match feature count. LUT export is useful eventually but cannot represent spatial or time-varying grain/glow.

## SDR Follow-Up (2026-10-07)

Fetched upstream again and verified that local HEAD and `origin/main` both remain at `86476afc5b077de77e2278e3658d1ba9309892a1`. Compared its Vulkan and Metal output paths with OpenEmulsion v0.39. This is source inspection, not a matched render of the user's installed SpektraFilm binary or its Clean Slate preset.

### Conversion Versus Rendering

Both projects use analytic Gamma 2.4 encoding for that output choice. Decoding LogC3, converting primaries and encoding Gamma 2.4 do not themselves define the desired scene-to-display appearance. The main architectural difference is where that appearance is formed:

- **OpenEmulsion:** original SDR viewing response before artistic negative/development/print response, followed by output encoding. Neutral removes creative response but retains the SDR viewing foundation when SDR rendering is active.
- **SpektraFilm:** profile exposure/density response, optional print development and spectral scan to destination-linear RGB, then optional perceptual output compression and output encoding. Its final SDR function does not apply an unconditional extra scene-luminance viewing curve to the already scanned image.

Sources: [film development](https://github.com/chaert-s/spektrafilm-ofx/blob/86476afc5b077de77e2278e3658d1ba9309892a1/shaders/vulkan/SpektraCurveDevelop.comp), [print and spectral scan](https://github.com/chaert-s/spektrafilm-ofx/blob/86476afc5b077de77e2278e3658d1ba9309892a1/shaders/vulkan/SpektraPrintScan.comp#L1299), [SDR finalization](https://github.com/chaert-s/spektrafilm-ofx/blob/86476afc5b077de77e2278e3658d1ba9309892a1/shaders/vulkan/SpektraScannerPost.comp#L499).

### Our Fixed SDR Choices

These are authored viewing parameters, not measured stock characteristics or a standardized ARRI/ACES rendering:

| Choice | v0.39 behavior |
| --- | --- |
| Middle gray | Scene-linear 0.18 maps to display-linear 0.12 |
| Main shoulder | Starts at scene luminance 0.60, mapped to 0.484; approaches display-linear 1 |
| Neutral scene white | Scene-linear 1 maps to approximately 0.6914 display-linear |
| Display gamut | Linear-RGB radial compression about mapped luminance, with normalized boundary knee 0.8 |
| Colored emitter detail | Above peak RGB 1, a chroma-gated root shoulder reduces intensity before the luminance/gamut mapping; maximum strength 2 |
| Highlight color retention | Chroma-gated peak-aware alternative blended at 0.5 by default, rising to 0.8 with the retention control |

See `ofx/src/ColorMath.h`, particularly `color_sdr_tone`, `color_sdr_gamut`, `color_sdr_detail_peak` and `color_render_work`. The negative and print stages have additional editable tone curves. Their combined response can therefore compress highlights more than the SDR foundation alone; that interaction should be measured rather than assumed harmless.

### Their Optional SDR Compression

The OFX Color Adaptation master defaults **off**, despite its four subordinate settings defaulting on. `colorAdaptationFlags` returns zero while the master is off. Scanner correction also defaults disabled; its numeric white/black levels do not imply a default pedestal or endpoint adjustment.

When output compression is enabled, SpektraFilm converts destination-linear RGB to OKLab, adjusts lightness/chroma separately and searches the destination gamut boundary along a perceptual hue direction. Fixed SDR settings include a lightness knee at OKLab L 0.7 with exponent 2.2, and normalized-chroma compression with exponent 6. These are not directly comparable to our scene-luminance knee or RGB boundary distance. The implementation has early-return cases, including in-bounds achromatic input, so it should not be described as an unconditional lightness curve on every pixel.

The default finalizer does not universally bound SDR RGB to 0..1 before encoding; optional gamut compression does. Profile density lookup also reaches endpoint densities beyond its sampled exposure domain. Its architecture does not guarantee unlimited highlight detail or establish that every displayed result is calibrated.

Sources: [OFX defaults](https://github.com/chaert-s/spektrafilm-ofx/blob/86476afc5b077de77e2278e3658d1ba9309892a1/src/SpektraFilmPlugin.cpp#L4982), [master flag policy](https://github.com/chaert-s/spektrafilm-ofx/blob/86476afc5b077de77e2278e3658d1ba9309892a1/src/SpektraParameters.h#L306), [perceptual compression](https://github.com/chaert-s/spektrafilm-ofx/blob/86476afc5b077de77e2278e3658d1ba9309892a1/shaders/vulkan/SpektraScannerPost.comp#L455), [analytic transfer tests](https://github.com/chaert-s/spektrafilm-ofx/blob/86476afc5b077de77e2278e3658d1ba9309892a1/tests/test_sdr_transfer_encode.py).

### Recommendation

Keep the user-validated v0.39 default during further evaluation. Use stage-isolated gray/exposure ramps, colored-emitter ramps and the five local clips to assess the combined SDR/negative/print response. Consider a small advanced SDR section for rolloff and gamut strength. A perceptual gamut alternative would need implementation and benchmarking against the current pipeline.

A matched appearance comparison needs SpektraFilm's actual process, film, paper, exposure/gamma, Color Adaptation and scanner settings, not just matching LogC3 and Rec.709 menus. Moving our SDR transform after the film stages is not a drop-in fix: those stages currently expect the existing perceptual working domain and would require redesign and regression checks. No rendering change was made during this audit.

## HDR Follow-Up (2026-10-07)

Fetched upstream again; local HEAD and `origin/main` still match
`86476afc5b077de77e2278e3658d1ba9309892a1`. Compared its Vulkan and Metal HDR
finalizers with OpenEmulsion v0.41. This is source inspection, not a matched
HDR-monitor comparison of the installed plugins. No rendering change was made.

### Output Architecture And Controls

Both projects use Rec.2020/D65 and the ST 2084 absolute-luminance transfer for PQ.
The important difference is the tone response before encoding, not a different
PQ definition:

- **SpektraFilm:** maps post-film/print/scan relative linear RGB to nits using
  reference white and a post-scan HDR Exposure EV trim. Hard Clip limits
  luminance to the chosen peak; Soft Rolloff leaves luminance below reference
  white unchanged and uses an exponential shoulder above it. Its named PQ
  1000, PQ 4000 and HLG 1000 presets all select **Hard Clip** at 203-nit white.
  Selecting a named HDR preset does not reset the separate exposure trim.
- **OpenEmulsion:** extends the artistic negative/print curve ceilings for HDR,
  then applies our original luminance-based viewing curve after creative and
  texture finishing. It has a shadow toe, a darker middle-gray anchor and a
  smooth rational highlight shoulder. Peak and reference white are adjustable;
  HDR-specific exposure and rolloff controls are not currently exposed.

SpektraFilm's reference white scales relative **scan white**, not an untouched
camera scene value. Our creative pipeline also changes the signal before HDR
finalization. Equal menu selections therefore cannot establish equal exposure
or appearance. Our artistic ceiling extension is an authored heuristic, not a
measured film-profile calibration. Our SDR Viewing controls do not affect HDR.

Sources: [HDR finalization](https://github.com/chaert-s/spektrafilm-ofx/blob/86476afc5b077de77e2278e3658d1ba9309892a1/shaders/vulkan/SpektraScannerPost.comp#L315),
[Metal implementation](https://github.com/chaert-s/spektrafilm-ofx/blob/86476afc5b077de77e2278e3658d1ba9309892a1/shaders/SpektraFilm.metal#L637),
[preset values](https://github.com/chaert-s/spektrafilm-ofx/blob/86476afc5b077de77e2278e3658d1ba9309892a1/src/SpektraFilmPlugin.cpp#L1464),
[HDR controls](https://github.com/chaert-s/spektrafilm-ofx/blob/86476afc5b077de77e2278e3658d1ba9309892a1/src/SpektraFilmPlugin.cpp#L4973).
Our corresponding functions are in `ofx/src/ColorMath.h` and
`ofx/src/FilmResponseConfig.h`.

### Isolated Neutral Response

These source-derived values compare **equal neutral inputs to each HDR
finalizer**, at 203-nit white, 1000-nit peak and zero exposure trim. They are not
end-to-end LogC3 footage measurements or a stock-fidelity comparison.

| Relative finalizer input | OpenEmulsion nits | SpektraFilm Hard Clip nits | SpektraFilm Soft Rolloff nits |
| --- | --- | --- | --- |
| 0.01 | 1.05 | 2.03 | 2.03 |
| 0.18 | 24.36 | 36.54 | 36.54 |
| 1 | 203.00 | 203.00 | 203.00 |
| 2 | 374.09 | 406.00 | 382.21 |
| 4 | 562.09 | 812.00 | 628.80 |
| 8 | 726.44 | 1000.00 | 865.99 |

Our finalizer has stronger shadow shaping and earlier highlight compression.
Neither choice is mandated by PQ. Their default has more linear highlight
headroom but a hard luminance endpoint; ours trades some separation for a smooth
approach to peak. Creative negative/print curves can add further compression.

### Saturated Highlights And Peak Bounds

SpektraFilm's Color Adaptation master defaults off. In that state its HDR
finalizer floors negative RGB, tone-maps **luminance**, and encodes without a
per-channel ceiling at the selected peak. A post-scan Rec.2020 red input
`(10, 0, 0)` becomes a 2030-nit red channel but only 533.28-nit luminance, so
Hard Clip does not limit it at a 1000-nit setting. This is not the same as an
incorrect PQ transfer: the selected luminance peak is simply not a channel bound.

Enabling its optional output compression adds an OKLab/OKLch gamut-boundary
search and bounds channels to the selected peak. In HDR, already in-bounds
colors return unchanged rather than receiving the optional SDR in-gamut
softening. OpenEmulsion's **rendered HDR** always uses linear-RGB radial gamut
compression and bounds channels to peak. That is a useful delivery constraint,
but can reduce saturated-highlight color and detail. Conversion Only does not
apply that rendered-HDR peak bound.

Source: [HDR gamut and encoding policy](https://github.com/chaert-s/spektrafilm-ofx/blob/86476afc5b077de77e2278e3658d1ba9309892a1/shaders/vulkan/SpektraScannerPost.comp#L508).
Our peak-aware colored-emitter detail handling is currently SDR-only; HDR needs
its own scene-to-output detail measurements before another algorithm change.

### HLG Caution

SpektraFilm offers HLG, which we currently do not. However, both inspected GPU
implementations use system gamma 1.0 at 1000 nits and apply the inverse gamma
separately to RGB channels. BT.2100's reference HLG path uses gamma 1.2 at
1000 nits and a luminance-based OOTF. Surround adjustments can change gamma,
but no matching surround-control intent was found in this path. This merits
a standards audit and HDR-monitor testing rather than comparison through an
SDR screenshot.

Sources: [HLG helper](https://github.com/chaert-s/spektrafilm-ofx/blob/86476afc5b077de77e2278e3658d1ba9309892a1/shaders/vulkan/SpektraScannerPost.comp#L351),
[ITU-R BT.2100-3, Table 5 and notes](https://www.itu.int/dms_pubrec/itu-r/rec/bt/R-REC-BT.2100-3-202502-I!!PDF-E.pdf).

### Recommendation

Keep the current PQ defaults while adding HDR viewing
adjustments in a future version: post-look exposure trim and highlight rolloff,
with neutral positions retaining the current output. Evaluate a more linear
viewing option only against isolated ramps and matched footage, not to imitate
one screenshot. Retain predictable peak bounds, but specifically test colored
emitters for local contrast and hue across peak settings. Add HLG only when
needed and validate its reference transfer independently.

### Local Numeric Check

Ran `tools/check_hdr_footage.py` against the preserved full-precision inputs in
`analysis/color-bench-v035`, using the native v0.41 ColorBench bridge. The 90
renders cover five clips at three timestamps, Neutral and 50D Daylight, and
400/1000/4000-nit peaks at 203-nit white. Decoded-PQ measurements stayed finite,
channels remained within the selected peak tolerance and alpha was unchanged.
The local report is `analysis/hdr-review-v041/results.json` (ignored).

At 1000-nit peak, the maximum luminance across these frames was approximately
952 nits for Neutral and 407 nits for 50D Daylight; maximum channels were about
999 and 753 nits respectively. This demonstrates that creative response uses
less output headroom, not that either look must reach peak or that local lens
detail is preserved. These are Color Only numeric checks with burn-ins included,
not GPU-performance checks, calibrated HDR previews or SpektraFilm A/B renders.

## Follow-Up Implementation

The subsequent v0.42 HDR Viewing implementation adds post-look exposure trim
and rational-shoulder reshaping to our existing HDR path.
Zero retains v0.41. Native
zero-default, inactive-path and source-keyed highlight diagnostics are recorded
in [Color Bench](COLOR_BENCH.md#hdr-viewing-audit-v042).

The subsequent v0.40 implementation exposes centered SDR Viewing Contrast,
Highlight Rolloff and Gamut Compression using shared
CPU/OpenCL math. Zero retains v0.39.
The stage-isolated ramp and local-footage audit is documented in
[Color Bench](COLOR_BENCH.md#sdr-viewing-audit-v040).

OpenEmulsion's strengths remain a smaller adjustable pipeline, independent module use, tested OpenCL/CPU paths and predictable zero/bypass behavior. Relative accuracy and performance need matched comparisons. The projects pursue different balances between scientific simulation, flexibility and implementation cost.
