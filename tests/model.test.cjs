'use strict';
const assert = require('node:assert/strict');
const {test} = require('node:test');
const M = require('../view/quest-model.js');
const data = [
 {id:'00ab12cc',name:'Alpha',state:'active',type:'Main',pinned:true,description:'Find the signal',objectives:[{index:1,text:'Visit the tower',state:'active'}]},
 {id:'00110002',name:'Beta',state:'completed',type:'FarHarbor',objectives:[{index:0,text:'Done',state:'completed'}]},
 {id:'00110003',name:'Gamma',state:'failed',type:'Modded',objectives:[]}
];
test('normalizes stable shape and objective data without mutating input',()=>{
 const result=M.normalizeSnapshot({ready:true,quests:data});
 assert.equal(result.ready,true);
 assert.equal(result.quests[0].id,'00AB12CC');
 assert.equal(result.quests[1].type,'Far Harbor');
 assert.equal(result.quests[0].objectives[0].text,'Visit the tower');
 assert.equal(data[0].id,'00ab12cc');
});
test('rejects malformed IDs and duplicates rather than accidentally pinning a different quest',()=>{
 const result=M.normalizeSnapshot({ready:true,quests:[data[0],{...data[0],id:'00AB12CC'},{...data[0],id:'<script>'},null]});
 assert.equal(result.quests.length,1);
});
test('filters status, text, category, favorites and hidden independently',()=>{
 const list=M.normalizeSnapshot({ready:true,quests:data}).quests;
 assert.deepEqual(M.filterQuests(list,{status:'active'}).map(q=>q.name),['Alpha']);
 assert.deepEqual(M.filterQuests(list,{status:'all',query:'signal'}).map(q=>q.name),['Alpha']);
 assert.deepEqual(M.filterQuests(list,{status:'all',query:'tower'}).map(q=>q.name),['Alpha']);
 assert.deepEqual(M.filterQuests(list,{status:'all',category:'Far Harbor'}).map(q=>q.name),['Beta']);
 assert.deepEqual(M.filterQuests(list,{status:'all',hidden:new Set(['00AB12CC'])}).map(q=>q.name),['Beta','Gamma']);
 assert.deepEqual(M.filterQuests(list,{status:'all',favoritesOnly:true,favorites:new Set(['00110003'])}).map(q=>q.name),['Gamma']);
});
test('category order follows game labels and then additional modded categories',()=>{
 const list=M.normalizeSnapshot({ready:true,quests:data}).quests;
 assert.deepEqual(M.listCategories(list).map(x=>x.name),['Main','Far Harbor','Modded']);
});
test('unknown and unavailable snapshots are safe',()=>{
 assert.deepEqual(M.normalizeSnapshot({ready:false}),{ready:false,quests:[]});
 assert.equal(M.normalizeSnapshot({ready:true,quests:[{id:'12345678',objectives:['bad',{text:'Obj',state:'strange'}]}]}).quests[0].objectives[0].state,'inactive');
});

test('malformed user-created quest type and numeric IDs cannot corrupt the model',()=>{
 const list=M.normalizeSnapshot({ready:true,quests:[{id:12345678,name:'Number',type:'__proto__',state:'active'}]}).quests;
 assert.equal(list[0].id,'12345678');
 assert.equal(list[0].type,'__proto__');
 assert.equal(M.listCategories(list)[0].name,'__proto__');
});


test('snapshot metadata preserves independent favorites and hidden state',()=>{
 const q=M.normalizeSnapshot({ready:true,quests:[{...data[0],favorite:true,hidden:true}]}).quests[0];
 assert.equal(q.favorite,true);assert.equal(q.hidden,true);assert.equal(q.pinned,true);
 assert.deepEqual(M.filterQuests([q],{status:'active',hidden:new Set([q.id])}),[]);
});
test('quest metadata is boolean-only and rejects malformed values',()=>{
 const q=M.normalizeQuest({...data[0],favorite:'true',hidden:1,pinned:{}});
 assert.equal(q.favorite,false);assert.equal(q.hidden,false);assert.equal(q.pinned,false);
});

test('manual grouping retains native category and safe override state',()=>{
 const q=M.normalizeQuest({id:'00000042',type:'Far Harbor',sourceType:'Far Harbor',categoryOverride:true});
 assert.equal(q.sourceType,'Far Harbor');assert.equal(q.categoryOverride,true);
 assert.equal(M.USER_CATEGORIES[0],'Automatic');
 assert.ok(M.USER_CATEGORIES.includes('Modded'));
 assert.equal(M.normalizeQuest({id:'00000042',type:'Side',categoryOverride:'yes'}).categoryOverride,false);
});


test('historical fallback flag is opt-in and keeps completion state',()=>{
 const quest=M.normalizeSnapshot({ready:true,quests:[{id:'00000F11',state:'completed',type:'Side',historyOnly:true}]}).quests[0];
 assert.equal(quest.historyOnly,true);
 assert.equal(quest.state,'completed');
 assert.equal(M.normalizeQuest({id:'00000F12'}).historyOnly,false);
});

test('quest sorting supports pinned, alphabetical and category order safely',()=>{
 const items=M.normalizeSnapshot({ready:true,quests:[
  {id:'00000102',name:'Zulu',type:'Other',state:'active'},
  {id:'00000103',name:'Alpha',type:'Main',state:'active'},
  {id:'00000104',name:'Bravo',type:'Side',state:'active',pinned:true}
 ]}).quests;
 assert.deepEqual(M.filterQuests(items,{sort:'pinned'}).map(x=>x.name),['Bravo','Alpha','Zulu']);
 assert.deepEqual(M.filterQuests(items,{sort:'name'}).map(x=>x.name),['Alpha','Bravo','Zulu']);
 assert.deepEqual(M.filterQuests(items,{sort:'name-desc'}).map(x=>x.name),['Zulu','Bravo','Alpha']);
 assert.deepEqual(M.filterQuests(items,{sort:'category'}).map(x=>x.name),['Alpha','Bravo','Zulu']);
 assert.deepEqual(M.filterQuests(items,{sort:'unknown'}).map(x=>x.name),['Bravo','Alpha','Zulu']);
});


test('Pip-Boy sync adds game-tracked quests without losing manual HUD pins',()=>{
 const quests=[
  {id:'00000001',name:'Pinned',state:'active',pinned:true,vanillaTracked:false},
  {id:'00000002',name:'Pip',state:'active',pinned:false,vanillaTracked:true},
  {id:'00000003',name:'Archived',state:'completed',pinned:true,vanillaTracked:true},
  {id:'00000004',name:'Hidden',state:'active',pinned:true,hidden:true,vanillaTracked:true}
 ];
 assert.deepEqual(M.trackerQuests(quests,{trackerSyncPipboy:false}).map(q=>q.name),['Pinned']);
 assert.deepEqual(M.trackerQuests(quests,{trackerSyncPipboy:true}).map(q=>q.name),['Pinned','Pip']);
 assert.equal(quests[0].pinned,true);
 assert.equal(quests[1].pinned,false);
});
