// Nondefault candidate-only, read-only ordinary TESQuest evidence.
#if !defined(K2040_QUESTWIDE_PRODUCTION_CANDIDATE) || defined(K2040_TRACKING_DRYRUN_QA)
#error Ordinary evidence must not compile in the default/QA build
#endif
#include "../../src/QuestWideProductionCandidate.h"
#include "../../src/GenericQuestInstanceSummary.h"
#include <RE/Fallout.h>
#include <RE/P/PipboyDataManager.h>
namespace K2040::QuestWideCandidate {
namespace {
RE::PipboyValue* Member(RE::PipboyObject* obj,const char* key) {
    return obj?obj->GetMember<RE::PipboyValue*>(RE::BSFixedString(key)):nullptr;
}
bool TypedId(RE::PipboyObject* obj,const char* key,std::uint32_t& out) {
    auto* v=Member(obj,key);
    if(!v || (v->GetType()!=RE::PipboyValue::kUint32 &&
               v->GetType()!=RE::PipboyValue::kInt32))return false;
    out=static_cast<std::uint32_t>(
        *static_cast<RE::PipboyPrimitiveValue<std::uint32_t>*>(v));
    return true;
}
bool Tracked(RE::PipboyObject* obj,bool& out) {
    auto* v=Member(obj,"active");
    if(!v || v->GetType()!=RE::PipboyValue::kBool)return false;
    out=static_cast<bool>(*static_cast<RE::PipboyPrimitiveValue<bool>*>(v));
    return true;
}
}
K2040::OneShotResearch::GenericMiscEvidence CaptureOrdinaryQuestEvidence(
    const K2040::OneShotResearch::Intent& request) {
    using namespace K2040::OneShotResearch;
    GenericMiscEvidence result{};
    if(!request.formId)return result;
    auto* quest=RE::TESForm::GetFormByID<RE::TESQuest>(request.formId);
    auto* player=RE::PlayerCharacter::GetSingleton();
    auto* pip=RE::PipboyDataManager::GetSingleton();
    if(!quest || !player || !pip || !pip->questData.questArray ||
       static_cast<std::uint32_t>(quest->data.questType)==kMisc)
        return result;
    result.selectedIsOrdinary=true;
    result.questInstanceID=quest->currentInstanceID;
    K2040::GenericQuestInstance::Index instances;
    {
        RE::BSAutoLock lock{player->questTargetLock};
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
    const bool engineTracked=(static_cast<std::uint32_t>(quest->data.flags)&kTracked)!=0;
    bool complete=true,consistent=true;
    std::uint32_t scanned=0;
    for(auto* value:pip->questData.questArray->elements) {
        if(++scanned>512u || !value || value->GetType()!=RE::PipboyValue::kObject){
            complete=false;break;
        }
        auto* row=static_cast<RE::PipboyObject*>(value);
        std::uint32_t id{};
        if(!TypedId(row,"formID",id)){complete=false;break;}
        if(id!=request.formId)continue;
        if(++result.ordinaryFeedRows!=1u){complete=false;break;}
        bool active{};
        if(!Tracked(row,active) || active!=engineTracked)consistent=false;
        // Typed instance values must agree if the Pip-Boy includes one.
        if(Member(row,"instance")) {
            std::uint32_t typedInstance{};
            if(!TypedId(row,"instance",typedInstance) ||
               typedInstance!=quest->currentInstanceID)consistent=false;
        }
    }
    result.pipboyFeedComplete=complete && result.ordinaryFeedRows==1u;
    result.feedAgreesWithQuest=result.pipboyFeedComplete && consistent;
    return result;
}
}
