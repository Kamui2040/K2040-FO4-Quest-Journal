#pragma once
// Pure, anonymous, selected-object GFx metadata census. No values,
// object identities, runtime pointers, dynamically named fields or writes.
#include <cstdint>
#include <cstring>
namespace K2040::PipboySelectedEntryResearch {
struct Schema {
    bool available{};
    bool overflow{};
    std::uint8_t total{}, integer{}, number{}, boolean{}, string{}, object{}, array{}, other{};
    // Only compile-time names in the whitelist below; never raw names.
    std::uint16_t nameMask{}, numericNameMask{};
    // Named numeric keys 0/1/2 are probed as schema shapes, NOT ABI proof.
    std::uint8_t ordinalMask{}, numericOrdinalMask{};
    bool nativeCommandTripleVerified{}; // must remain false
    bool nativeWriteAuthorized{};        // must remain false
};
inline constexpr const char* kAllowedNames[] = {
    "formID", "instance", "type", "questType", "active", "enabled",
    "objective", "index", "id", "questID", "questFormID", "instanceID",
    "category", "sortval", "text", "description"
};
static_assert(sizeof(kAllowedNames)/sizeof(kAllowedNames[0])==16);
template<class Value>
[[nodiscard]] Schema SummarizeSchema(const Value& selected) {
    Schema s{};
    if(!selected.IsObject())return s;
    s.available=true;
    selected.VisitMembers([&](const char* name, const Value& value) {
        if(s.overflow)return;
        if(s.total>=48){s.overflow=true;return;}
        ++s.total;
        if(value.IsInt() || value.IsUInt())++s.integer;
        else if(value.IsNumber())++s.number;
        else if(value.IsBoolean())++s.boolean;
        else if(value.IsString())++s.string;
        else if(value.IsArray())++s.array;
        else if(value.IsObject())++s.object;
        else ++s.other;
        if(!name)return;
        const bool numeric=value.IsInt()||value.IsUInt()||value.IsNumber();
        for(std::uint8_t i=0;i<16;++i){
            if(std::strcmp(name,kAllowedNames[i])==0){
                s.nameMask|=static_cast<std::uint16_t>(1u<<i);
                if(numeric)s.numericNameMask|=static_cast<std::uint16_t>(1u<<i);
                break;
            }
        }
        if(name[0]>='0' && name[0]<='2' && name[1]=='\0')
        {
            s.ordinalMask|=static_cast<std::uint8_t>(1u<<(name[0]-'0'));
            if(numeric)s.numericOrdinalMask|=static_cast<std::uint8_t>(1u<<(name[0]-'0'));
        }
    });
    // An oversized/unbounded schema is deliberately never accepted.
    if(s.overflow){s.nameMask=s.numericNameMask=0;s.ordinalMask=s.numericOrdinalMask=0;}
    return s;
}
[[nodiscard]] Schema ObserveOpenPipboySchemaOnUIThread();
} // namespace K2040::PipboySelectedEntryResearch
