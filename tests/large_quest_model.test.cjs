const test=require('node:test');
const assert=require('node:assert/strict');
const model=require('../view/quest-model.js');
function many(n=2048){
 return {ready:true,quests:Array.from({length:n},(_,i)=>({
   id:(i+1).toString(16).toUpperCase().padStart(8,'0'),
   name:'Quest '+i,
   type:i%7===0?'Misc':i%3===0?'Modded':'Side',
   state:i%11===0?'completed':'active',
   nativeInstanceID:0,nativeInstanceStatus:'consistent',
   trackingScope:'quest-wide',trackingReadiness:'candidate-read-only',
   nativeDisplayedCount:1,vanillaTracked:i%13===0,pinned:i%9===0,
   objectives:[{index:1,text:'Objective '+i,state:'active'}]
 }))};
}
test('2,048 simulated quests normalize uniquely with native evidence intact',()=>{
 const raw=many();raw.quests.push(raw.quests[0],{id:'not a form id'});
 const snapshot=model.normalizeSnapshot(JSON.stringify(raw));
 assert.equal(snapshot.quests.length,2048);
 assert.equal(new Set(snapshot.quests.map(q=>q.id)).size,2048);
 assert.equal(snapshot.quests[0].nativeInstanceID,0);
 assert.equal(snapshot.quests[0].objectives[0].state,'active');
});
test('large categories, search and passive tracker do not mutate Pip-Boy bits',()=>{
 const snapshot=model.normalizeSnapshot(many());
 const before=snapshot.quests.map(q=>q.vanillaTracked);
 const categories=model.listCategories(snapshot.quests);
 assert.equal(categories.reduce((sum,c)=>sum+c.count,0),2048);
 const filtered=model.filterQuests(snapshot.quests,{category:'Modded',status:'active',sort:'name',
   hidden:new Set(),favorites:new Set(),includeHidden:true});
 assert.ok(filtered.length>0 && filtered.every(q=>q.state==='active'&&q.type==='Modded'));
 const tracker=model.trackerQuests(snapshot.quests,{trackerSyncPipboy:true});
 assert.ok(tracker.length>0 && tracker.every(q=>q.pinned||q.vanillaTracked));
 assert.deepEqual(snapshot.quests.map(q=>q.vanillaTracked),before);
});
