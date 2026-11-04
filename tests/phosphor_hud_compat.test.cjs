"use strict";
const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const native = fs.readFileSync('src/main.cpp', 'utf8');
const html = fs.readFileSync('view/journal.html', 'utf8');
const policy = fs.readFileSync('src/TrackerCompatibility.h', 'utf8');

test('recognizes only loaded Phosphor plugin and its live HUD switch', () => {
  assert.match(native, /GetModuleHandleW\(L"PhosphorHUD\.dll"\)/);
  assert.match(native, /GetFullPathNameW\([\s\S]*?PhosphorHUD\.ini/);
  assert.match(native, /GetPrivateProfileIntW\(L"Main", L"bLiveHud", 1,/);
  assert.match(native, /iniPath\.data\(\)/);
  assert.match(native, /PhosphorHUD\.ini/);
  assert.match(native, /static const bool live = \[\]\(\) \{/);
  assert.doesNotMatch(native, /LoadLibraryW\(L"PhosphorHUD\.dll"\)/);
  assert.match(policy, /return userEnabled && \(!conflictingHudActive \|\| allowWithConflictingHud\)/);
});

test('Phosphor owns only our tracker surface, not Journal or settings', () => {
  assert.match(native, /void CreateViews\(\) \{[\s\S]*?if \(PhosphorHudActive\(\) && !K2040::Settings::Current\(\)\.trackerOverridePhosphor\) \{[\s\S]*?return;[\s\S]*?trackerView = views\.CreateView/);
  assert.match(native, /void CreateJournalView\(\) \{[\s\S]*?views\.CreateView\("K2040QuestJournal\/journal\.html"/);
  assert.match(native, /void OpenJournal\(\) \{[\s\S]*?CreateJournalView\(\)/);
  assert.match(native, /if \(TrackerMayShow\(\)\) views\.Show\(view\)/);
  assert.match(native, /if\(TrackerMayShow\(\)\)views\.Show\(trackerView\)/);
  assert.match(native, /if \(TrackerMayShow\(\) && trackerDomReady && Valid\(trackerView\)/);
});

test('auto suppression never overwrites the user preference or toggles it via the hotkey', () => {
  const toggle = native.slice(native.indexOf('    void ToggleTracker() {'), native.indexOf('    void UpdateHubBinding('));
  assert.match(toggle, /if \(PhosphorHudActive\(\) && !K2040::Settings::Current\(\)\.trackerOverridePhosphor\) \{[\s\S]*?return;[\s\S]*?const bool desired=!trackerEnabled/);
  assert.match(toggle, /Settings::ChangeAndSave\(desired\?/);
  assert.match(toggle, /TrackerMayShow\(\) && !trackerView\)CreateViews\(\)/);
  assert.match(html, /data-setting="trackerOverridePhosphor" type="checkbox"/);
  assert.match(html, /If Phosphor still shows its own tracker, both may appear/);
  assert.match(native, /TrackerMayShow\(\) && trackerDomReady\)SyncSnapshots\(\)/);
  assert.match(native, /TrackerMayShow\(\) && trackerDomReady\)SyncSettings\(\)/);
});

test('compatibility toggle is persisted and reaches the view and native policy', () => {
  const settings = fs.readFileSync('src/JournalSettings.cpp', 'utf8');
  const header = fs.readFileSync('src/JournalSettings.h', 'utf8');
  const js = fs.readFileSync('view/journal-settings.js', 'utf8');
  const ui = fs.readFileSync('view/app.js', 'utf8');
  const S = require('../view/journal-settings.js');
  assert.equal(S.defaults.trackerOverridePhosphor, false);
  assert.equal(S.normalize({}).trackerOverridePhosphor, false);
  assert.equal(S.normalize({trackerOverridePhosphor:true}).trackerOverridePhosphor, true);
  assert.equal(S.normalize({trackerOverridePhosphor:'true'}).trackerOverridePhosphor, false);
  assert.equal(S.normalize({trackerOverridePhosphor:false}).trackerOverridePhosphor, false);
  assert.match(header, /bool trackerOverridePhosphor = false/);
  assert.match(settings, /key=="trackerEnabled" \|\| key=="trackerOverridePhosphor"/);
  assert.match(settings, /"\\ntrackerOverridePhosphor="/);
  assert.match(js, /trackerOverridePhosphor:v\.trackerOverridePhosphor === true/);
  assert.match(ui, /trackerOverridePhosphor:'settingTrackerOverridePhosphor'/);
  const applySetting = native.slice(native.indexOf('    void UpdateSetting('), native.indexOf('    void ResetAppearance()'));
  const resetAppearance = native.slice(native.indexOf('    void ResetAppearance()'), native.indexOf('    void UpdateQuestFlag('));
  assert.match(applySetting, /TrackerMayShow\(\) && !trackerView\)CreateViews\(\)/);
  assert.match(resetAppearance, /TrackerMayShow\(\) && !trackerView\)CreateViews\(\)/);
  assert.match(native, /trackerEnabled, PhosphorHudActive\(\),[\s\S]*?Current\(\)\.trackerOverridePhosphor/);
  assert.match(native, /if\(TrackerMayShow\(\)\)views\.Show\(trackerView\)/);
});
