#pragma once
// TEST-ONLY semantic types: not real CommonLib/Fallout ABI definitions.
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <type_traits>
#include <vector>
namespace RE {
struct BSFixedString {
 std::string value;
 BSFixedString()=default;
 BSFixedString(const char* s):value(s?s:""){}
 const char* c_str() const {return value.c_str();}
};
struct BSString {
 std::string value;
 void Set(const char* s,std::uint32_t) {value=s?s:"";}
 const char* c_str()const{return value.c_str();}
};
struct TESQuest;
struct BGSQuestInstanceText{static void ParseString(BSString*,const TESQuest*,std::uint32_t){}};
struct PipboyValue {
 enum SERIALIZATION_DATA_TYPE {kBool=0,kInt32=3,kUint32=4,kString=6,kArray=7,kObject=8};
 virtual ~PipboyValue()=default;
 virtual SERIALIZATION_DATA_TYPE GetType()=0;
 std::uint32_t id{};bool dirty1{},dirty2{};std::uint16_t padding{};PipboyValue* parent{};
};
static_assert(sizeof(PipboyValue)==0x18);
template<class T>struct PipboyPrimitiveValue:PipboyValue {
 T m_value;
 PipboyPrimitiveValue(T value):m_value(value){}
 SERIALIZATION_DATA_TYPE GetType()override{return std::is_same_v<T,bool>?kBool:kUint32;}
 operator T()const{return m_value;}
};
struct PipboyString:PipboyValue {
 BSFixedString value;
 PipboyString(const char* v):value(v){}
 SERIALIZATION_DATA_TYPE GetType()override{return kString;}
};
struct PipboyArray:PipboyValue {
 std::vector<PipboyValue*> elements;
 SERIALIZATION_DATA_TYPE GetType()override{return kArray;}
};
struct PipboyObject:PipboyValue {
 std::unordered_map<std::string,PipboyValue*> members;
 // Emulate public CommonLibF4 PipboyObject::memberMap for read-only
 // member key enumeration in synthetic tests.
 std::unordered_map<std::string,PipboyValue*>& memberMap=members;
 template<class T>T GetMember(const BSFixedString& k) {
  auto i=members.find(k.value);
  return i==members.end()?nullptr:static_cast<T>(i->second);
 }
 SERIALIZATION_DATA_TYPE GetType()override{return kObject;}
};
struct TESForm {
 std::uint32_t formID{};
 [[nodiscard]] std::uint32_t GetFormID()const{return formID;}
 static inline std::unordered_map<std::uint32_t,TESQuest*> forms;
 template<class T>static T* GetFormByID(std::uint32_t id) {
  auto i=forms.find(id);return i==forms.end()?nullptr:static_cast<T*>(i->second);
 }
};
struct TESFullName{static std::string_view GetFullName(const TESQuest&);};
enum class QUEST_OBJECTIVE_STATE:std::int32_t{kDormant=0,kDisplayed=1,kCompleted=2,kCompletedDisplayed=3,kFailed=4,kFailedDisplayed=5};
enum class QuestFlag:std::uint32_t{kCompleted=0x2,kFailed=0x40};
struct BGSQuestObjective{std::uint32_t index{};QUEST_OBJECTIVE_STATE state{};BSFixedString displayText;TESQuest* ownerQuest{};};
struct StubState {
 QUEST_OBJECTIVE_STATE value{QUEST_OBJECTIVE_STATE::kDormant};
 std::int32_t underlying()const{return static_cast<std::int32_t>(value);}
};
struct BGSInstancedQuestObjective {
 BGSQuestObjective* objective{};
 std::uint32_t instanceID{};
 StubState enstanceState{};
};
struct BSSpinLock {
 int held{};
 void lock(){++held;}
 void unlock(){--held;}
};
template<class Mutex> struct BSAutoLock {
 Mutex& lockRef;
 explicit BSAutoLock(Mutex& lock):lockRef(lock){lockRef.lock();}
 ~BSAutoLock(){lockRef.unlock();}
};
template<class Mutex>BSAutoLock(Mutex&)->BSAutoLock<Mutex>;
struct PlayerCharacter {
 BSSpinLock questTargetLock;
 std::vector<BGSInstancedQuestObjective> objectives;
 static inline PlayerCharacter* singleton{};
 static PlayerCharacter* GetSingleton(){return singleton;}
};
struct TESQuest:TESForm{
 std::string displayName;
 struct Data{std::uint16_t flags{};std::int8_t priority{},questType{};} data;
 std::vector<BGSQuestObjective*> objectives;
 std::uint32_t currentInstanceID{};
};
inline std::string_view TESFullName::GetFullName(const TESQuest& q){return q.displayName;}
struct PipboyQuestData{PipboyArray* questArray{};};
struct PipboyDataManager{
 PipboyQuestData questData;
 static inline PipboyDataManager* singleton{};
 static PipboyDataManager* GetSingleton(){return singleton;}
};
}
