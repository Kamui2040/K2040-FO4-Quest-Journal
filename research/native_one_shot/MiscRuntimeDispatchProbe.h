#pragma once
// Bounded READ-ONLY OG163 runtime verification, shared with the opt-in
// Dogmeat-only candidate. Inspect the OG game executable after
// SteamStub initializes it. No writes, hooks, debugger, global
// memory scans or other game-function calls.
#if !defined(K2040_TRACKING_INTEGRATED_QA) && \
    !defined(K2040_DOGMEAT_PRODUCTION_CANDIDATE) && \
    !defined(K2040_SELECTOR_READONLY_QA)
#error This runtime probe requires an explicit isolated writer build
#endif
#include <Windows.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <utility>

namespace K2040::MiscVanillaResearch {
    inline constexpr std::uintptr_t kOGToggleCommandVtableRva=0x02D12B80u;
    inline constexpr std::uintptr_t kOGImageSize=0x06CE6000u;
    inline constexpr std::uintptr_t kOGCodeStart=0x00001000u;
    inline constexpr std::uintptr_t kOGCodeEnd=0x02C0C000u;
    inline constexpr std::size_t kHeaderLength=80u;
    inline constexpr std::size_t kSlots=4u;
    // OG-only internal target from the verified direct call in vtable slot 3.
    // This is a READ target, never a callable interface or write permission.
    inline constexpr std::uintptr_t kOGLinkedHelperRva=0x00C4B840u;
    inline constexpr std::size_t kLinkedHelperLength=160u;
    inline constexpr std::size_t kDirectCallOffset=0x13u;
    static_assert(kLinkedHelperLength<=256u);
    static_assert(kDirectCallOffset+5u<=kHeaderLength);
    // The PE fields we validate end at OptionalHeader.SizeOfImage.
    // IMAGE_NT_HEADERS64 is 264 bytes and exceeds Readable's 256-byte cap.
    // Read only the checked prefix instead of widening that cap.
    inline constexpr std::size_t kNTHeaderPrefixLength=
        offsetof(IMAGE_NT_HEADERS64,OptionalHeader)+
        offsetof(IMAGE_OPTIONAL_HEADER64,SizeOfImage)+sizeof(DWORD);
    static_assert(kNTHeaderPrefixLength==84u);
    static_assert(kNTHeaderPrefixLength<=256u);
    static_assert(kNTHeaderPrefixLength<=sizeof(IMAGE_NT_HEADERS64));

    // Selector-only diagnostic: the four known vtable slots are already
    // validated by CaptureRuntimeCode(). Re-read 192-byte bounded prefixes,
    // never scan for unknown functions or expand the Readable 256B cap.
    inline constexpr std::size_t kSelectorReadLength=192u;
    static_assert(kSelectorReadLength<=256u);
    enum class ReadFault : std::uint8_t {
        kNone, kInvalidRange, kQueryFailure, kNotCommitted,
        kGuarded, kRegionBoundary, kProtection
    };
    [[nodiscard]] inline bool Readable(const std::uintptr_t address,
                                        const std::size_t bytes,
                                        const bool executable,
                                        ReadFault* fault=nullptr) {
        if(fault)*fault=ReadFault::kNone;
        const auto reject=[fault](const ReadFault reason) {
            if(fault)*fault=reason;
            return false;
        };
        if(!address || !bytes || bytes>256u || address+bytes<address)
            return reject(ReadFault::kInvalidRange);
        MEMORY_BASIC_INFORMATION info{};
        if(!VirtualQuery(reinterpret_cast<const void*>(address),
                         &info,sizeof(info)))
            return reject(ReadFault::kQueryFailure);
        if(info.State!=MEM_COMMIT)return reject(ReadFault::kNotCommitted);
        if(info.Protect & (PAGE_GUARD|PAGE_NOACCESS))
            return reject(ReadFault::kGuarded);
        const auto start=reinterpret_cast<std::uintptr_t>(info.BaseAddress);
        const auto end=start+info.RegionSize;
        if(end<start || address<start || address+bytes>end)
            return reject(ReadFault::kRegionBoundary);
        const auto prot=info.Protect & 0xFFu;
        if(executable) {
            // Never treat PAGE_EXECUTE as safe for a data read.
            if(prot==PAGE_EXECUTE_READ ||
               prot==PAGE_EXECUTE_READWRITE ||
               prot==PAGE_EXECUTE_WRITECOPY)return true;
            return reject(ReadFault::kProtection);
        }
        if(prot==PAGE_READONLY || prot==PAGE_READWRITE ||
           prot==PAGE_WRITECOPY || prot==PAGE_EXECUTE_READ ||
           prot==PAGE_EXECUTE_READWRITE || prot==PAGE_EXECUTE_WRITECOPY)
            return true;
        return reject(ReadFault::kProtection);
    }

    enum class FailurePhase : std::uint8_t {
        kNone, kModule, kDOSMemory, kDOSHeader, kNTMemory,
        kNTHeader, kVtableMemory, kSlotBounds, kSlotMemory
    };
    struct CaptureDiagnostics {
        FailurePhase phase{FailurePhase::kNone};
        ReadFault fault{ReadFault::kNone};
        int slot{-1};
    };
    [[nodiscard]] inline const char* PhaseName(const FailurePhase phase) {
        switch(phase) {
        case FailurePhase::kNone:return "none";
        case FailurePhase::kModule:return "module";
        case FailurePhase::kDOSMemory:return "dos-memory";
        case FailurePhase::kDOSHeader:return "dos-header";
        case FailurePhase::kNTMemory:return "nt-memory";
        case FailurePhase::kNTHeader:return "nt-header";
        case FailurePhase::kVtableMemory:return "vtable-memory";
        case FailurePhase::kSlotBounds:return "slot-bounds";
        case FailurePhase::kSlotMemory:return "slot-memory";
        }
        return "unknown";
    }
    [[nodiscard]] inline const char* FaultName(const ReadFault fault) {
        switch(fault) {
        case ReadFault::kNone:return "none";
        case ReadFault::kInvalidRange:return "invalid-range";
        case ReadFault::kQueryFailure:return "query-failed";
        case ReadFault::kNotCommitted:return "not-committed";
        case ReadFault::kGuarded:return "guard-or-noaccess";
        case ReadFault::kRegionBoundary:return "region-boundary";
        case ReadFault::kProtection:return "protection-mismatch";
        }
        return "unknown";
    }

    struct Entry {
        std::uintptr_t rva{};
        std::string prefixHex;
    };
    enum class LinkedHelperStatus : std::uint8_t {
        kNotMatched, kOutOfBounds, kUnreadable, kCaptured
    };
    [[nodiscard]] inline const char* HelperStatusName(const LinkedHelperStatus status) {
        switch(status) {
        case LinkedHelperStatus::kNotMatched:return "call-target-mismatch";
        case LinkedHelperStatus::kOutOfBounds:return "target-out-of-bounds";
        case LinkedHelperStatus::kUnreadable:return "target-unreadable";
        case LinkedHelperStatus::kCaptured:return "captured";
        }
        return "unknown";
    }
    struct Snapshot {
        std::array<Entry,kSlots> entries{};
        std::optional<Entry> linkedHelper{};
        LinkedHelperStatus helperStatus{LinkedHelperStatus::kNotMatched};
        ReadFault helperFault{ReadFault::kNone};
    };

    [[nodiscard]] inline std::optional<Snapshot> CaptureRuntimeCode(
            CaptureDiagnostics* diagnostic=nullptr) {
        if(diagnostic)*diagnostic={};
        const auto reject=[diagnostic](const FailurePhase phase,
                                       const ReadFault fault=ReadFault::kNone,
                                       const int slot=-1)->std::optional<Snapshot> {
            if(diagnostic)*diagnostic={phase,fault,slot};
            return std::nullopt;
        };
        const auto base=reinterpret_cast<std::uintptr_t>(
            GetModuleHandleW(nullptr));
        if(!base)return reject(FailurePhase::kModule);
        ReadFault fault{};
        if(!Readable(base,sizeof(IMAGE_DOS_HEADER),false,&fault))
            return reject(FailurePhase::kDOSMemory,fault);
        IMAGE_DOS_HEADER dos{};
        std::memcpy(&dos,reinterpret_cast<const void*>(base),sizeof(dos));
        if(dos.e_magic!=IMAGE_DOS_SIGNATURE ||
           dos.e_lfanew<64 || dos.e_lfanew>2048)
            return reject(FailurePhase::kDOSHeader);
        const auto ntAddress=base+static_cast<std::uintptr_t>(dos.e_lfanew);
        if(!Readable(ntAddress,kNTHeaderPrefixLength,false,&fault))
            return reject(FailurePhase::kNTMemory,fault);
        IMAGE_NT_HEADERS64 nt{};
        std::memcpy(&nt,reinterpret_cast<const void*>(ntAddress),
                    kNTHeaderPrefixLength);
        if(nt.Signature!=IMAGE_NT_SIGNATURE ||
           nt.OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
           nt.OptionalHeader.SizeOfImage!=kOGImageSize ||
           nt.OptionalHeader.ImageBase!=0x140000000ull)
            return reject(FailurePhase::kNTHeader);
        const auto table=base+kOGToggleCommandVtableRva;
        if(!Readable(table,kSlots*sizeof(std::uintptr_t),false,&fault))
            return reject(FailurePhase::kVtableMemory,fault);
        std::array<std::uintptr_t,kSlots> entries{};
        std::memcpy(entries.data(),reinterpret_cast<const void*>(table),
                    sizeof(entries));
        Snapshot result;
        std::array<std::uint8_t,kHeaderLength> dispatchCode{};
        constexpr char hex[]="0123456789abcdef";
        for(std::size_t index=0;index<kSlots;++index) {
            const auto absolute=entries[index];
            if(absolute<base+kOGCodeStart ||
               absolute+kHeaderLength<absolute ||
               absolute+kHeaderLength>base+kOGCodeEnd)
                return reject(FailurePhase::kSlotBounds,
                              ReadFault::kNone,static_cast<int>(index));
            if(!Readable(absolute,kHeaderLength,true,&fault))
                return reject(FailurePhase::kSlotMemory,fault,
                              static_cast<int>(index));
            std::array<std::uint8_t,kHeaderLength> bytes{};
            std::memcpy(bytes.data(),reinterpret_cast<const void*>(absolute),
                        bytes.size());
            if(index==kSlots-1u)dispatchCode=bytes;
            auto& entry=result.entries[index];
            entry.rva=absolute-base;
            entry.prefixHex.reserve(bytes.size()*2);
            for(const auto byte:bytes) {
                entry.prefixHex.push_back(hex[byte>>4]);
                entry.prefixHex.push_back(hex[byte&0xf]);
            }
        }
        // The direct x64 call at the verified dispatch slot is the sole
        // authority for this additional research read; never invoke it.
        if(dispatchCode[kDirectCallOffset]!=0xE8u)return result;
        std::int32_t displacement{};
        std::memcpy(&displacement,
                    dispatchCode.data()+kDirectCallOffset+1u,
                    sizeof(displacement));
        const auto originRva=static_cast<std::int64_t>(entries[kSlots-1u]-base);
        const auto targetRva=originRva+
            static_cast<std::int64_t>(kDirectCallOffset+5u)+displacement;
        if(targetRva!=static_cast<std::int64_t>(kOGLinkedHelperRva))
            return result;
        const auto helperAddress=base+kOGLinkedHelperRva;
        if(helperAddress<base+kOGCodeStart ||
           helperAddress+kLinkedHelperLength<helperAddress ||
           helperAddress+kLinkedHelperLength>base+kOGCodeEnd) {
            result.helperStatus=LinkedHelperStatus::kOutOfBounds;
            return result;
        }
        if(!Readable(helperAddress,kLinkedHelperLength,true,
                     &result.helperFault)) {
            result.helperStatus=LinkedHelperStatus::kUnreadable;
            return result;
        }
        std::array<std::uint8_t,kLinkedHelperLength> helperBytes{};
        std::memcpy(helperBytes.data(),
                    reinterpret_cast<const void*>(helperAddress),
                    helperBytes.size());
        Entry helper{};
        helper.rva=kOGLinkedHelperRva;
        helper.prefixHex.reserve(helperBytes.size()*2u);
        for(const auto byte:helperBytes) {
            helper.prefixHex.push_back(hex[byte>>4]);
            helper.prefixHex.push_back(hex[byte&0xf]);
        }
        result.linkedHelper=std::move(helper);
        result.helperStatus=LinkedHelperStatus::kCaptured;
        return result;
    }
#ifdef K2040_SELECTOR_READONLY_QA
    // No native calls. Fail closed when the in-memory code differs from the
    // previously captured 80-byte baseline, including after a runtime change.
    [[nodiscard]] inline std::optional<std::array<Entry,kSlots>>
    CaptureSelectorPrefixes(const Snapshot& checked,
                            ReadFault* fault=nullptr) {
        if(fault)*fault=ReadFault::kNone;
        const auto base=reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
        if(!base || checked.helperStatus!=LinkedHelperStatus::kCaptured ||
           !checked.linkedHelper || checked.linkedHelper->rva!=kOGLinkedHelperRva)
            return std::nullopt;
        std::array<Entry,kSlots> longer{};
        constexpr char hex[]="0123456789abcdef";
        for(std::size_t i=0;i<kSlots;++i) {
            const auto& original=checked.entries[i];
            if(original.prefixHex.size()!=kHeaderLength*2 ||
               original.rva<kOGCodeStart ||
               original.rva+kSelectorReadLength<original.rva ||
               original.rva+kSelectorReadLength>kOGCodeEnd)
                return std::nullopt;
            const auto address=base+original.rva;
            std::array<std::uint8_t,kSelectorReadLength> bytes{};
            if(!Readable(address,bytes.size(),true,fault))
                return std::nullopt;
            std::memcpy(bytes.data(),reinterpret_cast<const void*>(address),
                        bytes.size());
            auto& entry=longer[i];
            entry.rva=original.rva;
            entry.prefixHex.reserve(bytes.size()*2);
            for(auto b:bytes) {
                entry.prefixHex.push_back(hex[b>>4]);
                entry.prefixHex.push_back(hex[b&0xf]);
            }
            if(entry.prefixHex.compare(0,original.prefixHex.size(),
                                      original.prefixHex)!=0)
                return std::nullopt;
        }
        return longer;
    }
#endif
}
