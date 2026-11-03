// Quest Journal self-owned native-MCM bridge. No global registry file writes.
#include "HubBindings.h"
#include <RE/Fallout.h>
#include <Scaleform/G/GFx_Movie.h>
#include <Scaleform/G/GFx_Value.h>
#include <Windows.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace K2040::Hub {
namespace {
    constexpr const char* kOwner="K2040_Quest_Journal";
    constexpr std::array<const char*,2> kIds{"questJournalToggle","questTrackerToggle"};
    const auto kMarker=std::filesystem::path(L"Data/MCM/Settings/K2040_Quest_Journal.hotkeys-initialized");
    struct Record {
        std::string owner,id;
        unsigned key{},mods{};
        bool operator==(const Record&) const = default;
    };
    struct Pending {
        unsigned key{},mods{},originalKey{},originalMods{};
        bool originalPresent{};
    };
    std::mutex mu;
    std::array<std::optional<Keyboard>,2> cached{};
    std::array<std::optional<Pending>,2> pending{};
    // A first-run user-clear counts as an intentional Unbound assignment,
    // not permission to regenerate F8/F10 on the next PauseMenu tick.
    std::array<bool,2> intentionalClear{};
    // Only identical consecutive native snapshots permit stale-zero cleanup.
    // UI thread only; no retained pointers to MCM's AS3 objects.
    std::optional<std::vector<Record>> priorLive;
    unsigned slot(Action a){return a==Action::Journal?0u:1u;}
    bool validKey(unsigned key,unsigned mods){
        if(key>260 || mods>7 || (!key&&mods))return false;
        if(key==VK_SHIFT||key==VK_CONTROL||key==VK_MENU||
           key==VK_LSHIFT||key==VK_RSHIFT||key==VK_LCONTROL||
           key==VK_RCONTROL||key==VK_LMENU||key==VK_RMENU||
           key==VK_NUMLOCK||key==VK_ESCAPE||key==VK_F9||
           (key=='J'&&!mods))return false;
        return true;
    }
    std::optional<std::vector<Record>> ReadLive(Scaleform::GFx::Movie* movie){
        Scaleform::GFx::Value records;
        if(!movie->Invoke("root.mcm.GetAllKeybinds",&records,nullptr,0) ||
           !records.IsArray() || records.GetArraySize()>2048)return {};
        std::vector<Record> out;
        for(unsigned i=0;i<records.GetArraySize();++i){
            Scaleform::GFx::Value row,owner,id,key,mods;
            if(!records.GetElement(i,&row)||!row.IsObject()||
               !row.GetMember("modName",&owner)||!owner.IsString()||
               !row.GetMember("keybindID",&id)||!id.IsString()||
               !row.GetMember("keycode",&key)||!key.IsInt()||
               !row.GetMember("modifiers",&mods)||!mods.IsInt()||
               key.GetInt()<0||mods.GetInt()<0)return {};
            if(static_cast<unsigned>(key.GetInt())>260 ||
               static_cast<unsigned>(mods.GetInt())>7)return {};
            out.push_back({owner.GetString(),id.GetString(),
                static_cast<unsigned>(key.GetInt()),
                static_cast<unsigned>(mods.GetInt())});
        }
        return out;
    }
    std::vector<Record> Owned(const std::vector<Record>& all,unsigned which){
        std::vector<Record> own;
        for(const auto& record:all)
            if(record.owner==kOwner&&record.id==kIds[which])
                own.push_back(record);
        return own;
    }
    bool Occupied(const std::vector<Record>& all,unsigned key,unsigned mods,
                  unsigned ignore){
        if(!key)return false;
        for(const auto& record:all)
            if(record.key==key && record.mods==mods &&
               !(record.owner==kOwner&&record.id==kIds[ignore]))
                return true;
        return false;
    }
    std::optional<Record> QueryNative(Scaleform::GFx::Movie* movie,unsigned which){
        Scaleform::GFx::Value args[2],value,key,mods;
        args[0]=kOwner;args[1]=kIds[which];
        if(!movie->Invoke("root.mcm.GetKeybind",&value,args,2)||
           !value.IsObject() || !value.GetMember("keycode",&key)||
           !value.GetMember("modifiers",&mods)||
           !key.IsInt() || !mods.IsInt() ||
           key.GetInt()<0 || mods.GetInt()<0)return {};
        return Record{kOwner,kIds[which],
                      static_cast<unsigned>(key.GetInt()),
                      static_cast<unsigned>(mods.GetInt())};
    }
    // Only one unbound and one real registration for this exact action.
    // If ANY other action uses (0,0), native ClearKeybind(0,0) is unsafe.
    std::optional<Record> UniqueStaleZeroSurvivor(
        const std::vector<Record>& all,unsigned which){
        const auto own=Owned(all,which);
        if(own.size()!=2)return {};
        if(std::count_if(all.begin(),all.end(),[](const Record& r){
            return r.key==0 && r.mods==0;
        })!=1)return {};
        if(std::count_if(own.begin(),own.end(),[](const Record& r){
            return r.key==0 && r.mods==0;
        })!=1)return {};
        for(const auto& item:own)if(item.key && validKey(item.key,item.mods) &&
            !Occupied(all,item.key,item.mods,which))return item;
        return {};
    }
    bool InvokeUpdate(Scaleform::GFx::Movie* movie,const char* method,
                      unsigned which,unsigned key,unsigned mods){
        Scaleform::GFx::Value args[4],result;
        args[0]=kOwner; args[1]=kIds[which];
        args[2]=static_cast<std::int32_t>(key);
        args[3]=static_cast<std::int32_t>(mods);
        return movie->Invoke(method,&result,args,4)&&
               result.IsBoolean()&&result.GetBoolean();
    }
    bool InvokeClear(Scaleform::GFx::Movie* movie,unsigned key,unsigned mods){
        Scaleform::GFx::Value args[2],result;
        args[0]=static_cast<std::int32_t>(key);
        args[1]=static_cast<std::int32_t>(mods);
        return movie->Invoke("root.mcm.ClearKeybind",&result,args,2)&&
               result.IsBoolean()&&result.GetBoolean();
    }
    void RefreshCache(const std::vector<Record>& all){
        std::lock_guard lock(mu);
        for(unsigned i=0;i<2;++i){
            const auto own=Owned(all,i);
            Keyboard value{};value.installed=true;
            // Duplicate/ambiguous native entries are never auto-deleted.
            // They must not drive the game's fallback hotkey poll.
            value.conflict=own.size()>1;
            value.registered=own.size()==1;
            if(own.size()==1){
                value.keycode=own[0].key;value.modifiers=own[0].mods;
                value.assigned=value.keycode && validKey(value.keycode,value.modifiers);
                value.conflict |= value.assigned &&
                    Occupied(all,value.keycode,value.modifiers,i);
            }
            // A cleared/missing action is genuinely Unbound once initialized.
            if(std::filesystem::is_regular_file(kMarker))
                value.registered=true;
            cached[i]=value;
        }
    }
}
Keyboard CachedNativeMcmBinding(Action action) noexcept {
    try {
        std::lock_guard lock(mu);
        return cached[slot(action)].value_or(Keyboard{});
    }catch(...){return {};}
}
bool StageNativeMcmBinding(Action action,unsigned key,unsigned mods) noexcept {
    try {
        if(!NativeMcmOwnsRegistry()||!validKey(key,mods))return false;
        // Stage only: do not write native MCM's disk registry.
        const auto old=ReadKeyboard(action);
        if(old.conflict)return false;
        std::lock_guard lock(mu);
        pending[slot(action)]=Pending{key,mods,old.keycode,old.modifiers,
                                     old.assigned || (old.registered && old.keycode!=0)};
        return true;
    }catch(...){return false;}
}
void RefreshNativeMcm() noexcept {
    try {
        if(!NativeMcmOwnsRegistry())return;
        auto* ui=RE::UI::GetSingleton();
        if(!ui || !ui->GetMenuOpen(RE::BSFixedString("PauseMenu"))){
            priorLive.reset();
            return;
        }
        const auto menu=ui->GetMenu(RE::BSFixedString("PauseMenu"));
        if(!menu || !menu->uiMovie)return;
        auto* movie=menu->uiMovie.get();
        auto live=ReadLive(movie);
        if(!live){
            priorLive.reset();
            return; // missing Categorizer bridge is a no-op
        }
        const bool stable=priorLive && *priorLive==*live;
        priorLive=*live;
        RefreshCache(*live);
        if(stable){
            for(unsigned i=0;i<2;++i){
                const auto survivor=UniqueStaleZeroSurvivor(*live,i);
                if(!survivor)continue;
                if(!InvokeClear(movie,0,0))continue;
                const auto after=ReadLive(movie);
                if(!after)break;
                auto expected=*live;
                const auto stale=std::find_if(expected.begin(),expected.end(),
                    [&](const Record& rec){
                        return rec.owner==kOwner && rec.id==kIds[i] &&
                               rec.key==0 && rec.mods==0;
                    });
                if(stale==expected.end())break;
                expected.erase(stale);
                if(*after!=expected || Owned(*after,i).size()!=1 ||
                   !QueryNative(movie,i) ||
                   QueryNative(movie,i)->key!=survivor->key ||
                   QueryNative(movie,i)->mods!=survivor->mods){
                    REX::WARN("Quest Journal native MCM duplicate cleanup "
                              "verification failed; leaving current manager state untouched");
                }else{
                    REX::WARN("Quest Journal native MCM: repaired one "
                              "unambiguous stale zero-chord duplicate");
                }
                priorLive.reset();
                RefreshCache(*after);
                return; // Do not mutate any other MCM record this cycle.
            }
        }

        // First-run defaults ONLY through the native registry, never
        // by synthesizing 0/0 or editing Keybinds.json preloader-side.
        if(!std::filesystem::is_regular_file(kMarker)){
            bool complete=true;
            for(unsigned i=0;i<2;++i){
                bool staged=false,cleared=false;
                {
                    std::lock_guard lock(mu);
                    staged=pending[i].has_value();
                    cleared=intentionalClear[i];
                }
                if(staged){complete=false;continue;}
                auto own=Owned(*live,i);
                if(own.size()>1){complete=false;break;}
                if(own.empty() && !cleared){
                    const unsigned key='G';
                    const unsigned mods=i?0u:1u;
                    if(Occupied(*live,key,mods,i))continue;
                    if(!InvokeUpdate(movie,"root.mcm.SetKeybind",i,key,mods)){
                        complete=false;break;
                    }
                    live=ReadLive(movie);
                    if(!live||Owned(*live,i).size()!=1 ||
                       !QueryNative(movie,i) ||
                       QueryNative(movie,i)->key!=key ||
                       QueryNative(movie,i)->mods!=mods){complete=false;break;}
                }
            }
            if(complete){
                std::error_code ec;
                std::filesystem::create_directories(kMarker.parent_path(),ec);
                if(!ec){
                    std::ofstream f(kMarker,std::ios::binary|std::ios::trunc);
                    if(f)f<<"initialized=1\n";
                }
                RefreshCache(*live);
            }
        }
        // Respect fresh MCM edits and reject duplicates rather than
        // destroying unknown or cross-mod bindings.
        for(unsigned i=0;i<2;++i){
            std::optional<Pending> request;
            {
                std::lock_guard lock(mu);
                request=pending[i];
            }
            if(!request)continue;
            live=ReadLive(movie);
            if(!live)break;
            const auto own=Owned(*live,i);
            if(own.size()>1)continue;
            const bool present=own.size()==1;
            const unsigned beforeKey=present?own[0].key:0u;
            const unsigned beforeMods=present?own[0].mods:0u;
            // An MCM GUI edit supersedes a stale Settings request.
            if(present!=request->originalPresent ||
               (present && (beforeKey!=request->originalKey ||
                            beforeMods!=request->originalMods))) {
                {
                    std::lock_guard lock(mu);
                    if(pending[i] && pending[i]->key==request->key &&
                       pending[i]->mods==request->mods)pending[i].reset();
                }
                RefreshCache(*live);
                continue;
            }
            if(request->key && Occupied(*live,request->key,request->mods,i))
                continue;
            // ClearKeybind operates by chord, not action ID; never clear a
            // duplicate/foreign key or an ambiguous zero/unbound chord.
            if(!request->key && present &&
               (!beforeKey || Occupied(*live,beforeKey,beforeMods,i)))
                continue;
            bool success=false;
            if(request->key==beforeKey && request->mods==beforeMods)success=true;
            else if(!request->key)
                success=!present || InvokeClear(movie,beforeKey,beforeMods);
            else
                success=InvokeUpdate(movie,present?"root.mcm.RemapKeybind":
                                    "root.mcm.SetKeybind",i,request->key,
                                     request->mods);
            if(!success)continue;
            const auto after=ReadLive(movie);
            if(!after)continue;
            const auto updated=Owned(*after,i);
            if((request->key==0 && updated.empty()) ||
               (request->key!=0 && updated.size()==1 &&
                updated[0].key==request->key && updated[0].mods==request->mods &&
                QueryNative(movie,i) &&
                QueryNative(movie,i)->key==request->key &&
                QueryNative(movie,i)->mods==request->mods)){
                {
                    std::lock_guard lock(mu);
                    if(pending[i]&&pending[i]->key==request->key&&
                       pending[i]->mods==request->mods){
                        intentionalClear[i]=request->key==0;
                        pending[i].reset();
                    }
                }
                RefreshCache(*after);
            }
        }
    }catch(...){/* no native MCM or unexpected runtime schema: fail closed */}
}
}
