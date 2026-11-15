'use strict';
const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const S=require('../view/journal-settings.js');
const manifest=JSON.parse(fs.readFileSync('view/settings-v1-manifest.json','utf8'));
const fields=manifest.categories.flatMap(c=>c.groups.flatMap(g=>g.fields));
const field=id=>fields.find(f=>f.id===id);

test('native journal and tracker themes remain independent',()=>{
 const s=S.normalize({hudColor:'#dc8a22',hudBackgroundColor:'#140c05'});
 assert.equal(s.theme,'pipboy'); assert.equal(s.journalTheme,'pipboy');
 assert.deepEqual(S.journalPalette(s),S.palette(s));
 const saved=S.normalize({theme:'fallout76',journalTheme:'custom',journalAccent:'#123456'});
 assert.equal(S.journalPalette(saved).accent,'#123456');
 assert.equal(S.palette(saved).accent,'#f5cb5b');
 assert.equal(S.normalize({journalTheme:'default'}).journalTheme,'pipboy');
 assert.equal(S.journalPalette(S.normalize({journalTheme:'dark'})).accent,'#66c4ff');
});

test('all independent theme controls exist in shared V1 manifest',()=>{
 for(const id of ['theme','journalTheme']){
   assert.equal(field(id).type,'select'); assert.equal(field(id).options.length,5);
 }
 for(const id of ['accent','text','panel','completed','journalAccent','journalText',
                  'journalPanel','journalCompleted']){
   assert.equal(field(id).type,'color');
   assert.equal(field(id).advanced,true);
 }
});

test('Quest Journal retires the separate settings editor, keeps V2 theme bridge',()=>{
 const html=fs.readFileSync('view/journal.html','utf8');
 const js=fs.readFileSync('view/app.js','utf8');
 assert.doesNotMatch(html,/id="openSettings"|id="settingsWorkspace"|data-settings-page/);
 assert.doesNotMatch(js,/changeSetting\(|showSettings\(|saveHubKeyboard|setHubNativeCapture/);
 assert.match(js,/window.applyUnifiedTheme=function/);
 assert.match(js,/state.unifiedTheme=theme==='native'\?null:theme/);
 assert.match(fs.readFileSync('src/main.cpp','utf8'),/K2040_GetSettingsAdapterV2/);
});
