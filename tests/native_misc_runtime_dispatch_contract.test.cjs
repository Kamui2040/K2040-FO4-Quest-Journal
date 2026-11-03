'use strict';
const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const capture=fs.readFileSync('research/native_one_shot/MiscRuntimeDispatchProbe.h','utf8');
const main=fs.readFileSync('src/main.cpp','utf8');
const ui=fs.readFileSync('view/app.js','utf8');
const policy=fs.readFileSync('research/native_one_shot/MiscVanillaSelectionPreflight.h','utf8');
test('runtime code is only read by isolated integrated QA; no writes, hooks, or debuggers',()=>{
 assert.match(capture,/#if !defined\(K2040_TRACKING_INTEGRATED_QA\)/);
 assert.match(capture,/kOGToggleCommandVtableRva=0x02D12B80u/);
 assert.match(capture,/kHeaderLength=80u/);
 assert.match(capture,/kSlots=4u/);
 assert.match(capture,/kOGLinkedHelperRva=0x00C4B840u/);
 assert.match(capture,/kLinkedHelperLength=160u/);
 assert.match(capture,/static_assert\(kLinkedHelperLength<=256u\)/);
 assert.match(capture,/dispatchCode\[kDirectCallOffset\]!=0xE8u/);
 assert.match(capture,/targetRva!=static_cast<std::int64_t>\(kOGLinkedHelperRva\)/);
 assert.match(capture,/Readable\(helperAddress,kLinkedHelperLength,true,/);
 assert.match(capture,/result.linkedHelper=std::move\(helper\)/);
 // Regression: 264-byte IMAGE_NT_HEADERS64 cannot pass a 256-byte guard.
 // Validate only the fields used, without broadening readable memory limits.
 assert.match(capture,/offsetof\(IMAGE_NT_HEADERS64,OptionalHeader\)/);
 assert.match(capture,/offsetof\(IMAGE_OPTIONAL_HEADER64,SizeOfImage\)/);
 assert.match(capture,/static_assert\(kNTHeaderPrefixLength<=256u\)/);
 assert.match(capture,/Readable\(ntAddress,kNTHeaderPrefixLength,false,&fault\)/);
 assert.match(capture,/std::memcpy\(&nt,reinterpret_cast<const void\*>\(ntAddress\),\s*kNTHeaderPrefixLength\)/);
 assert.doesNotMatch(capture,/Readable\(ntAddress,sizeof\(IMAGE_NT_HEADERS64\)/);
 assert.match(capture,/bytes>256u/);
 assert.match(capture,/VirtualQuery/);
 assert.match(capture,/GetModuleHandleW\(nullptr\)/);
 assert.match(capture,/nt\.OptionalHeader\.ImageBase!=0x140000000ull/);
 assert.match(capture,/nt\.OptionalHeader\.SizeOfImage!=kOGImageSize/);
 assert.match(capture,/Readable\(absolute,kHeaderLength,true,&fault\)/);
 assert.match(capture,/enum class ReadFault/);
 assert.match(capture,/enum class FailurePhase/);
 assert.match(capture,/kSlotMemory/);
 assert.match(capture,/kRegionBoundary/);
 assert.match(capture,/prot==PAGE_EXECUTE_READ \|\|/);
 assert.doesNotMatch(capture,/prot==PAGE_EXECUTE;/);
 for(const forbidden of ['VirtualProtect(', 'VirtualAlloc(', 'VirtualFree(', 'WriteProcessMemory(', 'memmove(', 'SetActive(', 'DebugActiveProcess(', 'CreateRemoteThread(', 'AddTaskPermanent(']){
  assert.ok(!capture.includes(forbidden),forbidden);
 }
});
test('synthetic x64 direct-call target must match the pinned OG helper',()=>{
 const origin=0x9FB850, offset=0x13, expected=0xC4B840;
 const bytes=Buffer.alloc(80,0x90);
 bytes[offset]=0xE8;
 bytes.writeInt32LE(expected-(origin+offset+5),offset+1);
 const matches=()=>bytes[offset]===0xE8 &&
   origin+offset+5+bytes.readInt32LE(offset+1)===expected;
 assert.equal(matches(),true);
 bytes[offset]=0x90;
 assert.equal(matches(),false,'unexpected opcode must not authorize an extra read');
 bytes[offset]=0xE8;
 bytes.writeInt32LE(0,offset+1);
 assert.equal(matches(),false,'unexpected target must not authorize an extra read');
});
test('read-only dispatch sampling is explicit one-shot and cannot escape stale journal context',()=>{
 const begin=main.indexOf('void QueueMiscDispatchProbe(');
 const end=main.indexOf('void SetIntegratedQAConsent(',begin);
 assert.ok(begin>0&&end>begin);
 const handler=main.slice(begin,end);
 for(const text of [
  'dryRunOgRuntime.load()', 'trackingSaveEpoch.Capture()', 'uiEpoch.Capture()',
  'TaskInterfaceVersion', 'miscDispatchCaptured.exchange(true',
  'task->AddTask([id,save,view]', 'QuestViewState::Observed().contains(id)',
  'dryRunSelectedFormId.load(std::memory_order_acquire)==id',
  'quest->data.questType)!=6u','CaptureRuntimeCode(', 'InteropCall(journalView',
  '"receiveMiscDispatchProbe"'
 ])assert.ok(handler.includes(text),text);
 for(const forbidden of ['SetActive(', 'AddTaskPermanent(', 'VirtualProtect(', 'WriteProcessMemory(']){
  assert.ok(!handler.includes(forbidden),forbidden);
 }
 assert.match(handler,/PhaseName\(diagnostic.phase\)/);
 assert.match(handler,/FaultName\(diagnostic.fault\)/);
 assert.match(handler,/diagnostic.slot/);
 assert.match(handler,/if\(image->linkedHelper\)/);
 assert.match(handler,/HelperStatusName/);
 assert.match(handler,/linked helper RVA/);
 assert.match(main,/BindUIEvent\(view,"requestMiscDispatchProbe"/);
});
test('UI requires intentional per-process read-only inspection and gives timeout feedback',()=>{
 assert.match(ui,/data-action="qa-misc-dispatch"/);
 assert.match(ui,/state\.qaDispatchUsed=true/);
 assert.match(ui,/emit\('requestMiscDispatchProbe',id\)/);
 assert.match(ui,/window\.receiveMiscDispatchProbe=function\(value\)/);
 assert.match(ui,/No reply to bounded runtime inspection/);
 assert.match(ui,/nativeTracking\.reason==='misc-objective'/);
 assert.match(policy,/kNativeMiscWritesAuthorized = false/);
 assert.match(fs.readFileSync('src/QuestTrackingPolicy.h','utf8'),/kVerifiedBackend\s*=\s*WriteBackend::kNone/);
});
