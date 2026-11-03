// Compiled ONLY in dedicated default-OFF repeatable Misc QA flavor.
#if !defined(K2040_REPEATABLE_MISC_QA) || !defined(K2040_TRACKING_DRYRUN_QA)
#error Repeatable native quest writer is opt-in QA only
#endif
#include "RepeatableOgWriteQA.h"
#include "../f4se_og_api/TaskInterface0623.h"
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
#include <atomic>
#include <utility>
namespace K2040::RepeatableQuestWide {
namespace {
using Setter=void(*)(RE::TESQuest*,bool);
constexpr std::uint64_t kSetterId=1567562u;
constexpr std::uintptr_t kSetterRva=0x005D6AC0u;
std::atomic_bool sessionConsent{false};
std::wstring ConsentIni(){
 wchar_t module[MAX_PATH]{};
 auto count=GetModuleFileNameW(nullptr,module,MAX_PATH);
 if(!count || count>=MAX_PATH)return {};
 std::wstring path(module,count);
 auto slash=path.find_last_of(L"\\/");
 if(slash==std::wstring::npos)return {};
 path.resize(slash+1);
 return path+L"Data\\F4SE\\Plugins\\K2040_Quest_Journal_Write_QA.ini";
}
}
void GrantSessionConsent(){
 sessionConsent.store(true,std::memory_order_release);
}
void ResetSessionConsent(){
 sessionConsent.store(false,std::memory_order_release);
}
bool ReadRepeatableConsent(){
 if(sessionConsent.load(std::memory_order_acquire))return true;
 auto path=ConsentIni();if(path.empty())return false;
 const auto yes=[&path](const wchar_t* key){
  wchar_t value[12]{};
  const auto n=GetPrivateProfileStringW(L"Safety",key,L"NO",value,12,path.c_str());
  return n==3 && std::wcscmp(value,L"YES")==0;
 };
 return yes(L"IHaveBackedUpTestSave") &&
        yes(L"PermitOneNativeSetActiveCall") &&
        yes(L"PermitRepeatableQuestTracking");
}
bool QueueRepeatable(const std::shared_ptr<Controller>& controller,
 const char* raw,std::uint64_t save,std::uint64_t view,
 K2040::OneShotResearch::ReadContext readFresh,Report report){
 if(!controller || !raw || !readFresh || !report || !ReadRepeatableConsent())return false;
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
 // Only values and owned callbacks cross into the F4SE game task.
 tasks->AddTask([controller,id,instance,desired=parsed->desiredTracked,
                 save,view,readFresh=std::move(readFresh),report=std::move(report)]() mutable {
  using namespace K2040::OneShotResearch;
  Intent request{1u,id,desired,save,view};
  auto context=readFresh();
  auto observation=ObserveOnTask(request,context);
  auto evidence=CaptureGenericMiscEvidence(request);
  const auto ticket=controller->Offer(request,observation,instance,evidence);
  if(!ticket){
   // A duplicate task while another operation is in flight must not
   // overwrite that operation's pending/confirmed browser result.
   const auto phase=controller->Get().phase;
   if(phase==Phase::kIdle)report(id,Outcome::kRefused);
   else if(phase==Phase::kBlocked)report(id,Outcome::kBlocked);
   return;
  }
  context=readFresh();
  observation=ObserveOnTask(request,context);
  evidence=CaptureGenericMiscEvidence(request);
  if(!controller->Claim(*ticket,observation,evidence)){
   // An unexpected stale/unknown claim remains blocked or queued, never
   // write anything just to recover a bad ticket.
   controller->BlockUncertain(*ticket);
   report(id,Outcome::kBlocked);return;
  }
  auto* quest=RE::TESForm::GetFormByID<RE::TESQuest>(id);
  REL::Relocation<Setter> setter{REL::ID(kSetterId)};
  const auto base=reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
  const bool eligible=ReadRepeatableConsent() && base &&
   setter.address()==base+kSetterRva && quest && quest->GetFormID()==id &&
   quest->currentInstanceID==instance &&
   static_cast<std::uint16_t>(quest->data.flags)==observation.questFlags &&
   ReviewGenericMisc(request,observation,instance,evidence)==Result::kWouldWrite &&
   controller->Get().phase==Phase::kExecuting;
  if(!eligible){
   (void)controller->RefuseBeforeWrite(*ticket);
   report(id,Outcome::kRefused);return;
  }
  REX::WARN("QuestRepeatableQA: SetActive ENTER desired={} (no FormID)",desired);
  setter(quest,desired);
  if(!controller->MarkWriteReturned(*ticket)){
   controller->BlockUncertain(*ticket);
   report(id,Outcome::kBlocked);return;
  }
  REX::WARN("QuestRepeatableQA: SetActive RETURNED; refresh required");
  report(id,Outcome::kPending); // NEVER claim confirmation here
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
  // The original selection or session vanished after a possible write.
  // Never silently unlock or retry a request with uncertain readback.
  controller->BlockUncertain(ticket);
  report(ticket.intent.formId,Outcome::kBlocked);
  return false;
 }
 const auto after=K2040::OneShotResearch::ObserveOnTask(ticket.intent,context);
 const auto evidence=K2040::OneShotResearch::CaptureGenericMiscEvidence(ticket.intent);
 if(!controller->ConfirmRefreshed(ticket,after,evidence)){
  // A temporarily lagging feed may be re-read (never re-written) at
  // most three times. Identity drift or exhausted evidence blocks.
  if(controller->Get().phase==Phase::kBlocked)
   report(ticket.intent.formId,Outcome::kBlocked);
  return false;
 }
 REX::INFO("QuestRepeatableQA: fresh native/Pip-Boy readback CONFIRMED");
 report(ticket.intent.formId,Outcome::kConfirmed);
 return true;
}
}
