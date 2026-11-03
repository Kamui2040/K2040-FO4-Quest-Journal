#pragma once
// Read-only, single known-property GFx census of an engine-supplied
// vanilla PipboyMenu code object. Never invoke code, enumerate functions,
// inspect pointers or log selected quest data. Default-off QA ONLY.
#include <cstdint>

namespace K2040::PipboySelectedEntryResearch {
enum class BindingState : std::uint8_t {
    kMenuUnavailable,
    kCodeObjectUnavailable,
    kCodeObjectNotObject,
    kSetQuestActiveUnavailable,
    kSetQuestActiveNotClosure,
    kSetQuestActiveClosure
};
// Fixed GFx value type census: no property values or pointers retained.
enum class BindingMemberType : std::uint8_t {
    kNotObserved,kUndefined,kNull,kBoolean,kInt,kUInt,kNumber,
    kString,kStringW,kObject,kArray,kDisplayObject,kClosure,kUnknown
};
[[nodiscard]] constexpr const char* BindingMemberTypeName(
        BindingMemberType t) noexcept {
    switch(t) {
    case BindingMemberType::kNotObserved:return "not-observed";
    case BindingMemberType::kUndefined:return "undefined";
    case BindingMemberType::kNull:return "null";
    case BindingMemberType::kBoolean:return "boolean";
    case BindingMemberType::kInt:return "int";
    case BindingMemberType::kUInt:return "uint";
    case BindingMemberType::kNumber:return "number";
    case BindingMemberType::kString:return "string";
    case BindingMemberType::kStringW:return "stringw";
    case BindingMemberType::kObject:return "object";
    case BindingMemberType::kArray:return "array";
    case BindingMemberType::kDisplayObject:return "display-object";
    case BindingMemberType::kClosure:return "closure";
    case BindingMemberType::kUnknown:return "unknown";
    }
    return "unknown";
}
template<class Value>
[[nodiscard]] constexpr BindingMemberType ClassifyMemberType(
        typename Value::ValueType t) noexcept {
    switch(t) {
    case Value::ValueType::kUndefined:return BindingMemberType::kUndefined;
    case Value::ValueType::kNull:return BindingMemberType::kNull;
    case Value::ValueType::kBoolean:return BindingMemberType::kBoolean;
    case Value::ValueType::kInt:return BindingMemberType::kInt;
    case Value::ValueType::kUInt:return BindingMemberType::kUInt;
    case Value::ValueType::kNumber:return BindingMemberType::kNumber;
    case Value::ValueType::kString:return BindingMemberType::kString;
    case Value::ValueType::kStringW:return BindingMemberType::kStringW;
    case Value::ValueType::kObject:return BindingMemberType::kObject;
    case Value::ValueType::kArray:return BindingMemberType::kArray;
    case Value::ValueType::kDisplayObject:return BindingMemberType::kDisplayObject;
    case Value::ValueType::kClosure:return BindingMemberType::kClosure;
    default:return BindingMemberType::kUnknown;
    }
}
struct CodeObjectBinding {
    BindingState state{BindingState::kMenuUnavailable};
    BindingMemberType memberType{BindingMemberType::kNotObserved};
    bool nativeCallbackAbiVerified{}; // MUST remain false
    bool nativeWriteAuthorized{};     // MUST remain false
};
template<class Movie, class Value>
[[nodiscard]] CodeObjectBinding InspectCodeObjectBinding(const Movie& movie) {
    CodeObjectBinding out{};
    Value root, codeObj, member;
    if (!movie.GetVariable(&root, "root.Menu_mc") || !root.IsObject())
        return out;
    out.state=BindingState::kCodeObjectUnavailable;
    if (!root.GetMember("BGSCodeObj", &codeObj))
        return out;
    out.state=BindingState::kCodeObjectNotObject;
    if (!codeObj.IsObject())
        return out;
    out.state=BindingState::kSetQuestActiveUnavailable;
    if (!codeObj.GetMember("SetQuestActive", &member))
        return out;
    // Scaleform's kClosure value represents a callable GFx function,
    // but the type alone does not establish which native callback backs it.
    out.memberType=ClassifyMemberType<Value>(member.GetType());
    out.state=out.memberType==BindingMemberType::kClosure
        ? BindingState::kSetQuestActiveClosure
        : BindingState::kSetQuestActiveNotClosure;
    return out;
}
[[nodiscard]] constexpr const char* BindingStateName(BindingState state) {
    switch (state) {
    case BindingState::kMenuUnavailable:return "menu-unavailable";
    case BindingState::kCodeObjectUnavailable:return "code-object-unavailable";
    case BindingState::kCodeObjectNotObject:return "code-object-wrong-type";
    case BindingState::kSetQuestActiveUnavailable:return "setquestactive-absent";
    case BindingState::kSetQuestActiveNotClosure:return "setquestactive-not-closure";
    case BindingState::kSetQuestActiveClosure:return "setquestactive-closure";
    }
    return "unknown";
}
[[nodiscard]] CodeObjectBinding ObserveOpenPipboyCodeObjectOnUIThread();
}
