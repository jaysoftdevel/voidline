#pragma once

namespace vl {
// Pixel-space placement for a label OUTSIDE the 2:1 game viewport. The
// viewport is centered in the drawable; on a full 2:1 window there is no
// unused border, and the caller should use its usual in-game fallback.
struct FpsBorderPlacement {
    int x=0, y=0; // top-left in drawable pixels
    bool outside=false;
};

inline FpsBorderPlacement fpsBorderPlacement(int pixelW,int pixelH,
        int viewX,int viewY,int viewW,int viewH,int corner,
        int panelW,int panelH,int margin) {
    if (pixelW<=0 || pixelH<=0 || viewW<=0 || viewH<=0 || panelW<=0 || panelH<=0)
        return {};
    const bool left=corner==0 || corner==2;
    const bool top=corner==0 || corner==1;
    const int viewTop=pixelH-viewY-viewH;
    const int upper=viewTop,lower=viewY;
    const int side=left?viewX:pixelW-viewX-viewW;
    // The preferred corner's horizontal letterbox is adjacent to the image.
    if ((top?upper:lower)>=panelH+margin && viewW>=panelW+2*margin) {
        return {left?viewX+margin:viewX+viewW-panelW-margin,
                top?viewTop-panelH-margin:viewTop+viewH+margin,true};
    }
    // On ultrawide displays the bars are at the sides instead.
    if (side>=panelW+margin && viewH>=panelH+2*margin) {
        return {left?viewX-panelW-margin:viewX+viewW+margin,
                top?viewTop+margin:viewTop+viewH-panelH-margin,true};
    }
    return {};
}
} // namespace vl
