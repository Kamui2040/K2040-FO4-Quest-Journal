#pragma once
// Isolated OG163 one-shot UI-thread native-code READ ONLY prefix observer.
// No hooks, dispatch, function invocation, executable writes or scanning.
// Captured proprietary bytes remain solely in a local PRIVATE F4SE QA log.
#if !defined(K2040_PIPBOY_SELECTION_READONLY_QA) || \
    !defined(K2040_PIPBOY_SELECTION_SCHEMA_READONLY_QA)
#error Requires isolated Pipboy selected-entry + schema read-only QA
#endif
#include "PipboyDispatchPrefixPolicy.h"
#include <Windows.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

namespace K2040::PipboyDispatchResearch {
inline constexpr std::size_t kWindowsPerMethod=4u; // 768 bytes per method
static_assert(kWindowsPerMethod*kPrefixBytes<=1024u);
struct Prefix {
    std::uintptr_t rva{};
    std::array<std::string,kWindowsPerMethod> windows{};
};
enum class Status : std::uint8_t {
    kModuleUnavailable,kDOSUnavailable,kInvalidDOS,kNTUnavailable,
    kInvalidPE,kVtableUnreadable,kVtableMismatch,
    kCodeUnreadable,kCaptured
};
[[nodiscard]] constexpr const char* StatusName(Status s) noexcept {
    switch(s) {
    case Status::kModuleUnavailable:return "module-unavailable";
    case Status::kDOSUnavailable:return "dos-unreadable";
    case Status::kInvalidDOS:return "dos-invalid";
    case Status::kNTUnavailable:return "nt-unreadable";
    case Status::kInvalidPE:return "pe-mismatch";
    case Status::kVtableUnreadable:return "vtable-unreadable";
    case Status::kVtableMismatch:return "vtable-mismatch";
    case Status::kCodeUnreadable:return "code-unreadable";
    case Status::kCaptured:return "captured";
    }
    return "unknown";
}
struct Capture {
    Status status{Status::kModuleUnavailable};
    Prefix call{},map{};
    bool callbackAbiVerified{false};
    bool nativeWriteAuthorized{false};
};
[[nodiscard]] inline bool Readable(const std::uintptr_t address,
                                   const std::size_t size,bool code) {
    if(!address || !size || size>256 || address+size<address)return false;
    MEMORY_BASIC_INFORMATION page{};
    if(!VirtualQuery(reinterpret_cast<const void*>(address),&page,
                     sizeof(page)) || page.State!=MEM_COMMIT ||
       (page.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return false;
    const auto start=reinterpret_cast<std::uintptr_t>(page.BaseAddress);
    if(start>address || page.RegionSize>UINTPTR_MAX-start ||
       address+size>start+page.RegionSize)return false;
    const auto protect=page.Protect&0xFFu;
    if(code)return protect==PAGE_EXECUTE_READ ||
                   protect==PAGE_EXECUTE_READWRITE ||
                   protect==PAGE_EXECUTE_WRITECOPY;
    return protect==PAGE_READONLY || protect==PAGE_READWRITE ||
           protect==PAGE_WRITECOPY || protect==PAGE_EXECUTE_READ ||
           protect==PAGE_EXECUTE_READWRITE ||
           protect==PAGE_EXECUTE_WRITECOPY;
}
[[nodiscard]] inline bool ReadPrefixes(std::uintptr_t base,Prefix& target) {
    constexpr char hex[]="0123456789abcdef";
    for(std::size_t window=0;window<kWindowsPerMethod;++window) {
        const auto rva=target.rva+window*kPrefixBytes;
        if(!ValidCodeRva(rva,kPrefixBytes))return false;
        if(base+rva<base || !Readable(base+rva,kPrefixBytes,true))return false;
        std::array<std::uint8_t,kPrefixBytes> bytes{};
        std::memcpy(bytes.data(),reinterpret_cast<const void*>(base+rva),
                    bytes.size());
        auto& output=target.windows[window];
        output.reserve(bytes.size()*2u);
        for(const auto byte:bytes) {
            output.push_back(hex[byte>>4u]);
            output.push_back(hex[byte&0x0fu]);
        }
    }
    return true;
}

[[nodiscard]] inline Capture CaptureOriginalOG163Prefixes() {
    Capture out{};
    const auto base=reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    if(!base)return out;
    if(!Readable(base,sizeof(IMAGE_DOS_HEADER),false)) {
        out.status=Status::kDOSUnavailable;return out;
    }
    IMAGE_DOS_HEADER dos{};
    std::memcpy(&dos,reinterpret_cast<const void*>(base),sizeof(dos));
    if(dos.e_magic!=IMAGE_DOS_SIGNATURE ||
       dos.e_lfanew<64 || dos.e_lfanew>2048) {
        out.status=Status::kInvalidDOS;return out;
    }
    constexpr auto ntSize=offsetof(IMAGE_NT_HEADERS64,OptionalHeader)+
        offsetof(IMAGE_OPTIONAL_HEADER64,SizeOfImage)+sizeof(DWORD);
    static_assert(ntSize==84u && ntSize<=256u);
    const auto ntAddress=base+static_cast<std::uintptr_t>(dos.e_lfanew);
    if(!Readable(ntAddress,ntSize,false)) {
        out.status=Status::kNTUnavailable;return out;
    }
    IMAGE_NT_HEADERS64 nt{};
    std::memcpy(&nt,reinterpret_cast<const void*>(ntAddress),ntSize);
    if(nt.Signature!=IMAGE_NT_SIGNATURE ||
       nt.FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64 ||
       nt.OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
       nt.OptionalHeader.ImageBase!=kImageBaseHeader ||
       nt.OptionalHeader.SizeOfImage!=kImageSize) {
        out.status=Status::kInvalidPE;return out;
    }
    const auto vtable=base+kPipboyPrimaryVtableRva;
    std::array<std::uintptr_t,kSlotCount> slots{};
    if(!Readable(vtable,sizeof(slots),false)) {
        out.status=Status::kVtableUnreadable;return out;
    }
    std::memcpy(slots.data(),reinterpret_cast<const void*>(vtable),
                sizeof(slots));
    if(slots[1]<base || slots[2]<base ||
       !ValidSlotRvas(slots[1]-base,slots[2]-base)) {
        out.status=Status::kVtableMismatch;return out;
    }
    out.call.rva=slots[1]-base;
    out.map.rva=slots[2]-base;
    if(!ReadPrefixes(base,out.call) || !ReadPrefixes(base,out.map)) {
        out.status=Status::kCodeUnreadable;
        out.call={};out.map={};return out;
    }
    out.status=Status::kCaptured;
    return out;
}
} // namespace K2040::PipboyDispatchResearch
