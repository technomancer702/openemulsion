# OpenEmulsion v0.32

Experimental Windows x64 genre presets and Selective Color. Stock and movie references are artistic interpretations, not measured film profiles or exact movie grades. Existing response math and recipe values are unchanged; the new finishing operation is neutral by default.

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
