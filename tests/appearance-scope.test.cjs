'use strict';
const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const vm=require('node:vm');
const Model=require('../view/quest-model.js');
const Settings=require('../view/journal-settings.js');

test('full-screen journal appearance stays independent of native mini-tracker preferences',()=>{
  const nodes={};
  const makeNode=()=>({
    style:{},
    value:'',textContent:'',innerHTML:'',checked:false,hidden:false,
    scrollTop:0,
    dataset:{},
    classList:{contains:()=>false,toggle(){},remove(){},add(){}},
    querySelectorAll:()=>[],
    addEventListener(){},focus(){},closest(){return null;}
  });
  const node=id=>nodes[id]||(nodes[id]=makeNode());
  const css={};
  const root={style:{
    fontSize:'',
    setProperty:(key,value)=>{css[key]=String(value);},
    getPropertyValue:key=>css[key]
  }};
  const previewCss={};
  const preview=makeNode();
  preview.style={setProperty:(key,value)=>{previewCss[key]=String(value);}};
  const body={className:'journal-page',classList:{
    contains:key=>key==='journal-page',
    remove(){},toggle(){}
  }};
  const document={
    body,documentElement:root,activeElement:null,
    getElementById:node,
    querySelector:selector=>selector==='.tracker-sample'?preview:node(selector),
    querySelectorAll:()=>[],
    addEventListener(){}
  };
  const window={location:{search:''},K2040QuestModel:Model,
    K2040JournalSettings:Settings,addEventListener(){}};
  vm.runInNewContext(fs.readFileSync('view/app.js','utf8'),{
    window,document,URLSearchParams,console,setTimeout:()=>0,setInterval:()=>0
  });
  const constantValues=()=>({
    accent:css['--ui-accent'],
    text:css['--ui-text'],
    completed:css['--ui-completed'],
    panel:css['--ui-panel-background'],
    size:root.style.fontSize
  });
  // Explicit K2040 Dark keeps the journal fixed while tracker theme changes.
  window.receiveJournalSettings(JSON.stringify({journalTheme:'dark'}));
  const initial=constantValues();
  assert.equal(initial.accent,'#66c4ff');
  assert.equal(initial.text,'#f4faff');
  assert.equal(initial.panel,'rgba(11,18,24,0.98)');
  assert.equal(initial.size,'14px');

  window.receiveJournalSettings(JSON.stringify({
    theme:'pipboy',journalTheme:'dark',hudColor:'#de5210',hudBackgroundColor:'#1c2b30',
    trackerOpacity:25,trackerScale:130,opacity:40,fontSize:150
  }));
  assert.deepEqual(constantValues(),initial);

  window.receiveJournalSettings(JSON.stringify({
    theme:'custom',journalTheme:'dark',accent:'#dc1466',text:'#abbacd',panel:'#030405',
    completed:'#ffaa00',trackerOpacity:100,trackerScale:75
  }));
  assert.deepEqual(constantValues(),initial);

  window.receiveJournalSettings(JSON.stringify({
    theme:'commonwealth',journalTheme:'dark',density:'compact',sidebarWidth:320,questListWidth:440
  }));
  assert.deepEqual(constantValues(),initial);
  assert.equal(css['--journal-sidebar-width'],'320px');
  assert.equal(css['--journal-list-width'],'440px');

  // Journal changes are independent and never overwrite saved tracker prefs.
  window.receiveJournalSettings(JSON.stringify({
    theme:'pipboy',hudColor:'#ff9900',
    journalTheme:'custom',journalAccent:'#aa2255',journalText:'#fcfcda',
    journalPanel:'#121a23',journalCompleted:'#55ffaa',
    journalOpacity:72,journalFontSize:120
  }));
  assert.equal(css['--ui-accent'],'#aa2255');
  assert.equal(css['--ui-text'],'#fcfcda');
  assert.equal(css['--ui-completed'],'#55ffaa');
  assert.equal(css['--ui-panel-background'],'rgba(18,26,35,0.72)');
  assert.equal(root.style.fontSize,'16.8px');

  window.receiveJournalSettings(JSON.stringify({theme:'contrast',journalTheme:'fallout76'}));
  assert.equal(css['--ui-accent'],'#f5cb5b');
  assert.equal(css['--ui-text'],'#ffffcb');
  assert.equal(css['--ui-panel-background'],'rgba(24,28,29,0.98)');

  window.receiveJournalSettings(JSON.stringify({theme:'fallout76',journalTheme:'contrast'}));
  assert.equal(css['--ui-accent'],'#00e5ff');
  assert.equal(css['--ui-completed'],'#63ff9b');

  window.receiveJournalSettings(JSON.stringify({theme:'commonwealth',journalTheme:'dark'}));
  assert.deepEqual(constantValues(),initial);
  window.receiveJournalSettings(JSON.stringify({
    journalTheme:'pipboy',hudColor:'#bb9944',hudBackgroundColor:'#162313'
  }));
  assert.equal(css['--ui-accent'],'#bb9944');
  assert.equal(css['--ui-text'],'#bb9944');
  assert.equal(css['--ui-panel-background'],'rgba(22,35,19,0.98)');
  // Hub themes temporarily override the live CSS tokens, never the owner INI.
  window.applyUnifiedTheme('midnight');
  assert.equal(css['--ui-accent'],'#b4a8ff');
  assert.equal(css['--ui-panel-background'],'rgba(23,26,49,0.98)');
  assert.equal(css['--bg'],'#090d18');
  window.applyUnifiedTheme('high-contrast');
  assert.equal(css['--ui-accent'],'#ffff73');
  window.applyUnifiedTheme('pipboy');
  assert.equal(css['--ui-accent'],'#93f694');
  window.applyUnifiedTheme('native');
  assert.equal(css['--ui-accent'],'#bb9944');
  assert.equal(css['--ui-panel-background'],'rgba(22,35,19,0.98)');
});
