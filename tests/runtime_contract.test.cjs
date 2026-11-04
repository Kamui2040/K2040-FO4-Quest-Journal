'use strict';
const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const s=fs.readFileSync('src/main.cpp','utf8');
test('OG query export and guarded load',()=>{
 assert.match(s,/F4SE_EXPORT bool F4SEAPI F4SEPlugin_Query/);
 assert.match(s,/if \(!load\) return false/);
 assert.match(s,/return messaging->RegisterListener\(Messages\)/);
});
test('cross-save and input ownership',()=>{
 assert.match(s,/bool hotkeysRegistered = false/);
 assert.match(s,/if \(!apiReady \|\| hotkeysRegistered\) return/);
 assert.match(s,/kPreLoadGame:[\s\S]*?DestroyViews\(\);[\s\S]*?QuestViewState::Clear\(\)/);
});
test('inactive views reject expensive native polling',()=>{
 assert.match(s,/ValidateJournalOwnership\(\);\s*if\(journalOpen && journalFocused\)SyncSnapshots\(\)/);
 assert.match(s,/if\(TrackerMayShow\(\) && trackerDomReady\)SyncSnapshots\(\)/);
});
test('uses the engine input registration API instead of mutating handlers directly',()=>{
 assert.match(s,/menus->RegisterHandler\(&Hotkeys::Get\(\)\)/);
 assert.doesNotMatch(s,/handlers\.push_back/);
});

test('quest collector matches CommonLibF4 TESFullName string_view return type',()=>{
 const c=fs.readFileSync('src/QuestCollector.cpp','utf8');
 const stub=fs.readFileSync('tests/stubs/RE/Fallout.h','utf8');
 assert.match(c,/std::string\(RE::TESFullName::GetFullName\(\*form\)\)/);
 assert.doesNotMatch(c,/const char\*\s+\w+\s*=\s*RE::TESFullName::GetFullName/);
 assert.match(stub,/static std::string_view GetFullName/);
});

test('appearance reset is one game-thread native operation rather than a series of UI mutations',()=>{
 const settings=fs.readFileSync('src/JournalSettings.cpp','utf8');
 const app=fs.readFileSync('view/app.js','utf8');
 assert.match(s,/interop.BindUIEvent\(view, "resetJournalAppearance"/);
 assert.match(settings,/bool ResetToDefaults\(\)/);
 assert.match(app,/emit\('resetJournalAppearance'\)/);
 assert.doesNotMatch(app,/for\(const \[key,value\] of Object.entries\(S.defaults\)\)/);
});

test('cursor fallback uses CommonLibF4 wrappers rather than hardcoded relocations',()=>{
 assert.match(s,/cursor->RegisterCursor\(\)/);
 assert.match(s,/cursor->UnregisterCursor\(\)/);
 assert.doesNotMatch(s,/REL::VariantID\(/);
});

test('native Prisma snapshot deduplication resets on view recreation',()=>{
 assert.match(s,/std::string lastPublishedJournal/);
 assert.match(s,/std::string lastPublishedTracker/);
 assert.match(s,/payload != lastPublishedJournal/);
 assert.match(s,/payload != lastPublishedTracker/);
 assert.match(s,/lastPublishedJournal\.clear\(\)/);
 assert.match(s,/lastPublishedTracker\.clear\(\)/);
});

test('native JSON dedup gates on independently ready Prisma views',()=>{
 assert.match(s,/bool trackerDomReady = false/);
 assert.match(s,/journalDomReady && Valid\(journalView\) && payload != lastPublishedJournal/);
 assert.match(s,/trackerDomReady && Valid\(trackerView\) && payload != lastPublishedTracker/);
 assert.match(s,/trackerDomReady=true/);
});

test('pin/favorite/hide mutation returns success for removal so UI refreshes',()=>{
 const state=fs.readFileSync('src/QuestViewState.cpp','utf8');
 assert.match(state,/if\(set\.erase\(id\)\)return true/);
 assert.match(s,/if \(!K2040::QuestViewState::Toggle\(kind, id\)\) return;[\s\S]{0,80}SyncSnapshots\(\)/);
});

test('Prisma DOM-ready callback only schedules game-thread initialization',()=>{
 assert.match(s,/void CompleteViewOnGameThread\(PrismaView view\)/);
 assert.match(s,/void OnDomReady\(PrismaView view\) \{[\s\S]*?tasks->AddTask\(\[view,generation\]\(\)\{/);
 assert.match(s,/if\(uiEpoch\.IsCurrent\(generation\)\)CompleteViewOnGameThread\(view\)/);
 const callback=s.slice(s.indexOf('    void OnDomReady(PrismaView view) {'),s.indexOf('    void DestroyViews()'));
 assert.doesNotMatch(callback,/InteropCall|BindUIEvent|views\.Focus|views\.Show|SyncSnapshots\(/);
});

test('global appearance updates roll back after failed file persistence',()=>{
 const settings=fs.readFileSync('src/JournalSettings.cpp','utf8');
 assert.match(settings,/bool ChangeAndSave\(std::string_view command\)/);
 assert.match(settings,/if\(Save\(\)\)\s*\{\s*lastFailureReason="none";\s*return true;\s*\}[\s\S]*?preferences=before/);
 assert.match(settings,/GetEnvironmentVariableW\(L"LOCALAPPDATA"/);
 assert.match(settings,/std::filesystem::path\(kLegacyPath\)/);
 assert.match(settings,/lastFailureReason="storage"/);
 assert.match(settings,/lastFailureReason="invalid"/);
 assert.match(s,/Settings::LastFailureReason\(\)/);
 assert.match(s,/if\(!K2040::Settings::ChangeAndSave\(desired\?/);
 assert.match(s,/const bool saved=K2040::Settings::ChangeAndSave\(setting\)/);
});

test('Prisma UI callbacks schedule native work and copy borrowed payloads',()=>{
 assert.match(s,/void QueueUIAction\(std::function<void\(\)> work\)/);
 assert.match(s,/void QueueUIString\(void \(\*action\)\(const char\*\), const char\* raw\)/);
 assert.match(s,/const std::string value\(raw\)/);
 assert.match(s,/QueueUIAction\(\[action,value\]\(\) \{ action\(value\.c_str\(\)\); \}\)/);
 assert.match(s,/QueueUIString\(UpdateSetting,cmd\)/);
 assert.match(s,/QueueUIString\(UpdateCategory,choice\)/);
 assert.match(s,/QueueUIAction\(\[\]\(\)\{if\(journalOpen && journalFocused\)CloseJournal\(\);\}\)/);
 assert.match(s,/QueueTrackerAction\(\[\]\(\)\{if\(TrackerMayShow\(\) && trackerDomReady\)SyncSnapshots\(\);\}\)/);
});

test('queued Prisma commands are rejected after a view/save transition',()=>{
 const epoch=fs.readFileSync('src/UILifecycleEpoch.h','utf8');
 assert.match(epoch,/std::atomic<std::uint64_t> epoch_/);
 assert.match(epoch,/void Invalidate\(\) noexcept/);
 assert.match(s,/const auto generation=uiEpoch\.Capture\(\)/);
 assert.match(s,/if\(uiEpoch\.IsCurrent\(generation\)\)work\(\)/);
 assert.match(s,/if\(uiEpoch\.IsCurrent\(generation\)\)CompleteViewOnGameThread\(view\)/);
 assert.match(s,/void DestroyViews\(\) \{[\s\S]*?uiEpoch\.Invalidate\(\)/);
 assert.match(s,/void CloseJournal\(\) \{[\s\S]*?uiEpoch\.Invalidate\(\)/);
});

test('asynchronous Prisma view creation does not abort journal before DOM-ready',()=>{
 assert.match(s,/if\(!apiReady \|\| journalView\)return;/);
 assert.match(s,/CreateJournalView\(\);[\s\S]{0,500}if\(!journalView\) \{/);
 assert.doesNotMatch(s,/CreateJournalView\(\);[\s\S]{0,150}if\(!Valid\(journalView\)\)/);
 assert.match(s,/if\(!journalOpen \|\| journalFocused \|\| !journalDomReady \|\| !Valid\(journalView\)\)return/);
});

test('Prisma render surfaces require a loaded game regardless of message order',()=>{
 assert.match(s,/bool gameLoaded = false/);
 assert.match(s,/if\(!gameLoaded \|\| !apiReady\)return;/);
 assert.match(s,/case F4SE::MessagingInterface::kGameDataReady:[\s\S]*?if\(InitializePrismaIfAvailable\(\) && gameLoaded\)CreateViews\(\)/);
 assert.match(s,/case F4SE::MessagingInterface::kPreLoadGame:[\s\S]*?gameLoaded=false;[\s\S]*?DestroyViews\(\)/);
 assert.match(s,/case F4SE::MessagingInterface::kPostLoadGame:[\s\S]*?DestroyViews\(\);[\s\S]*?gameLoaded=true;[\s\S]*?if\(InitializePrismaIfAvailable\(\)\)CreateViews\(\)/);
 assert.match(s,/if \(!trackerView && gameLoaded\) \{[\s\S]*?trackerView = views\.CreateView\("K2040QuestJournal\/tracker\.html"/);
});

test('new Prisma views are hidden immediately before DOM readiness',()=>{
 assert.match(s,/journalView=views\.CreateView\("K2040QuestJournal\/journal\.html", OnDomReady\);\s*[\s\S]{0,220}if\(journalView\)views\.Hide\(journalView\)/);
 assert.match(s,/trackerView = views\.CreateView\("K2040QuestJournal\/tracker\.html", OnTrackerDomReady\);\s*if \(trackerView\) views\.Hide\(trackerView\)/);
 assert.doesNotMatch(s,/if\(Valid\(journalView\)\)views\.Hide\(journalView\);/);
});

test('tracker initialization survives journal open and close',()=>{
 assert.match(s,/K2040::UILifecycleEpoch trackerEpoch/);
 assert.match(s,/trackerView = views\.CreateView\("K2040QuestJournal\/tracker\.html", OnTrackerDomReady\)/);
 assert.match(s,/void OnTrackerDomReady\(PrismaView view\)[\s\S]*?const auto generation=trackerEpoch\.Capture\(\)/);
 assert.match(s,/if\(trackerEpoch\.IsCurrent\(generation\)\)CompleteViewOnGameThread\(view\)/);
 assert.match(s,/void DestroyViews\(\) \{[\s\S]*?uiEpoch\.Invalidate\(\);[\s\S]*?trackerEpoch\.Invalidate\(\)/);
 const close=s.slice(s.indexOf('    void CloseJournal() {'),s.indexOf('    void OpenJournal()'));
 assert.doesNotMatch(close,/trackerEpoch\.Invalidate/);
});

test('native quest strings are bounded and memory-checked before copying',()=>{
 const source=fs.readFileSync('src/QuestCollector.cpp','utf8');
 const reader=fs.readFileSync('src/SafePipboyText.h','utf8');
 assert.match(source,/ReadBoundedGameString\(string->c_str\(\)\)/);
 assert.match(source,/ReadBoundedGameString\(objective->displayText.c_str\(\)\)/);
 assert.match(reader,/kMaxTextBytes = 4096/);
 assert.match(reader,/VirtualQuery\(cursor, &region, sizeof\(region\)\)/);
 assert.match(reader,/PAGE_NOACCESS \| PAGE_GUARD/);
 assert.match(reader,/std::memchr\(cursor, '\\0', available\)/);
});


test('late DOM-ready or canceled generation cleans up retired owned views',()=>{
 assert.match(s,/void RetireStaleView\(PrismaView view\)/);
 assert.match(s,/view != journalView && view != trackerView && Valid\(view\)/);
 assert.match(s,/views\.Destroy\(view\)/);
 assert.match(s,/if \(view != journalView && view != trackerView\) \{[\s\S]*?RetireStaleView\(view\)/);
 assert.match(s,/if\(uiEpoch\.IsCurrent\(generation\)\)CompleteViewOnGameThread\(view\);\s*else RetireStaleView\(view\)/);
 assert.match(s,/if\(trackerEpoch\.IsCurrent\(generation\)\)CompleteViewOnGameThread\(view\);\s*else RetireStaleView\(view\)/);
});

test('journal releases gameplay isolation if another menu or Prisma panel takes focus',()=>{
 assert.match(s,/bool CompetingGameMenuOpen\(\)/);
 for(const menu of ['PipboyMenu','PauseMenu','Console','DialogueMenu'])
   assert.ok(s.includes('menus->GetMenuOpen("'+menu+'")'));
 assert.match(s,/bool AnotherPrismaViewFocused\(\)/);
 assert.match(s,/focused && focused!=journalView/);
 assert.match(s,/void ValidateJournalOwnership\(\)[\s\S]*?CloseJournal\(\)/);
 assert.match(s,/if\(CompetingGameMenuOpen\(\) \|\| AnotherPrismaViewFocused\(\)\) \{\s*CloseJournal\(\);/);
 assert.match(s,/ValidateJournalOwnership\(\);\s*if\(journalOpen && journalFocused\)SyncSnapshots\(\)/);
});

test('late Prisma provider discovery retries after both new and loaded games',()=>{
 assert.match(s,/bool InitializePrismaIfAvailable\(\)/);
 assert.match(s,/if\(!apiReady\) \{[\s\S]*?Discover<UI::ApiFeature::Core>/);
 assert.match(s,/if\(!apiReady\)return false;/);
 assert.match(s,/RegisterHotkeysOnce\(\);\s*RegisterHubScaleformOnce\(\);\s*RegisterPadPollingOnce\(\);\s*return true;/);
 assert.match(s,/case F4SE::MessagingInterface::kNewGame:\s*case F4SE::MessagingInterface::kPostLoadGame:/);
 // The separate read-only GFx QA keeps a synchronized game-loaded
 // mirror between the native state change and the usual Prisma retry.
 assert.match(s,/gameLoaded=true;\s*if\(InitializePrismaIfAvailable\(\)\)CreateViews\(\);\s*#ifdef K2040_PIPBOY_SELECTION_READONLY_QA\s*\/\/ Permit the diagnostic only after save-load initialization\.\s*pipboyGfxGameLoaded\.store\(true,std::memory_order_release\);\s*#endif/);
 const discovery=s.slice(s.indexOf('    bool InitializePrismaIfAvailable()'),s.indexOf('    void Messages('));
 assert.doesNotMatch(discovery,/CreateView\(/);
});

test('first-tester build refuses stale DLLs from a prior compilation',()=>{
 const script=fs.readFileSync('scripts/build-linux.sh','utf8');
 assert.match(script,/build_marker="\$\(mktemp\)"/);
 assert.match(script,/trap 'rm -f -- "\$build_marker"' EXIT/);
 assert.match(script,/xmake -r K2040_Quest_Journal/);
 assert.match(script,/\[\[ "\$dll" -nt "\$build_marker" \]\]/);
 assert.match(script,/refuse stale tester/);
});

test('Prisma SDK build input is pinned and never vendored',()=>{
 const preflight=fs.readFileSync('scripts/build-preflight.py','utf8');
 const sdk=fs.readFileSync('scripts/prisma_header_pin.py','utf8');
 const stage=fs.readFileSync('scripts/stage-prisma-header.py','utf8');
 assert.match(sdk,/OFFICIAL_HEADER_BLOB = "9784daab39bf66bb179e6ef12bba63c75e50d3b7"/);
 assert.match(preflight,/official_header_matches\(HEADER\.read_bytes\(\)\)/);
 assert.match(stage,/os\.link\(tmp, destination\)/);
 assert.doesNotMatch(stage,/^\s*os\.replace\(/m);
});

test('native failed-quest fixture matches real CommonLibF4 flag 0x40',()=>{
 const stub=fs.readFileSync('tests/stubs/RE/Fallout.h','utf8');
 const caseFile=fs.readFileSync('tests/native_quest_collector.cpp','utf8');
 assert.match(stub,/kCompleted=0x2,kFailed=0x40/);
 assert.match(caseFile,/mainQuest\.data\.flags=static_cast<std::uint16_t>\(RE::QuestFlag::kFailed\)/);
 assert.match(caseFile,/json\.find\(.{0,80}failed.{0,20}std::string::npos/);
});

test('Bazzite build prepares configured LLVM PATH before clang-cl preflight',()=>{
 const shell=fs.readFileSync('scripts/build-linux.sh','utf8');
 const exportPath=shell.indexOf('export PATH="$K2040_LLVM_TOOL_DIR:$PATH"');
 const compilerCheck=shell.indexOf('for tool in xmake clang-cl node python3 git; do');
 assert.ok(exportPath>=0 && compilerCheck>exportPath);
 assert.match(shell,/K2040_PRISMA_HEADER_SOURCE:-/);
 assert.match(shell,/stage-prisma-header\.py" --source "\$K2040_PRISMA_HEADER_SOURCE"/);
 assert.match(shell,/'--toolchain=clang-cl\[llvm\]'/);
 assert.match(shell,/--ar="\$K2040_LLVM_TOOL_DIR\/link\.exe"/);
 assert.match(shell,/--linkdirs="\$K2040_WINDOWS_LINK_DIRS"/);
});

test("CommonLib headers precede Windows API string safety header",()=>{
 const source=fs.readFileSync("src/QuestCollector.cpp","utf8");
 assert.ok(source.indexOf("#include <RE/Fallout.h>")<source.indexOf("#include \"SafePipboyText.h\""));
});

test('passive tracker has deterministic four-anchor placement across repeated settings changes',()=>{
 const js=fs.readFileSync('view/app.js','utf8');
 const styles=fs.readFileSync('view/styles.css','utf8');
 assert.match(js,/document\.body\.className='tracker-page anchor-'\+s\.trackerAnchor/);
 for(const edge of ['top','bottom','left','right'])
   assert.match(js,new RegExp('tracker\\.style\\.'+edge+'='));
 for(const anchor of ['top-right','top-left','bottom-right','bottom-left'])
   assert.ok(styles.includes('.tracker-page.anchor-'+anchor+' .tracker-panel'));
 assert.match(js,/tracker\.style\.top=vertical==='top'\?'5%':'auto'/);
 assert.match(js,/tracker\.style\.bottom=vertical==='bottom'\?'5%':'auto'/);
 assert.match(js,/tracker\.style\.left=horizontal==='left'\?'3%':'auto'/);
 assert.match(js,/tracker\.style\.right=horizontal==='right'\?'3%':'auto'/);
});
test('journal controls use consistent in-page select, slider, checkbox and color skins',()=>{
 const styles=fs.readFileSync('view/styles.css','utf8');
 for(const selector of ['.journal-page select','select option','input[type=range]::-webkit-slider-thumb','input[type=checkbox]:checked','input[type=color]::-webkit-color-swatch'])
   assert.ok(styles.includes(selector),selector);
 assert.match(styles,/\.journal-page select:focus-visible/);
 assert.match(styles,/var\(--ui-accent,#66c4ff\)/);
});

test('K2040 contrast and MAP76-inspired presets have distinct scoped visual treatments',()=>{
 const css=fs.readFileSync('view/styles.css','utf8');
 const js=fs.readFileSync('view/app.js','utf8');
 const html=fs.readFileSync('view/journal.html','utf8');
 for(const key of ['theme-contrast','theme-fallout76','journal-theme-contrast','journal-theme-fallout76']) {
   assert.ok(css.includes(key),key);
   assert.ok(js.includes(key),key);
 }
 for(const key of ['data-theme="fallout76"','data-journal-theme="fallout76"']) assert.ok(html.includes(key));
 assert.match(css,/\.tracker-page\.theme-fallout76 \.tracker-panel/);
 assert.match(css,/\.journal-page\.journal-theme-fallout76 \.journal-shell/);
});
test('live Pip-Boy theme obtains HUD and background colors through the engine',()=>{
  const js=fs.readFileSync('view/journal-settings.js','utf8');
  const native=fs.readFileSync('src/main.cpp','utf8');
  const settings=fs.readFileSync('src/JournalSettings.cpp','utf8');
  const css=fs.readFileSync('view/styles.css','utf8');
  assert.match(native,/HUDMenuUtils::GetGameplayHUDColor\(\)/);
  assert.match(native,/HUDMenuUtils::GetGameplayHUDBackgroundColor\(\)/);
  assert.match(native,/K2040::Settings::Serialize\(\s*K2040::HudColor::ToCss/);
  assert.match(settings,/hudBackgroundColor/);
  assert.match(js,/hudColor:color\(v\.hudColor,/);
  assert.match(js,/accent:s\.pipboyColor,text:s\.pipboyColor,panel:s\.hudBackgroundColor,completed:s\.pipboyColor/);
  assert.match(native,/RE::INIPrefSettingCollection::GetSingleton\(\)/);
  assert.match(native,/fPipboyEffectColorR:Pipboy/);
  assert.match(css,/\.journal-page\.theme-pipboy/);
  assert.match(css,/\.tracker-page\.theme-pipboy/);
  assert.match(fs.readFileSync('view/app.js','utf8'),/state\.settings\.journalTheme==='pipboy'/);
});

test('Pip-Boy tracker uses an independent bracketed condensed HUD skin without losing anchors',()=>{
  const css=fs.readFileSync('view/styles.css','utf8');
  const js=fs.readFileSync('view/app.js','utf8');
  const html=fs.readFileSync('view/journal.html','utf8');
  assert.match(css,/\.tracker-page\.theme-pipboy \.tracker-panel \{[^}]*position:absolute/s);
  assert.match(css,/\.tracker-page\.theme-pipboy \.tracker-panel::before/);
  assert.match(css,/font-family:"Roboto Condensed"/);
  assert.match(css,/linear-gradient\(var\(--ui-accent\)/);
  assert.match(js,/preview\.classList\.toggle\('theme-pipboy',s\.theme==='pipboy'\)/);
  assert.match(html,/data-settings-page="journalAppearance"/);
  assert.match(html,/id="settingsJournalAppearance"/);
  assert.match(html,/data-journal-theme="custom"/);
  assert.match(html,/data-setting="journalOpacity"/);
});
