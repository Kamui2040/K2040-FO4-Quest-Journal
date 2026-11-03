'use strict';
const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const source=fs.readFileSync('research/native_write_test/QuestNativeWriteTest.cpp','utf8');
const config=fs.readFileSync('research/native_write_test/K2040_Quest_Native_Write_Test.ini','utf8');
const policy=fs.readFileSync('research/native_write_test/TestPolicy.h','utf8');
const separate=fs.readFileSync('research/native_write_test/xmake.lua','utf8');
const build=fs.readFileSync('scripts/build-native-quest-write-test.sh','utf8');
const prod=fs.readFileSync('xmake.lua','utf8');
const main=fs.readFileSync('src/main.cpp','utf8');
const gate=fs.readFileSync('src/QuestTrackingPolicy.h','utf8');

test('native write experiment is disabled until two explicit consents',()=>{
  assert.match(config,/IHaveBackedUpSave=NO/);
  assert.match(config,/PermitOneNativeTrackingWrite=NO/);
  assert.match(source,/ExplicitlyArmed\(\)/);
  assert.match(source,/exact\(L"IHaveBackedUpSave"\) && exact\(L"PermitOneNativeTrackingWrite"\)/);
  assert.match(source,/if \(!ExplicitlyArmed\(\)\)/);
  assert.match(source,/DISARMED by default; no callbacks installed/);
  assert.match(source,/RUNTIME_1_10_163/g);
});

test('test action must originate from deliberate held in-game chord on F4SE task',()=>{
  assert.match(source,/AddTaskPermanent\(FrameTick\)/);
  assert.match(source,/GetAsyncKeyState\(VK_CONTROL\)/);
  assert.match(source,/GetAsyncKeyState\(VK_MENU\)/);
  assert.match(source,/GetAsyncKeyState\(VK_SHIFT\)/);
  assert.match(source,/GetAsyncKeyState\(VK_F11\)/);
  assert.match(source,/kHoldDurationMs = 2000/);
  assert.match(source,/GetForegroundWindow\(\)/);
  assert.match(source,/GetWindowThreadProcessId/);
  assert.match(source,/prior.vanillaThread == GetCurrentThreadId\(\)/);
  assert.match(source,/attempted.exchange\(true/);
  assert.match(source,/kMaxCandidateAgeMs = 45000/);
});

test('native experiment observes quest event and re-resolves form and Pip-Boy feed',()=>{
  assert.match(source,/RE::BSTEventSink<Event>/);
  assert.match(source,/kUpdateQuestActiveStatus/);
  assert.match(source,/source->RegisterSink\(&observer\)/);
  assert.match(source,/GetFormByID<RE::TESQuest>\(prior.formId\)/);
  assert.match(source,/ReadMainQuestFeed\(prior.formId\)/);
  assert.match(source,/GetMember<RE::PipboyValue\*>/);
  assert.match(source,/formID/);
  assert.match(source,/active/);
  assert.match(policy,/authoritativeFeedTracked/);
  assert.match(policy,/gameTaskMatchesVanillaThread/);
  assert.match(policy,/s.questType == 6/);
  assert.match(policy,/s.candidateAgeMs > 45000/);
});

test('OG engine signature and relocation must match loaded executable',()=>{
  assert.match(source,/kSetRva = 0x005D6AC0u/);
  assert.match(source,/kEventSourceRva = 0x005E2FD0u/);
  assert.match(source,/kEventDispatchRva = 0x005F5110u/);
  assert.match(source,/kSetterId = 1567562u/);
  assert.match(source,/MatchesOg\(\)/);
  assert.match(source,/callTarget\(0x2F\)/);
  assert.match(source,/callTarget\(0x3C\)/);
  assert.match(source,/getter.address\(\)/);
  assert.match(source,/setter.address\(\)/);
});

test('one engine-managed call only, with no custom events or flag patches',()=>{
  assert.match(source,/setter\(quest, desired\)/);
  assert.match(source,/using NativeSetActive = void\(\*\)\(RE::TESQuest\*, bool\)/);
  assert.match(source,/one attempt/i);
  assert.doesNotMatch(source,/DispatchMethodCall|WriteProcessMemory|VirtualProtect|CreateThread|SetStage\(|CompleteQuest\(/);
  assert.doesNotMatch(source,/quest->data.flags\s*[+\-|&]?=/);
  assert.doesNotMatch(source,/\.Notify\(|->Notify\(/);
  assert.match(source,/no pending candidate/);
});

test('standalone DLL cannot accidentally change production Journal',()=>{
  assert.match(separate,/set_filename\("K2040_Quest_Native_Write_Test\.dll"\)/);
  assert.match(separate,/add_files\("QuestNativeWriteTest.cpp"\)/);
  assert.match(build,/K2040_WRITE_TEST_STAGE/);
  assert.match(build,/BLOCKED: unsafe native writer uses absent AddTaskPermanent/);
  assert.match(build,/exit 2/);
  assert.match(source,/#error K2040_NATIVE_WRITER_QUARANTINED_OG_F4SE_0_6_23_TASK_ABI_MISMATCH/);
  assert.match(build,/-P "\$proj"/);
  assert.doesNotMatch(prod,/native_write_test|QuestNativeWriteTest/);
  assert.doesNotMatch(main,/native_write_test|QuestNativeWriteTest|setter\(quest, desired\)/);
  assert.match(gate,/kVerifiedBackend\s*=\s*WriteBackend::kNone/);
  assert.match(main,/static_assert\(!K2040::QuestTracking::CanWritePipboy\(\)/);
});
