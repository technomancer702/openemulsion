# OpenEmulsion v0.41

Experimental Windows x64 film emulation with SDR and direct HDR PQ output. Stock and movie references remain artistic interpretations, not measured film profiles or exact movie grades. HDR monitor/host validation is still outstanding.

## Changes in v0.41

- Moved the collapsed SDR Viewing group to the bottom below Selective Color. Its controls, defaults, enabled-state policy, rendering math and preset format are unchanged.
- Added shipping-plugin descriptor checks to keep the group and its three sliders last, after all Selective Color controls.

## Changes in v0.40

- Added a collapsed SDR Viewing group: Viewing Contrast, Highlight Rolloff and Gamut Compression. All three default to zero, reproducing v0.39 exactly. Contrast keeps middle gray fixed; positive Rolloff starts the shoulder earlier; Gamut Compression adjusts display-boundary softening independently of Film Color's control.
- The viewing controls are ignored and greyed out outside SDR rendering. Built-in looks preserve them; format-6 user presets capture them, and complete older presets load with zero adjustments. Preserve Color Spaces also preserves SDR Viewing settings.
- Shared CPU/OpenCL math remains in the existing composite pass, with no extra buffers/passes/readbacks. Extended checks cover curve continuity, exposure ordering, endpoint combinations, preset migration/capture, shipping-plugin startup and GPU parity. Local five-clip comparisons verify exact zero-default and HDR/Conversion Only preservation; stage ramps separate viewing/negative/print response. Strong creative curves can still compress detail; no source reconstruction or spectral/stock calibration is claimed.

## Changes in v0.39

- Corrected a limitation of v0.38: reduced whitening did not establish preserved taillight lens detail. Full-resolution exposure sweeps showed the detail existed in the source but was compressed by SDR rendering.
- Added a smoothly joined square-root intensity shoulder for very bright, strongly colored SDR highlights before the existing tone/gamut response. It reserves more tonal separation for lens texture instead of only retaining hue. A logarithmic trial was rejected for reducing intermediate-intensity detail. No sharpening, spatial processing or reconstruction; saturated lights/reflections may become dimmer.
- Neutral/pale highlights, peak scene-linear RGB at/below one, gray/shadow anchors, HDR, Conversion Only, managed destinations, display-ready Auto and texture/bypass/matte policies are unchanged. Preset recipes/IDs/format 5 are unchanged. Existing SDR highlight renders intentionally change.
- Shared CPU/OpenCL composite math adds no passes/buffers/readbacks. Added independent numerical and intensity-contrast checks plus a full-resolution local before/after/exposure diagnostic using fixed source-selected nearby pixel pairs. Whitening/channel-variance statistics alone are not lens-detail evidence. Final Resolve appearance still needs user validation.

## Changes in v0.38

- Revised the SDR foundation's bright colored-emitter response. Previously, luminance-only highlight mapping pushed saturated red taillights toward white, making existing channel variation hard to see even with creative controls neutral. A peak-aware rational shoulder now preserves more color automatically, with a brightness tradeoff; no blanket exposure or saturation change.
- The alternate shoulder joins the original at peak scene-linear RGB one with matching value/slope. Its chroma gate is exposure invariant, avoiding the brightness reversals caused by simply increasing an intensity-ramped blend. Neutral/pale colors and colors whose peak is at most one retain the previous response. Both endpoints use radial gamut mapping; the retention slider remains an affine display-linear RGB blend.
- Highlight Color Retention adds to the new SDR default (effective fully gated blend 0.5 at zero, 0.8 at one). Zero no longer restores pre-v0.38 SDR rendering. Built-in look recipes, IDs and preset format 5 are unchanged; old SDR project/preset renders intentionally change in these highlights. HDR, Conversion Only, display-ready Auto, managed log/linear and texture/bypass/matte policies are unchanged.
- Shared CPU/OpenCL math stays in the existing composite pass, with no new buffers, blur passes or readbacks. Extended tests cover independent double-precision references, shoulder continuity, bounded/ordered exposure ramps, hue direction, slider continuity, neutral/low-intensity preservation and host startup.
- Added optional local full-precision footage comparisons and source-keyed taillight-region measurements against a retained pre-update native bridge, including exact HDR/Conversion Only checks. Private footage, previews and reports are ignored and not packaged. The plugin cannot recover detail already clipped in camera/source channels; strong creative tone/glow settings can still hide detail. Resolve appearance remains a native-host check.

## Changes in v0.37

- Fixed the crash when adding v0.36 to a clip: the new top-level HDR sliders passed no module parent to a helper that unconditionally dereferenced it. Root-level sliders now omit the parent assignment, as required by OFX; grouped module controls retain their existing parents.
- Added a minimal host regression that loads the actual OFX binary, describes Filter/General controls, and creates/destroys SDR/HDR instances. Verifies HDR slider defaults, page order, group references and initial enabled states. The unfixed v0.36 binary reproduces the descriptor crash under this test.
- Rendering math, preset recipes, parameter IDs and preset format 5 are unchanged. Resolve UI/playback and calibrated HDR appearance still require native-host confirmation.

## Changes in v0.36

- Added output-only Rec.2100 / PQ (Rec.2020) and Standard HDR (PQ) rendering. Auto renders HDR for scene-log/linear input sent to PQ. Existing input/output/rendering IDs, SDR math, managed log/linear and texture-only behavior are retained.
- Added top-level HDR Peak Luminance (400-10000 nits, default 1000) and HDR Reference White (80-300 nits, default 203). Context is preserved by built-in looks; controls grey out when inapplicable. Peak requires HDR rendering; reference white also defines Conversion Only PQ scaling.
- Original HDR luminance rendering runs after creative, grain/glow and selective finishing, not through an SDR curve first. Rec.2020 radial gamut mapping and ST 2084 absolute PQ encoding use shared CPU/OpenCL math in the existing composite pass, with no additional passes/buffers/readbacks. Neutral-anchored HDR matrix evaluation prevents PQ amplification of neutral-axis float rounding.
- During HDR rendering only, negative/print shoulder ceilings and print gamut headroom adapt to the selected peak/reference-white ratio. Print-heavy recipes can retain above-white highlights without stretching an SDR-limited result. Existing toe/pivot/knee/contrast/palette settings and all SDR/managed/Conversion Only response parameters are retained. A stable equivalent PQ evaluation avoids float highlight-step reversals near the peak.
- Conversion Only PQ skips viewing/gamut mapping and encodes relative linear white using the reference-white setting. Physical PQ endpoints clip negatives/above-10000-nit channels. Standard SDR does not render HDR output. Highlight Color Retention remains SDR-only.
- Portable preset format 5 captures HDR output context. Complete format-1/2/3/4 files receive 1000/203 defaults without changing existing choices or creative tuning. Older builds cannot load format-5 files; mixed/incomplete schemas are rejected.
- Extended numerical tests cover independent PQ/matrix anchors, gray/white/black, smooth joins, peak bounds, all-space/recipe CPU/OpenCL parity, queue ordering, mode/matte isolation, presets and comparative 4K HDR timings. Added local numeric HDR footage checks using retained precision inputs; footage/results are never packaged or uploaded.
- This is artistic HDR rendering, not an ACES/RCM output transform, inverse tone mapper or HDR10 metadata writer. Strong creative film/print shoulders can suppress HDR highlight headroom. Configure Resolve HDR monitoring/export separately. HLG and PQ input are not implemented; calibrated HDR display/Resolve appearance and other GPUs remain unverified. See `docs/COLOR_SPACES.md`.

## Changes in v0.35

- Added Highlight Color Retention inside Film Color below Gamut Compression. Zero/default retains v0.34 rendering. The 0-1 slider uses a deliberately restrained peak-channel blend, trading some highlight brightness for chroma without globally increasing saturation. Neutral grays and lower-intensity colors remain unchanged.
- The control requires Film Color and active SDR rendering. Greys out and is ignored in Conversion Only, managed log/linear output, Auto with display input, disabled Film Color, texture/bypass, full-strength Mono Negative and Selection Matte. Independent of creative Film Color/Tone Strength; available on Clean Slate.
- Uses shared CPU/OpenCL math in the existing composite pass, with no new image passes, buffers or readbacks. Source glow extraction, selection keys and grain coordinates stay unchanged. All built-in recipes retain zero retention and their previous appearance.
- Portable preset format 4 captures the new control. Complete format-1/2/3 files migrate with retention zero; existing rendering policies and settings remain intact. Older plugin builds cannot read new format-4 files.
- Added optional local original-footage bench tooling, precision/packing-level checks, source-hashed float comparisons, region statistics and tagged browser previews. Test footage and generated results are ignored, never uploaded or included in release ZIPs; Python/PyAV are developer-only dependencies.
- Expanded tests cover neutral/below-threshold preservation, smooth amount/gate/channel transitions, bounded monotonic exposure ramps, UI/render enable policy, all-space/recipe CPU/OpenCL parity, preset capture/migration and comparative 4K GPU-resident timings. Native Resolve appearance and host workflow still need confirmation. See `docs/COLOR_SPACES.md` and `docs/COLOR_BENCH.md`.

## Changes in v0.34

- Added Output Rendering below Output Color Space: Auto (default), Conversion Only, and Standard SDR. Auto supplies an original scene-to-display response for log/linear input sent to Rec.709/Gamma 2.4 or sRGB. Applies to every creative recipe and Neutral, not only Clean Slate.
- Deeper shadow placement without a black pedestal, continuous highlight rolloff, and smooth linear-light gamut compression for saturated/overbright emitters. No blanket saturation boost or imported stock/profile/LUT data.
- Viewing response runs after enabled Film Color camera balance, before the creative negative/development/print stages. Creative recipe numbers and film/print algorithms are unchanged. Auto does not double-render display-ready input or managed log/linear output; texture-only, disabled, bypass and matte workflows stay isolated.
- Source glow extraction and selective-color keys remain unchanged; grain coordinates remain stable. Grain weighting follows the newly rendered tonal values at its chosen insertion point. New operation is in the existing CPU/OpenCL composite pass, with no extra image passes, buffers or readbacks.
- Portable preset format 3 includes Output Rendering in the color-space context. Complete format-1/2 files retain all prior tuning and migrate to Conversion Only; choose Auto to use the improved foundation. Built-in presets preserve the selected rendering policy.
- Added a seventh test suite with viewing-curve anchors, continuity/gradation, gamut/skin/LED behavior, all-space policy and all-recipe module isolation. Extended GPU tests cover all three options, exposure ordering, alpha and exact display-ready/texture/bypass preservation. Existing all-recipe CPU/OpenCL checks remain active. Includes comparative 4K GPU-resident timings.
- Native Resolve appearance and performance still need footage evaluation. This is not an ARRI/ACES transform match, dehazing tool, automatic exposure correction, or full HDR renderer. See `docs/COLOR_SPACES.md`.

## Changes in v0.33

- Selective Color Enable now defaults to off in new instances and user-preset defaults. All built-in recipes disable it except the three Graphic Noir / Sin City accent recipes.
- Full/Color Only mode edits preserve an explicit Selective Color choice instead of automatically enabling it. Texture-only, bypass and glow-matte modes disable it; returning does not restore it automatically.
- Saved project values and user-preset snapshots retain their stored switch. Selecting Custom remains non-destructive. Rendering math, preset RGB values and file format are unchanged.
- Added all-recipe opt-in, default-snapshot and exhaustive mode-mask/transition checks. CPU/OpenCL regression coverage remains unchanged.

## Changes in v0.32

- Added Winter Crime (Fargo 1996), Steel Blue (Terminator 2) and Nostromo (Alien 1979), based on primary production interviews and original editable recipes.
- Added Graphic Noir category and red, blue and yellow Sin City interpretations, bringing the library to 52 named looks plus Custom. IDs are appended; older recipe values are unchanged.
- Added independent Selective Color with Amount, Keep Hue, Hue Range, Hue Feather, Minimum Saturation and Image/Selection Matte. Available in Full/Color Only, with Enable first and normal disabled-control greying. Amount defaults to zero.
- Source-keyed selection before film/texture avoids added grain or print shifts changing the key. Final desaturation after print/glow/grain keeps rejected pixels neutral. One hue interval, not object recognition, recoloring or automatic comic-book lighting.
- CPU/OpenCL share the new per-pixel operation in the existing composite pass; no extra buffers, blur passes or readbacks. Exact zero/disabled/bypass behavior remains, and selective-only output conversion is supported. Matte preserves alpha and is not gamma/log-encoded.
- Portable presets now use format version 2. Complete version-1 files migrate with all prior settings retained and Selective Color neutral/disabled. New files require v0.32 or later. Strict incomplete/mixed-schema rejection is retained.
- Added hue wrap/feather, neutral/HDR/negative handling, working-luminance, exact retention/zero, raw matte/alpha, independent output conversion, grain/glow finishing, all-space/mode/queue parity and legacy-file migration checks. Full recipe coverage and 4K timing include all 52 looks.
- Film-name references remain artistic interpretations, not exact matches or endorsements. Native UI, moving/noisy footage, Undo and project reload still need Resolve evaluation. See `docs/SELECTIVE_COLOR.md` and `docs/CREATIVE_LOOK_RESEARCH.md`.

## Changes in v0.31

- Removed `(Inspired)` from stock preset labels. The documentation and tooltip retain the artistic-approximation warning; no calibration claim is implied.
- Added Thriller, Horror and Sci-Fi categories with nine complete editable recipes, bringing the library to 46 named looks plus Custom.
- Thriller: Archive Thriller (Zodiac), Silver Thriller (Se7en), Sodium Noir (Nightcrawler). Restrained period color, print-led silver-retention contrast, and mixed city-night light are separate directions.
- Horror: Folk Dread (The Witch), Daylight Dread (Midsommar), Giallo Crimson (Suspiria 1977), Crimson Dream (Mandy). Covers near-gray naturalism, bright pastel daylight, sharper saturated giallo and softer red/violet nightmare imagery.
- Sci-Fi: Simulation Green (The Matrix) and Amber Wasteland (2049 Vegas). References the 1999 simulated world and the Vegas sequence specifically, not a single grade for either whole film.
- Added primary cinematographer/production sources and original design rationale in `docs/CREATIVE_LOOK_RESEARCH.md`. Source lighting, local grading, atmosphere and set palettes cannot be recreated by a global recipe; no third-party LUTs or presets are included.
- Appended new stored IDs without reordering existing recipes/categories. Preserves camera balance, input/output color spaces and grain seed. All original recipes and user-preset format remain unchanged.
- Added all-pair genre color-only separation, selected existing-look overlap, low-end gradation, relative shadow/midtone and green/amber/violet intent checks, plus a nine-look synthetic preview. All 46 recipes receive CPU/OpenCL color-space coverage and full-recipe 4K timing.
- Uses existing rendering stages only, with no new passes, buffers or dependencies. Glow-enabled recipes can cost more than no-glow recipes. Real-footage and native Resolve menu/undo/reload validation remain necessary.

## Changes in v0.30

- Added Preset Category directly below Film Gauge and a filtered Preset menu. Browsing categories never applies a look. All Presets includes every recipe, with Neutral first after Custom.
- Added Neutral / Clean Slate: zero creative film/print strengths, development, glow and grain. Keeps chosen input/output conversion, camera balance and grain seed. Full mode and module switches remain enabled; Film/Print strengths start at zero.
- Expanded from 12 to 37 named recipes: five cinema-negative targets, eleven still-negative targets, four color reversal targets, four monochrome looks, two print-inspired looks, ten creative looks, and Neutral.
- Still/reversal targets include Portra 160/400/800 and pushed variants, Ektar, Gold, Ultramax, PRO 400H, Superia, C200, Kodachrome, Ektachrome, Velvia and Provia. Added VERITA, Tri-X, HP5, 2383/2393-inspired print looks, Desert Chrome, Arctic Dusk, Golden Hour and Faded Instant. Names identify inspiration, not calibration or endorsement; no third-party recipes/profile data are bundled.
- Stable hidden recipe IDs are separate from transient filtered-menu positions. Old choices keep their IDs, saved controls stay authoritative, user-file format is unchanged, and category/project/undo notifications never reapply recipes.
- Added exhaustive category mapping, Neutral equivalence, distinct recipe, stock grain hierarchy, reversal/print isolation and creative-separation checks. CPU/OpenCL coverage and full-recipe 4K timing now include all 37 recipes. Near-zero gamma output checks bound linear-light error and displayed code error; exact bypass checks remain unchanged.
- No extra rendering passes, buffers, readbacks or dependencies. Some new creative looks use existing bloom/development stages. Native menu refresh, undo/reload and final appearance still need Resolve validation.

## Changes in v0.29

- Classic Cinema: richer color, denser print contrast, and deeper shadows.
- Neon Nights: more visible cool-shadow/warm-highlight separation and a cleaner print palette, without increasing glow strength/radii or adding blur passes.
- Seventies Print: warmer, more muted print color and softer lifted shadows.
- Super 8 Home Movie: brighter, more colorful reversal-inspired palette, retaining its gauge and coarse texture.
- 250D, 200T, and 500T: modest tone/shoulder/print refinements and restrained texture weighting. No automatic tungsten/daylight white-balance shift.
- 50D, Soft Portrait, Bleach Bypass, B&W Reversal, and Silver Noir retain their previous recipes.
- Added synthetic creative-separation, stock-family/texture/neutral-pivot, representative skin-hue, split-tone, and vintage-shadow checks, plus full-recipe 4K benchmarks. All looks retain CPU/OpenCL and color-space/mode parity coverage.
- Existing nodes and user preset snapshots keep their stored values. Select Custom, then a named preset to load its revision. No choice reordering, parameter/schema changes, or new rendering passes. Real-footage comparisons and calibrated film-scan validation remain outstanding; see `docs/LOOK_PRESETS.md`.

## Changes in v0.28

- User preset export now reads current host/UI parameter values immediately when Save is clicked, before opening the modal dialog, instead of sampling the button callback's time afterward.
- Loading restores all saved rendering settings by default, including input/output spaces, exposure/temperature/tint, and grain seed. Preserve switches are now opt-in for new nodes; existing v0.27 nodes retain their stored switches until unchecked.
- Added typed host-binding capture tests, immutable snapshot checks across simulated dialog-time changes, and complete default restoration checks. Existing format-version-1 files remain readable; adjustments absent from an older file must be resaved from the tuned node.
- Native Resolve save/reload comparison and undo behavior still require host validation. Presets store control snapshots, not animation curves, other nodes, or Resolve's external blend/bypass state.

## Changes in v0.27

- Added native Save Preset / Load Preset dialogs and portable `.oepreset` JSON snapshots. In v0.27, camera balance, color spaces, and grain seed were preserved by default; v0.28 makes preservation opt-in.
- Added complete preset validation, Unicode file paths, atomic replacement, and malformed/oversize/duplicate-field rejection before applying any settings. Snapshot imports replace creative keyframes in one undoable edit; animation curves are not exported.
- Added Grain Response: Post Print (original/default) or Negative & Print. The new optional response keys grain after Film Color/Development and adds it before Print, so print tone/color shapes the texture. It does not add buffers, readbacks, blur passes, or extra grain evaluations.
- Retained original texture-only grain, source-keyed halation/bloom, exact zero/disabled behavior, Mono finishing, stable grain coordinates, and all built-in looks. The new selector is unavailable in texture-only modes.
- Added preset I/O/preservation tests and CPU/OpenCL grain-stage parity, interaction, isolation, and 4K benchmark cases. See `docs/USER_PRESETS.md` and `docs/TEXTURE_CONTROLS.md` for limits and host validation still needed.

## Included

- Fifty-two categorized editable recipes, including Neutral, stock and creative interpretations, plus non-destructive Custom.
- Portable user preset snapshots with selective context preservation.
- Six original creative negative families, including monochrome finishing.
- Independent Film Color, Film Development, Print, Halation, Aura, Bloom, Grain and Selective Color modules, with mode-driven toggles and disabled-control greying.
- Film Color's continuous gamut-compression amount, with the near-zero jump fixed.
- Full, Standard, Extended, and Custom print styles, with inherited and locked preset recipes.
- LogC3 EI-800, LogC4, Sony, Blackmagic, RED, Canon, Panasonic, ACES, DaVinci Wide Gamut, and standard display/linear input choices.
- Smooth resolution-aware halation/Aura, separate linear-light bloom, and diagnostic mattes.
- Procedural grain styles, size/softness/roughness, desqueeze, channel intensity, and gauge presets.
- Push/Pull, Richness, and Split Tone; Push/Pull preserves grain geometry.
- OpenCL acceleration, multithreaded CPU fallback, and an original plugin icon.

## Limitations

Windows x64 only; no CUDA, Metal, macOS, Linux, or native Windows ARM build. OpenCL needs host-provided OpenCL buffers. Testing uses Resolve 21.1.1 and an AMD GPU; other systems need testing. The binary is unsigned and the release is experimental, not production-certified. Profiles are artistic approximations, not measured film stocks. See `docs/COLOR_SPACES.md` for unsupported encodings, metadata assumptions, and workflow limitations.

Project source, documentation, build scripts, and tests are in the matching source ZIP. External Resolve/OpenFX developer files are required to rebuild and are not redistributed. Compiler runtime notices remain inside the installed plugin bundle.
