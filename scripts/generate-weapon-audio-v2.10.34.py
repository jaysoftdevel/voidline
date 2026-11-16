#!/usr/bin/env python3
"""VOIDLINE weapon authoring script. Generates original gunshot and reload Foley.

This is sound design, NOT a field recording of a firearm. The previous sine-heavy
FX have been replaced with multi-band shock transients, diffuse reports, and
more detailed magazine/bolt/receiver mechanisms. Audio assets are committed to
assets/audio; no Python dependencies are needed by a player or the normal build.

Authoring-only requirements: numpy, scipy, soundfile.
"""
from pathlib import Path
import numpy as np
import soundfile as sf
from scipy.signal import butter, sosfilt

HZ = 48000
OUT = Path(__file__).resolve().parents[1] / 'assets' / 'audio'
OUT.mkdir(parents=True, exist_ok=True)


def filtered(x, lo=0, hi=0):
    if lo and hi:
        sos = butter(3, (max(35, lo), min(HZ*.47, hi)), fs=HZ, btype='band', output='sos')
    elif lo:
        sos = butter(3, lo, fs=HZ, btype='high', output='sos')
    else:
        sos = butter(3, hi, fs=HZ, btype='low', output='sos')
    return sosfilt(sos, x)


def noise(rng, length, lo=0, hi=0):
    return filtered(rng.standard_normal(length), lo, hi)


def put(out, sound, start, amp=1):
    idx = round(start*HZ)
    if idx >= len(out): return
    count = min(len(sound), len(out)-idx)
    if count: out[idx:idx+count] += sound[:count]*amp


def curve(t, attack, decay):
    return (1 - np.exp(-t/max(.00005,attack))) * np.exp(-t/max(.0001,decay))


def thump(rng, dur=.11, strength=1, low=130):
    n=round(HZ*dur); t=np.arange(n)/HZ
    # Unpitched mass followed by a short, damped mechanical resonance.
    a=noise(rng,n,48,1100) * np.exp(-t/0.035)
    b=np.sin(2*np.pi*(low*t - low*10*t*t)) * np.exp(-t/0.025)
    return strength*(.34*a+.45*b)


def metal(rng, freq=840, heavy=1, dur=.15):
    n=round(dur*HZ); t=np.arange(n)/HZ
    # Inharmonic metal modes, excited together by a sharp dry impact.
    modes=np.zeros(n)
    for ratio, vol, decay in ((1,.26,.033),(1.55,.17,.019),(2.31,.12,.023),
                               (3.48,.10,.014),(4.13,.08,.011),(5.78,.05,.010)):
        f=freq*ratio
        if f<HZ*.47:
            modes += vol*np.sin(2*np.pi*(f*t+f*.06*t*t)+rng.uniform(-np.pi,np.pi))*np.exp(-t/decay)
    exc=noise(rng,n,450,15000)*np.exp(-t/.006)
    body=noise(rng,n,180,2200)*np.exp(-t/.020)
    return heavy*(.45*exc+.52*modes+.16*body)


def scrape(rng, sec, intensity=.35, pitch=950):
    n=round(sec*HZ); t=np.arange(n)/HZ
    env=(np.sin(np.pi*np.clip(t/sec,0,1))**.58)
    grit=noise(rng,n,400,7800)
    # Irregular ratchet-like motion; non-sine teeth/drag.
    flutter=.5+.25*np.sin(2*np.pi*(19*t+38*t*t))+.25*np.sin(2*np.pi*34*t)
    reson=np.sin(2*np.pi*(pitch*t+90*t*t))*np.exp(-t*4)
    return intensity*(.24*grit*env*flutter+.12*reson*env)


def firearm_shot(kind, take):
    # seconds, crack, pressure, diffuse tail, receiver noise, characteristic pitch
    prof={
      'pistol':   (.44,1.00,.49,.16,.26,890),
      'carbine':  (.55,1.14,.77,.21,.32,760),
      'smg':      (.36,.95,.42,.12,.23,1010),
      'shotgun':  (.82,1.13,1.20,.42,.39,600),
      'arc':      (.67,1.02,.64,.19,.24,1220),
      'launcher': (1.00,.94,1.32,.57,.45,470),
      'railgun':  (.86,1.20,1.19,.38,.40,670),
      'enemy':    (.49,1.04,.62,.19,.23,770),
      'guardian': (.82,1.08,1.23,.40,.40,620),
    }
    sec, crack, pressure, tail, cycle, pitch=prof[kind]
    rng=np.random.default_rng(180001 + list(prof).index(kind)*953 + take*7919)
    out=np.zeros(round(sec*HZ)); start=.002
    n=round(min(.26,sec-.005)*HZ); t=np.arange(n)/HZ
    # Very fast impulse (sub-millisecond attack) + 8-ms snap. Unlike the old
    # algorithm, do not fade out the first 4 ms of the entire effect.
    direct=noise(rng,n,500,19000)*curve(t,.00012,.009)
    sharp=noise(rng,n,1700,20000)*curve(t,.00007,.004)
    close=noise(rng,n,150,4500)*curve(t,.00035,.034)
    # Bass blast is predominantly broadband pressure, avoiding sine "boing".
    bass=noise(rng,n,55,1000)*curve(t,.0007,.080 if kind in ('shotgun','launcher','railgun','guardian') else .049)
    put(out,crack*(.63*direct+.28*sharp+.48*close)+pressure*.52*bass,start)
    # Actual muzzle reports are clustered impulses, not one continuous sine.
    if kind in ('shotgun','launcher','railgun','guardian'):
        put(out,noise(rng,round(.035*HZ),180,9700)*np.exp(-np.arange(round(.035*HZ))/HZ/.008),start+.006,crack*.24)
        put(out,thump(rng,.19,1,75 if kind in ('launcher','guardian') else 103),start+.004,pressure*.47)
    # Irregular, band-limited early reflections add a perceived space/tail.
    reflected=filtered(out,hi=4700)
    for delay,gain in ((.017,.16),(.041,.21),(.084,.12),(.148,.10),(.224,.06),(.311,.04)):
        i=round(delay*HZ)
        if i<len(out): out[i:] += gain*tail*reflected[:-i]
    # Receiver moving/brief spent brass -- well below the gunfire itself.
    put(out,metal(rng,pitch,1,.10),.048+take*.0014,cycle*.13)
    put(out,metal(rng,1800,.28,.075),.114+take*.0019,cycle*.10)
    # Fictional weapons retain a character, but keep the ballistic report first.
    if kind in ('arc','railgun'):
        n=min(round(.23*HZ),len(out)); t=np.arange(n)/HZ
        chirp=np.sin(2*np.pi*((270 if kind=='arc' else 120)*t + (870 if kind=='arc' else 2200)*t*t))
        put(out,chirp*np.exp(-t/(.062 if kind=='arc' else .085)),.025,.055)
    return out


def reload_sequence(kind):
    rng=np.random.default_rng(352991+sum(map(ord,kind)))
    lengths={'pistol':.96,'carbine':1.18,'smg':1.01,'shotgun':1.50,
             'arc':1.30,'launcher':1.70,'railgun':1.76}
    out=np.zeros(round(HZ*lengths[kind]))
    def click(at,f=900,a=.65): put(out,metal(rng,f,a),at)
    def move(at,d=.22,a=.36,f=880): put(out,scrape(rng,d,a,f),at)
    def clack(at, f=530, a=.6):
        click(at,f,a)
        put(out,thump(rng,.11,a*.33,f*.25),at+.005)

    if kind=='pistol':
        # magazine release -> mag out -> fresh mag in -> slide rack -> battery
        click(.050,1320,.48)
        move(.132,.140,.45,530); click(.281,680,.55)
        clack(.474,680,.85)
        click(.537,1160,.27)
        move(.683,.128,.54,1250)
        clack(.814,880,1.18)
    elif kind=='smg':
        click(.045,1230,.38)
        move(.129,.149,.40,620); clack(.288,570,.72)
        clack(.553,710,.94)
        move(.673,.102,.48,1170)
        clack(.811,780,1.00)
    elif kind=='carbine':
        click(.055,900,.41)
        move(.163,.163,.48,630); clack(.331,525,.66)
        clack(.608,605,.97)  # magazine slap
        move(.790,.166,.59,1100) # charging handle pull
        clack(.978,680,1.14)
    elif kind=='shotgun':
        clack(.072,440,.73)
        # Three distinctly separated shells entering tubular magazine.
        for at in (.350,.606,.858):
            move(at,.062,.30,620)
            click(at+.071,930,.82)
        move(1.120,.160,.65,490)
        clack(1.304,440,1.19) # pump-action "cha-chunk"
    elif kind=='arc':
        click(.064,1170,.59)
        move(.192,.208,.43,1160)
        clack(.505,560,.92) # energy cell lock
        move(.724,.206,.52,1180)
        clack(1.064,920,.89)
    elif kind=='launcher':
        clack(.056,365,.94)   # heavy breech open
        move(.294,.235,.52,520)
        clack(.662,380,.88)   # shell seats
        move(.895,.276,.50,480)
        clack(1.319,315,1.16) # breech closes
        click(1.514,1120,.57) # latch
    elif kind=='railgun':
        clack(.068,410,.88)
        move(.318,.253,.58,660)
        click(.646,1040,.50)
        clack(.821,460,.89)
        move(1.072,.257,.53,1160)
        clack(1.425,385,1.15)
        click(1.588,1320,.42)
    return out


def write_ogg(name, sound, seed, stereo=.10):
    # Peak management without flattening the initial gunshot transient.
    peak=np.max(np.abs(sound))
    mono=sound*(.91/max(peak,1e-6))
    # Restrained program compression raises the audible blast/metal details
    # without crushing the sub-millisecond muzzle peak.
    mono=np.tanh(1.65*mono)/np.tanh(1.65)*.91
    mono=np.clip(mono,-.94,.94)
    # ~1-ms fade at end only; start transient is left intact.
    n=min(128,len(mono)//5)
    mono[-n:]*=np.linspace(1,0,n)
    rng=np.random.default_rng(seed)
    l=mono.copy(); r=mono.copy()
    # Small side-width confined to diffuse late energy, first transient centered.
    offset=round(.0078*HZ)
    r[offset:]+=np.concatenate((np.zeros(offset),mono[:-offset]))[offset:] * stereo*.22
    l[offset:]+=np.concatenate((np.zeros(offset),mono[:-offset]))[offset:] * stereo*.08
    stereo_file=np.stack([l,r],axis=1).astype(np.float32)
    stereo_file *= min(1.0, .83/max(1e-6, float(np.max(np.abs(stereo_file)))))
    filename=OUT/f'{name}.ogg'
    for _ in range(4):
        sf.write(filename,stereo_file,HZ,format='OGG',subtype='VORBIS')
        decoded, _ = sf.read(filename)
        if np.max(np.abs(decoded)) < .94: break
        stereo_file *= .91/max(1e-6, float(np.max(np.abs(decoded))))


def main():
    guns=('pistol','carbine','smg','shotgun','arc','launcher','railgun','enemy','guardian')
    for gun in guns:
        for take in range(1,5):
            write_ogg(gun + (f'-{take}' if take>1 else ''),firearm_shot(gun,take),take*1000+len(gun))
    for gun in guns[:7]:
        write_ogg('reload-'+gun,reload_sequence(gun),10000+len(gun),.12)
    print('Rendered 36 firearm reports and 7 mechanical reload sequences.')

if __name__=='__main__': main()
