#include "../src/GenericQuestTrackingScope.h"
#include "../src/QuestTrackingPolicy.h"
#include <cassert>
#include <string_view>

namespace G=K2040::GenericQuestTrackingScope;
using G::Outcome;
int main() {
    G::Observation o{};
    // Dynamic ESL/mod FormID -- deliberately no whitelist.
    o.selectedFormID=o.observedFormID=0xFE003ABC;
    o.selectedInstanceID=o.currentInstanceID=19u;
    o.selection=G::Selection::kMiscObjective;
    o.selectedInstanceTyped=true;
    o.runtimeOG163Verified=true;
    o.completePipboySnapshot=true;
    o.freshGameAndView=true;
    o.questRunning=true;
    o.currentTrackingKnown=true;
    o.currentlyTracked=false;
    o.desiredTracked=true;
    o.displayedMiscRowsForQuest=1;
    // A normal feed row must never pass as a fully verified native
    // selectedEntry, even if FormID/type/instance fields all agree.
    assert(G::Review(o)==Outcome::kCommandInputUnverified);
    o.commandOrigin=G::CommandOrigin::kQuestDataFeed;
    o.inGameCallbackArgumentsVerified=true;
    assert(G::Review(o)==Outcome::kCommandInputUnverified);
    o.commandOrigin=G::CommandOrigin::kObjectiveDisplayLeaf;
    assert(G::Review(o)==Outcome::kCommandInputUnverified);
    // Synthetic only: no real vanilla selectedEntry capture is asserted.
    o.commandOrigin=G::CommandOrigin::kNativePopulatedMiscSelectedEntry;
    o.inGameCallbackArgumentsVerified=false;
    assert(G::Review(o)==Outcome::kCommandInputUnverified);
    o.inGameCallbackArgumentsVerified=true;
    // Even with a schema and a synthetic callback=true flag, we require
    // independent observation of the in-game code-object callback ABI.
    assert(G::Review(o)==Outcome::kCommandInputUnverified);
    o.inGameCallbackEvidence=G::InGameCallbackEvidence::kSelectedEntrySchemaOnly;
    assert(G::Review(o)==Outcome::kCommandInputUnverified);
    o.inGameCallbackEvidence=G::InGameCallbackEvidence::kBGSCodeObjCallbackAbiObserved;
    o.commandChannel=G::CommandChannel::kCompanionAppRpcType5;
    assert(G::Review(o)==Outcome::kCommandInputUnverified);
    o.commandChannel=G::CommandChannel::kInGameScaleformCodeObject;
    // Synthetic policy test ONLY: no current OG163 capture has this proof.
    assert(G::Review(o)==Outcome::kQuestInstanceCandidateReadOnly);

    o.displayedMiscRowsForQuest=3;
    assert(G::Review(o)==Outcome::kRequiresExplicitQuestWideSelection);
    o.explicitQuestWideIntent=true;
    assert(G::Review(o)==Outcome::kQuestInstanceCandidateReadOnly);
    o.explicitQuestWideIntent=false;
    o.selection=G::Selection::kQuest;
    assert(G::Review(o)==Outcome::kCommandInputUnverified);
    o.commandOrigin=G::CommandOrigin::kQuestListSelectedEntry;
    assert(G::Review(o)==Outcome::kQuestInstanceCandidateReadOnly);
    o.commandOrigin=G::CommandOrigin::kNativePopulatedMiscSelectedEntry;
    assert(G::Review(o)==Outcome::kCommandInputUnverified);
    o.commandOrigin=G::CommandOrigin::kQuestListSelectedEntry;

    // Current instance, not objective number, is checked.
    o.selectedInstanceID=20;
    assert(G::Review(o)==Outcome::kWrongInstance);
    o.selectedInstanceID=19;
    o.selectedInstanceTyped=false;
    assert(G::Review(o)==Outcome::kWrongInstance);
    o.selectedInstanceTyped=true;

    o.currentTrackingKnown=false;
    assert(G::Review(o)==Outcome::kStaleOrIncomplete);
    o.currentTrackingKnown=true;
    o.runtimeOG163Verified=false;
    assert(G::Review(o)==Outcome::kStaleOrIncomplete);
    o.runtimeOG163Verified=true;
    o.freshGameAndView=false;
    assert(G::Review(o)==Outcome::kStaleOrIncomplete);
    o.freshGameAndView=true;
    o.completePipboySnapshot=false;
    assert(G::Review(o)==Outcome::kStaleOrIncomplete);
    o.completePipboySnapshot=true;

    o.questRunning=false;
    assert(G::Review(o)==Outcome::kQuestNotRunning);
    o.questRunning=true;
    o.questCompleted=true;
    assert(G::Review(o)==Outcome::kQuestNotRunning);
    o.questCompleted=false;
    o.questFailed=true;
    assert(G::Review(o)==Outcome::kQuestNotRunning);
    o.questFailed=false;

    o.currentlyTracked=o.desiredTracked;
    assert(G::Review(o)==Outcome::kAlreadyInRequestedState);
    o.currentlyTracked=false;
    o.selection=G::Selection::kMiscObjective;
    o.commandOrigin=G::CommandOrigin::kNativePopulatedMiscSelectedEntry;
    o.displayedMiscRowsForQuest=0;
    assert(G::Review(o)==Outcome::kStaleOrIncomplete);
    o.displayedMiscRowsForQuest=2;
    assert(G::Review(o)==Outcome::kRequiresExplicitQuestWideSelection);
    o.observedFormID=0;
    assert(G::Review(o)==Outcome::kInvalidIdentity);
    o.observedFormID=o.selectedFormID;
    o.selectedFormID=0; // synthetic Misc root can never be activated.
    assert(G::Review(o)==Outcome::kInvalidIdentity);
    o.selectedFormID=o.observedFormID;
    o.selection=G::Selection::kUnknown;
    assert(G::Review(o)==Outcome::kInvalidIdentity);

    o.inGameCallbackEvidence=G::InGameCallbackEvidence::kSelectedEntrySchemaOnly;
    o.selection=G::Selection::kQuest;
    o.commandOrigin=G::CommandOrigin::kQuestListSelectedEntry;
    o.selectedFormID=o.observedFormID=0xFE003ABC;
    o.selectedInstanceID=o.currentInstanceID;
    o.currentlyTracked=false;
    o.desiredTracked=true;
    o.displayedMiscRowsForQuest=1;
    assert(G::Review(o)==Outcome::kCommandInputUnverified);
    o.commandChannel=G::CommandChannel::kCompanionAppRpcType5;
    assert(G::Review(o)==Outcome::kCommandInputUnverified);
    o.commandChannel=G::CommandChannel::kInGameScaleformCodeObject;
    // Restore only in a *synthetic* test; no write backend exists.
    o.inGameCallbackEvidence=G::InGameCallbackEvidence::kBGSCodeObjCallbackAbiObserved;
    assert(G::Review(o)==Outcome::kQuestInstanceCandidateReadOnly);

    assert(std::string_view(G::Name(Outcome::kCommandInputUnverified))==
           "in-game-code-object-origin-or-abi-unverified");
    assert(std::string_view(G::Name(Outcome::kQuestInstanceCandidateReadOnly))==
           "quest-instance-candidate-NOT-write-authorized");
    // Even if a candidate is structurally recognized, the shipping
    // writer gate MUST remain off.
    static_assert(!K2040::QuestTracking::CanWritePipboy());
}
