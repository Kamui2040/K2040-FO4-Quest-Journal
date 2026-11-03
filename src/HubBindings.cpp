#include "HubBindings.h"
#include <Windows.h>
#include <algorithm>
#include <array>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <mutex>
#include <optional>
#include <sstream>
#include <string_view>
#include <vector>

namespace K2040::Hub {
namespace {
 const auto config=std::filesystem::path(L"Data/MCM/Config/K2040_Quest_Journal/config.json");
 const auto legacyHubConfig=std::filesystem::path(L"Data/MCM/Config/K2040_Mods/config.json");
 const auto registry=std::filesystem::path(L"Data/MCM/Settings/Keybinds.json");
 const auto padUser=std::filesystem::path(L"Data/MCM/Settings/K2040_Quest_Journal.ini");
 const auto legacyPadUser=std::filesystem::path(L"Data/MCM/Settings/K2040_Mods.ini");
 const auto minimapPadUser=std::filesystem::path(L"Data/MCM/Settings/K2040_Prisma_Minimap.ini");
 const auto quickAttachPadUser=std::filesystem::path(L"Data/MCM/Settings/K2040_Quick_Attach_Menu.ini");
 std::mutex ioMutex;
 constexpr std::size_t kMaxRegistry=1024*1024,kMaxIni=128*1024;
 std::string_view ActionId(Action a){return a==Action::Journal?"questJournalToggle":"questTrackerToggle";}
 std::string_view PadPrimaryName(Action a){return a==Action::Journal?"iQuestJournalPadPrimary":"iQuestTrackerPadPrimary";}
 std::string_view PadHeldName(Action a){return a==Action::Journal?"iQuestJournalPadModifier":"iQuestTrackerPadModifier";}
 std::optional<std::string> ReadFile(const std::filesystem::path& p,std::size_t cap){
  std::error_code ec;
  if(!std::filesystem::is_regular_file(p,ec)||ec||std::filesystem::file_size(p,ec)>cap||ec)return std::nullopt;
  std::ifstream f(p,std::ios::binary);if(!f)return std::nullopt;
  std::string result((std::istreambuf_iterator<char>(f)),{});
  if(result.size()>cap||(!f.good()&&!f.eof()))return std::nullopt;
  return result;
 }
 std::optional<std::pair<std::size_t,std::size_t>> FieldRange(std::string_view s,std::string_view name){
  const std::string needle="\""+std::string(name)+"\"";
  auto i=s.find(needle);if(i==s.npos)return std::nullopt;
  i+=needle.size();
  while(i<s.size()&&(s[i]==' '||s[i]=='\n'||s[i]=='\r'||s[i]=='\t'))++i;
  if(i==s.size()||s[i++]!=':')return std::nullopt;
  while(i<s.size()&&(s[i]==' '||s[i]=='\n'||s[i]=='\r'||s[i]=='\t'))++i;
  if(i==s.size())return std::nullopt;
  auto end=i;
  if(s[i]=='"'){
   ++end;
   bool escape=false;
   for(;end<s.size();++end){
    if(escape){escape=false;continue;}
    if(s[end]=='\\'){escape=true;continue;}
    if(s[end]=='"')return std::pair{i,end+1};
   }
   return std::nullopt;
  }
  while(end<s.size()&&s[end]!=','&&s[end]!='}'&&s[end]!=']'&&s[end]!=' '&&s[end]!='\r'&&s[end]!='\n')++end;
  return end>i?std::optional<std::pair<std::size_t,std::size_t>>{{i,end}}:std::nullopt;
 }
 std::optional<std::string_view> StringField(std::string_view s,std::string_view name){
  const auto r=FieldRange(s,name);
  if(!r||s[r->first]!='"'||r->second-r->first<2)return std::nullopt;
  const auto data=s.substr(r->first+1,r->second-r->first-2);
  if(data.find('\\')!=data.npos)return std::nullopt;
  return data;
 }
 std::optional<unsigned> NumberField(std::string_view s,std::string_view name){
  const auto r=FieldRange(s,name);if(!r)return std::nullopt;
  unsigned v{};
  const auto value=s.substr(r->first,r->second-r->first);
  const auto result=std::from_chars(value.data(),value.data()+value.size(),v);
  if(result.ec!=std::errc{}||result.ptr!=value.data()+value.size())return std::nullopt;
  return v;
 }
 struct Object {std::size_t start{},end{};std::string_view text;};
 std::vector<Object> RegistryObjects(std::string_view s){
  std::vector<Object> out;
  const auto key=s.find("\"keybinds\"");if(key==s.npos)return out;
  const auto arr=s.find('[',key+10);if(arr==s.npos)return out;
  bool quote=false,escape=false;unsigned depth=0;
  std::size_t start=s.npos;
  for(auto i=arr+1;i<s.size();++i){
   const char ch=s[i];
   if(quote){if(escape)escape=false;else if(ch=='\\')escape=true;else if(ch=='"')quote=false;continue;}
   if(ch=='"'){quote=true;continue;}
   if(ch=='{'){if(depth++==0)start=i;}
   else if(ch=='}'){if(!depth)break;if(--depth==0&&start!=s.npos)out.push_back({start,i+1,s.substr(start,i-start+1)});}
   else if(ch==']'&&!depth)break;
  }
  return out;
 }
 bool IsOwned(const Object& object){
  return StringField(object.text,"modName")==std::optional<std::string_view>{"K2040_Quest_Journal"};
 }
 struct RegistryBinding {std::string_view id;unsigned code{},modifiers{};bool valid{};};
 RegistryBinding Decode(const Object& object){
  const auto id=StringField(object.text,"id");
  const auto key=NumberField(object.text,"keycode");
  const auto mod=NumberField(object.text,"modifiers");
  return {id.value_or(""),key.value_or(0),mod.value_or(0),IsOwned(object)&&id.has_value()&&key.has_value()&&mod.has_value()&&*key<=260&&*mod<=7};
 }
 bool ValidKey(unsigned key,unsigned modifiers){
  return key<=260&&modifiers<=7&&key!=VK_SHIFT&&key!=VK_CONTROL&&key!=VK_MENU&&
    key!=VK_LSHIFT&&key!=VK_RSHIFT&&key!=VK_LCONTROL&&key!=VK_RCONTROL&&
    key!=VK_LMENU&&key!=VK_RMENU&&key!=VK_NUMLOCK&&key!=VK_ESCAPE&&key!=VK_F9
    &&!(key=='J'&&modifiers==0);
 }
 bool WriteAtomic(const std::filesystem::path& destination,const std::string& body){
  std::error_code ec;
  std::filesystem::create_directories(destination.parent_path(),ec);if(ec)return false;
  auto temp=destination;temp+=L".k2040-journal.tmp";
  {std::ofstream out(temp,std::ios::binary|std::ios::trunc);if(!out)return false;
   out<<body;out.flush();if(!out){std::filesystem::remove(temp,ec);return false;}}
  if(!MoveFileExW(temp.wstring().c_str(),destination.wstring().c_str(),
        MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){
   std::filesystem::remove(temp,ec);return false;
  }
  return true;
 }
 std::string Trim(std::string v){
  auto isSpace=[](unsigned char c){return c==' '||c=='\r'||c=='\n'||c=='\t';};
  const auto f=std::find_if_not(v.begin(),v.end(),isSpace);
  const auto l=std::find_if_not(v.rbegin(),v.rend(),isSpace).base();
  return f<l?std::string(f,l):std::string{};
 }
 unsigned IniNumber(const std::string& data,std::string_view wanted){
  bool section=false;std::istringstream f(data);std::string line;
  while(std::getline(f,line)){
   const auto t=Trim(line);
   if(t.starts_with("[")){section=t=="[Controller]";continue;}
   if(!section||t.empty()||t[0]==';'||t[0]=='#')continue;
   const auto pos=t.find('=');
   if(pos==t.npos||Trim(t.substr(0,pos))!=wanted)continue;
   const auto v=Trim(t.substr(pos+1));unsigned result{};
   const auto res=std::from_chars(v.data(),v.data()+v.size(),result);
   return res.ec==std::errc{}&&res.ptr==v.data()+v.size()&&result<=16?result:0;
  }
  return 0;
 }
 struct ControllerRaw {unsigned primary{},held{};};
 ControllerRaw Pair(const std::string& data,std::string_view p,std::string_view h){
  return {IniNumber(data,p),IniNumber(data,h)};
 }
 bool Collision(ControllerRaw first,ControllerRaw other){
  if(!first.primary||!other.primary)return false;
  // Prefer fail-closed over two mod handlers firing on a shared button.
  return first.primary==other.primary ||
   (other.held && first.primary==other.held) ||
   (first.held && first.held==other.primary) ||
   (first.held && other.held && first.held==other.held);
 }
 bool PadCollision(Action action,const std::string& text,ControllerRaw now){
  const auto other=action==Action::Journal?Action::Tracker:Action::Journal;
  if(Collision(now,Pair(text,PadPrimaryName(other),PadHeldName(other))))return true;
  // Minimap may still be on the experimental shared INI or its own child INI.
  // Read both without changing either other mod's preferences.
  for(const auto& path:{minimapPadUser,legacyPadUser}){
   if(const auto external=ReadFile(path,kMaxIni)){
    if(Collision(now,Pair(*external,"iMinimapPadPrimary","iMinimapPadModifier")))return true;
   }
  }
  // Quick Attach's controller values belong to its child MCM INI,
  // never to the parent menu or Quest Journal user preferences.
  if(const auto quick=ReadFile(quickAttachPadUser,kMaxIni)){
   if(Collision(now,Pair(*quick,"iQuickAttachPadPrimary","iQuickAttachPadModifier")))return true;
  }
  return false;
 }
 std::string JsonBool(bool x){return x?"true":"false";}
 bool IsRetiredLegacy(const Object& obj){
  return StringField(obj.text,"modName")==std::optional<std::string_view>{"K2040_Mods"}
    && (StringField(obj.text,"id")==std::optional<std::string_view>{"questJournalToggle"} ||
        StringField(obj.text,"id")==std::optional<std::string_view>{"questTrackerToggle"});
 }
}
bool NativeMcmOwnsRegistry() noexcept {
 try {
  std::error_code ec;
  // Loader order is NOT proof MCM is absent. If the DLL is installed,
  // native MCM alone owns its persistent and live keybind registry.
  return GetModuleHandleW(L"mcm.dll") != nullptr ||
    std::filesystem::is_regular_file(L"Data/F4SE/Plugins/mcm.dll",ec);
 }catch(...){return true;} // fail closed, never compete with an unknown owner
}
bool Installed() noexcept {
 try{
  // A bundled config file is not proof that MCM is currently running.
  // Old registry entries may survive uninstall or temporary MCM disablement.
  if(!GetModuleHandleW(L"mcm.dll") &&
     !GetModuleHandleW(L"F4SEMenuFramework.dll"))return false;
  std::error_code ec;
  return std::filesystem::is_regular_file(config,ec)&&!ec;
 }catch(...){return false;}
}
Keyboard ReadKeyboard(Action action) noexcept {
 Keyboard k{};try{
  if(NativeMcmOwnsRegistry()){
   const auto live=CachedNativeMcmBinding(action);
   if(live.installed)return live;
   // Until native MCM's PauseMenu bridge is live, old disk records can
   // be stale and MUST NOT drive game polling or Prisma Controls.
   // Factory fallback applies only to genuinely uninitialized actions.
   k.installed=Installed();
   std::error_code ec;
   k.registered=std::filesystem::is_regular_file(
       L"Data/MCM/Settings/K2040_Quest_Journal.hotkeys-initialized",ec);
   return k;
  }
  std::lock_guard lock(ioMutex);
  k.installed=Installed();if(!k.installed)return k;
  const auto doc=ReadFile(registry,kMaxRegistry);
  // MCM removes records on ClearKeybind. An initialized but absent action
  // is explicitly Unbound, NOT permission to reactivate Shift+G/G.
  const auto marker=std::filesystem::path(L"Data/MCM/Settings/K2040_Quest_Journal.hotkeys-initialized");
  const bool everInitialized=std::filesystem::is_regular_file(marker);
  if(!doc){k.registered=everInitialized;return k;}
  const auto objects=RegistryObjects(*doc);
  for(const auto& obj:objects){
   const auto binding=Decode(obj);
   if(!binding.valid||binding.id!=ActionId(action))continue;
   k.registered=true;k.keycode=binding.code;k.modifiers=binding.modifiers;
   k.assigned=binding.code!=0 && ValidKey(binding.code,binding.modifiers);
   if(k.assigned)for(const auto& other:objects) {
     // Ignore retired parent-owned copies: a completed migration must
     // not disable the newly registered child binding forever.
     if(StringField(other.text,"modName")==std::optional<std::string_view>{"K2040_Mods"}
        && (StringField(other.text,"id")==std::optional<std::string_view>{"questJournalToggle"}
         || StringField(other.text,"id")==std::optional<std::string_view>{"questTrackerToggle"}))
       continue;
     const auto candidate=Decode(other);
     const auto oKey=NumberField(other.text,"keycode"),oMod=NumberField(other.text,"modifiers");
     if(candidate.valid&&candidate.id!=binding.id&&candidate.code==binding.code&&candidate.modifiers==binding.modifiers)
      k.conflict=true;
     // The native registry may contain other modules with non-journal
     // identities. Detect duplicates across all registered namespaces.
     if(!candidate.valid && oKey && oMod && *oKey==binding.code && *oMod==binding.modifiers)
      k.conflict=true;
   }
   break;
  }
  if(!k.registered && everInitialized)k.registered=true;
 }catch(...){return {};}return k;
}
bool SaveKeyboard(Action action,unsigned key,unsigned modifiers) noexcept {
 if(!ValidKey(key,modifiers) || (!key&&modifiers))return false;
 // Native MCM owns the live registry. Stage a request to its Scaleform
 // bridge; disk writes race its save-on-close and can create duplicate IDs.
 if(NativeMcmOwnsRegistry())
  return StageNativeMcmBinding(action,key,modifiers);
 // Only F4SE Menu Framework's standalone fallback may edit disk.
 SeedDefaultKeyboardBindings();
 try{
  std::lock_guard lock(ioMutex);
  if(!Installed())return false;
  auto doc=ReadFile(registry,kMaxRegistry);if(!doc)return false;
  const auto objects=RegistryObjects(*doc);
  for(const auto& obj:objects){
   const auto target=Decode(obj);
   if(!target.valid||target.id!=ActionId(action))continue;
   if(key)for(const auto& other:objects) {
    if(StringField(other.text,"modName")==std::optional<std::string_view>{"K2040_Mods"}
       && (StringField(other.text,"id")==std::optional<std::string_view>{"questJournalToggle"}
         || StringField(other.text,"id")==std::optional<std::string_view>{"questTrackerToggle"}))
      continue;
    const auto otherKey=NumberField(other.text,"keycode"),otherMods=NumberField(other.text,"modifiers");
    if(otherKey&&otherMods&&*otherKey==key&&*otherMods==modifiers)return false;
   }
   std::string output(obj.text);
   auto keyRange=FieldRange(output,"keycode");
   auto modRange=FieldRange(output,"modifiers");
   if(!keyRange||!modRange)return false;
   // rightmost replacement first to preserve field offsets.
   struct R{std::size_t first,last;unsigned value;};
   std::array<R,2> fields{{{keyRange->first,keyRange->second,key},{modRange->first,modRange->second,modifiers}}};
   if(fields[0].first<fields[1].first)std::swap(fields[0],fields[1]);
   for(const auto& f:fields)output.replace(f.first,f.last-f.first,std::to_string(f.value));
   doc->replace(obj.start,obj.end-obj.start,output);
   return doc->size()<=kMaxRegistry&&WriteAtomic(registry,*doc);
  }
 }catch(...){return false;}return false;
}
Pad ReadPad(Action action) noexcept {
 Pad result{};try{
  std::lock_guard lock(ioMutex);result.installed=Installed();
  if(!result.installed)return result;
  auto text=ReadFile(padUser,kMaxIni).value_or("");
  const auto values=Pair(text,PadPrimaryName(action),PadHeldName(action));
  result.primary=values.primary;result.held=values.held;
  if(result.primary==result.held&&result.primary)result.conflict=true;
  if(result.held&&!result.primary)result.conflict=true;
  if(PadCollision(action,text,values))result.conflict=true;
 }catch(...){return {};}return result;
}
bool SavePad(Action action,unsigned primary,unsigned held) noexcept {
 if(primary>16||held>16||(!primary&&held)||(primary&&primary==held))return false;
 try{
  std::lock_guard lock(ioMutex);if(!Installed())return false;
  auto old=ReadFile(padUser,kMaxIni);if(!old && std::filesystem::exists(padUser))return false;
  const std::string initial=old.value_or("");
  if(PadCollision(action,initial,{primary,held}))return false;
  std::istringstream source(initial);std::string line,out;
  bool section=false,found=false,savedPrimary=false,savedHeld=false;
  const auto p=PadPrimaryName(action),h=PadHeldName(action);
  auto finish=[&](){
   if(!savedPrimary)out+=std::string(p)+"="+std::to_string(primary)+"\r\n";
   if(!savedHeld)out+=std::string(h)+"="+std::to_string(held)+"\r\n";
  };
  while(std::getline(source,line)){
   const auto clean=Trim(line);
   if(clean.starts_with("[")&&clean.ends_with("]")){
    if(section)finish();
    section=clean=="[Controller]";if(section)found=true;
   }
   if(section&&clean.starts_with(std::string(p)+"=")){
    out+=std::string(p)+"="+std::to_string(primary)+"\r\n";savedPrimary=true;
   }else if(section&&clean.starts_with(std::string(h)+"=")){
    out+=std::string(h)+"="+std::to_string(held)+"\r\n";savedHeld=true;
   }else out+=line+"\r\n";
  }
  if(section)finish();
  if(!found){out+="[Controller]\r\n";finish();}
  return out.size()<=kMaxIni&&WriteAtomic(padUser,out);
 }catch(...){return false;}
}
void MigrateLegacyIfNeeded() noexcept {
 try{
  if(!Installed())return;
  // Controller migration only when NO child user INI exists; do not erase
  // explicit Unbound choices or clobber other child field assignments.
  std::error_code ec;
  if(!std::filesystem::exists(padUser,ec) && !ec) {
   const auto old=ReadFile(legacyPadUser,kMaxIni);
   if(old) {
    const auto j=Pair(*old,PadPrimaryName(Action::Journal),PadHeldName(Action::Journal));
    const auto t=Pair(*old,PadPrimaryName(Action::Tracker),PadHeldName(Action::Tracker));
    auto valid=[](ControllerRaw c){return c.primary<=16&&c.held<=16&&
       (!c.held||c.primary)&&(!c.primary||c.primary!=c.held);};
    if(valid(j)&&valid(t)&&!Collision(j,t)&&!PadCollision(Action::Journal,"",j)
       &&!PadCollision(Action::Tracker,"",t)){
     std::string ini="[Controller]\r\n";
     for(auto a:{Action::Journal,Action::Tracker}){
       const auto c=a==Action::Journal?j:t;
       ini+=std::string(PadPrimaryName(a))+"="+std::to_string(c.primary)+"\r\n";
       ini+=std::string(PadHeldName(a))+"="+std::to_string(c.held)+"\r\n";
     }
     WriteAtomic(padUser,ini);
    }
   }
  }
  // Never migrate/write the shared keybind registry behind native MCM.
  // It owns live records and will save them on PauseMenu close.
  if(NativeMcmOwnsRegistry())return;
  // Only framework fallback may migrate unregistered legacy keybinds.
  std::lock_guard guard(ioMutex);
  auto json=ReadFile(registry,kMaxRegistry);if(!json)return;
  bool changed=false;
  for(const auto action:{Action::Journal,Action::Tracker}){
   const auto objects=RegistryObjects(*json);
   std::optional<Object> target,legacy;
   for(const auto& item:objects){
    if(IsOwned(item)&&StringField(item.text,"id")==ActionId(action))target=item;
    if(IsRetiredLegacy(item)&&StringField(item.text,"id")==ActionId(action))legacy=item;
   }
   if(!target||!legacy)continue;
   const auto ownKey=NumberField(target->text,"keycode");
   const auto oldKey=NumberField(legacy->text,"keycode");
   const auto oldMods=NumberField(legacy->text,"modifiers");
   if(!ownKey || *ownKey!=0 || !oldKey || !oldMods || !*oldKey ||
      !ValidKey(*oldKey,*oldMods))continue;
   bool collision=false;
   for(const auto& obj:objects){
    if(obj.start==target->start||IsRetiredLegacy(obj))continue;
    const auto code=NumberField(obj.text,"keycode");
    const auto mods=NumberField(obj.text,"modifiers");
    if(code&&mods&&*code==*oldKey&&*mods==*oldMods){collision=true;break;}
   }
   if(collision)continue;
   std::string replacement(target->text);
   const auto key=FieldRange(replacement,"keycode");
   const auto mod=FieldRange(replacement,"modifiers");
   if(!key||!mod)continue;
   struct R{std::size_t a,b;unsigned v;};
   std::array<R,2> fields{{{key->first,key->second,*oldKey},{mod->first,mod->second,*oldMods}}};
   if(fields[0].a<fields[1].a)std::swap(fields[0],fields[1]);
   for(auto f:fields)replacement.replace(f.a,f.b-f.a,std::to_string(f.v));
   json->replace(target->start,target->end-target->start,replacement);
   changed=true;
  }
  if(changed && json->size()<=kMaxRegistry)WriteAtomic(registry,*json);
 }catch(...){}
}
void SeedDefaultKeyboardBindings() noexcept {
 try {
  // Quick Attach's in-game confirmed r5 fix: NEVER preload/register MCM
  // records on disk before mcm.dll loads. It indexes by physical chord,
  // and multiple synthetic key=0/mod=0 records collide or revert.
  // Native first-registration happens only through live Scaleform.
  if(NativeMcmOwnsRegistry())return;
  std::error_code ec;
  const bool provider=std::filesystem::is_regular_file(
      L"Data/F4SE/Plugins/mcm.dll",ec) ||
      std::filesystem::is_regular_file(
      L"Data/F4SE/Plugins/F4SEMenuFramework.dll",ec);
  if(!provider||!std::filesystem::is_regular_file(config,ec)||ec)return;
  std::lock_guard guard(ioMutex);
  const auto marker=std::filesystem::path(L"Data/MCM/Settings/K2040_Quest_Journal.hotkeys-initialized");
  const bool initialized=std::filesystem::is_regular_file(marker,ec)&&!ec;
  // A later missing entry may have been deliberately cleared; never
  // recreate it, even as a synthetic zero/unbound placeholder.
  if(initialized)return;
  const bool exists=std::filesystem::exists(registry,ec);
  if(ec)return;
  auto data=ReadFile(registry,kMaxRegistry);
  if(exists&&!data)return;
  std::string json=data.value_or("{\"version\":1,\"keybinds\":[]}\n");
  auto opening=json.find("\"keybinds\"");
  if(opening==json.npos){
   // MCM also writes {"version":1} before any bindings exist.
   const auto last=json.find_last_not_of(" \r\t\n");
   if(last==json.npos||json[last]!='}')return;
   json.insert(last,",\"keybinds\":[]");
   opening=json.find("\"keybinds\"");
  }
  auto bracket=json.find('[',opening+10);
  if(bracket==json.npos)return;
  auto closing=bracket+1;
  bool quoted=false,escaped=false;unsigned depth=0;
  for(;closing<json.size();++closing){
   const char c=json[closing];
   if(quoted){if(escaped)escaped=false;else if(c=='\\')escaped=true;
              else if(c=='"')quoted=false;continue;}
   if(c=='"'){quoted=true;continue;}
   if(c=='{'){++depth;continue;}
   if(c=='}'){if(!depth)return;--depth;continue;}
   if(c==']'&&!depth)break;
  }
  if(closing==json.size())return;
  bool changed=false;
  for(const auto action:{Action::Journal,Action::Tracker}){
   const auto objs=RegistryObjects(json);
   bool found=false;
   for(const auto& obj:objs)
    if(IsOwned(obj)&&StringField(obj.text,"id")==ActionId(action)){found=true;break;}
   if(found)continue; // Existing user assignments, even 0, win.
   unsigned key='G';
   unsigned modifiers=action==Action::Journal?1u:0u;
   // Import an experimental parent-owned assignment before writing the
   // new default. Migrating afterward would find a nonzero child default
   // and incorrectly skip the user's custom key.
   if(!initialized)for(const auto& obj:objs) {
    if(IsRetiredLegacy(obj)&&StringField(obj.text,"id")==ActionId(action)){
     const auto candidate=NumberField(obj.text,"keycode");
     const auto chord=NumberField(obj.text,"modifiers");
     if(candidate&&chord&&ValidKey(*candidate,*chord)){
      key=*candidate;modifiers=*chord;
     }
    }
   }
   // A collision with another active record must not replace its assignment.
   // The retired parent record being migrated is not a competing action.
   for(const auto& obj:objs){
    if(IsRetiredLegacy(obj))continue;
    const auto oldKey=NumberField(obj.text,"keycode");
    const auto oldMod=NumberField(obj.text,"modifiers");
    if(key&&oldKey&&oldMod&&*oldKey==key&&*oldMod==modifiers){
     key=0;modifiers=0;break;
    }
   }
   if(!key)continue; // collision: preserve absence, never insert 0/0
   const std::string record=std::string("{\"id\":\"")+std::string(ActionId(action))+
      "\",\"keycode\":"+std::to_string(key)+
      ",\"modName\":\"K2040_Quest_Journal\",\"modifiers\":"+
      std::to_string(modifiers)+"}";
   closing=json.find(']',bracket);
   if(closing==json.npos)return;
   const bool nonempty=json.find_first_not_of(" \r\t\n",bracket+1)<closing;
   json.insert(closing,std::string(nonempty?",":"")+record);
   changed=true;
  }
  // No synthetic unbound migration. Existing user records remain intact.
  if(!initialized){
   std::filesystem::create_directories(marker.parent_path(),ec);
   if(ec)return;
   std::ofstream f(marker,std::ios::binary|std::ios::trunc);
   if(f)f<<"initialized=1\n";
  }
 }catch(...){}
}

std::uint32_t PadMask(unsigned b) noexcept{
 constexpr std::array<std::uint32_t,17> map={
  0,0x1000,0x2000,0x4000,0x8000,0x0001,0x0002,0x0004,0x0008,
  0x0100,0x0200,1u<<16,1u<<17,0x0010,0x0020,0x0040,0x0080};
 return b<map.size()?map[b]:0;
}
std::string KeyboardLabel(const Keyboard& b,std::string_view fallback){
 if(!b.installed)return std::string(fallback)+" (standalone; hub absent)";
 if(b.conflict)return "Shortcut conflict (disabled)";
 if(b.registered&&!b.assigned)return "Unbound";
 if(!b.assigned)return std::string(fallback)+" (standalone fallback)";
 std::string label;
 if(b.modifiers&2)label+="Ctrl+";
 if(b.modifiers&1)label+="Shift+";
 if(b.modifiers&4)label+="Alt+";
 if(b.keycode>=VK_F1&&b.keycode<=VK_F24)label+="F"+std::to_string(b.keycode-VK_F1+1);
 else if((b.keycode>='A'&&b.keycode<='Z')||(b.keycode>='0'&&b.keycode<='9'))
  label+=char(b.keycode);
  else if(b.keycode==258)label+="Mouse 3";
  else if(b.keycode==259)label+="Mouse 4";
  else if(b.keycode==260)label+="Mouse 5";
  else label+="VK "+std::to_string(b.keycode);
 return label;
}
std::string SnapshotJson(){
 const auto a=ReadKeyboard(Action::Journal),b=ReadKeyboard(Action::Tracker);
 const auto c=ReadPad(Action::Journal),d=ReadPad(Action::Tracker);
 const auto render=[&](const Keyboard& k,const Pad& pad,std::string_view fallback){
  return "{\"keycode\":"+std::to_string(k.keycode)+
   ",\"modifiers\":"+std::to_string(k.modifiers)+
   ",\"registered\":"+JsonBool(k.registered)+
   ",\"assigned\":"+JsonBool(k.assigned)+
   ",\"conflict\":"+JsonBool(k.conflict||pad.conflict)+
   ",\"primary\":"+std::to_string(pad.primary)+
   ",\"held\":"+std::to_string(pad.held)+
   ",\"label\":\""+KeyboardLabel(k,fallback)+"\"}";
 };
 return "{\"installed\":"+JsonBool(Installed())+
  ",\"journal\":"+render(a,c,"Shift+G")+
  ",\"tracker\":"+render(b,d,"G")+"}";
}
bool DecodeRequest(std::string_view raw,Action& action,unsigned& first,unsigned& second) noexcept {
 if(raw.size()>180)return false;
 const auto kind=StringField(raw,"action");
 if(!kind)return false;
 if(*kind=="journal")action=Action::Journal;
 else if(*kind=="tracker")action=Action::Tracker;
 else return false;
 const auto key=NumberField(raw,"first"),mods=NumberField(raw,"second");
 if(!key||!mods)return false;first=*key;second=*mods;return true;
}
}
