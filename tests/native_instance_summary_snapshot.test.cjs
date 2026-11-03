const test=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const M=require('../view/quest-model.js');
const native=fs.readFileSync('src/QuestCollector.cpp','utf8');
test('native objective instance summary preserved through normal model',()=>{
 const snapshot=M.normalizeSnapshot({ready:true,quests:[{id:'FE0021AA',state:'active',isMisc:true,sourceType:'Misc',trackingScope:'quest-wide',miscRowCount:2,nativeDisplayedCount:3,nativeInstanceID:0,nativeInstanceStatus:'consistent',pipboyFeedAgreement:'match',objectives:[{index:0,state:'active',text:'One'},{index:7,state:'active',text:'One'}]}]});
 const q=snapshot.quests[0];
 assert.equal(q.nativeInstanceID,0);
 assert.equal(q.nativeInstanceStatus,'consistent');
 assert.equal(q.trackingScope,'quest-wide');
 assert.equal(q.miscRowCount,2);
 assert.equal(q.nativeDisplayedCount,3);
 assert.equal(q.objectives.length,2); // duplicate text is valid identity-wise
 assert.equal(M.nativeTrackEligibility(q).allowed,false); // shipping writer still disabled
});
test('untrusted or unavailable native instance identity fails closed',()=>{
 const q=M.normalizeQuest({id:'FF001234',isMisc:true,nativeInstanceStatus:'multiple-instances',nativeInstanceID:-2,pipboyFeedAgreement:'made-up',miscRowCount:99999});
 assert.equal(q.nativeInstanceStatus,'multiple-instances');
 assert.equal(q.nativeInstanceID,null);
 assert.equal(q.pipboyFeedAgreement,'missing');
 assert.equal(q.miscRowCount,512);
 assert.equal(M.nativeTrackEligibility(q).allowed,false);
});
test('native reader uses the real player quest lock, bounded evidence, no flag write',()=>{
 assert.match(native,/RE::BSAutoLock guard\{player->questTargetLock\}/);
 assert.match(native,/GenericQuestInstance::kMaxObservations/);
 assert.match(native,/native\.Resolve\(item\.formId,form->currentInstanceID\)/);
 assert.match(native,/nativeInstanceStatus/);
 assert.doesNotMatch(native,/SetActive\(/);
});

test('tracking readiness is strictly normalized and never changes shipping writer gate',()=>{
 const candidate=M.normalizeQuest({id:'0006DFAF',state:'active',isMisc:true,
   trackingReadiness:'candidate-read-only'});
 assert.equal(candidate.trackingReadiness,'candidate-read-only');
 assert.equal(M.nativeTrackEligibility(candidate).allowed,false);
 for(const invalid of ['engine-ready','enabled',null,1,'write-allowed']){
   const q=M.normalizeQuest({id:'0006DFAF',trackingReadiness:invalid});
   assert.equal(q.trackingReadiness,'unknown-scope');
 }
});
