const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');

test('save/load reset clears the visible search field as well as query state',()=>{
  const source=fs.readFileSync(path.join(__dirname,'..','view','app.js'),'utf8');
  const body=source.match(/function resetQuestView\(\)\s*\{([\s\S]*?)\n  \}/)?.[1];
  assert.ok(body,'resetQuestView must be defined');
  assert.match(body,/state\.query=''/);
  assert.match(body,/el\('questSearch'\)\.value=''/);
});
