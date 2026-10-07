# SpektraFilm Source Review

Reviewed the existing ignored local checkout after `fetch` and `pull --ff-only` on 2026-10-06. Upstream `main` was already current at `86476af` (`simplify readme`). This is a review of that source revision, not a claim about an installed commercial/website binary or other branches. The checkout remains under ignored `analysis/spektrafilm-ofx`; none of its implementation or profile data is included in OpenEmulsion.

## What It Does Differently

- [Profile interface](https://github.com/chaert-s/spektrafilm-ofx/blob/86476af/src/SpektraProfileCurves.h): film/paper entries contain wavelength-dependent sensitivity, exposure/density curves, base/layer densities, illumination/scanning transforms, halation defaults and stock-calibration fields. This is profile-backed spectral simulation, not a collection of artistic slider recipes. Its presence alone does not establish accuracy against every actual stock/scan.
- [OFX controls and file workflow](https://github.com/chaert-s/spektrafilm-ofx/blob/86476af/src/SpektraFilmPlugin.cpp): independent film Stock and print Paper selectors, process/output roles, optional stock-calibrated development, HDR controls, film formats, diffusion, user preset snapshots and LUT export. In this revision, user presets are file-backed and the stock menu is separate; there is no top-level creative preset-category control named in this source. OpenEmulsion's category browser is our own UX design, not a port of theirs.
- [Build-time table generator](https://github.com/chaert-s/spektrafilm-ofx/blob/86476af/tools/generate_profile_curves.py): transforms profile data into compiled tables. It avoids requiring Python or loose JSON profiles for normal rendering, though its scientific build pipeline adds dependencies.
- [Parity/performance approach](https://github.com/chaert-s/spektrafilm-ofx/tree/86476af/PnP): structured images, stage outputs and timing comparisons against the reference Python implementation. This is a useful validation model. We should not infer relative speed from unmatched published/harness timings.
- [Platform architecture](https://github.com/chaert-s/spektrafilm-ofx/blob/86476af/README.md): Metal/Vulkan rather than our OpenCL/CPU implementation, with macOS/Windows and a Linux developer build. This is broader platform coverage, not automatically a better fit for our current tested Windows workflow.

Their stock inventory includes twenty camera-film targets (modern cinema negatives, still negatives including pushed Portra, and color reversal), plus print-film/paper profiles. Our v0.30 covers those camera-stock names as inspired recipes, adds two print-film-inspired looks, Neutral, B&W targets and creative grades. It does not offer their photographic-paper simulation or spectral profile fidelity.

## What To Improve Next

1. **Organized discovery and a clean start:** implemented in v0.30, with a non-destructive category browser, Neutral and a broader original recipe library. Native host workflow and real footage remain the next validation step.
2. **Independent stock character:** the main remaining limitation. Our six negative-family matrices are shared by many looks; new names mostly vary tone, saturation, density, viewing balance and texture. Independently authored per-stock palette/curve refinements could improve separation without a spectral pipeline. Establish rights-cleared reference scans/charts before claiming calibration.
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

Keep the user-validated v0.39 default during further evaluation. Use stage-isolated gray/exposure ramps, colored-emitter ramps and the five local clips to assess the combined SDR/negative/print response. Consider a small advanced SDR section for rolloff and gamut strength rather than more unrelated fixed patches. A perceptual gamut alternative merits independent implementation and benchmarking, not a direct port of upstream GPL shaders or a spectral rewrite.

A matched appearance comparison needs SpektraFilm's actual process, film, paper, exposure/gamma, Color Adaptation and scanner settings, not just matching LogC3 and Rec.709 menus. Moving our SDR transform after the film stages is not a drop-in fix: those stages currently expect the existing perceptual working domain and would require redesign and regression checks. No rendering change was made during this audit.

## Source Boundary

The subsequent v0.40 implementation exposes centered SDR Viewing Contrast,
Highlight Rolloff and Gamut Compression using independently authored shared
CPU/OpenCL math. Zero retains v0.39; no SpektraFilm shader/profile data was reused.
The stage-isolated ramp and local-footage audit is documented in
[Color Bench](COLOR_BENCH.md#sdr-viewing-audit-v040).

The upstream checkout declares [GPL-3.0](https://github.com/chaert-s/spektrafilm-ofx/blob/86476af/LICENSE.txt); OpenEmulsion uses MPL-2.0. This change only studies public architecture and independently implements presets using our existing controls. No upstream code, profile arrays, JSON datasets, numeric recipes, shader formulas or generated tables were imported. Any future reuse needs an explicit license/provenance review rather than treating public availability as permission to copy into the current project.

OpenEmulsion's strengths remain a smaller adjustable pipeline, independent module use, our tested OpenCL/CPU paths, predictable zero/bypass behavior and original creative response. These are workflow/engineering strengths, not evidence that our stock simulation is more accurate or that ours is faster on comparable hardware. The projects pursue different balances between scientific simulation, flexibility and implementation cost.
