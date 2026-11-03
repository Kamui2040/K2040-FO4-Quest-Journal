// Synthetic F4SE serialization test; never claim this proves Fallout runtime compatibility.
#include "QuestViewState.h"
#include <F4SE/Interfaces.h>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <vector>
struct Record {std::uint32_t type{},version{};std::vector<std::uint8_t> bytes;};
class FakeArchive final:public F4SE::SerializationInterface {
public:
 mutable std::uint32_t identity{};
 mutable EventCallback* save{};
 mutable EventCallback* load{};
 mutable EventCallback* revert{};
 mutable std::vector<Record> records;
 mutable std::size_t cursor{},position{};
 mutable bool corruptRemap{};
 void SetUniqueID(std::uint32_t id) const override {identity=id;}
 void SetSaveCallback(EventCallback* cb) const override {save=cb;}
 void SetLoadCallback(EventCallback* cb) const override {load=cb;}
 void SetRevertCallback(EventCallback* cb) const override {revert=cb;}
 bool OpenRecord(std::uint32_t type,std::uint32_t version) const override {records.push_back({type,version,{}});return true;}
 bool WriteRecordData(const void* bytes,std::uint32_t size) const override {
  if(records.empty())return false;
  const auto* begin=static_cast<const std::uint8_t*>(bytes);
  records.back().bytes.insert(records.back().bytes.end(),begin,begin+size);return true;
 }
 bool GetNextRecordInfo(std::uint32_t& type,std::uint32_t& version,std::uint32_t& len) const override {
  if(cursor>=records.size())return false;
  const auto& r=records[cursor++];type=r.type;version=r.version;
  len=static_cast<std::uint32_t>(r.bytes.size());position=0;return true;
 }
 std::uint32_t ReadRecordData(void* dst,std::uint32_t len) const override {
  const auto& r=records[cursor-1];if(position+len>r.bytes.size())return 0;
  std::memcpy(dst,r.bytes.data()+position,len);position+=len;return len;
 }
 std::optional<std::uint32_t> ResolveFormID(std::uint32_t id) const override {
  if(corruptRemap && id==102)return std::nullopt;
  return id+1000;
 }
};
namespace F4SE {const SerializationInterface* g_archive{};const SerializationInterface* GetSerializationInterface() noexcept{return g_archive;}}
int main() {
 FakeArchive a;F4SE::g_archive=&a;
 using namespace K2040::QuestViewState;
 RegisterSerialization();assert(a.identity && a.save && a.load && a.revert);
 assert(Toggle(Kind::Pin,101));assert(Toggle(Kind::Favorite,102));assert(Toggle(Kind::Hidden,103));
 assert(Get(Kind::Pin).contains(101) && !Toggle(Kind::Pin,0));
 assert(Toggle(Kind::Pin,101) && !Get(Kind::Pin).contains(101));
 assert(Toggle(Kind::Pin,101) && Get(Kind::Pin).contains(101));
 assert(SetCategory(101,"Modded") && CategoryFor(101)=="Modded");
 assert(SetCategory(102,"Far Harbor") && CategoryFor(102)=="Far Harbor");
 assert(!SetCategory(103,"<script>") && !SetCategory(0,"Other"));
 assert(SetCategory(103,"Side"));
 assert(Remember(101) && Remember(102) && Remember(103) && !Remember(0));
 a.save(&a);assert(a.records.size()==3 && a.records[0].bytes.size()==24 && a.records[1].bytes.size()==28 && a.records[2].bytes.size()==16);
 Clear();assert(Get(Kind::Pin).empty());a.cursor=0;a.corruptRemap=true;a.load(&a);
 assert(Get(Kind::Pin).contains(1101) && Get(Kind::Favorite).empty());
 assert(Get(Kind::Hidden).contains(1103));
 assert(Observed().contains(1101) && !Observed().contains(1102) && Observed().contains(1103));
 assert(CategoryFor(1101)=="Modded" && CategoryFor(1102).empty() && CategoryFor(1103)=="Side");
 assert(SetCategory(1101,"Automatic") && CategoryFor(1101).empty());
 a.revert(&a);assert(Get(Kind::Pin).empty() && Get(Kind::Hidden).empty() && CategoryFor(1103).empty() && Observed().empty());
 a.records[0].bytes[0]=0xFF;a.cursor=0;a.load(&a);
 assert(Get(Kind::Pin).empty());
 a.records[1].bytes[0]=0xFF;a.cursor=0;a.load(&a);
 assert(CategoryFor(1101).empty() && CategoryFor(1103).empty());
 a.records[2].bytes[0]=0xFF;a.cursor=0;a.load(&a);
 assert(Observed().empty());
}