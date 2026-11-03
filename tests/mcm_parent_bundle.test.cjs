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
test('build stages both MCM directories as part of the mod package',()=>{
 const xmake=fs.readFileSync(path.join(root,'xmake.lua'),'utf8');
 assert.match(xmake,/\{"K2040_Mods","K2040_Quest_Journal"\}/);
 assert.match(xmake,/os\.cp\(path\.join\(source,"\*"\),target\)/);
 const native=fs.readFileSync(path.join(root,'src/HubBindings.cpp'),'utf8');
 assert.ok(native.includes('GetModuleHandleW(L"mcm.dll")'));
 assert.ok(native.includes('GetModuleHandleW(L"F4SEMenuFramework.dll")'));
 assert.ok(native.includes('iQuickAttachPadPrimary'));
 assert.ok(native.includes('K2040_Quick_Attach_Menu.ini'));
 const stage=fs.readFileSync(path.join(root,'scripts/stage-test-package.py'),'utf8');
 assert.match(stage,/CANONICAL_PARENT_SHA256/);
 assert.match(stage,/result\[PARENT_PATH\]=raw/);
});

test('MCM fallback survives unbound but live registration does not rewrite disk at loader entry',()=>{
 const bindings=fs.readFileSync(path.join(root,'src/HubBindings.cpp'),'utf8');
 const entry=fs.readFileSync(path.join(root,'src/main.cpp'),'utf8');
 assert.match(entry,/K2040::Hub::SeedDefaultKeyboardBindings\(\);/);
 assert.match(entry,/const auto virtualKey=/);
 assert.match(bindings,/void SeedDefaultKeyboardBindings\(\) noexcept/);
 assert.match(bindings,/unsigned key='G'/);
 assert.match(bindings,/hotkeys-initialized/);
 assert.match(bindings,/if\(found\)continue/);
 assert.match(bindings,/if\(NativeMcmOwnsRegistry\(\)\)return;/);
 assert.match(bindings,/if\(initialized\)return;/);
 assert.match(bindings,/unsigned modifiers=action==Action::Journal\?1u:0u/);
 assert.match(bindings,/if\(!key\)continue;/);
 assert.doesNotMatch(bindings,/unsigned key=initialized\?0:/);
 assert.match(bindings,/if\(!k\.registered && everInitialized\)k\.registered=true;/);
 assert.match(bindings,/SeedDefaultKeyboardBindings\(\);/);
 // A registered-but-empty hotkey must NOT reactivate the standalone defaults.
 assert.match(entry,/!K2040::Hub::ReadKeyboard\(K2040::Hub::Action::Journal\)\.registered/);
 assert.match(entry,/!K2040::Hub::ReadKeyboard\(K2040::Hub::Action::Tracker\)\.registered/);
 const ui=fs.readFileSync(path.join(root,'view/app.js'),'utf8');
 assert.match(ui,/!hubControls\.journal\?\.registered/);
});
