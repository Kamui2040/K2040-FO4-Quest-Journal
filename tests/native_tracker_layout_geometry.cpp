#include "TrackerLayoutGeometry.h"
#include "K2040LayoutAdapterV2.h"
#include <cassert>
#include <cmath>
#include <limits>
#include <string>
int main() {
    using namespace K2040::TrackerLayout;
    static_assert(K2040_LAYOUT_ADAPTER_ABI_V2==2);
    static_assert(sizeof(K2040LayoutAdapterV2)>sizeof(void*)*5);
    const Geometry base{0.70,0.10,0.20,0.15};
    assert(Valid(base));
    assert(Proportional(base,base));
    assert(Proportional(base,{0.68,0.07,0.22,0.165}));
    assert(!Proportional(base,{0.70,0.10,0.21,0.15}));
    assert(!Valid({0.90,0.10,0.20,0.15}));
    assert(!Valid({0.70,0.90,0.20,0.20}));
    assert(!Valid({0.70,0.10,0.01,0.15}));
    assert(!Valid({0.70,0.10,std::numeric_limits<double>::infinity(),0.15}));
    assert(!Valid({0.70,0.10,std::numeric_limits<double>::quiet_NaN(),0.15}));
    assert(ValidBasis(7000,1000,2000,1500));
    assert(!ValidBasis(9000,1000,2000,1500));
    assert(!ValidBasis(0,0,150,1500));
    assert(Valid(FromBasis(7000,1000,2000,1500)));
    assert(Basis(0.1751)==1751);
    assert(ToJson(base).find("\"width\":0.200000")!=std::string::npos);
    const auto snapshot=ElementsJson(base);
    assert(snapshot.find("\"schema\":1")!=std::string::npos);
    assert(snapshot.find("\"id\":\"quest-tracker\"")!=std::string::npos);
    assert(snapshot.find("\"height\":0.150000")!=std::string::npos);
}
