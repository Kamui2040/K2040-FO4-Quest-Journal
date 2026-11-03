// NON-SHIPPING: read-only evidence for one selected Misc TESQuest.
// No native quest state changes, no queued pointers, no log of private IDs.
#if !(defined(K2040_TRACKING_NATIVE_WRITE_QA) && defined(K2040_GENERIC_MISC_QUESTWIDE_QA)) && !defined(K2040_REPEATABLE_MISC_QA) && !defined(K2040_QUESTWIDE_PRODUCTION_CANDIDATE)
#error Generic Misc observation exists ONLY in isolated write QA
#endif
#include <RE/Fallout.h>
#include <RE/P/PipboyDataManager.h>
#include "OneShotOgDryRun.h"
#include "../../src/GenericQuestInstanceSummary.h"

namespace K2040::OneShotResearch {
namespace {
RE::PipboyValue* Member(RE::PipboyObject* row,const char* key) {
    return row?row->GetMember<RE::PipboyValue*>(RE::BSFixedString(key)):nullptr;
}
bool Uint(RE::PipboyObject* row,const char* key,std::uint32_t& out) {
    auto* value=Member(row,key);
    if(!value || (value->GetType()!=RE::PipboyValue::kUint32 &&
                   value->GetType()!=RE::PipboyValue::kInt32))return false;
    out=static_cast<std::uint32_t>(
         *static_cast<RE::PipboyPrimitiveValue<std::uint32_t>*>(value));
    return true;
}
bool Bool(RE::PipboyObject* row,const char* key,bool& out) {
    auto* value=Member(row,key);
    if(!value || value->GetType()!=RE::PipboyValue::kBool)return false;
    out=static_cast<bool>(*static_cast<RE::PipboyPrimitiveValue<bool>*>(value));
    return true;
}
}
GenericMiscEvidence CaptureGenericMiscEvidence(const Intent& request) {
    GenericMiscEvidence result{};
    if(!request.formId)return result;
    auto* quest=RE::TESForm::GetFormByID<RE::TESQuest>(request.formId);
    auto* player=RE::PlayerCharacter::GetSingleton();
    auto* pip=RE::PipboyDataManager::GetSingleton();
    if(!quest || !player || !pip || !pip->questData.questArray)return result;
    result.selectedIsMisc=quest->data.questType==6;
    if(!result.selectedIsMisc)return result;
    result.questInstanceID=quest->currentInstanceID;
    K2040::GenericQuestInstance::Index instances;
    {
        RE::BSAutoLock guard{player->questTargetLock};
        for(const auto& inst:player->objectives) {
            if(instances.Scanned()>=K2040::GenericQuestInstance::kMaxObservations){
                instances.MarkUnavailable();break;
            }
            const auto* obj=inst.objective;
            instances.Observe(obj&&obj->ownerQuest?obj->ownerQuest->GetFormID():0u,
                obj?static_cast<std::uint32_t>(obj->index):0u,
                inst.instanceID,
                inst.enstanceState.underlying()==
                static_cast<std::int32_t>(RE::QUEST_OBJECTIVE_STATE::kDisplayed));
        }
    }
    result.nativeComplete=instances.Complete();
    result.nativeDisplayedCount=instances.DisplayedCount(request.formId);
    result.onlyCurrentInstance=instances.Resolve(request.formId,quest->currentInstanceID)==
        K2040::GenericQuestInstance::State::kConsistent;
    // The current game-managed TESQuest active flag, never a Pip-Boy guess.
    const bool native=bool(static_cast<std::uint32_t>(quest->data.flags)&kTracked);
    bool complete=true,consistent=true,matched=false;
    std::size_t scanned=0;
    for(auto* item:pip->questData.questArray->elements){
        if(++scanned>512u){complete=false;break;}
        if(!item || item->GetType()!=RE::PipboyValue::kObject){
            complete=false;break;
        }
        auto* parent=static_cast<RE::PipboyObject*>(item);
        std::uint32_t parentID{};
        if(!Uint(parent,"formID",parentID)){complete=false;break;}
        if(parentID==request.formId){ // only synthetic Misc folder accepted
            complete=false;break;
        }
        if(parentID!=0u)continue;
        auto* value=Member(parent,"objectives");
        if(!value || value->GetType()!=RE::PipboyValue::kArray){
            complete=false;break;
        }
        auto* children=static_cast<RE::PipboyArray*>(value);
        for(auto* child:children->elements) {
            if(++scanned>512u){complete=false;break;}
            if(!child || child->GetType()!=RE::PipboyValue::kObject){
                complete=false;break;
            }
            auto* row=static_cast<RE::PipboyObject*>(child);
            std::uint32_t id{};
            if(!Uint(row,"formID",id)){complete=false;break;}
            if(id!=request.formId)continue;
            matched=true;
            ++result.miscFeedRows;
            bool active{};
            if(!Bool(row,"active",active) || active!=native)
                consistent=false;
            // Optional field: if typed, it MUST agree; never assume it
            // exists or use it as the source of objective identity.
            auto* instanceMember=Member(row,"instance");
            if(instanceMember){
                std::uint32_t instanceID{};
                if(!Uint(row,"instance",instanceID) ||
                   instanceID!=result.questInstanceID)consistent=false;
            }
        }
        if(!complete)break;
    }
    result.pipboyFeedComplete=complete && matched;
    result.feedAgreesWithQuest=consistent && result.pipboyFeedComplete;
    return result;
}
} // namespace K2040::OneShotResearch
