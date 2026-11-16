#pragma once
#include "content/weapons.hpp"

namespace vl {
// Game inventory IDs stay stable for saves/networking. Slots and cycle order
// refer exclusively to guns currently visible in the on-screen quick bar.
template <class Guns>
int visibleWeaponAt(const Guns& guns,int slot) {
    if (slot<0) return -1;
    for (const int id:weaponDisplayOrder) {
        if (!guns[id].available) continue;
        if (slot--==0) return id;
    }
    return -1;
}

template <class Guns>
int nextVisibleWeapon(const Guns& guns,int current,int direction) {
    if (!direction) return current;
    int count=0,position=-1;
    for (const int id:weaponDisplayOrder) {
        if (!guns[id].available) continue;
        if (id==current) position=count;
        ++count;
    }
    if (count==0) return -1;
    if (position<0) return visibleWeaponAt(guns,0);
    const int slot=(position+(direction>0?1:count-1))%count;
    return visibleWeaponAt(guns,slot);
}
} // namespace vl
