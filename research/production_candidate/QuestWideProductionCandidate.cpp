// NONDEFAULT OG163 candidate. The accepted QA writer is left untouched.
// Separate authoritative evidence for ordinary top-level and Misc TESQuests.
#if !defined(K2040_QUESTWIDE_PRODUCTION_CANDIDATE) || defined(K2040_TRACKING_DRYRUN_QA)
#error Quest-wide candidate must be a separate non-QA opt-in build
#endif
#include "../../src/QuestWideProductionCandidate.h"
#include "../research/f4se_og_api/TaskInterface0623.h"
#include "../../src/QuestTrackingPolicy.h"
#include <RE/Fallout.h>
#include <F4SE/F4SE.h>
#include <REL/Relocation.h>
#include <Windows.h>
#ifdef ERROR
#undef ERROR
#endif
#include <charconv>
#include <string_view>
#include <utility>
namespace K2040::QuestWideCandidate {
namespace {
using Setter=void(*)(RE::TESQuest*,bool);
constexpr std::uint64_t kSetterId=1567562u;
constexpr std::uintptr_t kSetterRva=0x005D6AC0u;
static_assert(!K2040::QuestTracking::CanWritePipboy(),
              "This opt-in candidate must not alter the default writer policy");
}
bool Queue(const std::shared_ptr<Controller>& controller,
 const char* raw,std::uint64_t save,std::uint64_t view,
 K2040::OneShotResearch::ReadContext readFresh,Report report){
 if(!controller || !raw || !readFresh || !report)return false;
 const auto n=strnlen_s(raw,20);
 if(n!=19 || raw[10]!=':')return false;
 const auto parsed=K2040::QuestTracking::ParseRequest(std::string_view(raw,10));
 if(!parsed)return false;
 std::uint32_t instance{};
 const auto [end,err]=std::from_chars(raw+11,raw+19,instance,16);
 if(err!=std::errc{} || end!=raw+19)return false;
 const auto* tasks=F4SE::GetTaskInterface();
 if(!tasks || tasks->Version()!=K2040::OGF4SE::ABI0623::kTaskInterfaceVersion)
  return false;
 const auto id=parsed->formId;
 if(controller->Get().phase!=Phase::kIdle)return false;
 // Only owned primitives and callbacks cross into the F4SE task.
 tasks->AddTask([controller,id,instance,desired=parsed->desiredTracked,
                 save,view,readFresh=std::move(readFresh),report=std::move(report)]() mutable {
  using namespace K2040::OneShotResearch;
  Intent request{1u,id,desired,save,view};
  auto context=readFresh();
  auto observation=ObserveOnTask(request,context);
  auto evidence=observation.questType==kMisc
      ? CaptureGenericMiscEvidence(request)
      : CaptureOrdinaryQuestEvidence(request);
  const auto ticket=controller->Offer(request,observation,instance,evidence);
  if(!ticket){
   const auto phase=controller->Get().phase;
   if(phase==Phase::kIdle)report(id,Outcome::kRefused);
   else if(phase==Phase::kBlocked)report(id,Outcome::kBlocked);
   return;
  }
  context=readFresh();
  observation=ObserveOnTask(request,context);
  evidence=observation.questType==kMisc
      ? CaptureGenericMiscEvidence(request)
      : CaptureOrdinaryQuestEvidence(request);
  if(!controller->Claim(*ticket,observation,evidence)){
   // No setter was called while still Queued: rejection is safe to retry
   // after a fresh explicit user action. Executing/unknown is not.
   if(controller->CancelQueuedBeforeWrite(*ticket))report(id,Outcome::kRefused);
   else {controller->BlockUncertain(*ticket);report(id,Outcome::kBlocked);}
   return;
  }
  auto* quest=RE::TESForm::GetFormByID<RE::TESQuest>(id);
  REL::Relocation<Setter> setter{REL::ID(kSetterId)};
  const auto base=reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
  const bool eligible=base && setter.address()==base+kSetterRva &&
   quest && quest->GetFormID()==id && quest->currentInstanceID==instance &&
   static_cast<std::uint16_t>(quest->data.flags)==observation.questFlags &&
   ReviewQuestWide(request,observation,instance,evidence)==Result::kWouldWrite &&
   controller->Get().phase==Phase::kExecuting;
  if(!eligible){
   // A nonwrite can rearm only after proof that nothing was called.
   // Failed settlement is uncertainty, never a retryable refusal.
   if(controller->RefuseBeforeWrite(*ticket))report(id,Outcome::kRefused);
   else {controller->BlockUncertain(*ticket);report(id,Outcome::kBlocked);}
   return;
  }
  REX::INFO("QuestWideCandidate: SetActive ENTER desired={} (no FormID)",desired);
  setter(quest,desired);
  if(!controller->MarkWriteReturned(*ticket)){
   controller->BlockUncertain(*ticket);
   report(id,Outcome::kBlocked);return;
  }
  REX::INFO("QuestWideCandidate: SetActive RETURNED; refresh required");
  report(id,Outcome::kPending);
 });
 return true;
}
bool Reconcile(const std::shared_ptr<Controller>& controller,
 K2040::OneShotResearch::ReadContext readFresh,Report report){
 if(!controller || !readFresh || !report)return false;
 const auto status=controller->Get();
 if(status.phase!=Phase::kAwaitingRefresh || !status.pending)return false;
 const auto ticket=*status.pending;
 const auto context=readFresh();
 if(context.selectedId!=ticket.intent.formId ||
    context.saveEpoch!=ticket.intent.saveEpoch ||
    context.uiEpoch!=ticket.intent.uiEpoch){
  controller->BlockUncertain(ticket);
  report(ticket.intent.formId,Outcome::kBlocked);
  return false;
 }
 const auto after=K2040::OneShotResearch::ObserveOnTask(ticket.intent,context);
 const auto evidence=after.questType==K2040::OneShotResearch::kMisc
     ? K2040::OneShotResearch::CaptureGenericMiscEvidence(ticket.intent)
     : CaptureOrdinaryQuestEvidence(ticket.intent);
 if(!controller->ConfirmRefreshed(ticket,after,evidence)){
  if(controller->Get().phase==Phase::kBlocked)
   report(ticket.intent.formId,Outcome::kBlocked);
  return false;
 }
 REX::INFO("QuestWideCandidate: independent native/Pip-Boy readback confirmed");
 report(ticket.intent.formId,Outcome::kConfirmed);
 return true;
}
}
