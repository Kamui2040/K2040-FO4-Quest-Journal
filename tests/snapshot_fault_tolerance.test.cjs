'use strict';
const test=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const vm=require('node:vm');
const Model=require('../view/quest-model.js');
const Settings=require('../view/journal-settings.js');

function tracker(){
 const nodes=new Map();
 const node=id=>{
  if(!nodes.has(id))nodes.set(id,{textContent:'',innerHTML:'',style:{},
    classList:{contains:()=>false,toggle(){},remove(){},add(){}}});
  return nodes.get(id);
 };
 const document={body:{className:'tracker-page',classList:{contains:name=>name==='tracker-page'}},
  documentElement:{style:{setProperty(){}}},getElementById:node,
  querySelector:()=>null,querySelectorAll:()=>[],addEventListener(){},activeElement:null};
 const window={location:{search:''},K2040JournalSettings:Settings,K2040QuestModel:Model,
  addEventListener(){}};
 vm.runInNewContext(fs.readFileSync('view/app.js','utf8'),{
  window,document,URLSearchParams,console,setTimeout(){},setInterval(){}
 });
 return {window,node};
}
const initial=JSON.stringify({ready:true,quests:[{
 id:'00012345',name:'Keep Me Visible',state:'active',pinned:true,
 objectives:[{index:1,state:'active',text:'Important Objective'}]
}]});
test('invalid quest JSON cannot erase the last verified snapshot',()=>{
 const t=tracker();
 t.window.receiveQuestSnapshot(initial);
 assert.match(t.node('trackedQuests').innerHTML,/Keep Me Visible/);
 t.window.receiveQuestSnapshot('{broken-json');
 assert.match(t.node('trackedQuests').innerHTML,/Keep Me Visible/);
 t.window.receiveQuestSnapshot(initial);
 assert.match(t.node('trackedQuests').innerHTML,/Important Objective/);
});
test('circular, absent and unstringifiable inputs never replace valid quest data',()=>{
 const t=tracker();
 t.window.receiveQuestSnapshot(initial);
 const circular={ready:true};circular.self=circular;
 for(const bad of [circular,undefined,()=>{}]){
  assert.doesNotThrow(()=>t.window.receiveQuestSnapshot(bad));
  assert.match(t.node('trackedQuests').innerHTML,/Keep Me Visible/);
 }
});
test('explicit valid not-ready snapshot still clears old save data',()=>{
 const t=tracker();
 t.window.receiveQuestSnapshot(initial);
 t.window.receiveQuestSnapshot(JSON.stringify({ready:false,quests:[]}));
 assert.doesNotMatch(t.node('trackedQuests').innerHTML,/Keep Me Visible/);
 assert.match(t.node('trackedQuests').innerHTML,/Waiting for quest data/);
});

test('well-formed but invalid quest envelopes cannot blank tracked quests',()=>{
 const t=tracker();
 t.window.receiveQuestSnapshot(initial);
 for(const malformed of ['[]','{}','{"quests":[]}',
    '{"ready":true,"quests":"not-an-array"}','null','42']){
  t.window.receiveQuestSnapshot(malformed);
  assert.match(t.node('trackedQuests').innerHTML,/Keep Me Visible/,malformed);
 }
 t.window.receiveQuestSnapshot(JSON.stringify({ready:false,quests:[]}));
 assert.doesNotMatch(t.node('trackedQuests').innerHTML,/Keep Me Visible/);
});
