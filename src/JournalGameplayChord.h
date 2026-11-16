#pragma once
#include <cstdint>
#include <string_view>
#include <charconv>

namespace K2040::JournalGameplayChord {
struct Chord { std::uint32_t key{},modifiers{}; };
inline bool Parse(std::string_view input,Chord& result) noexcept {
    result={};
    if(input=="Unbound")return true;
    if(input.empty()||input.size()>32)return false;
    std::uint32_t modifiers=0,key=0;
    unsigned previousMod=0;
    while(true){
        const auto split=input.find('+');
        const auto part=input.substr(0,split);
        if(part=="Ctrl"||part=="Shift"||part=="Alt"){
            if(split==std::string_view::npos || key)return false;
            const unsigned order=part=="Ctrl"?1:part=="Shift"?2:3;
            if(order<=previousMod)return false;
            previousMod=order;
            modifiers|=part=="Ctrl"?2u:part=="Shift"?1u:4u;
        }else{
            if(split!=std::string_view::npos)return false;
            if(part.size()==1 &&
               ((part[0]>='A'&&part[0]<='Z')||(part[0]>='0'&&part[0]<='9')))
                key=static_cast<unsigned>(part[0]);
            else if(part=="Mouse3")key=258;
            else if(part=="Mouse4")key=259;
            else if(part=="Mouse5")key=260;
            else if(part.size()>1&&part[0]=='F'){
                unsigned num=0;
                const auto digits=part.substr(1);
                auto [end,error]=std::from_chars(digits.data(),digits.data()+digits.size(),num);
                if(error!=std::errc{}||end!=digits.data()+digits.size()||
                   num<1||num>12)return false;
                key=111u+num;
            }
            if(!key)return false;
        }
        if(split==std::string_view::npos)break;
        input.remove_prefix(split+1);
        if(input.empty())return false;
    }
    if(key==114u&&modifiers==0)return false; // F3 belongs to unified hub.
    result={key,modifiers};
    return true;
}
inline std::uint32_t Packed(std::string_view input) noexcept {
    Chord data{};
    if(!Parse(input,data))return 0;
    return data.key|(data.modifiers<<9);
}
inline bool Valid(std::string_view input) noexcept {
    Chord data{};return Parse(input,data);
}
}
