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
  assert.match(policy, /return userEnabled && !conflictingHudActive/);
});

test('Phosphor owns only our tracker surface, not Journal or settings', () => {
  assert.match(native, /void CreateViews\(\) \{[\s\S]*?if \(PhosphorHudActive\(\)\) \{[\s\S]*?return;[\s\S]*?trackerView = views\.CreateView/);
  assert.match(native, /void CreateJournalView\(\) \{[\s\S]*?views\.CreateView\("K2040QuestJournal\/journal\.html"/);
  assert.match(native, /void OpenJournal\(\) \{[\s\S]*?CreateJournalView\(\)/);
  assert.match(native, /if \(TrackerMayShow\(\)\) views\.Show\(view\)/);
  assert.match(native, /if\(TrackerMayShow\(\)\)views\.Show\(trackerView\)/);
  assert.match(native, /if \(TrackerMayShow\(\) && trackerDomReady && Valid\(trackerView\)/);
});

test('auto suppression never overwrites the user preference or toggles it via the hotkey', () => {
  const toggle = native.slice(native.indexOf('    void ToggleTracker() {'), native.indexOf('    void UpdateHubBinding('));
  assert.match(toggle, /if \(PhosphorHudActive\(\)\) \{[\s\S]*?return;[\s\S]*?const bool desired=!trackerEnabled/);
  assert.match(toggle, /Settings::ChangeAndSave\(desired\?/);
  assert.match(html, /When Phosphor HUD is active, this mini tracker hides automatically/);
  assert.match(native, /TrackerMayShow\(\) && trackerDomReady\)SyncSnapshots\(\)/);
  assert.match(native, /TrackerMayShow\(\) && trackerDomReady\)SyncSettings\(\)/);
});
