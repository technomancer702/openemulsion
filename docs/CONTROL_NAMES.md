# Control Names (v0.26)

Use familiar Filmbox terminology where a comparable control exists, while documenting OpenEmulsion's independent creative math and limits. Matching a label does not claim identical processing or measured film calibration.

## Label Changes

| Previous label | Current label | Parameter identifier (unchanged) |
| --- | --- | --- |
| Negative Density | Color Density | `density` |
| Color Richness | Richness | `colorRichness` |
| Neutralize Print | Neutralize Balance | `printNeutralize` |
| Neutral Width | Dead Zone Width | `splitWidth` |
| Print Tone | Print Tone Curve | `printTone` |

These are label/tooltip changes only. Numeric ranges, defaults, stored values, animation identifiers, preset recipes, UI enable policy, and CPU/OpenCL processing remain unchanged from v0.25.

## Comparable Controls and Limits

- Skin Hue stays Skin Hue. Filmbox describes its control as a magenta/green adjustment, matching the direction of ours. OpenEmulsion uses a smooth warm-color mask, not face detection or calibrated stock data. [Filmbox Color and Tone](https://videovillage.com/learn/filmbox/full-guide/negative/color-and-tone).
- Print Color stays Print Color. Filmbox's Cinema Color control also runs from more print-like at zero to more neutral/telecine-like at one. This is not the same as Print Color Strength, which blends the complete chromatic response, including saturation and compression. Print Tone Curve retains a Print prefix to distinguish it from Film Tone Strength and Print Tone Strength. Neutralize Balance removes only the print's tone-dependent cast. [Filmbox Print](https://videovillage.com/learn/filmbox/full-guide/print-module).
- Richness favors muted colors rather than applying uniform saturation. Dead Zone Width defines the neutral interval around the split-tone pivot. Both names match comparable Lab terminology. [Filmbox Lab](https://videovillage.com/learn/filmbox/full-guide/lab-module).
- Color Density follows Filmbox's earlier Color Density terminology. Filmbox 3.6 replaced that control with Hi-Sat Density, specifically more focused on extremely saturated colors. Our broader chroma-dependent brightness adjustment should not be relabeled Hi-Sat Density without changing its behavior. [Filmbox release notes](https://videovillage.com/filmbox/releasenotes).
- Print Red/Green/Blue remain stop-based linear-light balance controls. They are not renamed Printer Lights because they are not calibrated printer points. Push/Pull remains a creative tone/grain-strength adjustment rather than a claim to reproduce Filmbox's stock-dependent processing.

## Verification

The build and regression suites check icon packaging, module/preset behavior, historical response anchors, and CPU/OpenCL rendering. Source review confirms only descriptor labels/hints and the minor version changed in production code. Final label layout and host project reload still require Resolve checks.
