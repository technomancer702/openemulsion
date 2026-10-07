# SPDX-License-Identifier: MPL-2.0
"""Compare native lens detail at full resolution; exposure sweeps are diagnostic."""

import argparse
import hashlib
import json
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

from color_bench import ROOT, ICC, Renderer, decode, display_preview


def source_pairs(linear):
    """Nearby bright-red source pixels with similar chromaticity, different intensity.

    No output-dependent selection. Four-pixel spacing emphasizes lens structure
    rather than single-pixel noise; this is not a calibrated perceptual metric.
    """
    rgb = linear[...,:3]
    peak = rgb.max(-1)
    normalized = rgb/np.maximum(peak[...,None],1e-7)
    mask = (rgb[...,0]>4)&(rgb[...,0]>2*np.maximum(rgb[...,1],rgb[...,2]))
    change = np.abs(np.log2(np.maximum(peak[:,4:],1e-7)/np.maximum(peak[:,:-4],1e-7)))
    hue_delta = np.max(np.abs(normalized[:,4:]-normalized[:,:-4]),axis=-1)
    return mask[:,4:]&mask[:,:-4]&(change>.04)&(change<.7)&(hue_delta<.025)


def contrast_metrics(rendered, mask):
    y = rendered[...,:3].astype(np.float64)**2.4 @ [.2126,.7152,.0722]
    a, b = y[:,:-4][mask], y[:,4:][mask]
    contrast = np.abs(a-b)/np.maximum((a+b)*.5,1e-9)
    return {"source_keyed_pairs":len(a),
            "relative_luminance_contrast_percentiles":np.percentile(contrast,[10,50,90]).tolist()} if len(a) else {}


def panel_image(panels):
    width = min(panels[0][1].width*2,700)
    height = round(panels[0][1].height*width/panels[0][1].width)
    filtering = Image.Resampling.NEAREST if width>=panels[0][1].width else Image.Resampling.LANCZOS
    canvas = Image.new("RGB",(width*len(panels),height+25),"#202020")
    draw = ImageDraw.Draw(canvas)
    for index,(label,image) in enumerate(panels):
        draw.text((index*width+4,5),label,fill="white")
        canvas.paste(image.resize((width,height),filtering),(index*width,25))
    return canvas


def run(args):
    source, seconds = decode(args.clip,args.seconds,"ITU709","auto")
    height,width = source.shape[:2]
    x0,y0,x1,y1 = args.box
    if not 0<=x0<x1<=1 or not 0<=y0<y1<=1:
        raise ValueError("Expected normalized nonempty crop")
    bounds = [int(x0*width),int(y0*height),int(x1*width),int(y1*height)]
    left,top,right,bottom = bounds
    crop = np.ascontiguousarray(source[top:bottom,left:right])
    del source
    args.output.mkdir(parents=True,exist_ok=True)
    updated = Renderer(args.library)
    reference = Renderer(args.reference_bridge)
    linear = updated.linear(crop,0)
    np.save(args.output/"source-log.npy",crop)
    np.save(args.output/"source-linear.npy",linear)
    mask = source_pairs(linear)
    rows, base_panels, sweep_panels = [],[],[]
    for label,renderer in [(args.reference_label,reference),(args.updated_label,updated)]:
        for stops in [0,-2,-4]:
            settings = renderer.settings(args.preset,0)
            settings[4:7] = 2**stops
            rendered = renderer.render(crop,settings)
            stem = f"{label}-exposure{stops}"
            np.save(args.output/f"{stem}.npy",rendered)
            image = display_preview(rendered[...,:3])
            image.save(args.output/f"{stem}.png",icc_profile=ICC)
            sweep_panels.append((f"{label} {stops:+} EV",image))
            if stops==0:
                base_panels.append((f"{label}: same exposure",image))
            rows.append({"label":label,"stops":stops,"settings":settings.tolist(),
                         "lens_intensity_pairs":contrast_metrics(rendered,mask)})
            del rendered
    panel_image(base_panels).save(args.output/"before-after.png",icc_profile=ICC)
    # Keep exposure rows separate so a viewer does not reduce six panels to unreadable size.
    for index,label in enumerate([args.reference_label,args.updated_label]):
        panel_image(sweep_panels[index*3:(index+1)*3]).save(args.output/f"{label}-exposure-sweep.png",icc_profile=ICC)
    log = crop[...,:3]
    panels = [("Original LogC3 codes (diagnostic)",Image.fromarray(np.rint(np.clip(log,0,1)*255).astype(np.uint8)))]
    for channel,name in enumerate(["R","G","B"]):
        values = log[...,channel]
        low,high = np.percentile(values,[1,99])
        normalized = np.clip((values-low)/max(high-low,1e-6),0,1)
        panels.append((f"Log {name}: normalized diagnostic",Image.fromarray(np.rint(normalized*255).astype(np.uint8)).convert("RGB")))
    panel_image(panels).save(args.output/"source-channels.png",icc_profile=ICC)
    manifest = {"clip":str(args.clip.resolve()),"seconds":seconds,"source_size":[width,height],
                "bounds":bounds,"preset":args.preset,"rows":rows,
                "bridges_sha256":{label:hashlib.sha256(path.read_bytes()).hexdigest() for label,path in
                                  [(args.reference_label,args.reference_bridge),(args.updated_label,args.library)]},
                "sampling":"native source pixels, no decimation; previews enlarged nearest or reduced Lanczos",
                "measurement":"contrast between fixed source-keyed four-pixel pairs; not clipping recovery or perceptual ground truth",
                "source_channel_preview":"diagnostic log codes / independent channel normalization, not a viewing LUT"}
    (args.output/"manifest.json").write_text(json.dumps(manifest,indent=2),encoding="utf-8")
    print(f"Decoded {width}x{height} @ {seconds}s; crop {crop.shape[1]}x{crop.shape[0]} -> {args.output}",flush=True)
    for row in rows:
        if row["stops"]==0:
            print(row["label"],row["lens_intensity_pairs"],flush=True)


if __name__=="__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--clip",type=Path,default=ROOT/"test footage/Nigh Shot car tail lights.mov")
    parser.add_argument("--seconds",type=float,default=2.375)
    parser.add_argument("--box",type=float,nargs=4,default=[.285,.5,.37,.63])
    parser.add_argument("--preset",default="Neutral / Clean Slate")
    parser.add_argument("--library",type=Path,default=ROOT/"build/ofx/bench/ColorBench.dll")
    parser.add_argument("--reference-bridge",type=Path,required=True)
    parser.add_argument("--reference-label",default="v0.38")
    parser.add_argument("--updated-label",default="v0.39")
    parser.add_argument("--output",type=Path,default=ROOT/"analysis/lens-detail-v039")
    run(parser.parse_args())
