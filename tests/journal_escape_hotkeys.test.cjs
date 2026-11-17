'use strict';
const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const vm=require('node:vm');
const Model=require('../view/quest-model.js');
const Settings=require('../view/journal-settings.js');

function start(){
 const elements=new Map(),documentListeners=new Map(),nativeCalls=[],focusables=[],dispatchedEvents=[];
 let selectPopup=null;
 const element=(id='')=>{
  if(!elements.has(id)){
   const listeners=new Map();
   const classes=new Set();
   const e={id,style:{setProperty(){}},value:'',textContent:'',innerHTML:'',hidden:false,
     dataset:{},disabled:false,tabIndex:0,type:'button',tagName:'BUTTON',rect:{left:0,top:0,width:80,height:24},
     classList:{contains:name=>classes.has(name),add:name=>classes.add(name),remove:name=>classes.delete(name),toggle(name,on){if(on===undefined)on=!classes.has(name);on?classes.add(name):classes.delete(name);return on}},
     querySelector(){return element('nested')},querySelectorAll(){return ['settingsWorkspace','journalWorkspace'].includes(id)?focusables:[]},
     getAttribute(name){return this.attributes?.[name]??null},
     addEventListener(name,handler){const list=listeners.get(name)||[];list.push(handler);listeners.set(name,list)},
     closest(){return null},getClientRects(){return this.hidden?[]:[this.rect]},getBoundingClientRect(){return this.rect},
     focus(){doc.activeElement=this},scrollIntoView(){this.scrolled=true},
     click(){this.clicked=(this.clicked||0)+1;for(const handler of listeners.get('click')||[])handler()},
     dispatchEvent(event){this.dispatched=(this.dispatched||[]).concat(event.type);return true},
     scrollTop:0,_listeners:listeners};
   elements.set(id,e);
  }
  return elements.get(id);
 };
 const body={className:'journal-page',classList:{contains:k=>k==='journal-page',remove(){},toggle(){}}};
 const doc={body,documentElement:{style:{setProperty(){},fontSize:''}},activeElement:null,
  getElementById:element,
  querySelector:s=>s==='.k2040-select-menu'?selectPopup:element(s),
  querySelectorAll:s=>s==='[data-hub-capture]'?[element('captureButton')]:[],
  addEventListener(name,handler){const list=documentListeners.get(name)||[];list.push(handler);documentListeners.set(name,list)},
  dispatchEvent(event){dispatchedEvents.push(event);for(const handler of documentListeners.get(event.type)||[])handler(event);return true}};
 element('captureButton').dataset.hubCapture='journal';
 const win={location:{search:''},K2040QuestModel:Model,K2040JournalSettings:Settings,
  addEventListener(){},
  requestCloseJournal(){nativeCalls.push('close')},
  requestJournalEscape(){nativeCalls.push('escape')},
  acknowledgeCapturedEscape(){nativeCalls.push('ack')},
  requestHubBindings(){nativeCalls.push('bindings')},
  requestOpenJournalHubSettings(){nativeCalls.push('hub')}};
 vm.runInNewContext(fs.readFileSync('view/app.js','utf8'),{
  window:win,document:doc,URLSearchParams,console,Event:class{constructor(type){this.type=type}},
  KeyboardEvent:class{constructor(type,init){this.type=type;Object.assign(this,init)}},setTimeout:()=>0,setInterval:()=>0
 });
 const click=id=>{
  const handlers=element(id)._listeners.get('click')||[];
  assert.ok(handlers.length,id+' click not registered');handlers[0]();
 };
 const addControl=(id,rect,options={})=>{
  const control=element(id);Object.assign(control,options);control.rect=rect;focusables.push(control);return control;
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
 return {element,win,click,escapeDom,nativeCalls,dispatchedEvents,addControl,activeElement:()=>doc.activeElement,
  setActive:control=>{doc.activeElement=control},setSelectPopup:popup=>{selectPopup=popup}};
}
test('native Escape immediately closes the Journal, not a retired settings panel',()=>{
 const v=start();
 v.win.handleJournalEscape();
 assert.equal(v.nativeCalls.filter(x=>x==='close').length,1);
 assert.equal((v.element('openSettings')._listeners.get('click')||[]).length,1, 'hub-only settings button');
});
test('browser Escape routes through native and never starts local shortcut capture',()=>{
 const v=start();
 v.escapeDom();
 assert.equal(v.nativeCalls.at(-1),'escape');
 v.win.handleJournalEscape();
 assert.equal(v.nativeCalls.filter(x=>x==='close').length,1);
 assert.equal(typeof v.win.receiveHubNativeMouseCapture,'undefined');
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
test('controller direction chooses the nearest enabled visible control and scrolls it into view',()=>{
 const v=start();
 const first=v.addControl('first',{left:0,top:0,width:80,height:24});
 v.addControl('disabled',{left:0,top:30,width:80,height:24},{disabled:true});
 v.addControl('ariaDisabled',{left:0,top:40,width:80,height:24},{attributes:{'aria-disabled':'true'}});
 v.addControl('hidden',{left:0,top:45,width:80,height:24},{hidden:true});
 const next=v.addControl('next',{left:0,top:60,width:80,height:24});
 v.addControl('farther',{left:0,top:100,width:80,height:24});
 v.setActive(first);
 v.win.receiveJournalControllerAction('down');
 assert.equal(v.activeElement(),next);
 assert.equal(next.scrolled,true);
 assert.equal(next.classList.contains('controller-focus-visible'),true);
});
test('controller navigation starts at the first control when nothing in the panel is focused',()=>{
 const v=start();
 const first=v.addControl('first',{left:0,top:0,width:80,height:24});
 v.addControl('second',{left:0,top:30,width:80,height:24});
 v.setActive(null);
 v.win.receiveJournalControllerAction('down');
 assert.equal(v.activeElement(),first);
});
test('controller activation clicks the focused control',()=>{
 const v=start();
 const control=v.addControl('control',{left:0,top:0,width:80,height:24});
 v.setActive(control);
 v.win.receiveJournalControllerAction('activate');
 assert.equal(control.clicked,1);
});
test('controller activation chooses a focused option in the open custom select',()=>{
 const v=start();
 const option=v.element('popupOption');
 v.setActive(option);
 v.setSelectPopup({contains:node=>node===option});
 v.win.receiveJournalControllerAction('activate');
 assert.equal(option.clicked,1);
});
test('controller left and right adjust a focused range by one configured step',()=>{
 const v=start();
 const slider=v.addControl('slider',{left:0,top:0,width:80,height:24},
  {tagName:'INPUT',type:'range',value:'5',min:'0',max:'10',step:'2'});
 v.addControl('neighbor',{left:100,top:0,width:80,height:24});
 v.setActive(slider);
 v.win.receiveJournalControllerAction('right');
 assert.equal(slider.value,'7');
 assert.equal(v.activeElement(),slider);
 assert.ok(slider.dispatched.includes('input'));
 v.win.receiveJournalControllerAction('left');
 assert.equal(slider.value,'5');
});
test('header Settings button is reachable with directional controller navigation and mapped Confirm',()=>{
  const v=start();
  const first=v.addControl('first',{left:120,top:120,width:80,height:24});
  v.element('resetFilters').rect={left:15,top:15,width:80,height:24};
  v.element('openSettings').rect={left:120,top:15,width:80,height:24};
  v.element('closeJournal').rect={left:225,top:15,width:80,height:24};
  v.setActive(first);
  v.win.receiveJournalControllerAction('up');
  assert.equal(v.activeElement(),v.element('openSettings'));
  assert.equal(v.activeElement().scrolled,true);
  v.win.receiveJournalControllerAction('activate');
  assert.equal(v.nativeCalls.filter(x=>x==='hub').length,1,
    'Fallout-mapped Confirm activates the existing Settings button');
  assert.equal(v.nativeCalls.includes('close'),false,
    'JavaScript never closes the Journal before hub accepts the request');
});
test('mouse or keyboard button activation shares the one native hub route',()=>{
  const v=start();
  v.click('openSettings');
  assert.equal(v.nativeCalls.filter(x=>x==='hub').length,1);
  assert.equal(v.nativeCalls.includes('close'),false);
  v.win.receiveJournalHubOpenResult('unavailable');
  const notice=v.element('settingsHubStatus');
  assert.equal(notice.hidden,false);
  assert.match(notice.textContent,/Install or update K2040 Unified Settings/);
});
test('controller Back closes Journal without the retired editor',()=>{
 const v=start();
 v.win.receiveJournalControllerAction('back');
 assert.equal(v.nativeCalls.filter(x=>x==='close').length,1);
});
test('controller Back closes an open quest select before Journal',()=>{
 const v=start();let closed=0;
 v.setSelectPopup({});
 v.win.K2040JournalSelects={close(){closed++;}};
 v.win.receiveJournalControllerAction('back');
 assert.equal(closed,1);
 assert.equal(v.nativeCalls.filter(x=>x==='close').length,0);
});
test('controller directions pass through to an open custom select popup',()=>{
 const v=start();v.setSelectPopup({});
 v.win.receiveJournalControllerAction('right');
 assert.equal(v.dispatchedEvents.at(-1).key,'ArrowRight');
 assert.equal(v.dispatchedEvents.at(-1).bubbles,true);
});
test('native controller actions use the focused UI task path and skip colliding menu hotkeys',()=>{
 const s=fs.readFileSync('src/main.cpp','utf8');
 assert.match(s,/navigation\.Update\(/);
 assert.match(s,/QueueUIAction\(\[name\]\(\)/);
 assert.match(s,/!journalOpen\|\|!journalFocused\.load/);
 assert.match(s,/CompetingGameMenuOpen\(\).*AnotherPrismaViewFocused\(\)/s);
 assert.match(s,/receiveJournalControllerAction",name/);
 assert.match(s,/ConflictsWithMenuNavigation\(/);
});
test('the Journal retains gamepad navigation, but no individual settings editor',()=>{
 const html=fs.readFileSync('view/journal.html','utf8');
 const code=fs.readFileSync('view/app.js','utf8');
 assert.match(html,/id="openSettings"/);
 assert.doesNotMatch(html,/id="settingsWorkspace"/);
 assert.match(code,/function moveControllerFocus/);
 assert.match(code,/function visibleControllerControls/);
 assert.match(code,/window.receiveJournalControllerAction=function/);
});

test("controller-created ArrowDown with open custom select never crashes Journal",()=>{const v=start();v.setSelectPopup({});assert.doesNotThrow(()=>v.win.receiveJournalControllerAction("down"));assert.equal(v.dispatchedEvents.at(-1).key,"ArrowDown");});
