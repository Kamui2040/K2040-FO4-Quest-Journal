/* Dependency-free quest view model. No engine mutations or network access. */
(function (root, factory) {
  const model = factory();
  if (typeof module === 'object' && module.exports) module.exports = model;
  if (root) root.K2040QuestModel = model;
})(typeof window !== 'undefined' ? window : undefined, function () {
  'use strict';
  const ORDER = ['Main', 'Minutemen', 'Brotherhood', 'Railroad', 'Institute', 'Side', 'Misc', 'Automatron', 'Far Harbor', 'Nuka-World', 'Wasteland Workshop', 'Modded', 'Other'];
  // Category order is immutable; avoid rescanning ORDER in every sort comparison.
  const CATEGORY_RANK = new Map(ORDER.map((name,index)=>[name,index]));
  const USER_CATEGORIES = ['Automatic','Main','Minutemen','Brotherhood','Railroad','Institute',
    'Side','Misc','Automatron','Far Harbor','Nuka-World','Wasteland Workshop','Modded','Other'];
  const STATES = ['active', 'completed', 'failed'];
  const CATEGORY_ALIASES = Object.freeze({
    Brotherhood: 'Brotherhood', Institute: 'Institute', Minutemen: 'Minutemen', Railroad: 'Railroad',
    Automatron: 'Automatron', FarHarbor: 'Far Harbor', WastelandWorkshop: 'Wasteland Workshop',
    NukaWorld: 'Nuka-World', Miscellaneous: 'Misc', Misc: 'Misc', Main: 'Main', Side: 'Side'
  });
  function asText(value, fallback = '') { return typeof value === 'string' ? value : fallback; }
  function normalizeObjective(value) {
    if (!value || typeof value !== 'object') return null;
    const state = ['inactive', 'active', 'completed', 'failed'].includes(value.state) ? value.state : 'inactive';
    return { index: Number.isInteger(value.index) ? value.index : 0, text: asText(value.text, 'Unnamed objective'), state };
  }
  function normalizeQuest(value) {
    if (!value || typeof value !== 'object' || !/^[0-9A-Fa-f]{8}$/.test(String(value.id || ''))) return null;
    const state = STATES.includes(value.state) ? value.state : 'active';
    const originalType = asText(value.type, 'Other');
    return {
      id: String(value.id).toUpperCase(),
      name: asText(value.name).trim() || 'Untitled quest',
      description: asText(value.description),
      type: (Object.prototype.hasOwnProperty.call(CATEGORY_ALIASES, originalType) ? CATEGORY_ALIASES[originalType] : originalType) || 'Other',
      sourceType: asText(value.sourceType, originalType),
      categoryOverride: value.categoryOverride === true,
      state,
      vanillaTracked: value.vanillaTracked === true,
      pinned: value.pinned === true,
      favorite: value.favorite === true,
      hidden: value.hidden === true,
      historyOnly: value.historyOnly === true,
      isMisc: value.isMisc === true,
      trackingScope: value.trackingScope === 'quest-wide' ? 'quest-wide' : 'unknown',
      miscRowCount: Number.isInteger(value.miscRowCount) && value.miscRowCount >= 0 ? Math.min(value.miscRowCount,512) : 0,
      nativeDisplayedCount: Number.isInteger(value.nativeDisplayedCount) && value.nativeDisplayedCount >= 0 ? Math.min(value.nativeDisplayedCount,512) : 0,
      nativeInstanceID: Number.isInteger(value.nativeInstanceID) && value.nativeInstanceID >= 0 && value.nativeInstanceID <= 0xffffffff
        ? value.nativeInstanceID : null,
      nativeInstanceStatus: ['consistent','multiple-instances','instance-mismatch','no-displayed-objectives','unavailable'].includes(value.nativeInstanceStatus)
        ? value.nativeInstanceStatus : 'unavailable',
      pipboyFeedAgreement: ['match','mismatch','missing'].includes(value.pipboyFeedAgreement)
        ? value.pipboyFeedAgreement : 'missing',
      // Display-only. This value never grants the UI or native plugin write
      // permission; a selected quest must be revalidated on an F4SE task.
      trackingReadiness: [
        'invalid-quest','not-active','unknown-scope','instance-unverified',
        'native-objectives-unavailable','misc-feed-unavailable',
        'pipboy-feed-unverified','candidate-read-only'
      ].includes(value.trackingReadiness) ? value.trackingReadiness : 'unknown-scope',
      objectives: Array.isArray(value.objectives) ? value.objectives.map(normalizeObjective).filter(Boolean) : []
    };
  }
  // Category overrides change only presentation, never native tracking safety.
  function nativeTrackEligibility(quest) {
    if (!quest || quest.historyOnly || quest.state !== 'active')
      return {allowed:false,reason:'unavailable'};
    if (quest.isMisc || quest.sourceType === 'Misc')
      return {allowed:false,reason:'misc-objective'};
    return {allowed:true,reason:'eligible'};
  }
  function normalizeSnapshot(input) {
    const object = typeof input === 'string' ? JSON.parse(input) : input;
    if (!object || typeof object !== 'object') return { ready: false, quests: [] };
    const ids = new Set();
    const quests = [];
    for (const raw of Array.isArray(object.quests) ? object.quests : []) {
      const quest = normalizeQuest(raw);
      if (!quest || ids.has(quest.id)) continue;
      ids.add(quest.id);
      quests.push(quest);
    }
    return { ready: object.ready === true, quests };
  }
  function listCategories(quests) {
    const counts = new Map();
    for (const quest of quests) counts.set(quest.type, (counts.get(quest.type) || 0) + 1);
    return [...counts.entries()].sort(([a], [b]) => {
      const ia = ORDER.indexOf(a), ib = ORDER.indexOf(b);
      return (ia < 0 ? 100 : ia) - (ib < 0 ? 100 : ib) || a.localeCompare(b);
    }).map(([name, count]) => ({ name, count }));
  }
  function filterQuests(quests, filters) {
    const f = filters || {};
    const query = asText(f.query).trim().toLocaleLowerCase();
    return quests.filter(q => {
      if (f.status && f.status !== 'all' && q.state !== f.status) return false;
      if (f.category && f.category !== 'all' && q.type !== f.category) return false;
      if (!f.includeHidden && f.hidden && f.hidden.has(q.id)) return false;
      if (f.favoritesOnly && (!f.favorites || !f.favorites.has(q.id))) return false;
      // Avoid an objective-text array allocation for every quest on each keystroke.
      return !query || q.name.toLocaleLowerCase().includes(query) ||
        q.description.toLocaleLowerCase().includes(query) ||
        q.type.toLocaleLowerCase().includes(query) ||
        q.objectives.some(o => o.text.toLocaleLowerCase().includes(query));
    }).sort((a,b) => {
      if(f.sort === 'category') {
        const rankA = CATEGORY_RANK.get(a.type) ?? ORDER.length;
        const rankB = CATEGORY_RANK.get(b.type) ?? ORDER.length;
        const byCategory = rankA - rankB || a.type.localeCompare(b.type);
        if(byCategory) return byCategory;
      } else if(f.sort !== 'name' && f.sort !== 'name-desc') {
        // In the common pinned sort, avoid costly name comparisons between groups.
        const byPin = Number(b.pinned) - Number(a.pinned);
        if(byPin) return byPin;
      }
      const byName = a.name.localeCompare(b.name) || a.id.localeCompare(b.id);
      return f.sort === 'name-desc' ? -byName : byName;
    });
  }
  function trackerQuests(quests,settings) {
    const sync=!!(settings && settings.trackerSyncPipboy);
    return quests.filter(q=>q.state==='active' && !q.hidden && !q.historyOnly &&
      (q.pinned || (sync && q.vanillaTracked)));
  }
  function selectObjectives(quest) {
    return quest.objectives.filter(o => o.state === 'active' || o.state === 'completed');
  }
  return { ORDER, STATES, USER_CATEGORIES, normalizeQuest, normalizeSnapshot, nativeTrackEligibility, listCategories, filterQuests, trackerQuests, selectObjectives };
});
