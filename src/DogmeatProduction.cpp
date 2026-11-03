// Production-candidate native implementation. Does not include or link QA
// task/consent/logging code. Opt-in; ordinary builds retain deny-all writes.
#ifdef K2040_DOGMEAT_PRODUCTION_CANDIDATE
#include <RE/Fallout.h>
#include <Windows.h>
#include "DogmeatProduction.h"
#include "QuestCollector.h"
#include "../research/native_one_shot/SingleMiscWriteGate.h"
#include "../research/native_one_shot/MiscRuntimeDispatchProbe.h"

namespace K2040::DogmeatProduction {
Result ExecuteVerifiedToggle(const std::uint32_t formID,
                             const bool desiredTracked,
                             const std::function<bool()>& validSession) {
    using namespace K2040;
    if (formID!=SingleMiscQA::kVerifiedQuestFormID || !validSession())
        return Result::kRejected;

    // Snapshot ALL engine-managed quest/objective/Pip-Boy evidence. The
    // pure gate fails closed on unexpected rows, flags or instance IDs.
    auto before=Quests::InspectSingleMiscWriteCandidate(formID);
    before.validSession=validSession();
    const auto decision=SingleMiscQA::Review(before,desiredTracked);
    if(decision==SingleMiscQA::Verdict::kAlready)
        return Result::kAlready;
    if(decision!=SingleMiscQA::Verdict::kReady)
        return Result::kRejected;

    // This is an undocumented OG163 command helper. Never invoke it on a
    // different runtime, changed image, unchecked relocation or stale view.
    const auto image=MiscVanillaResearch::CaptureRuntimeCode();
    const auto base=reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    using Setter=void(*)(RE::TESQuest*,bool);
    REL::Relocation<Setter> setter{REL::ID(1567562u)};
    if(!image || !image->linkedHelper ||
       image->helperStatus!=
           MiscVanillaResearch::LinkedHelperStatus::kCaptured ||
       image->entries[3].rva!=0x009FB850u ||
       image->linkedHelper->rva!=
           MiscVanillaResearch::kOGLinkedHelperRva ||
       !base || setter.address()!=base+0x005D6AC0u ||
       !validSession())
        return Result::kRejected;

    // Repeat the full live read immediately before any game-state write.
    auto again=Quests::InspectSingleMiscWriteCandidate(formID);
    again.validSession=validSession();
    if(SingleMiscQA::Review(again,desiredTracked)!=
           SingleMiscQA::Verdict::kReady ||
       again.questFlags!=before.questFlags ||
       again.currentInstance!=before.currentInstance)
        return Result::kRejected;
    auto* quest=RE::TESForm::GetFormByID<RE::TESQuest>(formID);
    if(!quest || quest->GetFormID()!=formID ||
       static_cast<std::uint32_t>(quest->data.flags)!=before.questFlags ||
       !validSession())
        return Result::kRejected;

    // Verified by separate manual game testing (track/untrack, compass,
    // and save/reload) only for this exact DogmeatQuest data shape.
    using VanillaToggle=void(*)(std::uint32_t,std::uint32_t,std::uint32_t);
    auto toggle=reinterpret_cast<VanillaToggle>(
        base+MiscVanillaResearch::kOGLinkedHelperRva);
    toggle(formID,SingleMiscQA::kObservedInstance,6u);
    return Result::kProvisional; // UI must still observe fresh game state.
}
}
#endif
