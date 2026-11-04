'use strict';
const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const vm=require('node:vm');
const Model=require('../view/quest-model.js');
const Settings=require('../view/journal-settings.js');

function start(){
 const elements=new Map(),documentListeners=new Map(),nativeCalls=[];
 const element=(id='')=>{
  if(!elements.has(id)){
   const listeners=new Map();
   const e={id,style:{setProperty(){}},value:'',textContent:'',innerHTML:'',hidden:false,
     dataset:{},classList:{contains(){return false},add(){},remove(){},toggle(){}},
     querySelector(){return element('nested')},querySelectorAll(){return []},
     addEventListener(name,handler){const list=listeners.get(name)||[];list.push(handler);listeners.set(name,list)},
     closest(){return null},focus(){doc.activeElement=this},scrollTop:0,_listeners:listeners};
   elements.set(id,e);
  }
  return elements.get(id);
 };
 const body={className:'journal-page',classList:{contains:k=>k==='journal-page',remove(){},toggle(){}}};
 const doc={body,documentElement:{style:{setProperty(){},fontSize:''}},activeElement:null,
  getElementById:element,
  querySelector:s=>element(s),
  querySelectorAll:s=>s==='[data-hub-capture]'?[element('captureButton')]:[],
  addEventListener(name,handler){const list=documentListeners.get(name)||[];list.push(handler);documentListeners.set(name,list)}};
 element('captureButton').dataset.hubCapture='journal';
 const win={location:{search:''},K2040QuestModel:Model,K2040JournalSettings:Settings,
  addEventListener(){},
  requestCloseJournal(){nativeCalls.push('close')},
  requestJournalEscape(){nativeCalls.push('escape')},
  acknowledgeCapturedEscape(){nativeCalls.push('ack')},
  requestHubBindings(){nativeCalls.push('bindings')}};
 vm.runInNewContext(fs.readFileSync('view/app.js','utf8'),{
  window:win,document:doc,URLSearchParams,console,setTimeout:()=>0,setInterval:()=>0
 });
 const click=id=>{
  const handlers=element(id)._listeners.get('click')||[];
  assert.ok(handlers.length,id+' click not registered');handlers[0]();
 };
 const escapeDom=()=>{
  let prevented=false,stopped=false;
  const ev={key:'Escape',code:'Escape',repeat:false,target:element('target'),
    preventDefault(){prevented=true},stopPropagation(){},
    stopImmediatePropagation(){stopped=true;}};
  for(const handler of documentListeners.get('keydown')||[]){
    handler(ev);
    if(stopped)break;
  }
  assert.ok(prevented);
 };
 return {element,win,click,escapeDom,nativeCalls,activeElement:()=>doc.activeElement};
}
test('native Escape closes Settings first and Journal on next press',()=>{
 const v=start();
 v.click('openSettings');
 assert.equal(v.element('settingsWorkspace').hidden,false);
 assert.equal(v.activeElement().id,'[data-settings-page="appearance"]');
 v.win.handleJournalEscape();
 assert.equal(v.element('settingsWorkspace').hidden,true);
 assert.equal(v.activeElement().id,'openSettings');
 assert.equal(v.nativeCalls.includes('close'),false);
 v.win.handleJournalEscape();
 assert.equal(v.nativeCalls.filter(x=>x==='close').length,1);
});
test('browser Escape routes through native only once and shortcut capture consumes it',()=>{
 const v=start();
 v.click('openSettings');
 v.escapeDom();
 assert.equal(v.nativeCalls.at(-1),'escape');
 assert.equal(v.element('settingsWorkspace').hidden,false);
 v.win.receiveHubBindings(JSON.stringify({
   installed:true,journal:{registered:true},tracker:{registered:true}
 }));
 v.click('captureButton');
 v.escapeDom();
 assert.equal(v.nativeCalls.at(-1),'ack');
 assert.equal(v.element('settingsWorkspace').hidden,false);
});
test('native physical Escape and chord detection have scoped fallbacks',()=>{
 const s=fs.readFileSync('src/main.cpp','utf8');
 assert.match(s,/controller\.SetViewOwnsEscape\) controller\.SetViewOwnsEscape\(view,true\)/);
 assert.match(s,/interop\.BindUIEvent\(view, "requestJournalEscape"/);
 assert.match(s,/interop\.InteropCall\(journalView,"handleJournalEscape",""\)/);
 assert.match(s,/escapeUiAcknowledged\.exchange\(false/);
 assert.match(s,/const bool matched=k\.key && down\(virtualKey\(k\.key\)\)/);
 assert.match(s,/const bool rising=matched&&!k\.wasMatched/);
 assert.match(s,/const bool gameInputActive=focused \|\| ForegroundGame\(\)/);
 assert.doesNotMatch(s,/const bool rising=pressed&&!k\.wasDown/);
});
