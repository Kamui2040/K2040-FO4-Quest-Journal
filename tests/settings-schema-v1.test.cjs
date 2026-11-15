'use strict';
const test=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const crypto=require('node:crypto');
const {validateManifest,schema}=require('../scripts/validate-settings-manifest.cjs');
const S=require('../view/journal-settings.js');

const root=path.resolve(__dirname,'..');
const manifest=JSON.parse(fs.readFileSync(path.join(root,'view/settings-v1-manifest.json'),'utf8'));
const fields=manifest.categories.flatMap(c=>c.groups.flatMap(g=>g.fields));
const settings=fields.filter(f=>f.type!=='action');
const byId=new Map(fields.map(f=>[f.id,f]));
const cpp=fs.readFileSync(path.join(root,'src/JournalSettings.cpp'),'utf8');
const nativeHeader=fs.readFileSync(path.join(root,'src/JournalSettings.h'),'utf8');
const html=fs.readFileSync(path.join(root,'view/journal.html'),'utf8');
const hash=file=>crypto.createHash('sha256').update(fs.readFileSync(path.join(root,file))).digest('hex');
const clone=()=>structuredClone(manifest);
const problems=m=>validateManifest(m).join('; ');

test('canonical V1 validator and schema are pinned to upstream byte hashes',()=>{
  assert.equal(hash('schemas/k2040-settings-manifest.v1.schema.json'),
    'd913a4da9d554af1f9b9feb4a84b229bc211267eb7d260964576ea2ca29287dc');
  assert.equal(hash('scripts/validate-settings-manifest.cjs'),
    '2373ed7966f025410716d88fd79770335f03d5a4955098f396f7225f7c33a9b5');
  assert.equal(schema.$id,'urn:k2040:settings:manifest:v1');
  assert.deepEqual(validateManifest(manifest),[]);
  assert.equal(manifest.manifestVersion,1);
  assert.equal(manifest.moduleId,'quest-journal');
  assert.equal(manifest.nativeSettingsSchemaVersion,1);
  assert.equal(fields.length,32);
});

test('every native saved preference is visible once or explicitly internal',()=>{
  const body=nativeHeader.split('struct Preferences {')[1].split('};')[0];
  const native=[...body.matchAll(/^\s*(?:std::string|int|bool)\s+([a-zA-Z]\w*)\s*=/gm)].map(m=>m[1]);
  assert.equal(native.length,39,'Update inventory for a changed native Preferences struct');
  assert.equal(new Set(native).size,native.length);
  const covered=[...settings.map(f=>f.id),...manifest.internalKeys];
  assert.deepEqual(covered.slice().sort(),native.slice().sort());
  assert.deepEqual(covered.slice().sort(),Object.keys(S.defaults).sort());
  assert.equal(new Set(covered).size,covered.length);
  assert.equal(byId.get('journalHotkey').type,'keybind');
  assert.equal(byId.get('trackerHotkey').type,'keybind');
  assert.equal(byId.has('trackerSyncPipboy'),false);
  assert.equal(byId.has('opacity'),false);
  assert.equal(byId.has('fontSize'),false);
  for(const field of settings) {
    assert.ok(cpp.includes('key=="'+field.id+'"'),
      'Manifest setting missing native Apply handler: '+field.id);
    assert.ok(Object.hasOwn(S.defaults,field.id),'Missing JS normalization: '+field.id);
  }
});

test('control types, choices and number ranges match the existing native and standalone UI',()=>{
  for(const field of settings) {
    if(field.type==='toggle')
      assert.equal(typeof S.defaults[field.id],'boolean',field.id);
    if(field.type==='color')
      assert.match(S.defaults[field.id],/^#[a-f0-9]{6}$/i,field.id);
    if(field.type==='select') {
      assert.ok(['string','number'].includes(typeof S.defaults[field.id]),field.id);
      for(const option of field.options) {
        // Tracker and Journal keyboard actions may not use the same shortcut.
        const context=field.id==='trackerHotkey'?{journalHotkey:'G'}:
                      field.id==='journalHotkey'?{trackerHotkey:'F6'}:{};
        const normalized=S.normalize({...context,[field.id]:option.value});
        if(field.id==='trackerHotkey' && option.value==='G')continue;
        if(field.id==='journalHotkey' && option.value==='F6')continue;
        assert.equal(normalized[field.id],option.value,field.id+' option '+option.value);
      }
    }
    if(field.type==='integer') {
      const match=cpp.match(new RegExp('Int\\(val,out\\.'+field.id+',\\s*(\\d+),\\s*(\\d+)\\)'));
      assert.ok(match,'Native numeric bounds missing for '+field.id);
      assert.equal(field.min,Number(match[1]),field.id+' min');
      assert.equal(field.max,Number(match[2]),field.id+' max');
      assert.equal(S.normalize({[field.id]:field.min})[field.id],field.min);
      assert.equal(S.normalize({[field.id]:field.max})[field.id],field.max);
    }
  }
  assert.deepEqual(manifest.categories.map(c=>c.id),
    ['general','tracker-layout','tracker-appearance','journal-appearance','journal-layout','controls']);
});

test('hidden dependent fields do not clear native or standalone saved values',()=>{
  for(const id of ['trackerOverridePhosphor','trackerWidth','trackerScale','trackerOpacity',
                     'trackerAutoPinNew']) {
    assert.deepEqual(byId.get(id).visibleWhen,{all:['trackerEnabled']},id);
  }
  const s=S.normalize({trackerEnabled:false,trackerWidth:520,trackerScale:135,
    trackerOpacity:40,trackerAutoPinNew:true,trackerSyncPipboy:true,
    theme:'custom',accent:'#123456',journalTheme:'custom',journalAccent:'#abcdef'});
  assert.equal(s.trackerWidth,520);
  assert.equal(s.trackerScale,135);
  assert.equal(s.trackerOpacity,40);
  assert.equal(s.trackerSyncPipboy,true);
  assert.equal(s.trackerAutoPinNew,true);
  assert.equal(s.accent,'#123456');
  assert.equal(s.journalAccent,'#abcdef');
  for(const id of ['accent','text','panel','completed','journalAccent','journalText',
                    'journalPanel','journalCompleted'])
    assert.equal(byId.get(id).advanced,true,id);
});

test('gameplay shortcuts belong to hub-managed owner settings and reset is module-owned',()=>{
  assert.equal(fs.existsSync(path.join(root,'mcm/standalone/Data/MCM/Config/K2040_Quest_Journal')),false);
  for(const id of ['journalHotkey','trackerHotkey'])
    assert.equal(byId.get(id).type,'keybind');
  for(const id of ['journalPadPrimary','journalPadHeld','trackerPadPrimary','trackerPadHeld'])
    assert.equal(byId.get(id).type,'select');
  assert.equal(byId.has('trackerSyncPipboy'),false);
  assert.equal(manifest.internalKeys.includes('trackerSyncPipboy'),true);
  assert.equal(byId.get('redirectQuestKey').type,'toggle');
  assert.equal(byId.get('resetPreferences').owner,'module');
  assert.equal(byId.get('resetPreferences').actionId,'resetToDefaults');
  assert.match(cpp,/bool ResetToDefaults\(\)/);
  assert.match(cpp,/kLegacyPath/);
  assert.match(cpp,/LOCALAPPDATA/);
  assert.doesNotMatch(html,/id="openSettings"|id="settingsWorkspace"/);
  assert.match(html,/id="pipboySyncToggle"/);
  assert.doesNotMatch(html,/settings-v1-manifest\.json/);
});

test('malformed, unknown, duplicate and unsafe metadata fails closed',()=>{
  let v=clone();v.manifestVersion=2;assert.ok(problems(v));
  v=clone();v.categories[0].groups[0].fields[0].type='hotkey';assert.ok(problems(v));
  v=clone();v.categories[0].groups[0].fields[0].id='trackerOverridePhosphor';
  assert.match(problems(v),/duplicate setting/);
  v=clone();v.internalKeys.push('trackerEnabled');
  assert.match(problems(v),/internal and visible setting collide/);
  v=clone();v.categories[0].groups[0].fields[0].visibleWhen={all:['trackerOverridePhosphor']};
  assert.match(problems(v),/visibility dependency cycle/);
  v=clone();v.categories[0].groups[0].fields[0].visibleWhen={all:['trackerWidth']};
  assert.match(problems(v),/dependency must be a toggle/);
  v=clone();v.categories[0].groups[0].fields[0].visibleWhen={all:['nonexistent']};
  assert.match(problems(v),/missing dependency/);
  v=clone();const density=v.categories.find(c=>c.id==='journal-layout')
    .groups[0].fields[0];
  density.options[1].value=42;
  assert.match(problems(v),/mixed option types/);
});
