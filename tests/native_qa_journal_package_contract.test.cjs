'use strict';
const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const main=fs.readFileSync('src/main.cpp','utf8');
const web=fs.readFileSync('view/app.js','utf8');
const xmake=fs.readFileSync('xmake.lua','utf8');
const build=fs.readFileSync('scripts/build-one-shot-qa.sh','utf8');
const dry=fs.readFileSync('research/native_one_shot/OneShotOgDryRun.cpp','utf8');
const policy=fs.readFileSync('src/QuestTrackingPolicy.h','utf8');

test('regular production stays disabled, QA sources behind compile-time gate',()=>{
 assert.match(xmake,/option\("quest_dryrun_qa"\)/);
 assert.match(xmake,/set_default\(false\)/);
 assert.match(xmake,/if has_config\("quest_dryrun_qa"\) then/);
 assert.match(xmake,/add_defines\("K2040_TRACKING_DRYRUN_QA=1"\)/);
 assert.match(xmake,/add_files\("research\/native_one_shot\/OneShotOgDryRun\.cpp"\)/);
 assert.match(main,/#ifdef K2040_TRACKING_DRYRUN_QA/);
 assert.match(main,/constexpr const char\* qaLogName="K2040_Quest_Journal_DryRun_QA"/);
 assert.match(fs.readFileSync('scripts/build-linux.sh','utf8'),/--quest_dryrun_qa=n/);
 assert.match(main,/static_assert\(!K2040::QuestTracking::CanWritePipboy\(\)/);
 assert.match(policy,/kVerifiedBackend\s*=\s*WriteBackend::kNone/);
 assert.match(web,/<button[^>]+data-action="pipboy-track" disabled/);
 assert.match(web,/if\(!state\.qaIntegratedAvailable \|\| !state\.qaIntegratedConsent/);
 assert.match(web,/qaIntegratedAvailable:false/);
 assert.match(web,/window\.enableIntegratedTrackingQA/);
});

test('QA-specific preview does not double enqueue or invoke native writes',()=>{
 const handler=main.split('void QueueDryRunPreview(')[1].split('#endif')[0];
 assert.match(handler,/QueueFromExplicitPrismaClick/);
 assert.doesNotMatch(handler,/QueueUIString\(|QueueUIAction\(|AddTaskPermanent\s*\(/);
 assert.match(main,/interop\.BindUIEvent\(view, "requestPipboyTrackingPreview"/);
 assert.match(main,/interop\.BindUIEvent\(view, "qaSelectedQuest"/);
 assert.match(main,/InteropCall\(view,"enableTrackingDryRunQA","true"\)/);
 assert.match(dry,/tasks->AddTask\(std::move\(oneTask\)\)/);
 assert.doesNotMatch(dry,/SetActive\s*\(/);
});

test('QA browser feature is default-hidden, test-click only and never claims tracking success',()=>{
 assert.match(web,/qaPreviewEnabled:false/);
 assert.match(web,/window\.enableTrackingDryRunQA=function/);
 assert.match(web,/data-action="pipboy-dryrun"/);
 assert.match(web,/if\(!state\.qaPreviewEnabled/);
 assert.match(web,/emit\('requestPipboyTrackingPreview'/);
 assert.match(web,/'qaSelectedQuest'/);
 assert.match(web,/state.dogmeatProdAvailable\?'selectedDogmeatQuest':/);
 assert.match(web,/state.candidateAvailable\?'selectedQuestWideCandidate':'qaSelectedQuest'/);
 assert.match(web,/DRY RUN: eligible for a change; no tracking was modified/);
 assert.match(web,/DRY RUN: no response from the game task; no tracking change was requested/);
 assert.doesNotMatch(web,/emit\('requestPipboyTracking',/);
});

test('package contains only isolated QA flavor of normal Journal, not second simultaneous DLL',()=>{
 assert.match(build,/K2040_DRYRUN_QA_STAGE/);
 assert.match(build,/--quest_dryrun_qa=y/);
 assert.match(build,/K2040_Quest_Journal\.dll/);
 assert.match(build,/No game or Amethyst files deployed/);
 assert.match(build,/\*Amethyst\*/);
 assert.match(xmake,/set_targetdir\(path\.join\(qa_stage,"bin"\)\)/);
 assert.match(xmake,/local base = has_config\("quest_dryrun_qa"\)/);
});
