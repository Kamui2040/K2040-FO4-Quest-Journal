const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const S=require('../view/journal-settings.js');
const markup=fs.readFileSync('view/journal.html','utf8');
function options(attribute){
  const re=new RegExp('<button '+attribute+'="([^"]+)" type="button">([^<]+)</button>','g');
  return [...markup.matchAll(re)].map(([,value,label])=>({value,label}));
}
test('tracker and Journal offer identical theme labels in identical order',()=>{
 const tracker=options('data-theme'),journal=options('data-journal-theme');
 const expected=['Commonwealth Blue','Classic Pip-Boy','High Contrast',
   'Fallout 76 (MAP76-inspired)','Custom Colours'];
 assert.deepEqual(tracker.map(x=>x.label),expected);
 assert.deepEqual(journal.map(x=>x.label),expected);
 assert.equal(tracker.length,5);assert.equal(journal.length,5);
});
test('corresponding presets have identical colors and preserve existing Journal dark values',()=>{
 const tracker=options('data-theme'),journal=options('data-journal-theme');
 assert.deepEqual(tracker.map(x=>x.value),['commonwealth','pipboy','contrast','fallout76','custom']);
 assert.deepEqual(journal.map(x=>x.value),['dark','pipboy','contrast','fallout76','custom']);
 for(let i=0;i<tracker.length;i++){
  const setting=S.normalize({theme:tracker[i].value,journalTheme:journal[i].value,
    hudColor:'#78bc21',pipboyColor:'#113355',hudBackgroundColor:'#010203',
    accent:'#223344',text:'#445566',panel:'#667788',completed:'#8899aa',
    journalAccent:'#223344',journalText:'#445566',journalPanel:'#667788',
    journalCompleted:'#8899aa'});
  assert.deepEqual(S.palette(setting),S.journalPalette(setting),tracker[i].label);
 }
 const legacy=S.normalize({journalTheme:'dark'});
 assert.equal(legacy.journalTheme,'dark');
 assert.equal(S.journalPalette(legacy).accent,'#66c4ff');
 assert.equal(S.defaults.journalTheme,'pipboy');
 assert.equal(S.defaults.theme,'pipboy');
});
test('appearance selectors stay independent despite matching preset names',()=>{
 const journal=S.normalize({theme:'fallout76',journalTheme:'dark'});
 assert.notDeepEqual(S.palette(journal),S.journalPalette(journal));
 const tracker=S.normalize({theme:'commonwealth',journalTheme:'contrast'});
 assert.notDeepEqual(S.palette(tracker),S.journalPalette(tracker));
});
