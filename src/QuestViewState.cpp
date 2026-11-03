#include "QuestViewState.h"
#include <F4SE/F4SE.h>
#include <F4SE/Interfaces.h>
#include <array>
#include <string_view>
#include <unordered_map>
#include <algorithm>
#include <vector>

namespace K2040::QuestViewState {
namespace {
    constexpr std::uint32_t FourCC(char a,char b,char c,char d) {
        return std::uint32_t(a) | (std::uint32_t(b)<<8) | (std::uint32_t(c)<<16) | (std::uint32_t(d)<<24);
    }
    constexpr std::uint32_t kPluginID=FourCC('K','Q','J','L');
    constexpr std::uint32_t kRecord=FourCC('Q','V','S','1');
    constexpr std::uint32_t kCategoryRecord=FourCC('Q','V','C','1');
    constexpr std::uint32_t kObservedRecord=FourCC('Q','V','O','1');
    constexpr std::uint32_t kVersion=1, kLimit=2048;
    struct Header { std::uint32_t pins, favorites, hidden; };
    static_assert(sizeof(Header)==12);
    std::array<std::unordered_set<std::uint32_t>,3> entries;
    // Append-only numeric enum values persisted with co-saves.
    constexpr std::array<std::string_view,14> kCategories{
        "", "Main", "Minutemen", "Brotherhood", "Railroad", "Institute", "Side",
        "Misc", "Automatron", "Far Harbor", "Nuka-World", "Wasteland Workshop",
        "Modded", "Other"
    };
    struct CategoryEntry {std::uint32_t id;std::uint32_t category;};
    static_assert(sizeof(CategoryEntry)==8);
    std::unordered_map<std::uint32_t,std::uint32_t> categoryOverrides;
    std::unordered_set<std::uint32_t> observed;
    bool initialFeedSeen = false;

    void F4SEAPI SaveCallback(const F4SE::SerializationInterface* archive) {
        if(!archive)return;
        std::array<std::vector<std::uint32_t>,3> lists;
        for(std::size_t i=0;i<entries.size();++i) {
            lists[i].assign(entries[i].begin(),entries[i].end());
            std::sort(lists[i].begin(),lists[i].end());
        }
        if(!archive->OpenRecord(kRecord,kVersion))return;
        const Header header{std::uint32_t(lists[0].size()),std::uint32_t(lists[1].size()),std::uint32_t(lists[2].size())};
        if(!archive->WriteRecordData(header))return;
        for(const auto& list:lists)
            if(!list.empty() && !archive->WriteRecordData(list.data(),std::uint32_t(list.size()*sizeof(std::uint32_t))))return;
        std::vector<CategoryEntry> categories;
        for(const auto& [id,category]:categoryOverrides)
            if(id && category && category<kCategories.size())categories.push_back({id,category});
        std::sort(categories.begin(),categories.end(),[](const auto& a,const auto& b){return a.id<b.id;});
        if(!archive->OpenRecord(kCategoryRecord,kVersion))return;
        const auto count=static_cast<std::uint32_t>(categories.size());
        if(!archive->WriteRecordData(count))return;
        if(!categories.empty() && !archive->WriteRecordData(categories.data(),count*sizeof(CategoryEntry)))return;
        // Independent encountered-quest index, bounded and sorted deterministically.
        std::vector<std::uint32_t> known(observed.begin(),observed.end());
        std::sort(known.begin(),known.end());
        if(!archive->OpenRecord(kObservedRecord,kVersion))return;
        const auto knownCount=static_cast<std::uint32_t>(known.size());
        if(!archive->WriteRecordData(knownCount))return;
        if(!known.empty())archive->WriteRecordData(known.data(),knownCount*sizeof(std::uint32_t));
    }
    void F4SEAPI RevertCallback(const F4SE::SerializationInterface*) {
        for(auto& list:entries)list.clear();
        categoryOverrides.clear();
        observed.clear();
        initialFeedSeen=false;
    }
    void F4SEAPI LoadCallback(const F4SE::SerializationInterface* archive) {
        for(auto& list:entries)list.clear();
        categoryOverrides.clear();
        observed.clear();
        initialFeedSeen=false;
        if(!archive)return;
        std::uint32_t type=0,version=0,length=0;
        while(archive->GetNextRecordInfo(type,version,length)) {
            if(type==kObservedRecord && version==kVersion && length>=sizeof(std::uint32_t)) {
                std::uint32_t count{};
                if(archive->ReadRecordData(count)!=sizeof(count) || count>kLimit ||
                   length!=sizeof(count)+std::uint64_t(count)*sizeof(std::uint32_t))continue;
                std::unordered_set<std::uint32_t> restored;
                bool valid=true;
                for(std::uint32_t i=0;i<count;++i) {
                    std::uint32_t id{};
                    if(archive->ReadRecordData(id)!=sizeof(id)){valid=false;break;}
                    if(!id)continue;
                    const auto mapped=archive->ResolveFormID(id);
                    if(mapped && *mapped)restored.insert(*mapped);
                }
                if(valid)observed=std::move(restored);
                continue;
            }
            if(type==kCategoryRecord && version==kVersion && length>=sizeof(std::uint32_t)) {
                std::uint32_t count{};
                if(archive->ReadRecordData(count)!=sizeof(count) || count>kLimit ||
                   length!=sizeof(count)+std::uint64_t(count)*sizeof(CategoryEntry))continue;
                std::unordered_map<std::uint32_t,std::uint32_t> restored;
                bool valid=true;
                for(std::uint32_t i=0;i<count;++i) {
                    CategoryEntry item{};
                    if(archive->ReadRecordData(item)!=sizeof(item)){valid=false;break;}
                    if(!item.id || !item.category || item.category>=kCategories.size())continue;
                    auto newId=archive->ResolveFormID(item.id);
                    if(newId && *newId)restored[*newId]=item.category;
                }
                if(valid)categoryOverrides=std::move(restored);
                continue;
            }
            if(type!=kRecord || version!=kVersion || length<sizeof(Header))continue;
            Header header{};
            if(archive->ReadRecordData(header)!=sizeof(Header))continue;
            const std::array<std::uint32_t,3> sizes{header.pins,header.favorites,header.hidden};
            const std::uint64_t total=std::uint64_t(header.pins)+header.favorites+header.hidden;
            if(std::any_of(sizes.begin(),sizes.end(),[](auto n){return n>kLimit;}) ||
                sizeof(Header)+total*sizeof(std::uint32_t)!=length)continue;
            std::array<std::unordered_set<std::uint32_t>,3> restored;
            bool valid=true;
            for(std::size_t i=0;i<sizes.size();++i) {
                std::vector<std::uint32_t> old(sizes[i]);
                if(!old.empty() && archive->ReadRecordData(old.data(),std::uint32_t(old.size()*sizeof(std::uint32_t)))!=old.size()*sizeof(std::uint32_t)) {
                    valid=false;break;
                }
                for(const auto id:old) {
                    if(!id)continue;
                    const auto resolved=archive->ResolveFormID(id);
                    if(resolved && *resolved)restored[i].insert(*resolved);
                }
            }
            if(valid)entries=std::move(restored);
        }
    }
}
const std::unordered_set<std::uint32_t>& Get(Kind kind) {
    return entries[static_cast<std::size_t>(kind)];
}
void Clear() {for(auto& list:entries)list.clear();categoryOverrides.clear();observed.clear();initialFeedSeen=false;}
bool Toggle(Kind kind,std::uint32_t id) {
    if(!id)return false;
    auto& set=entries[static_cast<std::size_t>(kind)];
    if(set.erase(id))return true; // Removal is a successful mutation too.
    if(set.size()>=kLimit)return false;
    return set.insert(id).second;
}
bool SetCategory(std::uint32_t id, std::string_view category) {
    if(!id)return false;
    if(category.empty() || category=="Automatic") {
        categoryOverrides.erase(id);
        return true;
    }
    for(std::uint32_t index=1;index<kCategories.size();++index) {
        if(category==kCategories[index]) {
            if(!categoryOverrides.contains(id) && categoryOverrides.size()>=kLimit)return false;
            categoryOverrides[id]=index;
            return true;
        }
    }
    return false;
}
std::string_view CategoryFor(std::uint32_t id) {
    const auto iter=categoryOverrides.find(id);
    return iter!=categoryOverrides.end() && iter->second<kCategories.size() ? kCategories[iter->second] : std::string_view{};
}
bool Remember(std::uint32_t id) {
    if(!id)return false;
    if(observed.contains(id))return true;
    if(observed.size()>=kLimit)return false;
    return observed.insert(id).second;
}
const std::unordered_set<std::uint32_t>& Observed() {return observed;}
bool InitialFeedSeen() {return initialFeedSeen;}
void CompleteInitialFeed() {initialFeedSeen=true;}
// Auto-pins share the existing per-save QVS1 set, but never toggle an
// already-pinned quest or resurrect a manually unpinned observed quest.
bool PinNew(std::uint32_t id) {
    if(!id || entries[0].size()>=kLimit)return false;
    return entries[0].insert(id).second;
}
void RegisterSerialization() {
    if(const auto* archive=F4SE::GetSerializationInterface()) {
        archive->SetUniqueID(kPluginID);
        archive->SetSaveCallback(SaveCallback);
        archive->SetLoadCallback(LoadCallback);
        archive->SetRevertCallback(RevertCallback);
    }
}
}
