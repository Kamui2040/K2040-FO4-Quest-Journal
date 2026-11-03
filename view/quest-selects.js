/* Original, dependency-free dark option popup for Ultralight/PrismaUI_F4.
 * Native <select> popups ignore CSS option colors in the embedded browser.
 * Preserve the actual select as the single value/change-event source so all
 * existing MCM, Journal and controller callbacks keep their contracts.
 */
(function (root) {
  'use strict';
  if (!root || !root.document) return;
  const document=root.document;
  let active=null;
  const entries=new WeakMap();
  function close(restore=false) {
    if(!active)return;
    const previous=active;active=null;
    previous.trigger.setAttribute('aria-expanded','false');
    if(previous.menu.parentNode)previous.menu.parentNode.removeChild(previous.menu);
    if(restore && document.contains(previous.trigger))previous.trigger.focus();
  }
  function selectedLabel(select) {
    const option=select.options[select.selectedIndex];
    return option ? option.textContent.trim() : 'Choose…';
  }
  function update(select) {
    const trigger=entries.get(select);
    if(!trigger)return;
    trigger.textContent=selectedLabel(select);
    const label=select.id ? document.querySelector('label[for="'+select.id+'"]') : null;
    const name=select.getAttribute('aria-label') ||
      label?.querySelector('span')?.textContent.trim() ||
      label?.childNodes[0]?.textContent.trim() || 'Choose option';
    trigger.setAttribute('aria-label',name+': '+trigger.textContent);
    trigger.disabled=select.disabled;
  }
  function open(select,trigger) {
    if(active && active.select===select){close();return;}
    close();
    const options=Array.from(select.options).filter(option=>!option.hidden);
    if(!options.length)return;
    const menu=document.createElement('div');
    menu.className='k2040-select-menu';
    menu.setAttribute('role','listbox');
    menu.setAttribute('aria-label',select.getAttribute('aria-label')||'Choose option');
    const id=select.id||'journal-select';
    menu.id=id+'-options';
    trigger.setAttribute('aria-controls',menu.id);
    const buttons=[];
    options.forEach(option=>{
      const button=document.createElement('button');
      button.type='button';
      button.className='k2040-select-option';
      button.textContent=option.textContent;
      button.setAttribute('role','option');
      const isSelected=option.selected;
      button.setAttribute('aria-selected',String(isSelected));
      if(isSelected)button.classList.add('selected');
      button.disabled=option.disabled;
      button.addEventListener('click',()=>{
        if(button.disabled)return;
        const changed=select.value!==option.value;
        select.value=option.value;
        update(select);
        close(true);
        if(changed)select.dispatchEvent(new Event('change',{bubbles:true}));
      });
      menu.appendChild(button);
      buttons.push(button);
    });
    document.body.appendChild(menu);
    const rect=trigger.getBoundingClientRect();
    const vh=root.innerHeight || document.documentElement.clientHeight;
    const vw=root.innerWidth || document.documentElement.clientWidth;
    const rowHeight=34;
    const height=Math.min(options.length*rowHeight+8,Math.max(110,vh-24));
    const left=Math.max(8,Math.min(rect.left,Math.max(8,vw-Math.max(rect.width,150)-8)));
    let top=rect.bottom+3;
    if(top+height>vh-8 && rect.top-height-3>=8)top=rect.top-height-3;
    else top=Math.max(8,Math.min(top,vh-height-8));
    menu.style.left=left+'px';
    menu.style.top=top+'px';
    menu.style.width=Math.max(rect.width,150)+'px';
    menu.style.maxHeight=height+'px';
    active={select,trigger,menu,buttons};
    trigger.setAttribute('aria-expanded','true');
    const selected=buttons.find(b=>b.getAttribute('aria-selected')==='true'&&!b.disabled);
    (selected||buttons.find(b=>!b.disabled))?.focus();
  }
  function upgrade(select) {
    if(entries.has(select))return;
    const trigger=document.createElement('button');
    trigger.type='button';
    trigger.className='k2040-select-trigger';
    trigger.setAttribute('aria-haspopup','listbox');
    trigger.setAttribute('aria-expanded','false');
    select.classList.add('k2040-select-source');
    select.setAttribute('aria-hidden','true');
    select.tabIndex=-1;
    select.parentNode.insertBefore(trigger,select.nextSibling);
    entries.set(select,trigger);
    select.addEventListener('change',()=>update(select));
    trigger.addEventListener('click',()=>open(select,trigger));
    trigger.addEventListener('keydown',e=>{
      if(e.key==='ArrowDown'||e.key==='ArrowUp'||e.key===' '){
        e.preventDefault();open(select,trigger);
      }
    });
    update(select);
  }
  function sync() {
    // Category select is replaced by each Journal details render. Remove
    // stale overlays before accidentally retaining detached controls.
    if(active && !document.contains(active.select))close();
    document.querySelectorAll('.journal-page select:not(.k2040-select-source)').forEach(upgrade);
    document.querySelectorAll('.journal-page select.k2040-select-source').forEach(update);
  }
  function focus(select){(entries.get(select)||select)?.focus();}
  document.addEventListener('keydown',event=>{
    if(!active)return;
    if(event.key==='Escape'){
      event.preventDefault();event.stopImmediatePropagation();close(true);return;
    }
    if(event.key==='Tab'){close();return;}
    if(event.key==='ArrowDown'||event.key==='ArrowUp'||event.key==='Home'||event.key==='End'){
      event.preventDefault();event.stopImmediatePropagation();
      const enabled=active.buttons.filter(button=>!button.disabled);
      if(!enabled.length)return;
      const current=enabled.indexOf(document.activeElement);
      const next=event.key==='Home'?0:event.key==='End'?enabled.length-1:
        Math.min(enabled.length-1,Math.max(0,(current<0?0:current)+(event.key==='ArrowDown'?1:-1)));
      enabled[next].focus();return;
    }
  },true);
  document.addEventListener('pointerdown',event=>{
    if(active && !active.menu.contains(event.target) && event.target!==active.trigger)close();
  },true);
  root.addEventListener('resize',()=>close());
  root.addEventListener('scroll',()=>close(),true);
  root.K2040JournalSelects=Object.freeze({sync,close,focus});
  sync();
})(typeof window!=='undefined'?window:null);
