#!/usr/bin/env python3
"""Regenerate original VOIDLINE 2.10.31 music and weapon reload sounds.

Optional authoring-only requirements: numpy and soundfile (Ogg/Vorbis).
This script is NOT invoked during builds; the generated .ogg assets are shipped.
All synthesis is algorithmic and entirely original, with deterministic seeds.
"""
from pathlib import Path
import numpy as np
import soundfile as sf

OUT = Path(__file__).resolve().parents[1] / 'assets' / 'audio'
HZ = 24000


def fade(sound, tail=.016):
    n=min(len(sound)//3,int(HZ*tail))
    if n:
        ramp=np.sin(np.linspace(0,np.pi/2,n))**2
        sound[:n]*=ramp
        sound[-n:]*=ramp[::-1]
    return sound


def save(name, mono, width=.20):
    mono=np.tanh(mono*1.25)*.84
    # Short stereo comb reflection; retains compact Ogg size.
    left=mono.copy()
    delay=int(HZ*.011)
    right=mono.copy()
    if delay<len(mono):
        right[delay:]+=mono[:-delay]*width
    stereo=np.column_stack([left,right]).astype('float32')
    sf.write(OUT/(name+'.ogg'),stereo,HZ,format='OGG',subtype='VORBIS')


def mechanical(duration, impacts, slides=(), hum=(), seed=1):
    rng=np.random.default_rng(seed)
    n=int(HZ*duration)
    out=np.zeros(n,dtype=np.float64)
    for at,pitch,amplitude,decay in impacts:
        start=int(at*HZ)
        if start>=n:continue
        size=min(n-start,int(HZ*(decay*5+.09)))
        t=np.arange(size)/HZ
        # Hard switch click + spring/metal ring, not a pure electronic beep.
        grain=rng.standard_normal(size)
        transient=grain*np.exp(-t/(decay*.24+.002))
        ringing=np.sin(2*np.pi*(pitch*t+130*t*t))*np.exp(-t/decay)
        metallic=np.sin(2*np.pi*pitch*2.71*t)*np.exp(-t/(decay*.48))
        out[start:start+size]+=amplitude*(transient*.23+ringing*.49+metallic*.18)
    for at,length,amplitude,texture in slides:
        start=int(at*HZ)
        size=min(n-start,int(length*HZ))
        if size<=0:continue
        t=np.arange(size)/HZ
        env=np.sin(np.pi*np.minimum(1,t/length))**.7
        # Cartridge scraping, spring compression or magazine travel.
        scrape=rng.standard_normal(size)
        scrape=np.diff(np.r_[0,scrape])*.5
        teeth=np.sin(2*np.pi*(140+110*t)*t)*.3
        out[start:start+size]+=amplitude*(scrape*.11+teeth*texture)*env
    for at,length,start_pitch,end_pitch,amplitude in hum:
        begin=int(at*HZ);size=min(n-begin,int(length*HZ))
        if size<=0:continue
        t=np.arange(size)/HZ
        pitch=start_pitch+(end_pitch-start_pitch)*t/max(.001,length)
        phase=2*np.pi*np.cumsum(pitch)/HZ
        env=np.sin(np.pi*np.minimum(1,t/length))**1.2
        out[begin:begin+size]+=amplitude*(np.sin(phase)+.36*np.sin(phase*2.01))*env
    return fade(out,.009)


# Aligned to the weapon reload times, with different physical mechanisms.
RELOADS = {
    'pistol': (.78,[(.045,620,.55,.020),(.20,390,.38,.035),(.56,850,.70,.022)],[(.26,.22,.5,.5)],[]),
    'carbine':(.95,[(.035,410,.65,.044),(.25,480,.42,.026),(.73,710,.68,.035)],[(.31,.28,.52,.8)],[]),
    'shotgun':(1.27,[(.09,290,.62,.06),(.48,540,.47,.05),(.77,520,.47,.05),(1.11,350,.77,.075)],[(.18,.19,.3,.65),(.91,.15,.40,.7)],[]),
    'arc':    (1.15,[(.11,680,.28,.027),(.65,740,.45,.035),(.98,920,.40,.04)],[(.12,.24,.30,.2)],[(.36,.55,180,660,.29)]),
    'smg':    (1.05,[(.08,800,.38,.018),(.29,510,.68,.022),(.79,920,.70,.015)],[(.33,.32,.38,.88)],[]),
    'launcher':(1.65,[(.10,260,.83,.115),(.56,330,.64,.085),(1.17,420,.49,.045),(1.48,240,.98,.09)],[(.22,.46,.48,.70)],[]),
    'railgun':(1.75,[(.09,320,.78,.10),(.61,570,.60,.053),(1.46,290,.78,.085)],[(.27,.44,.35,.6)],[(.80,.60,125,850,.36)])
}
for idx,(name,(dur,impacts,slides,hum)) in enumerate(RELOADS.items()):
    save('reload-'+name,mechanical(dur,impacts,slides,hum,200+idx),.16)


# Original dark-electronic / industrial instrumental riffs. No recorded music,
# samples, public-domain melodies, or external synthesizer libraries are used.
def note(freq, t, kind='bass'):
    if kind=='lead':
        wave=(np.sin(2*np.pi*freq*t)+.40*np.sin(2*np.pi*freq*2*t+.2)
              +.28*np.sin(2*np.pi*freq*3*t))*np.exp(-t*1.4)
        return np.tanh(wave*1.8)
    if kind=='pad':
        return (.53*np.sin(2*np.pi*freq*t)+.22*np.sin(2*np.pi*freq*.998*t)
               +.22*np.sin(2*np.pi*freq*1.997*t))*np.exp(-t*.09)
    wave=np.sin(2*np.pi*freq*t)+.46*np.sin(2*np.pi*freq*2*t)+.22*np.sin(2*np.pi*freq*3*t)
    return np.tanh(1.8*wave)*np.exp(-t*3.2)


def put(buffer,at,frequency,seconds,amplitude,kind):
    start=int(at*HZ)
    if start>=len(buffer):return
    n=min(int(seconds*HZ),len(buffer)-start)
    t=np.arange(n)/HZ
    sound=note(frequency,t,kind)
    end=min(n,int(.02*HZ))
    if end: sound[-end:]*=np.linspace(1,0,end)
    buffer[start:start+n]+=sound*amplitude


def industrial(seed,style,biome):
    rng=np.random.default_rng(seed)
    tempo=[136,146,158,126,152][biome]+(style=='pursuit')*8
    beat=60/tempo
    bars=16
    duration=bars*4*beat
    n=int(duration*HZ)
    out=np.zeros(n,np.float64)
    roots=[46.25,55.,51.91,43.65,61.735]  # F#1, A1, G#1, F1, B1
    root=roots[biome]
    riff=[0,0,7,0,3,0,5,7, 0,-2,3,5,7,3,0,-2] if style=='assault' else [0,0,3,0,7,3,5,3,0,-2,0,7,5,3,0,-5]
    steps=bars*16
    for k in range(steps):
        t=k*beat/4
        scale=2**(riff[(k//2+biome)%len(riff)]/12)
        if k%2==0 or (k%4==3 and style=='pursuit'):
            put(out,t,root*2*scale,.21*beat,.23 if style=='assault' else .22,'bass')
        if k%8 in (3,6) and k//16%4!=3:
            put(out,t,root*8*scale,.40*beat,.095,'lead')
        # 16th metallic hats give a restless, aggressive rhythm.
        start=int(t*HZ);size=min(int(.065*HZ),n-start)
        if size>0 and (k%2==0 or k%8==7):
            noise=rng.standard_normal(size)
            noise-=np.convolve(noise,np.ones(13)/13,'same')
            env=np.exp(-np.arange(size)/HZ*(75 if k%4 else 42))
            out[start:start+size]+=noise*env*(.045 if k%4 else .070)
    for bar in range(bars):
        for beat_index in range(4):
            start=int((bar*4+beat_index)*beat*HZ)
            size=min(int(.18*HZ),n-start)
            if size<=0:continue
            t=np.arange(size)/HZ
            if beat_index in (0,2) or (bar%4==3 and beat_index==3):
                kick=np.sin(2*np.pi*(70-38*t)*t)*np.exp(-t*37)
                out[start:start+size]+=.42*kick
            if beat_index in (1,3):
                snare=rng.standard_normal(size)*np.exp(-t*27)
                out[start:start+size]+=.145*snare
        if bar%4 in (0,2):
            for interval in (0,3,7):
                put(out,bar*4*beat,root*4*2**(interval/12),2.5*beat,.043,'pad')
    # A gentle master decay at loop boundaries avoids an audible pop.
    return fade(out,.075)


def menu(seed,style):
    rng=np.random.default_rng(seed)
    tempo=76 if style=='atmosphere' else 94
    beat=60/tempo
    duration=8*4*beat
    n=int(HZ*duration);out=np.zeros(n)
    chords=[[0,3,7],[-2,3,5],[0,5,8],[-5,0,3]]
    for bar in range(8):
        for interval in chords[bar%4]:
            put(out,bar*4*beat,82.4*2**(interval/12),3.90*beat,.067,'pad')
        for k in range(8):
            if rng.random()>.6: continue
            at=bar*4*beat+k*.5*beat
            put(out,at,164.8*2**(chords[bar%4][k%3]/12),.6*beat,.023,'lead')
        if style=='hangar':
            at=bar*4*beat
            size=min(int(.17*HZ),n-int(at*HZ))
            t=np.arange(size)/HZ
            out[int(at*HZ):int(at*HZ)+size]+=.11*np.sin(2*np.pi*55*t)*np.exp(-t*19)
    return fade(out,.13)

for biome in range(5):
    for variant in ('assault','pursuit'):
        save(f'sector-{biome+1}-{variant}',industrial(410+biome*11+(variant=='pursuit'),variant,biome),.12)
for menu_style in ('atmosphere','hangar'):
    save('menu-'+menu_style,menu(700+(menu_style=='hangar'),menu_style),.25)
print('Generated 7 weapon-specific mechanical reloads and 12 original music tracks.')
