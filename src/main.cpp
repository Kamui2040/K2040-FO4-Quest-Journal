// K2040 Quest Journal - exploratory F4SE/Prisma UI plugin.
// Read-only quest capture, persisted appearance, guarded Prisma cursor ownership.
#include <RE/Fallout.h>
#include <F4SE/F4SE.h>
#include "PrismaUI_F4_Modern_API.h"
#include "QuestCollector.h"
#include "QuestTrackingPolicy.h"
#ifdef K2040_PIPBOY_SELECTION_READONLY_QA
#include "../research/native_pipboy_selection/ReadOnlySelectedEntry.h"
#ifdef K2040_PIPBOY_SELECTION_SCHEMA_READONLY_QA
#include "../research/native_pipboy_selection/ReadOnlySelectedSchema.h"
#include "../research/native_pipboy_selection/ReadOnlyCodeObjectBinding.h"
#include "../research/native_pipboy_selection/PipboyDispatchPrefixReadOnly.h"
#endif
#include "../research/f4se_og_api/TaskInterface0623.h"
#endif
#ifdef K2040_SELECTOR_READONLY_QA
#include "../research/native_one_shot/MiscRuntimeDispatchProbe.h"
#include "../research/f4se_og_api/TaskInterface0623.h"
#endif
#ifdef K2040_DOGMEAT_PRODUCTION_CANDIDATE
#include "DogmeatProduction.h"
#include "../research/f4se_og_api/TaskInterface0623.h"
#include <cstdio>
#endif
#ifdef K2040_TRACKING_DRYRUN_QA
#include "../research/native_one_shot/OneShotOgDryRun.h"
#include "../research/f4se_og_api/TaskInterface0623.h"
#include "MiscProbeLogSanitizer.h"
#ifdef K2040_TRACKING_INTEGRATED_QA
#include "../research/native_one_shot/OneShotOgIntegratedQA.h"
#include "../research/native_one_shot/MiscRuntimeDispatchProbe.h"
#endif
#ifdef K2040_TRACKING_NATIVE_WRITE_QA
#include "../research/native_one_shot/OneShotOgWriteQA.h"
#endif
#ifdef K2040_REPEATABLE_MISC_QA
#include "../research/native_one_shot/RepeatableOgWriteQA.h"
#endif
#include <memory>
#include <cstdio>
#endif
#ifdef K2040_QUESTWIDE_PRODUCTION_CANDIDATE
#include "QuestWideProductionCandidate.h"
#include <memory>
#include <cstdio>
#endif
#include "JournalSettings.h"
#include "TrackerCompatibility.h"
#include "HudColor.h"
#include <RE/H/HUDMenuUtils.h>
#include "QuestViewState.h"
#include "HubBindings.h"
#include "ControllerNavigation.h"
#include <Scaleform/G/GFx_FunctionHandler.h>
#include <Scaleform/G/GFx_Movie.h>
#include <Scaleform/G/GFx_Value.h>
#include <Windows.h>
#include <Xinput.h>
#include <atomic>
#include <array>
#include <chrono>
#include <thread>
#include "UILifecycleEpoch.h"
#include <RE/M/MenuCursor.h>
#include <RE/B/BSInputEnableManager.h>
#include <RE/B/BSInputEnableLayer.h>
#include <RE/O/OtherInputEvents.h>
#include <RE/U/UserEvents.h>
#include <charconv>
#include <cmath>
#include <optional>
#include <cstdint>
#include <functional>
#include <cstdio>
#include <string>
#include <string_view>

namespace {
    namespace UI = PRISMA_UI_FLAT_API;
    UI::CoreAPI core{};
    UI::ViewAPI views{};
    UI::InteropAPI interop{};
    UI::ControllerAPI controller{};
    PrismaView journalView = 0;
    PrismaView trackerView = 0;
    bool journalOpen = false;
    bool journalDomReady = false;
    bool trackerDomReady = false;
    std::atomic_bool journalFocused{false};
    bool trackerEnabled = true;
    // The user's saved preference is not changed by another HUD being loaded.
    // Phosphor HUD 0.9.24 OG ships PhosphorHUD.dll; bLiveHud is its one
    // persistent master switch, read by that mod at F4SE kPostLoad.
    bool PhosphorHudActive() {
        if (!GetModuleHandleW(L"PhosphorHUD.dll")) return false;
        // Cache after finding the loaded module, never cache an absent module.
        // Phosphor likewise reads bLiveHud once per process, not per save.
        static const bool live = []() {
            // GetPrivateProfileIntW resolves relative names under Windows,
            // not the game directory. Resolve exactly the path Phosphor reads.
            std::array<wchar_t, MAX_PATH> iniPath{};
            const DWORD count = GetFullPathNameW(
                L"Data\\F4SE\\Plugins\\PhosphorHUD.ini",
                static_cast<DWORD>(iniPath.size()), iniPath.data(), nullptr);
            if (count == 0 || count >= iniPath.size()) return true;
            return GetPrivateProfileIntW(L"Main", L"bLiveHud", 1,
                                         iniPath.data()) != 0;
        }();
        return live;
    }
    bool TrackerMayShow() {
        return K2040::TrackerCompatibility::ShowMiniTracker(
            trackerEnabled, PhosphorHudActive(),
            K2040::Settings::Current().trackerOverridePhosphor);
    }
    bool cursorFallbackRegistered = false;
    bool apiReady = false;
    bool gameLoaded = false;
    bool hotkeysRegistered = false;
    bool hubScaleformRegistered = false;
    std::atomic_bool padPolling{false};
    std::array<std::uint64_t,2> lastToggleTicks{0,0};
    // Shared by browser Escape events and the native physical-key fallback.
    // Only the game thread updates this timestamp.
    std::uint64_t lastJournalEscapeTick = 0;
    std::atomic_bool escapeUiAcknowledged{false};
    K2040::UILifecycleEpoch uiEpoch;
    K2040::UILifecycleEpoch trackerEpoch;
#ifdef K2040_PIPBOY_SELECTION_READONLY_QA
    K2040::UILifecycleEpoch pipboyGfxSaveEpoch;
    // Save notifications run on the game thread; the GFx UI task must
    // not read the non-atomic global gameLoaded flag across threads.
    std::atomic_bool pipboyGfxGameLoaded{false};
    // Input worker only schedules a single bounded UI-thread observation.
    // No GFx calls outside the UI task. No writer or game-state mutation.
    std::atomic_bool pipboyGfxQueued{false};
    std::atomic_bool pipboyGfxAttempted{false};
#endif
#ifdef K2040_SELECTOR_READONLY_QA
    K2040::UILifecycleEpoch selectorSaveEpoch;
    std::atomic_uint32_t selectorSelectedFormID{0};
    std::atomic_bool selectorProcessCaptured{false};
    std::atomic_bool selectorOG163{false};
#endif
#ifdef K2040_DOGMEAT_PRODUCTION_CANDIDATE
    K2040::UILifecycleEpoch dogmeatSaveEpoch;
    std::atomic_uint32_t dogmeatSelectedFormID{0};
    std::atomic_bool dogmeatProcessAttempted{false};
    std::atomic_bool dogmeatRuntimeOG163{false};
#endif
#ifdef K2040_QUESTWIDE_PRODUCTION_CANDIDATE
    K2040::UILifecycleEpoch candidateSaveEpoch;
    // A verified postwrite readback may continue after closing/reopening
    // the same Journal. Only save/new-game transitions invalidate it.
    K2040::UILifecycleEpoch candidateSessionEpoch;
    std::atomic_uint32_t candidateSelectedFormID{0};
    std::atomic_bool candidateOg163{false};
    std::shared_ptr<K2040::QuestWideCandidate::Controller> candidateGate=
        std::make_shared<K2040::QuestWideCandidate::Controller>();
    std::uint64_t lastCandidateReadbackMillis{0};
    void ResetCandidateGate(){
        candidateSelectedFormID.store(0,std::memory_order_release);
        candidateGate->BeginSession(candidateSaveEpoch.Capture(),candidateSessionEpoch.Capture());
    }
#endif
#ifdef K2040_TRACKING_DRYRUN_QA
    // Independent save and focused journal lifecycles. This exists ONLY in
    // the test-flavor DLL; the shipping plugin has no preview event handler.
    K2040::UILifecycleEpoch trackingSaveEpoch;
    std::shared_ptr<K2040::OneShotResearch::Gate> dryRunGate =
        std::make_shared<K2040::OneShotResearch::Gate>();
    std::atomic<std::uint32_t> dryRunSelectedFormId{0};
    std::atomic_bool dryRunOgRuntime{false};
#ifdef K2040_TRACKING_INTEGRATED_QA
    // QA-only UI consent is cleared on every focused view/save transition.
    // The bounded attempt budget is process-wide and NEVER reset on load.
    std::atomic_bool integratedQaEnabled{false};
    std::atomic_uint32_t integratedQaAttempts{0};
    // Read-only Misc inspector is independent of native write budget.
    // Process-local sequence numbers allow comparison of repeated probes
    // without recording selected quest FormIDs in diagnostic files.
    std::atomic_uint32_t miscProbeSequence{0};
    std::atomic_bool miscDispatchCaptured{false};
#ifdef K2040_SINGLE_MISC_WRITE_QA
    std::atomic_uint32_t singleMiscArmedFormId{0};
    std::atomic_bool singleMiscAttempted{false}; // NEVER reset on save/load
#endif
#endif
#ifdef K2040_TRACKING_NATIVE_WRITE_QA
    // Per-game-process one-attempt ceiling, NOT reset on save/load or view.
    std::atomic_bool writeQaAttempted{false};
    std::atomic_bool writeQaConsentAtLaunch{false};
#endif
#ifdef K2040_REPEATABLE_MISC_QA
    std::shared_ptr<K2040::RepeatableQuestWide::Controller> repeatableGate =
        std::make_shared<K2040::RepeatableQuestWide::Controller>();
    std::atomic_bool repeatableConsentAtLaunch{false};
    // Game-thread readbacks are spaced so one UI update burst cannot
    // prematurely exhaust the three-read verification budget.
    std::uint64_t lastRepeatableReadbackMillis{0};
#endif
    void ResetDryRunGate() {
#ifdef K2040_REPEATABLE_MISC_QA
        repeatableGate->BeginSession(trackingSaveEpoch.Capture(),uiEpoch.Capture());
        K2040::RepeatableQuestWide::ResetSessionConsent();
        repeatableConsentAtLaunch.store(false,std::memory_order_release);
#endif
#ifdef K2040_TRACKING_INTEGRATED_QA
        integratedQaEnabled.store(false,std::memory_order_release);
#ifdef K2040_SINGLE_MISC_WRITE_QA
        singleMiscArmedFormId.store(0,std::memory_order_release);
#endif
#endif
        dryRunSelectedFormId.store(0, std::memory_order_release);
        dryRunGate->BeginSession(trackingSaveEpoch.Capture(), uiEpoch.Capture());
    }
#endif
    std::string lastPublishedJournal;
    std::string lastPublishedTracker;
    RE::BSTSmartPointer<RE::BSInputEnableLayer> journalInputLayer;
    bool journalInputGuard = false;
    // Closing a Prisma window while the opening key is still physically down
    // must not expose that same press to Fallout 4's vanilla quick-data bind.
    // Only the journal's own input layer is held; nothing is remapped.
    std::atomic<std::uint32_t> deferredJournalReleaseKey{0};
    std::atomic<std::uint64_t> deferredJournalReleaseAt{0};
    std::atomic_bool deferredJournalReleaseQueued{false};
    // Native Mouse4/5 sampler mirrors Quick Attach's accepted Proton fix.
    // 0=inactive, 1=journal, 2=tracker. No cursor/window hooking.
    std::atomic_int hubNativeMouseCapture{0};

    // Prevent underlying player controls from processing inputs while the
    // full-screen Prisma journal is open. The passive HUD never uses this layer.
    bool AcquireJournalInputGuard() {
        if(journalInputGuard && journalInputLayer)return true;
        auto* manager=RE::BSInputEnableManager::GetSingleton();
        if(!manager)return false;
        if(!journalInputLayer &&
           !manager->AllocateNewLayer(journalInputLayer, "K2040 Quest Journal"))return false;
        constexpr auto gameplayEvents=static_cast<RE::OtherInputEvents::OTHER_EVENT_FLAG>(
            static_cast<std::uint32_t>(RE::OtherInputEvents::OTHER_EVENT_FLAG::kAll) &
            ~static_cast<std::uint32_t>(RE::OtherInputEvents::OTHER_EVENT_FLAG::kCursor));
        const bool userDisabled=manager->EnableUserEvent(
            journalInputLayer->layerID,RE::UserEvents::USER_EVENT_FLAG::kAll,false,
            RE::UserEvents::SENDER_ID::kMenu);
        const bool otherDisabled=manager->EnableOtherEvent(
            journalInputLayer->layerID,gameplayEvents,false,
            RE::UserEvents::SENDER_ID::kMenu);
        if(!userDisabled || !otherDisabled) {
            journalInputLayer.reset();
            return false;
        }
        journalInputGuard=true;
        return true;
    }
    void ReleaseJournalInputGuard() {
        deferredJournalReleaseKey.store(0,std::memory_order_release);
        deferredJournalReleaseAt.store(0,std::memory_order_release);
        deferredJournalReleaseQueued.store(false,std::memory_order_release);
        journalInputGuard=false;
        journalInputLayer.reset();
    }
    int VirtualKeyFromMcm(std::uint32_t key) {
        if(key==258)return VK_MBUTTON;
        if(key==259)return VK_XBUTTON1;
        if(key==260)return VK_XBUTTON2;
        return key<=255?static_cast<int>(key):0;
    }
    std::uint32_t HeldJournalShortcutKey() {
        const auto b=K2040::Hub::ReadKeyboard(K2040::Hub::Action::Journal);
        if(!b.installed||!b.assigned||b.conflict)return 0;
        const int key=VirtualKeyFromMcm(b.keycode);
        return key>0 && (GetAsyncKeyState(key)&0x8000)!=0
            ? static_cast<std::uint32_t>(key) : 0;
    }
    void FinishDeferredJournalInputGuard(std::uint32_t key) {
        deferredJournalReleaseQueued.store(false,std::memory_order_release);
        if(journalOpen || deferredJournalReleaseKey.load(std::memory_order_acquire)!=key)
            return; // a reopened journal still owns its guard
        const auto started=deferredJournalReleaseAt.load(std::memory_order_acquire);
        if((GetAsyncKeyState(static_cast<int>(key))&0x8000)!=0 &&
           GetTickCount64()-started<5000)return;
        ReleaseJournalInputGuard();
    }

    bool Valid(PrismaView view) {
        return apiReady && view && views.IsValid && views.IsValid(view);
    }
    std::string CurrentPipboyRgb() {
        // Fallout's Pip-Boy RGB sliders are stored separately from gameplay
        // HUD color. Read the live INIPref collection to follow user changes
        // without touching or copying any mutable INI files.
        auto* prefs=RE::INIPrefSettingCollection::GetSingleton();
        if(!prefs)return {};
        const auto channel=[prefs](const char* full,const char* shortName)->std::optional<float>{
            auto* value=prefs->GetSetting(full);
            if(!value)value=prefs->GetSetting(shortName);
            if(!value || value->GetType()!=RE::Setting::SETTING_TYPE::kFloat)return std::nullopt;
            const auto v=value->GetFloat();
            return std::isfinite(v) && v>=0.0F && v<=1.0F ? std::optional<float>{v}:std::nullopt;
        };
        const auto r=channel("fPipboyEffectColorR:Pipboy","fPipboyEffectColorR");
        const auto g=channel("fPipboyEffectColorG:Pipboy","fPipboyEffectColorG");
        const auto b=channel("fPipboyEffectColorB:Pipboy","fPipboyEffectColorB");
        if(!r || !g || !b)return {};
        return K2040::HudColor::ToCss(*r,*g,*b);
    }
    void SyncSettings() {
        if (!apiReady || !interop.InteropCall) return;
        // Read the live engine colors like Quick Attach Menu does (OG + AE).
        const auto hud=RE::HUDMenuUtils::GetGameplayHUDColor();
        const auto background=RE::HUDMenuUtils::GetGameplayHUDBackgroundColor();
        const std::string payload=K2040::Settings::Serialize(
            K2040::HudColor::ToCss(hud.r,hud.g,hud.b),
            K2040::HudColor::ToCss(background.r,background.g,background.b),
            CurrentPipboyRgb());
        if(Valid(journalView))interop.InteropCall(journalView,"receiveJournalSettings",payload.c_str());
        if(TrackerMayShow() && Valid(trackerView))interop.InteropCall(trackerView,"receiveJournalSettings",payload.c_str());
        K2040::Hub::MigrateLegacyIfNeeded();
        if(Valid(journalView)) {
            const auto hub=K2040::Hub::SnapshotJson();
            interop.InteropCall(journalView,"receiveHubBindings",hub.c_str());
        }
    }
    // Adapted from the reviewed Quick Attach Menu cursor fix. Register only if
    // focus did not increase the engine cursor owner count; never double-own.
    void ReleaseCursorFallback() {
        if(!cursorFallbackRegistered)return;
        if(auto* cursor=RE::MenuCursor::GetSingleton()) {
            // Version-aware CommonLibF4 wrapper, not a manual relocation ID.
            cursor->UnregisterCursor();
        }
        cursorFallbackRegistered=false;
    }
    void EnsureCursorOwnership(std::uint32_t ownersBefore) {
        auto* cursor=RE::MenuCursor::GetSingleton();
        if(!cursor)return;
        if (!cursorFallbackRegistered && cursor->registeredCursors<=ownersBefore) {
            const auto oldCount=cursor->registeredCursors;
            // CommonLibF4 already resolves the runtime-specific relocation.
            cursor->RegisterCursor();
            cursorFallbackRegistered=cursor->registeredCursors>oldCount;
        }
        cursor->ClearConstraints();
    }
    void SyncSnapshots() {
        if (!apiReady || !interop.InteropCall) return;
        const std::string payload = K2040::Quests::CaptureSnapshot(
            K2040::QuestViewState::Get(K2040::QuestViewState::Kind::Pin),
            K2040::QuestViewState::Get(K2040::QuestViewState::Kind::Favorite),
            K2040::QuestViewState::Get(K2040::QuestViewState::Kind::Hidden),
            K2040::Settings::Current().trackerAutoPinNew);
#ifdef K2040_REPEATABLE_MISC_QA
        const auto repeatableNow=GetTickCount64();
        if (repeatableGate->Get().phase==K2040::RepeatableQuestWide::Phase::kAwaitingRefresh &&
            repeatableNow-lastRepeatableReadbackMillis>=1500u) {
            lastRepeatableReadbackMillis=repeatableNow;
            (void)K2040::RepeatableQuestWide::Reconcile(repeatableGate,
                []() -> K2040::OneShotResearch::LiveJournalContext {
                    const auto id=dryRunSelectedFormId.load(std::memory_order_acquire);
                    return {id,trackingSaveEpoch.Capture(),uiEpoch.Capture(),
                            gameLoaded,journalOpen && journalFocused.load() && Valid(journalView),
                            K2040::QuestViewState::Observed().contains(id),
                            dryRunOgRuntime.load(std::memory_order_acquire)};
                },[](std::uint32_t id,K2040::RepeatableQuestWide::Outcome outcome) {
                    if(journalOpen && Valid(journalView)) {
                        char result[40]{};
                        const char* status=outcome==K2040::RepeatableQuestWide::Outcome::kConfirmed
                            ? "confirmed" : "blocked";
                        std::snprintf(result,sizeof(result),"%08X:%s",id,status);
                        interop.InteropCall(journalView,"receiveRepeatableTrackingResult",result);
                    }
                });
        }
#endif
#ifdef K2040_QUESTWIDE_PRODUCTION_CANDIDATE
        const auto candidateNow=GetTickCount64();
        // Tracker polling also calls SyncSnapshots while the Journal is
        // hidden. Do not interpret a closed/unfocused panel as failed native
        // readback; preserve the pending ticket for the next focused view.
        if(journalOpen && journalFocused.load(std::memory_order_acquire) &&
           Valid(journalView) &&
           candidateGate->Get().phase==K2040::QuestWideCandidate::Phase::kAwaitingRefresh &&
           candidateNow-lastCandidateReadbackMillis>=1500u){
            lastCandidateReadbackMillis=candidateNow;
            (void)K2040::QuestWideCandidate::Reconcile(candidateGate,
                []()->K2040::OneShotResearch::LiveJournalContext {
                    // Readback belongs to the issued ticket, not whichever
                    // quest is highlighted next. No new write is permitted.
                    const auto status=candidateGate->Get();
                    const auto id=status.pending ? status.pending->intent.formId : 0u;
                    return {id,candidateSaveEpoch.Capture(),candidateSessionEpoch.Capture(),
                            gameLoaded,journalOpen && journalFocused.load() && Valid(journalView),
                            K2040::QuestViewState::Observed().contains(id),
                            candidateOg163.load(std::memory_order_acquire)};
                },[](std::uint32_t id,K2040::QuestWideCandidate::Outcome outcome){
                    if(!journalOpen || !Valid(journalView))return;
                    char response[44]{};
                    const char* status=outcome==K2040::QuestWideCandidate::Outcome::kConfirmed
                        ? "confirmed":"blocked";
                    std::snprintf(response,sizeof(response),"%08X:%s",id,status);
                    interop.InteropCall(journalView,"receiveQuestWideCandidateResult",response);
                    interop.InteropCall(journalView,"enableQuestWideCandidate",
                        outcome==K2040::QuestWideCandidate::Outcome::kConfirmed
                            ?"ready":"blocked");
                });
        }
#endif
        // Independent delivery caches avoid repeatedly pushing identical JSON
        // through Prisma when its native update timers fire. Clear these when
        // any view is destroyed, since new views need an initial snapshot.
        if (journalDomReady && Valid(journalView) && payload != lastPublishedJournal) {
            interop.InteropCall(journalView, "receiveQuestSnapshot", payload.c_str());
            lastPublishedJournal=payload;
        }
        if (TrackerMayShow() && trackerDomReady && Valid(trackerView) && payload != lastPublishedTracker) {
            interop.InteropCall(trackerView, "receiveQuestSnapshot", payload.c_str());
            lastPublishedTracker=payload;
        }
    }
    void CreateJournalView();
    void CreateViews();
    void CloseJournal();
    bool CompetingGameMenuOpen() {
        if(const auto* menus=RE::UI::GetSingleton()) {
            return menus->GetMenuOpen("PipboyMenu") ||
                   menus->GetMenuOpen("PauseMenu") ||
                   menus->GetMenuOpen("Console") ||
                   menus->GetMenuOpen("DialogueMenu");
        }
        return false;
    }
    bool AnotherPrismaViewFocused() {
        if(!controller.GetFocusedView)return false;
        const auto focused=controller.GetFocusedView();
        return focused && focused!=journalView;
    }
    void ValidateJournalOwnership() {
        // A competing menu can appear *after* a journal has acquired gameplay
        // isolation. Abandon only our own focus/cursor/layer, never another mod.
        if(journalOpen && journalFocused &&
           (!gameLoaded || CompetingGameMenuOpen() || AnotherPrismaViewFocused()))
            CloseJournal();
    }
    void ShowJournalIfReady() {
        if(!journalOpen || journalFocused || !journalDomReady || !Valid(journalView))return;
        // A menu may have appeared while the asynchronous view loaded.
        if(CompetingGameMenuOpen() || AnotherPrismaViewFocused()) {
            CloseJournal();
            return;
        }
        // Acquire only when the page is ready. A failed DOM load must never
        // leave gameplay input disabled behind an invisible view.
        if(!AcquireJournalInputGuard()) {
            journalOpen=false;
            views.Hide(journalView);
            return;
        }
        views.Show(journalView);
        const auto* cursor=RE::MenuCursor::GetSingleton();
        const auto ownersBefore=cursor?cursor->registeredCursors:0;
        if(!views.Focus(journalView,false,false)){
            journalOpen=false;
            views.Hide(journalView);
            ReleaseCursorFallback();
            ReleaseJournalInputGuard();
            return;
        }
        journalFocused=true;
        deferredJournalReleaseKey.store(0,std::memory_order_release);
        EnsureCursorOwnership(ownersBefore);
        SyncSettings();
        SyncSnapshots();
    }
    void CloseJournal() {
        // Read the physical shortcut before releasing Prisma focus.
        const std::uint32_t heldKey=journalInputGuard?HeldJournalShortcutKey():0;
        uiEpoch.Invalidate();
#ifdef K2040_SELECTOR_READONLY_QA
        selectorSelectedFormID.store(0,std::memory_order_release);
#endif
#ifdef K2040_DOGMEAT_PRODUCTION_CANDIDATE
        dogmeatSelectedFormID.store(0,std::memory_order_release);
#endif
#ifdef K2040_TRACKING_DRYRUN_QA
        ResetDryRunGate();
#endif
#ifdef K2040_QUESTWIDE_PRODUCTION_CANDIDATE
        // Do not make closing the Journal to inspect the vanilla Pip-Boy
        // permanently block a write whose native call already returned.
        candidateSelectedFormID.store(0,std::memory_order_release);
#endif
        journalOpen = false;
        journalFocused = false;
        if (Valid(journalView)) {
            views.Unfocus(journalView);
            views.Hide(journalView);
        }
        ReleaseCursorFallback();
        if(heldKey) {
            // The vanilla Data-tab handler may otherwise process a held J
            // once Prisma unfocuses. Keep only our gameplay-input isolation
            // until key-up; a five-second failsafe prevents a stranded layer.
            deferredJournalReleaseAt.store(GetTickCount64(),std::memory_order_release);
            deferredJournalReleaseKey.store(heldKey,std::memory_order_release);
        } else {
            ReleaseJournalInputGuard();
        }
    }
    void HandleJournalEscape(bool fromUi=false) {
        // A DOM key-down is sufficient to consume this *physical* gesture,
        // regardless of how long the player holds Escape before key-up.
        if(fromUi)escapeUiAcknowledged.store(true,std::memory_order_release);
        // Native polling must not change a Prisma DOM directly. Forward the
        // request to its existing page, which knows if Settings is open.
        if(!journalOpen || !journalFocused || !Valid(journalView))return;
        const auto now=static_cast<std::uint64_t>(GetTickCount64());
        if(now-lastJournalEscapeTick<320)return; // DOM + poller same press
        lastJournalEscapeTick=now;
        if(interop.InteropCall)
            interop.InteropCall(journalView,"handleJournalEscape","");
        else
            CloseJournal();
    }
    void AcknowledgeCapturedEscape() {
        // Capturing a replacement shortcut consumes Escape in the browser.
        // Suppress the physical key-up fallback for the same key press.
        lastJournalEscapeTick=static_cast<std::uint64_t>(GetTickCount64());
        escapeUiAcknowledged.store(true,std::memory_order_release);
    }
    void OpenJournal() {
        // Never start Prisma rendering from main menu or a load transition.
        if(!gameLoaded || !apiReady)return;
        if(journalOpen) {CloseJournal();return;}
        // Do not steal focus from a vanilla menu or another Prisma panel.
        if (CompetingGameMenuOpen() || AnotherPrismaViewFocused())return;
        // Never reuse a closed panel. Quick Attach Menu observed that valid
        // hidden views can retain stale invisible cursor and render state.
        if (Valid(journalView)) {
            views.Unfocus(journalView);
            views.Hide(journalView);
            views.Destroy(journalView);
        }
        journalView=0;
        lastPublishedJournal.clear();
        journalDomReady=false;
        journalFocused=false;
        ReleaseCursorFallback();
        uiEpoch.Invalidate();
#ifdef K2040_SELECTOR_READONLY_QA
        selectorSelectedFormID.store(0,std::memory_order_release);
#endif
#ifdef K2040_DOGMEAT_PRODUCTION_CANDIDATE
        dogmeatSelectedFormID.store(0,std::memory_order_release);
#endif
#ifdef K2040_TRACKING_DRYRUN_QA
        ResetDryRunGate();
#endif
#ifdef K2040_QUESTWIDE_PRODUCTION_CANDIDATE
        // Do not make closing the Journal to inspect the vanilla Pip-Boy
        // permanently block a write whose native call already returned.
        candidateSelectedFormID.store(0,std::memory_order_release);
#endif
        journalOpen=true;
        CreateJournalView();
        // A nonzero handle denotes successful request to create the page.
        // Prisma may still report IsValid=false while Ultralight is loading;
        // only DOM-ready is allowed to focus, reveal or collect quest data.
        if(!journalView) {
            journalOpen=false;
            ReleaseJournalInputGuard();
        }
        // Focus is deferred until DOM-ready; closing early cancels it.
    }
    void ToggleTracker() {
        // Honor the explicit override; otherwise Phosphor owns the quest box
        // and its presence must not alter the user's saved tracker preference.
        if (PhosphorHudActive() && !K2040::Settings::Current().trackerOverridePhosphor) {
            if (Valid(trackerView)) views.Hide(trackerView);
            return;
        }
        const bool desired=!trackerEnabled;
        if(!K2040::Settings::ChangeAndSave(desired?"trackerEnabled:true":"trackerEnabled:false"))return;
        trackerEnabled=desired;
        // The view may not exist if Phosphor was active at startup and the
        // override was enabled while our own tracker remained switched off.
        if(TrackerMayShow() && !trackerView)CreateViews();
        if (!Valid(trackerView)) return;
        if (TrackerMayShow()) { views.Show(trackerView); SyncSnapshots(); }
        else views.Hide(trackerView);
        SyncSettings();
    }
    // A pending native MCM edit is NOT a persisted assignment until
    // the live PauseMenu bridge applies and verifies it.
    void UpdateHubBinding(const char* command,bool controllerBinding) {
        if(!command || !journalOpen || !journalFocused)return;
        K2040::Hub::Action action{};
        unsigned first{},second{};
        const bool valid=K2040::Hub::DecodeRequest(command,action,first,second);
        const bool saved=valid && (controllerBinding
            ? K2040::Hub::SavePad(action,first,second)
            : K2040::Hub::SaveKeyboard(action,first,second));
        if(Valid(journalView)) {
            const auto payload=K2040::Hub::SnapshotJson();
            interop.InteropCall(journalView,"receiveHubBindings",payload.c_str());
            const bool pendingMcm=saved && !controllerBinding &&
                K2040::Hub::NativeMcmOwnsRegistry();
            interop.InteropCall(journalView,"receiveJournalSettingsStatus",
                                !saved?"invalid":pendingMcm?"pending-mcm":"saved");
        }
    }
    void UpdateSetting(const char* setting) {
        if(!setting || !journalOpen || !journalFocused)return;
        const bool saved=K2040::Settings::ChangeAndSave(setting);
        if(!saved) {
            // The browser may already have shown the speculative appearance.
            // Send the previous validated settings back when disk write fails.
            SyncSettings();
            if(Valid(journalView))interop.InteropCall(journalView,"receiveJournalSettingsStatus",K2040::Settings::LastFailureReason());
            return;
        }
        trackerEnabled=K2040::Settings::Current().trackerEnabled;
        // Phosphor auto-hide skips creating the mini tracker at startup.
        // Opting in during gameplay must create it without a save reload.
        if(TrackerMayShow() && !trackerView)CreateViews();
        if(Valid(trackerView)) {
            if(TrackerMayShow())views.Show(trackerView);
            else views.Hide(trackerView);
        }
        SyncSettings();
        if(Valid(journalView))interop.InteropCall(journalView,"receiveJournalSettingsStatus",saved?"saved":K2040::Settings::LastFailureReason());
    }
    void ResetAppearance() {
        if(!journalOpen || !journalFocused)return;
        const bool saved=K2040::Settings::ResetToDefaults();
        trackerEnabled=K2040::Settings::Current().trackerEnabled;
        if(TrackerMayShow() && !trackerView)CreateViews();
        if(Valid(trackerView)) {
            if(TrackerMayShow())views.Show(trackerView);
            else views.Hide(trackerView);
        }
        SyncSettings();
        if(Valid(journalView))interop.InteropCall(journalView,"receiveJournalSettingsStatus",saved?"saved":K2040::Settings::LastFailureReason());
    }
    void UpdateQuestFlag(const char* formString, K2040::QuestViewState::Kind kind) {
        if (!formString || !journalOpen || !journalFocused) return;
        const std::string_view str(formString);
        if (str.size() != 8) return;
        std::uint32_t id = 0;
        auto [ptr, error] = std::from_chars(str.data(), str.data()+str.size(), id, 16);
        if (error != std::errc() || ptr != str.data()+str.size() || id == 0) return;
        // No engine mutation; only known loaded TESQuest forms can be pinned.
        if (!RE::TESForm::GetFormByID<RE::TESQuest>(id) ||
            !K2040::QuestViewState::Observed().contains(id)) return;
        if (!K2040::QuestViewState::Toggle(kind, id)) return;
        SyncSnapshots();
    }
    // SAFETY: unverified Papyrus Quest.SetActive dispatch caused a user CTD.
    // Fail closed, including callbacks from older cached Prisma UI pages.
    // Fail closed for both current and cached Prisma views. A separate
    // header owns input validation and the explicit backend-verification gate.
    // No F4SE/Papyrus/native gameplay write is reachable in this build.
    void UpdatePipboyTracking(const char* argument) {
        if(!argument || !journalOpen || !journalFocused || !Valid(journalView))
            return;
        const auto request=K2040::QuestTracking::ParseRequest(argument);
        if(!request)return;
        static_assert(!K2040::QuestTracking::CanWritePipboy(),
                      "Engine write backend has not passed runtime validation");
        const std::string payload=std::string(std::string_view(argument).substr(0,8))
            +":"+std::string(K2040::QuestTracking::kDisabledStatus);
        interop.InteropCall(journalView,"receiveQuestTrackingResult",payload.c_str());
    }
    void UpdateCategory(const char* argument) {
        if(!argument || !journalOpen || !journalFocused)return;
        const std::string_view command(argument);
        if(command.size()<10 || command.size()>40 || command[8]!=':')return;
        const auto idText=command.substr(0,8);
        std::uint32_t id{};
        const auto [end,error]=std::from_chars(idText.data(),idText.data()+idText.size(),id,16);
        if(error!=std::errc{} || end!=idText.data()+idText.size() || !id ||
           !RE::TESForm::GetFormByID<RE::TESQuest>(id) ||
           !K2040::QuestViewState::Observed().contains(id))return;
        if(!K2040::QuestViewState::SetCategory(id,command.substr(9)))return;
        SyncSnapshots();
    }
    // All UI-triggered native work is dispatched to F4SE's game thread.
    // In particular, never retain a Prisma callback's borrowed char* beyond
    // this invocation: the browser may invalidate it before AddTask runs.
#ifdef K2040_TRACKING_DRYRUN_QA
    void QueueDryRunPreview(const char* borrowed) {
        // This callback must NOT be nested in QueueUIString: the adapter
        // directly performs exactly ONE OG F4SE AddTask per accepted click.
        if (!borrowed || !gameLoaded || !journalOpen ||
            !journalFocused.load(std::memory_order_acquire) ||
            !Valid(journalView)) return;
        const auto parsed=K2040::QuestTracking::ParseRequest(
            std::string_view(borrowed, strnlen_s(borrowed,11)));
        if (!parsed) return;
        const auto id=parsed->formId;
        const auto ticketEpoch=uiEpoch.Capture();
        const auto saveEpoch=trackingSaveEpoch.Capture();
        const auto accepted=K2040::OneShotResearch::QueueFromExplicitPrismaClick(
            dryRunGate, borrowed, saveEpoch, ticketEpoch,
            []() -> K2040::OneShotResearch::LiveJournalContext {
                const auto id=dryRunSelectedFormId.load(std::memory_order_acquire);
                return {
                    id, trackingSaveEpoch.Capture(), uiEpoch.Capture(),
                    gameLoaded, journalOpen && journalFocused.load() &&
                    Valid(journalView), K2040::QuestViewState::Observed().contains(id),
                    dryRunOgRuntime.load(std::memory_order_acquire)
                };
            },
            [id](K2040::OneShotResearch::Result result) {
                if (!journalOpen || !journalFocused.load() ||
                    !Valid(journalView)) return;
                const char* status="rejected";
                switch (result) {
                    case K2040::OneShotResearch::Result::kWouldWrite:
                        status="would-write"; break;
                    case K2040::OneShotResearch::Result::kAlreadyCurrent:
                        status="already"; break;
                    case K2040::OneShotResearch::Result::kUnverified:
                        status="unverified"; break;
                    default: break;
                }
                char reply[45]{};
                std::snprintf(reply,sizeof(reply),"%08X:%s",id,status);
                interop.InteropCall(journalView,"receiveTrackingDryRunResult",reply);
                REX::INFO("JournalDryRunQA: task result={} (quest ID omitted)", status);
            });
        if (!accepted) {
            REX::WARN("JournalDryRunQA: request refused before scheduling");
        } else {
            REX::INFO("JournalDryRunQA: exactly one task submitted, no engine write");
        }
    }
#endif
#ifdef K2040_TRACKING_NATIVE_WRITE_QA
    void QueueNativeWriteTestOnce(const char* borrowed) {
        if (!borrowed || !gameLoaded || !journalOpen ||
            !journalFocused.load(std::memory_order_acquire) ||
            !Valid(journalView) ||
            !writeQaConsentAtLaunch.load(std::memory_order_acquire) ||
            writeQaAttempted.load(std::memory_order_acquire)) {
            REX::WARN("QuestNativeWriteQA: refused without launch consent or live UI");
            return;
        }
#ifdef K2040_GENERIC_MISC_QUESTWIDE_QA
        const auto n=strnlen_s(borrowed,20);
        if(n!=10 && n!=19)return;
        const auto preview=K2040::QuestTracking::ParseRequest(
            std::string_view(borrowed,10));
#else
        const auto preview=K2040::QuestTracking::ParseRequest(
            std::string_view(borrowed,strnlen_s(borrowed,11)));
#endif
        if(!preview)return;
        const auto id=preview->formId;
        const auto accepted=K2040::OneShotResearch::QueueWriteTestOnce(
            dryRunGate,borrowed,trackingSaveEpoch.Capture(),uiEpoch.Capture(),
            writeQaAttempted,
            []() -> K2040::OneShotResearch::LiveJournalContext {
                const auto id=dryRunSelectedFormId.load(std::memory_order_acquire);
                return {
                    id,trackingSaveEpoch.Capture(),uiEpoch.Capture(),
                    gameLoaded,journalOpen && journalFocused.load() &&
                    Valid(journalView),K2040::QuestViewState::Observed().contains(id),
                    dryRunOgRuntime.load(std::memory_order_acquire)
                };
            },
            [id](K2040::OneShotResearch::WriteTestResult result){
                if (!journalOpen || !journalFocused.load() ||
                    !Valid(journalView)) return;
                const char* status="denied";
                switch(result){
                case K2040::OneShotResearch::WriteTestResult::kAlready:
                    status="already"; break;
                case K2040::OneShotResearch::WriteTestResult::kUnverified:
                    status="unverified"; break;
                case K2040::OneShotResearch::WriteTestResult::kCalledProvisional:
                    status="called"; break;
                default: break;
                }
                char buffer[46]{};
                std::snprintf(buffer,sizeof(buffer),"%08X:%s",id,status);
                interop.InteropCall(journalView,"receiveTrackingWriteTestResult",buffer);
                REX::WARN("QuestNativeWriteQA: result={} (one action consumed)",status);
                SyncSnapshots();
            });
        if (accepted)
            REX::WARN("QuestNativeWriteQA: ONE explicit AddTask submitted (write possible)");
        else
            REX::WARN("QuestNativeWriteQA: request refused, NO task submitted");
    }
#endif
#ifdef K2040_QUESTWIDE_PRODUCTION_CANDIDATE
    void SetCandidateSelection(const char* raw){
        if(!raw || !journalOpen || !journalFocused.load() || !Valid(journalView))return;
        const auto size=strnlen_s(raw,9);
        if(size!=8)return;
        std::uint32_t id{};
        const auto [end,err]=std::from_chars(raw,raw+size,id,16);
        if(err!=std::errc{} || end!=raw+size || !id ||
           !K2040::QuestViewState::Observed().contains(id))return;
        candidateSelectedFormID.store(id,std::memory_order_release);
    }
    void QueueCandidateQuestWide(const char* raw){
        if(!raw || strnlen_s(raw,20)!=19 || raw[10]!=':')return;
        const auto parsed=K2040::QuestTracking::ParseRequest(std::string_view(raw,10));
        if(!parsed)return;
        const auto id=parsed->formId;
        const auto respondNoWrite=[id](){
            if(!journalOpen || !Valid(journalView))return;
            const auto status=candidateGate->Get();
            // The UI must reflect the actual native gate. A refused second
            // request cannot turn an in-flight or blocked first write into
            // a misleading ready button.
            if(status.phase==K2040::QuestWideCandidate::Phase::kBlocked) {
                interop.InteropCall(journalView,"enableQuestWideCandidate","blocked");
                return;
            }
            if(status.phase!=K2040::QuestWideCandidate::Phase::kIdle &&
               status.pending) {
                char pending[48]{};
                std::snprintf(pending,sizeof(pending),"pending:%08X:%u",
                    status.pending->intent.formId,
                    status.pending->intent.desiredTracked?1u:0u);
                interop.InteropCall(journalView,"enableQuestWideCandidate",pending);
                return;
            }
            char response[44]{};
            std::snprintf(response,sizeof(response),"%08X:refused",id);
            interop.InteropCall(journalView,"receiveQuestWideCandidateResult",response);
        };
        // Explicit nonwrite response: never leave JS globally pending
        // after a selection race or a no-longer-eligible request.
        if(!gameLoaded || !journalOpen || !journalFocused.load() ||
           !Valid(journalView) || !candidateOg163.load() ||
           id!=candidateSelectedFormID.load()) {
            respondNoWrite();return;
        }
        // A write request remains bound to the exact originating view.
        // Postwrite readback may survive view recreation; an old queued
        // write may never execute in a different panel generation.
        const auto originatingView=uiEpoch.Capture();
        const bool accepted=K2040::QuestWideCandidate::Queue(
            candidateGate,raw,candidateSaveEpoch.Capture(),candidateSessionEpoch.Capture(),
            [originatingView]()->K2040::OneShotResearch::LiveJournalContext {
                const auto selected=candidateSelectedFormID.load(std::memory_order_acquire);
                return {selected,candidateSaveEpoch.Capture(),candidateSessionEpoch.Capture(),
                        gameLoaded && uiEpoch.IsCurrent(originatingView),
                        journalOpen && journalFocused.load() && Valid(journalView) &&
                            uiEpoch.IsCurrent(originatingView),
                        K2040::QuestViewState::Observed().contains(selected),
                        candidateOg163.load(std::memory_order_acquire)};
            },[](std::uint32_t id,K2040::QuestWideCandidate::Outcome result){
                if(!journalOpen || !Valid(journalView))return;
                const char* status="refused";
                if(result==K2040::QuestWideCandidate::Outcome::kPending)status="pending";
                else if(result==K2040::QuestWideCandidate::Outcome::kBlocked)status="blocked";
                else if(result==K2040::QuestWideCandidate::Outcome::kConfirmed)status="confirmed";
                char response[44]{};
                std::snprintf(response,sizeof(response),"%08X:%s",id,status);
                interop.InteropCall(journalView,"receiveQuestWideCandidateResult",response);
                // Native status is the source of truth even when the view
                // changed selection before a delayed callback was delivered.
                const auto phase=candidateGate->Get().phase;
                if(phase==K2040::QuestWideCandidate::Phase::kIdle ||
                   phase==K2040::QuestWideCandidate::Phase::kBlocked)
                    interop.InteropCall(journalView,"enableQuestWideCandidate",
                        phase==K2040::QuestWideCandidate::Phase::kIdle?"ready":"blocked");
            });
        if(!accepted)respondNoWrite();
    }
#endif
#ifdef K2040_REPEATABLE_MISC_QA
    void GrantRepeatableQaConsent() {
        // Only an explicit in-game acknowledgement from this focused QA
        // panel may enable a new isolated session. No tracking write here.
        const bool eligible=gameLoaded && journalOpen && journalFocused.load() &&
            Valid(journalView) && dryRunOgRuntime.load() &&
            repeatableGate->Get().phase==K2040::RepeatableQuestWide::Phase::kIdle;
        if(eligible) {
            K2040::RepeatableQuestWide::GrantSessionConsent();
            repeatableConsentAtLaunch.store(true,std::memory_order_release);
        }
        if(journalOpen && Valid(journalView))
            interop.InteropCall(journalView,"enableRepeatableMiscTrackingQA",
                                eligible?"ready":"off");
    }
    void QueueRepeatableMiscRequest(const char* borrowed) {
        if(!borrowed || !repeatableConsentAtLaunch.load() || !gameLoaded ||
           !journalOpen || !journalFocused.load() || !Valid(journalView) ||
           !dryRunOgRuntime.load())return;
        const auto n=strnlen_s(borrowed,20);
        if(n!=19 || borrowed[10]!=':')return;
        const auto parsed=K2040::QuestTracking::ParseRequest(
            std::string_view(borrowed,10));
        if(!parsed || parsed->formId!=dryRunSelectedFormId.load())return;
        const auto id=parsed->formId;
        const bool accepted=K2040::RepeatableQuestWide::QueueRepeatable(
            repeatableGate,borrowed,trackingSaveEpoch.Capture(),uiEpoch.Capture(),
            []()->K2040::OneShotResearch::LiveJournalContext {
                const auto id=dryRunSelectedFormId.load(std::memory_order_acquire);
                return {id,trackingSaveEpoch.Capture(),uiEpoch.Capture(),
                        gameLoaded,journalOpen && journalFocused.load() && Valid(journalView),
                        K2040::QuestViewState::Observed().contains(id),
                        dryRunOgRuntime.load(std::memory_order_acquire)};
            },[](std::uint32_t id,K2040::RepeatableQuestWide::Outcome result){
                if(!journalOpen || !Valid(journalView))return;
                const char* status="refused";
                if(result==K2040::RepeatableQuestWide::Outcome::kPending)status="pending";
                else if(result==K2040::RepeatableQuestWide::Outcome::kBlocked)status="blocked";
                else if(result==K2040::RepeatableQuestWide::Outcome::kConfirmed)status="confirmed";
                char response[44]{};
                std::snprintf(response,sizeof(response),"%08X:%s",id,status);
                interop.InteropCall(journalView,"receiveRepeatableTrackingResult",response);
                // Next periodic view poll performs independent readback.
                // Never reconcile inline within the SetActive task.
            });
        if(!accepted && Valid(journalView)){
            char response[44]{};
            std::snprintf(response,sizeof(response),"%08X:refused",id);
            interop.InteropCall(journalView,"receiveRepeatableTrackingResult",response);
        }
    }
#endif
#ifdef K2040_TRACKING_INTEGRATED_QA
#ifdef K2040_SINGLE_MISC_WRITE_QA
    // An explicit ARM confirmation is separate from the writing command.
    // Never arm automatically, on startup, after load or on view recreation.
    void ArmSingleMiscWrite(const char* raw) {
        bool armed=false;
        // ARM itself is explicit single-test consent. The unrelated
        // integrated tracking toggle must not silently disable ARM.
        if(raw && !singleMiscAttempted.load(std::memory_order_acquire) &&
           dryRunOgRuntime.load(std::memory_order_acquire) &&
           gameLoaded && journalOpen && journalFocused.load() &&
           Valid(journalView)) {
            const std::string_view v(raw,strnlen_s(raw,9));
            std::uint32_t id{};
            if(v.size()==8) {
                const auto [end,error]=std::from_chars(v.data(),v.data()+8,id,16);
                armed=error==std::errc{} && end==v.data()+8 && id &&
                    id==dryRunSelectedFormId.load(std::memory_order_acquire) &&
                    K2040::QuestViewState::Observed().contains(id);
                if(armed)singleMiscArmedFormId.store(id,std::memory_order_release);
            }
        }
        if(!armed)singleMiscArmedFormId.store(0,std::memory_order_release);
        if(Valid(journalView))interop.InteropCall(journalView,
            "receiveMiscSingleArm",armed?"armed":"refused");
        REX::WARN("SingleMiscQA: arm={} (FormID omitted)",
                  armed?"armed":"refused");
    }

    // ONE submitted task can invoke ONE vanilla command. Never retry or
    // let an existing non-Misc/Papyrus writer handle this request.
    void QueueSingleMiscWrite(const char* raw) {
        if(!raw || !gameLoaded || !journalOpen ||
           !journalFocused.load(std::memory_order_acquire) ||
           !Valid(journalView) ||
           !dryRunOgRuntime.load(std::memory_order_acquire))return;
        const auto candidate=K2040::QuestTracking::ParseRequest(
            std::string_view(raw,strnlen_s(raw,11)));
        if(!candidate || candidate->formId!=
               K2040::SingleMiscQA::kVerifiedQuestFormID ||
           candidate->formId!=dryRunSelectedFormId.load(std::memory_order_acquire))
            return; // Observed() remains game-thread-only.
#ifndef K2040_SINGLE_MISC_RC
        // The legacy experimental flavor retains explicit ARM then EXECUTE.
        if(candidate->formId!=singleMiscArmedFormId.load(std::memory_order_acquire))
            return;
#endif
        const auto* tasks=F4SE::GetTaskInterface();
        if(!tasks || tasks->Version()!=K2040::OGF4SE::ABI0623::kTaskInterfaceVersion)
            return;
        const auto save=trackingSaveEpoch.Capture();
        const auto view=uiEpoch.Capture();
        const auto id=candidate->formId;
        const auto desired=candidate->desiredTracked;
        if(singleMiscAttempted.exchange(true,std::memory_order_acq_rel))return;
        singleMiscArmedFormId.store(0,std::memory_order_release);
        REX::WARN("SingleMiscQA: one attempt consumed, task submitted "
                  "desired={} (FormID omitted)",desired);
        tasks->AddTask([save,view,id,desired](){
            auto valid=[save,view,id](){
                return trackingSaveEpoch.IsCurrent(save) &&
                       uiEpoch.IsCurrent(view) && gameLoaded && journalOpen &&
                       journalFocused.load(std::memory_order_acquire) &&
                       Valid(journalView) && dryRunOgRuntime.load() &&
                       dryRunSelectedFormId.load()==id &&
                       K2040::QuestViewState::Observed().contains(id);
            };
            const char* status="refused";
            if(valid()) {
                auto before=K2040::Quests::InspectSingleMiscWriteCandidate(id);
                before.validSession=valid();
                auto decision=K2040::SingleMiscQA::Review(before,desired);
                if(decision==K2040::SingleMiscQA::Verdict::kAlready)
                    status="already";
                else if(decision==K2040::SingleMiscQA::Verdict::kReady) {
                    // Verify real game code remains the exact OG163 command
                    // dispatch -> helper path from our *user-tested* read-only
                    // capture. Do not call code from an unchecked RVA.
                    const auto image=K2040::MiscVanillaResearch::CaptureRuntimeCode();
                    const auto base=reinterpret_cast<std::uintptr_t>(
                        GetModuleHandleW(nullptr));
                    using Setter=void(*)(RE::TESQuest*,bool);
                    REL::Relocation<Setter> setter{REL::ID(1567562u)};
                    if(image && image->linkedHelper &&
                       image->helperStatus==
                           K2040::MiscVanillaResearch::LinkedHelperStatus::kCaptured &&
                       image->entries[3].rva==0x009FB850u &&
                       image->linkedHelper->rva==
                           K2040::MiscVanillaResearch::kOGLinkedHelperRva &&
                       base && setter.address()==base+0x005D6AC0u &&
                       valid()) {
                        auto again=K2040::Quests::InspectSingleMiscWriteCandidate(id);
                        again.validSession=valid();
                        if(K2040::SingleMiscQA::Review(again,desired)==
                           K2040::SingleMiscQA::Verdict::kReady &&
                           again.questFlags==before.questFlags &&
                           again.currentInstance==before.currentInstance) {
                            auto* quest=RE::TESForm::GetFormByID<RE::TESQuest>(id);
                            if(quest && quest->GetFormID()==id &&
                               static_cast<std::uint32_t>(quest->data.flags)==
                               before.questFlags && valid()) {
                                // Signature inferred from OG vtable slot 3,
                                // confirmed via bounded runtime capture.
                                // Third argument is the vtable type-6 value.
                                using VanillaToggle=void(*)(std::uint32_t,
                                                             std::uint32_t,
                                                             std::uint32_t);
                                auto toggle=reinterpret_cast<VanillaToggle>(
                                    base+K2040::MiscVanillaResearch::kOGLinkedHelperRva);
                                REX::WARN("SingleMiscQA: vanilla toggle ENTER "
                                          "instance=1 objective=50 (FormID omitted)");
                                toggle(id,K2040::SingleMiscQA::kObservedInstance,6u);
                                status="provisional";
                                REX::WARN("SingleMiscQA: vanilla toggle RETURNED "
                                          "manual Pip-Boy/compass verification required");
                            }
                        }
                    }
                }
            }
            integratedQaEnabled.store(false,std::memory_order_release);
            REX::WARN("SingleMiscQA: END result={} (one process attempt spent)",status);
            if(valid() || (journalOpen && Valid(journalView))) {
                char reply[50]{};
                std::snprintf(reply,sizeof(reply),"%08X:%s",id,status);
                interop.InteropCall(journalView,"receiveMiscSingleResult",reply);
                SyncSnapshots();
            }
        });
    }
#endif
    void QueueMiscTrackingProbe(const char* raw) {
        // Called on Prisma's borrowed-buffer thread: capture only a
        // validated 8-digit FormID, then inspect live game data on
        // the ONE supported F4SE AddTask callback.
        if(!raw || !gameLoaded || !journalOpen ||
           !journalFocused.load(std::memory_order_acquire) ||
           !Valid(journalView))return;
        const std::string_view value(raw,strnlen_s(raw,9));
        if(value.size()!=8)return;
        std::uint32_t id{};
        const auto [end,error]=std::from_chars(
            value.data(),value.data()+value.size(),id,16);
        if(error!=std::errc{} || end!=value.data()+value.size() || !id)
            return;
        const auto save=trackingSaveEpoch.Capture();
        const auto view=uiEpoch.Capture();
        const auto* tasks=F4SE::GetTaskInterface();
        if(!tasks ||
           tasks->Version()!=K2040::OGF4SE::ABI0623::kTaskInterfaceVersion)
            return;
        const auto probe=miscProbeSequence.fetch_add(
            1,std::memory_order_acq_rel)+1u;
        tasks->AddTask([id,save,view,probe](){
            if(!trackingSaveEpoch.IsCurrent(save) ||
               !uiEpoch.IsCurrent(view) || !gameLoaded ||
               !journalOpen || !journalFocused.load() ||
               !Valid(journalView) ||
               dryRunSelectedFormId.load(std::memory_order_acquire)!=id ||
               !K2040::QuestViewState::Observed().contains(id))
                return;
            const auto response=K2040::Quests::CaptureMiscTrackingProbe(id);
            // UI response includes the selected FormID to reject stale DOM
            // callbacks. Diagnostic logs NEVER record that identity.
            // An accepted response always starts with the id and then
            // the exact ",\"available":" field; strip the first field.
            if(const auto anonymous=K2040::Quests::AnonymousMiscProbeJson(response)) {
                REX::INFO("MiscTrackingQA: probe={} SNAPSHOT={} "
                          "(read-only, selected FormID omitted)",probe,*anonymous);
            } else {
                REX::INFO("MiscTrackingQA: probe={} unavailable or invalid snapshot "
                          "(read-only)",probe);
            }
            if(response.size()<4096)
                interop.InteropCall(journalView,"receiveMiscTrackingProbe",response.c_str());
            REX::INFO("MiscTrackingQA: one read-only Pip-Boy objective probe completed "
                      "(quest FormID omitted, no engine mutation)");
        });
        REX::INFO("MiscTrackingQA: probe={} one supported read-only AddTask submitted",
                  probe);
    }
    void QueueMiscDispatchProbe(const char* raw) {
        // No debugger: read four verified RTTI entries and one validated
        // direct-call target; never invoke any engine command.
        // One user-initiated attempt per game process. PRIVATE LOG ONLY.
        if(!raw || !gameLoaded || !journalOpen || !journalFocused.load() ||
           !Valid(journalView) || !dryRunOgRuntime.load())return;
        const std::string_view value(raw,strnlen_s(raw,9));
        if(value.size()!=8)return;
        std::uint32_t id{};
        const auto [end,error]=std::from_chars(
            value.data(),value.data()+value.size(),id,16);
        if(error!=std::errc{} || end!=value.data()+value.size() || !id)return;
        const auto save=trackingSaveEpoch.Capture();
        const auto view=uiEpoch.Capture();
        const auto* task=F4SE::GetTaskInterface();
        if(!task ||
           task->Version()!=K2040::OGF4SE::ABI0623::kTaskInterfaceVersion ||
           miscDispatchCaptured.exchange(true,std::memory_order_acq_rel))
            return;
        task->AddTask([id,save,view](){
            const bool allowed=trackingSaveEpoch.IsCurrent(save) &&
                uiEpoch.IsCurrent(view) && gameLoaded &&
                journalOpen && journalFocused.load() &&
                Valid(journalView) && dryRunOgRuntime.load() &&
                dryRunSelectedFormId.load(std::memory_order_acquire)==id &&
                K2040::QuestViewState::Observed().contains(id);
            if(!allowed)return;
            auto* quest=RE::TESForm::GetFormByID<RE::TESQuest>(id);
            if(!quest || static_cast<std::uint32_t>(quest->data.questType)!=6u)
                return;
            K2040::MiscVanillaResearch::CaptureDiagnostics diagnostic{};
            const auto image=K2040::MiscVanillaResearch::CaptureRuntimeCode(
                &diagnostic);
            if(!image) {
                REX::WARN("MiscDispatchQA: read-only executable snapshot "
                          "unavailable phase={} fault={} slot={} "
                          "(no memory changes)",
                          K2040::MiscVanillaResearch::PhaseName(diagnostic.phase),
                          K2040::MiscVanillaResearch::FaultName(diagnostic.fault),
                          diagnostic.slot);
                interop.InteropCall(journalView,
                    "receiveMiscDispatchProbe","unavailable");
                return;
            }
            // Executable bytes go ONLY into the local private F4SE log,
            // never into shipped package metadata or a public Git commit.
            for(std::size_t i=0;i<image->entries.size();++i)
                REX::INFO("MiscDispatchQA: slot={} RVA=0x{:X} bytes={} "
                          "(runtime read only)",i,image->entries[i].rva,
                          image->entries[i].prefixHex);
            if(image->linkedHelper) {
                REX::INFO("MiscDispatchQA: linked helper RVA=0x{:X} bytes={} "
                          "(runtime read only)",image->linkedHelper->rva,
                          image->linkedHelper->prefixHex);
            } else {
                REX::WARN("MiscDispatchQA: linked helper unavailable reason={} "
                          "fault={} (no memory changes)",
                          K2040::MiscVanillaResearch::HelperStatusName(
                              image->helperStatus),
                          K2040::MiscVanillaResearch::FaultName(
                              image->helperFault));
            }
            interop.InteropCall(journalView,
                "receiveMiscDispatchProbe","captured");
        });
        REX::INFO("MiscDispatchQA: one bounded read-only F4SE AddTask "
                  "submitted (no game state write)");
    }
    void SetIntegratedQAConsent(const char* command) {
        if (!command)return;
        const std::string_view cmd(command,strnlen_s(command,9));
        if (cmd=="DISABLE") {
            integratedQaEnabled.store(false,std::memory_order_release);
#ifdef K2040_SINGLE_MISC_WRITE_QA
            singleMiscArmedFormId.store(0,std::memory_order_release);
#endif
            dryRunGate->BeginSession(trackingSaveEpoch.Capture(),uiEpoch.Capture());
            if(Valid(journalView))
                interop.InteropCall(journalView,"receiveIntegratedTrackingConsent","disabled");
            REX::INFO("IntegratedTrackingQA: CONSENT disabled attemptsUsed={}",
                integratedQaAttempts.load(std::memory_order_acquire));
            return;
        }
        if (cmd!="ENABLE")return;
        if (gameLoaded && journalOpen && journalFocused.load() &&
            Valid(journalView) && dryRunOgRuntime.load() &&
            integratedQaAttempts.load()<K2040::OneShotResearch::kIntegratedMaxAttempts) {
            integratedQaEnabled.store(true,std::memory_order_release);
            interop.InteropCall(journalView,"receiveIntegratedTrackingConsent","enabled");
            REX::WARN("IntegratedTrackingQA: CONSENT enabled attemptsUsed={}",
                integratedQaAttempts.load(std::memory_order_acquire));
        } else if(Valid(journalView)) {
            REX::WARN("IntegratedTrackingQA: CONSENT rejected attemptsUsed={}",
                integratedQaAttempts.load(std::memory_order_acquire));
            interop.InteropCall(journalView,"receiveIntegratedTrackingConsent","rejected");
        }
    }
    void QueueIntegratedTracking(const char* raw) {
        if (!raw || !gameLoaded || !journalOpen ||
            !journalFocused.load(std::memory_order_acquire) ||
            !Valid(journalView) ||
            !integratedQaEnabled.load(std::memory_order_acquire))return;
        const auto parsed=K2040::QuestTracking::ParseRequest(
            std::string_view(raw,strnlen_s(raw,11)));
        if(!parsed)return;
        const auto id=parsed->formId;
        const auto success=K2040::OneShotResearch::QueueIntegratedTracking(
            dryRunGate,raw,trackingSaveEpoch.Capture(),uiEpoch.Capture(),
            integratedQaEnabled,integratedQaAttempts,
            []() -> K2040::OneShotResearch::LiveJournalContext {
                const auto id=dryRunSelectedFormId.load(std::memory_order_acquire);
                return {
                    id,trackingSaveEpoch.Capture(),uiEpoch.Capture(),
                    gameLoaded,journalOpen && journalFocused.load() &&
                    Valid(journalView),K2040::QuestViewState::Observed().contains(id),
                    dryRunOgRuntime.load(std::memory_order_acquire)
                };
            },
            [id](K2040::OneShotResearch::IntegratedResult result) {
                if (!journalOpen || !journalFocused.load() ||
                    !Valid(journalView))return;
                const char* status="rejected";
                switch(result) {
                case K2040::OneShotResearch::IntegratedResult::kAlready:
                    status="already"; break;
                case K2040::OneShotResearch::IntegratedResult::kUnverified:
                    status="unverified"; break;
                case K2040::OneShotResearch::IntegratedResult::kConfirmed:
                    status="confirmed"; break;
                case K2040::OneShotResearch::IntegratedResult::kProvisional:
                    status="provisional"; break;
                default: break;
                }
                const auto used=integratedQaAttempts.load(std::memory_order_acquire);
                const auto remaining=used>=K2040::OneShotResearch::kIntegratedMaxAttempts ?
                    0u : K2040::OneShotResearch::kIntegratedMaxAttempts-used;
                char response[56]{};
                std::snprintf(response,sizeof(response),"%08X:%s:%u",id,status,remaining);
                interop.InteropCall(journalView,"receiveIntegratedTrackingResult",response);
                REX::WARN("IntegratedTrackingQA: UI result={} remaining={} "
                          "(form omitted)",status,remaining);
                SyncSnapshots();
            });
        if(success)
            REX::WARN("IntegratedTrackingQA: UI accepted and task submitted "
                      "desired={} attemptsUsed={} (no FormID)",
                      parsed->desiredTracked,
                      integratedQaAttempts.load(std::memory_order_acquire));
        else
            REX::WARN("IntegratedTrackingQA: UI refused before task submission "
                      "desired={} attemptsUsed={} (no FormID)",
                      parsed->desiredTracked,
                      integratedQaAttempts.load(std::memory_order_acquire));
    }
#endif
#ifdef K2040_DOGMEAT_PRODUCTION_CANDIDATE
    void SetDogmeatSelection(const char* raw) {
        dogmeatSelectedFormID.store(0,std::memory_order_release);
        if(!raw || !gameLoaded || !journalOpen || !journalFocused.load() ||
           !Valid(journalView))return;
        const std::string_view text(raw,strnlen_s(raw,9));
        std::uint32_t form{};
        if(text.size()!=8)return;
        const auto [end,error]=std::from_chars(
            text.data(),text.data()+text.size(),form,16);
        if(error!=std::errc{} || end!=text.data()+text.size() ||
           form!=K2040::SingleMiscQA::kVerifiedQuestFormID ||
           !K2040::QuestViewState::Observed().contains(form))return;
        dogmeatSelectedFormID.store(form,std::memory_order_release);
    }

    // The ONLY write-capable handler in the opt-in production candidate.
    // Normal builds cannot compile this event or call this native helper.
    void QueueDogmeatProduction(const char* raw) {
        // Prisma callbacks may arrive off the game thread. Read ONLY
        // borrowed bytes and atomics here; game/Prisma state is checked
        // on the F4SE AddTask thread before any engine call.
        if(!raw || !dogmeatRuntimeOG163.load(std::memory_order_acquire))
            return;
        const auto parsed=K2040::QuestTracking::ParseRequest(
            std::string_view(raw,strnlen_s(raw,11)));
        if(!parsed || parsed->formId!=
            K2040::SingleMiscQA::kVerifiedQuestFormID ||
           parsed->formId!=
            dogmeatSelectedFormID.load(std::memory_order_acquire))return;
        const auto* tasks=F4SE::GetTaskInterface();
        if(!tasks || tasks->Version()!=
           K2040::OGF4SE::ABI0623::kTaskInterfaceVersion)return;
        const auto view=uiEpoch.Capture();
        const auto save=dogmeatSaveEpoch.Capture();
        const auto id=parsed->formId;
        const auto desired=parsed->desiredTracked;
        // Preserve the successfully tested process-wide one-attempt ceiling
        // pending independent production-candidate in-game validation.
        if(dogmeatProcessAttempted.exchange(true,std::memory_order_acq_rel))
            return;
        REX::WARN("DogmeatProduction: ONE candidate request submitted "
                  "desired={} (FormID omitted)",desired);
        tasks->AddTask([view,save,id,desired](){
            const auto valid=[view,save,id](){
                return uiEpoch.IsCurrent(view) &&
                    dogmeatSaveEpoch.IsCurrent(save) &&
                    gameLoaded && journalOpen &&
                    journalFocused.load(std::memory_order_acquire) &&
                    Valid(journalView) &&
                    dogmeatRuntimeOG163.load(std::memory_order_acquire) &&
                    dogmeatSelectedFormID.load(std::memory_order_acquire)==id &&
                    K2040::QuestViewState::Observed().contains(id);
            };
            K2040::DogmeatProduction::Result result=
                K2040::DogmeatProduction::Result::kRejected;
            if(valid())
                result=K2040::DogmeatProduction::ExecuteVerifiedToggle(
                    id,desired,valid);
            const char* status=result==K2040::DogmeatProduction::Result::kAlready
                ?"already":result==K2040::DogmeatProduction::Result::kProvisional
                ?"provisional":"refused";
            REX::WARN("DogmeatProduction: native command result={} "
                      "(one process attempt spent)",status);
            if(journalOpen && Valid(journalView)) {
                char response[50]{};
                std::snprintf(response,sizeof(response),"%08X:%s",id,status);
                interop.InteropCall(journalView,"receiveDogmeatTrackingResult",
                                    response);
                SyncSnapshots();
            }
        });
    }
#endif

#ifdef K2040_SELECTOR_READONLY_QA
    // Selection comes from the focused Journal, never a client-supplied
    // raw native pointer. This is only an identity check for code reading.
    void SetSelectorReadOnlySelection(const char* raw){
        selectorSelectedFormID.store(0,std::memory_order_release);
        if(!raw || !gameLoaded || !journalOpen || !journalFocused.load() ||
           !Valid(journalView))return;
        const std::string_view text(raw,strnlen_s(raw,9));
        if(text.size()!=8)return;
        std::uint32_t id{};
        const auto[end,error]=std::from_chars(
            text.data(),text.data()+text.size(),id,16);
        if(error!=std::errc{}||end!=text.data()+text.size()||!id||
           !K2040::QuestViewState::Observed().contains(id))return;
        selectorSelectedFormID.store(id,std::memory_order_release);
    }
    void QueueSelectorReadOnlyCapture(const char* raw){
        if(!raw || !selectorOG163.load(std::memory_order_acquire) ||
           !gameLoaded || !journalOpen || !journalFocused.load() ||
           !Valid(journalView))return;
        const std::string_view text(raw,strnlen_s(raw,9));
        if(text.size()!=8)return;
        std::uint32_t id{};
        const auto[end,error]=std::from_chars(
            text.data(),text.data()+text.size(),id,16);
        if(error!=std::errc{}||end!=text.data()+text.size()||!id||
           selectorSelectedFormID.load(std::memory_order_acquire)!=id||
           !K2040::QuestViewState::Observed().contains(id))return;
        const auto* tasks=F4SE::GetTaskInterface();
        if(!tasks || tasks->Version()!=
            K2040::OGF4SE::ABI0623::kTaskInterfaceVersion)return;
        const auto view=uiEpoch.Capture();
        const auto save=selectorSaveEpoch.Capture();
        if(selectorProcessCaptured.exchange(true,std::memory_order_acq_rel))
            return; // one attempt per process; no automatic retry
        tasks->AddTask([view,save,id](){
            if(!uiEpoch.IsCurrent(view)||!selectorSaveEpoch.IsCurrent(save)||
               !gameLoaded||!journalOpen||!journalFocused.load()||
               !Valid(journalView)||
               !selectorOG163.load(std::memory_order_acquire)||
               selectorSelectedFormID.load(std::memory_order_acquire)!=id||
               !K2040::QuestViewState::Observed().contains(id)||
               !RE::TESForm::GetFormByID<RE::TESQuest>(id))return;
            // Inspect all currently visible Pip-Boy rows as anonymous
            // structural classes, without log output containing FormIDs,
            // quest text, native pointers, or selectedEntry arguments.
            // This does NOT prove which selectedEntry positions map to
            // which named row keys and never enables a tracking writer.
            const auto rowAudit=K2040::Quests::CaptureSelectorRowCorrelation();
            REX::INFO("SelectorReadOnlyQA: row-audit={} "
                      "(anonymous, NOT command ABI)",rowAudit);
            K2040::MiscVanillaResearch::CaptureDiagnostics diagnostic{};
            const auto snapshot=
                K2040::MiscVanillaResearch::CaptureRuntimeCode(&diagnostic);
            const char* outcome="unavailable";
            if(!snapshot) {
                REX::WARN("SelectorReadOnlyQA: baseline unavailable "
                          "phase={} fault={} slot={}",
                    K2040::MiscVanillaResearch::PhaseName(diagnostic.phase),
                    K2040::MiscVanillaResearch::FaultName(diagnostic.fault),
                    diagnostic.slot);
            }else{
                K2040::MiscVanillaResearch::ReadFault fault{};
                const auto extended=
                    K2040::MiscVanillaResearch::CaptureSelectorPrefixes(
                        *snapshot,&fault);
                if(extended){
                    // Four known slots, each bounded to 192 bytes. PRIVATE
                    // per-launch F4SE log only; never send raw bytes to UI.
                    for(std::size_t i=0;i<extended->size();++i)
                        REX::INFO("SelectorReadOnlyQA: slot={} RVA=0x{:X} "
                                  "bytes={} (read only)",
                                  i,(*extended)[i].rva,
                                  (*extended)[i].prefixHex);
                    outcome="captured";
                }else{
                    REX::WARN("SelectorReadOnlyQA: extended fixed slots "
                              "unavailable fault={} (read only)",
                              K2040::MiscVanillaResearch::FaultName(fault));
                }
            }
            if(journalOpen && Valid(journalView))
                interop.InteropCall(journalView,
                   "receiveSelectorReadOnlyResult",outcome);
            REX::INFO("SelectorReadOnlyQA: END result={} (no game mutation)",
                      outcome);
        });
        REX::INFO("SelectorReadOnlyQA: one bounded game-thread task "
                  "submitted (no game mutation)");
    }
#endif

    void QueueUIAction(std::function<void()> work) {
        // Captured before dispatch on the Prisma callback thread. A save-load
        // or panel close invalidates any pending request from the old view.
        const auto generation=uiEpoch.Capture();
        if (const auto* tasks=F4SE::GetTaskInterface())
            tasks->AddTask([generation,work=std::move(work)]() mutable {
                if(uiEpoch.IsCurrent(generation))work();
            });
    }
    // The unfocused tracker must remain active while the journal is toggled.
    void QueueTrackerAction(std::function<void()> work) {
        const auto generation=trackerEpoch.Capture();
        if(const auto* tasks=F4SE::GetTaskInterface())
            tasks->AddTask([generation,work=std::move(work)]() mutable {
                if(trackerEpoch.IsCurrent(generation))work();
            });
    }
    void QueueUIString(void (*action)(const char*), const char* raw) {
        if (!raw) return;
        const std::string value(raw);
        if(value.size()>128)return;
        QueueUIAction([action,value]() { action(value.c_str()); });
    }

    // Only handles returned to our own Prisma DOM callbacks reach here.
    // An old generation can be canceled while its DOM task is pending:
    // reclaim that view as long as it is not one of our active handles.
    void RetireStaleView(PrismaView view) {
        if (view && view != journalView && view != trackerView && Valid(view))
            views.Destroy(view);
    }
    // All Prisma binding, UI state and game-engine work belongs on the game thread.
    // DOM-ready may arrive on the browser/render thread.
    void CompleteViewOnGameThread(PrismaView view) {
        // A page may finish loading after a rapid journal reopen or save
        // transition. Its handle no longer belongs to either live view.
        // Tear it down on the F4SE task thread instead of leaving an
        // invisible orphaned browser surface behind.
        if (view != journalView && view != trackerView) {
            RetireStaleView(view);
            return;
        }
        if (!Valid(view)) return;
        if (view == journalView) {
            interop.BindUIEvent(view, "requestCloseJournal", [](const char*) {
                QueueUIAction([](){if(journalOpen && journalFocused)CloseJournal();});
            });
            interop.BindUIEvent(view, "requestJournalEscape", [](const char*) {
                QueueUIAction([](){HandleJournalEscape(true);});
            });
            interop.BindUIEvent(view, "acknowledgeCapturedEscape", [](const char*) {
                QueueUIAction([](){AcknowledgeCapturedEscape();});
            });
            interop.BindUIEvent(view, "requestQuestSnapshot", [](const char*) {
                QueueUIAction([](){
                    ValidateJournalOwnership();
                    if(journalOpen && journalFocused)SyncSnapshots();
                });
            });
            interop.BindUIEvent(view, "requestJournalSettings", [](const char*) {
                QueueUIAction([](){
                    ValidateJournalOwnership();
                    if(journalOpen && journalFocused)SyncSettings();
                });
            });
            interop.BindUIEvent(view, "updateJournalSetting", [](const char* cmd) {
                QueueUIString(UpdateSetting,cmd);
            });
            interop.BindUIEvent(view, "requestHubBindings", [](const char*) {
                QueueUIAction([](){
                    if(journalOpen && journalFocused && Valid(journalView)) {
                        const auto payload=K2040::Hub::SnapshotJson();
                        interop.InteropCall(journalView,"receiveHubBindings",payload.c_str());
                    }
                });
            });
            interop.BindUIEvent(view,"setHubNativeCapture",[](const char* raw){
                const std::string_view action=raw?std::string_view(raw):std::string_view{};
                hubNativeMouseCapture.store(
                    action=="journal"?1:action=="tracker"?2:0,
                    std::memory_order_release);
            });
            interop.BindUIEvent(view, "saveHubKeyboard", [](const char* raw) {
                QueueUIString([](const char* value){UpdateHubBinding(value,false);},raw);
            });
            interop.BindUIEvent(view, "saveHubController", [](const char* raw) {
                QueueUIString([](const char* value){UpdateHubBinding(value,true);},raw);
            });
            interop.BindUIEvent(view, "resetJournalAppearance", [](const char*) {
                QueueUIAction([](){ResetAppearance();});
            });
            interop.BindUIEvent(view, "requestPipboyTracking", [](const char* request) {
                QueueUIString(UpdatePipboyTracking,request);
            });
#ifdef K2040_SELECTOR_READONLY_QA
            interop.BindUIEvent(view,"selectedSelectorReadOnlyQuest",
                [](const char* raw){
                    QueueUIString(SetSelectorReadOnlySelection,raw);
                });
            interop.BindUIEvent(view,"requestSelectorReadOnlyCapture",
                [](const char* raw){
                    QueueUIString(QueueSelectorReadOnlyCapture,raw);
                });
            interop.InteropCall(view,"enableSelectorReadOnlyQA",
                selectorProcessCaptured.load(std::memory_order_acquire)
                  ?"spent":"ready");
#endif
#ifdef K2040_DOGMEAT_PRODUCTION_CANDIDATE
            interop.BindUIEvent(view,"selectedDogmeatQuest",
                [](const char* raw){QueueUIString(SetDogmeatSelection,raw);});
            interop.BindUIEvent(view,"requestDogmeatTracking",
                [](const char* raw){QueueDogmeatProduction(raw);});
            interop.InteropCall(view,"enableDogmeatTrackingProduction",
                dogmeatProcessAttempted.load(std::memory_order_acquire)
                    ?"spent":"ready");
            REX::WARN("DogmeatProduction: narrow candidate {}",
                      dogmeatProcessAttempted.load()?"SPENT":"READY");
#endif
#ifdef K2040_QUESTWIDE_PRODUCTION_CANDIDATE
            interop.BindUIEvent(view,"selectedQuestWideCandidate",[](const char* id){
                QueueUIString(SetCandidateSelection,id);
            });
            interop.BindUIEvent(view,"requestQuestWideCandidateTracking",[](const char* raw){
                QueueUIString(QueueCandidateQuestWide,raw);
            });
            const auto status=candidateGate->Get();
            if(status.phase==K2040::QuestWideCandidate::Phase::kAwaitingRefresh &&
               status.pending) {
                char pending[48]{};
                std::snprintf(pending,sizeof(pending),"pending:%08X:%u",
                    status.pending->intent.formId,
                    status.pending->intent.desiredTracked?1u:0u);
                interop.InteropCall(view,"enableQuestWideCandidate",pending);
            } else {
                interop.InteropCall(view,"enableQuestWideCandidate",
                    status.phase==K2040::QuestWideCandidate::Phase::kIdle
                        ?"ready":"blocked");
            }
#endif
#ifdef K2040_TRACKING_DRYRUN_QA
            // QA-only explicit request, NOT wired to the production disabled
            // "Pip-Boy tracking" control or a double-queued UI handler.
            interop.BindUIEvent(view, "requestPipboyTrackingPreview",
                [](const char* request){ QueueDryRunPreview(request); });
            interop.BindUIEvent(view, "qaSelectedQuest", [](const char* raw) {
                if (!raw) return;
                const std::string_view id(raw, strnlen_s(raw,9));
                std::uint32_t form{};
                if (id.size()!=8) return;
                const auto [end,error]=std::from_chars(
                    id.data(),id.data()+id.size(),form,16);
                if (error==std::errc{} && end==id.data()+id.size())
                    dryRunSelectedFormId.store(form,std::memory_order_release);
            });
            interop.InteropCall(view,"enableTrackingDryRunQA","true");
            REX::INFO("JournalDryRunQA: preview-only controls available");
#ifdef K2040_TRACKING_INTEGRATED_QA
#ifndef K2040_SINGLE_MISC_RC
            interop.BindUIEvent(view,"setIntegratedTrackingConsent",
                [](const char* request){SetIntegratedQAConsent(request);});
            interop.BindUIEvent(view,"requestPipboyTrackingIntegrated",
                [](const char* request){QueueIntegratedTracking(request);});
#endif
            interop.BindUIEvent(view,"requestMiscTrackingProbe",
                [](const char* request){QueueMiscTrackingProbe(request);});
            interop.BindUIEvent(view,"requestMiscDispatchProbe",
                [](const char* request){QueueMiscDispatchProbe(request);});
#ifdef K2040_SINGLE_MISC_WRITE_QA
            const bool spent=singleMiscAttempted.load(std::memory_order_acquire);
#ifdef K2040_SINGLE_MISC_RC
            // Release-candidate view: ordinary Track/Stop button for the
            // strictly whitelisted vanilla DogmeatQuest. No ARM interface.
            interop.BindUIEvent(view,"requestMiscSingleTrackingRC",
                [](const char* request){QueueSingleMiscWrite(request);});
            interop.InteropCall(view,"enableMiscSingleTrackingRC",
                                spent?"spent":"ready");
            REX::WARN("SingleMiscRC: guarded one-click trial {}",
                      spent?"SPENT":"READY");
#else
            interop.BindUIEvent(view,"armMiscSingleWrite",
                [](const char* request){
                    QueueUIString(ArmSingleMiscWrite,request);});
            interop.BindUIEvent(view,"requestMiscSingleExecute",
                [](const char* request){QueueSingleMiscWrite(request);});
            interop.InteropCall(view,"enableMiscSingleWriteQA",
                                spent?"spent":"ready");
            REX::WARN("SingleMiscQA: one-shot vanilla experiment {}",
                      spent?"SPENT":"DISARMED");
#endif
#endif
#ifndef K2040_SINGLE_MISC_RC
            const auto used=integratedQaAttempts.load(std::memory_order_acquire);
            const auto remaining=used>=K2040::OneShotResearch::kIntegratedMaxAttempts
                ? 0u : K2040::OneShotResearch::kIntegratedMaxAttempts-used;
            char remainingText[12]{};
            std::snprintf(remainingText,sizeof(remainingText),"%u",remaining);
            interop.InteropCall(view,"enableIntegratedTrackingQA",remainingText);
            REX::INFO("IntegratedTrackingQA: view created DISARMED, remaining={}",remaining);
#endif // separate general-quest native tracking QA is hidden in the single-Misc RC
#endif
#ifdef K2040_REPEATABLE_MISC_QA
            interop.BindUIEvent(view,"grantRepeatableMiscTrackingQA",
                [](const char*){QueueUIAction(GrantRepeatableQaConsent);});
            interop.BindUIEvent(view,"requestRepeatableMiscQuestTracking",
                [](const char* request){QueueUIString(QueueRepeatableMiscRequest,request);});
            const auto repeatPhase=repeatableGate->Get().phase;
            interop.InteropCall(view,"enableRepeatableMiscTrackingQA",
                !repeatableConsentAtLaunch.load()?"off":
                repeatPhase==K2040::RepeatableQuestWide::Phase::kIdle?"ready":
                repeatPhase==K2040::RepeatableQuestWide::Phase::kAwaitingRefresh?"pending":"blocked");
#endif
#ifdef K2040_TRACKING_NATIVE_WRITE_QA
            interop.BindUIEvent(view,"requestPipboyTrackingWriteOnce",
                [](const char* request){QueueNativeWriteTestOnce(request);});
#ifdef K2040_GENERIC_MISC_QUESTWIDE_QA
            interop.InteropCall(view,"enableGenericMiscQuestWideQA",
                writeQaConsentAtLaunch.load() && !writeQaAttempted.load()
                    ? "true" : "false");
#endif
            if (writeQaConsentAtLaunch.load(std::memory_order_acquire) &&
                !writeQaAttempted.load(std::memory_order_acquire)) {
                interop.InteropCall(view,"enableNativeTrackingWriteQA","true");
                REX::WARN("QuestNativeWriteQA: ONE-WRITE TEST enabled by explicit INI consent");
            } else {
                REX::INFO("QuestNativeWriteQA: DISARMED (default)");
            }
#endif
#endif
            interop.BindUIEvent(view, "requestPinQuest", [](const char* id) {
                QueueUIString([](const char* form) {
                    UpdateQuestFlag(form,K2040::QuestViewState::Kind::Pin);
                },id);
            });
            interop.BindUIEvent(view, "requestFavoriteQuest", [](const char* id) {
                QueueUIString([](const char* form) {
                    UpdateQuestFlag(form,K2040::QuestViewState::Kind::Favorite);
                },id);
            });
            interop.BindUIEvent(view, "requestHideQuest", [](const char* id) {
                QueueUIString([](const char* form) {
                    UpdateQuestFlag(form,K2040::QuestViewState::Kind::Hidden);
                },id);
            });
            interop.BindUIEvent(view, "requestQuestCategory", [](const char* choice) {
                QueueUIString(UpdateCategory,choice);
            });
            // Keep A/B/D-pad on Prisma's built-in Enter/Escape/Arrow navigation.
            if (controller.BindControllerAction) {
                controller.BindControllerAction(view, "X", "journal.pin");
                controller.BindControllerAction(view, "Y", "journal.favorite");
                controller.BindControllerAction(view, "LB", "journal.status-prev");
                controller.BindControllerAction(view, "RB", "journal.status-next");
            }
            if (controller.SetViewRole) controller.SetViewRole(view, UI::ViewRole::kPanel);
            if (controller.SetViewOwnsEscape) controller.SetViewOwnsEscape(view,true);
            journalDomReady=true;
            if (!journalOpen) views.Hide(view);
        } else if (view == trackerView) {
            interop.BindUIEvent(view, "requestQuestSnapshot", [](const char*) {
                QueueTrackerAction([](){if(TrackerMayShow() && trackerDomReady)SyncSnapshots();});
            });
            interop.BindUIEvent(view, "requestJournalSettings", [](const char*) {
                QueueTrackerAction([](){if(TrackerMayShow() && trackerDomReady)SyncSettings();});
            });
            if (controller.SetViewRole) controller.SetViewRole(view, UI::ViewRole::kWidget);
            trackerDomReady=true;
            if (TrackerMayShow()) views.Show(view);
            else views.Hide(view);
        } else return; // Ignore stale callbacks from destroyed views.
        ShowJournalIfReady();
        SyncSettings();
        SyncSnapshots();
    }
    void OnDomReady(PrismaView view) {
        // Never dereference RE/Prisma data, bind JS callbacks, or touch native
        // focus from the DOM-ready callback's potentially foreign thread.
        // The task validates both the current view ID and IsValid when run,
        // so closing/loading during navigation invalidates stale requests.
        if (!view) return;
        const auto generation=uiEpoch.Capture();
        if (auto* tasks = F4SE::GetTaskInterface())
            tasks->AddTask([view,generation](){
                if(uiEpoch.IsCurrent(generation))CompleteViewOnGameThread(view);
                else RetireStaleView(view);
            });
    }
    void OnTrackerDomReady(PrismaView view) {
        if(!view)return;
        const auto generation=trackerEpoch.Capture();
        if(auto* tasks=F4SE::GetTaskInterface())
            tasks->AddTask([view,generation](){
                if(trackerEpoch.IsCurrent(generation))CompleteViewOnGameThread(view);
                else RetireStaleView(view);
            });
    }
    void DestroyViews() {
        uiEpoch.Invalidate();
        trackerEpoch.Invalidate();
#ifdef K2040_SELECTOR_READONLY_QA
        selectorSelectedFormID.store(0,std::memory_order_release);
#endif
#ifdef K2040_DOGMEAT_PRODUCTION_CANDIDATE
        dogmeatSelectedFormID.store(0,std::memory_order_release);
#endif
#ifdef K2040_TRACKING_DRYRUN_QA
        ResetDryRunGate();
#endif
#ifdef K2040_QUESTWIDE_PRODUCTION_CANDIDATE
        ResetCandidateGate();
#endif
        lastPublishedJournal.clear();
        lastPublishedTracker.clear();
        journalOpen = false;
        journalDomReady = false;
        trackerDomReady = false;
        journalFocused = false;
        ReleaseJournalInputGuard();
        if (!apiReady) return;
        for (PrismaView* handle : {&journalView, &trackerView}) {
            if (Valid(*handle)) {
                views.Unfocus(*handle);
                views.Hide(*handle);
                views.Destroy(*handle);
            }
            *handle = 0;
        }
        ReleaseCursorFallback();
    }
    void CreateJournalView() {
        // Do not issue a second CreateView while a first handle is loading.
        if(!apiReady || journalView)return;
        journalView=views.CreateView("K2040QuestJournal/journal.html", OnDomReady);
        // Prisma creates views visible. Hide a reserved nonzero handle
        // immediately, even when the page is still asynchronously loading.
        if(journalView)views.Hide(journalView);
    }
    void CreateViews() {
        if (!apiReady) return;
        // Avoid a conflicting mini tracker unless the user opts in.
        // The full-screen Journal is created separately.
        if (PhosphorHudActive() && !K2040::Settings::Current().trackerOverridePhosphor) {
            if (Valid(trackerView)) views.Hide(trackerView);
            return;
        }
        // Journal panel is lazily created for the next open (fresh per open).
        if (!trackerView && gameLoaded) {
            trackerDomReady=false;
            lastPublishedTracker.clear();
            trackerView = views.CreateView("K2040QuestJournal/tracker.html", OnTrackerDomReady);
            if (trackerView) views.Hide(trackerView);
        }
    }
    std::uint32_t KeyCode(const std::string& value) {
        if(value=="G" || value=="Shift+G")return static_cast<std::uint32_t>(RE::BS_BUTTON_CODE::kG);
        if(value=="F6")return static_cast<std::uint32_t>(RE::BS_BUTTON_CODE::kF6);
        if(value=="F7")return static_cast<std::uint32_t>(RE::BS_BUTTON_CODE::kF7);
        if(value=="F8")return static_cast<std::uint32_t>(RE::BS_BUTTON_CODE::kF8);
        if(value=="F10")return static_cast<std::uint32_t>(RE::BS_BUTTON_CODE::kF10);
        if(value=="F11")return static_cast<std::uint32_t>(RE::BS_BUTTON_CODE::kF11);
        if(value=="F12")return static_cast<std::uint32_t>(RE::BS_BUTTON_CODE::kF12);
        return 0;
    }
    void DispatchAction(K2040::Hub::Action action,bool controllerSource=false) {
        if(!gameLoaded || !apiReady || CompetingGameMenuOpen() || AnotherPrismaViewFocused())return;
        if(controllerSource && journalFocused)return; // focused journal owns LB/RB/X/Y
        if(action==K2040::Hub::Action::Tracker && journalFocused)return;
        const unsigned index=action==K2040::Hub::Action::Journal?0:1;
        const auto now=static_cast<std::uint64_t>(GetTickCount64());
        if(now-lastToggleTicks[index]<250)return; // MCM/fallback double fire protection
        lastToggleTicks[index]=now;
        if(action==K2040::Hub::Action::Journal)OpenJournal();
        else ToggleTracker();
    }
    void QueueInputAction(K2040::Hub::Action action,bool controllerSource) {
        if(const auto* tasks=F4SE::GetTaskInterface())
            tasks->AddTask([action,controllerSource](){DispatchAction(action,controllerSource);});
    }
    class HubMcmHandler final : public Scaleform::GFx::FunctionHandler {
    public:
        explicit HubMcmHandler(K2040::Hub::Action value):action(value){}
        void Call(const Params&) override {
            // MCM has already dispatched its live registered hotkey;
            // on-disk Keybinds.json may be stale until the next game save.
            QueueInputAction(action,false);
        }
    private:
        K2040::Hub::Action action;
    };
    bool F4SEAPI RegisterHubMcm(Scaleform::GFx::Movie* movie,Scaleform::GFx::Value* target) {
        if(!movie || !target || !target->IsObject())return false;
        for(const auto action:{K2040::Hub::Action::Journal,K2040::Hub::Action::Tracker}) {
            auto* handler=new HubMcmHandler(action);
            Scaleform::GFx::Value callback;
            movie->CreateFunction(&callback,handler);
            handler->Release();
            if(!target->SetMember(action==K2040::Hub::Action::Journal?"ToggleJournal":"ToggleTracker",callback))
                return false;
        }
        return true;
    }
    void RegisterHubScaleformOnce() {
        if(hubScaleformRegistered)return;
        if(const auto* scaleform=F4SE::GetScaleformInterface())
            hubScaleformRegistered=scaleform->Register("K2040_Quest_Journal",&RegisterHubMcm);
    }
    struct XInputSample {std::uint32_t buttons{};std::int16_t leftX{},leftY{};};
    XInputSample ReadXInputSample() {
        using Reader=DWORD (WINAPI*)(DWORD,XINPUT_STATE*);
        static const Reader read=[](){
            auto mod=LoadLibraryW(L"xinput1_4.dll");
            if(!mod)mod=LoadLibraryW(L"xinput1_3.dll");
            return mod?reinterpret_cast<Reader>(GetProcAddress(mod,"XInputGetState")):nullptr;
        }();
        if(!read)return {};
        XINPUT_STATE pad{};
        if(read(0,&pad)!=ERROR_SUCCESS)return {};
        static bool lt=false,rt=false;
        const auto hysteresis=[](bool& pressed,BYTE value) {
            if(!pressed&&value>=160)pressed=true;
            else if(pressed&&value<=96)pressed=false;
        };
        hysteresis(lt,pad.Gamepad.bLeftTrigger);
        hysteresis(rt,pad.Gamepad.bRightTrigger);
        std::uint32_t bits=pad.Gamepad.wButtons;
        if(lt)bits|=1u<<16;
        if(rt)bits|=1u<<17;
        return {bits,pad.Gamepad.sThumbLX,pad.Gamepad.sThumbLY};
    }
    const char* ControllerActionName(K2040::ControllerNavigation::Action action) {
        using Action=K2040::ControllerNavigation::Action;
        switch(action) {
        case Action::Up:return "up";
        case Action::Down:return "down";
        case Action::Left:return "left";
        case Action::Right:return "right";
        case Action::Activate:return "activate";
        case Action::Back:return "back";
        case Action::None:return nullptr;
        }
        return nullptr;
    }
    bool ForegroundGame() {
        auto window=GetForegroundWindow();if(!window)return false;
        DWORD pid{};GetWindowThreadProcessId(window,&pid);
        return pid==GetCurrentProcessId();
    }
#ifdef K2040_PIPBOY_SELECTION_READONLY_QA
    void QueuePipboySelectedEntryQA() {
        if(pipboyGfxAttempted.load(std::memory_order_acquire) ||
           pipboyGfxQueued.exchange(true,std::memory_order_acq_rel))
            return;
        const auto* tasks=F4SE::GetTaskInterface();
        if(!tasks || tasks->Version()!=K2040::OGF4SE::ABI0623::kTaskInterfaceVersion) {
            pipboyGfxQueued.store(false,std::memory_order_release);
            REX::WARN("PipboyGfxReadOnlyQA: task-interface-unavailable");
            return;
        }
        // Cross-thread epoch prevents a pending UI task from reading
        // a different save after pre-load/post-load transitions.
        const auto save=pipboyGfxSaveEpoch.Capture();
        tasks->AddUITask([save](){
            auto* ui=RE::UI::GetSingleton();
            if(!pipboyGfxSaveEpoch.IsCurrent(save) ||
               !pipboyGfxGameLoaded.load(std::memory_order_acquire) || !ui ||
               !ui->GetMenuOpen(RE::BSFixedString("PipboyMenu"))) {
                REX::INFO("PipboyGfxReadOnlyQA: pipboy-not-open; no attempt spent");
                pipboyGfxQueued.store(false,std::memory_order_release);
                return;
            }
            if(pipboyGfxAttempted.exchange(true,std::memory_order_acq_rel)) {
                pipboyGfxQueued.store(false,std::memory_order_release);
                return;
            }
            // One READ-ONLY attempt per process, even if GFx path is missing.
            // Do not log names, addresses, FormIDs, instances or objective text.
            const auto o=K2040::PipboySelectedEntryResearch::
                ObserveOpenPipboyOnUIThread();
            REX::INFO("PipboyGfxReadOnlyQA: result={} rootFormIDTyped={} "
                      "syntheticMiscParent={} nativeListSelection={} "
                      "childFormIDTyped={} childInstanceTyped={} "
                      "childTypeTyped={} childQuestTypeTyped={} "
                      "tripleVerified={} writeAuthorized={} (NO game mutation)",
                K2040::PipboySelectedEntryResearch::Name(o.status),
                o.rootFormIDTyped,o.syntheticMiscParent,
                o.selectedFromNativeObjectivesList,o.nestedFormIDTyped,
                o.nestedInstanceTyped,o.nestedTypeTyped,
                o.nestedQuestTypeTyped,o.nativeCommandTripleVerified,
                o.nativeWriteAuthorized);
#ifdef K2040_PIPBOY_SELECTION_SCHEMA_READONLY_QA
            const auto binding=K2040::PipboySelectedEntryResearch::
                ObserveOpenPipboyCodeObjectOnUIThread();
            REX::INFO("PipboyCodeObjectReadOnlyQA: state={} memberType={} "
                      "callbackAbiVerified={} writeAuthorized={} (NO invocation)",
                K2040::PipboySelectedEntryResearch::BindingStateName(binding.state),
                K2040::PipboySelectedEntryResearch::BindingMemberTypeName(
                    binding.memberType),
                binding.nativeCallbackAbiVerified,binding.nativeWriteAuthorized);
            // Explicit Ctrl+Shift+F11 one-shot only. Read fixed OG163
            // PipboyMenu vtable slots; no dispatch, no pointer invocation.
            // Any captured code bytes stay in the PRIVATE per-session QA log.
            if(o.status==K2040::PipboySelectedEntryResearch::Status::kMiscChildSelected &&
               binding.memberType==K2040::PipboySelectedEntryResearch::
                   BindingMemberType::kObject) {
                const auto native=K2040::PipboyDispatchResearch::
                    CaptureOriginalOG163Prefixes();
                REX::INFO("PipboyDispatchNativeReadOnlyQA: status={} "
                          "callbackAbiVerified={} writeAuthorized={} (NO invocation)",
                    K2040::PipboyDispatchResearch::StatusName(native.status),
                    native.callbackAbiVerified,native.nativeWriteAuthorized);
                if(native.status==K2040::PipboyDispatchResearch::Status::kCaptured) {
                    for(const auto& [label,entry]: {
                            std::pair{"call",std::cref(native.call)},
                            std::pair{"map",std::cref(native.map)}}) {
                        for(std::size_t window=0;
                            window<K2040::PipboyDispatchResearch::kWindowsPerMethod;
                            ++window) {
                            REX::INFO("PipboyDispatchNativeReadOnlyQA: "
                                      "method={} window={} rva={} hex={}",
                                label,window,entry.get().rva+
                                    window*K2040::PipboyDispatchResearch::kPrefixBytes,
                                entry.get().windows[window]);
                        }
                    }
                }
            }
            if(o.status==K2040::PipboySelectedEntryResearch::Status::kMiscChildSelected) {
                const auto s=K2040::PipboySelectedEntryResearch::
                    ObserveOpenPipboySchemaOnUIThread();
                REX::INFO("PipboySchemaReadOnlyQA: available={} overflow={} "
                          "total={} integer={} number={} boolean={} string={} "
                          "object={} array={} other={} nameMask={} numericNameMask={} "
                          "ordinalMask={} numericOrdinalMask={} "
                          "tripleVerified={} writeAuthorized={}",
                    s.available,s.overflow,s.total,s.integer,s.number,
                    s.boolean,s.string,s.object,s.array,s.other,
                    s.nameMask,s.numericNameMask,s.ordinalMask,s.numericOrdinalMask,
                    s.nativeCommandTripleVerified,
                    s.nativeWriteAuthorized);
            } else {
                REX::INFO("PipboySchemaReadOnlyQA: unavailable: selected Misc child required");
            }
#endif
            pipboyGfxQueued.store(false,std::memory_order_release);
            REX::INFO("PipboyGfxReadOnlyQA: END one-shot read-only");
        });
        REX::INFO("PipboyGfxReadOnlyQA: UI task submitted");
    }
#endif
    void PollHubPads() {
        struct Active {unsigned primary{},held{};bool wasDown{};};
        std::array<Active,2> active{};
        std::array<K2040::Hub::Pad,2> bindings{};
        auto next=std::chrono::steady_clock::time_point{};
        struct KeyboardActive {unsigned key{},mods{};bool wasMatched{};};
        std::array<KeyboardActive,2> keyboard{};
        K2040::ControllerNavigation::Sampler navigation;
        bool escapeWasDown=false;
        bool journalWasFocused=false;
        bool oldMouse4=false,oldMouse5=false;
#ifdef K2040_PIPBOY_SELECTION_READONLY_QA
        bool qaWasDown=false;
#endif
        auto nextKeyboard=std::chrono::steady_clock::time_point{};
        auto nextMcmRefresh=std::chrono::steady_clock::time_point{};
        const auto down=[](int vk){return vk>0 && (GetAsyncKeyState(vk)&0x8000)!=0;};
        const auto virtualKey=[](unsigned k)->int{
            if(k==258)return VK_MBUTTON;
            if(k==259)return VK_XBUTTON1;
            if(k==260)return VK_XBUTTON2;
            return k<=255?static_cast<int>(k):0;
        };
        while(padPolling.load()) {
            const auto now=std::chrono::steady_clock::now();
            // Native MCM owns the live registry. All GFx calls must run
            // through F4SE UI tasks, never on this input worker thread.
            if(now>=nextMcmRefresh && K2040::Hub::NativeMcmOwnsRegistry()){
                if(const auto* tasks=F4SE::GetTaskInterface())
                    tasks->AddUITask([](){K2040::Hub::RefreshNativeMcm();});
                nextMcmRefresh=now+std::chrono::milliseconds(750);
            }
            if(now>=nextKeyboard) {
                for(unsigned i=0;i<2;++i) {
                    const auto b=K2040::Hub::ReadKeyboard(
                        i?K2040::Hub::Action::Tracker:K2040::Hub::Action::Journal);
                    const unsigned key=b.installed&&b.assigned&&!b.conflict?b.keycode:0;
                    const unsigned mods=key?b.modifiers:0;
                    if(key!=keyboard[i].key||mods!=keyboard[i].mods) {
                        // A changed binding starts with the chord's current
                        // state, not just the base key's state.
                        const bool matched=down(virtualKey(key)) &&
                            down(VK_SHIFT)==bool(mods&1u) &&
                            down(VK_CONTROL)==bool(mods&2u) &&
                            down(VK_MENU)==bool(mods&4u);
                        keyboard[i]={key,mods,matched};
                    }
                }
                nextKeyboard=now+std::chrono::milliseconds(400);
            }
            const bool focused=journalFocused.load(std::memory_order_acquire);
            const bool shift=down(VK_SHIFT),ctrl=down(VK_CONTROL),alt=down(VK_MENU);
#ifdef K2040_PIPBOY_SELECTION_READONLY_QA
            // Isolated QA-only chord. Never read GFx on this worker.
            // Rising edge prevents repeat, even when the key is held.
            const bool qaChord=shift && ctrl && !alt && down(VK_F11);
            if(qaChord && !qaWasDown && ForegroundGame())
                QueuePipboySelectedEntryQA();
            qaWasDown=qaChord;
#endif
            // Capture the physical high-bit XBUTTON press: Prisma's DOM
            // does not consistently receive Mouse4/5 under Proton.
            const bool mouse4=down(VK_XBUTTON1),mouse5=down(VK_XBUTTON2);
            const int capture=hubNativeMouseCapture.load(std::memory_order_acquire);
            if(capture && focused && (mouse4&&!oldMouse4 || mouse5&&!oldMouse5)){
                const unsigned physical=mouse4&&!oldMouse4?259u:260u;
                const unsigned chord=(shift?1u:0u)|(ctrl?2u:0u)|(alt?4u:0u);
                if(hubNativeMouseCapture.exchange(0,std::memory_order_acq_rel)==capture){
                    if(const auto* tasks=F4SE::GetTaskInterface())
                        tasks->AddTask([capture,physical,chord](){
                            if(!journalOpen || !journalFocused || !Valid(journalView))return;
                            const auto action=capture==1?"journal":"tracker";
                            char request[90]{};
                            std::snprintf(request,sizeof(request),
                                "{\"action\":\"%s\",\"first\":%u,\"second\":%u}",
                                action,physical,chord);
                            UpdateHubBinding(request,false);
                            interop.InteropCall(journalView,"receiveHubNativeMouseCapture",action);
                        });
                }
            }
            oldMouse4=mouse4;oldMouse5=mouse5;
            if(!focused)hubNativeMouseCapture.store(0,std::memory_order_release);
            const bool gameInputActive=focused || ForegroundGame();
            // An exact chord transitions to pressed when *either* the base
            // key or its modifier completes the combination. Polling only
            // the J down-edge made Shift+J intermittently miss under Proton.
            for(unsigned i=0;i<2;++i) {
                auto& k=keyboard[i];
                const bool matched=k.key && down(virtualKey(k.key)) &&
                    shift==bool(k.mods&1u) && ctrl==bool(k.mods&2u) &&
                    alt==bool(k.mods&4u);
                const bool rising=matched&&!k.wasMatched;
                k.wasMatched=matched;
                if(!rising||!gameInputActive)continue;
                QueueInputAction(i?K2040::Hub::Action::Tracker:K2040::Hub::Action::Journal,false);
            }
            const bool escapeDown=down(VK_ESCAPE);
            // Do not interpret Escape held *before* the journal receives
            // focus as a close gesture upon its eventual release.
            if(!focused || !journalWasFocused) {
                escapeWasDown=escapeDown;
                if(!escapeDown)
                    escapeUiAcknowledged.store(false,std::memory_order_release);
            } else if(!escapeDown && escapeWasDown) {
                // Browser Escape may already have been processed on key-down.
                // Treat the entire physical press as one gesture even when held.
                if(!escapeUiAcknowledged.exchange(false,std::memory_order_acq_rel)) {
                    if(const auto* tasks=F4SE::GetTaskInterface())
                        tasks->AddTask([](){HandleJournalEscape();});
                }
                escapeWasDown=false;
            } else {
                escapeWasDown=escapeDown;
            }
            journalWasFocused=focused;
            const std::uint32_t guardKey=deferredJournalReleaseKey.load(
                std::memory_order_acquire);
            if(guardKey && (!down(static_cast<int>(guardKey)) ||
               GetTickCount64()-deferredJournalReleaseAt.load(std::memory_order_acquire)>=5000) &&
               !deferredJournalReleaseQueued.exchange(true,std::memory_order_acq_rel)) {
                if(const auto* tasks=F4SE::GetTaskInterface())
                    tasks->AddTask([guardKey](){FinishDeferredJournalInputGuard(guardKey);});
                else deferredJournalReleaseQueued.store(false,std::memory_order_release);
            }
            const auto pad=ReadXInputSample();
            const auto buttons=pad.buttons;
            const auto navigationAction=navigation.Update(
                {static_cast<std::uint16_t>(buttons),pad.leftX,pad.leftY},
                focused&&ForegroundGame(),now);
            if(const auto* name=ControllerActionName(navigationAction)) {
                QueueUIAction([name](){
                    if(!journalOpen||!journalFocused.load(std::memory_order_acquire)||
                       !Valid(journalView)||CompetingGameMenuOpen()||
                       AnotherPrismaViewFocused())return;
                    if(interop.InteropCall)
                        interop.InteropCall(journalView,
                            "receiveJournalControllerAction",name);
                });
            }
            if(now>=next) {
                for(unsigned i=0;i<2;++i) {
                    bindings[i]=K2040::Hub::ReadPad(i?K2040::Hub::Action::Tracker:K2040::Hub::Action::Journal);
                    if(bindings[i].primary!=active[i].primary || bindings[i].held!=active[i].held) {
                        active[i].primary=bindings[i].primary;
                        active[i].held=bindings[i].held;
                        active[i].wasDown=(buttons & K2040::Hub::PadMask(active[i].primary))!=0;
                    }
                }
                next=now+std::chrono::seconds(1);
            }
            for(unsigned i=0;i<2;++i) {
                const auto& b=bindings[i];
                const auto primary=K2040::Hub::PadMask(b.primary);
                const auto held=K2040::Hub::PadMask(b.held);
                const bool down=primary&&(buttons&primary)==primary;
                const bool complete=down&&(!held||(buttons&held)==held);
                const bool rising=down&&!active[i].wasDown;
                active[i].wasDown=down;
                if(rising&&complete&&!b.conflict&&ForegroundGame()&&
                   !K2040::ControllerNavigation::ConflictsWithMenuNavigation(
                       primary,held,focused))
                    QueueInputAction(i?K2040::Hub::Action::Tracker:K2040::Hub::Action::Journal,true);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(35));
        }
    }
    void RegisterPadPollingOnce(){
        if(padPolling.exchange(true))return;
        std::thread(PollHubPads).detach();
    }
    class Hotkeys final : public RE::BSInputEventUser {
    public:
        bool ShouldHandleEvent(const RE::InputEvent* event) override {
            return event && event->eventType == RE::INPUT_EVENT_TYPE::kButton &&
                   event->device == RE::INPUT_DEVICE::kKeyboard;
        }
        void OnButtonEvent(const RE::ButtonEvent* event) override {
            if (!event || !event->QJustPressed() || !apiReady) return;
            const auto key = static_cast<std::uint32_t>(event->GetBSButtonCode());
            const auto& preferences=K2040::Settings::Current();
            const bool shift=(GetAsyncKeyState(VK_SHIFT)&0x8000)!=0;
            const bool ctrl=(GetAsyncKeyState(VK_CONTROL)&0x8000)!=0;
            const bool alt=(GetAsyncKeyState(VK_MENU)&0x8000)!=0;
            if(ctrl || alt)return;
            const auto matches=[&](const std::string& binding){
                return key==KeyCode(binding) && shift==(binding=="Shift+G");
            };
            if(matches(preferences.journalHotkey) &&
               !K2040::Hub::ReadKeyboard(K2040::Hub::Action::Journal).registered)
                DispatchAction(K2040::Hub::Action::Journal);
            else if(matches(preferences.trackerHotkey) &&
                    !K2040::Hub::ReadKeyboard(K2040::Hub::Action::Tracker).registered)
                DispatchAction(K2040::Hub::Action::Tracker);
        }
        static Hotkeys& Get() { static Hotkeys instance; return instance; }
    };
    void RegisterHotkeysOnce() {
        if (!apiReady || hotkeysRegistered) return;
        if (auto* menus = RE::MenuControls::GetSingleton()) {
            // Use CommonLibF4's engine registration API, not a raw handlers-array append.
            menus->RegisterHandler(&Hotkeys::Get());
            hotkeysRegistered = true;
        }
    }
    bool InitializePrismaIfAvailable() {
        // kGameDataReady can precede Prisma provider readiness on some loads.
        // Keep retrying after a new game or completed save load, as the
        // existing minimap integration already does. Never create a view here.
        if(!apiReady) {
            apiReady = UI::Discover<UI::ApiFeature::Core>(UI::CoreApiVersion, core) &&
                       UI::Discover<UI::ApiFeature::View>(UI::ViewApiVersion, views) &&
                       UI::Discover<UI::ApiFeature::Interop>(UI::InteropApiVersion, interop);
            if(!apiReady)return false;
            K2040::Settings::Load();
            trackerEnabled=K2040::Settings::Current().trackerEnabled;
            // Optional controller API: missing support leaves keyboard/mouse.
            UI::Discover<UI::ApiFeature::Controller>(UI::ControllerApiVersion, controller);
        }
        RegisterHotkeysOnce();
        RegisterHubScaleformOnce();
        RegisterPadPollingOnce();
        return true;
    }
    void Messages(F4SE::MessagingInterface::Message* message) {
        if (!message) return;
        switch (message->type) {
        case F4SE::MessagingInterface::kGameDataReady:
            if(InitializePrismaIfAvailable() && gameLoaded)CreateViews();
            break;
        case F4SE::MessagingInterface::kInputLoaded:
            RegisterHotkeysOnce();
            break;
        case F4SE::MessagingInterface::kPreLoadGame:
#ifdef K2040_PIPBOY_SELECTION_READONLY_QA
            pipboyGfxSaveEpoch.Invalidate();
            pipboyGfxGameLoaded.store(false,std::memory_order_release);
#endif
#ifdef K2040_SELECTOR_READONLY_QA
            selectorSaveEpoch.Invalidate();
            selectorSelectedFormID.store(0,std::memory_order_release);
#endif
#ifdef K2040_DOGMEAT_PRODUCTION_CANDIDATE
            dogmeatSaveEpoch.Invalidate();
            dogmeatSelectedFormID.store(0,std::memory_order_release);
#endif
#ifdef K2040_TRACKING_DRYRUN_QA
            trackingSaveEpoch.Invalidate();
            ResetDryRunGate();
#endif
#ifdef K2040_QUESTWIDE_PRODUCTION_CANDIDATE
            candidateSaveEpoch.Invalidate();
            candidateSessionEpoch.Invalidate();
            ResetCandidateGate();
#endif
            // Block new Prisma views throughout a save transition.
            gameLoaded=false;
            DestroyViews();
            K2040::QuestViewState::Clear();
            break;
        case F4SE::MessagingInterface::kNewGame:
        case F4SE::MessagingInterface::kPostLoadGame:
#ifdef K2040_PIPBOY_SELECTION_READONLY_QA
            pipboyGfxSaveEpoch.Invalidate();
#endif
#ifdef K2040_SELECTOR_READONLY_QA
            selectorSaveEpoch.Invalidate();
            selectorSelectedFormID.store(0,std::memory_order_release);
#endif
#ifdef K2040_DOGMEAT_PRODUCTION_CANDIDATE
            dogmeatSaveEpoch.Invalidate();
            dogmeatSelectedFormID.store(0,std::memory_order_release);
#endif
#ifdef K2040_TRACKING_DRYRUN_QA
            trackingSaveEpoch.Invalidate();
            ResetDryRunGate();
#endif
#ifdef K2040_QUESTWIDE_PRODUCTION_CANDIDATE
            candidateSaveEpoch.Invalidate();
            candidateSessionEpoch.Invalidate();
            ResetCandidateGate();
#endif
            // Never erase F4SE-remapped co-save metadata at kPostLoadGame.
            if (message->type == F4SE::MessagingInterface::kNewGame)
                K2040::QuestViewState::Clear();
            DestroyViews();
            gameLoaded=true;
            if(InitializePrismaIfAvailable())CreateViews();
#ifdef K2040_PIPBOY_SELECTION_READONLY_QA
            // Permit the diagnostic only after save-load initialization.
            pipboyGfxGameLoaded.store(true,std::memory_order_release);
#endif
            break;
        default: break;
        }
    }
}

// Preserve the legacy F4SE export required by Fallout 4 1.10.163.
F4SE_EXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface* f4se, F4SE::PluginInfo* info) {
    if (!f4se || !info || f4se->IsEditor()) return false;
#if defined(K2040_TRACKING_DRYRUN_QA) || \
    defined(K2040_DOGMEAT_PRODUCTION_CANDIDATE) || \
    defined(K2040_SELECTOR_READONLY_QA) || \
    defined(K2040_PIPBOY_SELECTION_READONLY_QA)
    if (f4se->RuntimeVersion() != F4SE::RUNTIME_1_10_163) return false;
#endif
    info->infoVersion = F4SE::PluginInfo::kVersion;
    info->name = "K2040_Quest_Journal";
    info->version = 0x00000002;
    return true;
}
F4SE_PLUGIN_LOAD(const F4SE::LoadInterface* load) {
    if (!load) return false;
#ifdef K2040_PIPBOY_SELECTION_READONLY_QA
    if(load->RuntimeVersion()!=F4SE::RUNTIME_1_10_163)return false;
#endif
#ifdef K2040_SELECTOR_READONLY_QA
    selectorOG163.store(load->RuntimeVersion()==F4SE::RUNTIME_1_10_163,
                        std::memory_order_release);
    if(!selectorOG163.load(std::memory_order_acquire))return false;
#endif
#ifdef K2040_DOGMEAT_PRODUCTION_CANDIDATE
    dogmeatRuntimeOG163.store(
        load->RuntimeVersion()==F4SE::RUNTIME_1_10_163,
        std::memory_order_release);
    if(!dogmeatRuntimeOG163.load(std::memory_order_acquire))return false;
#endif
#ifdef K2040_QUESTWIDE_PRODUCTION_CANDIDATE
    candidateOg163.store(load->RuntimeVersion()==F4SE::RUNTIME_1_10_163,
                         std::memory_order_release);
    if(!candidateOg163.load())return false;
    ResetCandidateGate();
#endif
#ifdef K2040_TRACKING_DRYRUN_QA
    // Each nonshipping test flavor has its own diagnostic log.
#ifdef K2040_TRACKING_NATIVE_WRITE_QA
    constexpr const char* qaLogName="K2040_Quest_Journal_WriteOnce_QA";
#elif defined(K2040_TRACKING_INTEGRATED_QA)
    // F4SE::InitLog opens basic_file_sink_mt with truncate=true, so a
    // fixed filename would overwrite prior gameplay evidence on restart.
    // UTC date + time + PID gives each game launch its own log. The buffer
    // is static because InitInfo::logName is a borrowed const char*.
    static char qaSessionLogName[128]{};
    SYSTEMTIME utc{};
    GetSystemTime(&utc);
    std::snprintf(qaSessionLogName, sizeof(qaSessionLogName),
        "K2040_Quest_Journal_IntegratedTracking_QA_%04u%02u%02u_%02u%02u%02u_%lu",
        static_cast<unsigned>(utc.wYear),
        static_cast<unsigned>(utc.wMonth),
        static_cast<unsigned>(utc.wDay),
        static_cast<unsigned>(utc.wHour),
        static_cast<unsigned>(utc.wMinute),
        static_cast<unsigned>(utc.wSecond),
        static_cast<unsigned long>(GetCurrentProcessId()));
    const char* qaLogName=qaSessionLogName;
#else
    constexpr const char* qaLogName="K2040_Quest_Journal_DryRun_QA";
#endif
    F4SE::Init(load, F4SE::InitInfo{
        .log = true,
        .logName = qaLogName,
        .hook = false
    });
    dryRunOgRuntime.store(load->RuntimeVersion()==F4SE::RUNTIME_1_10_163);
    if (!dryRunOgRuntime.load()) return false;
    ResetDryRunGate();
#ifdef K2040_REPEATABLE_MISC_QA
    repeatableConsentAtLaunch.store(K2040::RepeatableQuestWide::ReadRepeatableConsent());
    REX::WARN("QuestRepeatableQA: startup consent={} (default NO)",
              repeatableConsentAtLaunch.load());
#elif defined(K2040_TRACKING_NATIVE_WRITE_QA)
    writeQaConsentAtLaunch.store(
        K2040::OneShotResearch::ReadExplicitWriteTestConsent(),
        std::memory_order_release);
    REX::WARN("QuestNativeWriteQA: startup consent={}; one native call max",
        writeQaConsentAtLaunch.load());
#elif defined(K2040_TRACKING_INTEGRATED_QA)
    REX::WARN(
        "IntegratedTrackingQA: START session log={} OG163={} "
        "writer=QA-only one-shot AddTask consent=DISARMED "
        "attemptLimit={}",
        qaLogName,dryRunOgRuntime.load(),
        K2040::OneShotResearch::kIntegratedMaxAttempts);
#else
    REX::INFO("JournalDryRunQA: isolated preview build loaded (NO SetActive)");
#endif
#elif defined(K2040_PIPBOY_SELECTION_READONLY_QA)
    static char qaSessionLogName[128]{};
    SYSTEMTIME utc{};
    GetSystemTime(&utc);
    std::snprintf(qaSessionLogName,sizeof(qaSessionLogName),
        "K2040_Quest_Journal_PipboyGfx_ReadOnly_%04u%02u%02u_%02u%02u%02u_%lu",
        static_cast<unsigned>(utc.wYear),static_cast<unsigned>(utc.wMonth),
        static_cast<unsigned>(utc.wDay),static_cast<unsigned>(utc.wHour),
        static_cast<unsigned>(utc.wMinute),static_cast<unsigned>(utc.wSecond),
        static_cast<unsigned long>(GetCurrentProcessId()));
    F4SE::Init(load,F4SE::InitInfo{
        .log=true,.logName=qaSessionLogName,.hook=false
    });
    REX::WARN("PipboyGfxReadOnlyQA: READY; Ctrl+Shift+F11 inside vanilla "
              "Pip-Boy Quests/Misc; UI-thread GFx read only; NO native writes");
#elif defined(K2040_SELECTOR_READONLY_QA)
    static char qaSessionLogName[128]{};
    SYSTEMTIME utc{};
    GetSystemTime(&utc);
    std::snprintf(qaSessionLogName,sizeof(qaSessionLogName),
        "K2040_Quest_Journal_Selector_ReadOnly_%04u%02u%02u_%02u%02u%02u_%lu",
        static_cast<unsigned>(utc.wYear),static_cast<unsigned>(utc.wMonth),
        static_cast<unsigned>(utc.wDay),static_cast<unsigned>(utc.wHour),
        static_cast<unsigned>(utc.wMinute),static_cast<unsigned>(utc.wSecond),
        static_cast<unsigned long>(GetCurrentProcessId()));
    F4SE::Init(load,F4SE::InitInfo{
        .log=true,.logName=qaSessionLogName,.hook=false
    });
    REX::WARN("SelectorReadOnlyQA: READY; no writer linked; one capture/process");
#else
    F4SE::Init(load);
#endif
    const auto* messaging = F4SE::GetMessagingInterface();
    if (!messaging) return false;
    K2040::Hub::SeedDefaultKeyboardBindings();
    K2040::QuestViewState::RegisterSerialization();
    return messaging->RegisterListener(Messages);
}
