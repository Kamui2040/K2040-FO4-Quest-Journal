#include "../research/native_pipboy_selection/ReadOnlySelectedSchema.h"
#include "../src/QuestTrackingPolicy.h"
#include <cassert>
#include <map>
#include <string>
#include <cstdint>
using namespace K2040::PipboySelectedEntryResearch;
struct Value {
    enum class Type{obj,i,u,n,b,s,a,unknown};
    Type type{Type::unknown};
    std::map<std::string,Value> members;
    bool IsObject()const{return type==Type::obj;}
    bool IsInt()const{return type==Type::i;}
    bool IsUInt()const{return type==Type::u;}
    bool IsNumber()const{return type==Type::n;}
    bool IsBoolean()const{return type==Type::b;}
    bool IsString()const{return type==Type::s;}
    bool IsArray()const{return type==Type::a;}
    template<class Fn> void VisitMembers(Fn&& visit)const {
        for(const auto& [key,value]:members)visit(key.c_str(),value);
    }
};
int main(){
    Value value{Value::Type::obj,{}};
    value.members["formID"]={Value::Type::u,{}};
    value.members["instance"]={Value::Type::i,{}};
    value.members["0"]={Value::Type::u,{}};
    value.members["1"]={Value::Type::n,{}};
    value.members["2"]={Value::Type::b,{}};
    value.members["private-quest-value"]={Value::Type::s,{}};
    const auto s=SummarizeSchema(value);
    assert(s.available && !s.overflow && s.total==6);
    assert(s.integer==3 && s.number==1 && s.boolean==1 && s.string==1);
    assert(s.nameMask==3 && s.numericNameMask==3 &&
           s.ordinalMask==7 && s.numericOrdinalMask==3);
    assert(!s.nativeCommandTripleVerified && !s.nativeWriteAuthorized);
    for(int i=0;i<49;++i)value.members["unlisted"+std::to_string(i)]={Value::Type::s,{}};
    const auto capped=SummarizeSchema(value);
    assert(capped.overflow && capped.total==48 && capped.nameMask==0 &&
           capped.numericNameMask==0 && capped.ordinalMask==0 &&
           capped.numericOrdinalMask==0);
    assert(!capped.nativeCommandTripleVerified && !capped.nativeWriteAuthorized);
    assert(!SummarizeSchema(Value{}).available);
    static_assert(!K2040::QuestTracking::CanWritePipboy());
}
