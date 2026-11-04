/* Public-safe validated appearance preferences shared by both Prisma views. */
(function (root, factory) {
  const settings = factory();
  if (typeof module === 'object' && module.exports) module.exports = settings;
  if (root) root.K2040JournalSettings = settings;
})(typeof window !== 'undefined' ? window : undefined, function () {
  'use strict';
  const defaults = Object.freeze({
    theme:'pipboy', accent:'#66c4ff', text:'#f4faff', panel:'#0b1218', completed:'#74e398',
    opacity:94, fontSize:100, density:'comfortable', sidebarWidth:240, questListWidth:360,
    journalTheme:'pipboy', journalAccent:'#66c4ff', journalText:'#f4faff',
    journalPanel:'#0b1218', journalCompleted:'#74e398',
    journalOpacity:98, journalFontSize:100,
    trackerAnchor:'top-right', trackerScale:100, trackerWidth:340, trackerOpacity:88,
    trackerEnabled:true, trackerOverridePhosphor:false, trackerMaxQuests:6, trackerShowCompleted:false,
    trackerAutoPinNew:false, trackerSyncPipboy:false,
    journalHotkey:'Shift+G', trackerHotkey:'G', redirectQuestKey:false
  });
  // K2040 cross-mod palette contract. High Contrast is exactly the palette
  // used by Quick Attach Menu's renderer; Fallout 76 values are drawn from
  // MAP76's visible design tokens, not copied assets or runtime code.
  const sharedPalettes = Object.freeze({
    default:Object.freeze({accent:'#66c4ff',text:'#f4faff',panel:'#0b1218',completed:'#74e398'}),
    contrast:Object.freeze({accent:'#00e5ff',text:'#ffffff',panel:'#000000',completed:'#63ff9b'}),
    fallout76:Object.freeze({accent:'#f5cb5b',text:'#ffffcb',panel:'#181c1d',completed:'#e0c870'})
  });
  const themeChoices = ['commonwealth','pipboy','contrast','fallout76','custom'];
  const safeKeys=['Shift+G','G','F6','F7','F8','F10','F11','F12'];
  const anchors = ['top-right','top-left','bottom-right','bottom-left'];
  function number(value, fallback, min, max) {
    const n = Number(value);
    return Number.isFinite(n) ? Math.round(Math.min(max, Math.max(min, n))) : fallback;
  }
  function color(value, fallback) { return typeof value === 'string' && /^#[0-9a-fA-F]{6}$/.test(value) ? value.toLowerCase() : fallback; }
  function normalize(input) {
    let v = input;
    if (typeof v === 'string') { try { v = JSON.parse(v); } catch { v = {}; } }
    if (!v || typeof v !== 'object' || Array.isArray(v)) v = {};
    const choose = (value, allowed, fallback) => allowed.includes(value) ? value : fallback;
    const journalHotkey=choose(v.journalHotkey,safeKeys,defaults.journalHotkey);
    return {
      theme:choose(v.theme, themeChoices, defaults.theme),
      // Read-only game-derived HUD colors; never save in the user's INI.
      hudColor:color(v.hudColor, '#20ff48'),
      // Pip-Boy has its own in-game RGB sliders; fallback to HUD if unavailable.
      pipboyColor:color(v.pipboyColor, color(v.hudColor, '#20ff48')),
      hudBackgroundColor:color(v.hudBackgroundColor, '#001407'),
      accent:color(v.accent, defaults.accent), text:color(v.text, defaults.text), panel:color(v.panel, defaults.panel), completed:color(v.completed, defaults.completed),
      opacity:number(v.opacity, defaults.opacity, 25, 100),
      journalTheme:choose(v.journalTheme==='default'?'pipboy':v.journalTheme,['pipboy','dark','contrast','fallout76','custom'],defaults.journalTheme),
      journalAccent:color(v.journalAccent,defaults.journalAccent),
      journalText:color(v.journalText,defaults.journalText),
      journalPanel:color(v.journalPanel,defaults.journalPanel),
      journalCompleted:color(v.journalCompleted,defaults.journalCompleted),
      journalOpacity:number(v.journalOpacity,defaults.journalOpacity,25,100),
      journalFontSize:number(v.journalFontSize,defaults.journalFontSize,75,150),
      fontSize:number(v.fontSize, defaults.fontSize, 75, 150),
      density:choose(v.density, ['comfortable','compact'], defaults.density),
      sidebarWidth:number(v.sidebarWidth, defaults.sidebarWidth, 160, 380),
      questListWidth:number(v.questListWidth, defaults.questListWidth, 240, 560),
      trackerAnchor:choose(v.trackerAnchor, anchors, defaults.trackerAnchor),
      trackerScale:number(v.trackerScale, defaults.trackerScale, 75, 150),
      trackerWidth:number(v.trackerWidth, defaults.trackerWidth, 260, 520),
      trackerOpacity:number(v.trackerOpacity, defaults.trackerOpacity, 25, 100),
      trackerEnabled:v.trackerEnabled === false ? false : true,
      trackerOverridePhosphor:v.trackerOverridePhosphor === true,
      trackerMaxQuests:number(v.trackerMaxQuests, defaults.trackerMaxQuests, 1, 12),
      trackerShowCompleted:v.trackerShowCompleted === true,
      trackerAutoPinNew:v.trackerAutoPinNew === true,
      trackerSyncPipboy:v.trackerSyncPipboy === true,
      journalHotkey,
      trackerHotkey:choose(v.trackerHotkey,safeKeys,defaults.trackerHotkey)===journalHotkey ? (journalHotkey==='G'?'Shift+G':'G') : choose(v.trackerHotkey,safeKeys,defaults.trackerHotkey),
      redirectQuestKey:false // Reserved until a verified input interception is available.
    };
  }
  function palette(s) {
    if (s.theme==='pipboy') return {accent:s.pipboyColor,text:s.pipboyColor,panel:s.hudBackgroundColor,completed:s.pipboyColor};
    if (s.theme==='contrast') return {...sharedPalettes.contrast};
    if (s.theme==='fallout76') return {...sharedPalettes.fallout76};
    if (s.theme==='custom') return {accent:s.accent,text:s.text,panel:s.panel,completed:s.completed};
    return {...sharedPalettes.default};
  }
  // Journal + its in-place Settings share one palette, independent of
  // the mini-tracker. The live game HUD colors are sampled in native code.
  function journalPalette(s) {
    if(s.journalTheme==='pipboy') return {
      accent:s.pipboyColor,text:s.pipboyColor,
      panel:s.hudBackgroundColor,completed:s.pipboyColor
    };
    if(s.journalTheme==='contrast') return {...sharedPalettes.contrast};
    if(s.journalTheme==='fallout76') return {...sharedPalettes.fallout76};
    if(s.journalTheme==='custom')
      return {accent:s.journalAccent,text:s.journalText,
              panel:s.journalPanel,completed:s.journalCompleted};
    return {...sharedPalettes.default};
  }
  function hexChannels(hex) {
    if (typeof hex!=='string' || !/^#[a-f\d]{6}$/i.test(hex)) return null;
    return [1,3,5].map(offset=>Number.parseInt(hex.slice(offset,offset+2),16));
  }
  function channelsHex(channels) {
    if(!Array.isArray(channels) || channels.length!==3 ||
       channels.some(v=>!Number.isInteger(v) || v<0 || v>255))return null;
    return '#'+channels.map(v=>v.toString(16).padStart(2,'0')).join('');
  }
  return {defaults,normalize,palette,journalPalette,sharedPalettes,themeChoices,anchors,hexChannels,channelsHex};
});
