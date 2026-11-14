#include "render/renderer.hpp"

namespace vl {
namespace {
Rect blendRect(Rect from,Rect to,float fraction) {
    return {mix(from.x,to.x,fraction),mix(from.y,to.y,fraction),mix(from.w,to.w,fraction),mix(from.h,to.h,fraction)};
}
}

void WorldMotion::capture(const Game& game) {
    enemies.clear(); platforms.clear(); shots.clear(); hostile.clear();
    for (const auto& enemy:game.enemies) enemies.push_back(enemy.box);
    for (const auto& platform:game.platforms) platforms.push_back(platform.box);
    for (const auto& shot:game.shots) shots.emplace_back(shot.id,shot.p);
    for (const auto& shot:game.hostile) hostile.emplace_back(shot.id,shot.p);
}

Rect Renderer::platformBox(const Game& game,size_t index) const {
    const auto& p=game.platforms[index];
    if (!p.lift || !priorWorld || index>=priorWorld->platforms.size()) return p.box;
    return blendRect(priorWorld->platforms[index],p.box,worldBlend);
}

Rect Renderer::enemyBox(const Game& game,size_t index) const {
    const auto& box=game.enemies[index].box;
    if (!priorWorld || index>=priorWorld->enemies.size()) return box;
    const auto& prior=priorWorld->enemies[index];
    if (length(prior.center()-box.center())>72) return box;
    return blendRect(prior,box,worldBlend);
}

Vec Renderer::shotPosition(const Shot& shot,bool friendly) const {
    if (!priorWorld || !shot.id) return shot.p;
    const auto& list=friendly?priorWorld->shots:priorWorld->hostile;
    const auto it=std::lower_bound(list.begin(),list.end(),shot.id,[](const auto& item,uint32_t id) {
        return item.first<id;
    });
    return it!=list.end() && it->first==shot.id?mix(it->second,shot.p,worldBlend):shot.p;
}
}
