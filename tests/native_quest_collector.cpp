// Synthetic quest data contract. No engine ABI or game binaries are present.
#include "QuestCollector.h"
#include "QuestViewState.h"
#include <RE/Fallout.h>
#include <F4SE/Interfaces.h>
#include <cassert>
#include <string>
namespace F4SE {
const SerializationInterface* g_archive{};
const SerializationInterface* GetSerializationInterface() noexcept{return g_archive;}
}
static RE::PipboyObject* Entry(std::uint32_t id,const char* name,const char* desc,bool tracked) {
 auto* entry=new RE::PipboyObject;
 entry->members["formID"]=new RE::PipboyPrimitiveValue<std::uint32_t>(id);
 entry->members["text"]=new RE::PipboyString(name);
 entry->members["desc"]=new RE::PipboyString(desc);
 entry->members["active"]=new RE::PipboyPrimitiveValue<bool>(tracked);
 return entry;
}
int main(){
 using namespace K2040;
 RE::PipboyDataManager manager;RE::PipboyDataManager::singleton=&manager;
 RE::PlayerCharacter player;RE::PlayerCharacter::singleton=&player;
 RE::PipboyArray entries;manager.questData.questArray=&entries;
 RE::TESQuest mainQuest;mainQuest.displayName="Test Quest";mainQuest.data.questType=1;
 RE::TESQuest miscQuest;miscQuest.displayName="Misc Job";miscQuest.data.questType=6;
 RE::BGSQuestObjective objective;objective.index=20;
 objective.state=RE::QUEST_OBJECTIVE_STATE::kDisplayed;
 objective.displayText=RE::BSFixedString("Reach the target");
 mainQuest.objectives.push_back(&objective);
 RE::BGSQuestObjective miscWanted,miscOther;
 miscWanted.index=42;miscWanted.state=RE::QUEST_OBJECTIVE_STATE::kDisplayed;
 miscWanted.displayText=RE::BSFixedString("Collect the marked item");
 miscOther.index=43;miscOther.state=RE::QUEST_OBJECTIVE_STATE::kDisplayed;
 miscOther.displayText=RE::BSFixedString("Do not show unrelated objective");
 miscQuest.objectives.push_back(&miscWanted);
 miscQuest.objectives.push_back(&miscOther);
 miscQuest.currentInstanceID=17;
 miscWanted.ownerQuest=&miscQuest;
 miscOther.ownerQuest=&miscQuest;
 player.objectives.push_back({&miscWanted,17,{RE::QUEST_OBJECTIVE_STATE::kDisplayed}});
 player.objectives.push_back({&miscOther,17,{RE::QUEST_OBJECTIVE_STATE::kDisplayed}});
 // Also exercise the native name fallback; unlike the old stub it returns string_view.
 assert(RE::TESFullName::GetFullName(mainQuest) == std::string_view("Test Quest"));
 mainQuest.formID=0x12345678;
 miscQuest.formID=0x12345679;
 RE::TESForm::forms[0x12345678]=&mainQuest;
 RE::TESForm::forms[0x12345679]=&miscQuest;
 auto* mainRow=Entry(0x12345678,"Test Quest","Description",true);
 mainRow->members["type"]=new RE::PipboyPrimitiveValue<std::uint32_t>(1);
 mainRow->members["instance"]=new RE::PipboyPrimitiveValue<std::uint32_t>(0);
 // Regular quest objectives are display leaves, not selectable
 // Misc quest-command rows. The v1 diagnostic misclassified these.
 auto* regularObjectives=new RE::PipboyArray;
 auto* displayLeaf=new RE::PipboyObject;
 displayLeaf->members["completed"]=new RE::PipboyPrimitiveValue<bool>(false);
 displayLeaf->members["enabled"]=new RE::PipboyPrimitiveValue<bool>(true);
 displayLeaf->members["text"]=new RE::PipboyString("Ordinary objective display");
 regularObjectives->elements.push_back(displayLeaf);
 mainRow->members["objectives"]=regularObjectives;
 entries.elements.push_back(mainRow);
 auto* folder=new RE::PipboyObject;
 folder->members["formID"]=new RE::PipboyPrimitiveValue<std::uint32_t>(0);
 folder->members["type"]=new RE::PipboyPrimitiveValue<std::uint32_t>(6);
 auto* nested=new RE::PipboyArray;
 auto* marked=Entry(0x12345679,"Misc Job","",false);
 marked->members["Obj"]=new RE::PipboyPrimitiveValue<std::uint32_t>(42);
 marked->members["instance"]=new RE::PipboyPrimitiveValue<std::uint32_t>(17);
 marked->members["type"]=new RE::PipboyPrimitiveValue<std::uint32_t>(6);
 marked->members["unknownIndexKey"]=new RE::PipboyPrimitiveValue<std::uint32_t>(99123);
 nested->elements.push_back(marked);
 auto* second=Entry(0x12345679,"Misc Job","",true);
 second->members["Obj"]=new RE::PipboyPrimitiveValue<std::uint32_t>(43);
 second->members["instance"]=new RE::PipboyPrimitiveValue<std::uint32_t>(17);
 second->members["type"]=new RE::PipboyPrimitiveValue<std::uint32_t>(6);
 nested->elements.push_back(second);
 // Repeating the same objective in the Pip-Boy must not duplicate the UI row.
 auto* repeat=Entry(0x12345679,"Misc Job","",false);
 repeat->members["Obj"]=new RE::PipboyPrimitiveValue<std::uint32_t>(42);
 nested->elements.push_back(repeat);
 folder->members["objectives"]=nested;
 entries.elements.push_back(folder);
 const std::unordered_set<std::uint32_t> pinned{0x12345678},none;
 auto json=Quests::CaptureSnapshot(pinned,none,none);
 assert(json.find("\"ready\":true")!=std::string::npos);
 assert(json.find("Test Quest")!=std::string::npos);
 assert(json.find("Reach the target")!=std::string::npos);
 assert(json.find("\"isMisc\":true")!=std::string::npos);
 assert(json.find("\"trackingScope\":\"quest-wide\"")!=std::string::npos);
 assert(json.find("\"nativeInstanceStatus\":\"consistent\"")!=std::string::npos);
 assert(json.find("\"nativeInstanceID\":17")!=std::string::npos);
 assert(json.find("\"nativeDisplayedCount\":2")!=std::string::npos);
 assert(json.find("\"miscRowCount\":3")!=std::string::npos);
 assert(json.find("\"pipboyFeedAgreement\":\"mismatch\"")!=std::string::npos);
 assert(json.find("\"trackingReadiness\":\"pipboy-feed-unverified\"")!=std::string::npos);
 assert(json.find("Collect the marked item")!=std::string::npos);
 // A second visible Misc row for the same TESQuest must contribute its
 // own objective, rather than being discarded by FormID deduplication.
 assert(json.find("Do not show unrelated objective")!=std::string::npos);
 assert(json.find("Collect the marked item")!=std::string::npos);
 assert(json.find("Misc Job")==json.rfind("Misc Job"));
 assert(json.find("Collect the marked item")==json.rfind("Collect the marked item"));
 assert(json.find("Do not show unrelated objective")==json.rfind("Do not show unrelated objective"));
 assert(QuestViewState::Observed().contains(0x12345678));
 assert(QuestViewState::Observed().contains(0x12345679));
 // A native type-6 Misc quest can be delivered as a top-level Pip-Boy row.
 // It must still be marked isMisc, preventing unsupported quest-level writes.
 RE::TESQuest topLevelMisc;
 topLevelMisc.displayName="Root-level Misc Job";
 topLevelMisc.data.questType=6;
 RE::TESForm::forms[0x1234567C]=&topLevelMisc;
 entries.elements.push_back(Entry(0x1234567C,"Root-level Misc Job","",false));
 const auto rootMiscJson=Quests::CaptureSnapshot(pinned,none,none);
 const auto rootMiscBegin=rootMiscJson.find("\"id\":\"1234567C\"");
 assert(rootMiscBegin!=std::string::npos);
 const auto rootMiscEnd=rootMiscJson.find('}',rootMiscBegin);
 assert(rootMiscEnd!=std::string::npos);
 const auto rootMiscRow=rootMiscJson.substr(rootMiscBegin,rootMiscEnd-rootMiscBegin);
 assert(rootMiscRow.find("\"sourceType\":\"Misc\"")!=std::string::npos);
 assert(rootMiscRow.find("\"isMisc\":true")!=std::string::npos);
 assert(rootMiscRow.find("\"trackingReadiness\":")!=std::string::npos);
#ifdef K2040_SELECTOR_READONLY_QA
 // Dynamic type/instance correlation over the complete synthetic feed;
 // the candidate labels DO NOT authorize a native quest toggle.
 const auto survey=Quests::CaptureSelectorRowCorrelation();
 assert(survey.find("\"available\":true")!=std::string::npos);
 assert(survey.find("\"rootRows\":2")!=std::string::npos);
 assert(survey.find("\"auditVersion\":2")!=std::string::npos);
 assert(survey.find("\"miscRows\":3")!=std::string::npos);
 assert(survey.find("\"ordinaryObjectiveRows\":1")!=std::string::npos);
 assert(survey.find("\"unknownParentChildren\":0")!=std::string::npos);
 assert(survey.find("\"schemaObjectiveDisplay\":[")!=std::string::npos);
 assert(survey.find("\"syntheticRows\":1")!=std::string::npos);
 assert(survey.find("\"typedRootTypeFields\":1")!=std::string::npos);
 assert(survey.find("\"typedMiscTypeFields\":2")!=std::string::npos);
 assert(survey.find("\"kind\":\"structural-candidate-NOT-command-ABI\",\"count\":3")!=std::string::npos);
 assert(survey.find("\"kind\":\"misc-root-type-six-candidate\",\"count\":1")!=std::string::npos);
 assert(survey.find("12345679")==std::string::npos);
 assert(survey.find("Misc Job")==std::string::npos);
 assert(survey.size()<4096u);
 // Two different typed keys must not be silently treated as one
 // valid native field when they disagree.
 marked->members["questType"]=new RE::PipboyPrimitiveValue<std::uint32_t>(5);
 const auto conflicting=Quests::CaptureSelectorRowCorrelation();
 assert(conflicting.find("\"kind\":\"conflicting-row-type-keys\",\"count\":1")!=std::string::npos);
 marked->members.erase("questType");
 // Unknown parent identity must not promote its nested display leaf
 // into an alleged selectable Misc quest-command entry.
 auto* unknownParent=new RE::PipboyObject;
 auto* unknownChildren=new RE::PipboyArray;
 unknownChildren->elements.push_back(new RE::PipboyObject);
 unknownParent->members["objectives"]=unknownChildren;
 entries.elements.push_back(unknownParent);
 const auto orphaned=Quests::CaptureSelectorRowCorrelation();
 assert(orphaned.find("\"unknownParentChildren\":1")!=std::string::npos);
 assert(orphaned.find("\"miscRows\":3")!=std::string::npos);
 entries.elements.pop_back();
#endif
#ifdef K2040_TRACKING_INTEGRATED_QA
 // Real nested Misc Pip-Boy entries can have DIFFERENT objective flags.
 // Reading a Misc objective must not toggle the whole TESQuest.
 const auto probeNested=Quests::CaptureMiscTrackingProbe(0x12345679);
 assert(probeNested.find("\"available\":true")!=std::string::npos);
 assert(probeNested.find("\"miscRows\":3")!=std::string::npos);
 assert(probeNested.find(
     "\"genericCandidateStatuses\":[\"candidate-NOT-native-command\","
     "\"candidate-NOT-native-command\",\"missing-instance\"]")
     !=std::string::npos);
 assert(probeNested.find("\"miscTracked\":1")!=std::string::npos);
 assert(probeNested.find("\"index\":42")!=std::string::npos);
 assert(probeNested.find("\"index\":43")!=std::string::npos);
 assert(probeNested.find("\"Obj:4\"")!=std::string::npos);
 assert(probeNested.find("\"unknownIndexKey:4\"")!=std::string::npos);
 assert(probeNested.find("99123")==std::string::npos);
 assert(probeNested.find("\"fieldSchema\":[")!=std::string::npos);
 assert(probeNested.find("\"displayedObjectiveCandidates\":[42,43]")!=std::string::npos);
 assert(probeNested.find("\"nativeObjectiveCollectionAvailable\":true")!=std::string::npos);
 assert(probeNested.find("\"nativeObjectiveCollectionComplete\":true")!=std::string::npos);
 assert(probeNested.find("\"nativeCurrentInstanceID\":17")!=std::string::npos);
 assert(probeNested.find("\"nativeObjectiveInstances\":[{\"index\":42,\"instanceID\":17},{\"index\":43,\"instanceID\":17}]")!=std::string::npos);
 assert(probeNested.find("\"nativeCandidateStatus\":\"single-candidate-not-verified\"")!=std::string::npos);
 assert(probeNested.find("\"nativeCandidateStatus\":\"missing-instance\"")!=std::string::npos);
 assert(player.questTargetLock.held==0);
 assert(probeNested.size()<4096u);
 // The actual QA collector also rejects ambiguous instance-only rows.
 auto* ambiguousRow=Entry(0x12345679,"Misc Job","",false);
 ambiguousRow->members["instance"]=new RE::PipboyPrimitiveValue<std::uint32_t>(17);
 nested->elements.push_back(ambiguousRow);
 const auto ambiguousProbe=Quests::CaptureMiscTrackingProbe(0x12345679);
 assert(ambiguousProbe.find("\"nativeCandidateStatus\":\"ambiguous\"")!=std::string::npos);
 assert(ambiguousProbe.find("\"nativeCandidateCount\":2")!=std::string::npos);
 RE::PlayerCharacter::singleton=nullptr;
 const auto noPlayer=Quests::CaptureMiscTrackingProbe(0x12345679);
 assert(noPlayer.find("\"nativeObjectiveCollectionAvailable\":false")!=std::string::npos);
 assert(noPlayer.find("\"nativeCandidateStatus\":\"incomplete\"")!=std::string::npos);
 RE::PlayerCharacter::singleton=&player;
 assert(player.questTargetLock.held==0 && miscQuest.data.flags==0);
 assert(probeNested.find("Collect the marked item")==std::string::npos);
 const auto probeRoot=Quests::CaptureMiscTrackingProbe(0x1234567C);
 assert(probeRoot.find("\"available\":true")!=std::string::npos);
 assert(probeRoot.find("\"rootRows\":1")!=std::string::npos);
 assert(probeRoot.find("\"miscRows\":0")!=std::string::npos);
 assert(Quests::CaptureMiscTrackingProbe(0x12345678).find(
     "\"available\":false")!=std::string::npos);
 // The read-only probe must not change any engine or Pip-Boy flags.
 assert(mainQuest.data.questType==1 && miscQuest.data.questType==6);
#ifdef K2040_SINGLE_MISC_WRITE_QA
 // The pre-existing multirow Misc fixture must be refused by the writer.
 const auto multi=Quests::InspectSingleMiscWriteCandidate(0x12345679);
 assert(multi.exactlyOneNestedRow==false);
 // A separate test-save-shaped quest with ONE native index=50 and one
 // nested row with typed instance=1 (but no objective index).
 RE::TESQuest single;single.data.questType=6;single.data.flags=0x801;
 single.currentInstanceID=1;
 RE::BGSQuestObjective dog;dog.index=50;
 dog.ownerQuest=&single;dog.state=RE::QUEST_OBJECTIVE_STATE::kDisplayed;
 single.objectives.push_back(&dog);
 player.objectives.push_back({&dog,1,{RE::QUEST_OBJECTIVE_STATE::kDisplayed}});
 single.formID=K2040::SingleMiscQA::kVerifiedQuestFormID;
 RE::TESForm::forms[single.formID]=&single;
 auto* row=Entry(single.formID,"Selected single Misc","",true);
 row->members["instance"]=new RE::PipboyPrimitiveValue<std::uint32_t>(1);
 row->members["enabled"]=new RE::PipboyPrimitiveValue<bool>(true);
 row->members["completed"]=new RE::PipboyPrimitiveValue<bool>(false);
 row->members["failed"]=new RE::PipboyPrimitiveValue<bool>(false);
 auto* singleFolder=new RE::PipboyObject;
 singleFolder->members["formID"]=new RE::PipboyPrimitiveValue<std::uint32_t>(0);
 auto* singleRows=new RE::PipboyArray;
 singleRows->elements.push_back(row);
 singleFolder->members["objectives"]=singleRows;
 entries.elements.push_back(singleFolder);
 auto proof=Quests::InspectSingleMiscWriteCandidate(single.formID);
 assert(proof.complete && !proof.truncated);
 assert(proof.exactlyOneNestedRow && proof.exactlyOneNativeDisplayed);
 assert(proof.nativeIndex==50 && proof.nativeInstance==1);
 assert(proof.nestedInstance==1 && proof.currentInstance==1);
 proof.validSession=true;
 using namespace K2040::SingleMiscQA;
 assert(Review(proof,false)==Verdict::kReady);
 assert(Review(proof,true)==Verdict::kAlready);
 assert(player.questTargetLock.held==0);
 row->members["enabled"]=new RE::PipboyPrimitiveValue<bool>(false);
 proof=Quests::InspectSingleMiscWriteCandidate(single.formID);
 proof.validSession=true;
 assert(Review(proof,false)==Verdict::kReject);
 row->members["enabled"]=new RE::PipboyPrimitiveValue<bool>(true);
 singleRows->elements.push_back(Entry(single.formID,"Duplicate","",true));
 proof=Quests::InspectSingleMiscWriteCandidate(single.formID);
 proof.validSession=true;
 assert(Review(proof,false)==Verdict::kReject);
 assert(player.questTargetLock.held==0);
#endif
#endif
 mainQuest.data.flags=static_cast<std::uint16_t>(RE::QuestFlag::kCompleted);
 entries.elements.clear();
 json=Quests::CaptureSnapshot(pinned,none,none);
 assert(json.find("\"historyOnly\":true")!=std::string::npos);
 assert(json.find("Test Quest")!=std::string::npos);
 assert(json.find("Misc Job")==std::string::npos);
 // Failed is bit 0x40 in the *real* CommonLibF4 QuestFlag header, not 0x04.
 mainQuest.data.flags=static_cast<std::uint16_t>(RE::QuestFlag::kFailed);
 json=Quests::CaptureSnapshot(pinned,none,none);
 assert(json.find("\"state\":\"failed\"")!=std::string::npos);
 assert(json.find("Test Quest")!=std::string::npos);
 RE::TESQuest secret;secret.displayName="Undiscovered Entry";
 secret.data.flags=static_cast<std::uint16_t>(RE::QuestFlag::kCompleted);
 RE::TESForm::forms[0x1234567A]=&secret;
 assert(Quests::CaptureSnapshot(pinned,none,none).find("Undiscovered Entry")==std::string::npos);
 // Auto-add starts after the initial valid Pip-Boy feed. The existing
 // journal never automatically changes a Pip-Boy tracked flag.
 QuestViewState::Clear();
 mainQuest.data.flags=0;
 entries.elements.clear();
 entries.elements.push_back(Entry(0x12345678,"Existing Quest","",false));
 auto baseline=Quests::CaptureSnapshot(none,none,none,true);
 assert(QuestViewState::InitialFeedSeen());
 assert(QuestViewState::Observed().contains(0x12345678));
 assert(!QuestViewState::Get(QuestViewState::Kind::Pin).contains(0x12345678));
 RE::TESQuest newQuest;newQuest.displayName="Newly Received";newQuest.data.questType=2;
 RE::TESForm::forms[0x1234567B]=&newQuest;
 entries.elements.push_back(Entry(0x1234567B,"Newly Received","",false));
 auto fresh=Quests::CaptureSnapshot(none,none,none,true);
 assert(QuestViewState::Get(QuestViewState::Kind::Pin).contains(0x1234567B));
 assert(QuestViewState::Toggle(QuestViewState::Kind::Pin,0x1234567B));
 assert(!QuestViewState::Get(QuestViewState::Kind::Pin).contains(0x1234567B));
 auto afterManualUnpin=Quests::CaptureSnapshot(none,none,none,true);
 assert(!QuestViewState::Get(QuestViewState::Kind::Pin).contains(0x1234567B));
 QuestViewState::Clear();
 assert(QuestViewState::Observed().empty());
}
