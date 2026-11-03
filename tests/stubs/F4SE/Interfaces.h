#pragma once
#include <cstdint>
#include <optional>
#define F4SEAPI
namespace F4SE {
 class SerializationInterface {
 public:
  using EventCallback=void(const SerializationInterface*);
  virtual ~SerializationInterface()=default;
  virtual void SetUniqueID(std::uint32_t) const=0;
  virtual void SetSaveCallback(EventCallback*) const=0;
  virtual void SetLoadCallback(EventCallback*) const=0;
  virtual void SetRevertCallback(EventCallback*) const=0;
  virtual bool OpenRecord(std::uint32_t,std::uint32_t) const=0;
  virtual bool WriteRecordData(const void*,std::uint32_t) const=0;
  template<class T> bool WriteRecordData(const T& v) const {return WriteRecordData(&v,sizeof(v));}
  virtual bool GetNextRecordInfo(std::uint32_t&,std::uint32_t&,std::uint32_t&) const=0;
  virtual std::uint32_t ReadRecordData(void*,std::uint32_t) const=0;
  template<class T> std::uint32_t ReadRecordData(T& v) const {return ReadRecordData(&v,sizeof(v));}
  virtual std::optional<std::uint32_t> ResolveFormID(std::uint32_t) const=0;
 };
 const SerializationInterface* GetSerializationInterface() noexcept;
}
