'use strict';
const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const vm=require('node:vm');
const Model=require('../view/quest-model.js');
const Settings=require('../view/journal-settings.js');

function setup(){
  const handlers={};const emissions=[];const nodes={};
  const make=()=>({
    style:{},dataset:{},checked:false,disabled:false,textContent:'',
    value:'',innerHTML:'',scrollTop:0,hidden:false,
    classList:{contains:()=>false,toggle(){},remove(){},add(){}},
    addEventListener(){},querySelectorAll:()=>[],getClientRects:()=>[],
    closest(){return null;},focus(){}
  });
  const element=id=>nodes[id]||(nodes[id]=Object.assign(make(),{id}));
  const document={
    body:{className:'journal-page',classList:{contains:()=>false,toggle(){},remove(){},add(){}}},
    documentElement:{style:{setProperty(){},removeProperty(){}}},
    getElementById:element,
    querySelector:()=>element('shell'),querySelectorAll:()=>[],
    addEventListener:(type,cb)=>{(handlers[type]??=[]).push(cb);}
  };
  const window={K2040QuestModel:Model,K2040JournalSettings:Settings,
    location:{search:''},addEventListener(){},
    requestJournalPipboySync:value=>emissions.push(value)};
  vm.runInNewContext(fs.readFileSync('view/app.js','utf8'),
    {window,document,URLSearchParams,console,setTimeout:()=>0,setInterval:()=>0});
  return {window,element,handlers,emissions};
}
test('mirror toggle belongs to Journal and is not a hub field',()=>{
  const html=fs.readFileSync('view/journal.html','utf8');
  const native=fs.readFileSync('src/main.cpp','utf8');
  const manifest=JSON.parse(fs.readFileSync('view/settings-v1-manifest.json','utf8'));
  const ids=manifest.categories.flatMap(c=>c.groups.flatMap(g=>g.fields.map(f=>f.id)));
  assert.match(html,/<input id="pipboySyncToggle" type="checkbox">/);
  assert.match(html,/id="pipboySyncStatus"/);
  assert.ok(!ids.includes('trackerSyncPipboy'));
  assert.ok(manifest.internalKeys.includes('trackerSyncPipboy'));
  assert.match(native,/BindUIEvent\(view, "requestJournalPipboySync"/);
  assert.match(native,/value=="true"\?"trackerSyncPipboy:true"/);
  assert.match(native,/"opacity","fontSize","trackerSyncPipboy"/);
  assert.match(native,/static_assert\(!K2040::QuestTracking::CanWritePipboy\(\)/);
});
test('direct Journal sync saves and restores previous state on storage failure',()=>{
  const h=setup(),box=h.element('pipboySyncToggle');
  h.window.receiveJournalSettings({trackerSyncPipboy:false});
  assert.equal(box.checked,false);
  box.checked=true;
  for(const fn of h.handlers.change)fn({target:box});
  assert.deepEqual(h.emissions,['true']);
  assert.equal(box.disabled,true);
  h.window.receiveJournalSettings({trackerSyncPipboy:true});
  h.window.receiveJournalPipboySyncStatus('saved');
  assert.equal(box.disabled,false);
  assert.equal(box.checked,true);
  assert.equal(h.element('pipboySyncStatus').textContent,'Preference saved.');
  box.checked=false;
  for(const fn of h.handlers.change)fn({target:box});
  assert.deepEqual(h.emissions,['true','false']);
  h.window.receiveJournalPipboySyncStatus('storage');
  assert.equal(box.checked,true);
  assert.equal(box.disabled,false);
  assert.match(h.element('pipboySyncStatus').textContent,/Unable to save/);
});
