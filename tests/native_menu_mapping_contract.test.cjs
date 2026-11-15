const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const root=path.resolve(__dirname,'..');
const native=fs.readFileSync(path.join(root,'src','main.cpp'),'utf8');
const ui=fs.readFileSync(path.join(root,'view','journal.html'),'utf8');

test('Journal reads Fallout 4 menu input mappings on game thread',()=>{
  assert.match(native,/GetMappedKey\(event,RE::INPUT_DEVICE::kGamepad,context\)/);
  for(const control of ['Accept','Cancel','Up','Down','Left','Right'])
    assert.match(native,new RegExp('key\\("'+control+'",Context::kBasicMenuNav\\)'));
  assert.match(native,/CaptureMenuBindings\(\);[\s\S]*?views\.Destroy\(journalView\)/);
  assert.match(native,/std::lock_guard<std::mutex> lock\(menuBindingsMutex\)/);
  assert.match(native,/focused&&ForegroundGame\(\),now,mappedMenu\)/);
});

test('optional X/Y shortcuts cannot override remapped menu actions',()=>{
  assert.match(native,/if\(!mappings\.Uses\(0x4000u\)\)/);
  assert.match(native,/if\(!mappings\.Uses\(0x8000u\)\)/);
  assert.match(native,/primary,held,focused,mappedMenu/);
});

test('Journal controller guidance uses actions rather than Xbox button labels',()=>{
  assert.match(ui,/menu controls to move, confirm and go back/);
  assert.doesNotMatch(ui,/Press A to choose and B to go back/);
});
