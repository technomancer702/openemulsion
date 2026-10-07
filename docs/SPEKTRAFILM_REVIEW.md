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

## Source Boundary

The upstream checkout declares [GPL-3.0](https://github.com/chaert-s/spektrafilm-ofx/blob/86476af/LICENSE.txt); OpenEmulsion uses MPL-2.0. This change only studies public architecture and independently implements presets using our existing controls. No upstream code, profile arrays, JSON datasets, numeric recipes, shader formulas or generated tables were imported. Any future reuse needs an explicit license/provenance review rather than treating public availability as permission to copy into the current project.

OpenEmulsion's strengths remain a smaller adjustable pipeline, independent module use, our tested OpenCL/CPU paths, predictable zero/bypass behavior and original creative response. These are workflow/engineering strengths, not evidence that our stock simulation is more accurate or that ours is faster on comparable hardware. The projects pursue different balances between scientific simulation, flexibility and implementation cost.
