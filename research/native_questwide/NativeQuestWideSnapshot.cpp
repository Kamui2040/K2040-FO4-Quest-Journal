// Non-shipping, UNWIRED native read-only adapter. No quest writes.
#if !defined(K2040_PIPBOY_SELECTION_READONLY_QA) || \
    !defined(K2040_PIPBOY_SELECTION_SCHEMA_READONLY_QA)
#error Native quest-wide snapshot adapter is isolated read-only QA only
#endif
#include <RE/Fallout.h>
#include <RE/P/PipboyDataManager.h>
#include "NativeQuestWideSnapshot.h"
#include <optional>

namespace K2040::QuestWideNativeResearch {
namespace {
std::optional<std::uint32_t> TypedUInt(
    RE::PipboyObject* object, const char* key) {
    if(!object)return std::nullopt;
    auto* value=object->GetMember<RE::PipboyValue*>(
        RE::BSFixedString(key));
    if(!value || (value->GetType()!=RE::PipboyValue::kUint32 &&
                  value->GetType()!=RE::PipboyValue::kInt32))
        return std::nullopt;
    return static_cast<std::uint32_t>(
        *static_cast<const RE::PipboyPrimitiveValue<std::uint32_t>*>(value));
}
std::optional<bool> TypedBool(RE::PipboyObject* object, const char* key) {
    if(!object)return std::nullopt;
    auto* value=object->GetMember<RE::PipboyValue*>(
        RE::BSFixedString(key));
    if(!value || value->GetType()!=RE::PipboyValue::kBool)
        return std::nullopt;
    return *static_cast<const RE::PipboyPrimitiveValue<bool>*>(value);
}
RE::PipboyArray* TypedArray(RE::PipboyObject* object,const char* key) {
    if(!object)return nullptr;
    auto* value=object->GetMember<RE::PipboyValue*>(
        RE::BSFixedString(key));
    return value && value->GetType()==RE::PipboyValue::kArray
        ? static_cast<RE::PipboyArray*>(value) : nullptr;
}
void Record(RE::PipboyObject* row,RowKind kind,
            std::uint32_t targetFormID,FeedLedger& feed) {
    feed.Observe(TypedUInt(row,"formID"), TypedUInt(row,"instance"),
                 TypedBool(row,"active"),kind,targetFormID);
}
}
NativeCapture CaptureNativeQuestWideReadOnly(
    std::uint32_t requestedFormID, const TrustedContext& ctx) {
    NativeCapture out{};
    auto& state=out.state;
    out.selectedInstanceIDFromContext=ctx.selectedInstanceID;
    state.selectedFormID=ctx.selectedFormID;
    state.saveEpoch=ctx.saveEpoch;
    state.viewEpoch=ctx.viewEpoch;
    state.runtimeOG163=ctx.runtimeOG163;
    state.gameLoaded=ctx.gameLoaded;
    state.gameThread=ctx.gameThread;
    state.journalFocused=ctx.journalFocused;
    state.instanceIdentityTyped=ctx.selectedInstanceTyped;
    // Never dereference engine state outside a previously verified context.
    if(!requestedFormID || !ctx.saveEpoch || !ctx.viewEpoch ||
       !ctx.runtimeOG163 || !ctx.gameLoaded ||
       !ctx.gameThread || !ctx.journalFocused ||
       ctx.selectedFormID!=requestedFormID)
        return out;
    const auto* quest=RE::TESForm::GetFormByID<RE::TESQuest>(
        requestedFormID);
    if(!quest)return out;
    state.formID=quest->GetFormID();
    state.formIsQuest=true;
    state.currentInstanceID=quest->currentInstanceID;
    state.flags=static_cast<std::uint32_t>(quest->data.flags);
    // Enabled means active in the runtime quest list but does not prove
    // it is safe to apply a quest-tracking action.
    state.questRunning=(state.flags&kEnabled)!=0;
    auto* source=RE::PipboyDataManager::GetSingleton();
    if(!source || !source->questData.questArray)return out;
    auto* list=source->questData.questArray;
    // Alias ownership and engine-equivalent side effects remain unverified.
    state.aliasTopologyAvailable=false;
    state.questAndMapConsumersAvailable=true;
    // A complete snapshot must account for *every* visited parent and
    // child. Never skip an ambiguous node and call the result complete.
    for(auto* element:list->elements) {
        if(out.feed.scanned>=kMaxFeedRows) {
            out.feed.complete=false;out.feed.truncated=true;break;
        }
        if(!element || element->GetType()!=RE::PipboyValue::kObject) {
            out.feed.Observe(std::nullopt,std::nullopt,std::nullopt,
                             RowKind::kOrdinaryQuest,requestedFormID);
            continue;
        }
        auto* parent=static_cast<RE::PipboyObject*>(element);
        const auto parentID=TypedUInt(parent,"formID");
        if(!parentID) {
            out.feed.Observe(std::nullopt,std::nullopt,std::nullopt,
                             RowKind::kOrdinaryQuest,requestedFormID);
            continue;
        }
        if(*parentID) {
            Record(parent,RowKind::kOrdinaryQuest,requestedFormID,out.feed);
            continue;
        }
        // A zero FormID is the Pip-Boy's synthetic Misc folder; it is
        // never itself a TESQuest or tracking target.
        out.feed.Observe(parentID,std::nullopt,std::nullopt,
                         RowKind::kOrdinaryQuest,requestedFormID);
        auto* children=TypedArray(parent,"objectives");
        if(!children) {
            out.feed.complete=false;continue;
        }
        for(auto* child:children->elements) {
            if(out.feed.scanned>=kMaxFeedRows) {
                out.feed.complete=false;out.feed.truncated=true;break;
            }
            if(!child || child->GetType()!=RE::PipboyValue::kObject) {
                out.feed.Observe(std::nullopt,std::nullopt,std::nullopt,
                                 RowKind::kMiscObjective,requestedFormID);
                continue;
            }
            Record(static_cast<RE::PipboyObject*>(child),
                   RowKind::kMiscObjective,requestedFormID,out.feed);
        }
        if(out.feed.truncated)break;
    }
    state.completePipboyFeed=out.feed.complete&&!out.feed.truncated;
    return out;
}
} // namespace K2040::QuestWideNativeResearch
