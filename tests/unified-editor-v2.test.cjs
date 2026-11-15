'use strict';
const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const crypto=require('node:crypto');

const load=path=>fs.readFileSync(path,'utf8');
const native=load('src/main.cpp');
const ui=load('view/app.js');
const html=load('view/journal.html');
const manifest=JSON.parse(load('view/settings-v1-manifest.json'));
const hash=path=>crypto.createHash('sha256').update(fs.readFileSync(path)).digest('hex');

test('V2 and theme-query native ABI remains byte-pinned to shared editor contract',()=>{
  assert.equal(hash('src/K2040SettingsAdapterV2.h'),
    'f70909115d0b6efa801b35cf977455b198b74a03bd3523ce16276135a348f062');
  assert.equal(hash('src/K2040ThemeClient.h'),
    '02d0fd800ce619f4427e5c5a261b3cedb87c31187a1dfc7f6cfaf8f857509908');
  assert.match(native,/K2040_GetSettingsAdapterV2\(\) noexcept/);
  assert.match(native,/"quest-journal",JournalEditorReady,JournalManifest,JournalValues,/);
  assert.match(native,/JournalApply,JournalReset,JournalTheme/);
});

test('V2 native owner reads packaged manifest and validates/saves native preferences',()=>{
  assert.match(native,/K2040QuestJournal"\/"settings-v1-manifest.json"/);
  assert.match(native,/K2040::Settings::Serialize\(\)/);
  assert.match(native,/JournalEditorReady\(\)\|\|!setting\|\|!jsonValue/);
  assert.match(native,/JournalEditorReady\(\)\|\|!field\|\|std::string_view\(field\)!="resetPreferences"/);
  assert.match(native,/K2040::Settings::ChangeAndSave\(std::string\(id\)/);
  assert.match(native,/K2040::Settings::ResetToDefaults\(\)/);
  assert.match(native,/if\(TrackerMayShow\(\)\&\&!trackerView\)CreateViews\(\)/);
  for(const key of ['opacity','fontSize','trackerSyncPipboy'])
    assert.ok(native.includes('"'+key+'"'),key);
});

test('V2 metadata excludes obsolete settings-only shortcuts and retains gameplay input',()=>{
  const fields=manifest.categories.flatMap(c=>c.groups.flatMap(g=>g.fields));
  assert.equal(fields.filter(f=>f.type==='action').length,1);
  assert.equal(fields.filter(f=>f.type!=='action').length,31);
  assert.deepEqual(manifest.internalKeys,['opacity','fontSize','trackerSyncPipboy']);
  assert.ok(fields.every(f=>!/(settingsHotkey|settingsShortcut|settingsToggle)/i.test(f.id)));
  assert.equal(fs.existsSync('mcm/Data/MCM/Config/K2040_Quest_Journal'),false);
  assert.doesNotMatch(native,/RegisterHubMcm|SeedDefaultKeyboardBindings|RefreshNativeMcm/);
  assert.match(native,/QueueInputAction\(i\?K2040::Hub::Action::Tracker:K2040::Hub::Action::Journal,false\)/);
  assert.doesNotMatch(native,/BindUIEvent\(view, "(?:updateJournalSetting|resetJournalAppearance|saveHubKeyboard|saveHubController)"/);
  assert.doesNotMatch(html,/id="openSettings"|id="settingsWorkspace"/);
  assert.doesNotMatch(ui,/function showSettings\(/);
});

test('global theme is inherited only as a transient overlay; native settings stay owner-owned',()=>{
  assert.match(native,/ThemeClient::Current\("quest-journal"\)/);
  assert.match(native,/legacy\.value_or\(""\)/);
  assert.match(native,/\?"native":gUnifiedAppearanceTheme\.c_str\(\)/);
  assert.match(ui,/state\.unifiedTheme=theme==='native'\?null:theme/);
  assert.match(ui,/const nativePalette=isTracker\?trackerPalette:S.journalPalette\(s\)/);
  assert.doesNotMatch(ui,/emit\('updateJournalSetting'/);
});

test('V2 manifest is staged for both Xmake and independent source ZIP packaging',()=>{
  assert.match(load('xmake.lua'),/os\.files\(path\.join\(os\.scriptdir\(\),"view","\*"\)\)/);
  assert.match(load('scripts/stage-test-package.py'),/"settings-v1-manifest.json"/);
  assert.match(native,/std::filesystem::path\("Data"\)\/"PrismaUI_F4"\/"views"/);
});
