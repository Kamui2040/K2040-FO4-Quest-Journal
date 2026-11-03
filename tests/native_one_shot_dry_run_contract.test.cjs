'use strict';
const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');

const bridge=fs.readFileSync('research/native_one_shot/OneShotOgDryRun.cpp','utf8');
const intent=fs.readFileSync('research/native_one_shot/OneShotIntent.h','utf8');
const pipeline=fs.readFileSync('src/main.cpp','utf8');
const ui=fs.readFileSync('view/app.js','utf8');
const policy=fs.readFileSync('src/QuestTrackingPolicy.h','utf8');
const build=fs.readFileSync('xmake.lua','utf8');

test('one-shot bridge uses supported OG F4SE task API exclusively',()=>{
 assert.match(bridge,/F4SE::GetTaskInterface\(\)/);
 assert.match(bridge,/tasks->AddTask\(std::move\(oneTask\)\)/);
 assert.match(bridge,/QueueOneShot\(gate, selectedId, desired, saveEpoch, uiEpoch/);
 assert.match(bridge,/TaskInterface::kVersion == 2/);
 assert.match(bridge,/OGF4SE::ABI0623::kSlotCount == 2/);
 assert.doesNotMatch(bridge,/\bAddTaskPermanent\s*\(|\.AddTaskPermanent\s*\(/);
 assert.doesNotMatch(bridge,/RegisterSink\(|SetActive\s*\(/);
 assert.doesNotMatch(bridge,/std::thread|CreateThread|GetAsyncKeyState/);
});
test('UI payload is parsed before task ownership transfer',()=>{
 assert.match(bridge,/strnlen_s\(borrowedUiPayload, 11\)/);
 assert.match(bridge,/QuestTracking::ParseRequest/);
 assert.match(bridge,/return QueueDryRun\(gate, request->formId, request->desiredTracked/);
 assert.match(intent,/const auto ticket = gate->Offer/);
 assert.match(intent,/gate->Current\(request\)/g);
 assert.match(intent,/const auto current=observe\(request\)/);
 assert.match(intent,/if \(gate->Settle\(request\)\) result\(decision\)/);
});
test('task re-reads authoritative engine state, no native writer or assumed success',()=>{
 assert.match(bridge,/GetFormByID<RE::TESQuest>\(intent.formId\)/);
 assert.match(bridge,/questData\.questArray->elements/);
 assert.match(bridge,/PipboyPrimitiveValue<bool>/);
 assert.match(bridge,/feedHasTrackedBit=true/);
 assert.match(bridge,/context\.runtimeOg163Verified/);
 assert.match(bridge,/callDestination\(0x2Fu\)/);
 assert.match(bridge,/callDestination\(0x3Cu\)/);
 assert.match(intent,/native != o.feedTracked/);
 assert.match(intent,/Result::kUnverified/);
 assert.match(intent,/Result::kWouldWrite/);
 assert.match(intent,/Verification::kConfirmed/);
 assert.doesNotMatch(bridge,/REL::ID\(1567562\)/);
 assert.doesNotMatch(bridge,/WriteProcessMemory\s*\(|DispatchMethodCall|VirtualProtect\s*\(/);
 assert.doesNotMatch(bridge,/quest->data\.flags\s*(?:=|\|=|&=)/);
});
test('shipping UI and native backend are still default deny',()=>{
 assert.match(pipeline,/interop\.BindUIEvent\(view,\s*"requestPipboyTracking"/);
 assert.match(pipeline,/QueueUIString\(UpdatePipboyTracking,request\)/);
 assert.match(ui,/<button[^>]+data-action="pipboy-track" disabled/);
 assert.match(ui,/if\(!state\.qaIntegratedAvailable \|\| !state\.qaIntegratedConsent/);
 assert.match(ui,/qaIntegratedAvailable:false/);
 assert.match(ui,/window\.enableIntegratedTrackingQA/);
 assert.match(policy,/kVerifiedBackend\s*=\s*WriteBackend::kNone/);
 const qa = build.match(/if has_config\("quest_dryrun_qa"\) then([\s\S]*?set_targetdir\([^\n]+\))\s+end/);
 assert.ok(qa, 'QA file inclusion must be gated');
 assert.match(qa[1], /add_defines\("K2040_TRACKING_DRYRUN_QA=1"\)/);
 assert.match(qa[1], /add_files\("research\/native_one_shot\/OneShotOgDryRun\.cpp"\)/);
 // The distinct normal-use candidate explicitly opts in to the same
 // read-only evidence source; neither variant joins the default build.
 const candidate=build.match(/if has_config\("quest_questwide_production_candidate"\) or\s+has_config\("quest_questwide_normal_tracking"\) then([\s\S]*?set_targetdir\([^\n]+\))\s+end/);
 assert.ok(candidate, 'the approved R2 module must remain behind two explicit off-by-default build flavors');
 assert.match(candidate[1], /add_files\("research\/native_one_shot\/OneShotOgDryRun\.cpp"\)/);
 assert.doesNotMatch(build.replace(qa[0], '').replace(candidate[0], ''), /add_files\("research\/native_one_shot/);
 assert.match(pipeline,/#ifdef K2040_TRACKING_DRYRUN_QA/);
 assert.match(pipeline,/QueueFromExplicitPrismaClick/);
});
