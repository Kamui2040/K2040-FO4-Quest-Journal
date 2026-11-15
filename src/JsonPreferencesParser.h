#pragma once

#include <charconv>
#include <cmath>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace k2040::json
{
    struct JsonValue;
    using JsonObject = std::map<std::string, JsonValue>;
    using JsonArray = std::vector<JsonValue>;

    struct JsonValue
    {
        using Value = std::variant<std::nullptr_t, bool, double, std::string, JsonArray, JsonObject>;
        Value value = nullptr;

        const JsonObject* AsObject() const { return std::get_if<JsonObject>(&value); }
        const JsonArray* AsArray() const { return std::get_if<JsonArray>(&value); }
        const std::string* AsString() const { return std::get_if<std::string>(&value); }
        const bool* AsBool() const { return std::get_if<bool>(&value); }
        const double* AsNumber() const { return std::get_if<double>(&value); }
    };

    // Files are size-limited by their callers. This independent depth limit
    // keeps nested but small inputs from exhausting the thread stack.
    class JsonParser
    {
    public:
        explicit JsonParser(std::string text) : text_(std::move(text)) {}

        [[nodiscard]] std::optional<JsonValue> Parse()
        {
            auto value = ParseValue(0);
            if (!value) return std::nullopt;
            SkipWhitespace();
            return pos_ == text_.size() ? std::move(value) : std::nullopt;
        }

    private:
        static constexpr std::size_t kMaxDepth = 64;

        std::optional<JsonValue> ParseValue(std::size_t depth)
        {
            if (depth > kMaxDepth) return std::nullopt;
            SkipWhitespace();
            if (pos_ >= text_.size()) return std::nullopt;
            const char c = text_[pos_];
            if (c == '{') return ParseObject(depth);
            if (c == '[') return ParseArray(depth);
            if (c == '"') {
                auto string = ParseString();
                if (!string) return std::nullopt;
                JsonValue value;
                value.value = std::move(*string);
                return value;
            }
            if (c == 't' && ConsumeLiteral("true")) {
                JsonValue value;
                value.value = true;
                return value;
            }
            if (c == 'f' && ConsumeLiteral("false")) {
                JsonValue value;
                value.value = false;
                return value;
            }
            if (c == 'n' && ConsumeLiteral("null")) {
                JsonValue value;
                value.value = nullptr;
                return value;
            }
            if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) return ParseNumber();
            return std::nullopt;
        }

        std::optional<JsonValue> ParseObject(std::size_t depth)
        {
            if (!Consume('{')) return std::nullopt;
            JsonObject object;
            SkipWhitespace();
            if (Consume('}')) {
                JsonValue value;
                value.value = std::move(object);
                return value;
            }
            while (true) {
                auto key = ParseString();
                if (!key || !Consume(':')) return std::nullopt;
                auto value = ParseValue(depth + 1);
                if (!value || object.contains(*key)) return std::nullopt;
                object.emplace(std::move(*key), std::move(*value));
                SkipWhitespace();
                if (Consume('}')) break;
                if (!Consume(',')) return std::nullopt;
            }
            JsonValue value;
            value.value = std::move(object);
            return value;
        }

        std::optional<JsonValue> ParseArray(std::size_t depth)
        {
            if (!Consume('[')) return std::nullopt;
            JsonArray array;
            SkipWhitespace();
            if (Consume(']')) {
                JsonValue value;
                value.value = std::move(array);
                return value;
            }
            while (true) {
                auto value = ParseValue(depth + 1);
                if (!value) return std::nullopt;
                array.push_back(std::move(*value));
                SkipWhitespace();
                if (Consume(']')) break;
                if (!Consume(',')) return std::nullopt;
            }
            JsonValue value;
            value.value = std::move(array);
            return value;
        }

        std::optional<JsonValue> ParseNumber()
        {
            const auto start = pos_;
            if (text_[pos_] == '-') ++pos_;
            if (pos_ == text_.size()) return std::nullopt;
            if (text_[pos_] == '0') {
                ++pos_;
            } else {
                if (text_[pos_] < '1' || text_[pos_] > '9') return std::nullopt;
                while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_]))) ++pos_;
            }
            if (pos_ < text_.size() && text_[pos_] == '.') {
                ++pos_;
                const auto fraction = pos_;
                while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_]))) ++pos_;
                if (pos_ == fraction) return std::nullopt;
            }
            if (pos_ < text_.size() && (text_[pos_] == 'e' || text_[pos_] == 'E')) {
                ++pos_;
                if (pos_ < text_.size() && (text_[pos_] == '+' || text_[pos_] == '-')) ++pos_;
                const auto exponent = pos_;
                while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_]))) ++pos_;
                if (pos_ == exponent) return std::nullopt;
            }
            double number = 0.0;
            const auto* first = text_.data() + start;
            const auto* last = text_.data() + pos_;
            const auto parsed = std::from_chars(first, last, number, std::chars_format::general);
            if (parsed.ec != std::errc{} || parsed.ptr != last || !std::isfinite(number)) return std::nullopt;
            JsonValue value;
            value.value = number;
            return value;
        }

        bool ReadUnicodeCodeUnit(std::uint32_t& result)
        {
            if (text_.size() - pos_ < 4) return false;
            result = 0;
            for (unsigned index = 0; index < 4; ++index) {
                const char c = text_[pos_++];
                std::uint32_t digit = 0;
                if (c >= '0' && c <= '9') digit = static_cast<std::uint32_t>(c - '0');
                else if (c >= 'a' && c <= 'f') digit = static_cast<std::uint32_t>(c - 'a') + 10;
                else if (c >= 'A' && c <= 'F') digit = static_cast<std::uint32_t>(c - 'A') + 10;
                else return false;
                result = (result << 4) | digit;
            }
            return true;
        }

        static void AppendUtf8(std::string& text, std::uint32_t codepoint)
        {
            if (codepoint <= 0x7F) text.push_back(static_cast<char>(codepoint));
            else if (codepoint <= 0x7FF) {
                text.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
                text.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
            } else if (codepoint <= 0xFFFF) {
                text.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
                text.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
                text.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
            } else {
                text.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
                text.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
                text.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
                text.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
            }
        }

        std::optional<std::string> ParseString()
        {
            if (!Consume('"')) return std::nullopt;
            std::string result;
            while (pos_ < text_.size()) {
                const char c = text_[pos_++];
                if (c == '"') return result;
                if (static_cast<unsigned char>(c) < 0x20) return std::nullopt;
                if (c != '\\') {
                    result.push_back(c);
                    continue;
                }
                if (pos_ >= text_.size()) return std::nullopt;
                switch (text_[pos_++]) {
                case '"': result.push_back('"'); break;
                case '\\': result.push_back('\\'); break;
                case '/': result.push_back('/'); break;
                case 'b': result.push_back('\b'); break;
                case 'f': result.push_back('\f'); break;
                case 'n': result.push_back('\n'); break;
                case 'r': result.push_back('\r'); break;
                case 't': result.push_back('\t'); break;
                case 'u': {
                    std::uint32_t codepoint = 0;
                    if (!ReadUnicodeCodeUnit(codepoint)) return std::nullopt;
                    if (codepoint >= 0xD800 && codepoint <= 0xDBFF) {
                        if (text_.size() - pos_ < 6 || text_[pos_] != '\\' || text_[pos_ + 1] != 'u')
                            return std::nullopt;
                        pos_ += 2;
                        std::uint32_t low = 0;
                        if (!ReadUnicodeCodeUnit(low) || low < 0xDC00 || low > 0xDFFF)
                            return std::nullopt;
                        codepoint = 0x10000 + ((codepoint - 0xD800) << 10) + (low - 0xDC00);
                    } else if (codepoint >= 0xDC00 && codepoint <= 0xDFFF) {
                        return std::nullopt;
                    }
                    AppendUtf8(result, codepoint);
                    break;
                }
                default: return std::nullopt;
                }
            }
            return std::nullopt;
        }

        bool Consume(char expected)
        {
            SkipWhitespace();
            if (pos_ < text_.size() && text_[pos_] == expected) {
                ++pos_;
                return true;
            }
            return false;
        }

        bool ConsumeLiteral(std::string_view literal)
        {
            if (text_.compare(pos_, literal.size(), literal) != 0) return false;
            pos_ += literal.size();
            return true;
        }

        void SkipWhitespace()
        {
            while (pos_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[pos_]))) ++pos_;
        }

        std::string text_;
        std::size_t pos_ = 0;
    };
}
