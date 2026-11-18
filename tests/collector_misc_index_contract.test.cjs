'use strict';
const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');

test('duplicate Misc objective rows resolve the first quest card by a stable index',()=>{
 const source=fs.readFileSync('src/QuestCollector.cpp','utf8');
 assert.match(source,/#include <unordered_map>/);
 assert.match(source,/std::unordered_map<std::uint32_t,std::size_t> questIndex/);
 assert.match(source,/if\s*\(!seen\.insert\(id\)\.second\)/);
 assert.match(source,/questIndex\.find\(id\)/);
 assert.match(source,/auto& quest=dest\[previous->second\]/);
 assert.match(source,/questIndex\.emplace\(id,dest\.size\(\)-1\)/);
 assert.doesNotMatch(source,/std::find_if\(dest\.begin\(\),dest\.end\(\)/);
});
