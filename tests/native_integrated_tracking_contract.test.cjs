'use strict';
const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const read=(f)=>fs.readFileSync(f,'utf8');
const main=read('src/main.cpp');
const browser=read('view/app.js');
const cfg=read('xmake.lua');
const source=read('research/native_one_shot/OneShotOgIntegratedQA.cpp');
const header=read('research/native_one_shot/OneShotOgIntegratedQA.h');
const script=read('scripts/build-integrated-tracking-qa.sh');

test('integrated native tracking ONLY in isolated opt-in QA flavor',()=>{
 assert.match(cfg,/option\("quest_integrated_tracking_qa"\)/);
 assert.match(cfg,/add_defines\("K2040_TRACKING_INTEGRATED_QA=1"\)/);
 assert.match(cfg,/add_files\("research\/native_one_shot\/OneShotOgIntegratedQA.cpp"\)/);
 assert.match(cfg,/not has_config\("quest_dryrun_qa"\) or has_config\("quest_native_write_qa"\)/);
 for(const file of ['scripts/build-linux.sh','scripts/build-one-shot-qa.sh','scripts/build-native-write-once-qa.sh']){
   assert.match(read(file),/--quest_integrated_tracking_qa=n/);
 }
 assert.match(script,/--quest_integrated_tracking_qa=y/);
 assert.match(script,/--quest_native_write_qa=n/);
 assert.match(script,/No game or Amethyst files deployed/);
 assert.match(script,/test ! -e "\$stage\/Data\/F4SE\/Plugins\/K2040_Quest_Journal_Write_QA.ini"/);
 assert.match(main,/#ifdef K2040_TRACKING_INTEGRATED_QA/);
 assert.match(main,/K2040_Quest_Journal_IntegratedTracking_QA/);
});

test('real writer has bounded per-process budget, no ini, only OG AddTask',()=>{
 assert.match(header,/kIntegratedMaxAttempts = 8/);
 assert.match(header,/compare_exchange_weak/);
 assert.match(source,/ReserveIntegratedAttempt\(attempts\)/);
 assert.match(source,/tasks?->AddTask\(/);
 assert.match(source,/GetFormByID<RE::TESQuest>/);
 assert.match(source,/ObserveOnTask\(requested,context\)/);
 assert.match(source,/ReviewOnGameTask\(requested,before\)/);
 assert.match(source,/decision==Result::kWouldWrite/);
 assert.match(source,/setter\.address\(\)==base\+kOGSetActiveRva/);
 assert.match(source,/setter\(quest,requested.desiredTracked\)/);
 assert.match(source,/feedOk/);
 assert.match(source,/kConfirmed/);
 assert.doesNotMatch(source,/AddTaskPermanent\s*\(|ReadExplicitWriteTestConsent|GetPrivateProfile|K2040_Quest_Journal_Write_QA.ini/);
 assert.doesNotMatch(source,/WriteProcessMemory\s*\(|VirtualProtect\s*\(/);
 assert.doesNotMatch(source,/quest->data.flags\s*(?:=|\|=|&=)/);
});

test('in-game user confirmation is required, cleared on view/save changes',()=>{
 assert.match(main,/integratedQaEnabled\{false\}/);
 assert.match(main,/integratedQaAttempts\{0\}/);
 assert.match(main,/void ResetDryRunGate\(\)/);
 assert.match(main,/integratedQaEnabled.store\(false/);
 assert.match(main,/setIntegratedTrackingConsent/);
 assert.match(main,/requestPipboyTrackingIntegrated/);
 assert.match(main,/void SetIntegratedQAConsent\(/);
 assert.match(main,/cmd!="ENABLE"/);
 assert.match(main,/integratedQaAttempts.load\(\)<K2040::OneShotResearch::kIntegratedMaxAttempts/);
 assert.match(main,/interop.InteropCall\(view,"enableIntegratedTrackingQA",remainingText\)/);
 assert.match(browser,/state.qaIntegratedRemaining=remaining/);
 assert.match(browser,/qaIntegratedAvailable:false/);
 assert.match(browser,/qaIntegratedConsent:false/);
 assert.match(browser,/window.enableIntegratedTrackingQA/);
 assert.match(browser,/window.receiveIntegratedTrackingConsent/);
 assert.match(browser,/emit\('setIntegratedTrackingConsent'/);
 assert.match(browser,/if\(!state.qaIntegratedAvailable \|\| !state.qaIntegratedConsent/);
 assert.match(browser,/emit\('requestPipboyTrackingIntegrated'/);
 assert.match(browser,/window.receiveIntegratedTrackingResult/);
 assert.match(browser,/QA test: /);
});

test('shipping remains disabled and no nested double AddTask binding',()=>{
 assert.match(read('src/QuestTrackingPolicy.h'),/kVerifiedBackend\s*=\s*WriteBackend::kNone/);
 assert.match(browser,/data-action="pipboy-track" disabled/);
 assert.match(main,/interop.BindUIEvent\(view, "requestPipboyTracking"/);
 const handler=main.split('void QueueIntegratedTracking(')[1].split('#endif')[0];
 assert.doesNotMatch(handler,/QueueUIAction\(|QueueUIString\(/);
 assert.match(handler,/OneShotResearch::QueueIntegratedTracking/);
 assert.doesNotMatch(source,/F4SEPlugin_Load|F4SEPlugin_Query|SetStage\s*\(/);
});
