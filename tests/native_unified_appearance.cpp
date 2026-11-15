#include "UnifiedAppearance.h"
#include <cassert>
#include <string>

int main(){
    const std::string valid=R"({"schema":1,"theme":"fallout76","palette":"hot-omni","colors":{"background":"#123456","panel":"#112233","text":"#AABBCC","accent":"#deadbe","muted":"#010203","border":"#998877","highlight":"#abcdef"}})";
    using K2040::UnifiedAppearance::Validated;
    assert(Validated(valid)==valid);
    std::string custom=valid;
    custom.replace(custom.find("hot-omni"),8,"user-19");
    assert(Validated(custom).has_value());
    std::string invalid=valid;
    invalid.replace(invalid.find("fallout76"),9,"unknown__");
    assert(!Validated(invalid));
    invalid=valid;
    invalid.replace(invalid.find("#deadbe"),7,"redblue");
    assert(!Validated(invalid));
    invalid=valid;
    invalid.replace(invalid.find("hot-omni"),8,"user-x99");
    assert(!Validated(invalid));
    invalid=valid;
    invalid.replace(invalid.find("\"border\":"),9,"\"boXder\":");
    assert(!Validated(invalid));
    assert(!Validated("{}"));
    assert(!Validated(valid+"garbage"));
}
