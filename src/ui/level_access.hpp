#pragma once
#include "content/types.hpp"

namespace vl {
// Debug mission access is a presentation/selection override, never persisted in Save.
inline bool canDeployLevel(int index,int cleared,bool debugUnlocked,int levelCount=sectorCount) {
    return index>=0 && index<levelCount && (debugUnlocked || index<=cleared);
}

// Playing ahead in debug mode cannot mark unplayed earlier sectors as completed.
inline int progressAfterCompletion(int cleared,int completedLevel) {
    return completedLevel<=cleared ? std::max(cleared,completedLevel+1) : cleared;
}
} // namespace vl
