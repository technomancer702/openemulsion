# OpenEmulsion v0.44.0 - 52 Presets, Selective Color, and HDR

OpenEmulsion is a free, open-source film emulation plugin for DaVinci Resolve. This is our second public release, bringing together everything added since v0.22: a much larger look library, portable presets, selective color, improved SDR rendering, and experimental HDR PQ output.

Use the complete film pipeline, build your own look from individual modules, or add grain, halation, and bloom alongside your existing grade.

## What's New Since v0.22

### 52 Built-in Looks

A new **Preset Category** dropdown makes the library easier to browse. Categories filter the menu without applying a look, and presets remain editable.

- **Film stocks and print looks:** cinema negatives, Portra, Ektar, Gold, Kodachrome, Ektachrome, Velvia, Provia, monochrome looks, and 2383/2393 print looks.
- **Movie and genre looks:** thriller, horror, sci-fi, vintage, and other creative directions, including Zodiac, Se7en, The Witch, Midsommar, Suspiria, Fargo, Terminator 2, and Alien.
- **Graphic Noir:** Sin City-style red, blue, and yellow accent looks.
- **Neutral / Clean Slate:** remove creative color and texture effects while retaining your chosen color conversion and viewing settings. **Custom / Current Settings** keeps your adjustments intact.

Stock and movie names describe artistic looks, not measured stock profiles or exact matches to an entire film.

### Save and Share Your Own Presets

Save complete control snapshots as portable **`.oepreset` files**, then load them into another instance or share them with other users. Loading restores all saved rendering settings by default. Optional Preserve switches let you retain the destination's camera balance, color-space/viewing settings, or grain seed.

Presets capture current settings, not animation curves, other nodes, or project color management. Saving now captures the current controls before opening the dialog, fixing an early issue with tuned looks not being fully saved.

### Selective Color

Keep a chosen hue while making the rest of the image monochrome. Adjust **Amount, Keep Hue, Hue Range, Hue Feather, and Minimum Saturation**, with a **Selection Matte** view for checking the key.

Selective Color defaults to off and is only enabled automatically by the Graphic Noir presets. It selects colors, not objects, so a shot-specific key may be needed to isolate the subject.

### Better SDR Rendering and Highlight Detail

- **Output Rendering** now separates **Auto**, **Conversion Only**, **Standard SDR**, and **Standard HDR (PQ)** workflows.
- **Auto** adds a scene-to-display response when converting log/linear footage to SDR, addressing the flat, grey-looking output seen in earlier versions. Display-ready inputs and managed log/linear destinations are left alone by Auto.
- Bright, saturated lights receive improved color and tonal separation, helping preserve detail in subjects such as red taillights. **Highlight Color Retention** offers additional adjustment.
- **SDR Viewing** adds **Viewing Contrast, Highlight Rolloff, and Gamut Compression**, separate from the creative film controls.
- Selected creative slider ranges and grain softness have been tuned for more useful adjustment.

The highlight changes cannot recover detail already clipped in the source. Strong film, print, or glow settings can still hide detail.

### Experimental HDR PQ Output

Direct **Rec.2100 / PQ (Rec.2020)** output is now available. Selecting **Standard HDR (PQ)** automatically selects the corresponding output space.

The **HDR Viewing** group provides **Peak Luminance, Reference White, Exposure Trim, and Highlight Rolloff**. HDR rendering operates without first squeezing the image through the SDR viewing curve, and film/print highlight headroom adapts to the selected HDR target.

HDR remains experimental and has not been validated on a calibrated HDR monitor. Configure Resolve's monitoring, export settings, and mastering metadata separately. HLG and PQ input are not supported.

### More Film Gauges and Grain Response

Film Gauge now includes **8 mm, Super 8, 16 mm, Super 16, 35 mm, Super 35, 65 mm, 70 mm, and Custom**. These adjust texture scale and strength, not image framing or cropping.

The new **Negative & Print** Grain Response places grain before Print so the print response shapes the texture. **Post Print** remains the default, and texture-only modes retain their existing behavior.

## Updating From v0.22

1. Close DaVinci Resolve and back up important projects.
2. Download and extract **`OpenEmulsion-v0.44.0-Windows-x64.zip`** from this release's assets.
3. Replace your installed **`OpenEmulsion.ofx.bundle`** with the complete new bundle in **`C:\Program Files\Common Files\OFX\Plugins`**. Keep only one installed copy.
4. Restart Resolve. No compiler, Python, or DCTL is needed for the prebuilt plugin.

**Existing projects may look different.** The SDR viewing foundation, colored-highlight response, and some slider mappings have intentionally changed since v0.22. Review upgraded shots before exporting; this is not a pixel-identical replacement for the older renderer.

For log footage going directly to SDR, choose the correct input space, **Rec.709 / Gamma 2.4** output, and **Auto**. Avoid adding a second log-to-display transform afterward. For RCM/ACES workflows, choose the space actually entering the plugin, **Same as Input** output, and **Conversion Only**, leaving display rendering to Resolve.

## Compatibility and Testing

- **Windows x64 only.** macOS, Linux, and native Windows ARM builds are not available.
- **OpenCL acceleration** when Resolve supplies OpenCL image buffers; otherwise a multithreaded CPU fallback. CUDA and Metal are not implemented.
- Development testing uses **DaVinci Resolve 21.1.1 on Windows with an AMD OpenCL GPU**. Other Resolve versions and GPU vendors need community testing.
- Automated coverage includes CPU/OpenCL rendering parity, presets, module isolation, SDR/HDR math, and plugin startup/control behavior. This does not replace clean-machine or calibrated-display testing.
- The binary is **unsigned**, and the plugin remains **experimental**. Test on non-critical material first.

Release numbering now uses **major.minor.patch**, starting with **0.44.0**. The numbering change itself does not alter rendering.

See the [README and before/after gallery](https://github.com/technomancer702/openemulsion#before-and-after), [installation guide](https://github.com/technomancer702/openemulsion/blob/main/release/INSTALL.md), and [detailed development changelog](https://github.com/technomancer702/openemulsion/blob/main/release/RELEASE_NOTES.md) for more information.

Bug reports, preset suggestions, and footage-based feedback are welcome through [GitHub Issues](https://github.com/technomancer702/openemulsion/issues). Include your Resolve version, GPU/driver, color-space settings, and steps to reproduce any problem.
