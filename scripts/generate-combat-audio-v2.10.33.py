#!/usr/bin/env python3
"""Render original VOIDLINE gun, impact, footwear, and weapon mechanism SFX.

Authoring-only: numpy, scipy, soundfile. The game ships Ogg/Vorbis data and
never needs these Python packages at runtime or during a normal build.

All signals are original algorithmic Foley/sound design. No sampled firearms,
commercial sound libraries, or externally owned recordings are used.
"""
from pathlib import Path
import numpy as np
import soundfile as sf
from scipy.signal import butter, sosfilt

HZ = 32000
ROOT = Path(__file__).resolve().parents[1] / 'assets' / 'audio'
ROOT.mkdir(parents=True, exist_ok=True)


def noise(rng, n, low=None, high=None):
    x = rng.standard_normal(n)
    if low and high:
        return sosfilt(butter(2, [low, high], btype='bandpass', fs=HZ, output='sos'), x)
    if high:
        return sosfilt(butter(2, high, fs=HZ, output='sos'), x)
    if low:
        return sosfilt(butter(2, low, btype='highpass', fs=HZ, output='sos'), x)
    return x


def insert(out, signal, seconds, amount=1):
    i = max(0, int(seconds * HZ))
    n = min(len(signal), len(out) - i)
    if n > 0:
        out[i:i+n] += amount * signal[:n]


def click(rng, duration=.072, base=760, weight=1):
    n = round(duration * HZ)
    t = np.arange(n) / HZ
    crack = noise(rng, n, 480, 11000) * np.exp(-t * 135)
    metallic = (np.sin(2*np.pi*base*t) + .49*np.sin(2*np.pi*base*2.38*t)) * np.exp(-t*53)
    thunk = np.sin(2*np.pi*(base*.22*t - 100*t*t)) * np.exp(-t*54)
    return weight * (.45*crack + .39*metallic + .19*thunk)


def subpulse(rng, duration, base=74, decay=27):
    n = round(duration * HZ)
    t = np.arange(n) / HZ
    phase = 2*np.pi*(base*t - min(base*.28,33)*t*t)
    low = np.sin(phase) * np.exp(-t*decay)
    body = noise(rng, n, 48, 480) * np.exp(-t*(decay*.92))
    return .86*low + .20*body


def echoes(mono, offsets=((.023,.15),(.056,.10),(.115,.065)), tone=8500):
    """Small industrial early reflections, not a long boomy synthetic reverb."""
    result = mono.copy()
    low = sosfilt(butter(2, tone, fs=HZ, output='sos'), mono)
    for time, gain in offsets:
        delay = int(time * HZ)
        if delay < len(result):
            result[delay:] += gain*low[:-delay]
    return result


def export(name, mono, seed, stereo_width=.09):
    # Preserve impact dynamics while avoiding digital clipping in the 32-voice mixer.
    mono = np.tanh(mono*1.24) * .77
    ramp = min(len(mono)//8, int(.004*HZ))
    if ramp:
        mono[:ramp] *= np.linspace(0,1,ramp)
        mono[-ramp:] *= np.linspace(1,0,ramp)
    rng = np.random.default_rng(seed+38000)
    # Very subtle independent room reflection on right channel; transient stays centered.
    other = mono.copy()
    delay=int(.014*HZ)
    other[delay:] += mono[:-delay] * stereo_width
    other *= .96
    sf.write(ROOT/(name+'.ogg'), np.column_stack([mono,other]).astype('float32'),
             HZ, format='OGG', subtype='VORBIS')


def gunshot(kind, take):
    settings={
        # duration, low pressure, crack, mechanical, tail, low fundamental
        'pistol':  (.43,.49,.90,.35,.20, 110),
        'carbine': (.57,.64,1.10,.29,.29, 88),
        'smg':     (.35,.39,.75,.57,.13, 126),
        'shotgun': (.76,1.03,1.15,.44,.39, 66),
        'arc':     (.60,.62,.74,.41,.25, 106),
        'launcher':(.96,1.26,.72,.72,.51, 52),
        'railgun': (.90,1.30,1.19,.65,.57, 62),
        'enemy':   (.43,.52,.85,.39,.19, 110),
        'guardian':(.84,1.16,1.07,.65,.52, 60),
    }
    dur, bass, snap, mechanism, tail, fundamental = settings[kind]
    rng = np.random.default_rng(14000+take+list(settings).index(kind)*100)
    out = np.zeros(round(HZ*dur))
    # Fast asymmetric muzzle pressure wave with gritty broadband crack.
    n = int(HZ*.21)
    t = np.arange(n)/HZ
    burst = noise(rng,n,130,13600)*np.exp(-t*185)
    body = noise(rng,n,160,2900)*np.exp(-t*52)
    insert(out, snap*(.78*burst+.44*body), .005+take*.0004)
    insert(out, bass*subpulse(rng,.37,fundamental,22 if bass>1 else 31),.006)
    # Receiver/breech noise is separate from the acoustic blast.
    insert(out, click(rng, base=740+take*42), .033, mechanism)
    if kind in ('smg','carbine'):
        insert(out, click(rng,.052,base=1180),.082,.20)
    elif kind=='shotgun':
        insert(out, click(rng,.095,base=430),.105,.29)
    elif kind=='launcher':
        insert(out, click(rng,.092,base=350),.03,.26)
        insert(out, subpulse(rng,.53,53,13),.065,.53)
    elif kind in ('arc','railgun','guardian'):
        n=min(int(.24*HZ),len(out)); t=np.arange(n)/HZ
        sweep=np.sin(2*np.pi*(370*t+1550*t*t)) * np.exp(-t*19)
        insert(out,sweep, .010, .13 if kind=='arc' else .21)
    # Outdoor/industrial slapback. Slow, quiet enough not to muddy rapid fire.
    out=echoes(out,((.027,.21*tail),(.063,.25*tail),(.112,.16*tail),(.183,.07*tail)),5900)
    export(kind if take==1 else f'{kind}-{take}',out,1500+take)


def impact(kind,take):
    rng=np.random.default_rng(20000+take+{'metal':0,'concrete':100,'rock':200}[kind])
    duration=.35 if kind=='metal' else .41
    out=np.zeros(round(duration*HZ))
    n=int(.23*HZ);t=np.arange(n)/HZ
    # Universal sharp penetration/slap; the following layers define material.
    crack=noise(rng,n,500,11900)*np.exp(-t*220)
    insert(out,crack,.002,.58)
    if kind=='metal':
        # Thin sharp edge + broad steel plate thump + irregular clang harmonics.
        ping=np.zeros(n)
        for pitch,level,rate in [(523,.20,37),(1087,.31,43),(1741,.19,64),(2773,.14,83)]:
            ping+=np.sin(2*np.pi*(pitch*t+pitch*11*t*t))*np.exp(-t*rate)*level
        insert(out,ping,.004,1)
        insert(out,subpulse(rng,.22,155,45),.002,.24)
        rattle=noise(rng,n,1000,8000)*np.exp(-t*42)
        insert(out,rattle,.031,.13)
    else:
        # Concrete/rock fracture has broad granular debris, not a ringing tone.
        fracture=noise(rng,n,280,6800)*np.exp(-t*(65 if kind=='concrete' else 48))
        insert(out,fracture,.007,.34 if kind=='concrete' else .48)
        insert(out,subpulse(rng,.19,105 if kind=='concrete' else 87,38),.002,.25)
        for j in range(9 if kind=='concrete' else 12):
            start=.032+j*.011+rng.uniform(-.006,.007)
            m=int(.020*HZ); times=np.arange(m)/HZ
            grit=noise(rng,m,600,9000)*np.exp(-times*rng.uniform(90,240))
            insert(out,grit,start,rng.uniform(.042,.11))
    out=echoes(out,((.016,.07),(.041,.06)),6400)
    export(f'hit-{kind}'+('' if take==1 else f'-{take}'),out,2000+take)


def boot(kind,take):
    rng=np.random.default_rng(28000+take+{'deck':0,'grate':100,'rock':200}[kind])
    out=np.zeros(round((.36+take*.005)*HZ))
    fundamental={'deck':72,'grate':80,'rock':57}[kind]*(1+rng.uniform(-.06,.06))
    # Heel hits with low-frequency boot mass (distinct from high-frequency tap).
    insert(out,subpulse(rng,.30,fundamental,22),.010,1.12)
    n=int(.15*HZ);t=np.arange(n)/HZ
    sole=noise(rng,n,105,2250)*np.exp(-t*48)
    insert(out,sole,.014,.22)
    # Forefoot takes load slightly later. Avoid machine-like equal spacing.
    insert(out,subpulse(rng,.16,fundamental*1.35,34),.096+rng.uniform(-.009,.009),.33)
    # Upper leather/rubber rub and soft toe drag.
    scrape=noise(rng,n,290,1400)*np.exp(-t*32)
    insert(out,scrape,.060,.075)
    if kind=='grate':
        n=int(.13*HZ);t=np.arange(n)/HZ
        ring=(np.sin(2*np.pi*490*t)+.34*np.sin(2*np.pi*1125*t))*np.exp(-t*42)
        insert(out,ring,.023,.125)
    elif kind=='rock':
        for i in range(8):
            n=int(.025*HZ);t=np.arange(n)/HZ
            debris=noise(rng,n,550,4600)*np.exp(-t*rng.uniform(110,240))
            insert(out,debris,.028+i*.012+rng.uniform(-.004,.004),.078)
    else:
        insert(out,click(rng,.08,base=300),.034,.10)
    export(f'boot-{kind}'+('' if take==1 else f'-{take}'),out,2800+take,.045)


def reload(kind):
    # Distinct series of synchronized steel-on-steel impacts, extraction,
    # chambering and final bolt/chamber snaps. Each duration fits its gun.
    steps={
        'pistol': (.80,[(.03,960,.52),(.16,590,.53),(.41,700,.30),(.63,1140,.87)],[(.23,.23,.26)]),
        'carbine':(.96,[(.04,590,.62),(.22,780,.53),(.61,630,.45),(.82,1110,.94)],[(.28,.32,.33)]),
        'shotgun':(1.27,[(.08,460,.64),(.38,660,.64),(.65,590,.62),(.94,740,.71),(1.15,410,1.0)],[(.17,.16,.23),(.77,.19,.31)]),
        'arc':    (1.15,[(.06,1100,.34),(.38,720,.60),(.84,1280,.77),(1.04,1340,.85)],[(.21,.39,.30)]),
        'smg':    (1.05,[(.05,970,.56),(.27,800,.68),(.69,1000,.50),(.87,1310,.91)],[(.32,.32,.30)]),
        'launcher':(1.66,[(.08,360,.80),(.47,390,.62),(.79,330,.73),(1.19,470,.69),(1.53,300,1.12)],[(.19,.46,.39),(.96,.25,.31)]),
        'railgun':(1.78,[(.08,490,.65),(.45,690,.66),(.81,510,.67),(1.38,750,.79),(1.63,450,1.11)],[(.19,.48,.37),(1.04,.31,.27)]),
    }
    duration, strikes, slides=steps[kind]
    rng=np.random.default_rng(33000+list(steps).index(kind))
    out=np.zeros(round(HZ*duration))
    for at,base,weight in strikes:
        insert(out,click(rng, .12 if base<550 else .072, base, weight),at)
        if base<550:
            insert(out,subpulse(rng,.15,base*.24,55),at,.18)
    for at, length, weight in slides:
        n=int(length*HZ);t=np.arange(n)/HZ
        env=np.sin(np.pi*np.clip(t/length,0,1))**.65
        slide=noise(rng,n,360,6200)*env
        teeth=np.sin(2*np.pi*(440*t+380*t*t))*env
        insert(out,.24*slide+.17*teeth,at,weight)
    export('reload-'+kind,out,3300,.12)


if __name__=='__main__':
    for name in ('pistol','carbine','smg','shotgun','arc','launcher','railgun','enemy','guardian'):
        for variant in range(1,5): gunshot(name,variant)
    for kind in ('metal','concrete','rock'):
        for variant in range(1,5): impact(kind,variant)
    for kind in ('deck','grate','rock'):
        for variant in range(1,5): boot(kind,variant)
    for kind in ('pistol','carbine','smg','shotgun','arc','launcher','railgun'): reload(kind)
    print('Generated 67 original Ogg sound effects: 36 shots, 12 impacts, 12 boots, 7 reloads.')
