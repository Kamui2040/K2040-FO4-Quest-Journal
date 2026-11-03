const test=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const vm=require('node:vm');
const source=fs.readFileSync('view/app.js','utf8');
function section(first,last){
  const start=source.indexOf(first),end=source.indexOf(last,start+first.length);
  assert.ok(start>=0 && end>start,first);
  return source.slice(start,end);
}
function ui(){
  const ctx={isTracker:false,state:{candidateAvailable:false,
    candidateStatus:'off',candidatePending:null,qaLastSelected:'old',
    trackingNotice:'',selected:'AAAA0002'},render(){this.rendered=(this.rendered||0)+1;},window:{}};
  vm.createContext(ctx);
  vm.runInContext(
    section('window.enableQuestWideCandidate=function','window.enableRepeatableMiscTrackingQA=function'),ctx);
  return ctx;
}
test('verified refusal never locks unrelated quests',()=>{
  const c=ui();c.window.enableQuestWideCandidate('ready');
  c.state.candidatePending={id:'00001001',desired:true};
  c.state.candidateStatus='pending';
  c.window.receiveQuestWideCandidateResult('00001001:refused');
  assert.equal(c.state.candidateStatus,'ready');
  assert.equal(c.state.candidatePending,null);
  assert.match(c.state.trackingNotice,/before a game-state change/);
});
test('changed highlighted quest cannot corrupt original readback status',()=>{
  const c=ui();c.window.enableQuestWideCandidate('ready');
  c.state.candidatePending={id:'00001001',desired:true};
  c.state.candidateStatus='pending';c.state.selected='00002002';
  c.window.receiveQuestWideCandidateResult('00001001:pending');
  assert.equal(c.state.candidateStatus,'pending');
  c.window.receiveQuestWideCandidateResult('00001001:confirmed');
  assert.equal(c.state.candidateStatus,'ready');
  assert.equal(c.state.candidatePending,null);
  assert.equal(c.state.selected,'00002002');
});
test('panel reopen can resume readback without issuing a fresh write',()=>{
  const c=ui();c.window.enableQuestWideCandidate('pending:0000ABCD:0');
  assert.equal(c.state.candidateStatus,'pending');
  assert.equal(c.state.candidatePending.id,'0000ABCD');
  assert.equal(c.state.candidatePending.desired,false);
  c.window.receiveQuestWideCandidateResult('0000ABCD:confirmed');
  assert.equal(c.state.candidateStatus,'ready');
  c.window.enableQuestWideCandidate('pending:0000ABCD:1');
  assert.equal(c.state.candidatePending.desired,true);
  c.window.enableQuestWideCandidate('ready');
  assert.equal(c.state.candidatePending,null);
});
test('uncertain outcome remains blocked and mismatched callbacks are ignored',()=>{
  const c=ui();c.window.enableQuestWideCandidate('ready');
  c.state.candidatePending={id:'00001001',desired:true};
  c.window.receiveQuestWideCandidateResult('00002222:refused');
  assert.equal(c.state.candidatePending.id,'00001001');
  c.window.receiveQuestWideCandidateResult('00001001:blocked');
  assert.equal(c.state.candidateStatus,'blocked');
  c.window.enableQuestWideCandidate('bogus');
  assert.equal(c.state.candidateStatus,'blocked');
});
test('only pending candidate triggers fast read-only polling; ordinary polling remains',()=>{
  assert.match(source,/state\.candidateStatus==='pending' &&[\s\S]{0,120}state\.candidatePending\)emit\('requestQuestSnapshot'\)/);
  assert.match(source,/\},1000\);/);
  assert.match(source,/\},isTracker\?8000:12000\);/);
  assert.match(source,/state\.candidateStatus!=='ready'/);
});
