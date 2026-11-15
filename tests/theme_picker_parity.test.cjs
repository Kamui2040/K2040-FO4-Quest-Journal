'use strict';
const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const S=require('../view/journal-settings.js');
const manifest=JSON.parse(fs.readFileSync('view/settings-v1-manifest.json','utf8'));
const fields=manifest.categories.flatMap(c=>c.groups.flatMap(g=>g.fields));
const options=id=>fields.find(f=>f.id===id).options;

test('tracker and journal expose their original five themes through one catalog',()=>{
 const tracker=options('theme'),journal=options('journalTheme');
 const labels=['Commonwealth Blue','Classic Pip-Boy','High Contrast','Fallout 76','Custom Colors'];
 assert.deepEqual(tracker.map(x=>x.label),labels);
 assert.deepEqual(journal.map(x=>x.label),labels);
 assert.deepEqual(tracker.map(x=>x.value),['commonwealth','pipboy','contrast','fallout76','custom']);
 assert.deepEqual(journal.map(x=>x.value),['dark','pipboy','contrast','fallout76','custom']);
});

test('corresponding theme presets preserve native color parity',()=>{
 const tracker=options('theme'),journal=options('journalTheme');
 for(let i=0;i<tracker.length;i++){
   const s=S.normalize({theme:tracker[i].value,journalTheme:journal[i].value,
     hudColor:'#78bc21',pipboyColor:'#113355',hudBackgroundColor:'#010203',
     accent:'#223344',text:'#445566',panel:'#667788',completed:'#8899aa',
     journalAccent:'#223344',journalText:'#445566',journalPanel:'#667788',
     journalCompleted:'#8899aa'});
   assert.deepEqual(S.palette(s),S.journalPalette(s),tracker[i].label);
 }
 assert.equal(S.normalize({journalTheme:'dark'}).journalTheme,'dark');
 assert.equal(S.defaults.journalTheme,'pipboy');
});

test('tracker and journal native appearance stays independent',()=>{
 const j=S.normalize({theme:'fallout76',journalTheme:'dark'});
 assert.notDeepEqual(S.palette(j),S.journalPalette(j));
 const t=S.normalize({theme:'commonwealth',journalTheme:'contrast'});
 assert.notDeepEqual(S.palette(t),S.journalPalette(t));
});
