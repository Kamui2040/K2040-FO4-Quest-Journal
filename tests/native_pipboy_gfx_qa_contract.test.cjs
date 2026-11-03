const {readFileSync}=require('node:fs');
const test=require('node:test');
const assert=require('node:assert/strict');
const read=p=>readFileSync(p,'utf8');
test('isolated variant has no writer source and only the intended hotkey',()=>{
  const mk=read('xmake.lua'),main=read('src/main.cpp'),
    build=read('scripts/build-pipboy-gfx-readonly-qa.sh');
  assert.match(mk,/option\("quest_pipboy_selection_readonly_qa"\)/);
  assert.match(mk,/add_files\("research\/native_pipboy_selection\/UnwiredGFxAdapter.cpp"\)/);
  assert.match(mk,/raise\("Pipboy GFx observer QA MUST exclude all writer/);
  assert.match(main,/K2040_PIPBOY_SELECTION_READONLY_QA/);
  assert.match(main,/AddUITask\(\[save\]\(\)\{/);
  assert.match(main,/pipboyGfxSaveEpoch\.IsCurrent\(save\)/);
  assert.match(main,/pipboyGfxSaveEpoch\.Invalidate\(\)/);
  // Save-transition messaging and the GFx UI task may execute on
  // different threads: the new callback must not read plain gameLoaded.
  assert.match(main,/std::atomic_bool pipboyGfxGameLoaded\{false\}/);
  assert.match(main,/pipboyGfxGameLoaded\.load\(std::memory_order_acquire\)/);
  assert.match(main,/pipboyGfxGameLoaded\.store\(false,std::memory_order_release\)/);
  assert.match(main,/pipboyGfxGameLoaded\.store\(true,std::memory_order_release\)/);
  assert.match(main,/shift && ctrl && !alt && down\(VK_F11\)/);
  assert.match(main,/pipboyGfxAttempted\.exchange\(true/);
  assert.match(main,/nativeListSelection=.*childFormIDTyped=/s);
  assert.match(build,/--quest_pipboy_selection_readonly_qa=y/);
  for (const option of [
    'quest_native_write_qa','quest_dryrun_qa','quest_integrated_tracking_qa',
    'quest_single_misc_write_qa','quest_single_misc_rc',
    'quest_dogmeat_production_candidate','quest_selector_readonly_qa'
  ])assert.match(build,new RegExp('--'+option+'=n'));
  assert.doesNotMatch(read('research/native_pipboy_selection/UnwiredGFxAdapter.cpp'),/->Invoke\(|->SetMember\(|->SetVariable\(/);
});
