#include "render/renderer.hpp"
#include "render/figures/gun_art.hpp"
#include "render/figures/humanoid.hpp"
#include "content/skeleton.hpp"
#include "game/stairs.hpp"
#include "game/ladder_gait.hpp"

using namespace vl::figures;

namespace vl {
Renderer::Humanoid Renderer::operativeFigure(const Character& character) const {
    using Look=Humanoid::Look;
    const int index=int(&character-characters.data());
    const Color trim=hex(character.color);
    struct Palette { uint32_t suit,shell,skin,hair,leather,glove; };
    static constexpr Palette palettes[]{
        {0x323b48,0x566474,0xd5a07c,0x2e211b,0x4a382c,0x232832},
        {0x234650,0x3a6c7a,0x9a6a4d,0x15141c,0x4d3d30,0x1c2d33},
        {0x2f2d49,0x4d4a72,0xc79b78,0x8d919c,0x433a33,0x23212f},
        {0x4e2e2b,0x995b44,0xbd916e,0x251a18,0x4a352a,0x302021},
        {0x243d52,0x4a7194,0x79533d,0x121b21,0x2f353b,0x1d2d3d},
        {0x3e324d,0x6b567e,0xbd917c,0x28202f,0x3d3247,0x2e233b}
    };
    const auto& pal=palettes[std::clamp(index,0,5)];
    Humanoid h;
    // Six independent silhouette identities: the last three are original rigs, not mirrored originals.
    constexpr Look rigs[]{Look::voss,Look::rook,Look::vale,Look::mercer,Look::cole,Look::nyx};
    h.look=rigs[std::clamp(character.rig<0?index:character.rig,0,5)];
    h.model=&characterSkeletons[static_cast<size_t>(std::clamp(index,0,5))];
    h.suit=hex(pal.suit); h.shell=hex(pal.shell);
    h.trim=trim; h.edge=mix(trim,hex(0xffffff),.7f); h.shadow=hex(0x1d2430).energy(1.1f);
    h.ink=hex(0x06080d); h.skin=hex(pal.skin); h.hair=hex(pal.hair);
    h.leather=hex(pal.leather); h.glove=hex(pal.glove); h.visor=trim;
    // Armor silhouette and wearable gear are customized in the articulated rig passes.
    if (index>=3) h.scarf=false;
    return h;
}

void Renderer::drawPlayer(const Game& g,const Operative& op) {
    const auto& p=op.player;
    const Vec shift=offset+playerShift;
    const Vec ground{p.box.center().x+shift.x,p.box.bottom()+shift.y};
    Humanoid h=operativeFigure(*op.character);
    h.ground=ground; h.facing=p.facing; h.turn=p.turn;
    h.ladder=p.ladder>=0;
    h.ladderAiming=h.ladder && (p.aiming || p.muzzle>0 || p.fireBuffer>0);
    // Face the rungs square-on while gripping them. Firing rotates just the
    // upper torso back into the aim-facing profile, without twisting the feet.
    if (h.ladder) h.turn=h.ladderAiming?float(p.facing)*.72f:0.f;
    const bool moving=std::abs(p.velocity.x)>25;
    const bool backwards=moving && p.velocity.x*float(p.facing)<0;
    // The gait is measured relative to facing: downhill backsteps must not
    // invert the pelvis while preserving a forward-running torso twist.
    h.stride=backwards?-1:1;
    for (int i=0;i<2;++i) { h.heels[i]=drawHeels[i]+offset; h.footAngle[i]=drawFootAngles[i]; }
    const bool onStair=g.stage && stairFooting(*g.stage,p.box.center().x,p.box.bottom(),17.f).has_value();
    h.grounded=p.support>=0 || p.ladder>=0 || onStair;
    h.lean=pose.lean; h.crouch=pose.crouch; h.run=pose.run; h.weight=pose.weight; h.cycle=pose.cycle;
    h.compression=pose.compression; h.pelvis=pose.pelvis; h.tuck=pose.tuck;
    if (backwards && onStair) {
        // Lower the shoulder/pelvis exaggeration while backpedaling downhill.
        h.weight*=.66f;
        h.lean*=.62f;
        h.run*=.40f;
        h.cycle=1.f-h.cycle;
    }
    // Deterministic simulation-driven animation selection. Every state maps to
    // one optional editable pose layer; transitions are eased by the rig's
    // existing gait/aim/crouch blends rather than snapping the whole body.
    if (p.ladder>=0 && g.stage) {
        // The full spine and pelvis face the ladder; the shoulders remain
        // square and the legs alternate between a planted and a stepping foot.
        // Rung contacts are anchored in world space, so releasing the key does
        // not leave the boots cycling in place.
        const Ladder& ladder=g.stage->ladders[static_cast<size_t>(p.ladder)];
        const auto climb=ladder_gait::sample(p.box.bottom(),ladder.top,ladder.bottom);
        h.ground.x=ladder.x+shift.x;
        h.action=ActionPose::stairs;
        h.actionPhase=0.f;
        h.actionWeight=0.f; // Sloped-stair keyframes are not a ladder climb.
        h.run=0; h.weight=0; h.stride=1;
        h.crouch=0; h.lean=0; h.pelvis=0; h.tuck=0; h.compression=0;
        for (int i=0;i<2;++i) {
            h.heels[i]={h.ground.x+(i==0?-7.f:7.f),climb.feet[i]+shift.y};
            h.footAngle[i]=0.f;
        }
        // Diagonal hand/foot coordination. The support hand stays anchored
        // to a rung until the opposite foot finishes its step.
        h.nearHand={h.ground.x-8.f,climb.hands[0]+shift.y};
        h.farHand={h.ground.x+8.f,climb.hands[1]+shift.y};
    }

    if (p.hp<=0) h.action=ActionPose::death;
    else if (p.ladder>=0) h.action=ActionPose::stairs;
    else if (p.flash>.12f) h.action=ActionPose::hit;
    else if (op.guns[op.gun].reload>0) h.action=ActionPose::reload;
    else if (p.muzzle>0) h.action=ActionPose::fire;
    else if (p.support<0 && !onStair) h.action=p.velocity.y<0?ActionPose::jump:ActionPose::fall;
    else if (p.landed>.1f) h.action=ActionPose::land;
    else if (p.crouched) h.action=ActionPose::crouch;
    else if (moving && g.stage && stairFooting(*g.stage,p.box.center().x,p.box.bottom(),8.f))
        h.action=ActionPose::stairs;
    else if (moving) h.action=p.sprinting?ActionPose::sprint:ActionPose::walk;
    else if (p.aiming) h.action=ActionPose::alert;
    else h.action=ActionPose::idle;
    h.actionWeight=p.ladder>=0?0.f:
        (h.action==ActionPose::walk||h.action==ActionPose::sprint||h.action==ActionPose::stairs)?
            std::clamp(h.weight,0.f,1.f):1.f;
    // Authoring timeline: poseFrames are the same five normalized keyframes
    // visible in Model Studio. Sample jumps by upward velocity, falls by fall
    // velocity, crouch by stance blending, and land by recovery time. The
    // existing physics, IK and terrain contact remain authoritative.
    if (p.ladder>=0) h.actionPhase=p.gait.cycle;
    else switch(h.action){
    case ActionPose::jump: h.actionPhase=clamp(1.f+p.velocity.y/700.f); break;
    case ActionPose::fall: h.actionPhase=clamp(p.velocity.y/650.f); break;
    case ActionPose::crouch: h.actionPhase=clamp(p.gait.crouch); break;
    case ActionPose::land: h.actionPhase=clamp(1.f-p.landed/.38f); break;
    case ActionPose::walk: case ActionPose::sprint: case ActionPose::stairs:
        h.actionPhase=pose.cycle-std::floor(pose.cycle); break;
    case ActionPose::fire: h.actionPhase=clamp(1.f-p.muzzle/.08f); break;
    case ActionPose::hit: h.actionPhase=clamp(1.f-p.flash/.7f); break;
    default: h.actionPhase=g.time-std::floor(g.time); break;
    }
    h.time=g.time; h.velocity=p.velocity; h.landed=p.landed;
    h.flinch=smooth((p.flash-.45f)/.24f);
    h.spin=p.flip>0?p.flipDirection*2*pi*smooth(1-p.flip/flipSeconds):0.f;
    if (p.flash>0 && std::sin(p.flash*80)>0) {
        h.suit=hex(0xd9a79a); h.shell=hex(0xffffff);
    }

    drawTrails(g,op,ground);

    // Weapon geometry in screen space.
    const float forwardFacing=p.facing>0?1.f:-1.f;
    const float counter=std::sin(pose.cycle*2*pi)*pose.weight;
    const float local=std::atan2(std::sin(p.aim),std::abs(std::cos(p.aim)));
    // The rifle comes up while the operative aims, fires or reloads and drops to a low carry afterwards. While it
    // hangs from the near hand the far arm is free to swing with the stride.
    // Smooth, two-handed shoulder presentation: the stock rises from the low
    // carry as the off-hand comes onto the foregrip; running no longer forces
    // a rifle back down while the player is actively aiming or shooting.
    const float shouldered=smooth(clamp(p.ready));
    const float lowered=clamp((1-shouldered)*(1.f+carry*.18f));
    const float barrel=mix(.62f+counter*.10f,local,shouldered);
    const Vec aim{std::cos(barrel)*forwardFacing,std::sin(barrel)};
    const Vec side{-aim.y,aim.x};
    const Vec up=forwardFacing>0?Vec{aim.y,-aim.x}:Vec{-aim.y,aim.x};
    const float recoil=p.muzzle>0?3.7f*p.muzzle/.069f*h.model->recoil:0;
    const float sway=std::sin(g.time*1.7f)*.35f*(1-pose.weight);
    const Vec mount=g.weaponPivot(p)+shift+Vec{-lowered*5.f*p.facing+h.model->weaponSeat*p.facing,(pose.crouch-p.gait.crouch)*12+lowered*8.f+pose.pelvis*.5f+sway};
    const Vec carried=mount+aim*muzzleReach[op.gun];
    const Vec raised=g.muzzle(op)+shift+Vec{0,pose.pelvis*.5f+sway};
    Vec muzzle=mix(raised,carried,lowered)-aim*(recoil*.45f);
    const GunArt& model=gunArt[op.gun];
    const Vec axisU=aim*model.length,axisV=up*(-model.length*model.aspect);
    h.aim=aim; h.recoil=recoil/3.7f; h.relax=lowered; h.shoulder=shouldered;
    auto layout=[&] {
        h.nearHand=muzzle+axisU*(model.gripU-1)+axisV*(model.gripV-model.muzzleV);
        h.farHand=muzzle+axisU*(model.foreU-1)+axisV*(model.foreV-model.muzzleV);
    };
    layout();
    if (h.ladder && !h.ladderAiming) {
        // The weapon is slung while both hands alternate on the rungs.
        // The legs and pelvis stay on the ladder's axis, not inside it.
        // Grips were calculated from actual rung heights above. Keep them
        // instead of generating a second, unrelated sine-wave arm animation.
        const Ladder& ladder=g.stage->ladders[static_cast<size_t>(p.ladder)];
        const auto climb=ladder_gait::sample(p.box.bottom(),ladder.top,ladder.bottom);
        h.nearHand={h.ground.x-8.f,climb.hands[0]+shift.y};
        h.farHand={h.ground.x+8.f,climb.hands[1]+shift.y};
        h.relax=0; h.nearRelax=0; h.shoulder=0;
    }
    // The support hand must reach the foregrip with a bent elbow. If the weapon is held too far out, it slides
    // back into the shoulder instead of stretching the arm.
    if (lowered>.04f) {
        // During the transition the rifle slides inward to keep the elbows
        // bent. At full shoulder presentation the rendered muzzle MUST match
        // Game::muzzle(), the actual bullet-spawn point.
        const Rig rig=buildRig(h);
        const float reach=length(h.farHand-spun(h,rig.farShoulder));
        constexpr float comfortable=19.f;
        if (reach>comfortable) { muzzle=muzzle-aim*(std::min(10.f,reach-comfortable)*lowered); layout(); }
    }
    const Color gunColor=hex(weapons[op.gun].color);
    if (!h.ladder || h.ladderAiming) h.weapon=[&](Vec,Vec) {
        sprite(model.name,muzzle,axisU,axisV,{1.f,model.muzzleV});
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
