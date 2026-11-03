const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const S=require('../view/journal-settings.js');
const read=name=>fs.readFileSync(name,'utf8');
const grouped=JSON.parse(read('mcm/Data/MCM/Config/K2040_Quest_Journal/config.json'));
const standalone=JSON.parse(read('mcm/standalone/Data/MCM/Config/K2040_Quest_Journal/config.json'));
test('native and Prisma standalone factory defaults agree without removing legacy user choices',()=>{
 assert.equal(S.defaults.journalHotkey,'Shift+G');
 assert.equal(S.defaults.trackerHotkey,'G');
 assert.equal(S.normalize({journalHotkey:'F8',trackerHotkey:'F10'}).journalHotkey,'F8');
 assert.equal(S.normalize({journalHotkey:'F8',trackerHotkey:'F10'}).trackerHotkey,'F10');
 const cpp=read('src/JournalSettings.h');
 assert.match(cpp,/journalHotkey = "Shift\+G"/);
 assert.match(cpp,/trackerHotkey = "G"/);
 assert.match(read('src/HubBindings.cpp'),/unsigned key='G';/);
 assert.match(read('src/HubBindings.cpp'),/unsigned modifiers=action==Action::Journal\?1u:0u/);
 assert.match(read('src/HubMcmLive.cpp'),/InvokeUpdate\(movie,"root.mcm.SetKeybind",i,key,mods\)/);
 assert.match(read('src/HubBindings.cpp'),/if\(initialized\)return;/);
 assert.match(read('src/HubBindings.cpp'),/if\(found\)continue/);
 const ui=read('view/app.js');
 assert.match(ui,/action,first:71,second:action==='journal'\?1:0/);
 assert.match(ui,/state.settings.journalHotkey==='Shift\+G'/);
 assert.match(read('src/main.cpp'),/shift==\(binding=="Shift\+G"\)/);
});
test('grouped and standalone MCM pages place keyboard toggles side by side',()=>{
 for(const config of [grouped,standalone]){
  const items=config.pages[0].content;
  const ids=items.map(item=>item.id);
  const journal=ids.indexOf('questJournalToggle');
  assert.ok(journal>=0);
  assert.equal(ids[journal+1],'questTrackerToggle');
  assert.ok(ids.indexOf('iQuestJournalPadPrimary:Controller')>journal+1);
  assert.ok(ids.indexOf('iQuestTrackerPadPrimary:Controller')>journal+1);
  assert.ok(items.some(item=>item.type==='text' && item.text.includes('Shift+G and G')));
 }
});
test('all Journal selects use one themed popup and preserve native change-event contract',()=>{
 const html=read('view/journal.html'),css=read('view/styles.css');
 const script=read('view/quest-selects.js'),pack=read('scripts/stage-test-package.py');
 assert.match(html,/<script src="quest-selects.js"><\/script>/);
 assert.match(css,/.journal-page \.k2040-select-menu/);
 assert.match(css,/--ui-popup-background/);
 assert.match(script,/select.dispatchEvent\(new Event\('change',\{bubbles:true\}\)\)/);
 assert.match(script,/role','listbox'/);
 assert.match(script,/event.stopImmediatePropagation\(\)/);
 assert.match(script,/querySelectorAll\('.journal-page select:not\(.k2040-select-source\)'\)/);
 assert.match(read('view/app.js'),/K2040JournalSelects\?\.sync\(\)/);
 assert.match(pack,/"quest-selects.js"/);
});
