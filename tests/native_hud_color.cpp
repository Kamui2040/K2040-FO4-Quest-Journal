#include "HudColor.h"
#include <cassert>
#include <limits>
int main() {
    using K2040::HudColor::ToCss;
    assert(ToCss(0,0,0)=="#000000");
    assert(ToCss(1,1,1)=="#ffffff");
    assert(ToCss(1,0.5F,0)=="#ff8000");
    assert(ToCss(0,1,0)=="#00ff00");
    assert(ToCss(-1,2,0.25F)=="#00ff40");
    assert(ToCss(std::numeric_limits<float>::quiet_NaN(),0,0)=="#000000");
    return 0;
}
