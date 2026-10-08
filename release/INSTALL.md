# Installing OpenEmulsion v0.44.0

This ZIP contains a prebuilt Windows x64 OFX plugin. No build tools or DCTL are needed. Development testing uses DaVinci Resolve 21.1.1 on Windows x64; other host versions and GPU vendors still need community testing.

## Install

1. Close DaVinci Resolve.
2. Extract the ZIP into a normal folder.
3. Copy the entire `OpenEmulsion.ofx.bundle` folder into `C:\Program Files\Common Files\OFX\Plugins`. Create `OFX\Plugins` if it does not exist. Approve the Windows administrator prompt if requested.
4. Keep `Contents\Win64\OpenEmulsion.ofx`, `Contents\Resources`, and all license files inside the bundle. Do not copy only the `.ofx` file or place the bundle inside another bundle.
5. Restart Resolve and look under **OpenFX > OpenEmulsion > OpenEmulsion**.

The extracted download folder is not needed after the bundle has been copied. Do not run the repository's development installer for this release ZIP.

## Update and Uninstall

Close Resolve first. To update, replace only the installed `OpenEmulsion.ofx.bundle` folder with the new complete bundle. To uninstall, remove only that folder, leaving other OFX plugins untouched. Projects using OpenEmulsion may report a missing effect after uninstalling.

Install only one copy. Developers already using `OFX_PLUGIN_PATH` should update that installation rather than also copying the release into the system folder. Duplicate copies with the same plugin identifier can lead to ambiguous loading.

## Quick Start

- Selecting **Standard HDR (PQ)** directly also selects **Rec.2100 / PQ (Rec.2020)** output and refreshes HDR control availability. This does not change input space or viewing tuning. Preset loading, undo/timeline notifications and other rendering choices retain their stored output context; HDR remains inactive if you later explicitly choose a non-PQ output.

- **HDR Viewing**, below SDR Viewing, contains Peak Luminance, Reference White, Exposure Trim (EV) and Highlight Rolloff. Leave the new controls at zero for unchanged v0.41 HDR output. Positive trim brightens post-look light before tone mapping; positive rolloff compresses above-white highlights more, while negative keeps them brighter. They grey out outside HDR rendering. Reference White remains available for PQ Conversion Only. User files save all four controls; Preserve Color Spaces retains the destination values on import.

- For an unconverted Alexa LogC3 clip in a manually managed project, select **ARRI Alexa LogC3 / Wide Gamut 3 (EI 800)** input, **Rec.709 / Gamma 2.4** output and **Output Rendering: Auto**. Auto adds a neutral SDR viewing response before the creative film stages. Do not add a second LogC3-to-709 viewing transform afterward.
- For direct HDR, select **Rec.2100 / PQ (Rec.2020)** output and **Standard HDR (PQ)**, with peak/reference white set to the target (default **1000/203 nits**). Auto also renders HDR for log/linear input sent to PQ. Use Clean Slate to evaluate the foundation; strong Film Tone/Print Tone can reduce specular headroom. Configure Resolve monitoring, export tags and mastering metadata separately. Do not apply another viewing transform to the rendered PQ. HLG and PQ input are not supported yet; calibrated HDR appearance still needs validation.
- **Output Rendering: Conversion Only** retains pre-v0.34 gamut/gamma conversion when another stage provides display rendering. Auto leaves display-ready Rec.709/sRGB input and managed log/linear output alone. Standard SDR is an explicit override for display output; avoid it on already-rendered footage unless deliberate.
- **Film Color > Highlight Color Retention** optionally adds to the SDR foundation's automatic colored-highlight response. Start at zero; increase toward 0.5 for further retention. Zero uses the improved v0.38 default, not the previous response. More retention trades some brightness for color/channel gradation, not a global saturation increase or recovery of clipped source data. The control greys out when the workflow cannot use it.
- For a color-managed timeline, select the color space actually entering the effect, not necessarily the camera's recording space. For DaVinci Wide Gamut/Intermediate processing, choose that input and **Same as Input** output, leaving the project's output transform in place.
- For your own LUT, use **Grain Only** or **Halation, Bloom & Grain Only**, or disable Film Color, Film Development, and Print in Full mode. Texture-only rendering returns the input encoding regardless of the output dropdown.
- Mode selection switches ordinary module Enable toggles. Selective Color defaults to disabled; Full/Color Only retain its explicit choice, while excluded modes disable it without automatically restoring it later. Only Graphic Noir presets enable it automatically. Bloom strength defaults to zero.
- Print's named presets lock their recipe knobs. Select Custom to edit the inherited recipe.
- Preset Category filters the Preset menu without changing the image. All Presets includes every recipe; Custom / Current Settings never resets tuning. Named recipes replace creative tuning/keyframes, not camera balance or color-space settings. Existing saved values remain unchanged.
- Starting Points > Neutral / Clean Slate removes creative film color/tone, development, grain and glow, retaining conversion, camera balance and the selected Output Rendering. Auto thus remains active for log-to-display output. Full mode and ordinary module switches stay enabled for editing; Selective Color stays disabled. Raise Film/Print strengths from zero when adding those responses.
- User Presets > Save/Load captures all current controls, including Output Rendering, before the dialog. Loading restores all saved settings by default and replaces their keyframes. Optional Preserve switches retain destination context; existing v0.27 nodes keep those switches on until unchecked. Old format-1/2 presets load with Conversion Only to preserve their prior rendering; select Auto afterward to adopt the new SDR foundation.
- Grain > Grain Response > Negative & Print optionally lets Print shape the texture. Post Print is the default; texture-only modes keep their original behavior.

Detailed controls and color-space limitations are in the included `docs` folder. Input color space is not auto-detected. This is not a standalone color-space converter, manufacturer viewing LUT, measured-stock simulator, certified HDR mastering system, or ACES output transform.

## Performance and Troubleshooting

OpenCL acceleration runs when Resolve supplies OpenCL image buffers. Otherwise the multithreaded CPU fallback is used. CUDA and Metal are not implemented. Keep unused modules disabled or their effect strengths at zero. Performance depends on GPU, resolution, active modules, and Resolve's own pipeline; benchmark results are not playback guarantees.

If the effect does not appear, check the exact folder structure, confirm there is only one installed copy, and restart Resolve. Check Resolve's video-plugin preferences for whether the effect loaded. If Windows blocks an extracted file, review the download's origin and Windows file properties; do not disable system-wide security features.

The binary is unsigned. Windows or antivirus software may warn about a new unsigned download. A SHA-256 file accompanies the ZIP so downloads can be checked against the publisher's copy; this is an integrity check, not a code signature or security certification.

This release is experimental. A clean installation on a separate machine has not yet been tested, and NVIDIA/Intel GPU paths are not certified. Test on non-critical material and retain backups. Please report reproducible problems at https://github.com/technomancer702/openemulsion/issues.
