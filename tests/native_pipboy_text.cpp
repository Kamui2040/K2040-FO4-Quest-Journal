#include "SafePipboyText.h"
#include <cassert>
#include <string>

int main() {
    using K2040::Quests::ReadBoundedGameString;
    assert(ReadBoundedGameString(nullptr).empty());
    assert(ReadBoundedGameString(reinterpret_cast<const char*>(0x1234)).empty());
    assert(ReadBoundedGameString("").empty());
    assert(ReadBoundedGameString("A normal quest title") == "A normal quest title");
    assert(ReadBoundedGameString("test\0ignored") == "test");
    const std::string tooLong(4096, 'x');
    assert(ReadBoundedGameString(tooLong.c_str()).empty());
    const std::string atLimit(4095, 'y');
    assert(ReadBoundedGameString(atLimit.c_str()) == atLimit);
}
