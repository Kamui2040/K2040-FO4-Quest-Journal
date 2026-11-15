'use strict';
const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const vm=require('node:vm');
const Model=require('../view/quest-model.js');
const Settings=require('../view/journal-settings.js');
function tracker(){
  const nodes=new Map(),reports=[];
  const get=id=>{
    if(!nodes.has(id))nodes.set(id,{
      style:{},textContent:'',innerHTML:'',
      classList:{contains:()=>false,toggle(){}},
      getBoundingClientRect:()=>({left:1380,top:54,width:340,height:155})
    });
    return nodes.get(id);
  };
  const rootStyle={setProperty(){},removeProperty(){}};
  const window={location:{search:''},innerWidth:1920,innerHeight:1080,
    K2040JournalSettings:Settings,K2040QuestModel:Model,
    addEventListener(){},reportTrackerLayoutMetrics:raw=>reports.push(JSON.parse(raw))};
  const document={
    body:{className:'tracker-page',classList:{contains:x=>x==='tracker-page',toggle(){}}},
    documentElement:{style:rootStyle,clientWidth:1920,clientHeight:1080},
    getElementById:get,querySelector:()=>null,querySelectorAll:()=>[],
    addEventListener(){},activeElement:null
  };
  vm.runInNewContext(fs.readFileSync('view/app.js','utf8'),
    {window,document,URLSearchParams,console,setTimeout(){},setInterval(){}});
  return {window,get,reports};
}
test('owner V2 preview is live and reversible while legacy HUD retains its anchors',()=>{
  const t=tracker(),s=t.get('tracker').style;
  t.window.receiveJournalSettings({trackerAnchor:'bottom-left',trackerWidth:340});
  assert.equal(s.bottom,'5%');
  assert.equal(s.left,'3%');
  assert.equal(s.width,'');
  t.window.applyJournalTrackerLayout({x:0.35,y:0.15,width:0.25,height:0.2});
  assert.equal(s.left,'35.0000vw');
  assert.equal(s.top,'15.0000vh');
  assert.equal(s.width,'25.0000vw');
  assert.equal(s.height,'20.0000vh');
  assert.ok(Number.parseFloat(s.fontSize)>14);
  t.window.applyJournalTrackerLayout({x:0.95,y:0.85,width:0.2,height:0.2});
  assert.equal(s.left,'35.0000vw');
  t.window.applyJournalTrackerLayout('native');
  assert.equal(s.width,'');
  assert.equal(s.bottom,'5%');
  assert.equal(s.height,'');
});
test('saved owner geometry survives browser settings refresh and clears on legacy width change',()=>{
  const t=tracker(),s=t.get('tracker').style;
  t.window.receiveJournalSettings({
    trackerLayoutEnabled:true,trackerLayoutX:2800,trackerLayoutY:1300,
    trackerLayoutWidth:2700,trackerLayoutHeight:2100
  });
  assert.equal(s.left,'28.0000vw');
  assert.equal(s.width,'27.0000vw');
  assert.equal(s.height,'21.0000vh');
  t.window.applyJournalTrackerLayout({x:0.32,y:0.14,width:0.3,height:0.23333});
  assert.equal(s.left,'32.0000vw');
  t.window.applyJournalTrackerLayout('native');
  assert.equal(s.left,'28.0000vw');
  t.window.receiveJournalSettings({trackerLayoutEnabled:false,trackerAnchor:'top-right'});
  assert.equal(s.right,'3%');
  assert.equal(s.width,'');
});
test('tracker sends read-only actual view geometry to native without flooding or writing quest state',()=>{
  const t=tracker();
  assert.ok(t.reports.length>=1);
  const g=t.reports[0];
  assert.equal(g.x,1380/1920);
  assert.equal(g.y,54/1080);
  assert.equal(g.width,340/1920);
  assert.equal(g.height,155/1080);
  const before=t.reports.length;
  t.window.receiveJournalSettings({trackerWidth:340});
  assert.equal(t.reports.length,before);
  t.window.applyJournalTrackerLayout({x:0.4,y:0.2,width:0.25,height:0.2});
  t.window.receiveJournalSettings({trackerWidth:340});
  assert.equal(t.reports.length,before);
});
