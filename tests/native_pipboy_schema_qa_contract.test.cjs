const {readFileSync}=require('node:fs');
const {test}=require('node:test');
const assert=require('node:assert/strict');
const read=p=>readFileSync(p,'utf8');
test('schema variant remains default-off and requires isolated observer',()=>{
  const mk=read('xmake.lua'),main=read('src/main.cpp'),adapter=read('research/native_pipboy_selection/UnwiredGFxAdapter.cpp');
  const build=read('scripts/build-pipboy-gfx-schema-readonly-qa.sh');
  assert.match(mk,/option\("quest_pipboy_selection_schema_readonly_qa"\)/);
  assert.match(mk,/not has_config\("quest_pipboy_selection_readonly_qa"\)/);
  assert.match(mk,/K2040_PIPBOY_SELECTION_SCHEMA_READONLY_QA=1/);
  assert.match(main,/ObserveOpenPipboySchemaOnUIThread\(\)/);
  assert.match(adapter,/SummarizeSchema\(child\)/);
  assert.match(build,/--quest_pipboy_selection_schema_readonly_qa=y/);
  for(const opt of ['quest_native_write_qa','quest_dryrun_qa','quest_integrated_tracking_qa',
      'quest_single_misc_write_qa','quest_single_misc_rc','quest_dogmeat_production_candidate','quest_selector_readonly_qa'])
    assert.match(build,new RegExp('--'+opt+'=n'));
  assert.doesNotMatch(adapter,/->Invoke\(|->SetMember\(|->SetVariable\(/);
  const header=read('research/native_pipboy_selection/ReadOnlySelectedSchema.h');
  assert.match(header,/if\(s.total>=48\)/);
  assert.match(header,/s.nameMask=s.numericNameMask=0;s.ordinalMask=s.numericOrdinalMask=0/);
  assert.doesNotMatch(header,/GetUInt\(|GetInt\(|GetString\(|GetNumber\(/);
});
