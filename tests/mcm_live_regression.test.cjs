'use strict';
const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const native=fs.readFileSync('src/main.cpp','utf8');
const source=fs.readFileSync('src/JournalSettings.cpp','utf8');
test('standalone gameplay keybinds follow saved owner preferences, not MCM',()=>{
  assert.match(native,/gameplayKeyboard\[0\]\.store/);
  assert.match(native,/gameplayKeyboard\[1\]\.store/);
  assert.match(native,/gameplayController\[0\]\.store/);
  assert.match(native,/gameplayController\[1\]\.store/);
  assert.match(native,/PublishGameplayBindings\(\)/);
  assert.match(native,/keyboard\(settings\.journalHotkey\)/);
  assert.doesNotMatch(native,/SetKeybind|RemapKeybind|ClearKeybind/);
  assert.doesNotMatch(native,/RefreshNativeMcm|RegisterHubMcm|ReadKeyboard/);
});
test('legacy controller values imported read-only and owner-local values win',()=>{
  assert.match(source,/ReadLegacyPad\(oldPath,old\)/);
  assert.match(source,/Data\/MCM\/Settings\/K2040_Quest_Journal\.ini/);
  assert.match(source,/saved\.find\(std::string\("\|"\)\+current\+"\|"\)/);
  assert.match(source,/K2040.*Quest.*Journal.*ini/);
  assert.match(source,/Save\(\)/);
  assert.doesNotMatch(source,/Keybinds\.json/);
  assert.doesNotMatch(source,/MoveFileExW\(.*oldPath/);
});
test('modifier and gamepad shortcut controls reject collisions before saving',()=>{
  assert.match(source,/next\.journalHotkey==next\.trackerHotkey/);
  assert.match(source,/next\.journalPadPrimary==next\.journalPadHeld/);
  assert.match(source,/next\.trackerPadPrimary==next\.trackerPadHeld/);
  assert.match(source,/next\.journalPadPrimary==next\.trackerPadPrimary/);
  assert.match(native,/ConflictsWithMenuNavigation/);
  assert.match(native,/ChordLatch/);
});
