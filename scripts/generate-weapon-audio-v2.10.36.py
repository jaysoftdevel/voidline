#!/usr/bin/env python3
"""Render VOIDLINE weapon audio from the user's 8 supplied MP3 recordings.

Authoring-only command:
  python3 scripts/generate-weapon-audio-v2.10.36.py --sample-dir /path/to/mp3-files

The normal game build uses the generated assets/audio/*.ogg files directly;
no MP3s, Python packages, or filesystem cache are required at runtime.
"""
import argparse
from pathlib import Path
import numpy as np
from pydub import AudioSegment
from scipy.signal import butter, sosfilt
import soundfile as sf

RATE = 48000
SAMPLE_NAMES = ('pistol', 'carbine', 'smg', 'laser', 'launcher+impact',
                'relaoding', 'reloading_2', 'reload_pistol')
RELOAD_LEN = {'pistol': .78, 'carbine': .95, 'smg': 1.05, 'shotgun': 1.27,
              'arc': 1.15, 'launcher': 1.65, 'railgun': 1.75}


def source(path):
    a = AudioSegment.from_mp3(path).set_frame_rate(RATE).set_channels(1).set_sample_width(2)
    return np.asarray(a.get_array_of_samples(), dtype=np.float32) / 32768.0


def clip(x, a, b):
    i, j = max(0, round(a*RATE)), round(b*RATE)
    return x[i:min(len(x),j)].copy()


def extend(x, n):
    if len(x) >= n: return x[:n].copy()
    return np.pad(x, (0,n-len(x)))


def shift(x, factor):
    # A slight playback-rate change produces alternate takes without adding
    # an artificial repeating synth to the user-provided gunshot recordings.
    i = np.arange(max(1,int(len(x)/factor)),dtype=np.float32)*factor
    return np.interp(i, np.arange(len(x)), x).astype(np.float32)


def low(x, cutoff):
    return sosfilt(butter(3,cutoff,fs=RATE,output='sos'),x).astype(np.float32)


def high(x, cutoff):
    return sosfilt(butter(2,cutoff,fs=RATE,output='sos',btype='high'),x).astype(np.float32)


def soften_end(x, tail=.055):
    if len(x)==0:return x
    n=min(len(x),max(1,round(RATE*tail)))
    x=x.copy()
    x[-n:]*=np.linspace(1,0,n,dtype=np.float32)
    # Some source samples begin with a little encoded silence. Preserve it,
    # while eliminating sharp crop boundaries.
    x[:min(len(x),120)]*=np.linspace(0,1,min(len(x),120),dtype=np.float32)
    return x


def insert(out, sample, at=0, gain=1):
    n=round(at*RATE)
    if n>=len(out):return
    size=min(len(sample),len(out)-n)
    out[n:n+size]+=gain*sample[:size]


def reverb(x, delays, length=None):
    # A few measured-delay reflections add distance, not a synthetic boing.
    n=max(len(x),round((length or len(x)/RATE)*RATE))
    out=extend(x,n)
    body=low(x,4100)
    for t,gain in delays:
        insert(out,body,t,gain)
    return out


def render_shot(name, i, samples):
    pistol, carbine, smg, laser, launcher = (samples[k] for k in
        ('pistol','carbine','smg','laser','launcher+impact'))
    rates=[.985,1.018,.965,1.035]
    rate=rates[i]
    if name=='pistol':
        x=clip(pistol, [0.,.408,.814,.408][i], [.38,.79,1.195,.795][i])
        x=shift(x,rate)
        x=extend(x,round(.45*RATE))
    elif name=='carbine':
        t=[.000,.187,.464,.742][i]
        x=shift(clip(carbine,t,t+.092),rate)
        x=reverb(x,[(.045,.16),(.081,.08)],.23)
    elif name=='smg':
        t=[.000,.094,.189,.326][i]
        x=shift(clip(smg,t,t+.092),rate)
        x=reverb(x,[(.041,.11)],.18)
    elif name=='shotgun':
        # No shotgun reference supplied: combine sharp recorded handgun
        # blast with low-pressure from the launcher, plus a short receiver click.
        crack=clip(pistol, [.00,.408,.814,.00][i], [.35,.76,1.165,.35][i]);
        boom=low(clip(launcher,.215,.9),480)
        x=np.zeros(round(.69*RATE),np.float32)
        insert(x,shift(crack,.83+.025*i),0,.72)
        insert(x,shift(boom,.96+.02*i),.006,.88)
        insert(x,high(clip(carbine,.000,.089),1200),.023,.16)
        x=reverb(x,[(.093,.16),(.171,.095)],.77)
    elif name=='arc':
        x=np.zeros(round(.9*RATE),np.float32)
        insert(x,shift(clip(laser,.065,.845),rate),0,1.)
        insert(x,high(clip(carbine,.002,.078),1600),.005,.14)
    elif name=='launcher':
        x=shift(clip(launcher,.204,1.30),rate)
        x=reverb(x,[(.092,.08),(.18,.045)],1.18)
    elif name=='railgun':
        x=np.zeros(round(.95*RATE),np.float32)
        insert(x,shift(clip(laser,.065,.82),rate*.89),0,.86)
        insert(x,high(clip(carbine,.002,.095),900),.011,.45)
        insert(x,low(clip(launcher,.218,.88),880),.005,.40)
    elif name=='enemy':
        x=shift(clip(carbine,[.000,.187,.464,.742][i],[.089,.276,.553,.831][i]),rate)
        x=reverb(x,[(.045,.13),(.084,.09)],.24)
    elif name=='guardian':
        x=np.zeros(round(.82*RATE),np.float32)
        insert(x,shift(clip(launcher,.218,.95),rate),0,.86)
        insert(x,high(clip(carbine,.000,.086),1800),.012,.24)
        insert(x,clip(laser,.069,.4),.034,.15)
    else:raise ValueError(name)
    return soften_end(x, .04 if name in ('carbine','smg','enemy') else .075)


def make_reload(name,samples):
    pistol=samples['reload_pistol']
    r1=samples['relaoding']
    r2=samples['reloading_2']
    length=RELOAD_LEN[name]
    out=np.zeros(round((length+.12)*RATE),np.float32)
    if name=='pistol':
        # Use the supplied pistol mechanics as the primary recording.
        insert(out,shift(clip(pistol,0,.966), .966/max(.72,length)),0,1.)
    elif name=='carbine':
        insert(out,shift(clip(r1,0,1.50),1.50/length),0,1.)
    elif name=='smg':
        insert(out,shift(clip(r2,0,1.50),1.50/length),0,1.)
    elif name=='shotgun':
        # Individual shell clicks derived from the user's hard metal action.
        insert(out,clip(r2,.23,.40),.06,.64)
        for t in (.32,.57,.82):insert(out,clip(r1,.83,1.04),t,.52)
        insert(out,clip(r2,.97,1.35),.95,.82)
    elif name=='arc':
        insert(out,shift(clip(r2,0,1.45),1.32),0,.80)
        insert(out,clip(samples['laser'],.10,.34),.81,.15)
    elif name=='launcher':
        insert(out,clip(r1,0,1.48),0,.95)
        insert(out,low(clip(r2,.965,1.30),1100),1.22,.46)
    elif name=='railgun':
        insert(out,clip(r2,0,1.5),0,.95)
        insert(out,clip(r1,.82,1.14),1.33,.42)
        insert(out,clip(samples['laser'],.10,.35),1.56,.12)
    return soften_end(out,.075)


def store(path, x, peak):
    x=np.nan_to_num(x.astype(np.float32),copy=False)
    # Preserve transient structure with one gain across the clip, no
    # aggressive compression or pitchy synthesized fundamental.
    mag=max(1.e-5,float(np.max(np.abs(x))))
    x=x*(peak/mag)
    path.parent.mkdir(parents=True,exist_ok=True)
    sf.write(path,x,RATE,format='OGG',subtype='VORBIS')


def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('--sample-dir',type=Path,required=True)
    ap.add_argument('--output-dir',type=Path,default=Path(__file__).resolve().parents[1]/'assets/audio')
    args=ap.parse_args()
    for name in SAMPLE_NAMES:
        if not (args.sample_dir/(name+'.mp3')).is_file():
            ap.error('missing supplied recording '+str(args.sample_dir/(name+'.mp3')))
    s={name:source(args.sample_dir/(name+'.mp3')) for name in SAMPLE_NAMES}
    guns=('pistol','carbine','smg','shotgun','arc','launcher','railgun','enemy','guardian')
    for gun in guns:
        for i in range(4):
            name=gun+('' if i==0 else '-'+str(i+1))+'.ogg'
            store(args.output_dir/name,render_shot(gun,i,s),.72)
    for gun in RELOAD_LEN:
        store(args.output_dir/('reload-'+gun+'.ogg'),make_reload(gun,s),.70)
    # Distinguish the launcher's actual explosive impact recording from its
    # muzzle event; in-game projectile collisions trigger this event.
    impact=clip(s['launcher+impact'],1.65,3.65)
    store(args.output_dir/'launcher-impact.ogg',soften_end(impact,.30),.72)
    print(f'Rendered {4*len(guns)+len(RELOAD_LEN)+1} Ogg effects from {len(s)} supplied recordings')

if __name__=='__main__':main()
