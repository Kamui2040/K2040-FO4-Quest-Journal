'use strict';
const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const source=fs.readFileSync('src/QuestTrackingPolicy.h','utf8');
const main=fs.readFileSync('src/main.cpp','utf8');
const ui=fs.readFileSync('view/app.js','utf8');

test('tracked quest writes require a separately verified backend',()=>{
  assert.match(source,/kVerifiedBackend\s*=\s*WriteBackend::kNone/);
  assert.match(source,/static_assert\(!CanWritePipboy\(\)/);
  assert.match(main,/static_assert\(!K2040::QuestTracking::CanWritePipboy\(\)/);
  assert.match(main,/QuestTracking::ParseRequest\(argument\)/);
  assert.match(main,/QuestTracking::kDisabledStatus/);
  assert.doesNotMatch(main,/DispatchMethodCall\(|DispatchTrackingPapyrus|ToggleQuestActiveStatusCommand/);
  assert.match(ui,/data-action="pipboy-track" disabled/);
});

test('cached requests use a strictly validated 8-digit FormID and one tracking digit',()=>{
  assert.match(source,/input\.size\(\) != 10 \|\| input\[8\] != ':'/);
  assert.match(source,/input\[9\] != '0' && input\[9\] != '1'/);
  assert.match(source,/if \(!formId\) return std::nullopt/);
  assert.match(source,/static_assert\(!ParseRequest\("00000000:1"\)/);
  assert.match(source,/static_assert\(!ParseRequest\("00000001:9"\)/);
  assert.match(source,/static_assert\(!ParseRequest\("not-a-quest"\)/);
});

test('old Pip-Boy command RTTI cannot authorize a native write',()=>{
  assert.match(source,/kVerifiedBackend\s*=\s*WriteBackend::kNone/);
  assert.doesNotMatch(main,/DispatchMethodCall\(|ToggleQuestActiveStatusCommand\(/);
  const build=fs.readFileSync('xmake.lua','utf8');
  assert.match(build,/option\("quest_questwide_normal_tracking"\)/);
  assert.match(build,/set_default\(false\)/);
  assert.match(build,/QuestWideProductionCandidate\.cpp/);
});
