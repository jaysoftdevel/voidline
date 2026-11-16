// Movement, jumping, flipping and determinism.
#include "support.hpp"
#include "game/enemy_ground.hpp"
#include "game/detail.hpp"
#include "game/running_cycle.hpp"
#include "game/boss_tactics.hpp"
#include "game/geometry.hpp"
#include "game/stairs.hpp"
#include "content/skeleton_types.hpp"

using namespace test;
using namespace vl;

namespace {
void bossAggression() {
    using namespace vl::boss_tactics;
    const auto early=decide(1.f,1.f,0,400.f,67.f,1.f,true);
    const auto raging=decide(1.f,.16f,2,400.f,67.f,1.f,true);
    CHECK(raging.desiredSpeed>early.desiredSpeed);
    CHECK(raging.attackInterval<early.attackInterval);
    CHECK(raging.telegraph<early.telegraph);
    CHECK(decide(1.f,.16f,2,400.f,67.f,1.f,false).attack==false);
    CHECK(decide(1.f,.16f,2,90.f,67.f,1.f,true).desiredSpeed<0.f);
}

void bossArenaPursuit() {
    Silent audio;
    Game game(audio);
    game.start(0,startSave());
    game.entry=0;
    const auto bossIt=std::find_if(game.enemies.begin(),game.enemies.end(),[](const Enemy& e) {
        return e.kind==EnemyKind::boss;
    });
    CHECK(bossIt!=game.enemies.end());
    if (bossIt==game.enemies.end()) return;
    Enemy& boss=*bossIt;
    game.player.box.x=boss.box.x-450.f;
    game.player.box.y=boss.box.bottom()-game.player.box.h;
    const float original=boss.box.x;
    for (int i=0;i<120;++i) game.tick();
    CHECK(game.bossActive);
    CHECK(boss.box.x<original-12.f);
    CHECK(boss.box.x>=game.stage->ground.back().x);
    boss.hp=std::max(1,boss.maxHp/4);
    for (int i=0;i<6;++i) game.tick();
    CHECK(game.bossPhase>=2);
}

void chordsDriveTheGame() {
    using namespace vl;
    Silent audio;
    Game game(audio);
    game.start(0, startSave());
    game.entry = 0;
    const float startX = game.player.box.x;
    const int ammo = game.guns[game.gun].ammo;
    game.input(Key::jump, true);
    for (Key k : {Key::right, Key::sprint, Key::automatic, Key::slow}) game.input(k, true);
    float scale = 1;
    for (int i = 0; i < 120; ++i) { scale = game.advance(1.f / 120); game.tick(); }
    CHECK(game.player.box.x > startX + 40);
    CHECK(game.guns[game.gun].ammo < ammo);
    CHECK(game.slowMotion());
    CHECK(scale < .9f);
    CHECK(game.slowEnergy < 1.f);
    // Release the chord and the world returns to full speed.
    for (Key k : {Key::right, Key::sprint, Key::automatic, Key::slow, Key::jump}) game.input(k, false);
    for (int i = 0; i < 240; ++i) scale = game.advance(1.f / 120);
    CHECK(scale > .99f);
}
void airFlip() {
    using namespace vl;
    Silent audio;
    Game game(audio);
    game.start(0, startSave());
    game.entry = 0;
    run(game, 30);
    CHECK(game.player.support >= 0);
    game.input(Key::jump, true);
    run(game, 60);
    CHECK(game.player.support < 0);
    CHECK(!game.player.airJumpUsed);
    game.input(Key::jump, false);
    const float before = game.player.velocity.y;
    game.input(Key::jump, true);
    run(game, 2);
    CHECK(game.player.airJumpUsed && game.player.flip > 0);
    CHECK(game.player.velocity.y < before);
    // Only one flip per airtime.
    game.input(Key::jump, false);
    run(game, 40);
    game.input(Key::jump, true);
    const float second = game.player.velocity.y;
    run(game, 2);
    CHECK(game.player.velocity.y > second - 1.f);
}
void deterministic() {
    using namespace vl;
    auto simulate = [] {
        Silent audio;
        Game game(audio);
        game.start(1, startSave());
        game.entry = 0;
        game.input(Key::right, true);
        game.input(Key::automatic, true);
        for (int i = 0; i < 600; ++i) {
            if (i % 90 == 10) game.input(Key::jump, true); else if (i % 90 == 40) game.input(Key::jump, false);
            game.tick();
        }
        return std::make_tuple(game.player.box.x, game.player.box.y, game.score, game.kills, game.particles.size());
    };
    CHECK(simulate() == simulate());
}

// Crouch, move and jump held together: the jump must not be swallowed by the crouch, whatever the press order.
void chordWithCrouch() {
    for (int order = 0; order < 2; ++order) {
        Silent audio;
        Game game(audio);
        game.start(0, startSave());
        game.entry = 0;
        run(game, 30);
        const float floorY = game.player.box.bottom();
        if (order == 0) { game.input(Key::crouch, true); game.input(Key::right, true); game.input(Key::sprint, true); }
        else { game.input(Key::right, true); game.input(Key::sprint, true); game.input(Key::crouch, true); }
        run(game, 40);
        CHECK(game.player.crouched);
        game.input(Key::jump, true);
        run(game, 24);
        CHECK(game.player.support < 0 && game.player.box.bottom() < floorY - 4);
        CHECK(!game.player.crouched);
        // The crouch key is still held in the air and after landing. The operative stays up until it is released.
        game.input(Key::right, false); game.input(Key::sprint, false);
        run(game, 240);
        CHECK(game.player.support >= 0 && !game.player.crouched);
        game.input(Key::crouch, false);
        game.input(Key::jump, false);
        run(game, 10);
        CHECK(!game.player.crouched);
        game.input(Key::crouch, true);
        run(game, 20);
        CHECK(game.player.crouched);
    }
}
}

void testEnemyGapSafety() {
    Stage stage{};
    stage.ground={{0,500,220,90},{340,500,310,90}};
    stage.ledges={{670,360,145,17}};
    Enemy enemy{};
    enemy.box={125,446,35,54}; enemy.base=500;
    CHECK(enemyHasFooting(stage,enemy,125,1));
    float at=125;
    for (int i=0;i<160;++i) {
        const float next=groundedEnemyX(stage,enemy,at,at+12.f);
        CHECK(next>=at && next<=at+12.001f);
        CHECK(enemyHasFooting(stage,enemy,next,1));
        at=next;
    }
    CHECK(at<220.f-enemy.box.w*.5f);
    // Reversing must work, while a fast step may not skip the empty gap.
    const float reversed=groundedEnemyX(stage,enemy,at,at-20.f);
    CHECK(reversed<at);
    CHECK(groundedEnemyX(stage,enemy,140,400)<220.f);
    // An enemy already stranded outside support never advances through air.
    CHECK(groundedEnemyX(stage,enemy,264,300)==264.f);
    // Raised deck support is valid at the enemy's own elevation.
    enemy.box={690,306,35,54}; enemy.base=360;
    CHECK(groundedEnemyX(stage,enemy,690,704)>690.f);
    // All generated grounded enemies must start on valid terrain; the edge
    // guard should never freeze them because of a misplaced spawn.
    for (const auto& sector:stages)
        for (const auto& placement:sector.enemies) {
            if (placement.kind==EnemyKind::drone || placement.kind==EnemyKind::interceptor ||
                placement.kind==EnemyKind::turret) continue;
            enemy.base=placement.y;
            const float w=spec(placement.kind).w;
            enemy.box={placement.x-w*.5f,placement.y-spec(placement.kind).h,w,spec(placement.kind).h};
            CHECK(enemyHasFooting(sector,enemy,enemy.box.x,1) ||
                  enemyHasFooting(sector,enemy,enemy.box.x,-1));
        }
}


namespace {
void cameraFollowsCursorButNeverLosesPlayer() {
    Silent audio;
    Game game(audio);
    game.start(0,startSave());
    game.entry=0;
    Stage flat=*game.stage;
    flat.worldWidth=5000;
    flat.ground={{0,500,5000,110}};
    flat.ledges.clear();flat.stairs.clear();flat.enemies.clear();flat.supplies.clear();
    flat.exit={4950,500};
    game.stage=&flat;
    game.platforms={{{0,500,5000,110},{},true}};
    game.enemies.clear(); game.supplies.clear();
    auto& p=game.player;
    p.box={2050,500-p.box.h,p.box.w,p.box.h};
    p.support=0;p.safe={2050,500};p.velocity={};
    game.camera=1400;game.cameraY=0;
    game.pointerActive=false;
    run(game,200);
    const float centered=game.camera;
    const float centeredY=game.cameraY;
    const auto screenCenter=[&] {
        return Vec{p.box.center().x-width*.5f,p.box.center().y-height*.5f};
    };
    CHECK(std::abs(centered-screenCenter().x)<2.f);
    CHECK(std::abs(centeredY-screenCenter().y)<2.f);
    // Moving the cursor through the central 80% cannot shift either axis.
    game.pointAt({p.box.center().x+900,p.box.center().y},true);
    run(game,120);
    CHECK(std::abs(game.camera-centered)<2.f);
    CHECK(std::abs(game.cameraY-centeredY)<2.f);
    // At the top margin, both axes look ahead smoothly instead of snapping.
    game.pointAt({p.box.center().x+900,game.cameraY-100.f},true);
    game.tick();
    CHECK(game.camera>centered);
    CHECK(game.camera<centered+20.f);
    run(game,65);
    CHECK(game.camera>centered+25.f);
    CHECK(game.cameraY<centeredY-25.f);
    // Leaving the edge smoothly returns to the actual player center.
    const float shiftedX=game.camera,shiftedY=game.cameraY;
    game.pointAt(p.box.center(),true);
    game.tick();
    CHECK(game.camera<shiftedX && game.camera>centered);
    CHECK(game.cameraY>shiftedY && game.cameraY<centeredY);
    run(game,240);
    CHECK(std::abs(game.camera-centered)<3.f);
    CHECK(std::abs(game.cameraY-centeredY)<3.f);
    // Bottom edge shifts down, then recenters when the cursor is neutral.
    game.pointAt({p.box.center().x,game.cameraY+height+100.f},true);
    run(game,65);
    CHECK(game.cameraY>centeredY+15.f);
    game.pointerActive=false;
    run(game,240);
    CHECK(std::abs(game.cameraY-centeredY)<3.f);
    CHECK(p.box.center().x-game.camera>width*.16f);
    CHECK(p.box.center().x-game.camera<width*.84f);
    CHECK(p.box.center().y-game.cameraY>height*.10f);
    CHECK(p.box.center().y-game.cameraY<height*.90f);
    // A jump crossing the upper 10% edge triggers vertical camera tracking.
    game.cameraY=0;
    p.box.y=-52; p.support=-1; p.velocity={};
    game.tick();
    CHECK(game.cameraY<-.1f);
    CHECK(p.box.center().y-game.cameraY>=height*.10f-.1f);
    // A descent past the lower 10% edge tracks downward, too.
    game.cameraY=0;
    p.box.y=height-18; p.support=-1; p.velocity={};
    game.tick();
    CHECK(game.cameraY>0);
    CHECK(p.box.center().y-game.cameraY<=height*.90f+.1f);
    // Restore the original player height for the teleport check.
    p.box.y=500-p.box.h;p.support=0;p.velocity={};
    // A sudden camera jump, teleport or sprint must be clamped back into view
    // in a single simulation tick, rather than drifting after the player.
    game.camera=0;game.cameraY=260;
    game.tick();
    CHECK(p.box.center().x-game.camera<width*.79f);
    CHECK(p.box.center().y-game.cameraY<height*.90f);
}

void stairJoinsRaisedPlatformInBothDirections() {
    Silent audio;
    Game game(audio);
    game.start(0,startSave());
    game.entry=0;
    Stage terrain=*game.stage;
    terrain.ground={{0,500,260,110},{315,420,580,110}};
    terrain.ledges.clear();
    terrain.stairs={{{230,500},{355,420}}};
    terrain.enemies.clear();terrain.supplies.clear();terrain.beacons.clear();
    terrain.worldWidth=1400;terrain.exit={1325,420};
    game.stage=&terrain;
    game.platforms={{{0,500,260,110},{},true},{{315,420,580,110},{},true}};
    game.enemies.clear();game.supplies.clear();
    auto& p=game.player;
    p.box={205,500-p.box.h,p.box.w,p.box.h};
    p.support=0;p.safe={205,500};p.velocity={};
    game.input(Key::right,true);
    run(game,150);
    CHECK(p.box.x>365.f);
    CHECK(std::abs(p.box.bottom()-420.f)<5.f);
    game.input(Key::right,false);
    p.box.x=380;p.box.y=420-p.box.h;p.support=1;p.velocity={};
    game.input(Key::left,true);
    run(game,135);
    CHECK(p.box.x<220.f);
    CHECK(std::abs(p.box.bottom()-500.f)<5.f);
    game.input(Key::left,false);
}
}

void runningStrideFootfallSequence() {
    // At one full run cycle two feet land half a stride apart. There must be
    // an airborne phase between each pair of contacts, not sliding feet.
    constexpr float stance=.38f;
    const auto strike=running::sample(0.f,stance);
    CHECK(strike.planted && strike.pitch>.2f);
    CHECK(running::sample(.32f,stance).planted);
    CHECK(!running::sample(.40f,stance).planted);
    CHECK(running::sample(.68f,stance).clearance>.65f);
    CHECK(running::sample(.68f,stance).progress>running::sample(.40f,stance).progress);
    CHECK(running::sample(.9999f,stance).clearance<.01f);
    CHECK(std::abs(running::sample(1.f,stance).pitch-strike.pitch)<.001f);
    for(float phase: {0.f,.25f,.40f,.50f,.68f,.90f}) {
        const auto far=running::sample(phase,stance);
        const auto near=running::sample(phase+.5f,stance);
        CHECK(running::sample(phase+1.f,stance).planted==far.planted);
        CHECK(std::abs(running::hipOffset(phase)-running::hipOffset(phase+.5f))<.0002f);
        if(phase==.40f || phase==.90f) CHECK(!far.planted && !near.planted);
    }
}

void testHumanLimbKinematics() {
    // Leg lengths must remain physical across large airborne foot offsets,
    // regardless of the direction or scaling of the model.
    for(float scale: {.84f,1.f,1.16f}) {
        const float thigh=15.6f*scale,shin=14.6f*scale;
        const Vec hip{0,-32};
        for(Vec target: {Vec{0,60},Vec{100,30},Vec{-95,-80},Vec{3,-33},Vec{0,-35},Vec{0,0}}) {
            const Vec ankle=reachableAnkle(hip,target,thigh,shin);
            const Vec knee=solveKnee(hip,ankle,thigh,shin,1);
            CHECK(std::abs(length(hip-knee)-thigh)<.01f);
            CHECK(std::abs(length(knee-ankle)-shin)<.01f);
            CHECK(length(ankle-hip)<=thigh+shin);
        }
    }
    // The actual forward foot decides the shoulder pair, independently of
    // running direction: same-side shoulder behind, opposite shoulder ahead.
    for(float axis: {-1.f,1.f}) {
        const float leadNear=running::footLead(14.f*axis,-14.f*axis,axis);
        const float leadFar=running::footLead(-14.f*axis,14.f*axis,axis);
        const auto near=running::counterShoulders(leadNear,4.f);
        const auto far=running::counterShoulders(leadFar,4.f);
        CHECK(leadNear>.99f && leadFar<-.99f);
        CHECK(near.near<0.f && near.far>0.f);
        CHECK(far.near>0.f && far.far<0.f);
        CHECK(std::abs(near.near+near.far)<.00001f);
        CHECK(std::abs(near.near-far.far)<.00001f);
        CHECK(std::abs(near.far-far.near)<.00001f);
        CHECK(running::counterShoulders(0.f,4.f).near==0.f);
        CHECK(running::counterShoulders(leadNear,0.f).near==0.f);
    }
    // Pelvis and shoulders counter-rotate; the other footstep reverses the
    // motion. Hip bob repeats twice in each full two-step cycle.
    for(float phase: {.06f,.13f,.29f,.43f}) {
        CHECK(std::abs(running::girdleTurn(phase)+running::girdleTurn(phase+.5f))<.0001f);
        CHECK(std::abs(running::hipOffset(phase)-running::hipOffset(phase+.5f))<.0002f);
    }
}

namespace {
void ladderTraversalAndDropThrough() {
    Silent audio;
    Game game(audio);
    game.start(0,startSave()); game.entry=0;
    Stage field=*game.stage;
    field.ground={{0,500,720,100}};
    field.ledges={{220,350,170,16},{410,595,190,16}};
    field.ladders={{288,350,500}};
    field.stairs.clear(); field.lifts.clear();
    field.enemies.clear(); field.supplies.clear();
    game.stage=&field;
    game.platforms={{{0,500,720,100},{},true},{{220,350,170,16},{}},{{410,595,190,16},{}}};
    auto& player=game.player;
    player.box={272,444,32,56};player.velocity={};player.support=0;
    game.resetPose();
    game.input(Key::jump,true);
    run(game,8);
    CHECK(player.ladder==0 && player.support==-1);
    const float feet=player.box.bottom();
    run(game,36);
    CHECK(player.box.bottom()<feet-30.f); // climbing, not free-falling
    const int loaded=game.guns[game.gun].ammo;
    game.input(Key::fire,true);
    run(game,12);
    CHECK(game.guns[game.gun].ammo<loaded); // upper body can shoot on rungs
    CHECK(player.ladder==0);
    game.input(Key::fire,false);
    run(game,115);
    CHECK(player.ladder==-1 && player.support==1);
    CHECK(std::abs(player.box.bottom()-350.f)<2.f);
    game.input(Key::jump,false);
    game.input(Key::crouch,true);
    run(game,18);
    CHECK(player.ladder==0 && player.dropThrough>0.f);
    CHECK(player.box.bottom()>366.f); // controlled descent through top deck
    run(game,155);
    CHECK(player.ladder==-1 && player.support==0); // steps off at bottom
    CHECK(std::abs(player.box.bottom()-500.f)<2.f);
    game.input(Key::crouch,false);
    // Re-enter the ladder by falling down its face. A falling operative is
    // caught by the rungs rather than passing through the open ladder path.
    player.box={272,301,32,56}; player.velocity={0,180}; player.support=-1;
    player.dropThrough=0;player.dropPlatform=-1;
    run(game,3);
    CHECK(player.ladder==0);
    game.input(Key::crouch,true);
    const float descendFeet=player.box.bottom();
    run(game,3);
    CHECK(player.ladder==0 && player.velocity.y==0.f);
    CHECK(player.box.bottom()>descendFeet);
}
}

namespace {
void airborneMomentumAndStairDrop() {
    Silent audio;
    Game game(audio);
    game.start(0,startSave()); game.entry=0;
    Stage field=*game.stage;
    field.ground={{0,500,850,100}};
    field.ledges.clear();field.stairs.clear();field.ladders.clear();
    field.lifts.clear();field.enemies.clear();field.supplies.clear();
    game.stage=&field;
    game.platforms={{{0,500,850,100},{},true}};
    game.player.box={190,444,32,56};game.player.velocity={};game.player.support=0;
    game.resetPose();
    game.input(Key::right,true);
    run(game,15);
    game.input(Key::jump,true);
    run(game,9);
    CHECK(game.player.support<0 && game.player.velocity.y<0);
    const float takeoffV=game.player.velocity.x;
    CHECK(takeoffV>40);
    game.input(Key::right,false);game.input(Key::left,true);
    run(game,22);
    // Reversing direction in the air must not reverse momentum.
    CHECK(game.player.velocity.x>takeoffV-3.0f);
    game.input(Key::jump,false);
    game.input(Key::jump,true); // existing midair flip adds vertical impulse only
    const float beforeFlip=game.player.velocity.x;
    run(game,3);
    CHECK(game.player.velocity.x>beforeFlip-1.f);

    field.stairs={{{230,500},{355,420}}};
    game.player.box={284,stairY(field.stairs[0],300)-56,32,56};
    game.player.velocity={};game.player.support=-1;
    game.player.jumpAge=0;game.player.dropThrough=0;
    game.input(Key::left,false);game.input(Key::jump,false);
    game.input(Key::crouch,true);
    const float startY=game.player.box.bottom();
    run(game,10);
    CHECK(game.player.dropThrough>0.f);
    CHECK(game.player.box.bottom()>startY+5.f);
    CHECK(game.player.ladder<0);
}

void clearExtractionGates() {
    for(const Stage& field:stages)
        for(const Stair& stair:field.stairs)
            CHECK(!exitStairConflict(stair,field.exit));
    for(const Stage& field:cooperativeStages)
        for(const Stair& stair:field.stairs)
            CHECK(!exitStairConflict(stair,field.exit));
}
} // namespace

void runMovementTests() {
    bossAggression();
    bossArenaPursuit();
    // Game action sampling uses the very same five editable poses that the
    // editor previews and the JSON export writes. No hidden pose lookup.
    SkeletonProfile authored{};
    const auto knee=static_cast<size_t>(SkeletonJoint::nearKnee);
    authored.actionFrames[static_cast<size_t>(ActionPose::jump)][0][knee]={0.f,0.f};
    authored.actionFrames[static_cast<size_t>(ActionPose::jump)][1][knee]={0.f,-4.f};
    authored.actionFrames[static_cast<size_t>(ActionPose::jump)][2][knee]={2.f,-6.f};
    authored.actionFrames[static_cast<size_t>(ActionPose::jump)][3][knee]={1.f,-3.f};
    authored.actionFrames[static_cast<size_t>(ActionPose::jump)][4][knee]={0.f,0.f};
    const auto first=skeletonOffset(authored,SkeletonJoint::nearKnee,0,0,ActionPose::jump,1.f,0.f);
    const auto quarter=skeletonOffset(authored,SkeletonJoint::nearKnee,0,0,ActionPose::jump,1.f,.25f);
    const auto middle=skeletonOffset(authored,SkeletonJoint::nearKnee,0,0,ActionPose::jump,1.f,.5f);
    const auto eighth=skeletonOffset(authored,SkeletonJoint::nearKnee,0,0,ActionPose::jump,1.f,.375f);
    CHECK(first.y==0.f && quarter.y==-4.f && middle.y==-6.f);
    CHECK(eighth.x==1.f && eighth.y==-5.f);
    CHECK(skeletonOffset(authored,SkeletonJoint::nearKnee,0,0,ActionPose::jump,.5f,.5f).y==-3.f);
    testHumanLimbKinematics();
    ladderTraversalAndDropThrough();
    airborneMomentumAndStairDrop();
    clearExtractionGates();
    runningStrideFootfallSequence();
    testEnemyGapSafety();
    cameraFollowsCursorButNeverLosesPlayer();
    stairJoinsRaisedPlatformInBothDirections();
    chordsDriveTheGame();
    airFlip();
    deterministic();
    chordWithCrouch();
}
