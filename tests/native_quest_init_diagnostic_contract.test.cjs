'use strict';
const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const source=fs.readFileSync('research/native_init_diag/NativeInitDiagnostic.cpp','utf8');
const xmake=fs.readFileSync('research/native_init_diag/xmake.lua','utf8');
const builder=fs.readFileSync('scripts/build-native-quest-init-diagnostic.sh','utf8');
const main=fs.readFileSync('src/main.cpp','utf8');
const production=fs.readFileSync('xmake.lua','utf8');
const policy=fs.readFileSync('src/QuestTrackingPolicy.h','utf8');

test('startup-only build is standalone OG and F4SE-logged',()=>{
  assert.match(source,/RUNTIME_1_10_163/);
  assert.match(source,/K2040_Quest_Native_Init_Diag/g);
  assert.match(source,/kGameDataReady/);
  assert.match(source,/\.hook = false/);
  assert.match(source,/\.log = true/);
  assert.match(xmake,/set_filename\("K2040_Quest_Native_Init_Diag.dll"\)/);
  assert.match(xmake,/add_files\("NativeInitDiagnostic.cpp"\)/);
  assert.match(builder,/K2040_INIT_DIAG_STAGE/);
  assert.match(builder,/-P "\$proj"/);
});

test('checkpoints precede Windows INI and native ABI reads',()=>{
  for(let n=0;n<=10;n++){
    assert.match(source,new RegExp('checkpoint='+String(n).padStart(2,'0')));
  }
  assert.ok(source.indexOf('checkpoint=01')<source.indexOf('GetModuleFileNameW'));
  assert.ok(source.indexOf('checkpoint=03')<source.indexOf('GetPrivateProfileStringW'));
  assert.ok(source.indexOf('checkpoint=05')<source.indexOf('MatchesOgLoadedCode();'));
  assert.ok(source.indexOf('checkpoint=07')<source.indexOf('F4SE::GetTaskInterface()'));
  assert.ok(source.indexOf('checkpoint=09')<source.indexOf('REL::Relocation<Getter>'));
  assert.match(source,/NO EVENTS\/HOOKS\/TASKS\/WRITES|no events\/hooks\/tasks\/writes/i);
});

test('reproduces only non-mutating native writer preflight reads',()=>{
  assert.match(source,/GetPrivateProfileStringW\(L"Safety"/);
  assert.match(source,/PermitOneNativeTrackingWrite/);
  assert.match(source,/IHaveBackedUpSave/);
  assert.match(source,/REL::ID\(476335\)/);
  assert.match(source,/REL::ID\(1567562\)/);
  assert.match(source,/callTarget\(0x2Fu\)/);
  assert.match(source,/callTarget\(0x3Cu\)/);
  assert.match(source,/VirtualQuery\(/);
  assert.doesNotMatch(source,/\b(?:AddTaskPermanent|AddTask|AddUITask|RegisterSink|SetActive\s*\(|WriteProcessMemory|VirtualProtect|DispatchMethodCall|CreateThread|SetStage\s*\()\b/);
  assert.doesNotMatch(source,/\b(?:setter|setActive|sourceGetter)\s*\(/);
  assert.doesNotMatch(source,/quest->data.flags\s*(?:=|\|=|&=)/);
});

test('does not alter production plugin safety gate',()=>{
  assert.doesNotMatch(main,/NativeInitDiagnostic|native_init_diag/);
  assert.doesNotMatch(production,/NativeInitDiagnostic|native_init_diag/);
  assert.match(policy,/kVerifiedBackend\s*=\s*WriteBackend::kNone/);
  assert.match(main,/static_assert\(!K2040::QuestTracking::CanWritePipboy\(\)/);
});
