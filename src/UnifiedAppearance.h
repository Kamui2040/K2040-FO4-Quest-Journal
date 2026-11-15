#pragma once
// Validate read-only hub presentation data before passing it to PrismaUI.
// User preferences remain exclusively owned by the Quest Journal.
#include "JsonPreferencesParser.h"
#include <array>
#include <algorithm>
#include <optional>
#include <string>
#include <string_view>

namespace K2040::UnifiedAppearance {
inline bool Hex(std::string_view color) {
    if(color.size()!=7 || color[0]!='#')return false;
    return std::all_of(color.begin()+1,color.end(),[](char c){
        return (c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F');
    });
}
inline const std::string* StringField(const k2040::json::JsonObject& object,
                                      std::string_view key) {
    const auto item=object.find(std::string(key));
    return item==object.end()?nullptr:item->second.AsString();
}
inline bool KnownTheme(std::string_view theme) {
    return theme=="fallout4"||theme=="fallout76"||
        theme=="doom-eternal"||theme=="fallout3-newvegas";
}
inline bool KnownPalette(std::string_view palette) {
    if(palette=="default"||palette=="fallout76"||palette=="radioactive-green"||
       palette=="monochrome"||palette=="midnight-blue"||palette=="hot-omni")
        return true;
    if(!palette.starts_with("user-")||palette.size()<6||palette.size()>24)
        return false;
    return std::all_of(palette.begin()+5,palette.end(),[](char c){
        return c>='0'&&c<='9';
    });
}
inline std::optional<std::string> Validated(std::string_view raw) {
    if(raw.size()<20||raw.size()>4095)return std::nullopt;
    const auto parsed=k2040::json::JsonParser(std::string(raw)).Parse();
    if(!parsed||!parsed->AsObject())return std::nullopt;
    const auto& object=*parsed->AsObject();
    const auto schema=object.find("schema");
    if(schema==object.end()||!schema->second.AsNumber()||
       *schema->second.AsNumber()!=1.0)return std::nullopt;
    const auto* theme=StringField(object,"theme");
    const auto* palette=StringField(object,"palette");
    if(!theme||!KnownTheme(*theme)||!palette||!KnownPalette(*palette))
        return std::nullopt;
    const auto colorsIt=object.find("colors");
    if(colorsIt==object.end()||!colorsIt->second.AsObject())
        return std::nullopt;
    const auto& colors=*colorsIt->second.AsObject();
    constexpr std::array<std::string_view,7> roles{
        "background","panel","text","accent","muted","border","highlight"
    };
    for(const auto role:roles){
        const auto* color=StringField(colors,role);
        if(!color||!Hex(*color))return std::nullopt;
    }
    return std::string(raw);
}
}
