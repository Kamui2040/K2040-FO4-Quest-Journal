#include "../src/MiscProbeLogSanitizer.h"
#include <cassert>
#include <string>
using K2040::Quests::AnonymousMiscProbeJson;
int main() {
    const std::string original=R"({"id":"12345678","available":true,"questType":6,"nativeTracked":false,"rootRows":1,"miscRows":2,"miscTracked":1,"objectives":[{"index":0,"activeKnown":true,"active":true},{"index":4,"activeKnown":true,"active":false}]})";
    const auto result=AnonymousMiscProbeJson(original);
    assert(result);
    assert(result->starts_with(R"({"available":true)"));
    assert(result->find("12345678")==std::string::npos);
    assert(result->find(R"("index":0)")!=std::string::npos);
    assert(result->find(R"("miscTracked":1)")!=std::string::npos);
    assert(!AnonymousMiscProbeJson(R"({"available":false})"));
    assert(!AnonymousMiscProbeJson(R"({"id":"BAD","available":true})"));
    assert(!AnonymousMiscProbeJson(R"({"id":"1234567Z","available":true})"));
    assert(!AnonymousMiscProbeJson(R"({"id":"12345678","unknown":true})"));
    assert(!AnonymousMiscProbeJson(std::string(4096,'x')));
}
