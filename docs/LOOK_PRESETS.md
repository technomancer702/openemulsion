# Look Presets

Added in v0.24, categorized in v0.30 and expanded with genre looks in v0.31-v0.32. Preset Category and Preset sit directly below Film Gauge and remain available in every mode, including Bypass. New instances default to All Presets / Custom without applying a look. Existing saved values are not reset.

## Categories And Neutral

The browser contains 52 named recipes plus Custom. Categories: All Presets, Starting Points, Cinema Negative, Still Negative, Reversal Film, Monochrome, Print Looks, Creative Looks, Thriller, Horror, Sci-Fi and Graphic Noir. Every filtered list starts with **Custom / Current Settings**, which does nothing to the image. Changing category only filters the menu: it never loads that category's first look. If the active look is outside the selected category, the menu shows Custom / Current Settings; switching back to its category or All Presets shows the stored name again. Silver Noir is under Monochrome.

**Neutral / Clean Slate**, in Starting Points and first after Custom in All Presets, resets creative tuning with film/print color and tone strengths at zero, development at zero, grain at zero, and all glow at zero. It loads Full mode, Custom gauge, Custom print style, and enabled module switches so you can build a look. Raise Film/Print strength sliders when adding their response; their other sliders alone cannot affect the image while those strengths remain zero.

Neutral retains Input/Output Color Space, camera Exposure/Temperature/Tint, and Grain Seed, including their animation, just like every built-in recipe. It is not an external effect bypass, camera-balance reset, manufacturer viewing LUT, gamut mapper, or HDR display rendering transform. With neutral camera balance, the only remaining operation is the plugin's existing input/output conversion. Negative/HDR values are not creatively clamped; same-space conversion can have floating-point round-trip error. Choose Bypass for exact RGBA pass-through.

## Library

| Preset | Creative target |
| --- | --- |
| 50D Daylight | Fine, restrained grain; cleaner color and gentle highlight shaping. |
| 250D Daylight | Balanced daytime starting point with moderate texture and near-neutral print balance. |
| 200T Tungsten | Fine texture, gentler contrast, and softer highlights; no automatic warm cast. |
| 500T Tungsten | More texture than 200T, stronger highlight shoulder and halation, with restrained shadow weighting. |
| B&W Reversal | Monochrome contrast with grain, without colored diffusion. |
| Classic Cinema | Richer color, denser print contrast, deeper shadows, and restrained halation. |
| Soft Portrait | Gentler contrast, saturation, and texture, with subtle bloom. |
| Neon Nights | Visible cool shadow/warm highlight split, cleaner print palette, stronger halation, and saturated-color compression. |
| Seventies Print | Muted vintage palette, warmer print balance, softer lifted shadows, and stronger texture. |
| Bleach Bypass | Bleach-retained family, lower saturation, harder contrast, and mostly monochrome grain. |
| Super 8 Home Movie | Brighter color-reversal-inspired palette, gently warm print balance, Super 8 gauge, and coarse texture. |
| Silver Noir | Monochrome, harder contrast, deeper blacks, and pronounced grain. |

## Expanded Library In v0.30

All twelve existing recipes are unchanged. The following original recipes add broader coverage:

| Category / Preset | Creative target |
| --- | --- |
| Starting Points / Neutral | Creative reset described above; retains conversion and camera balance. |
| Cinema Negative / VERITA 200D | Restrained daylight color, softer tone and fine-to-moderate texture. |
| Still Negative / Portra 160 | Gentle contrast and saturation, smooth highlights and fine grain. |
| Still Negative / Portra 400 | Related portrait palette with moderate grain and a little more contrast. |
| Still Negative / Portra 800 | Related faster-negative target with stronger texture and firmer tone. |
| Still Negative / Portra 800 Push +1 / +2 | Existing Push/Pull development at +1/+2, with grain-strength coupling and restrained saturation. Not calibrated push chemistry or exposure compensation. |
| Still Negative / Ektar 100 | Cleaner fine texture, stronger saturation and firmer contrast. |
| Still Negative / Gold 200 | Warm consumer-negative print interpretation and moderate grain. |
| Still Negative / Ultramax 400 | Punchier color/contrast and coarser texture than Gold. |
| Still Negative / PRO 400H | Gentle pastel-like rendering, fine-to-moderate grain and a restrained cool/green print balance. |
| Still Negative / Superia X-TRA 400 | More color/contrast and grain, with a restrained cool print interpretation. |
| Still Negative / C200 | Gentler related consumer-negative target with less texture than Superia. |
| Reversal Film / Kodachrome 64 | Firmer reversal contrast, denser color and a small warm viewing balance. |
| Reversal Film / Ektachrome 100 | Fine texture, moderately vivid color and a restrained cool viewing balance. |
| Reversal Film / Velvia 100 | Stronger saturation and contrast, enriched muted colors, fine grain. |
| Reversal Film / Provia 100F | More restrained saturation/contrast than Velvia and fine texture. |
| Monochrome / Tri-X 400 | Firmer B&W tone and pronounced conventional grain. |
| Monochrome / HP5 Plus 400 | Gentler B&W contrast, more open shadows and slightly softer texture. |
| Print Looks / 2383 Print | Film-print-style palette/contrast, with negative response and grain at zero. |
| Print Looks / 2393 Print | Denser, higher-contrast related print interpretation and stronger color. |
| Creative Looks / Desert Chrome | Warm desaturated highlights, cool shadows, harder tone and moderate grain. |
| Creative Looks / Arctic Dusk | Cool, muted color, restrained highlights and fine texture. |
| Creative Looks / Golden Hour | Warm soft print, warm-highlight split, gentle bloom and fine texture. |
| Creative Looks / Faded Instant | Softer faded contrast, warm muted color, lifted print blacks and textured diffusion. |

Color-reversal recipes use the Reversal family with Print Color/Tone Strength at zero, avoiding a second creative print response. Their small Print RGB trims are viewing balances and remain effective independently of those strengths. Print-inspired looks are complete replacement recipes, not independent choices layered over a selected negative; they preserve camera balance but reset creative negative response, grain and glow. Combine and save module adjustments yourself for a custom negative/print pairing.

The new stock-name coverage parallels the twenty camera-stock targets in the reviewed SpektraFilm checkout, but uses our own parameter recipes. It does not reproduce their spectral profiles, paper models, or datasets. Manufacturer names are reference identifiers, not endorsements. Historical/discontinued names describe the intended inspiration, not current product availability.

Public [Kodak film technical publications](https://www.kodakprofessional.com/en-gb/node/133), [VERITA information](https://www.kodak.com/en/motion/product/camera-films/verita-200d-5206-7206/), [Fujifilm negative/reversal data sheets](https://www.fujifilm.com/mx/es/consumer/support/films/negative-and-reversal), and [ILFORD HP5 information](https://www.ilfordphoto.com/hp5-plus-sheet-film?___store=ilford_brochure) provide qualitative starting points. Warm/cool viewing interpretations and all numeric recipes are artistic decisions, not measured stock fits. v0.31 removes the redundant `(Inspired)` UI suffix, not this limitation or the approximation warning in the tooltip.

## Genre Library In v0.31

Nine original recipes use genre names with film references in parentheses. These are editable interpretations, not official products, measured film matches, extracted LUTs, or reconstructions of an entire movie. The research, source facts, artistic choices and limits are in [Creative Look Research](CREATIVE_LOOK_RESEARCH.md).

| Category / Preset | Creative target |
| --- | --- |
| Thriller / Archive Thriller (Zodiac) | Muted earth/olive print, readable lower midtones, restrained contrast, fine light texture and no diffusion. |
| Thriller / Silver Thriller (Se7en) | Print-led silver-retention interpretation: dense blacks, stronger print contrast, drained chroma, dirty warmth/cool shadows and mostly monochrome texture. |
| Thriller / Sodium Noir (Nightcrawler) | Amber practical-light balance against cool shadows, shaped highlights, legible darks, light halation and very restrained texture. |
| Horror / Folk Dread (The Witch) | Near-gray muted earth colors, subtly cold balance, open shadow gradation, gentle highlights and minimal texture; no glow. |
| Horror / Daylight Dread (Midsommar) | Brighter midtones, softer pastel-like chroma and contrast, a soft highlight shoulder and light bloom; not a neon-green foliage treatment. |
| Horror / Giallo Crimson (Suspiria 1977) | Bold chroma, red/magenta print bias, blue shadows, firm contrast and restrained diffusion. Requires colored source lighting for the strongest result. |
| Horror / Crimson Dream (Mandy) | Denser red/magenta bias, violet-blue shadow separation, broader bloom, stronger halation and visible texture; interprets the nightmare sequences, not the natural opening. |
| Sci-Fi / Simulation Green (The Matrix) | Green shadows/lower midtones, firmer contrast, reduced chroma and moderate Super 35 texture, without a complementary magenta highlight split. Targets the simulated world of the 1999 film. |
| Sci-Fi / Amber Wasteland (2049 Vegas) | Strong amber/red print balance, suppressed blue, softer contrast, long highlight rolloff, broad restrained bloom and minimal grain. Vegas only, not every sequence of Blade Runner 2049. |

Existing recipe values and default controls are unchanged. New preset/category IDs are appended. Color-space settings, camera balance and seed remain preserved. Movie lighting, selective local corrections, set colors, atmosphere and framing cannot be generated by these recipes. Broad palette trims do not provide independent hue-selective foliage, red or blue calibration. Adapt exposure to the source; no preset can recover clipped or missing shadow detail.

## Additional Library In v0.32

| Category / Preset | Creative target |
| --- | --- |
| Thriller / Winter Crime (Fargo 1996) | Restrained winter color, near-neutral cool whites, graduated dark tones, moderate fine grain and no diffusion. |
| Sci-Fi / Steel Blue (Terminator 2) | Strong metallic blue/cyan shadow split against warm highlights, firmer print contrast, retained chroma and Super 35 texture. |
| Sci-Fi / Nostromo (Alien 1979) | Muted industrial palette, cyan/green shadows, restrained warm highlights, firm graduated blacks and conventional grain. |
| Graphic Noir: Graphic Noir / Red (Sin City) | High-contrast monochrome surroundings, retaining source reds with a feathered hue key. |
| Graphic Noir: Graphic Noir / Blue (Sin City) | Same tonal direction, retaining source blues. |
| Graphic Noir: Graphic Noir / Yellow (Sin City) | Same tonal direction, retaining source yellows; no blue-to-yellow recoloring. |

The three graphic-noir recipes enable [Selective Color](SELECTIVE_COLOR.md), not Mono Negative. All other recipes reset its Amount/View to zero. The feature retains one source hue interval, not a particular object; equally colored skin, clothes and backgrounds can also survive. Adjust the key or use separate tracked corrections when a specific object needs isolation. Existing recipe values remain unchanged, except the appended neutral controls/module switch; default rendering is unchanged.

Preset labels use a stable persistent ID; filtered menu positions are transient and not rendering inputs. Existing IDs 0-46 retain their meaning, including the hidden original `lookPreset` parameter. Persistent `presetCategory` stores browsing context; nonpersistent `presetBrowser` is rebuilt from category and stored ID on instantiation and selector restore notifications. User `.oepreset` files store ordinary render controls only and load as Custom. v0.32 adds format version 2 with neutral/disabled migration of complete version-1 files.

These are original artistic recipes using existing controls, not calibrated reproductions of manufacturer stocks or particular movie grades. Stock-inspired names describe creative targets only. Tungsten/daylight names do not apply camera white balance: input footage is interpreted using the selected Input Color Space and the user's camera balance. There is no hidden stock transform.

## v0.29 Tuning

Classic Cinema, Neon Nights, Seventies Print, and Super 8 Home Movie now have clearer tone/color identities, not just different grain or glow. 250D, 200T, and 500T have modest toe/shoulder/print refinements and retain a consistent near-neutral family appearance. 50D, Soft Portrait, Bleach Bypass, B&W Reversal, and Silver Noir are unchanged. Default Custom controls and renderer math are unchanged.

This is recipe tuning, not a stock-calibration upgrade. Public [50D technical data](https://www.kodak.com/content/products-brochures/motion-picture/KODAK-VISION3-50D-5203-7203-technical-information.pdf), [200T information](https://www.kodak.com/content/products-brochures/motion-picture/KODAK-VISION3-200T-5213-7213-brochure.pdf), and [Kodak processing guidance](https://www.kodak.com/en/motion/page/processing-techniques/) inform qualitative targets: family consistency, fine grain, highlight latitude, and silver-retained contrast/desaturation. Numeric recipe values are original artistic choices, not fits to these documents. Super 8 is a format, not a single stock; Home Movie now explicitly targets a colorful reversal-inspired presentation rather than the same faded-negative palette as Seventies Print.

Saved projects and user preset files store ordinary parameter values. They do not change on upgrade, even if their Preset label still names a built-in look. To try a revised recipe on an existing node, select Custom, then the named preset again. This replaces creative tuning/keys but retains camera balance, encoding, and grain seed as before. No choice order, parameter identifiers, file format, or ownership policy changed.

On nine synthetic linear-light color/skin patches at five exposure scales (45 samples), mean absolute display-RGB difference (8-bit-equivalent codes) rose from 1.065 to 3.992 for Classic/Neon and from 5.245 to 14.460 for Seventies/Super 8. 200T/500T remains restrained at 2.045; 50D/250D at 1.339. These values exclude grain/glow, combine tone and color differences, and are neither perceptual DeltaE nor stock-accuracy scores. Representative midtone skin hues retain warm channel ordering; this is not comprehensive skin validation.

Preset application still adds no image passes, buffers, or readbacks. Glow amounts/radii and active blur stages were not increased by this pass. Seventies Print now enables nonneutral Richness in the existing per-pixel development stage; per-look 4K benchmarks exercise every complete recipe. Timing depends on hardware/host and is not a playback guarantee.

## Selection And Editing

- Selecting a named preset loads Full mode, Film Gauge, all eight module switches, and a complete set of creative module settings. It can reactivate processing from Bypass or Grain Only.
- Input Color Space, Output Color Space, Film Color Exposure/Temperature/Tint, and Grain Seed are preserved, including their keyframes. Print exposure and RGB balance are part of the creative recipe and are replaced.
- Selecting a named preset replaces all keyframes on recipe-owned settings and module switches. The writes are grouped in one host edit block.
- Each preset loads Print Style = Custom, so the print recipe remains editable. Normal disabled-module greying still applies. A manually selected fixed print style retains its existing locking behavior.
- Editing recipe-owned settings, Film Gauge, Mode, Print Style, or a module switch changes Preset to Custom without resetting anything. Camera balance, color-space, and grain-seed edits do not change the preset label.
- Selecting Custom does nothing to the settings. It is not a reset/default recipe.
- Loading a project, undo/redo, host-generated parameter notifications, or moving the playhead never reapplies a recipe. Rendering reads the ordinary module settings, not the preset selector; saved edits remain authoritative.

Preset application only writes parameters. It adds no rendering passes, buffers, per-pixel branching, or external LUT files. Individual looks can enable existing diffusion/development stages and thus cost more than a color/grain-only recipe. The category browser performs no file I/O in rendering.

## Source And Validation

Recipes and their parameter ownership are defined in `ofx/src/LookPresetConfig.h`. Contributors can propose new recipes without changing the renderer. v0.27 adds [User Presets](USER_PRESETS.md) for portable file save/load. Built-in recipes retain Post Print grain and reset Grain Response along with other creative settings; user files capture the selected response. Resolve's normal saved effect settings remain available.

The stock families are informed by public [Kodak camera-film references](https://www.kodak.com/en/motion/products/camera-films/), but no manufacturer graphs, datasets, proprietary presets, or third-party implementation code are incorporated.

Automated checks cover complete recipe application, valid control ranges, preserved encoding/balance/seed, Custom inheritance, repeated selection, finite/monotonic gray ramps, and edit/restore/recursion policy. v0.29 adds synthetic creative-separation guards, stock-family restraint/texture hierarchy/neutral pivot checks, representative midtone skin channel ordering, Neon cool-shadow/warm-highlight identity, and vintage-shadow softness. The render harness exercises every recipe across all supported input/output spaces, mode overrides, alpha, and all-disabled bypass on CPU/OpenCL, and times every full recipe at 4K. Synthetic appearance checks are not validation against actual film scans. Dropdown placement, grouped undo/redo, project reload, keyframe replacement, and final appearance should also be checked in Resolve on real footage.

v0.30 adds exhaustive filtered-option/stable-ID round trips and invalid-index handling, distinct recipe checks, Neutral equivalence to conversion alone including negative/HDR chips, Portra texture/push hierarchy, reversal/print-stage isolation, B&W neutrality and new-creative color separation. All 37 recipes receive CPU/OpenCL input/output-space coverage and 4K timing. With all four film/print strengths at zero and gamma-2.4 output within 0.01 of zero, the numerical parity comparison also permits less than 1e-6 linear error and less than one 8-bit-equivalent displayed code; this accounts for wide-gamut cancellation amplified by the gamma curve near zero. Other parity tolerances, identity behavior, renderer math and historical math anchors are unchanged. Native dynamic-menu refresh and undo/reload behavior still require host validation; pure mapping tests are not an interactive Resolve session.

v0.31 covers all 46 recipes, the three appended genre categories and unchanged existing IDs. New synthetic checks compare every pair of genre looks without grain/glow, guard against selected existing-creative duplicates, check low-end gradation and relative shadow/midtone behavior, and verify green/amber/violet color identities. A separate nine-look synthetic preview is available as the render harness's tenth output argument. These checks establish internal recipe behavior and numerical separation, not perceptual accuracy to the named films. Real-footage and native host evaluation remain necessary.

v0.32 covers all 52 recipes, including six appended looks and the Graphic Noir category. The tenth preview argument now includes all fifteen movie/genre recipes; the eleventh shows only the six new recipes. Selective-only output conversion receives the same narrowly bounded gamma-2.4 near-black numerical allowance as zero-strength color response: less than 1e-6 linear error, less than one 8-bit-equivalent displayed code, and output magnitude below 0.01. Raw selection mattes, alpha and exact identity remain subject to their original strict comparisons. See [Selective Color](SELECTIVE_COLOR.md) for key, composition and preset-migration coverage.
