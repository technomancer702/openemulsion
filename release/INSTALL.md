# Installing OpenEmulsion v0.30

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

- For an unconverted Alexa LogC3 clip in a manually managed project, select **ARRI Alexa LogC3 / Wide Gamut 3 (EI 800)** input and **Rec.709 / Gamma 2.4** output. Do not add a second LogC3-to-709 conversion afterward.
- For a color-managed timeline, select the color space actually entering the effect, not necessarily the camera's recording space. For DaVinci Wide Gamut/Intermediate processing, choose that input and **Same as Input** output, leaving the project's output transform in place.
- For your own LUT, use **Grain Only** or **Halation, Bloom & Grain Only**, or disable Film Color, Film Development, and Print in Full mode. Texture-only rendering returns the input encoding regardless of the output dropdown.
- Mode selection switches module Enable toggles. Returning to Full enables all modules; turn off individual modules as needed. Bloom strength defaults to zero.
- Print's named presets lock their recipe knobs. Select Custom to edit the inherited recipe.
- Preset Category filters the Preset menu without changing the image. All Presets includes every recipe; Custom / Current Settings never resets tuning. Named recipes replace creative tuning/keyframes, not camera balance or color-space settings. Existing saved values remain unchanged.
- Starting Points > Neutral / Clean Slate removes film color/tone, development, grain and glow, retaining input/output conversion and camera balance. Full mode and module switches stay enabled for editing; raise Film/Print strengths from zero when adding those responses.
- User Presets > Save/Load captures all current controls before the dialog. Loading restores all saved settings by default and replaces their keyframes. Optional Preserve switches retain destination context; existing v0.27 nodes keep those switches on until unchecked.
- Grain > Grain Response > Negative & Print optionally lets Print shape the texture. Post Print is the default; texture-only modes keep their original behavior.

Detailed controls and color-space limitations are in the included `docs` folder. Input color space is not auto-detected. This is not a standalone color-space converter, manufacturer viewing LUT, measured-stock simulator, or complete HDR display transform.

## Performance and Troubleshooting

OpenCL acceleration runs when Resolve supplies OpenCL image buffers. Otherwise the multithreaded CPU fallback is used. CUDA and Metal are not implemented. Keep unused modules disabled or their effect strengths at zero. Performance depends on GPU, resolution, active modules, and Resolve's own pipeline; benchmark results are not playback guarantees.

If the effect does not appear, check the exact folder structure, confirm there is only one installed copy, and restart Resolve. Check Resolve's video-plugin preferences for whether the effect loaded. If Windows blocks an extracted file, review the download's origin and Windows file properties; do not disable system-wide security features.

The binary is unsigned. Windows or antivirus software may warn about a new unsigned download. A SHA-256 file accompanies the ZIP so downloads can be checked against the publisher's copy; this is an integrity check, not a code signature or security certification.

This release is experimental. A clean installation on a separate machine has not yet been tested, and NVIDIA/Intel GPU paths are not certified. Test on non-critical material and retain backups. Please report reproducible problems at https://github.com/technomancer702/openemulsion/issues.
