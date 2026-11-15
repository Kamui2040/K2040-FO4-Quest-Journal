const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const S=require('../view/journal-settings.js');
const read=name=>fs.readFileSync(name,'utf8');
test('Journal and tracker gameplay shortcuts live in owner preferences and hub editor',()=>{
  assert.equal(S.defaults.journalHotkey,'Shift+G');
  assert.equal(S.defaults.trackerHotkey,'G');
  assert.equal(S.normalize({journalHotkey:'F8',trackerHotkey:'F10'}).journalHotkey,'F8');
  assert.equal(S.normalize({journalHotkey:'F8',trackerHotkey:'F10'}).trackerHotkey,'F10');
  const cpp=read('src/JournalSettings.h');
  assert.match(cpp,/journalHotkey = "Shift\+G"/);
  assert.match(cpp,/trackerHotkey = "G"/);
  assert.match(read('src/main.cpp'),/keyboard\(settings\.journalHotkey\)/);
  assert.match(read('src/main.cpp'),/keyboard\(settings\.trackerHotkey\)/);
  const manifest=JSON.parse(read('view/settings-v1-manifest.json'));
  const fields=manifest.categories.flatMap(c=>c.groups.flatMap(g=>g.fields));
  for(const id of ['journalHotkey','trackerHotkey'])
    assert.equal(fields.find(f=>f.id===id)?.type,'keybind',id);
  for(const id of ['journalPadPrimary','journalPadHeld','trackerPadPrimary','trackerPadHeld'])
    assert.equal(fields.find(f=>f.id===id)?.type,'select',id);
  assert.doesNotMatch(read('view/journal.html'),/id="openSettings"|id="settingsWorkspace"/);
});
test('only the shared hub supplies MCM; Journal has no nested entries',()=>{
  assert.equal(fs.existsSync('mcm/Data/MCM/Config/K2040_Mods/config.json'),false);
  assert.equal(fs.existsSync('mcm/Data/MCM/Config/K2040_Quest_Journal'),false);
  assert.equal(fs.existsSync('dist/Data/MCM/Config/K2040_Mods/config.json'),false);
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
