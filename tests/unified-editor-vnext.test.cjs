const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const crypto=require('node:crypto');
const vm=require('node:vm');
const root=path=>fs.readFileSync(path,'utf8');
const hash=path=>crypto.createHash('sha256').update(fs.readFileSync(path)).digest('hex');
test('tagged Quest Journal descriptor resolves only the intended native provider',()=>{
  const descriptor=JSON.parse(root('providers/quest-journal.json'));
  assert.deepEqual(descriptor,{
    tag:'k2040.settings',protocolVersion:2,modId:'quest-journal',
    displayName:'Quest Journal',plugin:'K2040_Quest_Journal.dll',
    adapter:'K2040_GetSettingsAdapterV2'
  });
  const native=root('src/main.cpp');
  assert.match(native,/K2040_GetSettingsAdapterV2\(\) noexcept/);
  assert.match(root('scripts/stage-test-package.py'),/Data\/K2040\/Settings\/Providers\/quest-journal.json/);
  assert.match(root('xmake.lua'),/providers","quest-journal.json/);
});
test('appearance client matches pinned VNext header except approved MSVC build fix',()=>{
  const owner=root('src/K2040AppearanceClientV1.h');
  assert.equal(owner.includes('static_cast<std::size_t>(terminator-buffer)'),true);
  assert.equal(crypto.createHash('sha256').update(owner.replace(/\/\/ Build with MSVC\/clang-cl:[^\n]*\n[^\n]*\n    return std::string\(buffer,static_cast<std::size_t>\(terminator-buffer\)\);/,
    'return std::string(buffer,terminator);')).digest('hex'),
    'acaabb5b5d161e058d6b82f913c1e9dce7db4089782cbe1243aa9ebf378b2877');
  const native=root('src/main.cpp');
  assert.match(native,/AppearanceClient::Current\("quest-journal"\)/);
  assert.match(native,/UnifiedAppearance::Validated\(\*snapshot\)/);
  assert.match(native,/applyUnifiedAppearance/);
  assert.match(native,/if\(apiReady&&interop\.InteropCall\)SyncSettings\(\)/);
});
test('native hub color snapshot applies transiently and rejects unsafe payloads',()=>{
  const css={};const elements={};
  const makeNode=()=>({
    style:{},dataset:{},textContent:'',innerHTML:'',checked:false,hidden:false,
    scrollTop:0,getClientRects:()=>[1],
    classList:{contains:()=>false,toggle(){},remove(){},add(){}},
    querySelectorAll:()=>[],addEventListener(){},focus(){},closest(){return null;}
  });
  const get=id=>elements[id]||(elements[id]=makeNode());
  const document={
    body:{className:'journal-page',classList:{contains:()=>false,toggle(){},remove(){},add(){}}},
    documentElement:{style:{setProperty:(k,v)=>{css[k]=String(v);},
      removeProperty:k=>{delete css[k]}}},
    getElementById:get,querySelector:()=>get('shell'),querySelectorAll:()=>[],
    addEventListener(){}
  };
  const window={location:{search:''},K2040QuestModel:require('../view/quest-model.js'),
    K2040JournalSettings:require('../view/journal-settings.js'),addEventListener(){}};
  vm.runInNewContext(root('view/app.js'),{window,document,URLSearchParams,console,setTimeout:()=>0,setInterval:()=>0});
  window.receiveJournalSettings(JSON.stringify({journalTheme:'dark'}));
  const before=css['--ui-accent'];
  const valid={schema:1,theme:'doom-eternal',palette:'hot-omni',
    colors:{background:'#001122',panel:'#112233',text:'#ddeeff',
      accent:'#456789',muted:'#334455',border:'#556677',highlight:'#778899'}};
  window.applyUnifiedAppearance(JSON.stringify(valid));
  assert.equal(css['--ui-accent'],'#456789');
  assert.equal(css['--bg'],'#001122');
  assert.equal(css['--edge'],'#556677');
  window.applyUnifiedAppearance(JSON.stringify({...valid,colors:{...valid.colors,accent:'url(bad)'}}));
  assert.equal(css['--ui-accent'],'#456789');
  window.applyUnifiedAppearance('native');
  assert.equal(css['--ui-accent'],before);
  assert.equal(css['--edge'],undefined);
});
