#pragma once
// Pure geometry policy, shared by owner callbacks and standalone native tests.
// The hub and this mod both reject offscreen and non-finite layouts.
#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace K2040::TrackerLayout {
struct Geometry {
    double x{},y{},width{},height{};
};
inline bool Valid(Geometry g) noexcept {
    return std::isfinite(g.x)&&std::isfinite(g.y)&&
        std::isfinite(g.width)&&std::isfinite(g.height)&&
        g.x>=0&&g.y>=0&&g.width>0.015&&g.height>0.015&&
        g.x+g.width<=1.0000001&&g.y+g.height<=1.0000001;
}
inline bool Proportional(Geometry base,Geometry draft) noexcept {
    if(!Valid(base)||!Valid(draft))return false;
    // Both percentages must scale together: no independent aspect stretching.
    const double x=draft.width/base.width,y=draft.height/base.height;
    return std::isfinite(x)&&std::isfinite(y)&&
        std::abs(x-y)<=0.025*std::max(x,y);
}
inline int Basis(double value) noexcept {
    return static_cast<int>(std::lround(value*10000.0));
}
inline Geometry FromBasis(int x,int y,int w,int h) noexcept {
    return {x/10000.0,y/10000.0,w/10000.0,h/10000.0};
}
inline bool ValidBasis(int x,int y,int w,int h) noexcept {
    return x>=0&&y>=0&&w>150&&h>150&&w<=10000&&h<=10000&&
        x+w<=10000&&y+h<=10000;
}
inline std::string Number(double value) {
    char buffer[48]{};
    const auto result=std::to_chars(buffer,buffer+sizeof(buffer),
                                     value,std::chars_format::fixed,6);
    return result.ec==std::errc{}?std::string(buffer,result.ptr):std::string("0");
}
inline std::string ToJson(Geometry g) {
    return "{\"x\":"+Number(g.x)+",\"y\":"+Number(g.y)+
        ",\"width\":"+Number(g.width)+",\"height\":"+Number(g.height)+"}";
}
inline std::string ElementsJson(Geometry g) {
    return "{\"schema\":1,\"elements\":[{\"id\":\"quest-tracker\","
           "\"label\":\"Quest tracker\",\"x\":"+Number(g.x)+
           ",\"y\":"+Number(g.y)+",\"width\":"+Number(g.width)+
           ",\"height\":"+Number(g.height)+"}]}";
}
}
