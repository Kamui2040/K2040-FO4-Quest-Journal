'use strict';
const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const main=fs.readFileSync('src/main.cpp','utf8');
const worker=fs.readFileSync('research/native_one_shot/OneShotOgIntegratedQA.cpp','utf8');
const scripts=['scripts/build-linux.sh','scripts/build-one-shot-qa.sh','scripts/build-native-write-once-qa.sh',
               'scripts/build-integrated-tracking-qa.sh'];

test('integrated QA startup says native SetActive is available only after in-game consent',()=>{
 const startup=main.split('constexpr const char* qaLogName="K2040_Quest_Journal_WriteOnce_QA"')[1];
 assert.match(startup,/#elif defined\(K2040_TRACKING_INTEGRATED_QA\)/);
 assert.match(startup,/IntegratedTrackingQA: START session log=/);
 assert.match(startup,/writer=QA-only one-shot AddTask consent=DISARMED/);
 assert.match(startup,/#else\s+REX::INFO\("JournalDryRunQA: isolated preview build loaded \(NO SetActive\)"\)/);
 assert.match(main,/K2040_TRACKING_INTEGRATED_QA=1|K2040_TRACKING_INTEGRATED_QA/);
});
test('log name unique per game process and launch, not overwritten by next launch',()=>{
 assert.match(main,/static char qaSessionLogName\[128\]/);
 assert.match(main,/GetSystemTime\(&utc\)/);
 assert.match(main,/GetCurrentProcessId\(\)/);
 assert.match(main,/K2040_Quest_Journal_IntegratedTracking_QA_%04u%02u%02u_%02u%02u%02u_%lu/);
 assert.match(main,/const char\* qaLogName=qaSessionLogName/);
});
test('native operation diagnostics form a correlated, bounded, privacy-preserving trace',()=>{
 assert.match(worker,/REQUEST seq=\{\} desired=\{\} reserved=\{\}/);
 assert.match(worker,/TASK seq=\{\} started desired=\{\}/);
 assert.match(worker,/TASK seq=\{\} preflight=\{\}/);
 assert.match(worker,/TASK seq=\{\} SetActive ENTER/);
 assert.match(worker,/TASK seq=\{\} SetActive RETURNED/);
 assert.match(worker,/TASK seq=\{\} END result=\{\}/);
 assert.match(worker,/canceled before read/);
 assert.match(worker,/canceled after context/);
 assert.match(worker,/reason=stale-or-pending/);
 assert.match(worker,/reason=budget-exhausted/);
 assert.match(main,/IntegratedTrackingQA: UI accepted and task submitted/);
 assert.match(main,/IntegratedTrackingQA: CONSENT enabled/);
 assert.match(main,/IntegratedTrackingQA: CONSENT disabled/);
 assert.match(main,/IntegratedTrackingQA: CONSENT rejected/);
 assert.match(worker,/FormID omitted/);
 assert.match(main,/form omitted/);
});
test('new instrumentation does not introduce unsupported or automatic writing',()=>{
 assert.match(worker,/ReserveIntegratedAttempt\(attempts\)/);
 assert.match(worker,/task->AddTask\(/);
 assert.match(worker,/ReviewOnGameTask\(requested,before\)/);
 assert.match(worker,/setter\(quest,requested.desiredTracked\)/);
 assert.doesNotMatch(worker,/AddTaskPermanent\s*\(|Papyrus|WriteProcessMemory\s*\(/);
 assert.match(main,/kIntegratedMaxAttempts/);
 assert.match(fs.readFileSync('src/QuestTrackingPolicy.h','utf8'),/kVerifiedBackend\s*=\s*WriteBackend::kNone/);
 for(const name of scripts.slice(0,3))assert.match(fs.readFileSync(name,'utf8'),/--quest_integrated_tracking_qa=n/);
 assert.match(fs.readFileSync(scripts[3],'utf8'),/--quest_integrated_tracking_qa=y/);
});
