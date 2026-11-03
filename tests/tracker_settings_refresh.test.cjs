// Regression: HUD quest limit must refresh without a new quest snapshot.
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
  const rootStyle={setProperty(){}};
  const document={body:{className:'tracker-page',classList:{contains:s=>s==='tracker-page'}},
    documentElement:{style:rootStyle},getElementById:node,querySelector:()=>null,
    querySelectorAll:()=>[],addEventListener(){},activeElement:null};
  let nativeActions=0;
  const window={location:{search:''},K2040JournalSettings:Settings,
    K2040QuestModel:Model,addEventListener(){},updateJournalSetting(){nativeActions++}};
  vm.runInNewContext(fs.readFileSync('view/app.js','utf8'),{
    window,document,URLSearchParams,console,setTimeout(){},setInterval(){}
  });
  return {window,node,nativeActions:()=>nativeActions};
}
function fixture(){
  return JSON.stringify({ready:true,quests:Array.from({length:8},(_,index)=>({
    id:(0x10000+index).toString(16).toUpperCase().padStart(8,'0'),
    name:'Quest '+(index+1),state:'active',pinned:index<7,
    vanillaTracked:index===7,
    objectives:[{index:1,state:'active',text:'Goal '+(index+1)},
                {index:2,state:'completed',text:'Done '+(index+1)}]
  }))});
}
function count(node){return (node.innerHTML.match(/class="tracked-entry"/g)||[]).length}
test('settings change updates stale six-entry HUD without another quest snapshot',()=>{
  const t=tracker(),raw=fixture();
  t.window.receiveQuestSnapshot(raw);
  assert.equal(t.node('trackerCount').textContent,7);
  assert.equal(count(t.node('trackedQuests')),6);
  assert.match(t.node('trackedQuests').innerHTML,/\+1 more pinned quest/);
  t.window.receiveJournalSettings(JSON.stringify({trackerMaxQuests:12}));
  assert.equal(count(t.node('trackedQuests')),7);
  assert.doesNotMatch(t.node('trackedQuests').innerHTML,/more pinned quest/);
  // A polling cycle with identical JSON returns early but must not undo it.
  t.window.receiveQuestSnapshot(raw);
  assert.equal(count(t.node('trackedQuests')),7);
  assert.equal(t.nativeActions(),0);
});
test('tracker limit, Pip-Boy mirror and completed objective filters redraw immediately',()=>{
  const t=tracker();t.window.receiveQuestSnapshot(fixture());
  t.window.receiveJournalSettings({trackerMaxQuests:12,trackerSyncPipboy:true,trackerShowCompleted:true});
  assert.equal(t.node('trackerCount').textContent,8);
  assert.equal(count(t.node('trackedQuests')),8);
  assert.match(t.node('trackedQuests').innerHTML,/Done 1/);
  t.window.receiveJournalSettings({trackerMaxQuests:2,trackerSyncPipboy:true,trackerShowCompleted:false});
  assert.equal(count(t.node('trackedQuests')),2);
  assert.match(t.node('trackedQuests').innerHTML,/\+6 more pinned quests/);
  assert.doesNotMatch(t.node('trackedQuests').innerHTML,/Done 1/);
  assert.equal(t.nativeActions(),0);
});
