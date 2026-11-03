#include "../research/native_pipboy_selection/ReadOnlyCodeObjectBinding.h"
#include "../src/QuestTrackingPolicy.h"
#include <cassert>
#include <map>
#include <string>
using namespace K2040::PipboySelectedEntryResearch;
struct Value {
    enum class ValueType{ kUndefined,kNull,kBoolean,kInt,kUInt,kNumber,
        kString,kStringW,kObject,kArray,kDisplayObject,kClosure,kUnknown };
    ValueType type{ValueType::kUndefined};
    std::map<std::string,Value> members;
    bool IsObject()const{return type==ValueType::kObject;}
    ValueType GetType()const{return type;}
    bool GetMember(const char* name,Value* out)const {
        auto it=members.find(name);
        if(it==members.end())return false;
        *out=it->second;
        return true;
    }
};
struct Movie {
    Value root;
    bool available{true};
    bool GetVariable(Value* out,const char* path) const {
        if(!available || std::string(path)!="root.Menu_mc")return false;
        *out=root;
        return true;
    }
};
int main() {
    Movie movie{};
    auto check=[&](BindingState expected,
                   BindingMemberType type=BindingMemberType::kNotObserved) {
        auto result=InspectCodeObjectBinding<Movie,Value>(movie);
        assert(result.state==expected);
        assert(result.memberType==type);
        assert(!result.nativeCallbackAbiVerified);
        assert(!result.nativeWriteAuthorized);
    };
    check(BindingState::kMenuUnavailable);
    movie.root.type=Value::ValueType::kObject;
    check(BindingState::kCodeObjectUnavailable);
    movie.root.members["BGSCodeObj"]={Value::ValueType::kNumber,{}};
    check(BindingState::kCodeObjectNotObject);
    movie.root.members["BGSCodeObj"].type=Value::ValueType::kObject;
    check(BindingState::kSetQuestActiveUnavailable);
    movie.root.members["BGSCodeObj"].members["SetQuestActive"]={
        Value::ValueType::kNumber,{}};
    check(BindingState::kSetQuestActiveNotClosure, BindingMemberType::kNumber);
    using V=Value::ValueType;
    using M=BindingMemberType;
    for(const auto [actual,expected]: {
        std::pair{V::kUndefined,M::kUndefined},
        std::pair{V::kNull,M::kNull},
        std::pair{V::kBoolean,M::kBoolean},
        std::pair{V::kInt,M::kInt},
        std::pair{V::kUInt,M::kUInt},
        std::pair{V::kNumber,M::kNumber},
        std::pair{V::kString,M::kString},
        std::pair{V::kStringW,M::kStringW},
        std::pair{V::kObject,M::kObject},
        std::pair{V::kArray,M::kArray},
        std::pair{V::kDisplayObject,M::kDisplayObject},
        std::pair{V::kClosure,M::kClosure},
        std::pair{V::kUnknown,M::kUnknown}}) {
        movie.root.members["BGSCodeObj"].members["SetQuestActive"].type=actual;
        check(actual==V::kClosure
                  ? BindingState::kSetQuestActiveClosure
                  : BindingState::kSetQuestActiveNotClosure, expected);
        assert(std::string(BindingMemberTypeName(expected)).size()>0);
    }
    movie.available=false;
    check(BindingState::kMenuUnavailable);
    static_assert(!K2040::QuestTracking::CanWritePipboy());
}
