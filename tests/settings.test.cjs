'use strict';
const {test} = require('node:test');
const assert = require('node:assert/strict');
const S = require('../view/journal-settings.js');
test('independent journal preset and custom palette bounds',()=>{
  const defaults=S.normalize({});
  assert.equal(defaults.journalTheme,'pipboy');
  assert.equal(S.journalPalette(defaults).accent,'#20ff48');
  assert.equal(defaults.journalOpacity,98);
  assert.equal(defaults.journalFontSize,100);
  const s=S.normalize({theme:'pipboy',hudColor:'#ff6300',
      journalTheme:'custom',journalAccent:'#8877ff',journalPanel:'#131923',
      journalText:'#eeeeaa',journalCompleted:'#33ff77',journalOpacity:12,
      journalFontSize:900});
  assert.equal(S.palette(s).accent,'#ff6300');
  assert.equal(S.journalPalette(s).accent,'#8877ff');
  assert.equal(S.journalPalette(s).panel,'#131923');
  assert.equal(s.journalOpacity,25);
  assert.equal(s.journalFontSize,150);
  assert.equal(S.journalPalette(S.normalize({journalTheme:'contrast'})).panel,'#000000');
  assert.equal(S.journalPalette(S.normalize({journalTheme:'custom',journalAccent:'red'})).accent,'#66c4ff');
});
test('shared K2040 palettes match Quick Attach and MAP76 theme identities',()=>{
 const high={accent:'#00e5ff',text:'#ffffff',panel:'#000000',completed:'#63ff9b'};
 const fo76={accent:'#f5cb5b',text:'#ffffcb',panel:'#181c1d',completed:'#e0c870'};
 assert.deepEqual(S.palette(S.normalize({theme:'contrast'})),high);
 assert.deepEqual(S.journalPalette(S.normalize({journalTheme:'contrast'})),high);
 assert.deepEqual(S.sharedPalettes.contrast,high);
 assert.deepEqual(S.palette(S.normalize({theme:'fallout76'})),fo76);
 assert.deepEqual(S.journalPalette(S.normalize({journalTheme:'fallout76'})),fo76);
 assert.deepEqual(S.sharedPalettes.fallout76,fo76);
 assert.ok(S.themeChoices.includes('fallout76'));
 assert.equal(S.normalize({theme:'not-a-theme'}).theme,'pipboy');
 assert.equal(S.normalize({journalTheme:'not-a-theme'}).journalTheme,'pipboy');
 assert.equal(S.normalize({journalTheme:'default'}).journalTheme,'pipboy');
 assert.equal(S.normalize({journalTheme:'dark'}).journalTheme,'dark');
});
test('default palette and safe fallback',()=>{
 const a=S.normalize({});assert.equal(a.theme,'pipboy');assert.equal(S.palette(a).accent,'#20ff48');
 assert.equal(S.normalize('{').fontSize,100);
});
test('clamps numeric preferences and rejects malformed colours',()=>{
 const a=S.normalize({fontSize:500,trackerWidth:-30,opacity:-12,accent:'</style>',trackerOpacity:1000});
 assert.equal(a.fontSize,150);assert.equal(a.trackerWidth,260);assert.equal(a.opacity,25);assert.equal(a.accent,'#66c4ff');assert.equal(a.trackerOpacity,100);
});
test('allows bottom and left anchors with no manual placement',()=>{
 for(const value of S.anchors) assert.equal(S.normalize({trackerAnchor:value}).trackerAnchor,value);
 assert.equal(S.normalize({trackerAnchor:'top:0'}).trackerAnchor,'top-right');
});
test('redirection stays disabled while no proven vanilla input interceptor exists',()=>{
 assert.equal(S.normalize({redirectQuestKey:true}).redirectQuestKey,false);
});
test('palette picks custom, pipboy and contrast',()=>{
 assert.equal(S.palette(S.normalize({theme:'custom',accent:'#abcdef'})).accent,'#abcdef');
 assert.equal(S.palette(S.normalize({theme:'pipboy'})).accent,'#20ff48');
 const hud=S.normalize({theme:'pipboy',hudColor:'#EF5003',hudBackgroundColor:'#102030'});
 assert.deepEqual(S.palette(hud),{accent:'#ef5003',text:'#ef5003',panel:'#102030',completed:'#ef5003'});
 const invalid=S.normalize({theme:'pipboy',hudColor:'red',hudBackgroundColor:'not-a-color'});
 assert.equal(S.palette(invalid).accent,'#20ff48');
 assert.equal(S.palette(invalid).panel,'#001407');
 const contrast=S.normalize({theme:'contrast',hudColor:'#ff3300'});
 assert.equal(S.palette(contrast).accent,'#00e5ff');
 const custom=S.normalize({theme:'custom',accent:'#11aa22',hudColor:'#ff3300'});
 assert.equal(S.palette(custom).accent,'#11aa22');
 assert.equal(S.palette(S.normalize({theme:'contrast'})).text,'#ffffff');
});

test('configurable shortcuts refuse reserved hotkeys and duplicate assignments',()=>{
 assert.equal(S.normalize({journalHotkey:'F9'}).journalHotkey,'Shift+G');
 assert.equal(S.normalize({journalHotkey:'J'}).journalHotkey,'Shift+G');
 assert.equal(S.normalize({journalHotkey:'F10',trackerHotkey:'F10'}).trackerHotkey,'G');
 assert.equal(S.normalize({journalHotkey:'F6',trackerHotkey:'F11'}).trackerHotkey,'F11');
});

test('tracker limit and completed objective visibility are bounded',()=>{
 assert.equal(S.normalize({}).trackerMaxQuests,6);
 assert.equal(S.normalize({trackerMaxQuests:50}).trackerMaxQuests,12);
 assert.equal(S.normalize({trackerMaxQuests:-5}).trackerMaxQuests,1);
 assert.equal(S.normalize({trackerShowCompleted:'yes'}).trackerShowCompleted,false);
 assert.equal(S.normalize({trackerShowCompleted:true}).trackerShowCompleted,true);
});

test('Quick Attach-style completed palette is validated and theme aware',()=>{
 const s=S.normalize({theme:'custom',completed:'#123abc'});
 assert.equal(S.palette(s).completed,'#123abc');
 assert.equal(S.normalize({completed:'red'}).completed,'#74e398');
 assert.equal(S.palette(S.normalize({theme:'pipboy'})).completed,'#20ff48');
});
test('journal columns are bounded and defaults preserve fullscreen layout',()=>{
 assert.equal(S.normalize({}).sidebarWidth,240);
 assert.equal(S.normalize({}).questListWidth,360);
 assert.equal(S.normalize({sidebarWidth:999,questListWidth:-9}).sidebarWidth,380);
 assert.equal(S.normalize({questListWidth:-9}).questListWidth,240);
});
test('RGB channel conversion rejects invalid values and roundtrips colours',()=>{
 assert.deepEqual(S.hexChannels('#012aFF'),[1,42,255]);
 assert.equal(S.channelsHex([1,42,255]),'#012aff');
 assert.equal(S.hexChannels('bad'),null);
 assert.equal(S.channelsHex([0,256,2]),null);
});
