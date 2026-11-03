#pragma once

// Pure input validation and feature gate for Journal -> Pip-Boy writes.
// This header intentionally has NO Fallout, Papyrus, F4SE or Prisma includes.
// A previous GameVM Quest.SetActive dispatch crashed Fallout 4 in-game.
// Any future backend must be verified independently and approved before
// changing this default-deny policy (see docs/ENGINE_TRACKING_RESEARCH.md).
#include <cstdint>
#include <optional>
#include <string_view>

namespace K2040::QuestTracking {
    enum class WriteBackend : std::uint8_t {
        kNone,
        kVanillaPipboyCommand
    };

    // Never select an unverified backend. This is a review/QA safety gate.
    inline constexpr WriteBackend kVerifiedBackend = WriteBackend::kNone;
    [[nodiscard]] constexpr bool CanWritePipboy() noexcept {
        return kVerifiedBackend != WriteBackend::kNone;
    }

    struct Request {
        std::uint32_t formId;
        bool desiredTracked;
    };

    // Strictly reject malformed and zero IDs, including requests generated
    // by cached JavaScript pages from before the crash-safety hotfix.
    [[nodiscard]] constexpr std::optional<Request> ParseRequest(
        std::string_view input) noexcept {
        if (input.size() != 10 || input[8] != ':' ||
            (input[9] != '0' && input[9] != '1')) return std::nullopt;
        std::uint32_t formId = 0;
        for (std::size_t i=0; i<8; ++i) {
            const char c=input[i];
            const unsigned digit =
                (c>='0' && c<='9') ? unsigned(c-'0') :
                (c>='a' && c<='f') ? unsigned(c-'a')+10 :
                (c>='A' && c<='F') ? unsigned(c-'A')+10 : 0xffu;
            if(digit>0xf) return std::nullopt;
            formId = (formId << 4) | digit;
        }
        if (!formId) return std::nullopt;
        return Request{formId,input[9]=='1'};
    }

    inline constexpr std::string_view kDisabledStatus{"disabled"};

    static_assert(!CanWritePipboy(), "Never enable unverified quest writes");
    static_assert(ParseRequest("00000001:0").has_value());
    static_assert(ParseRequest("ff0080AA:1")->formId == 0xff0080aau);
    static_assert(!ParseRequest("00000000:1").has_value());
    static_assert(!ParseRequest("00000001:9").has_value());
    static_assert(!ParseRequest("not-a-quest").has_value());
}
