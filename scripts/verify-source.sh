#!/usr/bin/env bash
# Source-only tests. Never creates a game DLL or deploys into Fallout 4.
set -euo pipefail
cd "$(dirname "$0")/.."
for p in view/quest-model.js view/journal-settings.js view/demo-data.js view/quest-selects.js view/app.js; do node --check "$p"; done
node --test tests/*.test.cjs
python3 -m unittest discover -s tests -p 'test_*.py' -v
if command -v clang++ >/dev/null 2>&1; then
  tmp="$(mktemp -d)"
  trap 'rm -rf "$tmp"' EXIT
  clang++ -std=c++23 -O1 -Wall -Wextra -Itests/stubs -Isrc src/QuestViewState.cpp tests/native_view_state.cpp -o "$tmp/state"
  "$tmp/state"
  clang++ -std=c++23 -O1 -Wall -Wextra -Isrc tests/native_ui_epoch.cpp -o "$tmp/epoch"
  "$tmp/epoch"
  clang++ -std=c++23 -O1 -Wall -Wextra -Werror -pthread -Isrc tests/native_tracking_mailbox.cpp -o "$tmp/tracking-mailbox"
  "$tmp/tracking-mailbox"
  clang++ -std=c++23 -O1 -Wall -Wextra -Werror -pthread -Isrc tests/native_tracking_papyrus_wire.cpp -o "$tmp/tracking-wire"
  "$tmp/tracking-wire"
  # Non-shipping, engine-independent native quest mutation preflight fixture.
  clang++ -std=c++23 -O1 -Wall -Wextra -Werror tests/native_tracking_preflight.cpp -o "$tmp/native-tracking-preflight"
  "$tmp/native-tracking-preflight"
  # Offline-only experimental writer eligibility. No game memory calls.
  clang++ -std=c++23 -O1 -Wall -Wextra -Werror tests/native_write_test_policy.cpp -o "$tmp/native-write-test-policy"
  "$tmp/native-write-test-policy"
  # A genuinely new OG163 isolated QA scope: generic Misc quest-wide
  # eligibility, never an objective-specific native tracking write.
  clang++ -std=c++23 -O1 -Wall -Wextra -Werror tests/native_generic_misc_questwide_policy.cpp -o "$tmp/generic-misc-questwide"
  "$tmp/generic-misc-questwide"
  clang++ -std=c++23 -O1 -Wall -Wextra -Werror tests/native_repeatable_questwide_controller.cpp -o "$tmp/repeatable-questwide"
  "$tmp/repeatable-questwide"
  # Candidate-only ordinary quest scope: no F4SE game calls, default writer OFF.
  clang++ -std=c++23 -O1 -Wall -Wextra -Werror tests/native_questwide_ordinary_controller.cpp -o "$tmp/ordinary-controller"
  "$tmp/ordinary-controller"
  # Real candidate ordinary-feed adapter against synthetic RE structures.
  clang++ -std=c++23 -O1 -Wall -Wextra -Werror -DK2040_QUESTWIDE_PRODUCTION_CANDIDATE=1 -Itests/stubs -Isrc tests/native_questwide_ordinary_evidence.cpp research/production_candidate/OrdinaryQuestEvidence.cpp -o "$tmp/ordinary-evidence"
  "$tmp/ordinary-evidence"
  # Shipping-side candidate classifier: always read-only, never write grant.
  clang++ -std=c++23 -O1 -Wall -Wextra -Werror tests/native_questwide_tracking_readiness.cpp -o "$tmp/questwide-readiness"
  "$tmp/questwide-readiness"
  # Independently verified official F4SE 0.6.23 TaskInterface ABI.
  clang++ -std=c++23 -O1 -Wall -Wextra -Werror tests/native_f4se_og_task_abi.cpp -o "$tmp/native-f4se-og-abi"
  "$tmp/native-f4se-og-abi"
  # One-shot Journal action handshake (engine-independent dry-run only).
  clang++ -std=c++23 -O1 -Wall -Wextra -Werror -pthread tests/native_one_shot_intent.cpp -o "$tmp/native-one-shot-intent"
  "$tmp/native-one-shot-intent"
  clang++ -std=c++23 -O1 -Wall -Wextra -Werror -pthread tests/native_one_shot_scheduler.cpp -o "$tmp/native-one-shot-scheduler"
  "$tmp/native-one-shot-scheduler"
  clang++ -std=c++23 -O1 -Wall -Wextra -Werror -pthread tests/native_integrated_tracking_budget.cpp -o "$tmp/native-integrated-budget"
  "$tmp/native-integrated-budget"
  # Native OG Misc selection contract is research-only: ALWAYS fail closed
  # for ambiguous instances, multiple objectives, or unfinished saves.
  clang++ -std=c++23 -O1 -Wall -Wextra -Werror tests/native_misc_vanilla_selection_preflight.cpp -o "$tmp/native-misc-selection"
  "$tmp/native-misc-selection"
  # Narrow ONE-objective write preflight: engine-independent, denies ambiguity.
  clang++ -std=c++23 -O1 -Wall -Wextra -Werror tests/native_single_misc_write_gate.cpp -o "$tmp/single-misc-gate"
  "$tmp/single-misc-gate"
  # Production candidate pure gate shares the verified identity policy
  # without compiling ANY of the isolated QA macro definitions.
  clang++ -std=c++23 -O1 -Wall -Wextra -Werror tests/native_dogmeat_production_gate.cpp -o "$tmp/dogmeat-production-gate"
  "$tmp/dogmeat-production-gate"
  clang++ -std=c++23 -O1 -Wall -Wextra -Werror -DK2040_DOGMEAT_PRODUCTION_CANDIDATE=1 -Itests/stubs -Isrc src/QuestViewState.cpp src/QuestCollector.cpp tests/native_quest_collector.cpp -o "$tmp/dogmeat-production-collector"
  "$tmp/dogmeat-production-collector"
  # Always-on native quest instance summary, bounded to 512 observations.
  clang++ -std=c++23 -O1 -Wall -Wextra -Werror tests/native_generic_quest_instance_summary.cpp -o "$tmp/native-generic-instance"
  "$tmp/native-generic-instance"
  # General-form-ID, objective and radiant-instance read-only resolver.
  # These synthetic fixtures must not create or authorize a native writer.
  clang++ -std=c++23 -O1 -Wall -Wextra -Werror tests/native_generic_quest_row_identity.cpp -o "$tmp/generic-row-identity"
  "$tmp/generic-row-identity"
  # Independent MAP76-informed quest-WIDE dry-run: no callbacks,
  # no game state writes; all native flags and Misc scopes fail closed.
  clang++ -std=c++23 -O1 -Wall -Wextra -Werror tests/native_questwide_preflight.cpp -o "$tmp/questwide-native-preflight"
  "$tmp/questwide-native-preflight"
  # Native feed adapter's bounded ledger: no RE calls or game IO.
  clang++ -std=c++23 -O1 -Wall -Wextra -Werror tests/native_questwide_snapshot_ledger.cpp -o "$tmp/questwide-feed-ledger"
  "$tmp/questwide-feed-ledger"
  # Read-only vanilla quest+instance scope classification: multiple
  # Misc siblings must not silently become independent native toggles.
  clang++ -std=c++23 -O1 -Wall -Wextra -Werror tests/native_generic_quest_tracking_scope.cpp -o "$tmp/generic-quest-scope"
  "$tmp/generic-quest-scope"
  # Unwired read-only vanilla GFx selectedEntry source observation.
  # A mock movie proves no native quest mutation and refuses stale tabs.
  clang++ -std=c++23 -O1 -Wall -Wextra -Werror tests/native_pipboy_selected_entry_readonly.cpp -o "$tmp/native-selected-entry"
  "$tmp/native-selected-entry"
  # Anonymous, read-only real selectedEntry shape census (no values).
  clang++ -std=c++23 -O1 -Wall -Wextra -Werror tests/native_pipboy_schema_readonly.cpp -o "$tmp/native-schema"
  "$tmp/native-schema"
  clang++ -std=c++23 -O1 -Wall -Wextra -Werror tests/native_pipboy_code_object_binding.cpp -o "$tmp/native-code-object-binding"
  "$tmp/native-code-object-binding"
  clang++ -std=c++23 -O1 -Wall -Wextra -Werror tests/native_pipboy_dispatch_prefix_policy.cpp -o "$tmp/native-dispatch-prefix-policy"
  "$tmp/native-dispatch-prefix-policy"
  # Dedicated selector-only build flavor: bounded anonymized schema
  # survey has no writer symbols and never leaks quest FormIDs/names.
  clang++ -std=c++23 -O1 -Wall -Wextra -Werror -DK2040_SELECTOR_READONLY_QA=1 -Itests/stubs -Isrc src/QuestViewState.cpp src/QuestCollector.cpp tests/native_quest_collector.cpp -o "$tmp/selector-row-audit"
  "$tmp/selector-row-audit"
  # Typed, read-only named Pip-Boy row fields vs TESQuest identity.
  # Never imply a proven positional native SetQuestActive command ABI.
  clang++ -std=c++23 -O1 -Wall -Wextra -Werror tests/native_selector_row_correlation.cpp -o "$tmp/selector-row-correlation"
  "$tmp/selector-row-correlation"
  # Read-only QA correlation: a single candidate is never a verified row.
  clang++ -std=c++23 -O1 -Wall -Wextra -Werror tests/native_misc_objective_identity.cpp -o "$tmp/misc-objective-identity"
  "$tmp/misc-objective-identity"
  clang++ -std=c++23 -O1 -Wall -Wextra -Itests/stubs -Isrc src/QuestViewState.cpp src/QuestCollector.cpp tests/native_quest_collector.cpp -o "$tmp/collector"
  "$tmp/collector"
  # The integrated-only read-only Misc probe gets a second synthetic
  # collector pass, including nested per-objective active flag checks.
  clang++ -std=c++23 -O1 -Wall -Wextra -DK2040_TRACKING_INTEGRATED_QA=1 -Itests/stubs -Isrc src/QuestViewState.cpp src/QuestCollector.cpp tests/native_quest_collector.cpp -o "$tmp/collector-misc-qa"
  "$tmp/collector-misc-qa"
  # Distinct non-shipping one-shot flavor synthetic collector (no engine calls).
  clang++ -std=c++23 -O1 -Wall -Wextra -Werror -DK2040_TRACKING_INTEGRATED_QA=1 -DK2040_SINGLE_MISC_WRITE_QA=1 -Itests/stubs -Isrc src/QuestViewState.cpp src/QuestCollector.cpp tests/native_quest_collector.cpp -o "$tmp/single-misc-collector"
  "$tmp/single-misc-collector"
  clang++ -std=c++23 -O1 -Wall -Wextra -Werror tests/native_misc_probe_log_sanitizer.cpp -o "$tmp/misc-probe-privacy"
  "$tmp/misc-probe-privacy"
  clang++ -std=c++23 -O1 -Wall -Wextra -Isrc tests/native_pipboy_text.cpp -o "$tmp/text"
  "$tmp/text"
  clang++ -std=c++23 -O1 -Wall -Wextra -Itests/stubs -Isrc src/JournalSettings.cpp tests/native_journal_settings.cpp -o "$tmp/preferences"
  "$tmp/preferences"
  # Check the complete native serializer output through the actual JS
  # normalizer; substring assertions alone failed to catch malformed JSON.
  clang++ -std=c++23 -O1 -Wall -Wextra -Itests/stubs -Isrc src/JournalSettings.cpp tests/native_settings_payload.cpp -o "$tmp/payload"
  "$tmp/payload" | node tests/validate-native-settings-payload.cjs
  clang++ -std=c++23 -O1 -Wall -Wextra -Isrc tests/native_hud_color.cpp -o "$tmp/hud-colors"
  "$tmp/hud-colors"
else
  echo 'SKIP synthetic C++ tests: clang++ missing'
fi
if python3 -c 'import playwright.sync_api' >/dev/null 2>&1; then
  python3 tests/ui_smoke.py
else
  echo 'SKIP browser tests: Python Playwright missing'
fi
if git rev-parse --is-inside-work-tree >/dev/null 2>&1; then git diff --check; fi
echo 'PASS: source checks only; no Windows/CommonLibF4 game build or gameplay QA.'
