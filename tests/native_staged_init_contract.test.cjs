'use strict';
const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');

const src=fs.readFileSync('research/native_staged_init/NativeStagedInit.cpp','utf8');
const ini=fs.readFileSync('research/native_staged_init/K2040_Quest_Staged_Init.ini','utf8');
const build=fs.readFileSync('scripts/build-native-staged-init.sh','utf8');
const target=fs.readFileSync('research/native_staged_init/xmake.lua','utf8');
const production=fs.readFileSync('xmake.lua','utf8');
const main=fs.readFileSync('src/main.cpp','utf8');

test('separate OG F4SE diagnostic only, not part of production Quest Journal',()=>{
 assert.match(src,/RUNTIME_1_10_163/g);
 assert.match(src,/F4SEPlugin_Query/);
 assert.match(src,/F4SE_PLUGIN_LOAD/);
 assert.match(src,/\.hook = false/);
 assert.match(src,/K2040_Quest_Staged_Init/g);
 assert.match(target,/set_filename\("K2040_Quest_Staged_Init\.dll"\)/);
 assert.match(target,/add_files\("NativeStagedInit\.cpp"\)/);
 assert.match(build,/K2040_STAGED_INIT_STAGE/);
 assert.match(build,/-P "\$proj"/);
 assert.doesNotMatch(production,/NativeStagedInit|native_staged_init/);
 assert.doesNotMatch(main,/NativeStagedInit|native_staged_init/);
});
test('only event listener stage enabled in default package; task stages are opt-in',()=>{
 assert.match(ini,/EnableEventSink=YES/);
 assert.match(ini,/EnableOneShotTask=NO/);
 assert.match(ini,/EnablePermanentTask=NO/);
 assert.match(src,/enabled\(L"EnableEventSink"\)/);
 assert.match(src,/enabled\(L"EnableOneShotTask"\)/);
 assert.match(src,/enabled\(L"EnablePermanentTask"\)/);
 assert.match(src,/if \(modes > 1\)/);
 assert.match(src,/selected = \{\};/);
 assert.match(src,/GD3 INVALID multiple modes, no actions/);
});
test('event registration stage only reads event type, never dereferences quest',()=>{
 assert.match(src,/selected.eventSink/);
 assert.match(src,/RegisterSink\(&passiveSink\)/);
 assert.match(src,/kUpdateQuestActiveStatus|EventSource = RE::BSTEventSource<Event>/);
 assert.match(src,/kGetterId = 476335u/);
 assert.match(src,/getter.address\(\)/);
 assert.match(src,/VerifyMappedOg\(\)/);
 assert.match(src,/kMaxEventLog = 8u/);
 assert.doesNotMatch(src,/event\.quest->/);
 assert.match(src,/BSEventNotifyControl::kContinue/);
});
test('task stages only schedule after load and have no gameplay work',()=>{
 assert.match(src,/msg->type == M::kPostLoadGame/);
 assert.match(src,/taskQueued.exchange\(true\)/);
 assert.match(src,/tasks->AddTask\(\[\] \{/);
 assert.match(src,/GD3 UNSUPPORTED OG F4SE 0\.6\.23 permanent task; no actions/);
 assert.match(src,/if \(selected\.taskPermanent\)/);
 assert.doesNotMatch(src,/tasks->AddTaskPermanent\(/);
 assert.doesNotMatch(src,/PermanentNoop/);
 assert.doesNotMatch(src,/callbackCount.fetch_add/);
 assert.match(src,/task interface unavailable; no scheduling/);
 assert.match(src,/GD4 task test selected; deferring to post-load/);
});
test('no tracking writes, memory hooks, window or keyboard processing',()=>{
 assert.doesNotMatch(src,/\b(?:setter\s*\(|SetActive\s*\(quest|SetStage\s*\(|AddUITask\s*\(|WriteProcessMemory\s*\(|VirtualProtect\s*\(|CreateThread\s*\(|GetAsyncKeyState\s*\(|GetForegroundWindow\s*\()\b/);
 assert.doesNotMatch(src,/\.quest->|GetFormByID|questData|PipboyDataManager/);
 assert.doesNotMatch(src,/\bREL::ID\(1567562\)/);
 assert.doesNotMatch(src,/\.flags\s*(?:=|\|=|&=)/);
});
