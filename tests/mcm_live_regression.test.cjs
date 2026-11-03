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

test('mouse side-buttons use native XBUTTON high-bit sampler and Settings capture acknowledgment',()=>{
 assert.match(main,/const bool mouse4=down\(VK_XBUTTON1\),mouse5=down\(VK_XBUTTON2\)/);
 assert.match(main,/const unsigned physical=mouse4&&!oldMouse4\?259u:260u/);
 assert.match(main,/setHubNativeCapture/);
 assert.match(main,/hubNativeMouseCapture.exchange\(0,std::memory_order_acq_rel\)/);
 assert.match(main,/receiveHubNativeMouseCapture/);
 const ui=fs.readFileSync('view/app.js','utf8');
 assert.match(ui,/window.receiveHubNativeMouseCapture/);
 assert.match(ui,/emit\('setHubNativeCapture',capturingHub\)/);
 assert.match(ui,/emit\('setHubNativeCapture',''\)/);
 assert.match(ui,/pending-mcm/);
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

test('explicit Controls reset routes each default through this mod native writer',()=>{
 const ui=fs.readFileSync('view/app.js','utf8');
 const html=fs.readFileSync('view/journal.html','utf8');
 assert.match(ui,/data-hub-reset/);
 assert.match(ui,/action,first:71,second:action==='journal'\?1:0/);
 assert.match(ui,/emit\('saveHubKeyboard'/);
 assert.match(html,/data-hub-reset="journal">Reset Shift\+G/);
 assert.match(html,/data-hub-reset="tracker">Reset G/);
});
