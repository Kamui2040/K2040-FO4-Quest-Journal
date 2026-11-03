// OG163 read-only research adapter. Outside src/**.cpp; linked ONLY when
// the separate default-OFF quest_pipboy_selection_readonly_qa option
// is enabled. No production plugin or writer build includes it.
// Called only by the one-shot F4SE UI-thread-only QA entrypoint.
#include "ReadOnlySelectedEntry.h"
#ifdef K2040_PIPBOY_SELECTION_SCHEMA_READONLY_QA
#include "ReadOnlySelectedSchema.h"
#include "ReadOnlyCodeObjectBinding.h"
#endif
#include <RE/Fallout.h>
#include <Scaleform/G/GFx_Movie.h>
#include <Scaleform/G/GFx_Value.h>

namespace K2040::PipboySelectedEntryResearch {
[[nodiscard]] Observation ObserveOpenPipboyOnUIThread() {
    auto* ui=RE::UI::GetSingleton();
    if(!ui||!ui->GetMenuOpen(RE::BSFixedString("PipboyMenu")))
        return {};
    const auto menu=ui->GetMenu(RE::BSFixedString("PipboyMenu"));
    if(!menu || !menu->uiMovie)return {};
    // No Invoke, SetVariable, SetMember, event publication, or pointers
    // retained beyond this synchronous read. No native command dispatch.
    return Inspect<Scaleform::GFx::Movie,Scaleform::GFx::Value>(
        *menu->uiMovie.get());
}
#ifdef K2040_PIPBOY_SELECTION_SCHEMA_READONLY_QA
[[nodiscard]] Schema ObserveOpenPipboySchemaOnUIThread() {
    auto* ui=RE::UI::GetSingleton();
    if(!ui||!ui->GetMenuOpen(RE::BSFixedString("PipboyMenu")))return {};
    const auto menu=ui->GetMenu(RE::BSFixedString("PipboyMenu"));
    if(!menu||!menu->uiMovie)return {};
    using Value=Scaleform::GFx::Value;
    Value root,page,tab,list,selected,children,child,visible;
    if(!menu->uiMovie->GetVariable(&root,"root.Menu_mc")||!Object(root) ||
       !root.GetMember("CurrentPage",&page)||!Object(page) ||
       !page.GetMember("QuestsTab_mc",&tab)||!Object(tab) ||
       !tab.GetMember("visible",&visible)||!visible.IsBoolean()||
       !visible.GetBoolean() ||
       !tab.GetMember("QuestsList_mc",&list)||!Object(list) ||
       !list.GetMember("selectedEntry",&selected)||!Object(selected))return {};
    std::uint32_t rootID{};
    if(!UIntMember(selected,"formID",&rootID)||rootID!=0 ||
       !tab.GetMember("ObjectivesList_mc",&children)||!Object(children) ||
       !children.GetMember("selectedEntry",&child)||!child.IsObject())return {};
    // Read only. Visitors never log or retain dynamic member names or values.
    return SummarizeSchema(child);
}
[[nodiscard]] CodeObjectBinding ObserveOpenPipboyCodeObjectOnUIThread() {
    auto* ui=RE::UI::GetSingleton();
    if(!ui||!ui->GetMenuOpen(RE::BSFixedString("PipboyMenu")))return {};
    const auto menu=ui->GetMenu(RE::BSFixedString("PipboyMenu"));
    if(!menu||!menu->uiMovie)return {};
    // Inspect exactly one known function member on UI thread; no Invoke.
    return InspectCodeObjectBinding<Scaleform::GFx::Movie,Scaleform::GFx::Value>(
        *menu->uiMovie.get());
}
#endif
}
