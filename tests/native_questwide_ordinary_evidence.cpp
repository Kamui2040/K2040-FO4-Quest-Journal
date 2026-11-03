// Compile the candidate's real native ordinary-feed reader against
// semantic RE test stubs; no Fallout executable, game state or writer.
#include "../src/QuestWideProductionCandidate.h"
#include <RE/Fallout.h>
#include <RE/P/PipboyDataManager.h>
#include <cassert>
using namespace K2040;
RE::PipboyObject* Row(std::uint32_t id,bool active) {
 auto* row=new RE::PipboyObject;
 row->members["formID"]=new RE::PipboyPrimitiveValue<std::uint32_t>(id);
 row->members["active"]=new RE::PipboyPrimitiveValue<bool>(active);
 return row;
}
int main(){
 RE::TESQuest quest;quest.formID=0x12345678;quest.data.questType=1;
 quest.data.flags=OneShotResearch::kEnabled;quest.currentInstanceID=0;
 RE::TESForm::forms[quest.formID]=&quest;
 RE::PlayerCharacter player;RE::PlayerCharacter::singleton=&player;
 RE::BGSQuestObjective objective;objective.index=20;objective.ownerQuest=&quest;
 player.objectives.push_back({&objective,0,{RE::QUEST_OBJECTIVE_STATE::kDisplayed}});
 RE::PipboyDataManager manager;RE::PipboyDataManager::singleton=&manager;
 RE::PipboyArray entries;manager.questData.questArray=&entries;
 RE::PipboyObject* row=Row(quest.formID,false);entries.elements.push_back(row);
 // An unrelated synthetic Misc folder is never mistaken for an ordinary row.
 auto* misc=Row(0,false);auto* sub=new RE::PipboyArray;
 sub->elements.push_back(Row(0xEE006789,true));misc->members["objectives"]=sub;
 entries.elements.push_back(misc);
 OneShotResearch::Intent request{1,quest.formID,true,1,2};
 auto evidence=QuestWideCandidate::CaptureOrdinaryQuestEvidence(request);
 assert(evidence.selectedIsOrdinary && !evidence.selectedIsMisc);
 assert(evidence.nativeComplete && evidence.onlyCurrentInstance);
 assert(evidence.nativeDisplayedCount==1 && evidence.ordinaryFeedRows==1);
 assert(evidence.pipboyFeedComplete && evidence.feedAgreesWithQuest);
 assert(player.questTargetLock.held==0);
 // A distinct top-level row for the same FormID is ambiguous.
 entries.elements.push_back(Row(quest.formID,false));
 evidence=QuestWideCandidate::CaptureOrdinaryQuestEvidence(request);
 assert(!evidence.pipboyFeedComplete && evidence.ordinaryFeedRows==2);
 entries.elements.pop_back();
 // Missing or nonboolean tracked bit must fail closed.
 row->members.erase("active");
 evidence=QuestWideCandidate::CaptureOrdinaryQuestEvidence(request);
 assert(!evidence.feedAgreesWithQuest);
 row->members["active"]=new RE::PipboyPrimitiveValue<bool>(true);
 evidence=QuestWideCandidate::CaptureOrdinaryQuestEvidence(request);
 assert(!evidence.feedAgreesWithQuest);
 row->members["active"]=new RE::PipboyPrimitiveValue<bool>(false);
 // Optional typed instance is enforced if present.
 row->members["instance"]=new RE::PipboyPrimitiveValue<std::uint32_t>(1);
 evidence=QuestWideCandidate::CaptureOrdinaryQuestEvidence(request);
 assert(!evidence.feedAgreesWithQuest);
 row->members["instance"]=new RE::PipboyPrimitiveValue<std::uint32_t>(0);
 evidence=QuestWideCandidate::CaptureOrdinaryQuestEvidence(request);
 assert(evidence.feedAgreesWithQuest);
 // Different native objective instance cannot validate the same quest.
 player.objectives.push_back({&objective,1,{RE::QUEST_OBJECTIVE_STATE::kDisplayed}});
 evidence=QuestWideCandidate::CaptureOrdinaryQuestEvidence(request);
 assert(!evidence.onlyCurrentInstance);
 player.objectives.pop_back();
 // One failed/unavailable player observation never authorizes a write.
 RE::PlayerCharacter::singleton=nullptr;
 evidence=QuestWideCandidate::CaptureOrdinaryQuestEvidence(request);
 assert(!evidence.nativeComplete && !evidence.pipboyFeedComplete);
 RE::PlayerCharacter::singleton=&player;
 // The ordinary reader never accepts a Misc owning quest.
 quest.data.questType=OneShotResearch::kMisc;
 evidence=QuestWideCandidate::CaptureOrdinaryQuestEvidence(request);
 assert(!evidence.selectedIsOrdinary);
}
