#include "JsonPreferencesParser.h"
#include <cassert>
#include <string>

int main() {
    using k2040::json::JsonParser;
    auto yes = JsonParser("true").Parse();
    assert(yes && yes->AsBool() && *yes->AsBool());
    auto no = JsonParser("false").Parse();
    assert(no && no->AsBool() && !*no->AsBool());
    auto number = JsonParser("520").Parse();
    assert(number && number->AsNumber() && *number->AsNumber() == 520.0);
    auto negative = JsonParser("-1").Parse();
    assert(negative && negative->AsNumber() && *negative->AsNumber() == -1.0);
    auto hex = JsonParser("\"#aAbbCc\"").Parse();
    assert(hex && hex->AsString() && *hex->AsString() == "#aAbbCc");
    assert(!JsonParser("tru").Parse());
    assert(!JsonParser("\"unterminated").Parse());
    assert(!JsonParser("true false").Parse());
    assert(!JsonParser("{\"a\":true,\"a\":false}").Parse());
    assert(!JsonParser("1e999").Parse());
    auto object = JsonParser("{\"danger\":true}").Parse();
    assert(object && object->AsObject()); // owner must refuse non-scalar values
}
