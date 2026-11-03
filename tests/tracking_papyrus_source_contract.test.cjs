const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');

const source = (path) => fs.readFileSync(path, 'utf8');
const iface = source('research/papyrus/K2040QuestJournalNative.psc');
const worker = source('research/papyrus/K2040QuestJournalWorker.psc');
const stubs = source('research/native/TrackingPapyrusStub.cpp');
const main = source('src/main.cpp');
const build = source('xmake.lua');
const policy = source('src/QuestTrackingPolicy.h');
const wire = source('src/TrackingPapyrusWire.h');
const mailbox = source('src/TrackingMailbox.h');

test('Papyrus native declarations exactly match compile-only bindings', () => {
  const methods = [...iface.matchAll(/(?:Bool|Int\[\]) Function ([A-Za-z]+)\([^)]*\) Global Native/g)]
      .map(m => m[1]).sort();
  assert.deepEqual(methods, ['Cancel', 'Claim', 'Complete', 'IsCurrent', 'IsEnabled']);
  for (const name of methods) {
    assert.match(stubs, new RegExp('"' + name + '"sv'));
  }
  assert.match(stubs, /bool IsEnabled\(std::monostate\) \{ return false; \}/);
  assert.match(stubs, /std::vector<std::int32_t> Claim\(std::monostate\) \{ return \{\}; \}/);
});

test('worker has one guarded quest tracking write, no progression mutations', () => {
  assert.match(worker, /Scriptname K2040QuestJournalWorker extends Quest/);
  assert.match(worker, /K2040QuestJournalNative\.IsEnabled\(\)/);
  assert.match(worker, /K2040QuestJournalNative\.IsCurrent\(ticket\)/);
  assert.match(worker, /Game\.GetForm\(formID\) as Quest/);
  assert.match(worker, /target\.IsRunning\(\)/);
  assert.match(worker, /target\.IsCompleted\(\)/);
  assert.equal((worker.match(/target\.SetActive\(/g) || []).length, 1);
  assert.match(worker, /K2040QuestJournalNative\.Complete\(ticket, target\.IsActive\(\)\)/);
  assert.doesNotMatch(worker, /\.(?:Start|Stop|SetStage|CompleteQuest|SetObjectiveDisplayed)\(/);
});

test('research script and native bindings are not part of shipping F4SE plugin', () => {
  assert.match(build, /add_files\("src\/\*\*\.cpp"\)/);
  // The only allowed research compile input is guarded by the explicit,
  // default-OFF QA configuration. Shipping never compiles Papyrus stubs.
  const qa = build.match(/if has_config\("quest_dryrun_qa"\) then([\s\S]*?set_targetdir\([^\n]+\))\s+end/);
  assert.ok(qa, 'opt-in QA file list must have its own guarded block');
  assert.match(qa[1], /research\/native_one_shot\/OneShotOgDryRun\.cpp/);
  // The separate GFx observer is also opt-in and explicitly
  // incompatible with all quest write QA flavors and dryrun.
  const gfx = build.match(/if has_config\("quest_pipboy_selection_readonly_qa"\) then([\s\S]*?set_targetdir\([^\n]+\))\s+end/);
  assert.ok(gfx, 'the selectedEntry research adapter must have its own guarded block');
  assert.match(gfx[1], /add_files\("research\/native_pipboy_selection\/UnwiredGFxAdapter\.cpp"\)/);
  assert.match(gfx[1], /quest_native_write_qa/);
  assert.match(gfx[1], /quest_dryrun_qa/);
  const candidate=build.match(/if has_config\("quest_questwide_production_candidate"\) or\s+has_config\("quest_questwide_normal_tracking"\) then([\s\S]*?set_targetdir\([^\n]+\))\s+end/);
  assert.ok(candidate, 'opt-in R2 writer must be guarded by the candidate or normal flavor');
  assert.match(candidate[1], /research\/production_candidate\/QuestWideProductionCandidate\.cpp/);
  assert.doesNotMatch(build.replace(qa[0], '').replace(gfx[0], '').replace(candidate[0], ''),
      /add_files\("research\//);
  assert.doesNotMatch(build, /research\/papyrus|research\/native\/TrackingPapyrusStub/);
  assert.doesNotMatch(main, /TrackingPapyrusWire|TrackingPapyrusStub|TrackingMailbox/);
  assert.match(main, /static_assert\(!K2040::QuestTracking::CanWritePipboy\(\)/);
  assert.match(policy, /kVerifiedBackend\s*=\s*WriteBackend::kNone/);
  for (const src of [iface,worker,stubs,wire,mailbox]) {
    assert.doesNotMatch(src, /\/home\/k2040|\/run\/media\/k2040/);
  }
});

test('wire format documents signed high-bit FormIDs and caps ticket range', () => {
  assert.match(wire, /std::bit_cast<std::int32_t>\(request.formId\)/);
  assert.match(wire, /std::bit_cast<std::uint32_t>\(signedId\)/);
  assert.match(mailbox, /std::numeric_limits<std::int32_t>::max\(\)/);
  assert.match(worker, /request.Length == 3/);
});
