'use strict';
const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const S=require('../view/journal-settings.js');
const html=fs.readFileSync('view/journal.html','utf8');
const css=fs.readFileSync('view/styles.css','utf8');
const app=fs.readFileSync('view/app.js','utf8');
const native=fs.readFileSync('src/JournalSettings.cpp','utf8');

test('all new-install theme defaults use live Pip-Boy HUD palette while preserving independence',()=>{
 const s=S.normalize({hudColor:'#dc8a22',hudBackgroundColor:'#140c05'});
 assert.equal(s.theme,'pipboy');
 assert.equal(s.journalTheme,'pipboy');
 assert.deepEqual(S.journalPalette(s),S.palette(s));
 assert.equal(S.journalPalette(s).accent,'#dc8a22');
 assert.equal(S.journalPalette(s).panel,'#140c05');
 assert.equal(S.journalPalette(S.normalize({journalTheme:'dark'})).accent,'#66c4ff');
 const saved=S.normalize({theme:'fallout76',journalTheme:'custom',journalAccent:'#123456'});
 assert.equal(saved.theme,'fallout76');
 assert.equal(S.journalPalette(saved).accent,'#123456');
 assert.equal(S.palette(saved).accent,'#f5cb5b');
 assert.equal(S.normalize({journalTheme:'default'}).journalTheme,'pipboy');
});

test('settings and quest journal use one selected theme with independent tracker preview',()=>{
 assert.match(app,/document\.body\.classList\.toggle\('theme-pipboy',s\.journalTheme==='pipboy'\)/);
 assert.match(app,/S\.journalPalette\(s\)/);
 assert.match(app,/const p=isTracker\?trackerPalette:S\.journalPalette\(s\)/);
 assert.match(app,/s\.theme==='pipboy'\?' theme-pipboy'/);
 assert.match(app,/state\.settings\.journalTheme==='pipboy'/);
 assert.match(html,/data-journal-theme="pipboy"/);
 assert.match(html,/data-journal-theme="dark"/);
 assert.match(native,/val=="default"\?"pipboy":std::string\(val\)/);
});

test('all native WebKit shortcut and controller controls receive themed button skin',()=>{
 assert.equal((html.match(/class="setting-row hub-shortcut-row"/g)||[]).length,2);
 assert.equal((html.match(/class="setting-row hub-pad-row"/g)||[]).length,4);
 assert.match(css,/\.journal-page \.hub-shortcut-row button,/);
 assert.match(css,/\.journal-page \.hub-shortcut-row button:focus-visible/);
 assert.match(css,/\.journal-page \.hub-pad-row select/);
 assert.match(css,/\.journal-page \.settings-toggle:disabled/);
 assert.match(css,/\.journal-page \[hidden\] \{display:none !important\}/);
 assert.match(css,/\.journal-page\.theme-pipboy \.settings-nav,/);
 assert.match(css,/\.journal-page\.theme-pipboy \.primary-button/);
});
