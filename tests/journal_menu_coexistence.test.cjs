'use strict';
const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const cpp=fs.readFileSync('src/main.cpp','utf8');
const compatibility=fs.readFileSync('src/TrackerCompatibility.h','utf8');

test('only vanilla blocking menus hide the owner tracker; hub layout editing remains available',()=>{
  const match=cpp.match(/bool TrackerMayShowInGameplay\(\)\s*\{([^}]*)\}/);
  assert.ok(match,'tracker menu guard is required');
  assert.match(match[1],/gameLoaded && TrackerMayShow\(\) && !CompetingGameMenuOpen\(\)/);
  assert.doesNotMatch(match[1],/AnotherPrismaViewFocused/,'hub must be able to edit visible tracker');
  for(const name of ['PipboyMenu','PauseMenu','Console','DialogueMenu'])
    assert.match(cpp,new RegExp('menu!="'+name+'"'),'missing '+name+' transition');
  assert.match(cpp,/ui->RegisterSink<RE::MenuOpenCloseEvent>\(&trackerMenuWatch\)/);
  assert.match(cpp,/RegisterTrackerMenuWatchOnce\(\);/);
});

test('menu opens hide the tracker immediately; closes restore it only after a fresh menu check',()=>{
  const observer=cpp.split('class TrackerMenuWatch final')[1]?.split('TrackerMenuWatch trackerMenuWatch;')[0];
  assert.ok(observer);
  assert.match(observer,/if\(event.opening\)\s*\{[\s\S]*?views.Hide\(trackerView\)/);
  assert.match(observer,/if\(journalOpen\)CloseJournal\(\)/);
  assert.match(observer,/tasks->AddTask\(\[\]\(\)\{UpdateTrackerVisibility\(\);\}\)/);
  assert.match(cpp,/if\(TrackerMayShowInGameplay\(\) && trackerDomReady\)views.Show\(trackerView\)/);
  assert.match(cpp,/else views.Hide\(trackerView\)/);
});

test('settings changes and DOM-ready also use the same guarded tracker visibility',()=>{
  assert.ok((cpp.match(/UpdateTrackerVisibility\(\);/g)||[]).length>=5);
  assert.match(cpp,/trackerDomReady=true;\s*UpdateTrackerVisibility\(\);/);
  assert.match(cpp,/if\(TrackerMayShow\(\)\)SyncSnapshots\(\);/);
  assert.match(cpp,/ResetTrackerLayoutSession\(\);\s*uiEpoch.Invalidate\(\)/);
  assert.match(compatibility,/ShowMiniTracker/,'Phosphor coexistence must remain owner-controlled');
  assert.doesNotMatch(cpp,/K2040_HudResetLayoutDefaultsV1|K2040_HudLayoutProfilesV1/,
    'HUD-only layout exports do not belong to Quest Journal');
});
