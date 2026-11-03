'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const source = fs.readFileSync('research/native_writer/UnwiredNativeQuestCall.cpp','utf8');
const preflight = fs.readFileSync('research/native_writer/TrackingPreflight.h','utf8');
const main = fs.readFileSync('src/main.cpp','utf8');
const xmake = fs.readFileSync('xmake.lua','utf8');
const policy = fs.readFileSync('src/QuestTrackingPolicy.h','utf8');
const tests = fs.readFileSync('scripts/verify-source.sh','utf8');

test('native tracking engine evidence is review-only, no executable call',()=>{
  assert.match(source,/kOGSetActiveRelocationID = 1567562/);
  assert.match(source,/kOGGetActiveRelocationID = 690102/);
  assert.match(source,/offsetof\(RE::TESQuest, data\) == 0xF0/);
  assert.match(source,/offsetof\(RE::QUEST_DATA, flags\) == 0x4/);
  assert.match(source,/RE::TESQuestEvent::kUpdateQuestActiveStatus == 0/);
  assert.doesNotMatch(source,/\b(?:DispatchMethodCall|SetActive\s*\(|REL::Relocation\s*<|VirtualProtect|WriteProcessMemory)\b/);
});

test('all native writer inputs are required, and result demands Pip-Boy confirmation',()=>{
  for (const gate of [
    'runtimeIsOG163','gameLoaded','gameThread','journalOpenAndFocused',
    'formIsQuest','inCurrentPipboyFeed','currentSaveEpoch','currentUiEpoch',
    'kQuestEnabled','kQuestCompleted','kQuestFailed','kMiscQuestType'
  ]) assert.ok(preflight.includes(gate),'missing gate: '+gate);
  assert.match(preflight,/observed.pipboyFeedTracked == request.desiredTracked/);
  assert.match(preflight,/observed.engineFlagTracked == request.desiredTracked/);
  assert.match(preflight,/Decision::kReadyForUnwiredReview/);
  assert.match(tests,/tests\/native_tracking_preflight.cpp/);
});

test('production binary and UI still have no connection to research writer',()=>{
  assert.doesNotMatch(main,/native_writer|TrackingPreflight|OgSetActiveSignature/);
  assert.doesNotMatch(xmake,/native_writer|UnwiredNativeQuestCall|TrackingPreflight/);
  assert.match(main,/static_assert\(!K2040::QuestTracking::CanWritePipboy\(\)/);
  assert.match(policy,/kVerifiedBackend\s*=\s*WriteBackend::kNone/);
  assert.match(fs.readFileSync('view/app.js','utf8'),/data-action="pipboy-track" disabled/);
});
