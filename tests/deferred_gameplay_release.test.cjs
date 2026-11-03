'use strict';
const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const s=fs.readFileSync('src/main.cpp','utf8');

test('journal closing retains gameplay isolation until shortcut base key is physically released',()=>{
 assert.match(s,/std::atomic<std::uint32_t> deferredJournalReleaseKey\{0\}/);
 assert.match(s,/const std::uint32_t heldKey=journalInputGuard\?HeldJournalShortcutKey\(\):0/);
 assert.match(s,/if\(heldKey\)\s*\{[\s\S]*?deferredJournalReleaseKey\.store\(heldKey/);
 assert.match(s,/void FinishDeferredJournalInputGuard\(std::uint32_t key\)/);
 assert.match(s,/if\(journalOpen \|\| deferredJournalReleaseKey\.load\(std::memory_order_acquire\)!=key\)/);
 assert.match(s,/GetAsyncKeyState\(static_cast<int>\(key\)\)/);
 assert.match(s,/GetTickCount64\(\)-started<5000/);
 assert.match(s,/tasks->AddTask\(\[guardKey\]\(\)\{FinishDeferredJournalInputGuard\(guardKey\);\}\)/);
 assert.doesNotMatch(s,/SetWindowsHookEx|WH_KEYBOARD_LL|ControlMap::RemapButton|SaveRemappings/);
});
test('journal release keeps Esc behavior and no global fighting-input lock',()=>{
 assert.ok(s.includes("controller.SetViewOwnsEscape(view,true)"));
 assert.match(s,/if\(!escapeUiAcknowledged\.exchange\(false/);
 assert.match(s,/const bool rising=matched&&!k\.wasMatched/);
 assert.doesNotMatch(s,/EnableUserEvent\([\s\S]{0,100}USER_EVENT_FLAG::kFighting/);
});
