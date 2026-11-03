const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');

const main = fs.readFileSync('src/main.cpp', 'utf8');
const policy = fs.readFileSync('src/QuestTrackingPolicy.h', 'utf8');
const mailbox = fs.readFileSync('src/TrackingMailbox.h', 'utf8');

test('mailbox cannot invoke game/script machinery or directly mutate quests', () => {
  assert.doesNotMatch(mailbox, /#include\s*[<"](RE\/|F4SE\/|Prisma)/);
  assert.doesNotMatch(mailbox, /DispatchMethodCall|SetActive\s*\(|SetStage\s*\(|QuestFlag/);
  assert.match(mailbox, /Reset\(std::uint64_t session\)/);
  assert.match(mailbox, /IsCurrent\(std::uint64_t session, std::uint32_t ticket\)/);
  assert.match(mailbox, /TrackingResult Settle/);
});

test('research mailbox has no active entry point in current Fallout 4 plugin', () => {
  assert.doesNotMatch(main, /TrackingMailbox|#include\s*"TrackingMailbox\.h"/);
  assert.doesNotMatch(main, /GetPapyrusInterface\(\)|DispatchMethodCall/);
  assert.match(policy, /kVerifiedBackend\s*=\s*WriteBackend::kNone/);
  assert.match(main, /static_assert\(!K2040::QuestTracking::CanWritePipboy\(\)/);
});
