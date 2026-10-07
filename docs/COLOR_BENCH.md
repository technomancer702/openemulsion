# Offline Color Bench

Optional developer tooling, separate from the installed plugin. It renders original
footage through OpenEmulsion's native CPU color stages, using the same headers as
the CPU/OpenCL plugin. No Resolve scripting or screen capture is required.

The plugin does not need Python, PyAV or this DLL. The bench adds no runtime work
to Resolve and does not replace its installed OFX binary.

## Setup

After the usual Windows build setup:

```powershell
cmake -S ofx -B build/ofx -DBUILD_COLOR_BENCH=ON
cmake --build build/ofx
python -m pip install --target build/tuning-python-deps -r tools/requirements-color-bench.txt
python tools/test_color_bench.py
ctest --test-dir build/ofx --output-on-failure
```

Place footage in `test footage/`, which is ignored by Git. Results, float arrays
and previews stay in ignored `analysis/`. Neither original clips nor rendered
frames are release assets. No media is uploaded by these tools.

## Render

```powershell
python tools/color_bench.py --source 0
```

`--source` is an explicit native color-space ID: 0 = Alexa LogC3 / AWG3 (EI 800).
Container BT.709 transfer/primaries tags are not assumed to identify camera log.
All clips in one run must share the selected encoding/gamut. A LogC3 curve alone
does not establish the gamut; use the correct input primaries too.

Defaults: 20%, 50%, 80% frame positions, integer stride 2, Neutral / Clean Slate
and 50D Daylight, production SDR, conversion-only, and four experimental
peak-channel blend amounts. Use `--presets`, `--samples`, `--blends`, `--stride`,
`--output` and `--rois` to narrow a run. No temporal or spatial effects are
included: this is **Color Only**, not a complete look/playback benchmark.

Production Highlight Color Retention comparisons (v0.35 onward):

```powershell
python tools/color_bench.py --source 0 --blends --retentions 0.5 1 --output analysis/color-bench-v035
```

The bare `--blends` disables experimental models; `--retentions` adds actual
plugin slider values. They do not stack in the generated models. Zero/default
production retains the baseline. To check against a retained v0.34 local bench:

```powershell
python tools/check_color_reference.py analysis/color-bench
```

The checker requires existing input/output float arrays and verifies exact
zero-retention output, without needing the original clips again.

HDR PQ numeric evaluation (v0.36 onward), using retained source floats:

```powershell
python tools/check_hdr_footage.py analysis/color-bench
```

Runs Clean Slate/50D at 400/1000/4000-nit targets and 203-nit white by default.
The ignored JSON report records source/DLL hashes, settings, alpha checks,
peak-code bounds and independently decoded Rec.2020 luminance in nits.
`--presets`, `--peaks`, `--white`, `--output` narrow/extend a run. All inputs
must share the source ID recorded by the reference bench. Percentiles include
burn-ins and are diagnostics, not quality scores. This does not generate an
SDR preview pretending to display HDR; a calibrated HDR monitor is required
to judge PQ appearance. The ordinary HTML bench remains SDR-only.

The decoder currently requires integer planar YUV 4:4:4 (8-16 bit), including
the supplied 12-bit ProRes 4444 clips. It fails on subsampled formats rather than
silently choosing a chroma reconstruction. `--matrix` selects YCbCr *packing*
coefficients (ITU709 by default), separate from camera gamut and log transfer.
`--range auto` respects decoded MPEG/legal or JPEG/full tags, rejecting unknown
levels; use an override only when known. Direct plane conversion retains code
precision, negative reconstructed RGB and nominal superwhites. It does not apply
the BT.709 OETF to footage marked as LogC3.

Packing equations follow [ITU-R BT.709](https://www.itu.int/rec/R-REC-BT.709)
and the [Khronos Data Format Specification](https://registry.khronos.org/DataFormat/specs/1.4/dataformat.1.4.html).
PyAV performs the compressed video decode; NumPy unpacks decoded planes and
applies nominal-level normalization and the non-constant-luminance matrix.

## Regions

Optional JSON identifies normalized, axis-aligned rectangles per filename:

```json
{
  "example.mov": {
    "exclude": [[0, 0.73, 0.29, 1]],
    "regions": {"Skin": [0.3, 0.1, 0.5, 0.4]}
  }
}
```

Run with `--rois analysis/bench-rois.json`. Exclusions affect full-frame summary
metrics only, not the visible image, scopes or named regions. There are no
automatic burn-in exclusions. Regions are stationary and may cover different
objects as the camera or subject moves; inspect each frame. Samples are stride
decimated, not filtered. Use stride 1 when measuring tiny highlight detail.

## Interpretation

Open `analysis/color-bench/index.html` directly in a browser. Compare clips,
frames, presets, render models and cropped regions. Output float `.npy` arrays
retain values before PNG clipping. The manifest records settings, source hashes,
native DLL hash, Git revision/dirty state, decoder version and input metadata.

Rec.709/Gamma 2.4 output values are decoded to display linear and then sRGB
encoded for tagged browser PNG previews. The preview is not a calibrated Resolve
viewer comparison. Source linear arrays are in linear Rec.709 primaries, not
camera-native primaries; negative channels can represent out-of-gamut color.

Near-white pixel counts are **not proof of clipping**. Code saturation uses
nonnegative RGB for a bounded diagnostic; negative channels are still retained
in float arrays and counted separately in the range statistics. It is an
observational statistic, not a perceptual quality or film-accuracy score. Scopes
are output code distributions/waveforms, not HDR luminance in nits. More
saturation, darker shadows or closer similarity to another plugin are not
automatic improvements.

**Peak blend** is an independent, bench-only experiment blending production
luminance-based rendering toward peak-RGB scaling in bright colored pixels.
It trades highlight brightness/approach-to-white for stronger retained chroma.
It does not alter the installed plugin and is not a spectral film model. Neutral
grays and lower-intensity pixels retain the production response. Evaluate all
shots before choosing an algorithm; do not optimize one taillight at the expense
of other footage.
