'use strict';
const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const files={
  build:fs.readFileSync('xmake.lua','utf8'),
  script:fs.readFileSync('scripts/build-dogmeat-production-candidate.sh','utf8'),
  native:fs.readFileSync('src/DogmeatProduction.cpp','utf8'),
  main:fs.readFileSync('src/main.cpp','utf8'),
  ui:fs.readFileSync('view/app.js','utf8'),
  gate:fs.readFileSync('research/native_one_shot/SingleMiscWriteGate.h','utf8'),
  policy:fs.readFileSync('src/QuestTrackingPolicy.h','utf8')
};
test('production candidate is opt-in, excludes all QA, stages outside game',()=>{
 assert.match(files.build,/option\("quest_dogmeat_production_candidate"\)\s+set_default\(false\)/);
 assert.match(files.build,/raise\("Dogmeat production candidate may not compile any QA/);
 assert.match(files.build,/K2040_DOGMEAT_PRODUCTION_CANDIDATE=1/);
 assert.match(files.script,/--quest_dryrun_qa=n/);
 assert.match(files.script,/--quest_native_write_qa=n/);
 assert.match(files.script,/--quest_integrated_tracking_qa=n/);
 assert.match(files.script,/--quest_single_misc_write_qa=n/);
 assert.match(files.script,/--quest_single_misc_rc=n/);
 assert.match(files.script,/--quest_dogmeat_production_candidate=y/);
 assert.match(files.policy,/kVerifiedBackend = WriteBackend::kNone/);
});
test('engine call requires exact quest, fresh preflight and verified OG executable',()=>{
 assert.match(files.native,/formID!=SingleMiscQA::kVerifiedQuestFormID/);
 assert.match(files.native,/InspectSingleMiscWriteCandidate\(formID\)/g);
 assert.match(files.native,/SingleMiscQA::Review\(again,desiredTracked\)/);
 assert.match(files.native,/MiscVanillaResearch::CaptureRuntimeCode\(\)/);
 assert.match(files.native,/image->entries\[3\]\.rva!=0x009FB850u/);
 assert.match(files.native,/setter\.address\(\)!=base\+0x005D6AC0u/);
 assert.match(files.native,/toggle\(formID,SingleMiscQA::kObservedInstance,6u\)/);
 assert.match(files.gate,/o\.formID!=kVerifiedQuestFormID/);
});
test('native request uses one task, UI selection and lifecycle hard gates',()=>{
 const callback=files.main.split('void QueueDogmeatProduction(const char* raw) {')[1]
   .split('tasks->AddTask(')[0];
 assert.doesNotMatch(callback,/Valid\(journalView\)|journalOpen|gameLoaded/);
 assert.match(callback,/dogmeatRuntimeOG163\.load\(std::memory_order_acquire\)/);
 assert.match(files.main,/dogmeatProcessAttempted\.exchange\(true,std::memory_order_acq_rel\)/);
 assert.match(files.main,/dogmeatSaveEpoch.IsCurrent\(save\)/);
 assert.match(files.main,/uiEpoch.IsCurrent\(view\)/);
 assert.match(files.main,/dogmeatSelectedFormID\.load\(std::memory_order_acquire\)==id/);
 assert.match(files.main,/dogmeatRuntimeOG163\.load\(std::memory_order_acquire\)/);
 assert.match(files.main,/requestDogmeatTracking/);
 assert.match(files.main,/enableDogmeatTrackingProduction/);
 assert.match(files.main,/selectedDogmeatQuest/);
 assert.match(files.ui,/window.enableDogmeatTrackingProduction/);
 assert.match(files.ui,/window.receiveDogmeatTrackingResult/);
 assert.match(files.ui,/state.dogmeatProdAvailable\?'requestDogmeatTracking'/);
 assert.match(files.ui,/state.singleRcUsed=true; \/\/ fail closed immediately/);
});
