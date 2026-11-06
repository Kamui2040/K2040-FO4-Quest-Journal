'use strict';
const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const collect=fs.readFileSync('src/QuestCollector.cpp','utf8');
const main=fs.readFileSync('src/main.cpp','utf8');
const view=fs.readFileSync('view/app.js','utf8');
const model=fs.readFileSync('view/quest-model.js','utf8');

test('crash-prone Papyrus tracking mutation is removed, including stale UI callbacks',()=>{
 assert.doesNotMatch(main,/DispatchTrackingPapyrus|vm->DispatchMethodCall|GetObjectHandleForObject|GetHandleForObject\(type,quest\)/);
 assert.doesNotMatch(collect,/SetActive|CheckPipboyTrackingEligibility/);
 assert.match(main,/interop\.BindUIEvent\(view,\s*"requestPipboyTracking"/);
 assert.match(main,/QueueUIString\(UpdatePipboyTracking,request\)/);
 const disabled=main.split('void UpdatePipboyTracking(')[1].split('void UpdateCategory(')[0];
 assert.match(disabled,/QuestTracking::ParseRequest\(argument\)/);
 assert.match(disabled,/static_assert\(!K2040::QuestTracking::CanWritePipboy\(\)/);
 assert.match(disabled,/QuestTracking::kDisabledStatus/);
 assert.match(disabled,/receiveQuestTrackingResult/);
 assert.doesNotMatch(disabled,/QuestViewState::Toggle|SetStage|DispatchMethodCall|SetActive|GetVM/);
});
test('shipping UI remains disabled; native writes require QA DLL and user consent',()=>{
 assert.match(view,/<button[^>]*data-action="pipboy-track" disabled /);
 assert.match(view,/if\(!state\.qaIntegratedAvailable \|\| !state\.qaIntegratedConsent/);
 assert.match(view,/qaIntegratedAvailable:false/);
 assert.match(view,/window\.enableIntegratedTrackingQA/);
 assert.doesNotMatch(view,/emit\('requestPipboyTracking'/);
 assert.match(view,/status==='disabled'/);
 assert.match(view,/data-action="pin"/);
});
test('collector uses engine active quest flag, not only optional feed active property',()=>{
 assert.match(collect,/QuestFlag::kActive/);
 assert.match(collect,/kQuestTrackedBit=0x800u/);
 assert.match(collect,/const bool engineTracked=/);
 assert.match(collect,/const bool feedTracked=Boolean\(entry,"active"\)/);
 assert.match(collect,/item\.pipboyTracked=engineTracked \|\| feedTracked/);
 assert.match(collect,/vanillaTracked/);
});
test('Pip-Boy mirror is additive to manual pins, still respects hidden and history',()=>{
 assert.match(model,/q\.pinned \|\| \(sync && q\.vanillaTracked\)/);
 assert.match(model,/!q\.hidden && !q\.historyOnly/);
});
test('Quick Data redirect follows and consumes the mapped gameplay action',()=>{
 const settings=fs.readFileSync('src/JournalSettings.cpp','utf8');
 const input=fs.readFileSync('src/main.cpp','utf8');
 assert.match(settings,/redirectQuestKey/);
 assert.match(settings,/redirectQuestKey\\":\"\+\(p\.redirectQuestKey\?"true":"false"\)/);
 assert.match(input,/QUserEvent\(\)=="QuickData"/);
 assert.match(input,/handled\s*=\s*RE::InputEvent::HANDLED_RESULT::kStop/);
 assert.match(input,/DispatchAction\(K2040::Hub::Action::Journal\)/);
 assert.match(input,/!CompetingGameMenuOpen\(\) && !AnotherPrismaViewFocused\(\)/);
 const ui=fs.readFileSync('view/journal.html','utf8');
 assert.match(ui,/id="redirectQuestKey" type="checkbox" data-setting="redirectQuestKey"/);
 assert.match(ui,/follows your current key binding and is off by default/);
});
