'use strict';
const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const source=fs.readFileSync('research/native_event_observer/QuestTrackingEventObserver.cpp','utf8');
const project=fs.readFileSync('research/native_event_observer/xmake.lua','utf8');
const build=fs.readFileSync('scripts/build-native-quest-event-observer.sh','utf8');
const production=fs.readFileSync('xmake.lua','utf8');
const main=fs.readFileSync('src/main.cpp','utf8');
const policy=fs.readFileSync('src/QuestTrackingPolicy.h','utf8');

test('OG-only observer subscribes via the verified event-source getter',()=>{
  assert.match(source,/kVerifiedOgEventGetter\s*=\s*476335/);
  assert.match(source,/kGetterRva\s*=\s*0x005E2FD0u/);
  assert.match(source,/kDispatchRva\s*=\s*0x005F5110u/);
  assert.match(source,/MatchesCapturedOgEngine\(\)/);
  assert.match(source,/GetModuleHandleW\(nullptr\)/);
  assert.match(source,/callTarget\(0x2Fu\)/);
  assert.match(source,/callTarget\(0x3Cu\)/);
  assert.match(source,/getter.address\(\)/);
  assert.match(source,/no subscription/);
  assert.match(source,/RUNTIME_1_10_163/g);
  assert.match(source,/kGameDataReady/);
  assert.match(source,/REL::Relocation<Getter> getter\{REL::ID\(kVerifiedOgEventGetter\)\}/);
  assert.match(source,/source->RegisterSink\(&sink\)/);
  assert.match(source,/RE::BSTEventSink<Event>/);
  assert.match(source,/kUpdateQuestActiveStatus/);
  assert.match(source,/sizeof\(Event\) == 0x10/);
  assert.match(source,/offsetof\(RE::QUEST_DATA, flags\) == 0x4/);
});

test('observer cannot write to quests or fabricate notifications',()=>{
  assert.match(source,/\.hook = false/);
  assert.match(source,/\.log = true/);
  assert.match(source,/kEventLimit = 256/);
  assert.match(source,/return RE::BSEventNotifyControl::kContinue/);
  assert.match(source,/event.quest->GetFormID\(\)/);
  assert.match(source,/event.quest->data.flags/);
  assert.doesNotMatch(source,/\b(?:WriteProcessMemory|VirtualProtect|VirtualAlloc|AddTask|DispatchMethodCall|SetActive\s*\(|SetStage\s*\(|Notify\s*\(|InstallHook|SetFlags\s*\(|SetTracked\s*\()\b/);
  assert.doesNotMatch(source,/\b(?:\.flags\s*=|\.flags\s*\|=|\.flags\s*&=)/);
  assert.doesNotMatch(source,/RegisterForExternalEvent|CreateThread|std::thread/);
});

test('observer omits personal save names and only emits bounded numeric events',()=>{
  assert.match(source,/kPreLoadGame/);
  assert.match(source,/kPostLoadGame/);
  assert.match(source,/kPreSaveGame/);
  assert.match(source,/kPostSaveGame/);
  assert.match(source,/form=\{:08X\}/);
  assert.match(source,/no save name recorded|name omitted/);
  assert.doesNotMatch(source,/msg->data\s*\)/);
  assert.doesNotMatch(source,/GetFullName\(|GetFormEditorID\(/);
});

test('standalone observer is excluded from shipping build and cannot enable writer',()=>{
  assert.match(project,/set_filename\("K2040_Quest_Event_Observer\.dll"\)/);
  assert.match(project,/add_files\("QuestTrackingEventObserver\.cpp"\)/);
  assert.match(build,/K2040_OBSERVER_STAGE/);
  assert.match(build,/-P "\$proj"/);
  assert.match(build,/NOT installed, deployed, loaded, or validated/);
  assert.doesNotMatch(main,/QuestTrackingEventObserver|native_event_observer|kVerifiedOgEventGetter/);
  assert.doesNotMatch(production,/QuestTrackingEventObserver|native_event_observer/);
  assert.match(policy,/kVerifiedBackend\s*=\s*WriteBackend::kNone/);
  assert.match(main,/static_assert\(!K2040::QuestTracking::CanWritePipboy\(\)/);
});
