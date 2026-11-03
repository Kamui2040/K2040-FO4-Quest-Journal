const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');

const diagnostic = fs.readFileSync('research/native_probe/QuestSetActiveProbe.cpp', 'utf8');
const project = fs.readFileSync('research/native_probe/xmake.lua', 'utf8');
const buildScript = fs.readFileSync('scripts/build-native-quest-probe.sh', 'utf8');
const productionProject = fs.readFileSync('xmake.lua', 'utf8');
const productionMain = fs.readFileSync('src/main.cpp', 'utf8');
const writePolicy = fs.readFileSync('src/QuestTrackingPolicy.h', 'utf8');

test('read-only diagnostic samples OG quest functions and guarded follow-on code', () => {
  assert.match(diagnostic, /kGetActiveRva\s*=\s*0x005D6AA0u/);
  assert.match(diagnostic, /kSetActiveRva\s*=\s*0x005D6AC0u/);
  assert.match(diagnostic, /kSampleBytes\s*=\s*128/);
  assert.match(diagnostic, /kSetActiveFirstCallOffset\s*=\s*0x2Fu/);
  assert.match(diagnostic, /entry\[kSetActiveFirstCallOffset\] == 0xE8u/);
  assert.match(diagnostic, /Report\("TESQuest::SetActive follow-on call destination"/);
  assert.match(diagnostic, /RuntimeVersion\(\) != F4SE::RUNTIME_1_10_163/g);
  assert.match(diagnostic, /kGameDataReady/);
  assert.match(diagnostic, /VirtualQuery/);
  assert.match(diagnostic, /PAGE_GUARD \| PAGE_NOACCESS/);
  assert.match(diagnostic, /GetModuleHandleW\(nullptr\)/);
  assert.match(diagnostic, /std::memcpy\(out.data\(\), reinterpret_cast<const void\*>\(ptr\), out.size\(\)\)/);
});

test('diagnostic has no game write calls, hooks, or quest pointer manipulation', () => {
  assert.match(diagnostic, /\.hook = false/);
  assert.match(diagnostic, /\.log = true/);
  assert.match(diagnostic, /\.logName = "K2040_Quest_SetActive_Probe"/);
  assert.match(diagnostic, /REX::INFO\("K2040_QuestProbe: READ-ONLY byte samples at GameDataReady"\)/);
  assert.doesNotMatch(diagnostic, /\b(?:WriteProcessMemory|VirtualProtect|VirtualAlloc|CreateRemoteThread|SetWindowsHookEx|InstallHook|DispatchMethodCall|BindNativeMethod|SetActive\s*\()\b/);
  assert.doesNotMatch(diagnostic, /(?:RE::)?TESQuest\s*\*/);
  assert.doesNotMatch(diagnostic, /\bRegisterForExternalEvent\b/);
});

test('diagnostic is built independently and never deployed by project rules', () => {
  assert.match(project, /set_filename\("K2040_Quest_SetActive_Probe\.dll"\)/);
  assert.match(project, /set_targetdir\(out\)/);
  assert.match(buildScript, /K2040_PROBE_STAGE/);
  assert.match(buildScript, /-P "\$proj"/);
  assert.match(buildScript, /NOT installed, deployed, loaded, or validated/);
  assert.doesNotMatch(productionProject, /QuestSetActiveProbe|native_probe|build-native-quest-probe/);
  assert.doesNotMatch(productionMain, /QuestSetActiveProbe|ReadOnlySample|kSetActiveRva/);
  assert.match(writePolicy, /kVerifiedBackend\s*=\s*WriteBackend::kNone/);
});
