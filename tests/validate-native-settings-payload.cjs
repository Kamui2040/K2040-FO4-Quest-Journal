'use strict';
// Exercise the exact C++ string that PrismaUI passes to window.receiveJournalSettings.
// This catches malformed JSON even when substring-only C++ fixtures pass.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const S = require('../view/journal-settings.js');

const records = fs.readFileSync(0, 'utf8').trim().split(/\r?\n/);
assert.equal(records.length, 7);
const parsed = records.map(raw => {
  const data = JSON.parse(raw); // Must never silently fall back to defaults.
  assert.equal(typeof data, 'object');
  const normal = S.normalize(raw);
  assert.deepEqual(normal.theme, data.theme);
  assert.deepEqual(normal.trackerAnchor, data.trackerAnchor);
  assert.deepEqual(normal.questListWidth, data.questListWidth);
  assert.deepEqual(normal.hudColor,data.hudColor);
  assert.deepEqual(normal.hudBackgroundColor,data.hudBackgroundColor);
  assert.deepEqual(normal.pipboyColor,data.pipboyColor);
  assert.equal(normal.journalTheme,data.journalTheme);
  assert.equal(normal.journalOpacity,data.journalOpacity);
  assert.equal(normal.journalFontSize,data.journalFontSize);
  assert.equal(normal.journalAccent,data.journalAccent);
  return {data, normal};
});
assert.equal(parsed[0].data.theme,'pipboy');
assert.equal(parsed[0].normal.journalTheme,'pipboy');
assert.equal(parsed[0].normal.trackerAnchor,'top-right');
assert.equal(parsed[1].data.theme,'contrast');
assert.equal(parsed[1].normal.trackerAnchor,'bottom-right');
assert.equal(parsed[1].normal.questListWidth,430);
assert.equal(parsed[1].normal.trackerWidth,260);
assert.equal(parsed[1].normal.opacity,71);
assert.equal(parsed[1].normal.trackerEnabled,false);
assert.equal(parsed[1].normal.trackerShowCompleted,true);
assert.equal(parsed[2].data.theme,'custom');
assert.equal(parsed[2].normal.accent,'#a1b2c3');
assert.equal(parsed[2].normal.trackerAnchor,'bottom-left');
assert.equal(parsed[3].normal.theme,'pipboy');
assert.equal(parsed[3].normal.hudColor,'#ff7733');
assert.equal(parsed[3].normal.hudBackgroundColor,'#0a0011');
assert.equal(parsed[3].normal.pipboyColor,'#b34299');
assert.deepEqual(S.palette(parsed[3].normal),{
  accent:'#b34299', text:'#b34299', completed:'#b34299', panel:'#0a0011'
});
assert.equal(parsed[4].normal.journalTheme,'custom');
assert.equal(parsed[4].normal.journalOpacity,79);
assert.equal(parsed[4].normal.journalFontSize,125);
assert.deepEqual(S.journalPalette(parsed[4].normal),{
  accent:'#b12345',text:'#eeeeaa',panel:'#111927',completed:'#a2e377'
});
assert.equal(S.palette(parsed[4].normal).accent,'#ff7733');
assert.equal(parsed[5].normal.theme,'fallout76');
assert.equal(parsed[5].normal.journalTheme,'fallout76');
assert.deepEqual(S.palette(parsed[5].normal),S.sharedPalettes.fallout76);
assert.deepEqual(S.journalPalette(parsed[5].normal),S.sharedPalettes.fallout76);
assert.equal(parsed[6].normal.trackerAutoPinNew,true);
assert.equal(parsed[6].normal.trackerSyncPipboy,true);
console.log('PASS: native JSON payload parses and retains independent journal/tracker appearance');
