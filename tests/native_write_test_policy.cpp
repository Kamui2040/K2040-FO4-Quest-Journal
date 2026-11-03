#include "../research/native_write_test/TestPolicy.h"
#include <cassert>
#include <cstdint>
using namespace K2040::NativeWriteTest;

int main() {
    Snapshot sample{
        true, false, true, true, true, true, true, true, true,
        true, true, false,
        0x1234u, 0x1234u, 1, 1, 1000,
        kEnabled, 1
    };
    assert(Evaluate(sample) == Decision::kAllowExactlyOnce);
    auto deny = sample;
    deny.armed=false;
    assert(Evaluate(deny) == Decision::kDeny);
    deny=sample;deny.alreadyUsed=true;assert(Evaluate(deny)==Decision::kDeny);
    deny=sample;deny.gameLoaded=false;assert(Evaluate(deny)==Decision::kDeny);
    deny=sample;deny.validOgRuntime=false;assert(Evaluate(deny)==Decision::kDeny);
    deny=sample;deny.matchingEngine=false;assert(Evaluate(deny)==Decision::kDeny);
    deny=sample;deny.deliberateChord=false;assert(Evaluate(deny)==Decision::kDeny);
    deny=sample;deny.gameWindowForeground=false;assert(Evaluate(deny)==Decision::kDeny);
    deny=sample;deny.gameTaskMatchesVanillaThread=false;assert(Evaluate(deny)==Decision::kDeny);
    deny=sample;deny.pipboyClosed=false;assert(Evaluate(deny)==Decision::kDeny);
    deny=sample;deny.currentlyVisibleInPipboy=false;assert(Evaluate(deny)==Decision::kDeny);
    deny=sample;deny.authoritativeFeedAvailable=false;assert(Evaluate(deny)==Decision::kDeny);
    deny=sample;deny.candidateFormId=0;assert(Evaluate(deny)==Decision::kDeny);
    deny=sample;deny.resolvedFormId++;assert(Evaluate(deny)==Decision::kDeny);
    deny=sample;deny.sessionEpoch++;assert(Evaluate(deny)==Decision::kDeny);
    deny=sample;deny.candidateAgeMs=45001;assert(Evaluate(deny)==Decision::kDeny);
    deny=sample;deny.questType=6;assert(Evaluate(deny)==Decision::kDeny);
    deny=sample;deny.authoritativeFeedTracked=true;assert(Evaluate(deny)==Decision::kDeny);
    deny=sample;deny.questFlags=static_cast<std::uint16_t>(kEnabled|kTracked);deny.authoritativeFeedTracked=true;assert(Evaluate(deny)==Decision::kAllowExactlyOnce);
    for (unsigned bits=0;bits<65536;++bits) {
        auto each=sample;
        each.questFlags=static_cast<std::uint16_t>(bits);
        each.authoritativeFeedTracked=(bits & kTracked)!=0;
        const auto result=Evaluate(each);
        const bool valid=(bits & kEnabled) && !(bits & (kCompleted|kFailed));
        assert((result==Decision::kAllowExactlyOnce)==valid);
    }
}
