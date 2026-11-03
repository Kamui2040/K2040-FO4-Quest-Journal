const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const sh = fs.readFileSync('scripts/compile-papyrus-og.sh', 'utf8');
const sanitize = fs.readFileSync('scripts/sanitize-papyrus-pas.py', 'utf8');
const main = fs.readFileSync('src/main.cpp', 'utf8');
const build = fs.readFileSync('xmake.lua', 'utf8');

test('Papyrus build requires explicit external game and staging directories', () => {
  assert.match(sh, /K2040_FO4_GAME_ROOT/);
  assert.match(sh, /K2040_PAPYRUS_STAGE/);
  assert.match(sh, /Stage must be a separate dedicated directory outside game\/repo/);
  assert.match(sh, /Creation Kit is not the verified 1\.10\.162\.0 depot build/);
  assert.match(sh, /6928748513006443409/);
});

test('official compiler produces only assembly before header sanitation', () => {
  assert.match(sh, /-asmonly/);
  assert.match(sh, /sanitize-papyrus-pas\.py/);
  assert.match(sh, /PapyrusAssembler\.exe/);
  assert.match(sh, /Private build metadata leaked/);
  assert.match(sanitize, /"source"/);
  assert.match(sanitize, /"user"/);
  assert.match(sanitize, /"computer"/);
});

test('Papyrus tooling cannot accidentally ship via production xmake or runtime', () => {
  assert.match(build, /add_files\("src\/\*\*\.cpp"\)/);
  assert.doesNotMatch(build, /research\/papyrus|PapyrusAssembler|compile-papyrus-og/);
  assert.doesNotMatch(main, /TrackingPapyrusWire|K2040QuestJournalWorker/);
  assert.match(sh, /cp -p -- "\$game\/Papyrus Compiler\/"\* "\$work\/tools\/"/);
  assert.doesNotMatch(sh, /(?:^|\n)\s*(?:mv|install|rsync)\s+[^\n]*\$game\//);
});
