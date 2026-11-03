'use strict';
const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const vm=require('node:vm');
const Model=require('../view/quest-model.js');
const Settings=require('../view/journal-settings.js');

test('tracker settings delivery actually moves all four corners without touching focus',()=>{
  const nodes={
    tracker:{style:{}},
    trackerCount:{textContent:''},
    trackedQuests:{innerHTML:''}
  };
  const calls=[];
  const errors=[];
  const body={className:'tracker-page',classList:{contains:key=>key==='tracker-page'}};
  const root={style:{fontSize:'',setProperty:(name,value)=>calls.push([name,value])}};
  const window={
    location:{search:''},
    K2040QuestModel:Model,
    K2040JournalSettings:Settings
  };
  const document={
    body,documentElement:root,
    getElementById:id=>nodes[id]||null
  };
  vm.runInNewContext(fs.readFileSync('view/app.js','utf8'),{
    window,document,URLSearchParams,console:{error:(...args)=>errors.push(args)},
    setTimeout:()=>0,setInterval:()=>0
  });
  const targets={
    'top-right':['5%','auto','auto','3%'],
    'top-left':['5%','auto','3%','auto'],
    'bottom-right':['auto','5%','auto','3%'],
    'bottom-left':['auto','5%','3%','auto']
  };
  for(const anchor of ['bottom-left','top-right','bottom-right','top-left','bottom-left']){
    window.receiveJournalSettings(JSON.stringify({trackerAnchor:anchor}));
    const {top,bottom,left,right}=nodes.tracker.style;
    assert.deepEqual([top,bottom,left,right],targets[anchor]);
    assert.equal(body.className,'tracker-page anchor-'+anchor+' theme-pipboy');
  }
  window.receiveJournalSettings('{"trackerAnchor":"top-right",bad');
  assert.equal(body.className,'tracker-page anchor-bottom-left theme-pipboy');
  assert.equal(errors.length,1);
  window.receiveJournalSettings(JSON.stringify({theme:'pipboy',hudColor:'#cf6622',hudBackgroundColor:'#111b21',trackerAnchor:'bottom-right'}));
  assert.equal(body.className,'tracker-page anchor-bottom-right theme-pipboy');
  assert.ok(calls.some(([key,value])=>key==='--ui-accent' && value==='#cf6622'));
  assert.ok(calls.some(([key,value])=>key==='--ui-accent-rgb' && value==='207,102,34'));
  window.receiveJournalSettings(JSON.stringify({theme:'contrast',trackerAnchor:'top-left'}));
  assert.equal(body.className,'tracker-page anchor-top-left theme-contrast');
  assert.ok(calls.some(([key,value])=>key==='--ui-accent' && value==='#00e5ff'));
  window.receiveJournalSettings(JSON.stringify({theme:'fallout76',trackerAnchor:'bottom-left'}));
  assert.equal(body.className,'tracker-page anchor-bottom-left theme-fallout76');
  assert.ok(calls.some(([key,value])=>key==='--ui-accent' && value==='#f5cb5b'));
  assert.ok(calls.some(([key])=>key==='--tracker-width'));
  assert.equal(nodes.trackedQuests.innerHTML.includes('Waiting for quest data'),true);
  assert.equal(window.requestCloseJournal,undefined);
});
