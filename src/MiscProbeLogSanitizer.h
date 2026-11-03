#pragma once
// Integrated QA ONLY: strip selected quest FormID from read-only snapshot.
#include <optional>
#include <string>
#include <string_view>
namespace K2040::Quests {
    [[nodiscard]] inline std::optional<std::string> AnonymousMiscProbeJson(
        std::string_view raw) {
        constexpr std::string_view prefix=R"({"id":")";
        constexpr std::string_view afterId=R"(","available":)";
        if (raw.size()<prefix.size()+8+afterId.size()+1 ||
            raw.size()>=4096 || !raw.starts_with(prefix) || raw.back()!='}')
            return std::nullopt;
        const auto digits=raw.substr(prefix.size(),8);
        for(const char ch:digits) {
            if (!((ch>='0'&&ch<='9') ||
                  (ch>='a'&&ch<='f') ||
                  (ch>='A'&&ch<='F')))return std::nullopt;
        }
        const auto remainder=raw.substr(prefix.size()+digits.size());
        if (!remainder.starts_with(afterId))return std::nullopt;
        // Drop the terminating quote and comma after the ID, retain JSON.
        std::string sanitized="{";
        sanitized.append(remainder.substr(2));
        return sanitized;
    }
}
