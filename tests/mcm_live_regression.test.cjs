'use strict';
const {test}=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs');
const hub=fs.readFileSync('src/HubBindings.cpp','utf8');
const live=fs.readFileSync('src/HubMcmLive.cpp','utf8');
const main=fs.readFileSync('src/main.cpp','utf8');
test('native MCM owns registry even if its DLL is not yet loaded',()=>{
 assert.match(hub,/is_regular_file\(L"Data\/F4SE\/Plugins\/mcm.dll"/);
 assert.match(hub,/if\(NativeMcmOwnsRegistry\(\)\)return;/);
 assert.match(hub,/if\(NativeMcmOwnsRegistry\(\)\)\s*return StageNativeMcmBinding/);
 assert.match(hub,/if\(initialized\)return;/);
 assert.match(hub,/if\(!key\)continue; \/\/ collision: preserve absence/);
});
test('live MCM updates use native API, never write Keybinds.json',()=>{
 assert.match(live,/GetAllKeybinds/);
 assert.match(live,/root\.mcm\.SetKeybind/);
 assert.match(live,/root\.mcm\.RemapKeybind/);
 assert.match(live,/root\.mcm\.ClearKeybind/);
 assert.match(live,/Owned\(\*live,i\)\.size\(\)!=1/);
 assert.match(live,/if\(own\.size\(\)>1\)continue/);
 assert.match(live,/if\(!request->key && present/);
 assert.match(live,/Occupied\(\*live,beforeKey,beforeMods,i\)/);
 assert.doesNotMatch(live,/WriteAtomic\(/);
 assert.match(main,/AddUITask\(\[\]\(\)\{K2040::Hub::RefreshNativeMcm\(\);\}\)/);
});
test('reserved modifier and numlock key values cannot be saved',()=>{
 for(const key of ['VK_SHIFT','VK_CONTROL','VK_MENU','VK_LSHIFT',
  'VK_RSHIFT','VK_LCONTROL','VK_RCONTROL','VK_LMENU','VK_RMENU',
  'VK_NUMLOCK','VK_ESCAPE','VK_F9'])assert.match(hub,new RegExp('key!='+key));
});

test('retired settings capture no longer polls mouse buttons or registers browser writers',()=>{
 const ui=fs.readFileSync('view/app.js','utf8');
 assert.doesNotMatch(main,/hubNativeMouseCapture|oldMouse4|receiveHubNativeMouseCapture/);
 assert.doesNotMatch(main,/BindUIEvent\(view, "saveHubKeyboard"/);
 assert.doesNotMatch(main,/BindUIEvent\(view, "saveHubController"/);
 assert.doesNotMatch(ui,/saveHubKeyboard|setHubNativeCapture|captureButton/);
 assert.match(main,/K2040::Hub::RefreshNativeMcm/);
});

test('native MCM refuses stale disk registry when live bridge has not initialized',()=>{
 assert.match(hub,/Until native MCM's PauseMenu bridge is live/);
 assert.match(hub,/k.registered=std::filesystem::is_regular_file/);
 assert.match(hub,/if\(live.installed\)return live;/);
});
test('unambiguous stale zero duplicate recovered only after stable observations',()=>{
 assert.match(live,/const bool stable=priorLive && \*priorLive==\*live/);
 assert.match(live,/if\(stable\)/);
 assert.match(live,/own.size\(\)!=2/);
 assert.match(live,/rec.key==0 && rec.mods==0/);
 assert.match(live,/if\(\*after!=expected/);
 assert.match(live,/QueryNative\(movie,i\)/);
 assert.match(live,/InvokeClear\(movie,0,0\)/);
 assert.match(live,/if\(stale==expected.end\(\)\)break;/);
 assert.match(live,/priorLive.reset\(\)/);
});

test('Quest and tracker shortcut assignments remain native MCM gameplay controls',()=>{
 const grouped=JSON.parse(fs.readFileSync('mcm/Data/MCM/Config/K2040_Quest_Journal/config.json','utf8'));
 const controls=grouped.pages.flatMap(p=>p.content);
 const keyboard=controls.filter(c=>c.type==='hotkey').map(c=>c.id);
 assert.deepEqual(keyboard,['questJournalToggle','questTrackerToggle']);
 assert.doesNotMatch(fs.readFileSync('view/journal.html','utf8'),/data-hub-reset|standaloneHotkeys/);
 assert.match(hub,/StageNativeMcmBinding/);
});

test('controller gameplay assignments remain in the owning MCM config',()=>{
 const grouped=JSON.parse(fs.readFileSync('mcm/Data/MCM/Config/K2040_Quest_Journal/config.json','utf8'));
 const controls=grouped.pages.flatMap(p=>p.content);
 for(const id of ['iQuestJournalPadPrimary:Controller','iQuestJournalPadModifier:Controller',
   'iQuestTrackerPadPrimary:Controller','iQuestTrackerPadModifier:Controller'])
   assert.ok(controls.some(c=>c.id===id),id);
 assert.doesNotMatch(fs.readFileSync('view/journal.html','utf8'),/hubJournalPrimary|hubTrackerHeld/);
});
