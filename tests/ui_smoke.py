"""Optional browser-only smoke. Requires Playwright and an installed Chromium.

This intentionally uses set_content and local source embedding: no network and no
real Fallout game state. It must not be counted as native/game runtime QA.
"""
from pathlib import Path
import os
from playwright.sync_api import sync_playwright

view = Path(__file__).resolve().parent.parent / "view"

def html_document(name):
    doc = (view / f"{name}.html").read_text(encoding="utf-8")
    doc = doc.replace('<link rel="stylesheet" href="styles.css">',
                      '<style>' + (view / "styles.css").read_text(encoding="utf-8") + '</style>')
    for filename in ("quest-model.js", "journal-settings.js", "demo-data.js", "app.js"):
        doc = doc.replace(f'<script src="{filename}"></script>',
                          '<script>' + (view / filename).read_text(encoding="utf-8") + '</script>')
    return doc

with sync_playwright() as p:
    binary = os.environ.get("CHROMIUM_PATH", "/usr/bin/chromium")
    browser = p.chromium.launch(executable_path=binary, headless=True, args=["--no-sandbox"])
    for name in ("journal", "tracker"):
        page = browser.new_page(viewport={"width": 1440, "height": 900})
        errors = []
        page.on("pageerror", lambda error: errors.append(str(error)))
        page.set_content(html_document(name), wait_until="load")
        if name == "journal":
            # Mock game-thread Prisma callbacks: browser check only.
            page.evaluate("""() => {
              // In-game this callback is bound by the native Prisma bridge.
              window.resetJournalAppearance = () => {
                window.receiveJournalSettings({});
                window.receiveJournalSettingsStatus('saved');
              };
              for (const [fn, flag] of [['requestPinQuest','pinned'],
                ['requestFavoriteQuest','favorite'],['requestHideQuest','hidden']]) {
                window[fn] = id => {
                  const q=window.K2040DemoData.quests.find(q=>q.id.toUpperCase()===id);
                  if(q)q[flag]=!q[flag];
                  window.receiveQuestSnapshot(window.K2040DemoData);
                };
              }
            }""")
        page.evaluate("window.receiveQuestSnapshot(window.K2040DemoData)")
        if name == "journal":
            assert page.locator("[data-quest]").count() == 5
            assert page.locator("#detailPane").get_by_text("The Road to Sanctuary").count() == 1
            first_quest=page.locator('[data-quest]').first
            first_quest.focus()
            quest_id=first_quest.get_attribute('data-quest')
            page.evaluate("window.receiveQuestSnapshot({...window.K2040DemoData,quests:window.K2040DemoData.quests.map(q=>({...q,description:(q.description||'')+' updated'}))})")
            assert page.evaluate("document.activeElement?.dataset.quest") == quest_id
            page.locator('#questCategory').focus()
            page.evaluate("window.receiveQuestSnapshot({...window.K2040DemoData,quests:window.K2040DemoData.quests.map(q=>({...q,description:(q.description||'')+' changed'}))})")
            assert page.evaluate("document.activeElement?.id") == 'questCategory'
            # Arrow keys inside the sort selector must not be hijacked by quest navigation.
            consumed = page.evaluate("""() => {
              const sort = document.getElementById('questSort');
              sort.focus();
              const event = new KeyboardEvent('keydown', {key:'ArrowDown',bubbles:true,cancelable:true});
              sort.dispatchEvent(event);
              return event.defaultPrevented;
            }""")
            assert not consumed
            page.locator("#questSearch").fill("Signals")
            assert page.locator("[data-quest]").count() == 1
            page.locator("#resetFilters").click()
            assert page.locator("[data-quest]").count() == 5
            page.locator('[data-action="pin"]').click()
            page.locator('#questCategory').select_option('Modded')
            assert page.locator('#questCategory').input_value() == 'Modded'
            page.locator('#questCategory').select_option('Automatic')
            assert page.locator('#questCategory').input_value() == 'Automatic'
            page.locator('[data-action="favorite"]').click()
            assert 'Favorited' in page.locator('[data-action="favorite"]').inner_text()
            page.locator('[data-action="hide"]').click()
            assert page.locator('[data-quest]').count() == 4
            assert page.locator('#detailPane').get_by_text('The Road to Sanctuary').count() == 0
            page.locator('[data-hidden="1"]').click()
            assert page.locator('[data-quest]').count() == 5
            page.evaluate("window.dispatchEvent(new CustomEvent('prisma-controller-action', {detail:{button:'RB',action:'journal.status-next',state:'pressed'}}))")
            assert 'Completed quests' in page.locator('#listHeading').inner_text()
            page.evaluate("window.dispatchEvent(new CustomEvent('prisma-controller-action', {detail:{button:'LB',action:'journal.status-prev',state:'pressed'}}))")
            page.locator('#openSettings').click()
            assert page.locator('#settingsWorkspace').is_visible()
            assert not page.locator('#journalWorkspace').is_visible()
            page.locator('[data-theme="contrast"]').click()
            assert page.locator('.tracker-sample').evaluate(
                "node => node.style.getPropertyValue('--sample-accent')") == '#00e5ff'
            assert page.evaluate("document.documentElement.style.getPropertyValue('--ui-accent')") == '#66c4ff'
            page.locator('[data-theme="fallout76"]').click()
            assert page.locator('.tracker-sample').evaluate(
                "node => node.style.getPropertyValue('--sample-accent')") == '#f5cb5b'
            assert page.locator('.tracker-sample').evaluate(
                "node => node.classList.contains('theme-fallout76')")
            page.locator('[data-theme="pipboy"]').click()
            assert page.locator('[data-theme="pipboy"]').get_attribute('class') == 'selected'
            assert page.locator('#settingAccentHex').is_disabled()
            page.locator('[data-theme="custom"]').click()
            assert page.locator('#settingAccentHex').is_enabled()
            page.locator('#settingAccentHex').fill('#112233')
            page.locator('#settingAccentHex').press('Tab')
            assert page.locator('#settingAccentHex').input_value() == '#112233'.upper()
            assert page.locator('#settingAccent').input_value() == '#112233'
            assert page.locator('[data-palette="accent"] [data-rgb-value="0"]').inner_text() == '17'
            assert page.locator('[data-palette="accent"] [data-rgb-value="1"]').inner_text() == '34'
            assert page.locator('[data-palette="accent"] [data-rgb-value="2"]').inner_text() == '51'
            page.locator('[data-palette="accent"] [data-rgb-channel="1"]').evaluate("""node => {
              node.value = '68';
              node.dispatchEvent(new Event('input',{bubbles:true}));
              node.dispatchEvent(new Event('change',{bubbles:true}));
            }""")
            assert page.locator('#settingAccentHex').input_value() == '#114433'.upper()
            # Changing the tracker palette must not recolour the journal UI.
            assert page.evaluate("document.documentElement.style.getPropertyValue('--ui-accent')") == '#66c4ff'
            assert page.locator('.tracker-sample').evaluate(
                "node => node.style.getPropertyValue('--sample-accent')") == '#114433'
            page.locator('#settingCompletedHex').fill('#88ffaa')
            page.locator('#settingCompletedHex').press('Tab')
            assert page.evaluate("document.documentElement.style.getPropertyValue('--ui-completed')") == '#74e398'
            assert page.locator('.tracker-sample').evaluate(
                "node => node.style.getPropertyValue('--sample-completed')") == '#88ffaa'
            assert page.locator('.appearance-sample').count() == 1
            assert page.locator('.appearance-sample .sample-done').count() == 1
            assert page.locator('.appearance-sample .sample-completed-symbol').evaluate(
                "node => getComputedStyle(node).color") == 'rgb(136, 255, 170)'
            page.locator('#settingAccentHex').fill('#invalid')
            page.locator('#settingAccentHex').press('Tab')
            assert page.locator('#settingAccentHex').input_value() == '#114433'.upper()
            page.locator('[data-settings-page="journal"]').click()
            assert page.locator('#settingsJournal').is_visible()
            page.locator('#settingSidebarWidth').evaluate("""node => {
              node.value='320';
              node.dispatchEvent(new Event('input',{bubbles:true}));
              node.dispatchEvent(new Event('change',{bubbles:true}));
            }""")
            assert page.evaluate("document.documentElement.style.getPropertyValue('--journal-sidebar-width')") == '320px'
            page.locator('#settingQuestListWidth').evaluate("""node => {
              node.value='450';
              node.dispatchEvent(new Event('input',{bubbles:true}));
              node.dispatchEvent(new Event('change',{bubbles:true}));
            }""")
            assert page.evaluate("document.documentElement.style.getPropertyValue('--journal-list-width')") == '450px'
            page.locator('[data-settings-page="appearance"]').click()
            page.locator('#resetAppearance').click()
            assert page.locator('#settingSidebarWidth').input_value() == '240'
            assert page.locator('#settingQuestListWidth').input_value() == '360'
            assert page.locator('#settingAccentHex').is_disabled()
            assert page.evaluate("document.documentElement.style.getPropertyValue('--ui-completed')") == '#74e398'
            page.locator('[data-settings-page="journalAppearance"]').click()
            assert page.locator('#settingsJournalAppearance').is_visible()
            page.locator('[data-journal-theme="contrast"]').click()
            assert page.evaluate("document.documentElement.style.getPropertyValue('--ui-accent')") == '#00e5ff'
            page.locator('[data-journal-theme="fallout76"]').click()
            assert page.evaluate("document.documentElement.style.getPropertyValue('--ui-accent')") == '#f5cb5b'
            assert page.locator('body').evaluate(
                "node => node.classList.contains('journal-theme-fallout76')")
            assert page.locator('#settingJournalAccentHex').is_disabled()
            page.locator('[data-journal-theme="custom"]').click()
            assert page.locator('#settingJournalAccentHex').is_enabled()
            page.locator('#settingJournalAccentHex').fill('#ba5511')
            page.locator('#settingJournalAccentHex').press('Tab')
            assert page.evaluate("document.documentElement.style.getPropertyValue('--ui-accent')") == '#ba5511'
            assert page.locator('.tracker-sample').evaluate(
                "node => node.style.getPropertyValue('--sample-accent')") == '#66c4ff'
            page.locator('#settingJournalOpacity').evaluate("""node => {
              node.value='63';
              node.dispatchEvent(new Event('input',{bubbles:true}));
              node.dispatchEvent(new Event('change',{bubbles:true}));
            }""")
            assert page.evaluate("document.documentElement.style.getPropertyValue('--ui-panel-background')") == 'rgba(11,18,24,0.63)'
            page.locator('[data-settings-page="tracker"]').click()
            assert page.locator('#settingTrackerAnchor').evaluate(
                'node => getComputedStyle(node).appearance') == 'none'
            assert page.locator('#settingTrackerWidth').evaluate(
                'node => getComputedStyle(node).appearance') == 'none'
            assert page.locator('#settingTrackerEnabled').evaluate(
                'node => getComputedStyle(node).appearance') == 'none'
            page.locator('#settingTrackerAnchor').select_option('bottom-left')
            assert page.locator('#settingTrackerAnchor').input_value() == 'bottom-left'
            page.locator('[data-settings-page="controls"]').click()
            page.locator('#settingJournalHotkey').select_option('F7')
            assert page.locator('#settingJournalHotkey').input_value() == 'F7'
            page.locator('#settingTrackerHotkey').select_option('F11')
            assert page.locator('#settingTrackerHotkey').input_value() == 'F11'
            page.locator('#settingJournalHotkey').select_option('F11')
            assert page.locator('#settingJournalHotkey').input_value() == 'F7'
            page.locator('#backToJournal').click()
            assert page.locator('#journalWorkspace').is_visible()
        else:
            assert page.locator(".tracked-entry").count() == 2
            # Hidden quests must also disappear from the passive tracker.
            page.evaluate('window.receiveQuestSnapshot({"ready":true,"quests":[{"id":"0001A001","state":"active","pinned":true,"hidden":true}]})')
            assert page.locator(".tracked-entry").count() == 0
            page.evaluate('window.receiveQuestSnapshot({"ready":true,"quests":[{"id":"0000A001","name":"A","state":"active","pinned":true,"objectives":[{"index":1,"state":"completed","text":"Finished step"}]},{"id":"0000A002","name":"B","state":"active","pinned":true,"objectives":[]}]})')
            page.evaluate('window.receiveJournalSettings({"trackerAnchor":"bottom-right","trackerScale":120,"trackerMaxQuests":1,"trackerShowCompleted":true})')
            assert page.locator('.tracked-entry').count() == 1
            assert 'Finished step' in page.locator('.tracked-entry').inner_text()
            assert page.locator('body').get_attribute('class') == 'tracker-page anchor-bottom-right'
            # Changing any anchor must clear the opposite edges; unlike a
            # static selector assertion, this tests actual inline placement.
            for anchor, edges in {
                'top-right': ('5%', 'auto', 'auto', '3%'),
                'top-left': ('5%', 'auto', '3%', 'auto'),
                'bottom-right': ('auto', '5%', 'auto', '3%'),
                'bottom-left': ('auto', '5%', '3%', 'auto'),
            }.items():
                page.evaluate('window.receiveJournalSettings({"trackerAnchor":"' + anchor + '"})')
                actual = page.locator('#tracker').evaluate(
                    'node => [node.style.top,node.style.bottom,node.style.left,node.style.right]')
                assert tuple(actual) == edges, (anchor, actual)
        assert not errors, errors
        page.close()
        print(f"PASS: {name} browser-only interface")
    browser.close()
