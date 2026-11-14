#!/usr/bin/env python3
"""Reproducible side-view weapon sprites using VOIDLINE's dark steel/colored armor language.
Run with Python 3, cairosvg and Pillow. Runtime only needs the baked WebP files.
"""
from pathlib import Path
import io, random, math
import cairosvg
from PIL import Image

OUT=Path(__file__).resolve().parent.parent/'assets'
INK='#080e1b'; SEAM='#111d2e'; DARK='#202d40'; STEEL='#34465c'; MID='#576b7b'; EDGE='#94acbf'; LIGHT='#c5d2dd'

class Art:
    def __init__(self,w,h,name,accent,hot):
        self.w=w;self.h=h;self.name=name;self.accent=accent;self.hot=hot;self.fragments=[]
        self.fragments.append(f'''<defs>
<linearGradient id="steel" x2=".16" y2="1"><stop stop-color="#8395a7"/><stop offset=".12" stop-color="#526779"/><stop offset=".52" stop-color="#35475c"/><stop offset=".90" stop-color="#243346"/><stop offset="1" stop-color="#111b2c"/></linearGradient>
<linearGradient id="inkplate" x2=".1" y2="1"><stop stop-color="#526379"/><stop offset=".2" stop-color="#29394d"/><stop offset="1" stop-color="#111929"/></linearGradient>
<linearGradient id="armor" x2=".15" y2="1"><stop stop-color="{hot}"/><stop offset=".13" stop-color="{accent}"/><stop offset=".7" stop-color="{accent}"/><stop offset="1" stop-color="#4e3438"/></linearGradient>
<linearGradient id="glass" x2="0" y2="1"><stop stop-color="#bbfcff"/><stop offset=".35" stop-color="#55f7fb"/><stop offset=".73" stop-color="#027f9c"/><stop offset="1" stop-color="#064054"/></linearGradient>
<radialGradient id="power"><stop stop-color="#e4ffff"/><stop offset=".31" stop-color="{hot}"/><stop offset=".73" stop-color="{accent}"/><stop offset="1" stop-color="#0e273e"/></radialGradient>
</defs>''')
    def add(self,s):self.fragments.append(s)
    def poly(self,points,fill='url(#steel)',stroke=INK,sw=5):
        pt=' '.join(','.join(map(str,p)) for p in points)
        self.add(f'<polygon points="{pt}" fill="{fill}" stroke="{stroke}" stroke-width="{sw}" stroke-linejoin="round"/>')
    def path(self,d,fill='url(#steel)',stroke=INK,sw=5):self.add(f'<path d="{d}" fill="{fill}" stroke="{stroke}" stroke-width="{sw}" stroke-linecap="round" stroke-linejoin="round"/>')
    def rect(self,x,y,w,h,fill='url(#steel)',rx=2,stroke=INK,sw=4):self.add(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{rx}" fill="{fill}" stroke="{stroke}" stroke-width="{sw}"/>')
    def line(self,x1,y1,x2,y2,col=EDGE,width=3,op=1):self.add(f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" stroke="{col}" stroke-width="{width}" opacity="{op}" stroke-linecap="round"/>')
    def circle(self,x,y,r,fill='url(#steel)',stroke=INK,sw=4):self.add(f'<circle cx="{x}" cy="{y}" r="{r}" fill="{fill}" stroke="{stroke}" stroke-width="{sw}"/>')
    def bolt(self,x,y,r=3):
        self.circle(x,y,r,'#a1b6c5',INK,2)
        self.line(x-r*.43,y,x+r*.43,y,INK,1)
    def slots(self,x,y,n,step=19,slot=9,height=8):
        for i in range(n):
            px=x+i*step
            self.poly([(px+5,y),(px+slot+5,y),(px+slot,y+height),(px,y+height)],INK,SEAM,1)
            self.line(px+4,y+height-1,px+slot-1,y+height-1,'#70899a',1)
    def highlights(self,x0,x1,y,step=32):
        for x in range(x0,x1,step):self.line(x,y,x+11,y,EDGE,1.5,.6)
    def render(self):
        svg=f'<svg xmlns="http://www.w3.org/2000/svg" width="{self.w}" height="{self.h}" viewBox="0 0 {self.w} {self.h}">'+''.join(self.fragments)+'</svg>'
        raw=cairosvg.svg2png(bytestring=svg.encode())
        im=Image.open(io.BytesIO(raw)).convert('RGBA')
        # Worn, brushed alloys, in the same slightly textured aesthetic as painted original sprites.
        rng=random.Random(1070+sum(map(ord,self.name)))
        pix=im.load()
        for _ in range(int(self.w*self.h*.09)):
            x=rng.randrange(6,self.w-6);y=rng.randrange(6,self.h-6)
            r,g,b,a=pix[x,y]
            if a<190 or max(r,g,b)<22:continue
            delta=rng.randrange(-33,38)
            pix[x,y]=(max(0,min(255,r+delta)),max(0,min(255,g+delta)),max(0,min(255,b+delta)),a)
        im.save(OUT/(self.name+'.webp'),format='WEBP',quality=95,method=6,exact=True)
        return im

# PISTOL — short enclosed slide, compact sidearm silhouette and an ergonomic high-angle grip.
p=Art(580,320,'pistol','#b58d56','#ead0a0')
p.poly([(36,93),(45,79),(80,75),(80,161),(39,158)],'url(#inkplate)')
p.poly([(65,80),(430,78),(459,89),(500,94),(514,130),(479,156),(72,160)],'url(#steel)')
p.poly([(476,96),(555,96),(557,139),(479,146)],'url(#inkplate)')
p.rect(537,105,32,27,'#161e2b',3)
p.rect(557,112,12,14,INK,2,'#667b89',2)
p.poly([(118,145),(281,144),(308,186),(280,202),(157,199),(131,178)],'url(#inkplate)')
p.path('M 175 185 L 266 184 Q 287 188 287 204 L 263 295 L 176 296 L 155 278 Z','url(#steel)')
p.poly([(184,201),(254,198),(246,273),(185,279),(172,265)],'#263348')
p.poly([(175,286),(263,285),(263,304),(169,305),(163,295)],'#162234')
for x in range(191,240,13):p.line(x,218,x+28,242,'#53647a',2)
p.poly([(262,159),(367,159),(365,183),(344,191),(312,195),(302,222),(281,218),(294,182)],'url(#steel)')
p.poly([(294,179),(346,182),(339,191),(318,194),(309,207),(297,204)],'#0a111f',SEAM,3)
p.path('M 297 194 Q 319 182 342 186 Q 344 203 328 214 L 302 215',fill='none',stroke='#8da4b2',sw=4)
p.poly([(80,81),(409,75),(458,84),(493,96),(477,132),(83,137)],'url(#inkplate)')
p.poly([(94,82),(330,80),(309,97),(94,100)],'#637887',INK,2)
p.poly([(99,109),(287,109),(276,123),(96,124)],'#916b4a',SEAM,3)
p.poly([(322,80),(425,81),(459,92),(445,116),(306,118)],'url(#armor)')
p.poly([(418,82),(460,93),(474,101),(468,112),(437,116)],'#e8bd80',SEAM,2)
p.rect(491,107,49,18,'#233348',2)
for x in (151,175,199,223):p.line(x,86,x-7,100,INK,4)
p.rect(334,100,68,12,'#071a28',2)
p.rect(342,102,54,7,'url(#glass)',1,INK,1)
p.poly([(87,68),(114,65),(121,78),(84,82)],'#1b2d41')
p.poly([(449,76),(474,75),(487,91),(456,89)],'#1b2d41')
p.circle(138,149,6);p.circle(416,145,5)
p.bolt(110,123);p.bolt(297,120);p.bolt(448,106)
p.line(68,146,471,146,'#151c2b',3)
p.line(87,87,404,83,'#9cb0ba',2)
p.line(179,204,177,270,'#7793a6',2,.6)
p.render()

# SMG — telescoping stock, stepped receiver, perforated barrel jacket, diagonal magazine.
s=Art(740,320,'smg','#cfaf43','#f7df8c')
s.poly([(20,132),(64,114),(117,115),(136,133),(135,216),(106,232),(20,225)],'url(#inkplate)')
s.poly([(30,151),(99,137),(104,199),(40,208)],'#324457')
s.poly([(48,161),(88,154),(91,183),(46,191)],'#0d1624')
s.poly([(115,146),(213,148),(215,163),(113,165)],'#172437')
s.rect(141,157,130,13,'#46586b',4)
s.rect(148,161,122,4,EDGE,1,SEAM,1)
s.poly([(225,121),(377,112),(414,124),(493,130),(524,165),(507,202),(246,209),(215,181)],'url(#steel)')
s.poly([(330,182),(406,184),(406,209),(372,299),(291,293),(305,240)],'url(#steel)')
s.poly([(311,225),(362,216),(350,279),(304,281)],'#1e2c41')
for y in range(239,280,11):s.line(314,y,349,y-4,'#60788c',2)
s.poly([(452,194),(511,200),(486,311),(438,300)],'url(#inkplate)')
s.poly([(462,208),(495,214),(475,290),(448,285)],'#526273')
for y in range(219,281,14):s.line(462,y,481,y+4,SEAM,3)
s.poly([(383,195),(456,195),(452,212),(422,216),(406,229),(382,226)],'#0d1a2a')
s.path('M 379 205 Q 407 212 415 229',fill='none',stroke='#728a9a',sw=4)
s.poly([(495,140),(644,142),(666,159),(665,196),(491,201)],'url(#inkplate)')
s.rect(650,151,72,32,'url(#steel)',2)
s.rect(706,161,22,14,'#151f30',3)
s.poly([(243,116),(382,113),(409,128),(492,135),(485,182),(239,180),(219,161)],'url(#armor)')
s.poly([(254,122),(342,120),(336,139),(246,141)],'#f1d18b')
s.poly([(372,130),(451,137),(467,153),(446,163),(361,160)],'#5c7181')
s.rect(385,151,48,14,'#0d1f2b',3);s.rect(391,153,36,8,'url(#glass)',1)
s.poly([(230,104),(312,103),(315,114),(222,120)],'#1a2739')
s.poly([(302,87),(373,87),(379,110),(304,109)],'url(#steel)')
s.rect(332,94,31,9,'url(#glass)',2)
s.poly([(505,132),(637,133),(652,147),(512,150)],'url(#steel)')
s.slots(528,164,6,20,10,13)
s.rect(532,187,115,10,'#111927',2)
s.rect(252,179,208,13,'url(#inkplate)',3)
s.rect(258,185,151,4,'#5f7483',2,SEAM,1)
s.bolt(267,155);s.bolt(458,178);s.bolt(635,151)
s.highlights(256,484,125)
s.line(31,221,109,221,'#8191a0',3)
s.render()

# LAUNCHER — high volume tube, rotating drum, heat shield, shoulder rest and heavy fore-support.
l=Art(850,340,'launcher','#c65a40','#ffac78')
l.poly([(25,132),(71,103),(133,103),(147,133),(149,215),(117,232),(24,223)],'url(#inkplate)')
l.poly([(42,145),(121,122),(123,195),(40,208)],'#526274')
l.poly([(79,157),(120,144),(119,190),(76,200)],'#142234')
l.rect(145,145,96,29,'#222e42',5)
l.rect(152,152,86,12,'#5c6875',1)
l.poly([(209,127),(318,111),(404,115),(436,134),(439,205),(336,230),(225,208)],'url(#steel)')
l.path('M 259 202 L 337 202 L 358 225 L 342 311 L 270 310 L 244 265 Z','url(#inkplate)')
l.poly([(277,239),(334,228),(318,287),(267,288)],'#3e5264')
for y in range(244,285,11):l.line(282,y,321,y-9,'#698293',2)
l.poly([(335,120),(487,101),(615,101),(691,122),(696,213),(601,225),(371,227),(323,202)],'url(#inkplate)')
l.rect(620,115,195,94,'url(#steel)',13)
l.rect(794,131,48,61,'#243347',6)
l.rect(820,148,26,29,INK,5,'#8da6b3',3)
l.rect(833,156,12,14,INK,2)
l.rect(649,126,149,10,'#8196a6',3)
l.rect(647,196,152,8,'#182738',3)
l.poly([(357,119),(558,112),(615,119),(623,163),(580,196),(358,190)],'url(#armor)')
l.poly([(369,123),(555,122),(565,141),(371,142)],'#ec9966',SEAM,2)
l.slots(492,147,6,17,10,15)
l.rect(325,101,178,12,'url(#steel)',2)
l.rect(414,86,102,19,'url(#inkplate)',3)
l.rect(467,91,31,9,'url(#glass)',2)
l.poly([(538,99),(596,102),(608,120),(531,118)],'#5e6d7c')
l.circle(472,231,78,INK,INK,4)
l.circle(472,231,68,'#34465a',SEAM,5)
l.circle(472,231,57,'url(#inkplate)','#8295a4',5)
for angle in (0,72,144,216,288):
    x=472+37*math.cos(math.radians(angle));y=231+37*math.sin(math.radians(angle))
    l.circle(round(x,1),round(y,1),15,'#111d2e','#7f8e9a',3)
    l.circle(round(x,1),round(y,1),8,'url(#power)',SEAM,2)
l.circle(472,231,9,'#b4a9a1',INK,4)
l.poly([(587,210),(630,202),(623,258),(601,273),(566,256)],'url(#steel)')
l.poly([(587,222),(616,219),(612,251),(583,244)],'#27364a')
l.poly([(273,117),(336,108),(340,136),(279,150)],'url(#armor)')
l.rect(252,160,69,12,'#26364c',3)
l.bolt(262,182);l.bolt(328,163);l.bolt(606,182);l.bolt(775,180)
l.line(24,220,119,225,'#a4b4c0',3)
l.render()

# RAILGUN — open dual-rail barrel, exposed capacitors/coils, long precision stock and violet trim.
r=Art(920,275,'railgun','#7d68c2','#cdbaff')
r.poly([(25,105),(60,75),(167,73),(198,100),(193,179),(149,204),(24,201)],'url(#inkplate)')
r.poly([(39,112),(110,88),(165,91),(173,118),(171,165),(49,178)],'#52657c')
r.poly([(50,120),(110,107),(147,111),(148,156),(56,164)],'#112034')
r.poly([(182,111),(277,112),(286,143),(184,144)],'url(#steel)')
r.poly([(255,88),(434,82),(493,111),(503,179),(434,200),(280,197),(244,164)],'url(#steel)')
r.poly([(304,183),(371,189),(359,247),(330,265),(295,258),(273,238)],'url(#inkplate)')
r.poly([(303,198),(351,197),(345,243),(310,243)],'#34485e')
for y in range(205,244,10):r.line(315,y,339,y+4,'#8096a7',2)
r.poly([(410,187),(454,181),(460,243),(435,263),(403,246)],'url(#steel)')
r.rect(408,201,33,36,'#1b2940',3)
r.path('M 363 178 Q 380 200 409 201',fill='none',stroke='#839bad',sw=5)
r.poly([(471,99),(859,100),(886,116),(890,152),(858,166),(473,172)],'url(#inkplate)')
r.poly([(484,96),(863,96),(887,107),(863,121),(488,122)],'#657b92')
r.poly([(490,155),(855,153),(882,162),(865,177),(491,180)],'#40526c')
r.rect(877,123,37,29,'url(#steel)',4)
r.rect(896,131,21,13,INK,2)
r.rect(492,126,362,23,'#0a1728',4)
r.rect(503,132,344,10,'url(#glass)',3)
r.line(520,135,848,135,'#c3ffff',2,.75)
for x in range(523,831,35):
    r.poly([(x,121),(x+11,123),(x+14,154),(x+3,158)],'#101c31',SEAM,2)
    r.line(x+8,124,x+8,149,r.hot,3,.9)
r.poly([(263,91),(447,89),(485,108),(466,163),(278,166),(250,137)],'url(#armor)')
r.poly([(272,98),(408,95),(427,107),(414,121),(274,122)],'#9b91d5',SEAM,2)
r.poly([(284,132),(459,130),(452,146),(278,148)],'url(#steel)')
r.slots(324,132,6,20,11,10)
r.poly([(345,74),(408,69),(429,87),(323,89)],'url(#inkplate)')
r.rect(359,77,45,7,'url(#glass)',2)
r.poly([(422,67),(471,65),(488,83),(422,88)],'#35445b')
r.poly([(478,174),(617,176),(617,193),(478,191)],'url(#steel)')
r.poly([(629,177),(736,175),(737,188),(630,191)],'#27384e')
r.bolt(292,159);r.bolt(451,155);r.bolt(857,167)
r.line(55,192,168,187,'#8fa3b4',3)
r.render()
print('Generated: '+', '.join(str(OUT/(n+'.webp')) for n in ['pistol','smg','launcher','railgun']))
