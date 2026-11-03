const test=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const M=require('../view/quest-model.js');
const ui=fs.readFileSync('view/app.js','utf8');
test('opt-in candidate does not change the shipping-side readiness-only model',()=>{
 const q=M.normalizeQuest({id:'0006DFAF',isMisc:true,state:'active',
   trackingReadiness:'candidate-read-only',nativeInstanceID:0,
   nativeInstanceStatus:'consistent',pipboyFeedAgreement:'match',miscRowCount:1,
   trackingScope:'quest-wide'});
 assert.equal(M.nativeTrackEligibility(q).allowed,false);
 assert.equal(q.nativeInstanceID,0);
 assert.match(ui,/state\.candidateAvailable/);
 assert.match(ui,/selectedQuestWideCandidate/);
 assert.match(ui,/receiveQuestWideCandidateResult/);
});
test('candidate has an explicit pending acknowledgement and fail-closed result',()=>{
 assert.match(ui,/state\.candidatePending=\{id,desired\}/);
 assert.match(ui,/state\.candidateStatus='blocked'/);
 assert.match(ui,/q\.trackingReadiness!=='candidate-read-only'/);
});

test('ordinary and Misc candidate actions share one request path, but different readiness gates',()=>{
 assert.match(ui,/const questWideCandidateEligible=/);
 assert.match(ui,/selected\.nativeDisplayedCount>0/);
 assert.match(ui,/\(!selected\.isMisc \|\| selected\.miscRowCount>0\)/);
 assert.match(ui,/\(q\.isMisc && q\.miscRowCount<=0\)/);
 assert.match(ui,/state\.candidateAvailable[\s\S]*questWideCandidateEligible/);
});
