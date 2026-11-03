#pragma once
// Non-shipping read-only adapter: source in research/, not src/.
// Context must be produced by a verified game-thread/UI session owner.
// This API does not authorize a native write or invoke any quest method.
#include "QuestWideSnapshotLedger.h"
#include <cstdint>

namespace K2040::QuestWideNativeResearch {
struct TrustedContext {
    std::uint64_t saveEpoch{},viewEpoch{};
    std::uint32_t selectedFormID{},selectedInstanceID{};
    bool selectedInstanceTyped{};
    bool runtimeOG163{},gameLoaded{},gameThread{},journalFocused{};
};
struct NativeCapture {
    Snapshot state{};
    FeedLedger feed{};
    // Independently observed selected instance: never trust a request to
    // fill or replace the UI selection's identity.
    std::uint32_t selectedInstanceIDFromContext{};
    // A typed row and complete feed cannot prove full vanilla side effects.
    [[nodiscard]] Result Review(const Request& request) const noexcept {
        if(!state.instanceIdentityTyped ||
           request.selectedInstanceID!=selectedInstanceIDFromContext)
            return {Outcome::kInstanceMismatch};
        return QuestWideNativeResearch::Review(request,state,feed.Rows());
    }
};
// Call only synchronously on the F4SE game thread, never from a web view.
[[nodiscard]] NativeCapture CaptureNativeQuestWideReadOnly(
    std::uint32_t requestedFormID, const TrustedContext& context);
}
