'use strict';
const {test}=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs');
const read=p=>fs.readFileSync(p,'utf8');
const x=read('xmake.lua'),build=read('scripts/build-selector-readonly-qa.sh');
const native=read('src/main.cpp'),probe=read('research/native_one_shot/MiscRuntimeDispatchProbe.h'),ui=read('view/app.js');
test('standalone selector flavor cannot combine with any quest writing flavor',()=>{
 assert.match(x,/option\("quest_selector_readonly_qa"\)\s+set_default\(false\)/);
 assert.match(x,/Selector-only research MUST exclude every quest-writing/);
 assert.match(x,/K2040_SELECTOR_READONLY_QA=1/);
 for(const flag of ['quest_dryrun_qa','quest_integrated_tracking_qa',
  'quest_native_write_qa','quest_single_misc_write_qa','quest_single_misc_rc',
  'quest_dogmeat_production_candidate']) {
  assert.match(x,new RegExp('has_config\\("'+flag+'"\\)'));
  assert.match(build,new RegExp('--'+flag+'=n'));
 }
 assert.match(build,/--quest_selector_readonly_qa=y/);
 assert.match(build,/K2040_SELECTOR_READONLY_STAGE/);
 assert.doesNotMatch(build,/K2040_DRYRUN_QA_STAGE/);
 assert.match(read('src/QuestTrackingPolicy.h'),/kVerifiedBackend = WriteBackend::kNone/);
});
test('captures only four vetted bounded slots, private log, no runtime mutation',()=>{
 assert.match(probe,/kSelectorReadLength=192u/);
 assert.match(probe,/static_assert\(kSelectorReadLength<=256u\)/);
 assert.match(probe,/CaptureSelectorPrefixes\(const Snapshot& checked/);
 assert.match(probe,/checked.helperStatus!=LinkedHelperStatus::kCaptured/);
 assert.match(probe,/Readable\(address,bytes.size\(\),true,fault\)/);
 assert.match(probe,/entry.prefixHex.compare\(0,original.prefixHex.size\(\)/);
 const section=probe.split('CaptureSelectorPrefixes(const Snapshot& checked')[1];
 for(const forbidden of ['SetActive(', 'VirtualProtect(', 'WriteProcessMemory(',
   'VirtualAlloc(', 'CreateRemoteThread(', 'process_vm_readv('])
  assert.ok(!section.includes(forbidden),forbidden);
 assert.match(native,/K2040_Quest_Journal_Selector_ReadOnly_/);
 assert.match(native,/SelectorReadOnlyQA: slot=/);
 assert.match(native,/selectorProcessCaptured.exchange\(true,std::memory_order_acq_rel\)/);
 assert.match(native,/selectorSaveEpoch.IsCurrent\(save\)/);
 assert.match(native,/uiEpoch.IsCurrent\(view\)/);
 assert.match(native,/TaskInterfaceVersion/);
 assert.match(native,/CaptureSelectorRowCorrelation\(\)/);
 assert.match(native,/SelectorReadOnlyQA: row-audit=\{\}/);
 assert.match(read('src/SelectorRowCorrelation.h'),/structural-candidate-NOT-command-ABI/);
 assert.match(read('src/QuestCollector.cpp'),/schemaSyntheticRoot/);
 assert.match(read('src/QuestCollector.cpp'),/output.size\(\)<=4096u/);
 assert.match(native,/receiveSelectorReadOnlyResult/);
 assert.match(native,/requestSelectorReadOnlyCapture/);
});
test('selector UI is explicit, one-shot and never sends bytes to JS',()=>{
 assert.match(ui,/state.selectorReadOnlyAvailable/);
 assert.match(ui,/data-action="qa-selector-readonly"/);
 assert.match(ui,/state.selectorReadOnlyUsed=true/);
 assert.match(ui,/emit\('requestSelectorReadOnlyCapture',id\)/);
 assert.match(ui,/Inspect quest row types \(READ-ONLY, once\)/);
 assert.match(ui,/window.enableSelectorReadOnlyQA/);
 assert.match(ui,/window.receiveSelectorReadOnlyResult/);
 assert.match(ui,/No reply to bounded selector capture/);
});
