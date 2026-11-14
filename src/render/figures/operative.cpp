#include "render/renderer.hpp"
#include "render/figures/gun_art.hpp"
#include "render/figures/humanoid.hpp"

using namespace vl::figures;

namespace vl {
void Renderer::drawPlayer(const Game& g,const Operative& op) {
    using Look=Humanoid::Look;
    const auto& p=op.player;
    const Vec shift=offset+playerShift;
    const Vec ground{p.box.center().x+shift.x,p.box.bottom()+shift.y};
    const int index=int(op.character-characters.data());
    const Color trim=hex(op.character->color);
    const bool hurtBlink=p.flash>0 && std::sin(p.flash*80)>0;

    struct Palette { uint32_t suit,shell,skin,hair,leather,glove; };
    static constexpr Palette palettes[]{
        {0x323b48,0x566474,0xd5a07c,0x2e211b,0x4a382c,0x232832},
        {0x234650,0x3a6c7a,0x9a6a4d,0x15141c,0x4d3d30,0x1c2d33},
        {0x2f2d49,0x4d4a72,0xc79b78,0x8d919c,0x433a33,0x23212f}
    };
    const auto& pal=palettes[std::clamp(index,0,2)];
    Humanoid h;
    h.look=static_cast<Look>(std::clamp(index,0,2));
    h.ground=ground; h.facing=p.facing; h.turn=p.turn;
    const bool moving=std::abs(p.velocity.x)>25;
    h.stride=moving?pose.direction:p.facing;
    for (int i=0;i<2;++i) { h.heels[i]=drawHeels[i]+offset; h.footAngle[i]=drawFootAngles[i]; }
    h.grounded=p.support>=0;
    h.lean=pose.lean; h.crouch=pose.crouch; h.run=pose.run; h.weight=pose.weight; h.cycle=pose.cycle;
    h.compression=pose.compression; h.pelvis=pose.pelvis; h.tuck=pose.tuck;
    h.time=g.time; h.velocity=p.velocity; h.landed=p.landed;
    h.flinch=smooth((p.flash-.45f)/.24f);
    h.spin=p.flip>0?p.flipDirection*2*pi*smooth(1-p.flip/flipSeconds):0.f;
    h.scarf=true;
    h.suit=hurtBlink?hex(0xd9a79a):hex(pal.suit);
    h.shell=hurtBlink?hex(0xffffff):hex(pal.shell);
    h.trim=trim; h.edge=mix(trim,hex(0xffffff),.7f); h.shadow=hex(0x1d2430).energy(1.1f);
    h.ink=hex(0x06080d); h.skin=hex(pal.skin); h.hair=hex(pal.hair);
    h.leather=hex(pal.leather); h.glove=hex(pal.glove); h.visor=trim;

    drawTrails(g,op,ground);

    // Weapon geometry in screen space.
    const float forwardFacing=p.facing>0?1.f:-1.f;
    const float counter=std::sin(pose.cycle*2*pi)*pose.weight;
    const float local=std::atan2(std::sin(p.aim),std::abs(std::cos(p.aim)));
    // The rifle comes up while the operative aims, fires or reloads and drops to a low carry afterwards. While it
    // hangs from the near hand the far arm is free to swing with the stride.
    const float lowered=std::max(carry,1-p.ready);
    const float barrel=mix(local,.58f+counter*.13f,lowered);
    const Vec aim{std::cos(barrel)*forwardFacing,std::sin(barrel)};
    const Vec side{-aim.y,aim.x};
    const Vec up=forwardFacing>0?Vec{aim.y,-aim.x}:Vec{-aim.y,aim.x};
    const float recoil=p.muzzle>0?3.7f*p.muzzle/.069f:0;
    const float sway=std::sin(g.time*1.7f)*.35f*(1-pose.weight);
    const Vec mount=g.weaponPivot(p)+shift+Vec{-lowered*5.f*p.facing,(pose.crouch-p.gait.crouch)*12+lowered*7.f+pose.pelvis*.5f+sway};
    const Vec carried=mount+aim*muzzleReach[op.gun];
    const Vec raised=g.muzzle(op)+shift+Vec{0,pose.pelvis*.5f+sway};
    Vec muzzle=mix(raised,carried,lowered)-aim*(recoil*.45f);
    const bool art=op.gun<3;
    const GunArt& model=gunArt[art?op.gun:0];
    const float gunLength=model.length,gunHeight=model.length*model.aspect;
    const Vec axisU=aim*gunLength,axisV=up*(-gunHeight);
    h.aim=aim; h.recoil=recoil/3.7f; h.relax=lowered;
    // Hand positions for the drawn weapons, in the gun's own coordinates: forward along the barrel, then up.
    struct Grip { float nearU,nearV,farU,farV; };
    static constexpr Grip grips[]{{-9.5f,-3.f,-6.f,-2.6f},{-16.f,-5.5f,-9.f,-5.5f},{-17.f,-5.5f,-10.f,-6.f},{-22.5f,-6.f,-45.f,-1.f}};
    const Grip* grip=art?nullptr:&grips[op.gun-3];
    auto layout=[&] {
        h.nearHand=art?muzzle+axisU*(model.gripU-1)+axisV*(model.gripV-model.muzzleV):muzzle+aim*grip->nearU+up*grip->nearV;
        h.farHand=art?muzzle+axisU*(model.foreU-1)+axisV*(model.foreV-model.muzzleV):muzzle+aim*grip->farU+up*grip->farV;
    };
    layout();
    // The support hand must reach the foregrip with a bent elbow. If the weapon is held too far out, it slides
    // back into the shoulder instead of stretching the arm.
    if (lowered<.5f) {
        const Rig rig=buildRig(h);
        const float reach=length(h.farHand-spun(h,rig.farShoulder));
        constexpr float comfortable=19.f;
        if (reach>comfortable) { muzzle=muzzle-aim*std::min(10.f,reach-comfortable); layout(); }
    }
    const Color gunColor=hex(weapons[op.gun].color);
    h.weapon=[&](Vec,Vec) {
        if (art) sprite(model.name,muzzle,axisU,axisV,{1.f,model.muzzleV});
        else gunModel(op.gun,muzzle,aim,up,1.f,gunColor);
    };
    humanoid(h);

    if (p.muzzle>0) {
        const float flash=p.muzzle/.069f;
        const Vec spread=side*(5.6f+flash*7.f),tip=muzzle+aim*(11+flash*22);
        halo(muzzle,52,gunColor.energy(2.2f).alpha(flash*.70f));
        triangle(muzzle-spread,tip,muzzle+spread,gunColor.energy(2.8f).alpha(.92f));
        triangle(muzzle-spread*.3f,tip-aim*2,muzzle+spread*.3f,hex(0xfff6df).energy(3.1f));
        for (int i=-1;i<=1;i+=2) {
            const Vec dir=normal(aim+side*(.62f*i));
            triangle(muzzle-side*(1.6f*i),muzzle+dir*(10+flash*12),muzzle+side*(1.6f*i)*.2f,gunColor.energy(2.6f).alpha(.85f));
        }
        ellipse(muzzle+aim*2,{4.2f+flash*3.f,4.2f+flash*3.f},hex(0xfff6df).energy(3.2f));
    }
}
} // namespace vl
