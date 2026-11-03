// K2040 Quest Journal — ISOLATED OBSERVATION-ONLY diagnostic module.
//
// This source is NOT part of the shipping Quest Journal target.
// Does not find quest instances, register hooks, call a TESQuest method,
// patch memory, start quests, or mutate any game/save state.
// Requires opt-in isolated-profile testing before installation.
// Read-only game-memory samples are reported to a separate F4SE diagnostics log.
// The probe writes only its OWN log file, never game data or save files.
#include <RE/Fallout.h>
#include <F4SE/F4SE.h>
#include <F4SE/Version.h>
#include <Windows.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace {
    constexpr std::uintptr_t kGetActiveRva = 0x005D6AA0u;
    constexpr std::uintptr_t kSetActiveRva = 0x005D6AC0u;
    constexpr std::size_t kSampleBytes = 128;
    // In the verified OG in-memory prologue, 0x2F is the first call E8.
    // Decode only a relative destination; never execute or hook it.
    constexpr std::size_t kSetActiveFirstCallOffset = 0x2Fu;
    std::atomic_bool emitted{ false };

    bool ReadOnlySample(std::uintptr_t rva, std::array<unsigned char, kSampleBytes>& out) noexcept {
        const HMODULE image = GetModuleHandleW(nullptr);
        if (!image) return false;
        const auto base = reinterpret_cast<std::uintptr_t>(image);
        const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(image);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE ||
            dos->e_lfanew < static_cast<LONG>(sizeof(IMAGE_DOS_HEADER)) ||
            dos->e_lfanew > 0x1000) return false;
        const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE ||
            nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
            nt->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64) return false;

        const auto imageSize = static_cast<std::uintptr_t>(nt->OptionalHeader.SizeOfImage);
        if (rva > imageSize || kSampleBytes > imageSize - rva) return false;

        // Prove target is inside a section marked executable by the PE image.
        bool inExecSection = false;
        const auto* sections = IMAGE_FIRST_SECTION(nt);
        for (std::uint16_t i = 0; i < nt->FileHeader.NumberOfSections && i < 48; ++i) {
            const auto sectionRva = static_cast<std::uintptr_t>(sections[i].VirtualAddress);
            const auto sectionSize = static_cast<std::uintptr_t>(
                sections[i].Misc.VirtualSize > sections[i].SizeOfRawData
                    ? sections[i].Misc.VirtualSize : sections[i].SizeOfRawData);
            if (rva >= sectionRva && rva - sectionRva <= sectionSize &&
                kSampleBytes <= sectionSize - (rva - sectionRva) &&
                (sections[i].Characteristics & IMAGE_SCN_MEM_EXECUTE)) {
                inExecSection = true;
                break;
            }
        }
        if (!inExecSection) return false;

        const auto ptr = base + rva;
        MEMORY_BASIC_INFORMATION memory{};
        if (!VirtualQuery(reinterpret_cast<const void*>(ptr), &memory, sizeof(memory)) ||
            memory.State != MEM_COMMIT ||
            (memory.Protect & (PAGE_GUARD | PAGE_NOACCESS))) return false;
        const auto protect = memory.Protect & 0xffu;
        if (protect != PAGE_EXECUTE && protect != PAGE_EXECUTE_READ &&
            protect != PAGE_EXECUTE_READWRITE && protect != PAGE_EXECUTE_WRITECOPY) return false;
        const auto regionEnd = reinterpret_cast<std::uintptr_t>(memory.BaseAddress) + memory.RegionSize;
        if (ptr > regionEnd || kSampleBytes > regionEnd - ptr) return false;

        // The only memory operation involving the game image is this read.
        std::memcpy(out.data(), reinterpret_cast<const void*>(ptr), out.size());
        return true;
    }

    void Report(const char* label, std::uintptr_t rva) noexcept {
        std::array<unsigned char, kSampleBytes> bytes{};
        if (!ReadOnlySample(rva, bytes)) {
            REX::WARN("K2040_QuestProbe: guarded memory sample unavailable: {}", label);
            return;
        }
        char text[512]{};
        int count = std::snprintf(text, sizeof(text),
                                  "K2040_QuestProbe: %s RVA=0x%08llX runtime-bytes=",
                                  label, static_cast<unsigned long long>(rva));
        if (count < 0 || static_cast<std::size_t>(count) >= sizeof(text)) return;
        for (unsigned char byte : bytes) {
            const int written = std::snprintf(text + count, sizeof(text) - count,
                                              "%02X", static_cast<unsigned>(byte));
            if (written != 2) return;
            count += written;
        }
        if (count + 2 < static_cast<int>(sizeof(text))) {
            text[count++] = '\n';
            text[count] = 0;
        }
        REX::INFO("{}", text);
    }

    void F4SEAPI Observe(F4SE::MessagingInterface::Message* message) noexcept {
        if (!message || message->type != F4SE::MessagingInterface::kGameDataReady ||
            emitted.exchange(true, std::memory_order_acq_rel)) return;
        REX::INFO("K2040_QuestProbe: READ-ONLY byte samples at GameDataReady");
        Report("TESQuest::GetActive", kGetActiveRva);
        Report("TESQuest::SetActive", kSetActiveRva);
        // Observe the direct follow-on call seen in the v2 runtime trace.
        // Fail closed if its opcode changes; the destination is still read-only.
        std::array<unsigned char, kSampleBytes> entry{};
        if (ReadOnlySample(kSetActiveRva, entry) &&
            entry[kSetActiveFirstCallOffset] == 0xE8u) {
            std::int32_t delta{};
            std::memcpy(&delta, entry.data() + kSetActiveFirstCallOffset + 1,
                        sizeof(delta));
            const std::int64_t rva = static_cast<std::int64_t>(kSetActiveRva)
                + static_cast<std::int64_t>(kSetActiveFirstCallOffset) + 5
                + static_cast<std::int64_t>(delta);
            if (rva > 0 && rva <= 0x7FFFFFFFll)
                Report("TESQuest::SetActive follow-on call destination",
                       static_cast<std::uintptr_t>(rva));
            else
                REX::WARN("K2040_QuestProbe: invalid follow-on target");
        } else {
            REX::WARN("K2040_QuestProbe: follow-on call opcode not verified");
        }
        REX::INFO("K2040_QuestProbe: NO quest calls, hooks, patches, or GAME writes");
    }
}

// Legacy OG F4SE 0.6.23 entrypoints. Do NOT compile or deploy as normal mod.
F4SE_EXPORT bool F4SEAPI F4SEPlugin_Query(
    const F4SE::QueryInterface* f4se, F4SE::PluginInfo* info) {
    if (!f4se || !info || f4se->IsEditor() ||
        f4se->RuntimeVersion() != F4SE::RUNTIME_1_10_163) return false;
    info->infoVersion = F4SE::PluginInfo::kVersion;
    info->name = "K2040_Quest_SetActive_Probe";
    info->version = 3;
    return true;
}

F4SE_PLUGIN_LOAD(const F4SE::LoadInterface* load) {
    if (!load || load->RuntimeVersion() != F4SE::RUNTIME_1_10_163) return false;
    F4SE::Init(load, F4SE::InitInfo{
        .log = true,
        .logName = "K2040_Quest_SetActive_Probe",
        .hook = false
    });
    const auto* messages = F4SE::GetMessagingInterface();
    return messages && messages->RegisterListener(Observe);
}
