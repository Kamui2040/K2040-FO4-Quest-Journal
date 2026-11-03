'use strict';
const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const main=fs.readFileSync('src/main.cpp','utf8');
const collector=fs.readFileSync('src/QuestCollector.cpp','utf8');
const build=fs.readFileSync('xmake.lua','utf8');
const script=fs.readFileSync('scripts/build-integrated-tracking-qa.sh','utf8');
const ui=fs.readFileSync('view/app.js','utf8');
const policy=fs.readFileSync('src/QuestTrackingPolicy.h','utf8');
const misc=fs.readFileSync('research/native_one_shot/MiscVanillaSelectionPreflight.h','utf8');
test('single-Misc experimental write is an opt-in, separate QA flavor only',()=>{
 assert.match(build,/option\("quest_single_misc_write_qa"\)/);
 assert.match(build,/set_default\(false\)/);
 assert.match(build,/has_config\("quest_single_misc_write_qa"\)/);
 assert.match(build,/K2040_SINGLE_MISC_WRITE_QA=1/);
 assert.match(script,/--quest_single_misc_write_qa=/);
 assert.match(main,/#ifdef K2040_SINGLE_MISC_WRITE_QA[\s\S]*singleMiscArmedFormId/);
 assert.match(policy,/kVerifiedBackend = WriteBackend::kNone/);
 assert.match(misc,/kNativeMiscWritesAuthorized = false/);
});
test('one-use native command guarded by fresh task/selection/instance and real dispatch',()=>{
 assert.match(main,/singleMiscAttempted.exchange\(true,std::memory_order_acq_rel\)/);
 assert.match(main,/singleMiscArmedFormId.store\(0,std::memory_order_release\)/);
 assert.match(main,/trackingSaveEpoch.IsCurrent\(save\)/);
 assert.match(main,/uiEpoch.IsCurrent\(view\)/);
 assert.match(main,/CaptureRuntimeCode\(\)/);
 assert.match(main,/image->linkedHelper/);
 assert.match(main,/setter.address\(\)==base\+0x005D6AC0u/);
 assert.match(main,/InspectSingleMiscWriteCandidate\(id\)/);
 assert.match(main,/SingleMiscQA::Review\(again,desired\)/);
 assert.match(main,/toggle\(id,K2040::SingleMiscQA::kObservedInstance,6u\)/);
 assert.doesNotMatch(main,/AddTaskPermanent/);
 assert.match(collector,/RE::BSAutoLock lock\{player->questTargetLock\}/);
 assert.match(collector,/nativeCount>1u/);
 assert.match(collector,/nested>1u/);
 assert.match(collector,/questDisplayed!=1u/);
});
test('ARM and EXECUTE exposed only by isolated DLL; no INI; user controls one attempt',()=>{
 assert.match(ui,/window.enableMiscSingleWriteQA/);
 assert.match(ui,/data-action="qa-single-arm"/);
 assert.match(ui,/data-action="qa-single-execute"/);
 assert.match(ui,/emit\('armMiscSingleWrite',id\)/);
 assert.match(ui,/emit\('requestMiscSingleExecute'/);
 assert.match(ui,/state.qaSingleUsed=true/);
 assert.match(main,/InteropCall\(view,"enableMiscSingleWriteQA",\s*spent\?"spent":"ready"\)/);
 assert.match(ui,/state.qaSingleUsed=String\(value\)==='spent'/);
 assert.match(ui,/ARM did not receive a response/);
 assert.match(ui,/No other QA toggle is needed/);
 // Regression: unrelated generic consent must not disable ARM/EXECUTE.
 const armNative=main.split('void ArmSingleMiscWrite(')[1].split('void QueueSingleMiscWrite(')[0];
 const executeNative=main.split('void QueueSingleMiscWrite(')[1].split('void QueueMiscTrackingProbe(')[0];
 assert.doesNotMatch(armNative,/integratedQaEnabled\.load/);
 assert.doesNotMatch(executeNative,/integratedQaEnabled\.load/);
 const armUi=ui.split("if(target.dataset.action==='qa-single-arm')")[1]
   .split("if(target.dataset.action==='qa-single-execute')")[0];
 const executeUi=ui.split("if(target.dataset.action==='qa-single-execute')")[1]
   .split("if(target.dataset.action==='qa-misc-inspect')")[0];
 assert.doesNotMatch(armUi,/qaIntegratedConsent/);
 assert.doesNotMatch(executeUi,/qaIntegratedConsent/);
 assert.match(main,/QueueUIString\(ArmSingleMiscWrite,request\)/);
 assert.match(main,/receiveMiscSingleResult/);
 assert.match(script,/K2040_Quest_Journal_Write_QA.ini/);
});
