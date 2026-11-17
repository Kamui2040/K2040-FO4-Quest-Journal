'use strict';
const {test}=require('node:test'),assert=require('node:assert/strict'),
      fs=require('node:fs'),vm=require('node:vm');
const js=fs.readFileSync('view/app.js','utf8');
const css=fs.readFileSync('view/styles.css','utf8');
const Model=require('../view/quest-model.js'),Settings=require('../view/journal-settings.js');
const themes=['fallout4','fallout76','doom-eternal','fallout3-newvegas'];
const colors={background:'#101010',panel:'#202020',text:'#ececec',accent:'#eecc11',
 muted:'#777777',border:'#444444',highlight:'#ffffff'};
function fixture(tracker){
 const classes=new Set([tracker?'tracker-page':'journal-page']);
 const tokenMap={};
 const body={classList:{
  contains:n=>classes.has(n),add:n=>classes.add(n),remove:n=>classes.delete(n),
  toggle(n,on){if(on===undefined)on=!classes.has(n);on?classes.add(n):classes.delete(n);return on}
 }};
 Object.defineProperty(body,'className',{get:()=>[...classes].join(' '),
  set:s=>{classes.clear();for(const v of String(s).split(/\s+/))if(v)classes.add(v);}});
 const nodes=new Map();
 function node(id){if(!nodes.has(id))nodes.set(id,{
   style:{},dataset:{},textContent:'',innerHTML:'',hidden:false,checked:false,
   classList:{contains:()=>false,toggle(){},add(){},remove(){}},
   addEventListener(){},querySelectorAll(){return []},focus(){},closest(){return null},
   getClientRects(){return [1]}
  });return nodes.get(id)}
 const document={
  body,documentElement:{style:{setProperty:(k,v)=>tokenMap[k]=String(v),
   removeProperty:k=>delete tokenMap[k],fontSize:''}},
  getElementById:node,querySelector:()=>node('shell'),querySelectorAll:()=>[],
  addEventListener(){}
 };
 const window={location:{search:''},K2040QuestModel:Model,
  K2040JournalSettings:Settings,addEventListener(){}};
 vm.runInNewContext(js,{window,document,URLSearchParams,console,
  setTimeout:()=>0,setInterval:()=>0});
 return {window,classes,tokenMap};
}
for(const tracker of [false,true]){
 test((tracker?'tracker':'full journal')+' applies one structural theme class after palette snapshots',()=>{
  const {window,classes,tokenMap}=fixture(tracker);
  for(const theme of themes){
   window.applyUnifiedAppearance({schema:1,theme,palette:'hot-omni',colors});
   for(const id of themes)assert.equal(classes.has('k2040-style-'+id),id===theme,id);
   assert.equal(tokenMap['--ui-accent'],'#eecc11');
  }
  window.applyUnifiedAppearance('native');
  for(const theme of themes)assert.equal(classes.has('k2040-style-'+theme),false);
 });
}
test('each theme visibly changes full quest frames and compact tracker frames independently of RGB',()=>{
 const chunk=css.split('/* K2040 Framework structural language — real Journal + tracker.')[1];
 assert.ok(chunk);
 for(const theme of themes){
  const prefix='body.k2040-style-'+theme;
  for(const part of ['.journal-shell','.tracker-panel','.app-header','.tracker-heading',
                      '.quest-row','.tracked-entry'])
   assert.ok(chunk.includes(prefix+' '+part),theme+' must style '+part);
 }
 assert.doesNotMatch(chunk, /#[0-9A-Fa-f]{6}/);
 assert.match(chunk,/var\(--ui-accent/);
 assert.doesNotMatch(js,/classList\.toggle\?\.\('k2040-style-/);
});
