#include <RE/Fallout.h>
#include <RE/P/PipboyDataManager.h>
#include "QuestCollector.h"
#include "GenericQuestInstanceSummary.h"
#include "QuestWideTrackingReadiness.h"
#include "QuestViewState.h"
#ifdef K2040_SELECTOR_READONLY_QA
#include "SelectorRowCorrelation.h"
#endif
#ifdef K2040_TRACKING_INTEGRATED_QA
#include "../research/native_one_shot/MiscObjectiveIdentity.h"
#include "GenericQuestRowIdentity.h"
#endif
// SafePipboyText includes Windows.h; CommonLib must be included first.
#include "SafePipboyText.h"
#include <algorithm>
#include <array>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace K2040::Quests {
namespace {
    struct Objective {
        std::uint32_t index{};
        std::string text;
        std::string state;
    };
    struct Quest {
        std::uint32_t formId{};
        std::string name;
        std::string description;
        std::string type;
        std::string sourceType;
        bool categoryOverride{};
        std::string state;
        bool pipboyTracked{};
        bool isMisc{};
        bool pinned{};
        bool favorite{};
        bool hidden{};
        bool historyOnly{};
        std::uint32_t miscRowCount{};
        std::uint32_t nativeDisplayedCount{};
        std::optional<std::uint32_t> nativeInstanceID;
        std::string nativeInstanceStatus{"unavailable"};
        std::string pipboyFeedAgreement{"missing"};
        std::vector<Objective> objectives;
    };

    // The Pip-Boy collection is engine-managed and may be absent before a save is ready.
    RE::PipboyValue* Member(RE::PipboyObject* object, const char* key) {
        return object ? object->GetMember<RE::PipboyValue*>(RE::BSFixedString(key)) : nullptr;
    }
    RE::PipboyArray* Array(RE::PipboyObject* object, const char* key) {
        auto* value = Member(object, key);
        return value && value->GetType() == RE::PipboyValue::kArray
            ? reinterpret_cast<RE::PipboyArray*>(value) : nullptr;
    }
    std::uint32_t Uint(RE::PipboyObject* object, const char* key) {
        auto* value = Member(object, key);
        if (!value || (value->GetType() != RE::PipboyValue::kUint32 &&
                       value->GetType() != RE::PipboyValue::kInt32)) return 0;
        return *reinterpret_cast<RE::PipboyPrimitiveValue<std::uint32_t>*>(value);
    }
    bool Boolean(RE::PipboyObject* object, const char* key) {
        auto* value = Member(object, key);
        if (!value || value->GetType() != RE::PipboyValue::kBool) return false;
        return *reinterpret_cast<RE::PipboyPrimitiveValue<bool>*>(value);
    }
    std::string Text(RE::PipboyObject* object, const char* key) {
        auto* value = Member(object, key);
        if (!value || value->GetType() != RE::PipboyValue::kString) return {};
        // CommonLibF4 currently exposes a tagged PipboyValue; the string payload
        // location is not a public accessor. THIS OFFSET MUST BE VALIDATED ON TARGET
        // RUNTIMES before distributing a native DLL.
        constexpr std::uintptr_t kStringOffset = 0x18;
        auto* string = reinterpret_cast<const RE::BSFixedString*>(
            reinterpret_cast<std::uintptr_t>(value) + kStringOffset);
        if (!string) return {};
        return ReadBoundedGameString(string->c_str());
    }
    std::string JsonEscape(std::string_view raw) {
        std::string out;
        out.reserve(raw.size() + 8);
        static constexpr char digits[] = "0123456789ABCDEF";
        for (unsigned char ch : raw) {
            switch (ch) {
                case '\\': out += "\\\\"; break;
                case '"': out += "\\\""; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default:
                    if (ch < 0x20) {
                        out += "\\u00";
                        out += digits[ch >> 4];
                        out += digits[ch & 0xF];
                    } else out += static_cast<char>(ch);
            }
        }
        return out;
    }
    std::string Quoted(std::string_view value) { return "\"" + JsonEscape(value) + "\""; }
    std::string FormId(std::uint32_t id) {
        std::ostringstream s;
        s << std::uppercase << std::hex << std::setfill('0') << std::setw(8) << id;
        return s.str();
    }
    std::string Category(std::uint32_t type) {
        // Raw quest types are not reliable classifications for every mod-added quest.
        switch (type) {
            case 1: return "Main";
            case 2: return "Brotherhood";
            case 3: return "Institute";
            case 4: return "Minutemen";
            case 5: return "Railroad";
            case 6: return "Misc";
            case 7: return "Side";
            case 8: return "Automatron";
            case 9: return "Wasteland Workshop";
            case 10: return "Far Harbor";
            default: return "Other";
        }
    }
    std::string ObjectiveState(std::uint32_t state) {
        if (state == static_cast<std::uint32_t>(RE::QUEST_OBJECTIVE_STATE::kCompleted) ||
            state == static_cast<std::uint32_t>(RE::QUEST_OBJECTIVE_STATE::kCompletedDisplayed)) return "completed";
        if (state == static_cast<std::uint32_t>(RE::QUEST_OBJECTIVE_STATE::kFailed) ||
            state == static_cast<std::uint32_t>(RE::QUEST_OBJECTIVE_STATE::kFailedDisplayed)) return "failed";
        if (state == static_cast<std::uint32_t>(RE::QUEST_OBJECTIVE_STATE::kDisplayed)) return "active";
        return "inactive";
    }
    std::string QuestState(const RE::TESQuest& quest) {
        const auto flags = static_cast<std::uint32_t>(quest.data.flags);
        if (flags & static_cast<std::uint32_t>(RE::QuestFlag::kFailed)) return "failed";
        if (flags & static_cast<std::uint32_t>(RE::QuestFlag::kCompleted)) return "completed";
        return "active";
    }
#if defined(K2040_TRACKING_INTEGRATED_QA) || \
    defined(K2040_DOGMEAT_PRODUCTION_CANDIDATE) || \
    defined(K2040_SELECTOR_READONLY_QA)
    // Never interpret an absent numeric Pip-Boy field as a valid zero.
    std::optional<std::uint32_t> OptionalUint(RE::PipboyObject* row, const char* key) {
        auto* value=Member(row,key);
        if(!value || (value->GetType()!=RE::PipboyValue::kUint32 &&
                      value->GetType()!=RE::PipboyValue::kInt32))
            return std::nullopt;
        return Uint(row,key);
    }
#endif
    // Miscellaneous rows may point at one objective within a larger quest.
    // Do not infer a zero index from a missing member: index 0 is valid.
    std::optional<std::uint32_t> MiscObjectiveIndex(RE::PipboyObject* row) {
        for (const char* key : {"Obj", "obj", "index", "indexID", "objective"}) {
            auto* member = Member(row, key);
            if (!member) continue;
            const auto type = member->GetType();
            if (type == RE::PipboyValue::kUint32 || type == RE::PipboyValue::kInt32)
                return Uint(row, key);
        }
        return std::nullopt;
    }
#if defined(K2040_TRACKING_INTEGRATED_QA) || \
    defined(K2040_SELECTOR_READONLY_QA)
    // Schema-only introspection of engine-managed Pip-Boy row keys.
    // Never record pointer values, text, real form IDs, or other payload.
    // Keys/types are diagnostic, not trusted as an objective index.
    std::string ProbePrimitiveSchema(RE::PipboyObject* row) {
        if(!row)return "[]";
        std::vector<std::string> fields;
        std::uint32_t seen=0;
        for(const auto& entry:row->memberMap) {
            if(++seen>128u)break;
            const char* ptr=entry.first.c_str();
            if(!ptr || !entry.second)continue;
            std::string key;
            for(std::size_t i=0;i<25u;++i) {
                const auto ch=ptr[i];
                if(!ch)break;
                const auto allowed=(ch>='a'&&ch<='z') ||
                    (ch>='A'&&ch<='Z') || (ch>='0'&&ch<='9') ||
                    ch=='_';
                if(!allowed) {key.clear();break;}
                key+=ch;
            }
            if(key.empty() || key.size()>24u || ptr[key.size()]!='\0')continue;
            const auto type=static_cast<unsigned>(entry.second->GetType());
            if(type>8u)continue;
            fields.push_back(key+":"+std::to_string(type));
        }
        std::sort(fields.begin(),fields.end());
        // Keep a manageable schema that fits in one 4K read-only UI payload.
        if(fields.size()>24u)fields.resize(24u);
        std::string out="[";
        for(const auto& field:fields) {
            if(out.size()>1)out+=",";
            out+=Quoted(field);
        }
        return out+"]";
    }
#endif
    void ReadObjectives(const RE::TESQuest* form, Quest& dest, bool onlyActiveForMisc,
                        RE::PipboyObject* row = nullptr) {
        const auto selectedIndex = onlyActiveForMisc ? MiscObjectiveIndex(row) : std::nullopt;
        if (!form) return;
        for (auto* objective : form->objectives) {
            if (!objective) continue;
            const auto state = ObjectiveState(static_cast<std::uint32_t>(objective->state));
            if (onlyActiveForMisc && state != "active") continue;
            if (selectedIndex && objective->index != *selectedIndex) continue;
            if (state == "inactive") continue;  // Avoid unrevealed future spoilers.
            // Several Pip-Boy Misc rows may refer to the same TESQuest but
            // distinct objective indexes. Preserve each once in the journal.
            if (std::any_of(dest.objectives.begin(),dest.objectives.end(),
                [objective](const Objective& known){return known.index==objective->index;})) continue;
            std::string text = ReadBoundedGameString(objective->displayText.c_str());
            if (text.empty()) continue;
            // Runtime alias substitution: text can contain quest instance placeholders.
            RE::BSString expanded;
            expanded.Set(text.c_str(), 0xFFFF);
            RE::BGSQuestInstanceText::ParseString(&expanded, const_cast<RE::TESQuest*>(form), form->currentInstanceID);
            if (expanded.c_str()) {
                const auto parsed = ReadBoundedGameString(expanded.c_str());
                if (!parsed.empty()) text = parsed;
            }
            dest.objectives.push_back({objective->index, std::move(text), state});
        }
    }
    void ReadQuest(RE::PipboyObject* entry, bool misc, std::unordered_set<std::uint32_t>& seen,
                   const std::unordered_set<std::uint32_t>& pinned,
                   const std::unordered_set<std::uint32_t>& favorite,
                   const std::unordered_set<std::uint32_t>& hidden, std::vector<Quest>& dest,
                   bool autoPinNew) {
        const auto id = Uint(entry, "formID");
        if (!id) return;
        auto* form = RE::TESForm::GetFormByID<RE::TESQuest>(id);
        if (!form) return;
        if (!seen.insert(id).second) {
            // A quest may appear in multiple Misc objective rows. The first
            // row owns the quest metadata; subsequent rows add distinct
            // *visible* objective indexes only, without a second quest card.
            if (misc) {
                const auto previous=std::find_if(dest.begin(),dest.end(),
                    [id](const Quest& item){return item.formId==id;});
                if(previous!=dest.end()) {
                    ++previous->miscRowCount;
                    auto* tracked=Member(entry,"active");
                    const bool typed=tracked && tracked->GetType()==RE::PipboyValue::kBool;
                    const bool native=(static_cast<std::uint32_t>(form->data.flags)&0x800u)!=0;
                    if(!typed) previous->pipboyFeedAgreement="missing";
                    else if(Boolean(entry,"active")!=native)
                        previous->pipboyFeedAgreement="mismatch";
                    ReadObjectives(form,*previous,true,entry);
                }
            }
            return;
        }
        Quest item{};
        item.formId = id;
        // First valid Pip-Boy feed is a baseline, not a flood of "new" quests.
        // Subsequent newly visible active quests can be auto-pinned once.
        const bool wasKnown=K2040::QuestViewState::Observed().contains(id);
        if (!K2040::QuestViewState::Remember(id)) return;
        if (autoPinNew && K2040::QuestViewState::InitialFeedSeen() &&
            !wasKnown && QuestState(*form)=="active")
            K2040::QuestViewState::PinNew(id);
        item.name = Text(entry, "text");
        if (item.name.empty()) {
            // CommonLibF4's static GetFullName returns std::string_view, not const char*.
            item.name = std::string(RE::TESFullName::GetFullName(*form));
        }
        item.description = Text(entry, "desc");
        item.sourceType = misc ? "Misc" : Category(static_cast<std::uint32_t>(form->data.questType));
        const auto category=K2040::QuestViewState::CategoryFor(id);
        item.categoryOverride=!category.empty();
        item.type=item.categoryOverride?std::string(category):item.sourceType;
        item.state = QuestState(*form);
        // Top-level Pip-Boy rows can still represent native Misc quests.
        // Never infer native quest-level tracking eligibility from row nesting.
        item.isMisc = misc ||
            static_cast<std::uint32_t>(form->data.questType) == 6u;
        item.miscRowCount=misc?1u:0u;
        // QuestFlag::kActive is the engine tracking flag; an absent Pip-Boy
        // feed "active" member must not hide a currently tracked quest.
        const auto questFlags=static_cast<std::uint32_t>(form->data.flags);
        // CommonLibF4 RE/Q/QuestFlag.h: QuestFlag::kActive = 0x800.
        // Use the verified bit: this project PCH does not expose that enumerator.
        constexpr std::uint32_t kQuestTrackedBit=0x800u;
        const bool engineTracked=(questFlags & kQuestTrackedBit)!=0;
        const bool feedTracked=Boolean(entry,"active");
        auto* nativeFeed=Member(entry,"active");
        const bool typedFeed=nativeFeed && nativeFeed->GetType()==RE::PipboyValue::kBool;
        item.pipboyFeedAgreement=typedFeed
            ? (feedTracked==engineTracked?"match":"mismatch") : "missing";
        item.pipboyTracked=engineTracked || feedTracked;
        item.pinned = pinned.contains(id);
        item.favorite = favorite.contains(id);
        item.hidden = hidden.contains(id);
        ReadObjectives(form, item, misc, entry);
        dest.push_back(std::move(item));
    }
    std::string Serialize(const std::vector<Quest>& quests, bool ready) {
        std::string out = ready ? "{\"ready\":true,\"quests\":[" : "{\"ready\":false,\"quests\":[";
        bool firstQuest = true;
        for (const Quest& quest : quests) {
            if (!firstQuest) out += ',';
            firstQuest = false;
            out += "{\"id\":" + Quoted(FormId(quest.formId));
            out += ",\"name\":" + Quoted(quest.name);
            out += ",\"description\":" + Quoted(quest.description);
            out += ",\"type\":" + Quoted(quest.type);
            out += ",\"sourceType\":" + Quoted(quest.sourceType);
            out += std::string(",\"categoryOverride\":") + (quest.categoryOverride ? "true" : "false");
            out += ",\"state\":" + Quoted(quest.state);
            out += std::string(",\"vanillaTracked\":") + (quest.pipboyTracked ? "true" : "false");
            out += std::string(",\"isMisc\":") + (quest.isMisc ? "true" : "false");
            out += ",\"trackingScope\":\"quest-wide\"";
            out += ",\"miscRowCount\":" + std::to_string(quest.miscRowCount);
            out += ",\"nativeDisplayedCount\":" + std::to_string(quest.nativeDisplayedCount);
            out += ",\"nativeInstanceID\":" + (quest.nativeInstanceID ? std::to_string(*quest.nativeInstanceID) : "null");
            out += ",\"nativeInstanceStatus\":" + Quoted(quest.nativeInstanceStatus);
            out += ",\"pipboyFeedAgreement\":" + Quoted(quest.pipboyFeedAgreement);
            // This is a display-only preflight; the production native
            // writer remains absent and the QA writer has separate gates.
            const auto readiness=K2040::QuestWideReadiness::Assess({
                quest.formId,quest.state=="active",quest.historyOnly,true,
                quest.isMisc,quest.nativeInstanceStatus=="consistent",
                quest.nativeInstanceID.has_value(),quest.nativeDisplayedCount,
                quest.miscRowCount,quest.pipboyFeedAgreement=="match"});
            out += ",\"trackingReadiness\":" +
                Quoted(K2040::QuestWideReadiness::Name(readiness));
            out += std::string(",\"pinned\":") + (quest.pinned ? "true" : "false");
            out += std::string(",\"favorite\":") + (quest.favorite ? "true" : "false");
            out += std::string(",\"hidden\":") + (quest.hidden ? "true" : "false");
            out += std::string(",\"historyOnly\":") + (quest.historyOnly ? "true" : "false");
            out += ",\"objectives\":[";
            bool firstObjective = true;
            for (const Objective& objective : quest.objectives) {
                if (!firstObjective) out += ',';
                firstObjective = false;
                out += "{\"index\":" + std::to_string(objective.index);
                out += ",\"text\":" + Quoted(objective.text);
                out += ",\"state\":" + Quoted(objective.state) + '}';
            }
            out += "]}";
        }
        return out + "]}";
    }
}

std::string CaptureSnapshot(const std::unordered_set<std::uint32_t>& pinnedQuestIds,
                            const std::unordered_set<std::uint32_t>& favoriteQuestIds,
                            const std::unordered_set<std::uint32_t>& hiddenQuestIds, bool autoPinNew) {
    auto* source = RE::PipboyDataManager::GetSingleton();
    if (!source || !source->questData.questArray) return Serialize({}, false);
    auto* array = source->questData.questArray;
    std::unordered_set<std::uint32_t> seen;
    std::vector<Quest> quests;
    for (auto* value : array->elements) {
        if (!value || value->GetType() != RE::PipboyValue::kObject) continue;
        auto* entry = reinterpret_cast<RE::PipboyObject*>(value);
        if (Uint(entry, "formID")) {
            ReadQuest(entry, false, seen, pinnedQuestIds, favoriteQuestIds, hiddenQuestIds, quests,autoPinNew);
        } else if (auto* misc = Array(entry, "objectives")) {
            for (auto* nested : misc->elements) {
                if (nested && nested->GetType() == RE::PipboyValue::kObject)
                    ReadQuest(reinterpret_cast<RE::PipboyObject*>(nested), true, seen, pinnedQuestIds, favoriteQuestIds, hiddenQuestIds, quests,autoPinNew);
            }
        }
    }
    K2040::QuestViewState::CompleteInitialFeed();
    // Consult only player-encountered IDs; never scan the load order for
    // undiscovered quest forms (which would spoil unrevealed content).
    std::vector<std::uint32_t> observedIDs(K2040::QuestViewState::Observed().begin(),
                                           K2040::QuestViewState::Observed().end());
    std::sort(observedIDs.begin(),observedIDs.end());
    for(const auto id:observedIDs) {
        if(seen.contains(id))continue;
        const auto* form=RE::TESForm::GetFormByID<RE::TESQuest>(id);
        if(!form)continue;
        const auto state=QuestState(*form);
        if(state!="completed" && state!="failed")continue;
        Quest item{};
        item.formId=id;
        item.name=std::string(RE::TESFullName::GetFullName(*form));
        if(item.name.empty())continue;
        item.sourceType=Category(static_cast<std::uint32_t>(form->data.questType));
        const auto replacement=K2040::QuestViewState::CategoryFor(id);
        item.categoryOverride=!replacement.empty();
        item.type=item.categoryOverride?std::string(replacement):item.sourceType;
        item.state=state;
        item.historyOnly=true;
        item.pinned=pinnedQuestIds.contains(id);
        item.favorite=favoriteQuestIds.contains(id);
        item.hidden=hiddenQuestIds.contains(id);
        ReadObjectives(form,item,false);
        seen.insert(id);
        quests.push_back(std::move(item));
    }
    // This is authoritative *read-side* data, not a native write grant.
    // Acquire the engine's questTargetLock once; copy only bounded values.
    K2040::GenericQuestInstance::Index native;
    if(auto* player=RE::PlayerCharacter::GetSingleton()) {
        RE::BSAutoLock guard{player->questTargetLock};
        for(const auto& inst:player->objectives) {
            if(native.Scanned()>=K2040::GenericQuestInstance::kMaxObservations) {native.MarkUnavailable();break;}
            const auto* obj=inst.objective;
            const bool displayed=inst.enstanceState.underlying()==
                static_cast<std::int32_t>(RE::QUEST_OBJECTIVE_STATE::kDisplayed);
            native.Observe(obj&&obj->ownerQuest?obj->ownerQuest->GetFormID():0u,
                           obj?static_cast<std::uint32_t>(obj->index):0u,
                           inst.instanceID,displayed);
        }
    } else native.MarkUnavailable();
    for(auto& item:quests) {
        if(item.historyOnly)continue;
        const auto* form=RE::TESForm::GetFormByID<RE::TESQuest>(item.formId);
        item.nativeInstanceStatus=K2040::GenericQuestInstance::Name(
            form?native.Resolve(item.formId,form->currentInstanceID):
                 K2040::GenericQuestInstance::State::kUnavailable);
        item.nativeDisplayedCount=native.DisplayedCount(item.formId);
        if(form && native.Resolve(item.formId,form->currentInstanceID)==
                       K2040::GenericQuestInstance::State::kConsistent)
            item.nativeInstanceID=form->currentInstanceID;
    }
    return Serialize(quests, true);
}

#if defined(K2040_SINGLE_MISC_WRITE_QA) || \
    defined(K2040_DOGMEAT_PRODUCTION_CANDIDATE)
SingleMiscQA::Observation InspectSingleMiscWriteCandidate(std::uint32_t id) {
    // Fail-closed research ONLY. No pointer leaves this game-thread read.
    SingleMiscQA::Observation o{};
    const auto* manager=RE::PipboyDataManager::GetSingleton();
    auto* player=RE::PlayerCharacter::GetSingleton();
    const auto* quest=id?RE::TESForm::GetFormByID<RE::TESQuest>(id):nullptr;
    if(!id || !manager || !manager->questData.questArray ||
       !player || !quest)return o;
    o.selectedAndObserved=true; // caller separately validates observed ID.
    o.formID=quest->GetFormID();
    o.questType=static_cast<std::uint32_t>(quest->data.questType);
    o.questFlags=static_cast<std::uint32_t>(quest->data.flags);
    o.currentInstance=quest->currentInstanceID;
    std::uint32_t nativeCount=0;
    bool bounded=true;
    {
        RE::BSAutoLock lock{player->questTargetLock};
        std::uint32_t scanned=0;
        for(const auto& instance:player->objectives) {
            if(++scanned>512u) {bounded=false;break;}
            const auto* obj=instance.objective;
            if(!obj || obj->ownerQuest!=quest ||
               instance.enstanceState.underlying()!=
               static_cast<std::int32_t>(RE::QUEST_OBJECTIVE_STATE::kDisplayed))
                continue;
            if(++nativeCount>1u)break;
            o.nativeIndex=static_cast<std::uint32_t>(obj->index);
            o.nativeInstance=instance.instanceID;
        }
    }
    o.exactlyOneNativeDisplayed=nativeCount==1u;
    // Also cross-check the owning quest's ordinary objective list: the
    // instanced player list alone must not hide another displayed objective.
    std::uint32_t questDisplayed=0,questWalked=0;
    for(const auto* obj:quest->objectives) {
        if(++questWalked>256u) {bounded=false;break;}
        if(obj && static_cast<std::uint32_t>(obj->state)==
                  static_cast<std::uint32_t>(RE::QUEST_OBJECTIVE_STATE::kDisplayed))
            ++questDisplayed;
    }
    if(questDisplayed!=1u)bounded=false;
    std::uint32_t root=0,nested=0,outerWalked=0;
    for(auto* v:manager->questData.questArray->elements) {
        if(++outerWalked>512u) {bounded=false;break;}
        if(!v || v->GetType()!=RE::PipboyValue::kObject)continue;
        auto* row=static_cast<RE::PipboyObject*>(v);
        if(Uint(row,"formID")==id)++root;
        auto* children=Array(row,"objectives");
        if(!children)continue;
        std::uint32_t innerWalked=0;
        for(auto* child:children->elements) {
            if(++innerWalked>128u){bounded=false;break;}
            if(!child || child->GetType()!=RE::PipboyValue::kObject)continue;
            auto* entry=static_cast<RE::PipboyObject*>(child);
            if(Uint(entry,"formID")!=id)continue;
            if(++nested>1u)break;
            const auto flag=[entry](const char* key, bool& typed)->bool {
                auto* member=Member(entry,key);
                typed=member && member->GetType()==RE::PipboyValue::kBool;
                return typed && Boolean(entry,key);
            };
            o.nestedActive=flag("active",o.nestedActiveTyped);
            o.nestedEnabled=flag("enabled",o.nestedEnabledTyped);
            bool completedKnown=false,failedKnown=false;
            o.nestedCompleted=flag("completed",completedKnown);
            o.nestedFailed=flag("failed",failedKnown);
            if(!completedKnown || !failedKnown)bounded=false;
            const auto instance=OptionalUint(entry,"instance");
            o.nestedInstanceTyped=instance.has_value();
            o.nestedInstance=instance.value_or(0);
            o.nestedIndexAbsent=!MiscObjectiveIndex(entry).has_value();
        }
    }
    o.rootRowAbsent=root==0u;
    o.exactlyOneNestedRow=nested==1u;
    o.truncated=!bounded;
    o.complete=bounded;
    return o;
 }
#endif
#ifdef K2040_SELECTOR_READONLY_QA
std::string CaptureSelectorRowCorrelation() {
    // Whole-feed structural survey for an isolated READ-ONLY build.
    // The named members of a Pip-Boy row are not proven to be the
    // positional selectedEntry arguments consumed by the game command.
    auto* manager=RE::PipboyDataManager::GetSingleton();
    if(!manager || !manager->questData.questArray)
        return "{\"available\":false}";
    using namespace K2040::SelectorRowCorrelation;
    constexpr std::size_t kStatuses=
        static_cast<std::size_t>(Status::kCandidateNotVerified)+1u;
    std::array<std::uint32_t,kStatuses> totals{};
    std::uint32_t roots=0,children=0,synthetic=0;
    // Objective-display leaves beneath an actual quest root are NOT
    // nested Misc quest entries. Count separately and never ask the
    // quest/instance correlation policy to validate their missing IDs.
    std::uint32_t ordinaryObjectiveRows=0,unknownParentChildren=0;
    std::uint32_t typeKnown=0,instanceKnown=0;
    std::uint32_t typeKeyQuest=0,typeKeyGeneric=0,typeKeysBoth=0;
    std::uint32_t typeKnownRoot=0,typeKnownChild=0;
    std::string rootSchema="[]",miscSchema="[]",syntheticSchema="[]";
    std::string objectiveSchema="[]";
    bool rootSampled=false,miscSampled=false,syntheticSampled=false;
    bool objectiveSampled=false;
    bool truncated=false;
    std::uint32_t visited=0;
    const auto inspect=[&](RE::PipboyObject* row,Kind kind){
        if(!row)return;
        auto formID=OptionalUint(row,"formID");
        auto instance=OptionalUint(row,"instance");
        auto namedQuestType=OptionalUint(row,"questType");
        auto namedType=OptionalUint(row,"type");
        const bool both=namedQuestType && namedType;
        const bool ambiguous=both && *namedQuestType!=*namedType;
        const auto category=namedQuestType ? namedQuestType : namedType;
        if(category)++typeKnown;
        if(instance)++instanceKnown;
        if(namedQuestType)++typeKeyQuest;
        if(namedType)++typeKeyGeneric;
        if(both)++typeKeysBoth;
        if(kind==Kind::kQuestRoot) {
            ++roots;
            if(category)++typeKnownRoot;
            if(!rootSampled) {rootSchema=ProbePrimitiveSchema(row);rootSampled=true;}
        } else if(kind==Kind::kMiscChild) {
            ++children;
            if(category)++typeKnownChild;
            if(!miscSampled) {miscSchema=ProbePrimitiveSchema(row);miscSampled=true;}
        } else {
            ++synthetic;
            if(!syntheticSampled) {
                syntheticSchema=ProbePrimitiveSchema(row);syntheticSampled=true;
            }
        }
        RE::TESQuest* quest=formID && *formID
            ? RE::TESForm::GetFormByID<RE::TESQuest>(*formID) : nullptr;
        const Observation observation{
            kind,formID,instance,category,ambiguous,
            quest!=nullptr,
            quest ? quest->formID : 0u,
            quest ? quest->currentInstanceID : 0u,
            quest ? static_cast<std::uint32_t>(quest->data.questType) : 0u
        };
        ++totals[static_cast<std::size_t>(Compare(observation))];
    };
    for(auto* value:manager->questData.questArray->elements) {
        if(++visited>512u) {truncated=true;break;}
        if(!value||value->GetType()!=RE::PipboyValue::kObject)continue;
        auto* row=static_cast<RE::PipboyObject*>(value);
        const auto formID=OptionalUint(row,"formID");
        const bool isSynthetic=formID && *formID==0u;
        inspect(row,isSynthetic?Kind::kSyntheticMiscRoot:Kind::kQuestRoot);
        auto* group=Array(row,"objectives");
        if(!group)continue;
        std::uint32_t nestedVisited=0;
        for(auto* nested:group->elements) {
            if(++nestedVisited>128u || ++visited>512u) {
                truncated=true;break;
            }
            if(!nested || nested->GetType()!=RE::PipboyValue::kObject)
                continue;
            auto* child=static_cast<RE::PipboyObject*>(nested);
            if(isSynthetic) {
                // Only the synthetic Misc folder owns selectable
                // nested quest rows. Invalid children stay invalid,
                // rather than being silently mapped to a real quest.
                inspect(child,Kind::kMiscChild);
            }else if(formID && *formID) {
                // A normal quest's 'objectives' array is display-only:
                // its leaves intentionally lack formID/instance/type.
                ++ordinaryObjectiveRows;
                if(!objectiveSampled) {
                    objectiveSchema=ProbePrimitiveSchema(child);
                    objectiveSampled=true;
                }
            }else {
                // Unknown parent: do not assume this is a selectable
                // Misc objective or a valid native command candidate.
                ++unknownParentChildren;
            }
        }
        if(truncated)break;
    }
    std::string output="{\"available\":true,\"auditVersion\":2,\"rootRows\":"+
        std::to_string(roots)+",\"miscRows\":"+std::to_string(children)+
        ",\"syntheticRows\":"+std::to_string(synthetic)+
        ",\"ordinaryObjectiveRows\":"+std::to_string(ordinaryObjectiveRows)+
        ",\"unknownParentChildren\":"+std::to_string(unknownParentChildren)+
        ",\"typedRowTypeFields\":"+std::to_string(typeKnown)+
        ",\"typedInstanceFields\":"+std::to_string(instanceKnown)+
        ",\"questTypeKeys\":"+std::to_string(typeKeyQuest)+
        ",\"typeKeys\":"+std::to_string(typeKeyGeneric)+
        ",\"bothKeys\":"+std::to_string(typeKeysBoth)+
        ",\"typedRootTypeFields\":"+std::to_string(typeKnownRoot)+
        ",\"typedMiscTypeFields\":"+std::to_string(typeKnownChild)+
        ",\"truncated\":"+(truncated?"true":"false")+
        ",\"statusCounts\":[";
    for(std::size_t i=0;i<totals.size();++i) {
        if(i)output+=',';
        output+="{\"kind\":"+
            Quoted(Name(static_cast<Status>(i)))+
            ",\"count\":"+std::to_string(totals[i])+"}";
    }
    output+="],\"schemaQuestRoot\":"+rootSchema+
        ",\"schemaMiscChild\":"+miscSchema+
        ",\"schemaSyntheticRoot\":"+syntheticSchema+
        ",\"schemaObjectiveDisplay\":"+objectiveSchema+"}";
    // Exactly key names/types, row class and aggregate counts only:
    // no quest names, FormIDs, quest IDs, pointers, objectives or save data.
    return output.size()<=4096u
        ? output : "{\"available\":false,\"truncated\":true}";
}
#endif

#ifdef K2040_TRACKING_INTEGRATED_QA
std::string CaptureMiscTrackingProbe(std::uint32_t id) {
    // Strict read-only snapshot of the normal quest-array and nested
    // Pip-Boy Misc objective rows. Objective "active" is not assumed
    // equivalent to the quest tracking setter or objective displayed state.
    auto* manager=RE::PipboyDataManager::GetSingleton();
    auto* quest=id?RE::TESForm::GetFormByID<RE::TESQuest>(id):nullptr;
    if(!manager || !manager->questData.questArray || !quest)
        return "{\"available\":false}";
    const auto type=static_cast<std::uint32_t>(quest->data.questType);
    const bool nativeTracked=(static_cast<std::uint32_t>(quest->data.flags)&0x800u)!=0;
    // Capture only bounded value identities while the engine owns its lock.
    // NQT uses this same PlayerCharacter objective/questTargetLock pattern.
    // These are numerical CANDIDATES, never a pointer retained after unlock.
    std::vector<K2040::MiscIdentity::NativeObjective> nativeObjectives;
    nativeObjectives.reserve(12u);
    bool nativeTruncated=false;
    auto* player=RE::PlayerCharacter::GetSingleton();
    if(player) {
        RE::BSAutoLock guard{player->questTargetLock};
        std::uint32_t scanned=0;
        for(const auto& instance:player->objectives) {
            if(++scanned>512u) {nativeTruncated=true;break;}
            const auto* obj=instance.objective;
            if(!obj || obj->ownerQuest!=quest ||
               instance.enstanceState.underlying()!=
                   static_cast<std::int32_t>(RE::QUEST_OBJECTIVE_STATE::kDisplayed))
                continue;
            if(nativeObjectives.size()>=12u) {nativeTruncated=true;break;}
            nativeObjectives.push_back({
                static_cast<std::uint32_t>(obj->index),instance.instanceID});
        }
    }
    const bool nativeComplete=player && !nativeTruncated;
    std::uint32_t rootRows=0, rootTracked=0, rootActiveKnown=0;
    std::uint32_t nestedCount=0, nestedActive=0;
    bool capped=false, first=true;
    // A second, generic read-only correlation over ALL bounded child
    // rows. The old per-row probe remains unchanged as a control.
    std::vector<K2040::GenericQuestRowIdentity::Row> genericRows;
    genericRows.reserve(12u);
    std::string nested="[";
    std::uint32_t traversed=0;
    for(auto* value: manager->questData.questArray->elements) {
        if(++traversed>512u) {capped=true;break;}
        if(!value || value->GetType()!=RE::PipboyValue::kObject)continue;
        auto* row=static_cast<RE::PipboyObject*>(value);
        if(Uint(row,"formID")==id) {
            ++rootRows;
            auto* flag=Member(row,"active");
            if(flag && flag->GetType()==RE::PipboyValue::kBool) {
                ++rootActiveKnown;
                if(Boolean(row,"active"))++rootTracked;
            }
        }
        auto* group=Array(row,"objectives");
        if(!group)continue;
        std::uint32_t walked=0;
        for(auto* member:group->elements) {
            if(++walked>128u) {capped=true;break;}
            if(!member || member->GetType()!=RE::PipboyValue::kObject)continue;
            auto* nestedRow=static_cast<RE::PipboyObject*>(member);
            if(Uint(nestedRow,"formID")!=id)continue;
            ++nestedCount;
            auto* flag=Member(nestedRow,"active");
            const bool known=flag && flag->GetType()==RE::PipboyValue::kBool;
            const bool active=known && Boolean(nestedRow,"active");
            if(active)++nestedActive;
            const auto objectiveIndex=MiscObjectiveIndex(nestedRow);
            // "instance" is the numeric Pip-Boy FIELD, not yet proven
            // equal to native BGSInstancedQuestObjective::instanceID.
            const auto pipboyInstance=OptionalUint(nestedRow,"instance");
            const auto correlation=K2040::MiscIdentity::Compare(
                {objectiveIndex,pipboyInstance},nativeObjectives,nativeComplete);
            if(nestedCount>12u) {capped=true;continue;}
            genericRows.push_back({id,
                K2040::GenericQuestRowIdentity::Kind::kMiscObjective,
                objectiveIndex,pipboyInstance,known});
            if(!first)nested+=',';
            first=false;
            nested+="{\"indexKnown\":";
            nested+=objectiveIndex?"true":"false";
            nested+=",\"index\":";
            nested+=std::to_string(objectiveIndex.value_or(0));
            nested+=",\"activeKnown\":";
            nested+=known?"true":"false";
            nested+=",\"active\":";
            nested+=active?"true":"false";
            nested+=",\"instanceKnown\":";
            nested+=pipboyInstance?"true":"false";
            nested+=",\"instance\":";
            nested+=std::to_string(pipboyInstance.value_or(0));
            nested+=",\"nativeCandidateStatus\":";
            nested+=Quoted(K2040::MiscIdentity::StatusName(correlation.status));
            nested+=",\"nativeCandidateCount\":";
            nested+=std::to_string(correlation.count);
            // First few rows include field names/types to identify the
            // unknown index field without exposing raw Pip-Boy payloads.
            if(nestedCount<=3u) {
                nested+=",\"fieldSchema\":";
                nested+=ProbePrimitiveSchema(nestedRow);
            }
            nested+='}';
        }
    }
    nested+=']';
    std::vector<K2040::GenericQuestRowIdentity::Native> generalNative;
    generalNative.reserve(nativeObjectives.size());
    for (const auto& objective:nativeObjectives)
        generalNative.push_back({id,objective.index,objective.instanceID,true});
    const auto generalResults=K2040::GenericQuestRowIdentity::Resolve(
        genericRows,generalNative,nativeComplete && !capped);
    // Anonymous category statuses only, not pointer values, save names or
    // player quest IDs. This can be compared across diverse quest shapes.
    std::string generalStatus="[";
    for (std::size_t i=0;i<generalResults.size();++i) {
        if(i)generalStatus+=',';
        generalStatus+=Quoted(K2040::GenericQuestRowIdentity::StatusName(
            generalResults[i].status));
    }
    generalStatus+=']';
    // A native Misc type or a nested Pip-Boy Misc row is necessary.
    // Never claim that a normal quest's root row is a Misc objective.
    if(type!=6u && nestedCount==0u)return "{\"available\":false}";
    std::string out="{\"id\":"+Quoted(FormId(id));
    out+=",\"available\":true,\"questType\":"+std::to_string(type);
    out+=",\"nativeTracked\":";
    out+=nativeTracked?"true":"false";
    out+=",\"rootRows\":"+std::to_string(rootRows);
    out+=",\"rootActiveKnown\":"+std::to_string(rootActiveKnown);
    out+=",\"rootTracked\":"+std::to_string(rootTracked);
    out+=",\"miscRows\":"+std::to_string(nestedCount);
    out+=",\"miscTracked\":"+std::to_string(nestedActive);
    // Candidate objective indexes are from the owning TESQuest, NOT a
    // proven mapping to any Pip-Boy Misc row. Restrict to displayed entries.
    std::string candidateIndices="[";
    std::uint32_t objectivesRead=0, displayedCount=0;
    for(const auto* objective:quest->objectives) {
        if(++objectivesRead>256u) {capped=true;break;}
        if(!objective)continue;
        const auto state=static_cast<std::uint32_t>(objective->state);
        if(state!=static_cast<std::uint32_t>(RE::QUEST_OBJECTIVE_STATE::kDisplayed) &&
           state!=static_cast<std::uint32_t>(RE::QUEST_OBJECTIVE_STATE::kCompletedDisplayed) &&
           state!=static_cast<std::uint32_t>(RE::QUEST_OBJECTIVE_STATE::kFailedDisplayed))
            continue;
        if(++displayedCount>16u) {capped=true;break;}
        if(candidateIndices.size()>1u)candidateIndices+=",";
        candidateIndices+=std::to_string(objective->index);
    }
    candidateIndices+="]";
    out+=",\"displayedObjectiveCandidates\":"+candidateIndices;
    out+=",\"nativeObjectiveCollectionAvailable\":";
    out+=player?"true":"false";
    out+=",\"nativeObjectiveCollectionComplete\":";
    out+=nativeComplete?"true":"false";
    out+=",\"nativeObjectiveCollectionTruncated\":";
    out+=nativeTruncated?"true":"false";
    out+=",\"nativeCurrentInstanceID\":";
    out+=std::to_string(quest->currentInstanceID);
    out+=",\"nativeObjectiveInstances\":[";
    for(std::size_t i=0;i<nativeObjectives.size();++i) {
        if(i)out+=',';
        out+="{\"index\":"+std::to_string(nativeObjectives[i].index);
        out+=",\"instanceID\":"+std::to_string(nativeObjectives[i].instanceID)+'}';
    }
    out+=']';
    out+=",\"truncated\":";
    out+=capped?"true":"false";
    out+=",\"genericCandidateStatuses\":"+generalStatus;
    out+=",\"objectives\":"+nested+"}";
    return out;
}
#endif
}
