/* K2040 Quest Journal / Prisma UI view logic. No external scripts or networking. */
(function () {
  'use strict';
  const M = window.K2040QuestModel;
  const S = window.K2040JournalSettings;
  if (!S) throw new Error('Missing journal-settings.js');
  if (!M) throw new Error('Missing quest-model.js');
  const isTracker = document.body.classList.contains('tracker-page');
  // Matches the shared K2040 hub theme tokens; these are transient UI
  // colors, never changes to the player's two saved palettes.
  const sharedThemes=Object.freeze({
    default:{accent:'#88c4f1',text:'#e3eaf2',panel:'#1a2938',bg:'#101923',muted:'#b0c0ce'},
    midnight:{accent:'#b4a8ff',text:'#e3eaf2',panel:'#171a31',bg:'#090d18',muted:'#b0c0ce'},
    'high-contrast':{accent:'#ffff73',text:'#ffffff',panel:'#171717',bg:'#050505',muted:'#f1f1f1'},
    pipboy:{accent:'#93f694',text:'#b2ffb5',panel:'#122b18',bg:'#09150d',muted:'#9bd69d'}
  });
  const state = {
    snapshot: {ready:false, quests:[]}, query:'', status:'active', category:'all', sort:'pinned', selected:null,
    favorites:new Set(), hidden:new Set(), includeHidden:false, favoritesOnly:false,
    demo: new URLSearchParams(window.location.search).get('demo') === '1', lastSnapshotRaw:null,
    settings: S.normalize({}), unifiedTheme:null, unifiedAppearance:null,
    layoutPreview:null,lastTrackerMetrics:null,
    trackingRequest:null, trackingNotice:'',
    qaPreviewEnabled:false, qaPreviewPending:null, qaLastSelected:null,
    qaWriteEnabled:false, qaWriteArmed:false, qaWriteArmedId:null, qaWriteAttempted:false,
    qaWritePending:null,qaGenericMiscAvailable:false,
    candidateAvailable:false,candidateStatus:'off',candidatePending:null,
    repeatableStatus:'off',repeatableAvailable:false,
    repeatableConsentPending:false,repeatablePending:null,
    qaIntegratedAvailable:false, qaIntegratedConsent:false, qaIntegratedPending:null,
    qaIntegratedRemaining:8, qaMiscPending:null, qaMiscSummary:'',
    qaDispatchPending:null, qaDispatchUsed:false, qaDispatchSummary:'',
    qaSingleAvailable:false, qaSingleArmedId:null, qaSingleUsed:false,
    qaSingleArmPending:false, qaSinglePending:null,
    singleRcAvailable:false, singleRcUsed:false, singleRcPending:null,
    dogmeatProdAvailable:false,
    selectorReadOnlyAvailable:false,selectorReadOnlyUsed:false,
    selectorReadOnlyPending:null,selectorReadOnlySummary:''
  };
  const el = id => document.getElementById(id);
  // Gameplay shortcuts are owner settings edited in the Unified Settings
  // hub, never stored/registered as extra MCM keybinds.
  const html = value => String(value == null ? '' : value).replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
  const prettyState = s => ({active:'Active',completed:'Completed',failed:'Failed',inactive:'Not started'})[s] || 'Unknown';
    const channels = hex => hex.slice(1).match(/.{2}/g).map(x=>parseInt(x,16)).join(',');
  const emit = (name, value='') => { if (typeof window[name] === 'function') window[name](String(value)); };
  const visualThemes=new Set(['fallout4','fallout76','doom-eternal','fallout3-newvegas']);
  const builtInPalettes=new Set([
    'default','fallout76','radioactive-green','monochrome','midnight-blue','hot-omni'
  ]);
  const colorRoles=['background','panel','text','accent','muted','border','highlight'];
  const hexColor=/^#[0-9a-fA-F]{6}$/;
  const styles=['fallout4','fallout76','doom-eternal','fallout3-newvegas'];
  function applyUnifiedStyleClass(){
    const current=state.unifiedAppearance?.theme;
    const list=document.body.classList;
    for(const name of styles){
      const label='k2040-style-'+name;
      // Ultralight builds differ in DOMTokenList.toggle(force) support.
      // Explicit add/remove keeps frame styling in sync with palette tokens.
      if(current===name){
        if(typeof list.add==='function')list.add(label);
        else if(typeof list.toggle==='function')list.toggle(label,true);
      }else{
        if(typeof list.remove==='function')list.remove(label);
        else if(typeof list.toggle==='function')list.toggle(label,false);
      }
    }
  }
  function readAppearance(input){
    const raw=typeof input==='string'?input:JSON.stringify(input);
    if(raw==='native')return null;
    if(raw.length>4095)return undefined;
    try{
      const data=JSON.parse(raw);
      if(!data||data.schema!==1||!visualThemes.has(data.theme)||
         !(builtInPalettes.has(data.palette)||/^user-[0-9]{1,19}$/.test(data.palette))||
         !data.colors||typeof data.colors!=='object'||Array.isArray(data.colors)||
         colorRoles.some(role=>typeof data.colors[role]!=='string'||
           !hexColor.test(data.colors[role])))return undefined;
      return data;
    }catch{return undefined;}
  }
  function validTrackerGeometry(g){
    if(!g || typeof g!=='object'||Array.isArray(g))return false;
    const values=['x','y','width','height'].map(key=>g[key]);
    return values.every(v=>typeof v==='number'&&Number.isFinite(v)) &&
      g.x>=0&&g.y>=0&&g.width>.015&&g.height>.015 &&
      g.x+g.width<=1.000001&&g.y+g.height<=1.000001;
  }
  function ownerTrackerGeometry(s){
    if(s.trackerLayoutEnabled!==true)return null;
    const result={x:s.trackerLayoutX/10000,y:s.trackerLayoutY/10000,
      width:s.trackerLayoutWidth/10000,height:s.trackerLayoutHeight/10000};
    return validTrackerGeometry(result)?result:null;
  }
  function positionTracker(s){
    const tracker=el('tracker');
    if(!tracker)return;
    const edit=state.layoutPreview||ownerTrackerGeometry(s);
    if(!edit){
      // rem-based tracker labels must return to the default owner text scale.
      document.documentElement.style.fontSize=(14*s.trackerScale/100).toFixed(3)+'px';
      const [vertical,horizontal]=s.trackerAnchor.split('-');
      tracker.style.top=vertical==='top'?'5%':'auto';
      tracker.style.bottom=vertical==='bottom'?'5%':'auto';
      tracker.style.left=horizontal==='left'?'3%':'auto';
      tracker.style.right=horizontal==='right'?'3%':'auto';
      for(const property of ['width','height','maxHeight','fontSize'])
        tracker.style[property]='';
    }else{
      tracker.style.top=(edit.y*100).toFixed(4)+'vh';
      tracker.style.left=(edit.x*100).toFixed(4)+'vw';
      tracker.style.bottom='auto';tracker.style.right='auto';
      tracker.style.width=(edit.width*100).toFixed(4)+'vw';
      tracker.style.height=(edit.height*100).toFixed(4)+'vh';
      tracker.style.maxHeight=(edit.height*100).toFixed(4)+'vh';
      const viewportWidth=Number(window.innerWidth||document.documentElement.clientWidth)||1920;
      const ratio=edit.width*viewportWidth/s.trackerWidth;
      const size=(14*s.trackerScale/100*Math.min(3,Math.max(.35,ratio))).toFixed(3)+'px';
      tracker.style.fontSize=size;
      document.documentElement.style.fontSize=size; // rem-based labels also resize
    }
    tracker.classList?.toggle?.('layout-edited',Boolean(edit));
  }
  // The hub owns all selection and input. Only validated geometry is applied.
  window.applyJournalTrackerLayout=function(input){
    if(!isTracker)return;
    if(input==='native')state.layoutPreview=null;
    else{
      let next;
      try{next=typeof input==='string'?JSON.parse(input):input;}catch{return;}
      if(!validTrackerGeometry(next))return;
      state.layoutPreview={x:next.x,y:next.y,width:next.width,height:next.height};
    }
    positionTracker(state.settings);
  };
  function reportTrackerMetrics(){
    if(!isTracker||state.demo||state.layoutPreview||
       state.settings.trackerLayoutEnabled||
       typeof window.reportTrackerLayoutMetrics!=='function')return;
    const tracker=el('tracker');
    const width=Number(window.innerWidth||document.documentElement.clientWidth);
    const height=Number(window.innerHeight||document.documentElement.clientHeight);
    if(!tracker||typeof tracker.getBoundingClientRect!=='function'||
       !(width>0&&height>0))return;
    const rect=tracker.getBoundingClientRect();
    const geometry={x:rect.left/width,y:rect.top/height,
      width:rect.width/width,height:rect.height/height};
    if(!validTrackerGeometry(geometry))return;
    const previous=state.lastTrackerMetrics;
    if(previous&&Object.keys(geometry).every(key=>
        Math.abs(previous[key]-geometry[key])<0.002))return;
    state.lastTrackerMetrics=geometry;
    emit('reportTrackerLayoutMetrics',JSON.stringify(geometry));
  }
  function applySettings() {
    const s=state.settings, trackerPalette=S.palette(s), root=document.documentElement;
    // Full-screen journal and its in-place settings share one saved theme.
    // The compact tracker remains independently configurable.
    const nativePalette=isTracker?trackerPalette:S.journalPalette(s);
    const next=state.unifiedAppearance;
    const nextColors=next?.colors;
    const shared=nextColors
      ? {accent:nextColors.accent,text:nextColors.text,panel:nextColors.panel,
         bg:nextColors.background,muted:nextColors.muted,
         completed:nextColors.highlight,border:nextColors.border,
         highlight:nextColors.highlight}
      : state.unifiedTheme?sharedThemes[state.unifiedTheme]:null;
    const p=shared?{completed:shared.accent,...shared}:nativePalette;
    const rgb=channels(p.panel);
    root.style.fontSize=isTracker?'14px':(14*s.journalFontSize/100)+'px';
    // Native palette always wins when the hub is missing or unavailable.
    // Unlike body-scoped CSS, these root tokens override earlier inline
    // native properties and therefore affect both Prisma views immediately.
    if(shared){
      root.style.setProperty('--bg',shared.bg);
      root.style.setProperty('--blue',shared.accent);
      root.style.setProperty('--text',shared.text);
      root.style.setProperty('--muted',shared.muted);
      if(nextColors){
        root.style.setProperty('--edge',shared.border);
        root.style.setProperty('--gold',shared.highlight);
      }else{
        root.style.removeProperty?.('--edge');
        root.style.removeProperty?.('--gold');
      }
    }else{
      for(const key of ['--bg','--blue','--text','--muted','--edge','--gold'])
        root.style.removeProperty?.(key);
    }
    root.style.setProperty('--ui-accent-rgb',channels(p.accent));
    root.style.setProperty('--ui-accent',p.accent);
    root.style.setProperty('--ui-text',p.text);
    root.style.setProperty('--ui-popup-background',p.panel);
    root.style.setProperty('--ui-completed',p.completed);
    root.style.setProperty('--journal-sidebar-width',s.sidebarWidth+'px');
    root.style.setProperty('--journal-list-width',s.questListWidth+'px');
    root.style.setProperty('--ui-panel-rgb',rgb);
    root.style.setProperty('--ui-panel-background',isTracker
      ? 'rgba('+rgb+','+(s.trackerOpacity/100)+')'
      : 'rgba('+rgb+','+(s.journalOpacity/100)+')');
    root.style.setProperty('--tracker-background','rgba('+rgb+','+(s.trackerOpacity/100)+')');
    root.style.setProperty('--tracker-width',s.trackerWidth+'px');
    root.style.setProperty('--tracker-scale',String(s.trackerScale/100));
    root.style.setProperty('--tracker-opacity',String(s.trackerOpacity));
    if (isTracker) {
      document.body.className='tracker-page anchor-'+s.trackerAnchor+
        (s.theme==='pipboy'?' theme-pipboy':s.theme==='fallout76'?' theme-fallout76':s.theme==='contrast'?' theme-contrast':'');
      positionTracker(s);
      applyUnifiedStyleClass();
      return;
    }
    // Apply the journal's own palette to both workspaces and settings;
    // do not derive it from the separately themed mini-tracker.
    document.body.classList.toggle('theme-pipboy',s.journalTheme==='pipboy');
    document.body.classList.toggle('journal-theme-dark',s.journalTheme==='dark');
    document.body.classList.toggle('journal-theme-fallout76',s.journalTheme==='fallout76');
    document.body.classList.toggle('journal-theme-contrast',s.journalTheme==='contrast');
    applyUnifiedStyleClass();
    document.querySelector('.journal-shell').classList.toggle('compact',s.density==='compact');
  }
  function receiveJournalSettings(input) {
    let settings=input;
    if(typeof input==='string'){
      try { settings=JSON.parse(input); }
      catch(err){
        // A native payload format error must never overwrite the active
        // appearance with browser defaults; keep the last valid settings.
        console.error('Malformed native Quest Journal settings JSON',err);
        return;
      }
    }
    state.settings=S.normalize(settings);
    if(!isTracker){
      const checkbox=el('pipboySyncToggle');
      if(checkbox) {
        checkbox.checked=state.settings.trackerSyncPipboy;
        checkbox.disabled=false;
      }
    }
    applySettings();
    // The passive tracker often receives unchanged quest JSON. Its snapshot
    // cache suppresses repeated render() calls, so a new visible quest limit
    // or Pip-Boy mirror setting must redraw from the observed read-only
    // snapshot immediately, without waiting for another quest update.
    if(isTracker)renderTracker();
    else window.K2040JournalSelects?.sync();
  }
  window.receiveJournalPipboySyncStatus=function(status){
    if(isTracker)return;
    const box=el('pipboySyncToggle'),notice=el('pipboySyncStatus');
    if(box) {
      box.disabled=false;
      box.checked=state.settings.trackerSyncPipboy;
    }
    if(notice)notice.textContent=status==='saved'?'Preference saved.':
      status==='storage'?'Unable to save; previous setting restored.':
      'Invalid request; previous setting restored.';
  };
  // The only settings entry goes to the dedicated page in the shared hub.
  // If the hub is absent/old, preserve the Journal and explain why.
  window.receiveJournalHubOpenResult=function(reason){
    if(isTracker)return;
    const status=el('settingsHubStatus');
    if(!status)return;
    status.textContent=reason==='unavailable'
      ?'Install or update K2040 Unified Settings to open Quest Journal settings.'
      :'Unable to open the Unified Settings page.';
    status.hidden=false;
  };
  // Escape always closes the Journal; no separate settings panel exists.
  window.handleJournalEscape=function(){
    if(!isTracker)emit('requestCloseJournal');
  };
  let controllerFocused=null;
  function clearControllerFocus(){
    controllerFocused?.classList.remove('controller-focus-visible');
    controllerFocused=null;
  }
  function visibleControllerControls(){
    const panel=el('journalWorkspace');
    // The Settings action lives in the header, outside journalWorkspace.
    // Include header buttons in the same Fallout-mapped controller navigation;
    // preserve the existing workspace-first default when nothing has focus.
    const workspace=Array.from(panel.querySelectorAll('button,input:not([type="hidden"]),select,textarea,[tabindex]'));
    const header=['resetFilters','openSettings','closeJournal'].map(el).filter(Boolean);
    return [...workspace,...header].filter(node=>!node.disabled&&!node.hidden&&node.tabIndex>=0&&
        node.getAttribute?.('aria-hidden')!=='true'&&node.getAttribute?.('aria-disabled')!=='true'&&
        node.getClientRects().length>0);
  }
  function moveControllerFocus(direction){
    const controls=visibleControllerControls();
    if(!controls.length)return;
    const current=document.activeElement;
    if(!controls.includes(current)){
      focusControllerControl(controls[0]);return;
    }
    if(current.type==='range'&&(direction==='left'||direction==='right')){
      const step=current.step==='any'?1:Number(current.step)||1;
      const min=current.min===''?-Infinity:Number(current.min);
      const max=current.max===''?Infinity:Number(current.max);
      const delta=direction==='right'?step:-step;
      const next=Math.min(max,Math.max(min,Number(current.value)+delta));
      if(next!==Number(current.value)){
        current.value=String(next);
        current.dispatchEvent(new Event('input',{bubbles:true}));
        current.dispatchEvent(new Event('change',{bubbles:true}));
      }
      return;
    }
    const origin=current.getBoundingClientRect();
    const cx=origin.left+origin.width/2,cy=origin.top+origin.height/2;
    const horizontal=direction==='left'||direction==='right';
    const sign=direction==='left'||direction==='up'?-1:1;
    let best=null,bestScore=Infinity;
    for(const node of controls){
      if(node===current)continue;
      const rect=node.getBoundingClientRect();
      const dx=rect.left+rect.width/2-cx,dy=rect.top+rect.height/2-cy;
      const primary=(horizontal?dx:dy)*sign;
      if(primary<=0)continue;
      const cross=Math.abs(horizontal?dy:dx);
      const score=primary+cross*1.5;
      if(score<bestScore){best=node;bestScore=score;}
    }
    if(best)focusControllerControl(best);
  }
  function focusControllerControl(node){
    clearControllerFocus();
    controllerFocused=node;
    node.classList.add('controller-focus-visible');
    node.focus();
    node.scrollIntoView({block:'nearest',inline:'nearest'});
  }
  window.receiveJournalControllerAction=function(action){
    if(isTracker)return;
    if(action==='back'){
      if(document.querySelector('.k2040-select-menu')){
        window.K2040JournalSelects?.close(true);return;
      }
      window.handleJournalEscape();return;
    }
    if(action==='activate'){
      const target=document.activeElement;
      const popup=document.querySelector('.k2040-select-menu');
      if(target&&((popup&&popup.contains(target)&&!target.disabled)||visibleControllerControls().includes(target)))target.click();
      return;
    }
    if(['up','down','left','right'].includes(action)){
      if(document.querySelector('.k2040-select-menu')){
        const key='Arrow'+action[0].toUpperCase()+action.slice(1);
        document.dispatchEvent(new KeyboardEvent('keydown',{key,bubbles:true,cancelable:true}));
        return;
      }
      moveControllerFocus(action);
    }
  };
  if(!isTracker){
    document.addEventListener('pointerdown',clearControllerFocus,true);
    document.addEventListener('pointermove',clearControllerFocus,true);
    document.addEventListener('keydown',clearControllerFocus,true);
  }
  // Unified appearance is a transient token overlay, never a native
  // preference update. Both journal and tracker retain their own settings.
  window.applyUnifiedTheme=function(theme){
    // "native" is used internally when the optional hub disappears.
    if(theme!=='native' && !Object.prototype.hasOwnProperty.call(sharedThemes,theme))return;
    state.unifiedTheme=theme==='native'?null:theme;
    applySettings();
  };
  window.applyUnifiedAppearance=function(input){
    const next=readAppearance(input);
    if(next===undefined)return; // reject invalid hub data without changing UI
    state.unifiedAppearance=next;
    applySettings();
  };
  window.receiveJournalSettings=receiveJournalSettings;
  function stats() {
    return {all:state.snapshot.quests.length,
      active:state.snapshot.quests.filter(q=>q.state==='active').length,
      completed:state.snapshot.quests.filter(q=>q.state==='completed').length,
      failed:state.snapshot.quests.filter(q=>q.state==='failed').length};
  }
  function filtered() { return M.filterQuests(state.snapshot.quests, state); }
  function renderJournal() {
    if (isTracker) return;
    el('connection').textContent = state.demo ? 'Demo data' : state.snapshot.ready ? 'Connected to Fallout 4' : 'Awaiting game data';
    el('connection').classList.toggle('connected', state.snapshot.ready);
    const totals = stats();
    el('statusFilters').innerHTML = [
      ['active','Active',totals.active],['completed','Completed',totals.completed],
      ['failed','Failed',totals.failed],['all','All quests',totals.all]
    ].map(([key,label,total]) => `<button class="filter-button ${state.status===key?'selected':''}" data-status="${key}"><span>${label}</span><span class="count">${total}</span></button>`).join('')+
      `<button class="filter-button ${state.favoritesOnly?'selected':''}" data-favorites="1"><span>Favorites</span><span>★</span></button>`+
      `<button class="filter-button ${state.includeHidden?'selected':''}" data-hidden="1"><span>Show hidden</span><span>${state.hidden.size}</span></button>`;
    const categoryList = M.listCategories(state.snapshot.quests);
    el('categories').innerHTML = `<button class="filter-button ${state.category==='all'?'selected':''}" data-category="all"><span>All categories</span><span class="count">${totals.all}</span></button>`+
      categoryList.map(({name,count}) => `<button class="filter-button ${state.category===name?'selected':''}" data-category="${html(name)}"><span>${html(name)}</span><span class="count">${count}</span></button>`).join('');
    el('listHeading').textContent = state.category === 'all' ? (state.status === 'all' ? 'All quests' : prettyState(state.status)+' quests') : state.category;
    const visible = filtered();
    // A hidden or filtered-out quest must not remain visible in the detail pane.
    if(!visible.some(q=>q.id===state.selected))state.selected=visible[0]?.id||null;
    el('resultCount').textContent = visible.length;
    const list = el('questList');
    list.innerHTML = visible.length ? visible.map(q=>`<button role="option" aria-selected="${state.selected===q.id}" class="quest-row ${state.selected===q.id?'current':''}" data-quest="${q.id}">
      <span class="quest-row-title">${html(q.name)} ${q.pinned?'<span class="pin-indicator" title="Pinned to HUD">◆</span>':''}</span>
      <span class="quest-row-sub">${html(q.type)} <span>·</span> ${prettyState(q.state)} ${q.vanillaTracked?'<span class="vanilla-indicator">· Map tracked</span>':''}</span>
    </button>`).join('') : `<div class="empty-list">${state.snapshot.ready ? 'No quests match these filters.' : 'Waiting for a Fallout 4 save to provide quest data.'}</div>`;
    const selected = visible.find(q=>q.id===state.selected);
    if (!selected) {
      el('detailPane').innerHTML='<div class="detail-empty"><div class="empty-symbol">✧</div><h2>Select a quest</h2><p>Choose a quest to see its journal entry and objectives.</p></div>';
      return;
    }
    const objectives = selected.objectives;
    const nativeTracking = M.nativeTrackEligibility(selected);
    const genericMiscQAReady = state.qaGenericMiscAvailable && selected.isMisc &&
      selected.trackingScope==='quest-wide' && selected.nativeInstanceStatus==='consistent' &&
      selected.nativeInstanceID!==null && selected.pipboyFeedAgreement==='match' &&
      selected.miscRowCount>0;
    const repeatableEligible=selected.isMisc && !selected.historyOnly &&
      selected.state==='active' && selected.trackingScope==='quest-wide' &&
      selected.nativeInstanceStatus==='consistent' && selected.nativeInstanceID!==null &&
      selected.pipboyFeedAgreement==='match' && selected.miscRowCount>0;
    const questWideCandidateEligible=!selected.historyOnly &&
      selected.state==='active' && selected.trackingScope==='quest-wide' &&
      selected.nativeInstanceStatus==='consistent' && selected.nativeInstanceID!==null &&
      selected.nativeDisplayedCount>0 && selected.pipboyFeedAgreement==='match' &&
      selected.trackingReadiness==='candidate-read-only' &&
      (!selected.isMisc || selected.miscRowCount>0);
    const oneShotScopeReady = state.qaGenericMiscAvailable
      ? genericMiscQAReady : !selected.isMisc; // never repeat ordinary QA in the generic flavor
    el('detailPane').innerHTML = `<div class="quest-details">
      <div class="detail-top"><span class="eyebrow">${html(selected.type.toUpperCase())} / ${prettyState(selected.state).toUpperCase()}</span><span class="quest-id">FORM ${html(selected.id)}</span></div>
      <h2>${html(selected.name)}</h2>
      ${selected.isMisc ? '<p class="muted">Vanilla Pip-Boy tracking applies to the owning quest, not to individual Misc objectives. '+
        'Displayed Misc rows: '+selected.miscRowCount+'. Native instance: '+html(selected.nativeInstanceStatus)+
        '; Pip-Boy agreement: '+html(selected.pipboyFeedAgreement)+'. '+
        'Read-only native preflight: '+html(selected.trackingReadiness)+
        '. A valid preflight does not enable the shipping tracking writer.</p>' : ''}
      <div class="detail-actions"><button class="primary-button" data-action="pin">${selected.pinned?'◆ Unpin from HUD':'◇ Pin to HUD'}</button>
      ${(state.singleRcAvailable||state.dogmeatProdAvailable) && selected.id==='0006DFAF' &&
          nativeTracking.reason==='misc-objective'
        ? '<button class="ghost-button" data-action="rc-single-track" '+
          ((state.singleRcUsed||state.singleRcPending||!state.snapshot.ready)?
            'disabled ':'')+
          'title="Verified only for vanilla DogmeatQuest objective 50 / instance 1; one QA attempt per game session">'+
          (selected.vanillaTracked?'Stop tracking in Pip-Boy':'Track in Pip-Boy')+
          '</button>'
        : state.qaIntegratedAvailable
        ? '<button class="ghost-button" data-action="pipboy-track" '+(
            state.qaIntegratedConsent && state.qaIntegratedRemaining>0 &&
            !state.qaIntegratedPending && state.snapshot.ready &&
            nativeTracking.allowed
              ? '' : 'disabled ')+
          'title="'+(nativeTracking.reason==='misc-objective'
            ? 'Misc objectives use their owning quest tracking bit; unverified generic native writer remains disabled'
            : 'QA: native Pip-Boy tracking via F4SE')+'">'+
          (selected.vanillaTracked?'Stop tracking in Pip-Boy':'Track in Pip-Boy')+'</button>'
        : state.candidateAvailable
        ? state.candidateStatus==='ready' && questWideCandidateEligible
          ? '<button class="ghost-button" data-action="questwide-candidate-track" '+
            (state.candidatePending?'disabled ':'')+
            'title="Explicit opt-in OG163 candidate: native quest-wide tracking with fresh instance and Pip-Boy checks">'+
            (selected.vanillaTracked?'Stop tracking owning quest':'Track owning quest in Pip-Boy')+'</button>'
          : '<span class="muted">Quest-wide tracking '+html(state.candidateStatus)+
            '; native eligibility '+html(selected.trackingReadiness)+
            '. No write without consistent evidence.</span>'
        : state.repeatableStatus==='ready' && repeatableEligible
        ? '<button class="ghost-button" data-action="repeatable-misc-track" '+
          (state.repeatablePending?'disabled ':'')+
          'title="Experimental: engine-managed quest-wide tracking, verified before next action">'+
          (selected.vanillaTracked?'Stop tracking owning quest':'Track owning quest in Pip-Boy')+'</button>'
        : state.repeatableStatus!=='off'
        ? '<span class="muted">Native Misc tracking '+html(state.repeatableStatus)+
          '. No further action until fresh readback verifies this quest.</span>'
        : '<button class="ghost-button" data-action="pipboy-track" disabled title="Native quest-wide write backend is not enabled in this build">Pip-Boy tracking: disabled in this build</button>'}
      ${(state.singleRcAvailable||state.dogmeatProdAvailable) && nativeTracking.reason==='misc-objective'
        ? '<span class="muted">'+
          (selected.id==='0006DFAF'
            ? 'Verified DogmeatQuest only; one direct change per game process in this candidate. Use a backed-up save.'
            : 'Pip-Boy tracking for this Misc quest remains unsupported. No native write is available.')+
          '</span>' : ''}
      ${state.repeatableAvailable && selected.isMisc && state.repeatableStatus==='off'
        ? '<button class="ghost-button" data-action="repeatable-consent" '+
          (state.repeatableConsentPending?'disabled ':'')+
          'title="Nonshipping OG163 QA: acknowledge a backed-up disposable test save; no quest is changed until a separate Track click">'+
          'I have backed up my TEST SAVE — enable native tracking QA</button>' : ''}
      ${state.qaIntegratedAvailable ? '<button class="ghost-button" data-action="qa-integrated-consent" '+
        (state.qaIntegratedRemaining===0?'disabled ':'')+
        'title="In-game consent; use a backed-up test save">'+
        (state.qaIntegratedConsent?'Disable native tracking QA':'Enable native tracking QA (test save only)')+
        '</button>' : ''}
      ${state.qaIntegratedAvailable ? '<span class="muted">QA test: '+state.qaIntegratedRemaining+
        ' native actions remaining this game session; no INI editing required.</span>' : ''}
      ${state.qaIntegratedAvailable && nativeTracking.reason==='misc-objective'
        ? '<span class="muted">Misc entries share a quest-wide vanilla tracking bit. The Journal native Misc writer is still disabled pending instance and readback acceptance; use Pip-Boy or Pin to HUD.</span>' : ''}
      ${state.qaSingleAvailable && nativeTracking.reason==='misc-objective'
        ? '<span class="muted">Single-objective experiment: ARM, then EXECUTE once on a BACKED-UP TEST SAVE. No other QA toggle is needed.</span>'+
          '<button class="ghost-button" data-action="qa-single-arm" '+
          ((state.qaSingleUsed||state.qaSingleArmPending||state.qaSinglePending)?
            'disabled ':'')+
          'title="ONE experimental real vanilla tracking operation on a backed-up TEST SAVE">'+
          (state.qaSingleArmedId===selected.id
            ? 'ARMED: single-objective experiment'
            : 'ARM Greet the dog tracking test (backup save)')+'</button>'
        : ''}
      ${state.qaSingleAvailable && state.qaSingleArmedId===selected.id &&
          nativeTracking.reason==='misc-objective' && !state.qaSingleUsed
        ? '<button class="ghost-button" data-action="qa-single-execute" '+
          (state.qaSinglePending?'disabled ':'')+
          'title="REAL game-state change; ONE attempt per game process">'+
          (selected.vanillaTracked
            ? 'EXECUTE once: Stop tracking (TEST SAVE)'
            : 'EXECUTE once: Track (TEST SAVE)')+'</button>'
        : ''}
      ${state.qaIntegratedAvailable && nativeTracking.reason==='misc-objective'
        ? '<button class="ghost-button" data-action="qa-misc-inspect" '+(state.qaMiscPending?'disabled ':'')+
          'title="Read-only game Pip-Boy Misc objective inspection">Inspect Misc tracking (read-only)</button>' : ''}
      ${state.qaIntegratedAvailable && state.qaMiscSummary && nativeTracking.reason==='misc-objective'
        ? '<p class="detail-notice" role="status">'+html(state.qaMiscSummary)+'</p>' : ''}
      ${state.qaIntegratedAvailable && nativeTracking.reason==='misc-objective'
        ? '<button class="ghost-button" data-action="qa-misc-dispatch" '+
          ((state.qaDispatchUsed||state.qaDispatchPending)?'disabled ':'')+
          'title="One bounded read-only game code metadata snapshot; never writes tracking">Inspect vanilla dispatch (read-only, once)</button>' : ''}
      ${state.qaDispatchSummary && nativeTracking.reason==='misc-objective'
        ? '<p class="detail-notice" role="status">'+html(state.qaDispatchSummary)+'</p>' : ''}
      ${state.selectorReadOnlyAvailable
        ? '<button class="ghost-button" data-action="qa-selector-readonly" '+
          ((state.selectorReadOnlyUsed||state.selectorReadOnlyPending)?'disabled ':'')+
          'title="Standalone read-only OG163 selector code capture: no game state writes">'+
          'Inspect quest row types (READ-ONLY, once)</button>' : ''}
      ${state.selectorReadOnlySummary
        ? '<p class="detail-notice" role="status">'+html(state.selectorReadOnlySummary)+'</p>' : ''}
      ${state.qaPreviewEnabled?'<button class="ghost-button" data-action="pipboy-dryrun" '+(state.qaPreviewPending?'disabled ':'')+'title="Read-only test; never changes quest tracking">DRY RUN: check Pip-Boy tracking eligibility</button>':''}
      ${state.qaWriteEnabled&&!state.qaWriteAttempted&&oneShotScopeReady?'<button class="ghost-button" data-action="pipboy-write-arm" title="Only on your backed-up TEST SAVE">'+(state.qaWriteArmed?'ARMED: click to DISARM':'ARM ONE NATIVE WRITE (TEST SAVE)')+'</button>':''}
      ${state.qaWriteEnabled&&state.qaWriteArmed&&state.qaWriteArmedId===selected.id&&oneShotScopeReady&&!state.qaWriteAttempted?'<button class="ghost-button" data-action="pipboy-write-once" title="REAL in-engine SetActive, once per game process">EXECUTE ONE REAL TRACKING CHANGE (TEST)</button>':''}
      <button class="ghost-button" data-action="favorite">${state.favorites.has(selected.id)?'★ Favorited':'☆ Favorite'}</button>
      <button class="ghost-button" data-action="hide">${state.hidden.has(selected.id)?'Show quest':'Hide quest'}</button></div>
      <div class="quest-category-edit"><label for="questCategory">Category</label>
        <select id="questCategory" aria-label="Quest category"></select>
        <span class="muted">Only changes how this journal groups the quest.</span>
      </div>
      ${selected.historyOnly?'<p class="detail-notice">Previously encountered quest. The current Pip-Boy feed no longer provides its journal description.</p>':''}
      <div class="detail-section"><div class="section-caption">JOURNAL</div><p class="journal-text">${selected.description?html(selected.description):'<span class="muted">No journal description provided by the game.</span>'}</p></div>
      <div class="detail-section"><div class="section-caption">OBJECTIVES <span class="count">${objectives.length}</span></div>
      <div class="objective-list">${objectives.length?objectives.map(o=>`<div class="objective ${html(o.state)}"><span class="objective-glyph">${o.state==='completed'?'✓':o.state==='failed'?'×':o.state==='active'?'◇':'·'}</span><span class="objective-text">${html(o.text)}</span><span class="objective-status">${prettyState(o.state)}</span></div>`).join(''):'<p class="muted">No objective data is currently available.</p>'}</div></div>
      <div class="detail-notice" role="status" aria-live="polite">${html(state.trackingNotice||'HUD pinning is independent from Pip-Boy tracking. Tracking changes do not modify quest stages or objectives.')}</div>
    </div>`;
    const categorySelect=el('questCategory');
    for(const name of M.USER_CATEGORIES){
      const option=document.createElement('option');
      option.value=name;
      option.textContent=name==='Automatic'?'Automatic ('+selected.sourceType+')':name;
      categorySelect.appendChild(option);
    }
    categorySelect.value=selected.categoryOverride?selected.type:'Automatic';
  }
  function renderTracker(){
    if (!isTracker) return;
    const eligible = M.trackerQuests(state.snapshot.quests,state.settings);
    const quests = eligible.slice(0,state.settings.trackerMaxQuests);
    el('trackerCount').textContent=eligible.length;
    if(!quests.length){
      el('trackedQuests').innerHTML=`<div class="tracker-hint">${state.snapshot.ready?'No tracked active quests. Pin one in the journal or enable Pip-Boy sync.':'Waiting for quest data…'}</div>`;
      reportTrackerMetrics();
      return;
    }
    const rows=quests.map(q=>{
      const objectives=M.selectObjectives(q).filter(o=>state.settings.trackerShowCompleted || o.state==='active');
      const visible=objectives.length?objectives:[{text:'No active objectives',state:'inactive'}];
      return `<section class="tracked-entry">
        <div class="tracked-title">${html(q.name)}</div>
        ${visible.map(o=>`<div class="tracked-objective ${o.state==='completed'?'completed':''}"><span class="tracker-bullet">${o.state==='completed'?'✓':'◇'}</span><span>${html(o.text)}</span></div>`).join('')}
      </section>`;
    }).join('');
    const remainder=eligible.length-quests.length;
    el('trackedQuests').innerHTML=rows+(remainder?`<div class="tracker-hint">+${remainder} more pinned quest${remainder===1?'':'s'} · Change limit in Settings Hub</div>`:'');
    reportTrackerMetrics();
}
  // Polling or a quest-action update must not discard keyboard/controller
  // focus simply because the corresponding row was re-created in the DOM.
  // This restores DOM focus *inside* the existing Prisma view only.
  function focusIdentity() {
    const node=document.activeElement;
    if(!node || !node.closest || !node.closest('#questList,#statusFilters,#categories,#detailPane'))return null;
    if(node.id==='questCategory' ||
       (node.classList?.contains('k2040-select-trigger') &&
        node.previousElementSibling?.id==='questCategory'))return {id:'questCategory'};
    const parent=node.closest('#questList,#statusFilters,#categories,#detailPane');
    if(!parent)return null;
    for(const key of ['quest','status','category','action','favorites','hidden']){
      if(node.dataset && Object.prototype.hasOwnProperty.call(node.dataset,key))
        return {region:parent.id,key,value:node.dataset[key]};
    }
    return null;
  }
  function restoreFocus(identity) {
    if(!identity || isTracker)return;
    if(identity.id==='questCategory'){window.K2040JournalSelects?.focus(el('questCategory'));return;}
    const region=el(identity.region);
    if(!region)return;
    const candidate=Array.from(region.querySelectorAll('[data-'+identity.key+']'))
      .find(node=>node.dataset[identity.key]===identity.value);
    if(candidate)candidate.focus();
  }
  function render(){
    const identity=isTracker?null:focusIdentity();
    const scrolling=!isTracker?document.querySelector('.quest-list-column'):null;
    const oldScroll=scrolling?scrolling.scrollTop:0;
    renderJournal();
    renderTracker();
    if((state.qaPreviewEnabled || state.dogmeatProdAvailable ||
        state.candidateAvailable || state.selectorReadOnlyAvailable) &&
       !isTracker && !state.demo &&
       state.qaLastSelected!==state.selected){
      // Changing the selected quest must invalidate pending intent.
      state.qaPreviewPending=null;
      state.qaWriteArmed=false;
      state.qaIntegratedPending=null;
      state.qaMiscPending=null;
      state.qaMiscSummary='';
      state.qaLastSelected=state.selected;
      state.selectorReadOnlyPending=null;
      if(state.selected)
        emit(state.selectorReadOnlyAvailable?'selectedSelectorReadOnlyQuest':
             state.dogmeatProdAvailable?'selectedDogmeatQuest':
             state.candidateAvailable?'selectedQuestWideCandidate':'qaSelectedQuest',
             state.selected);
    }
    if(scrolling)scrolling.scrollTop=oldScroll;
    if(!isTracker)window.K2040JournalSelects?.sync();
    restoreFocus(identity);
  }
  function receiveQuestSnapshot(input) {
    const raw=typeof input==='string'?input:JSON.stringify(input);
    if(raw===state.lastSnapshotRaw)return;
    state.lastSnapshotRaw=raw;
    try {state.snapshot=M.normalizeSnapshot(input);} catch(err){console.error('Invalid quest snapshot',err);state.snapshot={ready:false,quests:[]};}
    if(state.trackingRequest){
      const matched=state.snapshot.quests.find(q=>q.id===state.trackingRequest.id);
      if(matched && matched.vanillaTracked===state.trackingRequest.desired){
        state.trackingNotice='Confirmed: Pip-Boy tracking updated.';
        state.trackingRequest=null;
      }
    }
    state.favorites=new Set(state.snapshot.quests.filter(q=>q.favorite).map(q=>q.id));
    state.hidden=new Set(state.snapshot.quests.filter(q=>q.hidden).map(q=>q.id));
    if (!state.selected || !state.snapshot.quests.some(q=>q.id===state.selected)) {
      state.selected=state.snapshot.quests.find(q=>q.state==='active')?.id || state.snapshot.quests[0]?.id || null;
    }
    render();
  }
  function resetQuestView() {
    state.snapshot={ready:false,quests:[]};state.lastSnapshotRaw=null;state.selected=null;state.trackingRequest=null;state.trackingNotice='';state.qaPreviewPending=null;state.qaLastSelected=null;state.qaWriteArmed=false;state.qaWriteArmedId=null;state.qaGenericMiscAvailable=false;state.qaWritePending=null;state.candidateAvailable=false;state.candidateStatus='off';state.candidatePending=null;state.repeatableStatus='off';state.repeatableAvailable=false;state.repeatableConsentPending=false;state.repeatablePending=null;state.qaIntegratedConsent=false;state.qaIntegratedPending=null;state.qaMiscPending=null;state.qaMiscSummary='';state.favorites.clear();state.hidden.clear();state.category='all';state.status='active';state.sort='pinned';state.query='';if(!isTracker){el('questSort').value='pinned';el('questSearch').value='';}render();
  }
  window.receiveQuestSnapshot=receiveQuestSnapshot;
  window.receiveQuestTrackingResult=function(raw){
    if(isTracker || !state.trackingRequest)return;
    const [id,status]=String(raw||'').split(':');
    if(id!==state.trackingRequest.id)return;
    if(status==='disabled'){
      state.trackingNotice='Pip-Boy tracking changes disabled pending crash investigation.';
      state.trackingRequest=null;
    }else if(status==='rejected'){
      state.trackingNotice='Tracking request rejected: quest no longer eligible or engine unavailable.';
      state.trackingRequest=null;
    }else if(status==='unchanged'){
      state.trackingNotice='Pip-Boy already has the requested tracking state.';
      state.trackingRequest=null;
    }else if(status==='queued'){
      state.trackingNotice='Tracking request sent; awaiting confirmation from Pip-Boy.';
    }
    render();
  };
  // Never enabled by the regular Journal; only the QA DLL grants this.
  window.enableTrackingDryRunQA=function(value){
    if(isTracker || String(value)!=='true')return;
    state.qaPreviewEnabled=true;
    state.qaLastSelected=null;
    render();
  };
  window.receiveTrackingDryRunResult=function(raw){
    if(isTracker || !state.qaPreviewEnabled || !state.qaPreviewPending)return;
    const [id,status]=String(raw||'').split(':');
    if(id!==state.qaPreviewPending.id)return;
    state.qaPreviewPending=null;
    state.trackingNotice=({
      'would-write':'DRY RUN: eligible for a change; no tracking was modified.',
      'already':'DRY RUN: already in the requested state; no changes.',
      'unverified':'DRY RUN: engine and Pip-Boy state could not be verified; no changes.',
      'rejected':'DRY RUN: eligibility denied; no changes.'
    })[status]||'DRY RUN: no tracking change performed.';
    render();
  };
  window.enableNativeTrackingWriteQA=function(value){
    // Only the separate native write QA DLL with TWO explicit INI consents
    // can enable this. The other two Journal flavors never send it.
    if(isTracker || String(value)!=='true')return;
    state.qaWriteEnabled=true;
    state.qaWriteArmed=false;
    render();
  };
  window.enableQuestWideCandidate=function(value){
    if(isTracker)return;
    const parts=String(value||'').split(':');
    state.candidateAvailable=true;
    if(parts.length===3 && parts[0]==='pending' &&
       /^[0-9A-F]{8}$/.test(parts[1]) && /^[01]$/.test(parts[2])) {
      // The same save can resume *readback only* after a normal view close.
      // No new engine write is permitted until this is independently verified.
      state.candidateStatus='pending';
      state.candidatePending={id:parts[1],desired:parts[2]==='1'};
      state.trackingNotice='Waiting for the previous quest request to settle; no new native write will be issued.';
    } else {
      state.candidateStatus=['ready','blocked'].includes(parts[0])?parts[0]:'blocked';
      state.candidatePending=null;
    }
    state.qaLastSelected=null;
    render();
  };
  window.receiveQuestWideCandidateResult=function(raw){
    if(isTracker || !state.candidateAvailable)return;
    const [id,result]=String(raw||'').split(':');
    if(!state.candidatePending || state.candidatePending.id!==id)return;
    if(result==='confirmed'){
      state.candidatePending=null;state.candidateStatus='ready';
      state.trackingNotice='Native TESQuest tracking confirmed by fresh engine and Pip-Boy readback.';
    }else if(result==='pending'){
      state.candidateStatus='pending';
      state.trackingNotice='Engine write returned; verifying the original quest before any further tracking action.';
    }else if(result==='refused'){
      // A proven prewrite refusal is not an uncertain game-state change.
      state.candidatePending=null;state.candidateStatus='ready';
      state.trackingNotice='Tracking refused before a game-state change. Other eligible quests remain available.';
    }else{
      state.candidatePending=null;state.candidateStatus='blocked';
      state.trackingNotice='Tracking outcome uncertain. Further writes blocked in this game session.';
    }
    render();
  };
  window.enableRepeatableMiscTrackingQA=function(value){
    if(isTracker)return;
    state.repeatableAvailable=true;
    state.repeatableConsentPending=false;
    state.repeatableStatus=['off','ready','pending','blocked'].includes(value)?value:'blocked';
    render();
  };
  window.receiveRepeatableTrackingResult=function(raw){
    if(isTracker)return;
    const [id,result]=String(raw||'').split(':');
    if(state.repeatablePending && state.repeatablePending.id!==id)return;
    if(result==='confirmed'){
      state.repeatableStatus='ready';state.repeatablePending=null;
      state.trackingNotice='Confirmed by fresh engine and Pip-Boy Misc readback.';
    }else if(result==='pending'){
      state.repeatableStatus='pending';
      state.trackingNotice='Native change returned; awaiting independent refreshed state.';
    }else{
      state.repeatableStatus='blocked';state.repeatablePending=null;
      state.trackingNotice='Native change refused or uncertain. No automatic retry.';
    }
    render();
  };
  window.enableGenericMiscQuestWideQA=function(value){
    if(isTracker)return;
    state.qaGenericMiscAvailable=value==='true';
    render();
  };
  window.receiveTrackingWriteTestResult=function(raw){
    if(isTracker || !state.qaWriteEnabled || !state.qaWritePending)return;
    const [id,status]=String(raw||'').split(':');
    if(id!==state.qaWritePending.id)return;
    state.qaWritePending=null;
    state.qaWriteArmed=false;
    state.trackingNotice=({
      called:'TEST WRITE RETURNED: provisional only. Check Pip-Boy, compass and a separate test save/reload.',
      already:'TEST WRITE SKIPPED: quest already in requested state.',
      unverified:'TEST WRITE REFUSED: Pip-Boy and game data did not agree.',
      denied:'TEST WRITE REFUSED by safety checks.'
    })[status]||'TEST WRITE: no confirmation available. Inspect QA log.';
    render();
  };
  // Enabled ONLY by a separately compiled integrated native QA DLL.
  window.enableIntegratedTrackingQA=function(value){
    const remaining=Number(value);
    if(isTracker || !Number.isInteger(remaining) ||
       remaining<0 || remaining>8)return;
    state.qaIntegratedAvailable=true;
    state.qaIntegratedRemaining=remaining;
    state.qaIntegratedConsent=false;
    state.qaLastSelected=null;
    render();
  };
  window.receiveIntegratedTrackingConsent=function(status){
    if(isTracker || !state.qaIntegratedAvailable)return;
    const value=String(status);
    state.qaIntegratedConsent=value==='enabled';
    state.qaIntegratedPending=null;
    state.qaLastSelected=null;
    state.trackingNotice=value==='enabled'
      ? 'QA tracking enabled for this Journal view. Use a backed-up test save; engine writes affect vanilla Pip-Boy and compass.'
      : value==='disabled'
        ? 'Native tracking QA disabled. No further tracking requests accepted.'
        : 'Native tracking QA refused: save, view or attempt budget unavailable.';
    render();
  };
  window.receiveIntegratedTrackingResult=function(raw){
    if(isTracker || !state.qaIntegratedAvailable || !state.qaIntegratedPending)return;
    const [id,status,remaining]=String(raw||'').split(':');
    if(id!==state.qaIntegratedPending.id)return;
    state.qaIntegratedPending=null;
    const remainingCount=Number(remaining);
    if(Number.isInteger(remainingCount) && remainingCount>=0 && remainingCount<=8)
      state.qaIntegratedRemaining=remainingCount;
    if(status==='confirmed'){
      state.trackingNotice='Engine and Pip-Boy agree on the updated tracking state; checking fresh Journal snapshot.';
    } else if(status==='provisional'){
      state.trackingNotice='Native call returned but tracking needs confirmation. Check vanilla Pip-Boy and compass before further actions.';
      state.trackingRequest=null;
    } else {
      state.trackingRequest=null;
      state.trackingNotice=({
        already:'Quest already has that tracking state.',
        unverified:'Native tracking refused: game and Pip-Boy state did not agree.',
        rejected:'Native tracking refused by the QA safety checks.'
      })[status]||'Tracking result unavailable; inspect the QA log before continuing.';
    }
    if(state.qaIntegratedRemaining===0 ||
       status==='provisional' || status==='unverified')
      state.qaIntegratedConsent=false;
    render();
  };
  // Strictly read-only Misc objective inspection; never changes vanilla
  // quest flags, objective visibility, stages or tracking.
  window.enableMiscSingleWriteQA=function(value){
    if(isTracker || !['ready','spent'].includes(String(value)))return;
    state.qaSingleAvailable=true;
    state.qaSingleArmedId=null;
    state.qaSingleArmPending=false;
    state.qaSingleUsed=String(value)==='spent';
    if(state.qaSingleUsed)
      state.trackingNotice='The single-objective test attempt was already used this game session. Do not retry.';
    render();
  };
  window.receiveMiscSingleArm=function(value){
    if(isTracker || !state.qaSingleAvailable)return;
    const armed=String(value)==='armed' && state.qaSingleArmPending &&
      state.selected===state.qaSingleArmPending;
    state.qaSingleArmedId=armed?state.qaSingleArmPending:null;
    state.qaSingleArmPending=false;
    state.trackingNotice=armed
      ? 'Single-objective TEST ARMED. EXECUTE will change the real Pip-Boy state once; use a backed-up save.'
      : 'Single-objective test refused or selection changed; no tracking state changed.';
    render();
  };
  window.enableMiscSingleTrackingRC=function(value){
    if(isTracker || !['ready','spent'].includes(String(value)))return;
    state.singleRcAvailable=true;
    state.singleRcUsed=String(value)==='spent';
    state.singleRcPending=null;
    if(state.singleRcUsed)
      state.trackingNotice='Single-quest tracking attempt already consumed this game session. No retry.';
    render();
  };
  window.enableDogmeatTrackingProduction=function(value){
    if(isTracker || !['ready','spent'].includes(String(value)))return;
    state.dogmeatProdAvailable=true;
    state.singleRcUsed=String(value)==='spent';
    state.singleRcPending=null;
    state.qaLastSelected=null;
    render();
  };
  window.receiveMiscSingleResult=function(raw){
    if(isTracker)return;
    if((state.singleRcAvailable||state.dogmeatProdAvailable) &&
       state.singleRcPending){
      const [id,status]=String(raw||'').split(':');
      if(id!==state.singleRcPending)return;
      state.singleRcPending=null;
      state.singleRcUsed=true;
      state.trackingNotice=status==='provisional'
        ? 'Tracking command returned. Check Pip-Boy and compass; the new state is provisional until confirmed.'
        : status==='already'
          ? 'Pip-Boy tracking is already at the requested state. No change was needed.'
          : 'Tracking refused by safety checks. No game-state change was requested.';
      state.trackingRequest=null;
      render();return;
    }
    if(!state.qaSingleAvailable || !state.qaSinglePending)return;
    const [id,status]=String(raw||'').split(':');
    if(id!==state.qaSinglePending)return;
    state.qaSinglePending=null;
    state.qaSingleArmedId=null;
    state.qaSingleUsed=true;
    state.qaIntegratedConsent=false;
    state.trackingNotice=status==='provisional'
      ? 'Vanilla tracking command returned. Check Pip-Boy, compass and save/reload; this is not yet confirmed.'
      : status==='already' ? 'Tracking was already at the requested state; no write.'
      : 'One-shot command REFUSED. No native tracking change was attempted.';
    render();
  };
  window.receiveDogmeatTrackingResult=function(raw){
    window.receiveMiscSingleResult(raw);
  };
  window.enableSelectorReadOnlyQA=function(value){
    if(isTracker || !['ready','spent'].includes(String(value)))return;
    state.selectorReadOnlyAvailable=true;
    state.selectorReadOnlyUsed=String(value)==='spent';
    state.selectorReadOnlyPending=null;
    state.qaLastSelected=null;
    render();
  };
  window.receiveSelectorReadOnlyResult=function(value){
    if(isTracker || !state.selectorReadOnlyPending)return;
    state.selectorReadOnlyPending=null;
    state.selectorReadOnlySummary=String(value)==='captured'
      ? 'Read-only row survey and selector prefixes captured in a private per-session F4SE log. No tracking changed.'
      : 'Selector code capture unavailable. No tracking changed.';
    render();
  };
  window.receiveMiscDispatchProbe=function(value){
    if(isTracker || !state.qaDispatchPending)return;
    state.qaDispatchPending=null;
    state.qaDispatchSummary=String(value)==='captured'
      ? 'Read-only vanilla handler metadata captured in the private native QA log. No game state changed.'
      : 'Read-only handler bytes unavailable; no game state changed.';
    render();
  };
  window.receiveMiscTrackingProbe=function(raw){
    if(isTracker || !state.qaIntegratedAvailable || !state.qaMiscPending)return;
    let data;
    try{data=JSON.parse(String(raw||''));}catch(_){return;}
    if(!data || data.available!==true ||
       data.id!==state.qaMiscPending || !Array.isArray(data.objectives)){
      state.qaMiscPending=null;
      state.qaMiscSummary='Read-only Misc probe unavailable for the selected quest.';
      render();return;
    }
    const n=x=>Number.isInteger(x)&&x>=0?x:0;
    const details=data.objectives.slice(0,12).map(row=>
      'objective '+(row.indexKnown?n(row.index):'index unknown')+
      ': '+(row.activeKnown?(row.active?'active flag true':'active flag false'):'active flag absent')+
      (row.instanceKnown?'; numeric row instance '+n(row.instance):'; row instance unavailable')+
      (typeof row.nativeCandidateStatus==='string'
        ? '; native correlation '+row.nativeCandidateStatus+
          ' ('+n(row.nativeCandidateCount)+' candidates; NOT verified row mapping)'
        : '')+
      (Array.isArray(row.fieldSchema)?' [Pip-Boy fields: '+
        row.fieldSchema.slice(0,24).filter(v=>typeof v==='string').join(', ')+']':'')).join('; ');
    const genericCandidates=Array.isArray(data.genericCandidateStatuses)
      ? data.genericCandidateStatuses.slice(0,12).map((value,i)=>
          'row '+(i+1)+': '+(typeof value==='string'?value:'invalid')).join(', ')
      : '';
    const candidates=Array.isArray(data.displayedObjectiveCandidates)
      ? data.displayedObjectiveCandidates.slice(0,16).filter(Number.isInteger).join(', ')
      : '';
    const nativePairs=Array.isArray(data.nativeObjectiveInstances)
      ? data.nativeObjectiveInstances.slice(0,12).map(item=>
          n(item.index)+'@instance '+n(item.instanceID)).join(', ')
      : '';
    state.qaMiscSummary='Read-only Pip-Boy inspection - native quest tracking bit: '+
      (data.nativeTracked?'true':'false')+
      '; root quest rows: '+n(data.rootRows)+' (tracked: '+n(data.rootTracked)+
      ', known active: '+n(data.rootActiveKnown)+'); Misc objective rows: '+
      n(data.miscRows)+' (active: '+n(data.miscTracked)+'). '+
      (details||'No nested Misc rows found.')+
      '; generic structural candidates (read-only, NOT write permission): '+
      (genericCandidates||'none')+
      '; displayed TESQuest objective indexes (NOT verified row mapping): '+
      (candidates||'none')+
      '; native current quest instance: '+n(data.nativeCurrentInstanceID)+
      '; locked native objective/instance candidates: '+
      (data.nativeObjectiveCollectionAvailable
        ? (nativePairs||'none')+
          (data.nativeObjectiveCollectionComplete?'':' [INCOMPLETE]')
        : 'unavailable')+
      (data.truncated?' [Partial result: row scan bounded]':'');
    state.qaMiscPending=null;
    state.trackingNotice='Read-only Misc snapshot complete. Toggle in the vanilla Pip-Boy, then inspect again to compare.';
    render();
  };
  window.resetQuestView=resetQuestView;
  // UI event callbacks come from the native Prisma view and are only registered after DOM readiness.
  if (!isTracker) {
    el('questSearch').addEventListener('input',e=>{state.query=e.target.value;render();});
    el('questSort').addEventListener('change',e=>{
      if(['pinned','name','name-desc','category'].includes(e.target.value)) {
        state.sort=e.target.value;render();
      }
    });
    el('resetFilters').addEventListener('click',()=>{state.query='';state.status='active';state.category='all';state.favoritesOnly=false;state.includeHidden=false;state.sort='pinned';el('questSearch').value='';el('questSort').value='pinned';render();});
    el('closeJournal').addEventListener('click',()=>emit('requestCloseJournal'));
    el('openSettings').addEventListener('click',()=>{
      if(state.demo){window.receiveJournalHubOpenResult('unavailable');return;}
      const status=el('settingsHubStatus');if(status)status.hidden=true;
      emit('requestOpenJournalHubSettings');
    });
    document.addEventListener('change',e=>{
      if(e.target.id==='pipboySyncToggle'){
        const enabled=Boolean(e.target.checked);
        if(state.demo) {
          state.settings.trackerSyncPipboy=enabled;
          el('pipboySyncStatus').textContent='Demo only — not saved.';
        } else {
          e.target.disabled=true;
          el('pipboySyncStatus').textContent='Saving…';
          emit('requestJournalPipboySync',enabled?'true':'false');
        }
        return;
      }
      if(e.target.id!=='questCategory' || !state.selected)return;
      const category=e.target.value;
      if(!M.USER_CATEGORIES.includes(category))return;
      if(!state.demo)emit('requestQuestCategory',state.selected+':'+category);
      else {
        const quest=state.snapshot.quests.find(q=>q.id===state.selected);
        if(quest){quest.categoryOverride=category!=='Automatic';quest.type=quest.categoryOverride?category:quest.sourceType;render();}
      }
    });
    document.addEventListener('click',e=>{
      const target=e.target.closest('button');if(!target)return;
      if(target.dataset.status){state.status=target.dataset.status;render();}
      else if(target.dataset.category){state.category=target.dataset.category;render();}
      else if(target.dataset.favorites){state.favoritesOnly=!state.favoritesOnly;render();}
      else if(target.dataset.hidden){state.includeHidden=!state.includeHidden;render();}
      else if(target.dataset.quest){state.selected=target.dataset.quest;state.qaSingleArmedId=null;render();}
      else if(target.dataset.action){const id=state.selected;if(!id)return;
        // Shipping fails closed: only integrated QA exposes this control
        // after explicit in-Journal native consent.
        if(target.dataset.action==='qa-integrated-consent'){
          if(!state.qaIntegratedAvailable || state.qaIntegratedPending ||
             state.qaIntegratedRemaining===0 || state.demo)return;
          emit('setIntegratedTrackingConsent',
               state.qaIntegratedConsent?'DISABLE':'ENABLE');
          return;
        }
        if(target.dataset.action==='qa-selector-readonly'){
          if(!state.selectorReadOnlyAvailable || state.selectorReadOnlyUsed ||
             state.selectorReadOnlyPending || !state.snapshot.ready ||
             state.demo)return;
          const quest=state.snapshot.quests.find(q=>q.id===id);
          if(!quest)return;
          state.selectorReadOnlyUsed=true; // fail closed, no auto retries
          state.selectorReadOnlyPending=id;
          state.selectorReadOnlySummary='Inspecting quest row types and verified OG163 command slots (read-only).';
          emit('requestSelectorReadOnlyCapture',id);
          const ticket=id;
          window.setTimeout(()=>{
            if(state.selectorReadOnlyPending===ticket){
              state.selectorReadOnlyPending=null;
              state.selectorReadOnlySummary='No reply to bounded selector capture; no game state changed.';
              render();
            }
          },10000);
          render();
          return;
        }
        if(target.dataset.action==='qa-misc-dispatch'){
          if(!state.qaIntegratedAvailable || !state.snapshot.ready ||
             state.qaDispatchUsed || state.qaDispatchPending || state.demo)return;
          const quest=state.snapshot.quests.find(q=>q.id===id);
          if(M.nativeTrackEligibility(quest).reason!=='misc-objective')return;
          state.qaDispatchUsed=true;
          state.qaDispatchPending=id;
          state.qaDispatchSummary='Inspecting fixed native handler headers (read-only).';
          emit('requestMiscDispatchProbe',id);
          const ticket=id;
          window.setTimeout(()=>{
            if(state.qaDispatchPending===ticket){
              state.qaDispatchPending=null;
              state.qaDispatchSummary='No reply to bounded runtime inspection; no game state changed.';
              render();
            }
          },10000);
          render();return;
        }
        if(target.dataset.action==='rc-single-track'){
          if(!(state.singleRcAvailable||state.dogmeatProdAvailable) ||
             state.singleRcUsed ||
             state.singleRcPending || !state.snapshot.ready || state.demo ||
             id!=='0006DFAF')return;
          const quest=state.snapshot.quests.find(q=>q.id===id);
          if(M.nativeTrackEligibility(quest).reason!=='misc-objective')return;
          state.singleRcUsed=true; // fail closed immediately, even on timeout
          state.singleRcPending=id;
          state.trackingNotice='Checking the verified single-objective quest. One native attempt max; do not retry.';
          emit(state.dogmeatProdAvailable?'requestDogmeatTracking':
               'requestMiscSingleTrackingRC',
               id+':'+(quest.vanillaTracked?'0':'1'));
          const ticket=id;
          window.setTimeout(()=>{
            if(state.singleRcPending===ticket){
              state.singleRcPending=null;
              state.trackingNotice='No engine reply. Do not retry; inspect the game and native log.';
              render();
            }
          },10000);
          render();return;
        }
        if(target.dataset.action==='qa-single-arm'){
          if(!state.qaSingleAvailable || state.qaSingleUsed ||
             state.qaSingleArmPending || state.qaSinglePending ||
             state.demo || !state.snapshot.ready)return;
          const q=state.snapshot.quests.find(q=>q.id===id);
          if(M.nativeTrackEligibility(q).reason!=='misc-objective')return;
          state.qaSingleArmPending=id;
          state.trackingNotice='Checking ARM request. No tracking change will occur until EXECUTE.';
          emit('armMiscSingleWrite',id);
          const ticket=id;
          window.setTimeout(()=>{
            if(state.qaSingleArmPending===ticket){
              state.qaSingleArmPending=false;
              state.qaSingleArmedId=null;
              state.trackingNotice='ARM did not receive a response. No tracking write was attempted.';
              render();
            }
          },10000);
          render();return;
        }
        if(target.dataset.action==='qa-single-execute'){
          if(!state.qaSingleAvailable || state.qaSingleUsed ||
             state.qaSingleArmedId!==id || state.qaSingleArmPending ||
             state.qaSinglePending || state.demo || !state.snapshot.ready)return;
          const q=state.snapshot.quests.find(q=>q.id===id);
          if(M.nativeTrackEligibility(q).reason!=='misc-objective')return;
          state.qaSinglePending=id;
          state.qaSingleArmedId=null;
          state.qaSingleUsed=true; // never auto retry, including on timeout
          state.trackingNotice='One-shot experiment submitted. Do NOT retry; check the game after the response.';
          emit('requestMiscSingleExecute',id+':'+(q.vanillaTracked?'0':'1'));
          render();return;
        }
        if(target.dataset.action==='qa-misc-inspect'){
          if(!state.qaIntegratedAvailable || !state.snapshot.ready ||
             state.qaMiscPending || state.demo)return;
          const quest=state.snapshot.quests.find(q=>q.id===id);
          if(M.nativeTrackEligibility(quest).reason!=='misc-objective')return;
          state.qaMiscPending=id;
          state.qaMiscSummary='';
          state.trackingNotice='Reading current Misc objective entries (no tracking change).';
          emit('requestMiscTrackingProbe',id);
          const ticket=id;
          window.setTimeout(()=>{
            if(state.qaMiscPending===ticket){
              state.qaMiscPending=null;
              state.trackingNotice='Misc read-only probe timed out. No tracking write was attempted.';
              render();
            }
          },10000);
          render();return;
        }
        if(target.dataset.action==='pipboy-track'){
          if(!state.qaIntegratedAvailable || !state.qaIntegratedConsent ||
             state.qaIntegratedPending || state.qaIntegratedRemaining===0 ||
             !state.snapshot.ready || state.demo)return;
          const quest=state.snapshot.quests.find(q=>q.id===id);
          const eligibility=M.nativeTrackEligibility(quest);
          if(!eligibility.allowed){
            state.trackingNotice=eligibility.reason==='misc-objective'
              ? 'Misc objectives cannot safely use the native quest-level tracking writer. Use vanilla Pip-Boy; Pin to HUD works separately.'
              : 'This quest is not eligible for native tracking.';
            render();return;
          }
          const desired=!quest.vanillaTracked;
          const pending={id,desired};
          state.qaIntegratedPending=pending;
          state.trackingRequest={id,desired};
          state.trackingNotice='Native tracking request queued; awaiting engine and Pip-Boy confirmation.';
          emit('requestPipboyTrackingIntegrated',id+':'+(desired?'1':'0'));
          window.setTimeout(()=>{
            if(state.qaIntegratedPending===pending){
              state.qaIntegratedPending=null;
              state.trackingRequest=null;
              state.trackingNotice='No QA task response. Check the vanilla Pip-Boy and logs before another attempt.';
              render();
            }
          },10000);
          render();return;
        }
        if(target.dataset.action==='questwide-candidate-track'){
          if(!state.candidateAvailable || state.candidateStatus!=='ready' ||
             state.candidatePending || state.demo || !state.snapshot.ready)return;
          const q=state.snapshot.quests.find(q=>q.id===id);
          if(!q || q.historyOnly || q.state!=='active' ||
             q.trackingScope!=='quest-wide' || q.nativeInstanceStatus!=='consistent' ||
             q.nativeInstanceID===null || q.nativeDisplayedCount<=0 ||
             q.pipboyFeedAgreement!=='match' ||
             (q.isMisc && q.miscRowCount<=0) ||
             q.trackingReadiness!=='candidate-read-only')return;
          const desired=!q.vanillaTracked;
          state.candidatePending={id,desired};state.candidateStatus='pending';
          state.trackingNotice='Quest-wide tracking queued. Awaiting authoritative readback.';
          emit('requestQuestWideCandidateTracking',id+':'+(desired?'1':'0')+
            ':'+q.nativeInstanceID.toString(16).toUpperCase().padStart(8,'0'));
          render();return;
        }
        if(target.dataset.action==='repeatable-consent'){
          // Separate from the tracking action: no game state changed here.
          if(!state.repeatableAvailable || state.repeatableStatus!=='off' ||
             state.repeatableConsentPending || state.demo)return;
          state.repeatableConsentPending=true;
          state.trackingNotice='Requesting QA consent for this backed-up test session. No quest has been changed.';
          emit('grantRepeatableMiscTrackingQA');
          render();return;
        }
        if(target.dataset.action==='repeatable-misc-track'){
          if(state.repeatableStatus!=='ready' || state.repeatablePending || state.demo ||
             !state.snapshot.ready)return;
          const q=state.snapshot.quests.find(q=>q.id===id);
          if(!q || !q.isMisc || q.historyOnly || q.state!=='active' ||
             q.trackingScope!=='quest-wide' || q.nativeInstanceStatus!=='consistent' ||
             q.nativeInstanceID===null || q.pipboyFeedAgreement!=='match' ||
             q.miscRowCount<=0)return;
          const desired=!q.vanillaTracked;
          state.repeatablePending={id,desired};state.repeatableStatus='pending';
          state.trackingNotice='Quest-wide native tracking queued. Awaiting fresh engine readback.';
          emit('requestRepeatableMiscQuestTracking',id+':'+(desired?'1':'0')+
            ':'+q.nativeInstanceID.toString(16).toUpperCase().padStart(8,'0'));
          render();return;
        }
        if(target.dataset.action==='pipboy-write-arm'){
          if(!state.qaWriteEnabled || state.qaWriteAttempted || state.demo)return;
          const q=state.snapshot.quests.find(q=>q.id===id);
          if(!q || q.historyOnly || q.state!=='active')return;
          if(state.qaGenericMiscAvailable && !q.isMisc)return;
          if(q.isMisc && !(state.qaGenericMiscAvailable && q.trackingScope==='quest-wide' &&
              q.nativeInstanceStatus==='consistent' && q.nativeInstanceID!==null &&
              q.pipboyFeedAgreement==='match' && q.miscRowCount>0))return;
          state.qaWriteArmed=state.qaWriteArmedId===id?!state.qaWriteArmed:true;
          state.qaWriteArmedId=state.qaWriteArmed?id:null;
          state.trackingNotice=state.qaWriteArmed?
            'ARMED FOR ONE REAL TEST WRITE. Use only a backed-up TEST SAVE.':
            'Native test write disarmed; no state changed.';
          render();return;
        }
        if(target.dataset.action==='pipboy-write-once'){
          if(!state.qaWriteEnabled || !state.qaWriteArmed ||
             state.qaWriteArmedId!==id || state.qaWriteAttempted ||
             !state.snapshot.ready || state.demo)return;
          const chosen=state.snapshot.quests.find(q=>q.id===id);
          if(!chosen || chosen.historyOnly || chosen.state!=='active')return;
          if(state.qaGenericMiscAvailable && !chosen.isMisc)return;
          if(chosen.isMisc && !(state.qaGenericMiscAvailable &&
             chosen.trackingScope==='quest-wide' && chosen.nativeInstanceStatus==='consistent' &&
             chosen.nativeInstanceID!==null && chosen.pipboyFeedAgreement==='match' &&
             chosen.miscRowCount>0))return;
          state.qaWriteArmed=false;
          state.qaWriteArmedId=null;
          state.qaWriteAttempted=true; // UI also refuses further clicks
          state.qaWritePending={id};
          state.trackingNotice='ONE REAL WRITE requested. Do not assume success; check the game.';
          const payload=id+':'+(!chosen.vanillaTracked?'1':'0')+
            (chosen.isMisc?':'+chosen.nativeInstanceID.toString(16).toUpperCase().padStart(8,'0'):'');
          emit('requestPipboyTrackingWriteOnce',payload);
          render();return;
        }
        if(target.dataset.action==='pipboy-dryrun'){
          if(!state.qaPreviewEnabled || state.qaPreviewPending ||
             !state.snapshot.ready || state.demo)return;
          const chosen=state.snapshot.quests.find(q=>q.id===id);
          if(!chosen || chosen.isMisc || chosen.historyOnly ||
             chosen.state!=='active')return;
          const desired=!chosen.vanillaTracked;
          state.qaPreviewPending={id,desired};
          state.trackingNotice='DRY RUN: checking game state; no quest writes.';
          emit('requestPipboyTrackingPreview',id+':'+(desired?'1':'0'));
          // A missing F4SE task interface can fail before native scheduling;
          // never leave the UI permanently disabled or claim a write succeeded.
          const ticket=state.qaPreviewPending;
          window.setTimeout(()=>{
            if(state.qaPreviewPending===ticket){
              state.qaPreviewPending=null;
              state.trackingNotice='DRY RUN: no response from the game task; no tracking change was requested.';
              render();
            }
          },10000);
          render();
          return;
        }
        if(target.dataset.action==='favorite'){
          if(!state.demo)emit('requestFavoriteQuest',id);
          else {const q=state.snapshot.quests.find(q=>q.id===id);if(q){q.favorite=!q.favorite;state.favorites[q.favorite?'add':'delete'](id);}render();}
        }
        if(target.dataset.action==='hide'){
          if(!state.demo)emit('requestHideQuest',id);
          else {const q=state.snapshot.quests.find(q=>q.id===id);if(q){q.hidden=!q.hidden;state.hidden[q.hidden?'add':'delete'](id);}render();}
        }
        if(target.dataset.action==='pin'){
          // Native backend is authoritative. In preview mode, update local state only.
          if(!state.demo&&typeof window.requestPinQuest==='function')emit('requestPinQuest',id);
          else {const quest=state.snapshot.quests.find(q=>q.id===id);if(quest)quest.pinned=!quest.pinned;render();}
        }
      }
    });
    function moveSelection(direction){
      const quests=filtered();if(!quests.length)return;
      const index=quests.findIndex(q=>q.id===state.selected);
      const next=Math.max(0,Math.min(quests.length-1,index+direction));
      state.selected=quests[index<0?0:next].id;render();
    }
    window.addEventListener('prisma-controller-action',({detail})=>{
      if(!detail || detail.state!=='pressed')return;
      const statuses=['active','completed','failed','all'];
      if(detail.action==='journal.pin' && state.selected)emit('requestPinQuest',state.selected);
      else if(detail.action==='journal.favorite' && state.selected)emit('requestFavoriteQuest',state.selected);
      else if(detail.action==='journal.status-prev'){state.status=statuses[(statuses.indexOf(state.status)+3)%4];render();}
      else if(detail.action==='journal.status-next'){state.status=statuses[(statuses.indexOf(state.status)+1)%4];render();}
    });
    document.addEventListener('keydown',e=>{
      if(e.key==='Escape'){
        e.preventDefault();
        if(state.demo)window.handleJournalEscape();
        else emit('requestJournalEscape');
      }
      else if(!e.repeat &&
        (!e.ctrlKey && !e.altKey &&
         (e.key.toUpperCase()==='G' &&
          (state.settings.journalHotkey==='Shift+G' ? e.shiftKey :
           state.settings.journalHotkey==='G' && !e.shiftKey) ||
          e.key===state.settings.journalHotkey && !e.shiftKey)) &&
        !e.target.closest('input,textarea,select,button,[contenteditable="true"]')) {
        e.preventDefault();emit('requestCloseJournal');
      }
      // Preserve native arrow navigation in sorting and text/edit controls.
      else if(e.key==='ArrowDown' &&
              !e.target.closest('input,textarea,select,[contenteditable="true"]')){moveSelection(1);e.preventDefault();}
      else if(e.key==='ArrowUp' &&
              !e.target.closest('input,textarea,select,[contenteditable="true"]')){moveSelection(-1);e.preventDefault();}
    });
  }
  if(state.demo){
    try {receiveJournalSettings(localStorage.getItem('K2040QuestJournalPreviewSettings')||{});}catch{receiveJournalSettings({});}
    receiveQuestSnapshot(window.K2040DemoData);
  } else {receiveJournalSettings({});render();}

  // If the page loads with the game plugin, request an initial snapshot after callbacks are bound.
  if(!state.demo){
    setTimeout(()=>{emit('requestJournalSettings');emit('requestQuestSnapshot');},0);
    // Only while a real candidate action is pending, independently poll
    // for native readback; the normal Journal refresh remains inexpensive.
    if(!isTracker)setInterval(()=>{
      if(state.candidateAvailable && state.candidateStatus==='pending' &&
         state.candidatePending)emit('requestQuestSnapshot');
    },1000);
    // Bounded passive polling for ordinary quest/view updates.
    setInterval(()=>{
      if(isTracker && !state.settings.trackerEnabled)return;
      emit('requestQuestSnapshot');
      // HUD tint can be changed by the player during gameplay. Re-read only
      // while the Pip-Boy theme needs engine colors; both views remain passive.
      if(state.settings.theme==='pipboy' || (!isTracker && state.settings.journalTheme==='pipboy'))emit('requestJournalSettings');
    },isTracker?8000:12000);
  }
})();
