// Movement, jumping, flipping and determinism.
#include "support.hpp"
#include "game/enemy_ground.hpp"
#include "game/detail.hpp"

using namespace test;
using namespace vl;

namespace {
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

void runMovementTests() {
    testEnemyGapSafety();
    cameraFollowsCursorButNeverLosesPlayer();
    stairJoinsRaisedPlatformInBothDirections();
    chordsDriveTheGame();
    airFlip();
    deterministic();
    chordWithCrouch();
}
