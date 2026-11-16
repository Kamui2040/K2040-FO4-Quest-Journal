const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const crypto=require('node:crypto');
const path=require('node:path');
const root=path.resolve(__dirname,'..');
test('Quest Journal owner has no MCM parent, child or keyboard action',()=>{
 for(const rel of [
  'mcm/Data/MCM/Config/K2040_Mods/config.json',
  'dist/Data/MCM/Config/K2040_Mods/config.json',
  'mcm/Data/MCM/Config/K2040_Quest_Journal/config.json'])
  assert.equal(fs.existsSync(path.join(root,rel)),false,rel);
 const xmake=fs.readFileSync(path.join(root,'xmake.lua'),'utf8');
 assert.doesNotMatch(xmake,/os\.cp\(parent,parentdest\)/);
 const stage=fs.readFileSync(path.join(root,'scripts/stage-test-package.py'),'utf8');
 assert.match(stage,/every owner QA ZIP excludes MCM/);
 assert.doesNotMatch(stage,/result\[PARENT_PATH\]=raw/);
});
test('MCM is never used to register Journal gameplay shortcuts',()=>{
 const entry=fs.readFileSync(path.join(root,'src/main.cpp'),'utf8');
 assert.match(entry,/PublishGameplayBindings\(\)/);
 assert.doesNotMatch(entry,/SeedDefaultKeyboardBindings|RefreshNativeMcm|RegisterHubMcm/);
 assert.match(entry,/gameplayKeyboard\[0\]\.store/);
 assert.match(entry,/gameplayController\[0\]\.store/);
});
