#pragma once
#include "content/content.hpp"

namespace vl {
// Debug loadouts are session-only. They are never written to a campaign slot.
inline Save makeDebugLoadout(const Save& saved) {
    Save result=saved;
    result.owned.fill(true);
    return result;
}
inline constexpr int nextDebugRank(int current,int maximum) {
    return current>=maximum?0:current+1;
}
}
