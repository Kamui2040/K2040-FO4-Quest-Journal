'use strict';
const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const read=name=>fs.readFileSync(name,'utf8');
const native=read('src/main.cpp');
test('Journal ships no child MCM config, keybind registry or native MCM bridge',()=>{
  for(const name of [
    'mcm/Data/MCM/Config/K2040_Quest_Journal',
    'mcm/standalone/Data/MCM/Config/K2040_Quest_Journal',
    'src/HubBindings.cpp','src/HubMcmLive.cpp'
  ]) assert.equal(fs.existsSync(name),false,name);
  for(const removed of ['RegisterHubMcm','RegisterHubScaleformOnce','RefreshNativeMcm',
      'SeedDefaultKeyboardBindings','StageNativeMcmBinding','ReadKeyboard','ReadPad'])
    assert.equal(native.includes(removed),false,removed);
  assert.match(native,/PublishGameplayBindings/);
  assert.match(native,/gameplayKeyboard/);
  assert.match(native,/gameplayController/);
  assert.match(native,/requestJournalPipboySync/);
});
test('standalone or grouped owner packaging never adds MCM',()=>{
 const stage=read('scripts/stage-test-package.py');
 assert.match(stage,/every owner QA ZIP excludes MCM/);
 assert.doesNotMatch(stage,/result\[PARENT_PATH\]=raw/);
 const build=read('xmake.lua');
 assert.doesNotMatch(build,/local parentdest=/);
 assert.equal(fs.existsSync('mcm/Data/MCM/Config/K2040_Mods/config.json'),false);
});
