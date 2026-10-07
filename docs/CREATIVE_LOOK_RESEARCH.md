# Creative Look Research

Research and original recipe design for v0.31. The goal is a useful range of
editable looks, not a film-frame matching claim. Sources are interviews with the
cinematographers and production collaborators, rather than commercial look-pack
descriptions. No third-party LUT, preset, image, profile dataset or source code
was incorporated.

## Selection Principles

- Different tonal and color relationships matter more than a long list of titles.
- Thriller should include restrained period photography as well as dense silver
  prints; horror should include bright daylight and saturated expressionism.
- The reference is a particular visual direction or sequence, not every shot in
  a movie. Lighting, art direction, lenses, weather and local grading matter.
- Keep camera balance and color-space choices under the user's control. Avoid
  hiding a fixed camera exposure change inside a recipe.
- Grain is secondary, especially for digitally photographed references. Glow is
  selective, not a default way to announce that a look is cinematic.
- Use existing modules for this pass. No spectral renderer, extra blur pass or
  new per-pixel palette operation is introduced.

## Thriller

### Archive Thriller (Zodiac)

**Source facts:** Harris Savides discusses the Viper capture and his effort to
give the period imagery an older, restrained character. He describes subtle
production design and deliberately subdued colors, and resists treating a whole
film as a handful of generic viewing LUTs. This is not the silver-retention
photochemical process used on Se7en.
[Savides interview, Filmmaker](https://filmmakermagazine.com/archives/issues/winter2007/line_items/there_yet.php).

**Our interpretation:** muted earth color, a small olive shadow bias, gentle
contrast and readable lower midtones, fine light grain, no diffusion. The olive
choice and numeric values are our artistic translation, not a measured palette
or a color recipe specified by Savides. Its restraint is intentional; increasing
contrast to make every thriller loud would undermine this reference.

### Silver Thriller (Se7en)

**Source facts:** Darius Khondji describes Deluxe's CCE silver-retention treatment
on prints, dense blacks and reduced color. He also stresses mixed warm lanterns
and cooler fluorescent light, and discusses the green lighting of the Sloth
sequence. Set preparation and exposure were essential parts of the result.
[Khondji interview excerpt, ASC](https://theasc.com/article/book-excerpt-conversations-with-darius-khondji/).

**Our interpretation:** use the ordinary cinema negative family, with contrast
and drained chroma driven substantially by Print, dirty warm print trims, cooler
shadow separation and mostly monochrome texture. This is distinct from the
existing negative-family Bleach Bypass recipe. Dense is not an excuse to flatten
every shadow step. The preset does not reproduce CCE chemistry or all release
print and subsequent video-master variations.

### Sodium Noir (Nightcrawler)

**Source facts:** Robert Elswit photographed nights with Alexa XT and day scenes
on Kodak 5213. Elswit and Michael Bauman describe practical streetlight backgrounds,
small additions and tunable fixtures emulating metal-halide, sodium and tungsten
sources. The locations' lighting relationships helped define time and place.
[Elswit/Bauman interview, ASC](https://theasc.com/article/breaking-news-nightcrawler/).

**Our interpretation:** amber/yellow warmth against cool darks, stronger highlight
shaping, restrained texture and small halation. The amber/cool emphasis is an
artistic selection from the mixed city-light direction, not the look of every
interior or daylight sequence. Compared with Neon Nights, this is warmer and
less grainy, not another saturated neon palette with a new name.

## Horror

### Folk Dread (The Witch)

**Source facts:** Robert Eggers and Jarin Blaschke describe a desaturated, gray,
oppressive visual direction achieved through sets, costumes and overcast/dawn/dusk
shooting. The film used Alexa rather than the initially desired film stock;
Blaschke favored spare, direct lighting and vintage lenses.
[Eggers/Blaschke production interviews, ASC](https://theasc.com/article/conjuring-a-coven-for-the-witch/).

**Our interpretation:** low chroma with restrained cool balance, gentle highlight
shaping, minimal texture and no glow. Maintain low-end gradation rather than
simulating gloom by crushing every dark value. Desaturating source footage cannot
substitute for that weather, negative fill, costumes or set palette. This is
near-gray naturalism, unlike Arctic Dusk's more apparent blue development split.

### Daylight Dread (Midsommar)

**Source facts:** Pawel Pogorzelski describes a bright-looking production, testing
the DXL2 and lenses, and references Black Narcissus for pastel color rendition,
especially foliage. It is a useful counterexample to equating horror with dark,
hard, desaturated imagery.
[Pogorzelski interview, British Cinematographer](https://britishcinematographer.co.uk/pawel-pogorzelski-midsommar/).

**Our interpretation:** brighter print midtones, softer contrast and pastel-like
chroma, a soft shoulder, light bloom and minimal grain. The recipe is not a
selective foliage correction: saturation/richness affect broad color response.
It cannot create high-key sunlight in a dark source. Brightness is handled with
Print tuning, preserving the user's camera Exposure/Temperature/Tint.

### Giallo Crimson (Suspiria 1977)

**Source facts:** Luciano Tovoli describes Eastman 5254, deliberately extreme
color range, and Technicolor dye-transfer printing with modified color-contrast
handling. Strong colored lighting and a carefully designed palette are central;
Tovoli notes that even a supervised digital transfer is not equivalent to an
original dye-transfer print. This reference is the 1977 film, not the 2018 remake.
[Tovoli interview, ASC](https://theasc.com/article/suspiria-terror-in-technicolor/).

**Our interpretation:** firmer reversal-family tone, bold chroma, red/magenta
print bias and blue-shadow separation with comparatively small diffusion. This
works best with actual colored source lighting. Global balance/split tone cannot
give two equally bright objects independent red and blue lighting, or reconstruct
Technicolor separations. Treat it as an expressive giallo direction, not a dye-
transfer simulation or exact Suspiria palette calibration.

### Crimson Dream (Mandy)

**Source facts:** Benjamin Loeb describes primary reds/blues contrasted with the
early warm natural scenes, stacks of lens filters, strong colored lighting, and
later grading that introduced blue/purple separation into red-filtered images.
The darker nightmare sequences have a different direction from the domestic
opening.
[Loeb production interview, ASC](https://theasc.com/article/mandy-edge-of-darkness/).

**Our interpretation:** denser red/magenta balance, violet-blue shadows, broader
bloom, stronger halation and visible soft texture. Its diffusion and softer
highlight behavior distinguish it from the sharper Giallo Crimson. It does not
reproduce specific filters, local relighting or the entire movie; source colors
and highlight placement still determine the result.

## Sci-Fi

### Simulation Green (The Matrix)

**Source facts:** Bill Pope explains the original 1999 film's two worlds: a cold
blue physical future and a less appealing simulated reality using green filters
and color timing. He shot Super 35 on Kodak 5279/5274 and wanted visible grain.
[Pope interview, ASC's original production coverage](https://theasc.com/article/flashback-the-matrix-cinematography/).

**Our interpretation:** green shadow/lower-midtone split, firmer tone, reduced
chroma, moderate Super 35 grain and no glow. The complementary magenta highlight
split is deliberately disabled. It targets the simulated world, not the blue
ship scenes or a specific home-video master. The primary reference describes
the intent, not a numeric grading prescription.

### Amber Wasteland (2049 Vegas)

**Source facts:** Roger Deakins describes the Vegas sequence's in-camera pink/
orange filter pack, colored light, amber-lit backing and considerable atmosphere.
This is one sequence's direction, not a single look covering Blade Runner 2049.
[Deakins interview, ASC](https://theasc.com/article/uncanny-valley-blade-runner-2049/).

**Our interpretation:** pronounced amber/red Print balance, reduced blue,
softened contrast, extended highlight rolloff, broad restrained bloom and minimal
grain. Unlike Desert Chrome, it does not introduce a contrasting cyan-shadow
direction. Bloom is not depth-dependent atmospheric fog, and the broad channel
trims are not a selective hue remap to one orange palette.

## Verification And Next Steps

All nine recipes pass full parameter/range and repeatability checks, synthetic
finite/monotonic ramps, CPU/OpenCL comparisons in every supported input/output
space and full-recipe 4K timing. Genre intent checks compare all 36 pairs with
grain/glow omitted, check selected existing-creative overlaps, low-end gradation,
relative shadow/midtone brightness, and green/amber/violet identities. These are
engineering checks, not proof of a perceptual film match. Neutral input, source
normalization and output gamma materially affect appearance.

Before claiming closer resemblance, compare normalized real footage: daylight
foliage/white clothing, skin, low-light practicals, saturated red/blue light and
dark neutral surfaces. Review motion and highlights, not just still thumbnails.
Check category switching, undo and project reload in Resolve. Keep one stable
reference direction per look and document subsequent tuning.

Do not pad this set with another cold-blue horror look or an orthochromatic B&W
film label based only on broad saturation. A Lighthouse-style direction would
benefit from independent monochrome channel/spectral weighting; it should not
simply rename Silver Noir. Additional romance, western or faded period recipes
should first demonstrate a useful difference from Soft Portrait, Golden Hour,
Desert Chrome and Seventies Print. Hue-selective palette tools and licensed
reference footage would improve fidelity more than many near-duplicate titles.
