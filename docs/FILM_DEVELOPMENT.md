# Film Development (v0.19)

An independently switchable creative stage between Film Color and Print, available in Full and Color Only. Its first control is Enable. Film Color may be disabled while Development remains enabled; camera exposure/balance still belongs only to Film Color. Development ignores Film Color/Print strengths and does not rewrite their settings or print presets.

Push/Pull, Color Richness, and Split Tone default to zero. At those defaults the stage is skipped completely, including output conversion when Development is the only enabled color stage. The other split knobs do nothing until Split Tone is nonzero. Disabling Development restores the original color and grain behavior without resetting its knobs.

## Controls

- `Push / Pull`: -3 to +3, default 0. Positive values increase contrast above middle gray, deepen lower midtones, and introduce a neutral shadow fog/lift. Negative values soften contrast, suppress the deepest shadows, and reduce grain strength. Middle gray remains anchored. This is a creative development scale, not calibrated laboratory processing stops or camera exposure compensation.
- `Color Richness`: -1 to +1, default 0. Positive values enrich muted colors more than already saturated ones; negative values reduce muted chroma. Neutral gray and working-space luminance are preserved. This is separate from ordinary Saturation and Negative Density.
- `Split Tone`: 0 to 1, default 0. Colors shadows toward Shadow Hue and highlights along the opposite chromatic direction. It preserves working-space luminance and does not tint zero-luminance black by itself.
- `Shadow Hue`: 0-360 degrees, default 220 (blue/cyan shadows with warm highlights). Red is 0, green is 120, blue is 240. 0 and 360 are identical.
- `Split Pivot`: 0.2-0.8, default 0.46135613, the scene-linear 18% gray pivot in the managed perceptual domain. These are not camera log values, stops, or output nits.
- `Neutral Width`: 0-0.3, default 0.1. Defines a completely unaffected interval centered on Split Pivot. At zero, the pivot remains neutral and the two sides still join smoothly.
- `Shadow Intensity`, `Highlight Intensity`: 0-2, default 1. Independent multipliers on their respective split-tone sides. Zero disables that side.

Print response runs afterward and may compress or modify the final appearance. Full-strength Mono Negative neutralizes the final composite after development, print, and texture, so Split Tone cannot leak color into Mono. At partial Film Color Strength, the existing partial monochrome finishing remains in effect.

## Texture and Color Management

Push/Pull scales enabled grain strength by `2^(0.22 * amount)` in Full mode. Positive amounts make grain stronger; negative amounts make it gentler. At a fixed frame/seed it does not change grain size, sampling coordinates, stretch, or the noise pattern. Size remains controlled by Grain Size, style, Film Gauge, and render resolution. This fixes the grain enlargement/sliding present in v0.17-v0.18. It never enables zero-strength or disabled grain. It does not alter halation selection/spread, Aura, or the original source-highlight key. Tonal weights still respond to the developed image's luminance.

Grain Only, Halation, Bloom & Grain Only, Halation Matte, Bloom Matte, and Bypass ignore all Development controls, including grain coupling. For your own LUT in Full mode, disable Film Color, Film Development, and Print. Texture-only output preserves the input encoding. Non-neutral Development on its own counts as a color stage and honors the Output Color Space selector.

## Original Math and Limits

No proprietary stock measurements, LUTs, shaders, or equations are used. This is an artistic model, not a physically calibrated emulsion or reconstruction of another plugin.

Push/Pull uses a monotonic rational shadow curve and linear upper-range contrast around a fixed pivot. Contrast is prepared once as `2^(0.18 * amount)`. Positive shadow fog and negative shadow suppression use a squared distance below the pivot. RGB chroma scales with luminance away from black; near black the added fog is predominantly neutral. Negative-luminance inputs are not forced through the development tone curve.

Richness scales chroma by `1 + 0.65 * amount / (1 + 4 * relativeChroma^2)`. Relative chroma is the channel span divided by the larger of luminance and 0.08. Split Tone uses a frame-prepared RGB hue direction with its luminance removed, opposing smooth shadow/highlight masks, and a near-black envelope. The hue direction's sign is reversed for highlights; this is not an HSV hue rotation or calibrated film dye model.

Development itself has no hard RGB clamp, highlight shoulder, or guaranteed output-gamut mapping. Strong richness/split settings can generate out-of-gamut values, particularly when Print and Film Color compression are disabled. Existing enabled negative/print finishing and downstream color management determine the final range. It is not a full HDR display transform.

All calculations share C++/OpenCL math and run in the existing final image pass. No extra image passes, frame textures, or GPU readbacks are added. Exponentials and hue setup run once per render, not per pixel.

## Verification

Tests cover exact neutral defaults and inactive-stage isolation, monotonic neutral/HDR ramps, fixed gray, neutral Richness, luminance-preserving opposing split hues, dead-zone isolation, intensity endpoints, Push/Pull grain strength coupling with fixed geometry across every style/gauge/stretch and HD/4K/8K heights, all module masks, all 15 inputs and six modes, alpha, and Mono preservation. OpenCL rendered-grain residual checks also verify spatial stability for every style. Grain tests cover stretch correlation/variance and exact channel multipliers. Host UI, animation/save/reload, undo/redo, and subjective footage appearance still require Resolve checks.

The optional fifth preview argument adds a development chart:

```powershell
.\build\ofx\HalationTests.exe analysis\grain.bmp analysis\response.bmp analysis\texture.bmp analysis\mono.bmp analysis\development.bmp
```

Development columns are Pull -2, Neutral, Push +2, and Split Tone + Richness + 2x grain stretch. Rows are gray, warm/skin-like colors, saturated colors, and grain. This is synthetic, not a recommended preset or validation against measured scans.

## Public Workflow References

[Filmbox's lab guide](https://videovillage.com/learn/filmbox/full-guide/lab-module) documents development, richness, and opposing split-tone controls. [Dehancer's developer guide](https://www.dehancer.com/learn/articles/dehancer-film-developer) discusses creative control of contrast, gamma, and color development. Their public workflow descriptions inform the control surface, not our mathematical implementation.
