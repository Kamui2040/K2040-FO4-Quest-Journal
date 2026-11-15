const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const crypto=require('node:crypto');
const path=require('node:path');
const root=path.resolve(__dirname,'..');
test('Quest Journal bundles exact canonical K2040 parent without centralized actions',()=>{
 const names=[
  'mcm/Data/MCM/Config/K2040_Mods/config.json',
  'dist/Data/MCM/Config/K2040_Mods/config.json'
 ];
 const data=[fs.readFileSync(path.join(root,names[0]))];
 // Fresh source clones have no generated dist folder; validate staged copy only when present.
 if(fs.existsSync(path.join(root,names[1])))data.push(fs.readFileSync(path.join(root,names[1])));
 if(data.length>1)assert.deepEqual(data[0],data[1]);
 assert.equal(crypto.createHash('sha256').update(data[0]).digest('hex'),
  '8eccc0c1313d1ec982e6094650835e7f83b42e03c56d8435a81ff55c3aba49eb');
 const parent=JSON.parse(data[0].toString());
 assert.equal(parent.modName,'K2040_Mods');
 assert.deepEqual(parent.pages,[]);
 assert.deepEqual(parent.pluginRequirements,[]);
 for(const id of ['questJournalToggle','questTrackerToggle','minimapSettingsToggle','openQuickMenu'])
  assert.doesNotMatch(data[0].toString(),new RegExp(id));
});
test('build stages only canonical shared MCM parent, leaving hub as sole keybind owner',()=>{
 const xmake=fs.readFileSync(path.join(root,'xmake.lua'),'utf8');
 assert.match(xmake,/parentdest/);
 assert.doesNotMatch(xmake,/for _,name in ipairs\(\{"K2040_Mods","K2040_Quest_Journal"\}/);
 const stage=fs.readFileSync(path.join(root,'scripts/stage-test-package.py'),'utf8');
 assert.match(stage,/CANONICAL_PARENT_SHA256/);
 assert.match(stage,/result\[PARENT_PATH\]=raw/);
 assert.doesNotMatch(stage,/keybinds\.json/);
});
test('MCM is never used to register Journal gameplay shortcuts',()=>{
 const entry=fs.readFileSync(path.join(root,'src/main.cpp'),'utf8');
 assert.match(entry,/PublishGameplayBindings\(\)/);
 assert.doesNotMatch(entry,/SeedDefaultKeyboardBindings|RefreshNativeMcm|RegisterHubMcm/);
 assert.match(entry,/gameplayKeyboard\[0\]\.store/);
 assert.match(entry,/gameplayController\[0\]\.store/);
});
