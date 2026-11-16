#!/usr/bin/env python3
"""Original boot push-off and cloth jump cues built from game-owned field recordings.
Can regenerate with: python3 scripts/generate-jump-audio-v2.10.38.py
Requires numpy, scipy, soundfile; generated Oggs are prepackaged for builds.
"""
from pathlib import Path
import numpy as np
from scipy.signal import butter, sosfilt
import soundfile as sf

ROOT=Path(__file__).resolve().parent.parent
AUDIO=ROOT/'assets/audio'
HZ=48000

def read(name):
    data,rate=sf.read(AUDIO/name,always_2d=True,dtype='float32')
    mono=data.mean(axis=1)
    if rate != HZ:
        from scipy.signal import resample_poly
        from math import gcd
        c=gcd(rate,HZ)
        mono=resample_poly(mono,HZ//c,rate//c)
    return mono

def filter_signal(y,kind,cut):
    return sosfilt(butter(2,cut,kind,fs=HZ,output='sos'),y)

def add_piece(target,source,start,gain,speed=1.0,fade=0.015):
    at=round(start*HZ)
    length=min(len(target)-at,round(len(source)/speed))
    if length<=0:return
    part=np.interp(np.arange(length)*speed,np.arange(len(source)),source,left=0,right=0)
    env=np.ones(length)
    ramp=min(length//3,round(HZ*fade))
    if ramp:
        env[:ramp]*=np.linspace(0,1,ramp)
        env[-ramp:]*=np.linspace(1,0,ramp)
    target[at:at+length]+=gain*part*env

def envelope(n,attack,decay):
    t=np.arange(n)/HZ
    return np.minimum(1,t/max(0.001,attack))*np.exp(-t/max(0.001,decay))

boot=read('boot-deck.ogg')
cloth=read('cloth.ogg')
# Existing short recordings, not firearm samples: no tonal arcade "boing".
# A textured boot sole push, low rubber impact and a short clothing brush.
for variant in range(3):
    rng=np.random.default_rng(832+variant)
    duration=.31+variant*.012
    n=round(duration*HZ)
    sound=np.zeros(n)
    # Rasp of the sole moving against the deck, different each takeoff.
    patch=boot[round((.018+variant*.008)*HZ):round((.20+variant*.008)*HZ)]
    patch=filter_signal(patch,'lowpass',1800+variant*130)
    add_piece(sound,patch,.019,0.74,speed=1.08-variant*.036,fade=.023)
    # Low, brief weight transfer with no unrealistic pitch sweep.
    noise=filter_signal(rng.standard_normal(n),'lowpass',125)
    noise/=max(1e-6,np.max(np.abs(noise)))
    t=np.arange(n)/HZ
    low_env=np.exp(-((t-(.05+variant*.003))/.039)**2)
    sound+=noise*low_env*.18
    # Fabric/kit air friction as the body leaves the ground.
    brushed=filter_signal(cloth,'highpass',340)
    add_piece(sound,brushed,.060,.39,speed=.95+variant*.025,fade=.040)
    # Subtle wind/wear noise, not a digital ascending swoosh.
    air=filter_signal(rng.standard_normal(n),'bandpass',[490,2600])
    air/=max(1e-6,np.max(np.abs(air)))
    sound+=air*envelope(n,.045,.11)*.047
    sound-=np.mean(sound)
    sound=np.tanh(sound*1.65)
    sound*=.74/max(.74,np.max(np.abs(sound)))
    stereo=np.column_stack((sound,np.roll(sound,7)*.94)).astype('float32')
    sf.write(AUDIO/('jump.ogg' if variant==0 else f'jump-{variant+1}.ogg'),stereo,HZ,format='OGG',subtype='VORBIS')
# Midair flip is suit fabric, with no "foot on ground" impulse.
for variant in range(2):
    rng=np.random.default_rng(291+variant)
    n=round(.24*HZ)
    cloth_part=filter_signal(cloth,'bandpass',[410,3800])
    sound=np.zeros(n)
    add_piece(sound,cloth_part,.012,.70,speed=1.18+variant*.07,fade=.047)
    air=filter_signal(rng.standard_normal(n),'bandpass',[570,4100])
    air/=max(1e-6,np.max(np.abs(air)))
    sound+=air*envelope(n,.025,.075)*.045
    sound=np.tanh(sound*1.48)
    sf.write(AUDIO/('jump-air.ogg' if variant==0 else 'jump-air-2.ogg'),
             np.column_stack((sound,np.roll(sound,5)*.92)).astype('float32'),HZ,format='OGG',subtype='VORBIS')
print('Generated three grounded jump and two air movement Oggs')
