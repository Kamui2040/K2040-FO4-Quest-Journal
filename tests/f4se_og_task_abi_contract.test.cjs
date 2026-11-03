'use strict';
const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const abi=fs.readFileSync('research/f4se_og_api/TaskInterface0623.h','utf8');
const original=fs.readFileSync('research/native_write_test/QuestNativeWriteTest.cpp','utf8');
const oldBuilder=fs.readFileSync('scripts/build-native-quest-write-test.sh','utf8');
const staged=fs.readFileSync('research/native_staged_init/NativeStagedInit.cpp','utf8');
const stagedIni=fs.readFileSync('research/native_staged_init/K2040_Quest_Staged_Init.ini','utf8');

test('official release ABI model documents two callbacks despite interface version 2',()=>{
  assert.match(abi,/official F4SE tag v0\.6\.23/);
  assert.match(abi,/struct TaskInterface/);
  assert.match(abi,/void \(\*addTask\)\(void\*\)/);
  assert.match(abi,/void \(\*addUITask\)\(void\*\)/);
  assert.match(abi,/sizeof\(TaskInterface\) == 24/);
  assert.match(abi,/kSlotCount = 2/);
  assert.doesNotMatch(abi,/void \(\*addTaskPermanent\)/);
});

test('all buildable native sources must avoid unavailable AddTaskPermanent call',()=>{
  const roots=['src','research/native_probe','research/native_writer','research/native_event_observer','research/native_init_diag','research/native_staged_init'];
  const found=[];
  for(const dir of roots){
    if(!fs.existsSync(dir))continue;
    for(const name of fs.readdirSync(dir,{withFileTypes:true})){
      if(name.isFile() && /\.cpp$/.test(name.name)){
        const file=path.join(dir,name.name);
        const content=fs.readFileSync(file,'utf8');
        if(/(?:->|\.)AddTaskPermanent\s*\(/.test(content))found.push(file);
      }
    }
  }
  assert.deepEqual(found,[]);
  assert.doesNotMatch(staged,/AddTaskPermanent\s*\(/);
  assert.match(staged,/GD3 UNSUPPORTED OG F4SE 0\.6\.23 permanent task; no actions/);
  assert.match(stagedIni,/EnablePermanentTask=NO/);
});

test('crashing historical writer is unbuildable even with environment override',()=>{
  assert.match(original,/#error K2040_NATIVE_WRITER_QUARANTINED_OG_F4SE_0_6_23_TASK_ABI_MISMATCH/);
  assert.match(oldBuilder,/exit 2/);
  assert.match(oldBuilder,/BLOCKED: unsafe native writer/);
  assert.doesNotMatch(oldBuilder,/K2040_NATIVE_WRITE_CRASH_REVIEWED/);
});
