'use strict';
const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const root=path.resolve(__dirname,'..');
const dir='mcm/Data/MCM/Config/K2040_Quest_Journal/';
const obj=f=>JSON.parse(fs.readFileSync(path.join(root,f),'utf8'));

test('MCM grouped child preserves own namespace and two functional callback actions',()=>{
 const m=obj(dir+'config.json'),kb=obj(dir+'keybinds.json');
 assert.equal(m.modName,'K2040_Quest_Journal');
 assert.equal(m.ownerModName,'K2040_Mods');
 assert.deepEqual(m.content,[]);
 assert.equal(m.pages.length,1);
 assert.equal(m.pages[0].pageDisplayName,'Quest Journal');
 const actions=m.pages[0].content.filter(x=>x.type==='hotkey');
 assert.deepEqual(actions.map(x=>x.id),['questJournalToggle','questTrackerToggle']);
 assert.ok(actions.every(x=>x.valueOptions.allowModifierKeys===1));
 assert.equal(kb.modName,'K2040_Quest_Journal');
 assert.deepEqual(kb.keybinds.map(x=>[x.id,x.action.type,x.action.command]),[
  ['questJournalToggle','RunConsoleCommand',''],
  ['questTrackerToggle','RunConsoleCommand','']
 ]);
 const pads=m.pages[0].content.filter(x=>x.type==='dropdown');
 assert.equal(pads.length,4);
 assert.equal(new Set(pads.map(x=>x.id)).size,4);
 assert.ok(pads.every(x=>x.valueOptions.sourceType==='ModSettingInt'));
 assert.ok(pads.every(x=>x.valueOptions.options.length===17));
});
test('standalone child differs only by owner grouping, no duplicate live menu',()=>{
 const g=obj(dir+'config.json'),s=obj('mcm/standalone/'+dir.slice(4)+'config.json');
 assert.equal(s.modName,g.modName);
 assert.equal(Object.hasOwn(s,'ownerModName'),false);
 const expected={...g};delete expected.ownerModName;
 assert.deepEqual(s,expected);
 assert.deepEqual(obj('mcm/standalone/'+dir.slice(4)+'keybinds.json'),obj(dir+'keybinds.json'));
 const defaults=fs.readFileSync(path.join(root,dir,'settings.ini'),'utf8');
 for(const key of ['iQuestJournalPadPrimary','iQuestJournalPadModifier','iQuestTrackerPadPrimary','iQuestTrackerPadModifier'])assert.match(defaults,new RegExp(key+'=0'));
});
test('child-owned native adapter has no parent-owned live action registration',()=>{
 const native=fs.readFileSync(path.join(root,'src/HubBindings.cpp'),'utf8');
 assert.match(native,/Data\/MCM\/Config\/K2040_Quest_Journal\/config.json/);
 assert.match(native,/Data\/MCM\/Settings\/K2040_Quest_Journal.ini/);
 assert.match(native,/K2040_Quest_Journal/);
 assert.match(native,/MigrateLegacyIfNeeded/);
 assert.match(native,/Data\/MCM\/Settings\/K2040_Mods.ini/);
 const entry=fs.readFileSync(path.join(root,'src/main.cpp'),'utf8');
 assert.match(entry,/Register\("K2040_Quest_Journal",\s*&RegisterHubMcm\)/);
 assert.match(entry,/DispatchAction/);
 assert.doesNotMatch(entry,/redirectQuestKey:true/);
});
