const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const vm=require('node:vm');
const source=fs.readFileSync('view/quest-selects.js','utf8');
function harness(){
 const root={innerWidth:1280,innerHeight:720,listeners:{},addEventListener(name,fn){(this.listeners[name]??=[]).push(fn)}};
 class Elem {
  constructor(tag){this.tagName=tag;this.children=[];this.attributes={};this.listeners={};this.style={};this.disabled=false;this._value='';this._classes=new Set();this.classList={add:x=>this._classes.add(x),contains:x=>this._classes.has(x)};this.className='';this.textContent='';this.parentNode=null;}
  setAttribute(k,v){this.attributes[k]=String(v)}
  getAttribute(k){return this.attributes[k]||null}
  addEventListener(name,fn){(this.listeners[name]??=[]).push(fn)}
  dispatchEvent(event){for(const fn of this.listeners[event.type]||[])fn(event);return true}
  appendChild(el){el.parentNode=this;this.children.push(el);return el}
  insertBefore(el,before){const index=this.children.indexOf(before);el.parentNode=this;if(index<0)this.children.push(el);else this.children.splice(index,0,el);return el}
  removeChild(el){const index=this.children.indexOf(el);if(index>=0)this.children.splice(index,1);el.parentNode=null;return el}
  contains(el){return this===el || this.children.some(c=>c.contains(el))}
  get nextSibling(){if(!this.parentNode)return null;const siblings=this.parentNode.children;return siblings[siblings.indexOf(this)+1]||null}
  getBoundingClientRect(){return {left:90,top:80,bottom:114,width:190,height:34}}
  focus(){document.activeElement=this}
 }
 const document={body:new Elem('body'),documentElement:{clientHeight:720,clientWidth:1280},listeners:{},activeElement:null,
   createElement:tag=>new Elem(tag),
   addEventListener(name,fn){(this.listeners[name]??=[]).push(fn)},
   querySelector(){return null},
   querySelectorAll(selector){const all=[];function scan(node){if(node.tagName==='select')all.push(node);node.children.forEach(scan)}scan(this.body);
     return selector.includes(':not(')?all.filter(x=>!x.classList.contains('k2040-select-source')):all.filter(x=>x.classList.contains('k2040-select-source'))},
   contains:node=>document.body.contains(node)};
 root.document=document;
 const select=new Elem('select');select.id='questSort';select.setAttribute('aria-label','Sort');select.options=[
   {value:'pinned',textContent:'Pinned first',selected:true,disabled:false,hidden:false},
   {value:'category',textContent:'Category',selected:false,disabled:false,hidden:false}];
 Object.defineProperty(select,'selectedIndex',{get(){return this.options.findIndex(x=>x.selected)}});
 Object.defineProperty(select,'value',{get(){return this.options.find(x=>x.selected)?.value||''},set(v){this.options.forEach(o=>o.selected=o.value===v)}});
 document.body.appendChild(select);
 class Event {constructor(type,options){this.type=type;this.bubbles=!!options?.bubbles}}
 vm.runInNewContext(source,{window:root,Event});
 return {root,document,select};
}
test('dark listbox commits to real select and emits one change event',()=>{
 const {root,document,select}=harness();
 const trigger=document.body.children[1];
 assert.equal(trigger.tagName,'button');
 assert.equal(trigger.textContent,'Pinned first');
 let changes=0;select.addEventListener('change',event=>{assert.equal(event.bubbles,true);++changes});
 trigger.listeners.click[0]();
 const menu=document.body.children[2];
 assert.equal(menu.getAttribute('role'),'listbox');
 assert.equal(menu.children.length,2);
 menu.children[1].listeners.click[0]();
 assert.equal(select.value,'category');
 assert.equal(changes,1);
 assert.equal(trigger.textContent,'Category');
 assert.equal(document.body.children.length,2);
 // Programmatic native settings updates synchronize the custom trigger.
 select.value='pinned';root.K2040JournalSelects.sync();
 assert.equal(trigger.textContent,'Pinned first');
});
test('Escape closes dropdown without forwarding the Journal close action',()=>{
 const {document}=harness();
 const trigger=document.body.children[1];trigger.listeners.click[0]();
 let stopped=false,prevented=false;
 const event={key:'Escape',preventDefault(){prevented=true},stopImmediatePropagation(){stopped=true}};
 document.listeners.keydown[0](event);
 assert.equal(stopped,true);assert.equal(prevented,true);
 assert.equal(document.body.children.length,2);
 assert.equal(trigger.getAttribute('aria-expanded'),'false');
});
