#pragma once
// Generic SCALEFORM READ-ONLY observation of currently selected
// vanilla Pip-Boy entries. No game state, cursor, UI, or AS3 mutation.
// Linked only in the isolated UI-task QA flavor, never in production.
#include <cstdint>

namespace K2040::PipboySelectedEntryResearch {
enum class Status : std::uint8_t {
    kMenuUnavailable, kPageUnavailable, kTabUnavailable,
    kQuestSelectionUnavailable, kRootIdentityUnavailable,
    kOrdinaryQuestSelection, kMiscChildNotSelected, kMiscChildSelected
};
struct Observation {
    Status status{Status::kMenuUnavailable};
    bool rootFormIDTyped{};
    bool syntheticMiscParent{};
    bool selectedFromNativeObjectivesList{};
    bool nestedFormIDTyped{};
    bool nestedInstanceTyped{};
    bool nestedTypeTyped{};
    bool nestedQuestTypeTyped{};
    bool nativeCommandTripleVerified{}; // MUST remain false
    bool nativeWriteAuthorized{};        // MUST remain false
};
template<class Value>
[[nodiscard]] bool UIntMember(const Value& object, const char* key,
                              std::uint32_t* result=nullptr) {
    Value value;
    if (!object.GetMember(key, &value))return false;
    if (value.IsUInt()) {
        if(result)*result=value.GetUInt();
        return true;
    }
    if(value.IsInt() && value.GetInt()>=0){
        if(result)*result=static_cast<std::uint32_t>(value.GetInt());
        return true;
    }
    return false;
}
template<class Value>
[[nodiscard]] bool Object(const Value& value) {
    return value.IsObject() || value.IsDisplayObject();
}
template<class Movie, class Value>
[[nodiscard]] Observation Inspect(const Movie& movie) {
    // The literal path components are PUBLIC AS3 members, not engine
    // addresses. Runtime resolution is unverified until a safe UI-task QA.
    Observation out{};
    Value menu,page,tab,questList,selected,objectives,nested;
    if(!movie.GetVariable(&menu,"root.Menu_mc")||!Object(menu))
        return out;
    out.status=Status::kPageUnavailable;
    if(!menu.GetMember("CurrentPage",&page)||!Object(page))
        return out;
    out.status=Status::kTabUnavailable;
    if(!page.GetMember("QuestsTab_mc",&tab)||!Object(tab))
        return out;
    Value visible;
    // Fail closed when another Data-page tab is active or visibility
    // cannot be confirmed; stale selectedEntry objects may remain.
    if(!tab.GetMember("visible",&visible)||!visible.IsBoolean()||
       !visible.GetBoolean())
        return out;
    if(!tab.GetMember("QuestsList_mc",&questList)||!Object(questList))
        return out;
    out.status=Status::kQuestSelectionUnavailable;
    if(!questList.GetMember("selectedEntry",&selected)||!Object(selected))
        return out;
    std::uint32_t id{};
    out.status=Status::kRootIdentityUnavailable;
    out.rootFormIDTyped=UIntMember(selected,"formID",&id);
    if(!out.rootFormIDTyped)return out;
    if(id!=0u) {
        out.status=Status::kOrdinaryQuestSelection;
        return out;
    }
    out.syntheticMiscParent=true;
    out.status=Status::kMiscChildNotSelected;
    if(!tab.GetMember("ObjectivesList_mc",&objectives)||!Object(objectives)||
       !objectives.GetMember("selectedEntry",&nested)||!Object(nested))
        return out;
    // Unlike general quest-array objective leaves, this is the exact
    // vanilla objectives-list selectedEntry after onQuestSelection.
    // NEVER read identifying values into logs or hand them to a writer.
    out.selectedFromNativeObjectivesList=true;
    out.status=Status::kMiscChildSelected;
    out.nestedFormIDTyped=UIntMember(nested,"formID");
    out.nestedInstanceTyped=UIntMember(nested,"instance");
    out.nestedTypeTyped=UIntMember(nested,"type");
    out.nestedQuestTypeTyped=UIntMember(nested,"questType");
    return out;
}
[[nodiscard]] constexpr const char* Name(Status s) noexcept {
    switch(s) {
    case Status::kMenuUnavailable:return "menu-unavailable";
    case Status::kPageUnavailable:return "page-unavailable";
    case Status::kTabUnavailable:return "tab-unavailable";
    case Status::kQuestSelectionUnavailable:return "quest-selection-unavailable";
    case Status::kRootIdentityUnavailable:return "root-identity-unavailable";
    case Status::kOrdinaryQuestSelection:return "ordinary-root-selected";
    case Status::kMiscChildNotSelected:return "misc-root-child-not-selected";
    case Status::kMiscChildSelected:return "native-misc-list-child-selected";
    }
    return "unknown";
}
// Only linked into the independent OG163 read-only UI-task QA flavor.
[[nodiscard]] Observation ObserveOpenPipboyOnUIThread();
} // namespace K2040::PipboySelectedEntryResearch
