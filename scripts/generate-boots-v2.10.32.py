#!/usr/bin/env python3
"""Original boot-footstep recordings for VOIDLINE.

Optional *authoring* tool (numpy + soundfile); the game build only needs the
shipped Ogg files. A footstep is a layered heel impact, tread flex and toe
release, with rock/grate/deck-specific contact. Seeds are deterministic.
"""
from pathlib import Path
import numpy as np
import soundfile as sf
from scipy.signal import butter, sosfilt

HZ=24000
OUT=Path(__file__).resolve().parents[1]/'assets'/'audio'

def filt(x,lo=None,hi=None):
    if lo is None: return sosfilt(butter(2,hi,fs=HZ,output='sos'),x)
    if hi is None: return sosfilt(butter(2,lo,fs=HZ,btype='high',output='sos'),x)
    return sosfilt(butter(2,[lo,hi],fs=HZ,btype='bandpass',output='sos'),x)

def place(out,signal,start,gain=1):
    off=int(start*HZ)
    if off>=len(out):return
    n=min(len(out)-off,len(signal))
    if n>0:out[off:off+n]+=gain*signal[:n]

def hit(rng,seconds,base,brightness):
    n=int(seconds*HZ);t=np.arange(n)/HZ
    # Heel lands as a low body thud plus a brief textured tread strike.
    low=np.sin(2*np.pi*(base*t-25*t*t))*np.exp(-t*46)
    ring=np.sin(2*np.pi*(base*2.15*t+40*t*t))*np.exp(-t*27)
    noise=rng.standard_normal(n)
    grain=filt(noise,155,brightness)*np.exp(-t*49)
    return .59*low+.12*ring+.40*grain

def boot(kind,take):
    rng=np.random.default_rng(3200+take+{'deck':0,'grate':20,'rock':40}[kind])
    duration=.32+take*.006
    n=int(HZ*duration)
    out=np.zeros(n,np.float64)
    base={'deck':72,'grate':92,'rock':65}[kind]*(1+rng.uniform(-.07,.07))
    place(out,hit(rng,.24,base,1900),.012+.003*take,.85)
    # Boot upper creaks slightly as weight compresses; no heavy reverb.
    t=np.arange(int(.14*HZ))/HZ
    leather=filt(rng.standard_normal(len(t)),290,900)*np.exp(-t*31)
    place(out,leather,.047,.078)
    # Toe contact and heel lift occur after weight transfers.
    place(out,hit(rng,.13,base*1.38,2650),.075+.003*(take%2),.34)
    if kind=='grate':
        # Hollow mesh bounce: short damped metallic teeth under the boot.
        t=np.arange(int(.14*HZ))/HZ
        chime=(np.sin(2*np.pi*510*t)+.43*np.sin(2*np.pi*1050*t))*np.exp(-t*31)
        place(out,chime,.022,.21)
        mesh=filt(rng.standard_normal(len(t)),1600,5700)*np.exp(-t*52)
        place(out,mesh,.033,.070)
    elif kind=='rock':
        # Small gritty stones move after the tread hits, non-periodically.
        for j in range(7):
            at=.040+j*.016+rng.uniform(-.007,.007)
            count=int(.025*HZ)
            t=np.arange(count)/HZ
            grit=filt(rng.standard_normal(count),650,4300)*np.exp(-t*rng.uniform(110,230))
            place(out,grit,at,rng.uniform(.032,.100))
    else:
        # Hard deck: dry shell knock and a little rubber friction.
        t=np.arange(int(.135*HZ))/HZ
        scrape=filt(rng.standard_normal(len(t)),450,2050)*np.exp(-t*38)
        place(out,scrape,.095,.085)
    # Smooth the seam, retain a strong but undistorted transient.
    fade=int(.018*HZ)
    out[:min(fade,n)]*=np.linspace(0,1,min(fade,n))
    out[-fade:]*=np.linspace(1,0,fade)
    out=np.tanh(out*1.18)*.72
    # A very quiet second contact gives realistic left/right boot separation.
    other=np.zeros_like(out)
    delay=int(.0045*HZ)
    other[delay:]=out[:-delay]
    stereo=np.column_stack([out,other*.92]).astype('float32')
    name='boot-'+kind+('' if take==1 else '-'+str(take))
    sf.write(OUT/(name+'.ogg'),stereo,HZ,format='OGG',subtype='VORBIS')

if __name__=='__main__':
    for kind in ('deck','grate','rock'):
        for take in range(1,5):boot(kind,take)
    print('Generated 12 original boot footsteps (4 variations per surface).')
