#include "JournalSettings.h"
#include "JournalGameplayChord.h"
#include "TrackerLayoutGeometry.h"
#include <algorithm>
#include <array>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <windows.h>

namespace K2040::Settings {
namespace {
    // An Amethyst-managed Data directory may be immutable or redirected.
    // Store mutable player preferences in the per-user Windows/Proton profile,
    // and read the legacy game-relative INI only for backwards compatibility.
    constexpr auto kLegacyPath = "Data/F4SE/Plugins/K2040_Quest_Journal.ini";
    std::filesystem::path UserPath() {
        std::array<wchar_t, 32768> directory{};
        const auto length=GetEnvironmentVariableW(L"LOCALAPPDATA",directory.data(),
                                                  static_cast<DWORD>(directory.size()));
        if(!length || length>=directory.size())return {};
        return std::filesystem::path(directory.data()) /
               "K2040" / "FO4-Quest-Journal.ini";
    }
    std::filesystem::path ActivePath() {
        const auto user=UserPath();
        return user.empty() ? std::filesystem::path(kLegacyPath) : user;
    }
    Preferences preferences;
    const char* lastFailureReason = "none";
    // Legacy controller user values are read once only. No writing to MCM.
    unsigned ReadLegacyPad(const std::filesystem::path& path,const char* requested) {
        std::ifstream file(path);
        std::string line;
        bool controller=false;
        while(std::getline(file,line)){
            if(line=="[Controller]" || line=="[Controller]\r") {
                controller=true;continue;
            }
            if(!line.empty()&&line[0]=='['){controller=false;continue;}
            if(!controller)continue;
            const auto split=line.find('=');
            if(split==line.npos || line.substr(0,split)!=requested)continue;
            const auto value=std::string_view(line).substr(split+1);
            unsigned n{};
            const auto [end,error]=std::from_chars(value.data(),value.data()+value.size(),n);
            return error==std::errc{} && (end==value.data()+value.size() ||
                   (end+1==value.data()+value.size()&&*end=='\r')) && n<=16 ?n:0;
        }
        return 0;
    }
    std::string StoredKeys(const std::filesystem::path& path) {
        std::ifstream input(path);
        std::string keys,line;
        while(std::getline(input,line)){
            const auto split=line.find('=');
            if(split!=line.npos)keys.append("|").append(line.substr(0,split)).append("|");
        }
        return keys;
    }
    bool Choice(std::string_view value, std::initializer_list<std::string_view> valid) {
        return std::find(valid.begin(), valid.end(), value) != valid.end();
    }
    bool Hex(std::string_view value) {
        if (value.size() != 7 || value[0] != '#') return false;
        for (std::size_t i=1; i<7; ++i)
            if (!((value[i]>='0' && value[i]<='9') || (value[i]>='a' && value[i]<='f') || (value[i]>='A' && value[i]<='F'))) return false;
        return true;
    }
    bool Int(std::string_view raw, int& result, int minValue, int maxValue) {
        int value{};
        if (raw.empty() || raw.size() > 4) return false;
        const auto [ptr,error] = std::from_chars(raw.data(),raw.data()+raw.size(),value);
        if (error != std::errc{} || ptr != raw.data()+raw.size() || value < minValue || value > maxValue) return false;
        result = value; return true;
    }
    bool Apply(std::string_view key, std::string_view val, Preferences& out) {
        if (key=="theme") { if (!Choice(val,{"commonwealth","pipboy","contrast","fallout76","custom"}))return false;out.theme=val; }
        else if (key=="accent" || key=="text" || key=="panel" || key=="completed") {
            if (!Hex(val))return false;
            if(key=="accent")out.accent=val;
            else if(key=="text")out.text=val;
            else if(key=="panel")out.panel=val;
            else out.completed=val;
        }
        else if (key=="journalTheme") {
            if(!Choice(val,{"default","pipboy","dark","contrast","fallout76","custom"}))return false;
            // Upgrade legacy K2040 Dark default to the newly requested live
            // Pip-Boy default. Dark remains explicitly selectable as "dark".
            out.journalTheme=val=="default"?"pipboy":std::string(val);
        }
        else if (key=="journalAccent" || key=="journalText" ||
                 key=="journalPanel" || key=="journalCompleted") {
            if(!Hex(val))return false;
            if(key=="journalAccent")out.journalAccent=val;
            else if(key=="journalText")out.journalText=val;
            else if(key=="journalPanel")out.journalPanel=val;
            else out.journalCompleted=val;
        }
        else if (key=="journalOpacity")return Int(val,out.journalOpacity,25,100);
        else if (key=="journalFontSize")return Int(val,out.journalFontSize,75,150);
        else if (key=="opacity")return Int(val,out.opacity,25,100);
        else if (key=="fontSize")return Int(val,out.fontSize,75,150);
        else if (key=="density") {if(!Choice(val,{"compact","comfortable"}))return false;out.density=val;}
        else if (key=="sidebarWidth")return Int(val,out.sidebarWidth,160,380);
        else if (key=="questListWidth")return Int(val,out.questListWidth,240,560);
        else if (key=="trackerAnchor") {if(!Choice(val,{"top-right","top-left","bottom-right","bottom-left"}))return false;out.trackerAnchor=val;}
        else if (key=="trackerScale")return Int(val,out.trackerScale,75,150);
        else if (key=="trackerWidth")return Int(val,out.trackerWidth,260,520);
        else if (key=="trackerOpacity")return Int(val,out.trackerOpacity,25,100);
        else if (key=="trackerLayoutEnabled") {
            if(val!="true"&&val!="false")return false;
            out.trackerLayoutEnabled=val=="true";
        }
        else if (key=="trackerLayoutX")return Int(val,out.trackerLayoutX,0,10000);
        else if (key=="trackerLayoutY")return Int(val,out.trackerLayoutY,0,10000);
        else if (key=="trackerLayoutWidth")return Int(val,out.trackerLayoutWidth,0,10000);
        else if (key=="trackerLayoutHeight")return Int(val,out.trackerLayoutHeight,0,10000);
        else if (key=="trackerMaxQuests")return Int(val,out.trackerMaxQuests,1,12);
        else if (key=="trackerShowCompleted" || key=="trackerAutoPinNew" || key=="trackerSyncPipboy") {
            if(val!="true" && val!="false")return false;
            if(key=="trackerShowCompleted")out.trackerShowCompleted=val=="true";
            else if(key=="trackerAutoPinNew")out.trackerAutoPinNew=val=="true";
            else out.trackerSyncPipboy=val=="true";
        }
        else if (key=="journalPadPrimary")return Int(val,out.journalPadPrimary,0,16);
        else if (key=="journalPadHeld")return Int(val,out.journalPadHeld,0,16);
        else if (key=="trackerPadPrimary")return Int(val,out.trackerPadPrimary,0,16);
        else if (key=="trackerPadHeld")return Int(val,out.trackerPadHeld,0,16);
        else if (key=="journalHotkey" || key=="trackerHotkey") {
            if (!K2040::JournalGameplayChord::Valid(val))return false;
            if(key=="journalHotkey")out.journalHotkey=val;
            else out.trackerHotkey=val;
        }
        else if (key=="trackerEnabled" || key=="trackerOverridePhosphor") {
            if(val!="true" && val!="false")return false;
            if(key=="trackerEnabled")out.trackerEnabled=val=="true";
            else out.trackerOverridePhosphor=val=="true";
        }
        else if (key=="redirectQuestKey") {
            if(val!="true" && val!="false")return false;
            out.redirectQuestKey=val=="true";
        }
        else return false;
        return true;
    }
}
void Load() {
    preferences=Preferences{};
    auto path=ActivePath();
    // Existing Data/F4SE settings are imported once if no user configuration
    // has yet been written. Do not overwrite or delete the legacy original.
    if(path != std::filesystem::path(kLegacyPath) &&
       !std::filesystem::exists(path))
        path=kLegacyPath;
    std::ifstream stream(path);
    std::string line;
    while (std::getline(stream,line)) {
        if (line.empty() || line[0]=='#' || line.size()>128)continue;
        const auto pos=line.find('=');if(pos==std::string::npos)continue;
        Apply(std::string_view(line).substr(0,pos),std::string_view(line).substr(pos+1),preferences);
    }
    if(preferences.journalHotkey==preferences.trackerHotkey)
        preferences.trackerHotkey=preferences.journalHotkey=="G"?"Shift+G":"G";
    // Preserve old controller bindings without writing to the retired
    // MCM Settings folder. Owner-local preferences take priority.
    const auto oldPath=std::filesystem::path(L"Data/MCM/Settings/K2040_Quest_Journal.ini");
    if(std::filesystem::is_regular_file(oldPath)){
        const auto saved=StoredKeys(path);
        const auto migrate=[&](const char* old,const char* current,int& value){
            if(saved.find(std::string("|")+current+"|")!=saved.npos)return;
            const auto val=ReadLegacyPad(oldPath,old);
            if(val<=16u)value=static_cast<int>(val);
        };
        migrate("iQuestJournalPadPrimary","journalPadPrimary",preferences.journalPadPrimary);
        migrate("iQuestJournalPadModifier","journalPadHeld",preferences.journalPadHeld);
        migrate("iQuestTrackerPadPrimary","trackerPadPrimary",preferences.trackerPadPrimary);
        migrate("iQuestTrackerPadModifier","trackerPadHeld",preferences.trackerPadHeld);
    }
    if(!preferences.journalPadPrimary ||
       preferences.journalPadHeld==preferences.journalPadPrimary)
        preferences.journalPadHeld=0;
    if(!preferences.trackerPadPrimary ||
       preferences.trackerPadHeld==preferences.trackerPadPrimary)
        preferences.trackerPadHeld=0;
    // Corrupt geometry must never enable an offscreen widget or overwrite INI.
    if(preferences.trackerLayoutEnabled &&
       !K2040::TrackerLayout::ValidBasis(preferences.trackerLayoutX,
           preferences.trackerLayoutY,preferences.trackerLayoutWidth,
           preferences.trackerLayoutHeight))preferences.trackerLayoutEnabled=false;
}
const Preferences& Current() {return preferences;}
std::string Serialize(std::string_view hudColor, std::string_view hudBackgroundColor,
                      std::string_view pipboyColor) {
    const auto& p=preferences;
    if (!Hex(hudColor)) hudColor="#20ff48";
    if (!Hex(hudBackgroundColor)) hudBackgroundColor="#001407";
    if (!Hex(pipboyColor)) pipboyColor=hudColor;
    return "{\"theme\":\""+p.theme+"\",\"accent\":\""+p.accent+"\",\"text\":\""+p.text+
        "\",\"panel\":\""+p.panel+"\",\"completed\":\""+p.completed+"\",\"opacity\":"+std::to_string(p.opacity)+
        ",\"fontSize\":"+std::to_string(p.fontSize)+",\"density\":\""+p.density+
        "\",\"sidebarWidth\":"+std::to_string(p.sidebarWidth)+",\"questListWidth\":"+std::to_string(p.questListWidth)+
        ",\"journalTheme\":\""+p.journalTheme+"\",\"journalAccent\":\""+p.journalAccent+
        "\",\"journalText\":\""+p.journalText+"\",\"journalPanel\":\""+p.journalPanel+
        "\",\"journalCompleted\":\""+p.journalCompleted+"\",\"journalOpacity\":"+std::to_string(p.journalOpacity)+
        ",\"journalFontSize\":"+std::to_string(p.journalFontSize)+
        ",\"trackerAnchor\":\""+p.trackerAnchor+"\",\"trackerScale\":"+std::to_string(p.trackerScale)+
        ",\"trackerWidth\":"+std::to_string(p.trackerWidth)+",\"trackerOpacity\":"+std::to_string(p.trackerOpacity)+
        ",\"trackerLayoutEnabled\":"+(p.trackerLayoutEnabled?"true":"false")+
        ",\"trackerLayoutX\":"+std::to_string(p.trackerLayoutX)+
        ",\"trackerLayoutY\":"+std::to_string(p.trackerLayoutY)+
        ",\"trackerLayoutWidth\":"+std::to_string(p.trackerLayoutWidth)+
        ",\"trackerLayoutHeight\":"+std::to_string(p.trackerLayoutHeight)+
        ",\"trackerEnabled\":"+(p.trackerEnabled?"true":"false")+
        ",\"trackerOverridePhosphor\":"+(p.trackerOverridePhosphor?"true":"false")+
        ",\"trackerMaxQuests\":"+std::to_string(p.trackerMaxQuests)+
        ",\"trackerShowCompleted\":"+(p.trackerShowCompleted?"true":"false")+
        ",\"trackerAutoPinNew\":"+(p.trackerAutoPinNew?"true":"false")+
        ",\"trackerSyncPipboy\":"+(p.trackerSyncPipboy?"true":"false")+
        ",\"journalPadPrimary\":"+std::to_string(p.journalPadPrimary)+
        ",\"journalPadHeld\":"+std::to_string(p.journalPadHeld)+
        ",\"trackerPadPrimary\":"+std::to_string(p.trackerPadPrimary)+
        ",\"trackerPadHeld\":"+std::to_string(p.trackerPadHeld)+
        ",\"journalHotkey\":\""+p.journalHotkey+"\",\"trackerHotkey\":\""+p.trackerHotkey+
        "\",\"hudColor\":\""+std::string(hudColor)+"\",\"hudBackgroundColor\":\""+std::string(hudBackgroundColor)+"\",\"pipboyColor\":\""+std::string(pipboyColor)+"\",\"redirectQuestKey\":"+(p.redirectQuestKey?"true":"false")+"}";
}
bool Change(std::string_view command) {
    if (command.empty() || command.size()>128) return false;
    const auto p=command.find(':');
    if (p==std::string_view::npos)return false;
    auto next=preferences;
    if (!Apply(command.substr(0,p),command.substr(p+1),next))return false;
    // Legacy width and anchor controls remain effective after a custom edit.
    const auto key=command.substr(0,p);
    if(key=="trackerAnchor"||key=="trackerWidth")
        next.trackerLayoutEnabled=false;
    if(next.trackerLayoutEnabled &&
       !K2040::TrackerLayout::ValidBasis(next.trackerLayoutX,next.trackerLayoutY,
           next.trackerLayoutWidth,next.trackerLayoutHeight))return false;
    if(next.journalHotkey==next.trackerHotkey)return false;
    if((!next.journalPadPrimary && next.journalPadHeld) ||
       (!next.trackerPadPrimary && next.trackerPadHeld) ||
       (next.journalPadPrimary && next.journalPadPrimary==next.journalPadHeld) ||
       (next.trackerPadPrimary && next.trackerPadPrimary==next.trackerPadHeld) ||
       (next.journalPadPrimary && next.journalPadPrimary==next.trackerPadPrimary))
        return false;
    preferences=std::move(next);
    return true;
}
bool Save() {
    const std::filesystem::path dest=ActivePath();
    const std::filesystem::path staging=dest.wstring()+L".tmp";
    std::error_code error;
    std::filesystem::create_directories(dest.parent_path(),error);
    if (error)return false;
    {
        std::ofstream file(staging, std::ios::trunc);
        if(!file)return false;
        const auto& p=preferences;
        file << "# K2040 Quest Journal - validated global appearance settings\n"
             << "theme=" << p.theme << "\naccent=" << p.accent << "\ntext=" << p.text
             << "\npanel=" << p.panel << "\ncompleted=" << p.completed << "\nopacity=" << p.opacity
             << "\nfontSize=" << p.fontSize
             << "\njournalTheme=" << p.journalTheme
             << "\njournalAccent=" << p.journalAccent
             << "\njournalText=" << p.journalText
             << "\njournalPanel=" << p.journalPanel
             << "\njournalCompleted=" << p.journalCompleted
             << "\njournalOpacity=" << p.journalOpacity
             << "\njournalFontSize=" << p.journalFontSize
             << "\ndensity=" << p.density
             << "\nsidebarWidth=" << p.sidebarWidth << "\nquestListWidth=" << p.questListWidth
             << "\ntrackerAnchor=" << p.trackerAnchor << "\ntrackerScale=" << p.trackerScale
             << "\ntrackerWidth=" << p.trackerWidth << "\ntrackerOpacity=" << p.trackerOpacity
             << "\ntrackerLayoutEnabled=" << (p.trackerLayoutEnabled?"true":"false")
             << "\ntrackerLayoutX=" << p.trackerLayoutX
             << "\ntrackerLayoutY=" << p.trackerLayoutY
             << "\ntrackerLayoutWidth=" << p.trackerLayoutWidth
             << "\ntrackerLayoutHeight=" << p.trackerLayoutHeight
             << "\ntrackerEnabled=" << (p.trackerEnabled ? "true":"false")
             << "\ntrackerOverridePhosphor=" << (p.trackerOverridePhosphor ? "true":"false")
             << "\ntrackerMaxQuests=" << p.trackerMaxQuests
             << "\ntrackerShowCompleted=" << (p.trackerShowCompleted ? "true":"false")
             << "\ntrackerAutoPinNew=" << (p.trackerAutoPinNew ? "true":"false")
             << "\ntrackerSyncPipboy=" << (p.trackerSyncPipboy ? "true":"false")
             << "\njournalHotkey=" << p.journalHotkey
             << "\ntrackerHotkey=" << p.trackerHotkey
             << "\njournalPadPrimary=" << p.journalPadPrimary
             << "\njournalPadHeld=" << p.journalPadHeld
             << "\ntrackerPadPrimary=" << p.trackerPadPrimary
             << "\ntrackerPadHeld=" << p.trackerPadHeld
             << "\nredirectQuestKey=" << (p.redirectQuestKey ? "true":"false") << '\n';
        file.flush();if(!file)return false;
    }
    if (!MoveFileExW(staging.wstring().c_str(),dest.wstring().c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) {
        std::filesystem::remove(staging,error);return false;
    }
    return true;
}
}

namespace K2040::Settings {
bool ResetToDefaults() {
    const Preferences before=preferences;
    preferences=Preferences{};
    // Only the Journal may change its one-way Pip-Boy HUD mirror setting.
    preferences.trackerSyncPipboy=before.trackerSyncPipboy;
    if(Save()) {
        lastFailureReason="none";
        return true;
    }
    preferences=before;
    lastFailureReason="storage";
    return false;
}
}

namespace K2040::Settings {
bool ChangeAndSave(std::string_view command) {
    const Preferences before=preferences;
    if(!Change(command)) {
        lastFailureReason="invalid";
        return false;
    }
    if(Save()) {
        lastFailureReason="none";
        return true;
    }
    preferences=before;
    lastFailureReason="storage";
    return false;
}
bool SaveTrackerLayout(int x,int y,int width,int height) {
    if(!K2040::TrackerLayout::ValidBasis(x,y,width,height)) {
        lastFailureReason="invalid";return false;
    }
    const Preferences before=preferences;
    preferences.trackerLayoutEnabled=true;
    preferences.trackerLayoutX=x;
    preferences.trackerLayoutY=y;
    preferences.trackerLayoutWidth=width;
    preferences.trackerLayoutHeight=height;
    if(Save()) {lastFailureReason="none";return true;}
    preferences=before;
    lastFailureReason="storage";
    return false;
}
const char* LastFailureReason() {
    return lastFailureReason;
}
}
