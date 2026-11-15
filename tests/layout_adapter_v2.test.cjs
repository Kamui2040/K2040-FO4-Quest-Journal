'use strict';
const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const crypto=require('node:crypto');
const hash=file=>crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');
const read=file=>fs.readFileSync(file,'utf8');
const native=read('src/main.cpp');
const settings=read('src/JournalSettings.cpp');
const ui=read('view/app.js');
test('owner exports the exact hub ABI and guards the game-thread preview lifecycle',()=>{
  assert.equal(hash('src/K2040LayoutAdapterV2.h'),
    '6067e572100a9c232d5bed7408080430bcff330a50918405f2a447170fa468dd');
  assert.match(native,/K2040_GetLayoutAdapterV2\(\) noexcept/);
  assert.match(native,/"quest-journal",TrackerLayoutAvailable,TrackerLayoutElements,/);
  assert.match(native,/TrackerLayoutBegin,TrackerLayoutPreview,TrackerLayoutRetry,/);
  assert.match(native,/TrackerLayoutFinish/);
  assert.match(native,/gameLoaded && apiReady && TrackerMayShow\(\)/);
  assert.match(native,/trackerDomReady && Valid\(trackerView\)/);
  assert.match(native,/K2040::TrackerLayout::Proportional\(trackerEditBaseline,next\)/);
  assert.match(native,/std::string_view\(element\)!="quest-tracker"/);
  assert.match(native,/ResetTrackerLayoutSession\(\);\s*uiEpoch.Invalidate\(\)/);
});
test('Save is atomic and Abort and Retry never persist owner or savegame data',()=>{
  assert.match(native,/if\(commit\) \{/);
  assert.match(native,/SaveTrackerLayout\(x,y,width,height\)/);
  assert.match(native,/trackerEditDraft=trackerEditBaseline;/);
  assert.match(native,/PushTrackerGeometry\(trackerEditBaseline\)/);
  assert.match(native,/PushTrackerGeometry\(std::nullopt\)/);
  assert.match(settings,/const Preferences before=preferences;\s*preferences.trackerLayoutEnabled=true;/);
  assert.match(settings,/preferences=before;\s*lastFailureReason="storage"/);
  assert.match(settings,/if\(key=="trackerAnchor"\|\|key=="trackerWidth"\)/);
  assert.match(settings,/!K2040::TrackerLayout::ValidBasis\(preferences.trackerLayoutX,/);
  assert.doesNotMatch(native,/K2040_GetLayoutAdapterV1/);
});
test('tracker renderer uses the same saved schema and native-only preview surface',()=>{
  assert.match(ui,/window.applyJournalTrackerLayout=function\(input\)/);
  assert.match(ui,/state.layoutPreview=\{x:next.x,y:next.y,width:next.width,height:next.height\}/);
  assert.match(ui,/tracker.style.width=\(edit.width\*100\)/);
  assert.match(ui,/tracker.style.height=\(edit.height\*100\)/);
  assert.match(ui,/document.documentElement.style.fontSize=size/);
  assert.match(ui,/emit\('reportTrackerLayoutMetrics',JSON.stringify\(geometry\)\)/);
  assert.match(native,/BindUIEvent\(view, "reportTrackerLayoutMetrics"/);
  const manifest=JSON.parse(read('view/settings-v1-manifest.json'));
  const fields=manifest.categories.flatMap(c=>c.groups.flatMap(g=>g.fields));
  for(const k of ['trackerLayoutEnabled','trackerLayoutX','trackerLayoutY',
                   'trackerLayoutWidth','trackerLayoutHeight']){
    assert.ok(manifest.internalKeys.includes(k),k);
    assert.ok(fields.every(f=>f.id!==k),k);
  }
  assert.ok(fields.every(f=>f.id!=='trackerSyncPipboy'));
});
test('combined QA package requires the layout ABI and exported v2 getter',()=>{
  const source=read('scripts/package-unified-vnext-qa.py');
  assert.match(source,/native layout V2 ABI header drift/);
  assert.match(source,/K2040_GetLayoutAdapterV2/);
  assert.match(source,/K2040_Unified_Settings\.dll/);
});
