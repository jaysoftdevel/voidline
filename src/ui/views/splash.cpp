#include "ui/app.hpp"
#include "ui/common.hpp"
#include <cmath>

using namespace vl::ui;

namespace vl {
void App::splash() {
    const float t=startup;
    const float appear=smooth(t/.95f);
    const float vanish=1-smooth((t-2.65f)/.5f);
    const float alpha=appear*vanish;
    const Color cyan=hex(0x8eefe1),warm=hex(0xffae7b);
    r.gradient(screen,hex(0x040b12),hex(0x0c1b2a));
    // Moving grid: diagonals travel toward the signal center as the uplink boots.
    for (int i=-10;i<25;++i) {
        const float x=std::fmod(i*96.f+t*31.f+2200.f,1536.f)-170;
        r.line({x,0},{x-155.f,height},.9f,cyan.alpha(.055f*alpha));
    }
    for (int i=0;i<13;++i) {
        const float y=36.f+i*54.f;
        r.line({0,y},{width,y},.8f,cyan.alpha(.045f*alpha));
    }
    const Vec center{600,236};
    const float radius=84.f+22.f*(1-smooth(t/1.2f));
    for (int ring=0;ring<3;++ring) {
        const float r0=radius+ring*16.f;
        for (int s=0;s<64;++s) {
            const float a=(s/64.f)*2*pi+t*(ring%2?-.31f:.45f);
            if (s%16<3) continue;
            const Vec a0=center+ray(a)*r0,a1=center+ray(a+2*pi/64.f)*r0;
            r.line(a0,a1,ring?1.2f:2.1f,cyan.alpha((ring?.24f:.65f)*alpha));
        }
    }
    for (int i=0;i<6;++i) {
        const float a=t*1.1f+i*pi/3;
        const Vec p=center+ray(a)*radius;
        r.halo(p,14,cyan.alpha(.23f*alpha));
        r.ellipse(p,{2.5f,2.5f},warm.alpha(alpha));
    }
    const float scan=std::fmod(t*340.f,430.f);
    r.beam({center.x-215+scan,center.y-115},2,{center.x-215+scan,center.y+112},21,
           cyan.alpha(.18f*alpha),cyan.alpha(0));
    r.halo(center,110,cyan.alpha(.10f*alpha));
    r.image("voidline-logo.png",{center.x-51,center.y-51,102,102},hex(0xe9fbff).alpha(alpha));
    r.line({362,379},{838,379},1.5f,cyan.alpha(.55f*alpha));
    r.text({376,436},"VOIDLINE",81,white.alpha(alpha),true);
    r.text({470,470},"VEGA-9  /  SIGNAL REACQUIRED",15,cyan.alpha(alpha),true);
    const float progress=smooth(t/2.7f);
    r.rect({470,511,260,4},hex(0x18313b).alpha(alpha));
    for (int i=0;i<20;++i) {
        if ((i+1)/20.f>progress) break;
        r.rect({471.f+i*13,511,11,4},(i%5==0?warm:cyan).alpha(alpha));
    }
    r.text({482,541},t<.95f?"INITIALIZING LINK":t<2.2f?"SYNCHRONIZING MISSION DATA":"SYSTEM READY",12,pale.alpha(alpha));
    r.text({985,570},"PRESS ANY KEY TO SKIP",11,muted.alpha(alpha));
}
} // namespace vl
