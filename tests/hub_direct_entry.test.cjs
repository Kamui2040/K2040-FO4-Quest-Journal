'use strict';
const {test}=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs'),path=require('node:path');
const read=f=>fs.readFileSync(path.join(__dirname,'..',f),'utf8');
test('Journal settings button requests only the shared hub owner page',()=>{
  const html=read('view/journal.html'),js=read('view/app.js'),native=read('src/main.cpp');
  assert.match(html,/<button id="openSettings"[^>]*>Settings<\/button>/);
  assert.match(html,/id="pipboySyncToggle"/,'Pip-Boy mirror must stay in the Journal');
  assert.doesNotMatch(html,/id="settingsWorkspace"|K2040_Settings_Hub\/index\.html/);
  assert.match(js,/emit\('requestOpenJournalHubSettings'\)/);
  assert.match(js,/receiveJournalHubOpenResult/);
  assert.match(native,/BindUIEvent\(view,"requestOpenJournalHubSettings"/);
  assert.match(native,/GetModuleHandleW\(L"K2040_Unified_Settings\.dll"\)/);
  assert.match(native,/GetProcAddress\(hub,K2040_REQUEST_MODULE_SETTINGS_V1_EXPORT\)/);
  assert.match(native,/request\("quest-journal"\)/);
  assert.match(native,/if\(!request \|\| !request\("quest-journal"\)\)\{reportUnavailable\(\);return;\}\s*CloseJournal\(\);/);
  assert.match(native,/interop\.InteropCall\(journalView,"receiveJournalHubOpenResult","unavailable"\)/);
  assert.doesNotMatch(js,/window\.open\(|location\.href\s*=.*Settings_Hub/);
});
test('Mod-only package has no individual MCM registration or shared settings DLL',()=>{
  const script=read('scripts/stage-test-package.py');
  assert.match(script,/Quest Journal must not ship an MCM menu or user settings/);
  assert.match(script,/def with_mcm\(/);
  assert.doesNotMatch(script,/K2040_Unified_Settings\.dll/);
  assert.equal(fs.existsSync(path.join(__dirname,'..','mcm','Data','MCM','Config','K2040_Quest_Journal')),false);
  assert.equal(fs.existsSync(path.join(__dirname,'..','mcm','Data','MCM','Config','K2040_Mods')),false);
});
