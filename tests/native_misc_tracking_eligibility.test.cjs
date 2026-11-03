'use strict';
const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const M=require('../view/quest-model.js');
const app=fs.readFileSync('view/app.js','utf8');
const collector=fs.readFileSync('src/QuestCollector.cpp','utf8');

function quest(extra={}) {
  return M.normalizeQuest({
    id:'12345678',name:'QA objective',sourceType:'Main',type:'Main',
    state:'active',isMisc:false,historyOnly:false,...extra
  });
}

test('Misc classification comes from engine/native context, not UI grouping',()=>{
  assert.deepEqual(M.nativeTrackEligibility(quest({isMisc:true,sourceType:'Main',type:'Main'})),
    {allowed:false,reason:'misc-objective'});
  assert.deepEqual(M.nativeTrackEligibility(quest({isMisc:false,sourceType:'Misc',type:'Misc'})),
    {allowed:false,reason:'misc-objective'});
  assert.deepEqual(M.nativeTrackEligibility(quest({isMisc:true,sourceType:'Misc',type:'Side',categoryOverride:true})),
    {allowed:false,reason:'misc-objective'});
  assert.deepEqual(M.nativeTrackEligibility(quest({sourceType:'Main',type:'Misc',categoryOverride:true})),
    {allowed:true,reason:'eligible'});
});

test('inactive/history-only are never eligible for native tracking',()=>{
  for(const state of ['completed','failed']) {
    assert.equal(M.nativeTrackEligibility(quest({state})).allowed,false);
  }
  assert.equal(M.nativeTrackEligibility(quest({historyOnly:true})).allowed,false);
  assert.equal(M.nativeTrackEligibility(null).allowed,false);
});

test('native type-6 top-level rows are marked Misc by the collector',()=>{
  assert.match(collector,/item\.isMisc = misc \|\|[\s\S]{0,100}questType\)\s*==\s*6u/);
});

test('QA tracking button and actual click handler use the SAME eligibility rule',()=>{
  assert.match(app,/const nativeTracking = M\.nativeTrackEligibility\(selected\)/);
  assert.match(app,/nativeTracking\.allowed/);
  assert.match(app,/const eligibility=M\.nativeTrackEligibility\(quest\)/);
  assert.match(app,/Misc entries share a quest-wide vanilla tracking bit/);
  assert.match(app,/data-action="pipboy-track" disabled/);
  assert.doesNotMatch(app,/if\(!quest \|\| quest\.isMisc \|\|/);
});

test('engine-level Misc write restriction remains active',()=>{
  const guard=fs.readFileSync('research/native_one_shot/OneShotIntent.h','utf8');
  assert.match(guard,/o\.questType == kMisc/);
  const production=fs.readFileSync('src/QuestTrackingPolicy.h','utf8');
  assert.match(production,/kVerifiedBackend\s*=\s*WriteBackend::kNone/);
});

test('shipping UI does not confuse native read-only eligibility with write permission',()=>{
 assert.match(app,/Read-only native preflight:/);
 assert.match(app,/A valid preflight does not enable the shipping tracking writer/);
 assert.match(app,/Pip-Boy tracking: disabled in this build/);
 assert.match(app,/data-action="pipboy-track" disabled/);
});
