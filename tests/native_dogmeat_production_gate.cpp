// Pure production-candidate gate test; no QA defines, no game memory.
#define K2040_DOGMEAT_PRODUCTION_CANDIDATE 1
#include "../research/native_one_shot/SingleMiscWriteGate.h"
#include <cassert>
using namespace K2040::SingleMiscQA;
int main() {
    Observation o{};
    o.formID=kVerifiedQuestFormID;
    o.validSession=o.selectedAndObserved=o.complete=true;
    o.rootRowAbsent=o.exactlyOneNestedRow=true;
    o.nestedActiveTyped=o.nestedEnabledTyped=o.nestedEnabled=true;
    o.nestedInstanceTyped=o.nestedIndexAbsent=true;
    o.exactlyOneNativeDisplayed=true;
    o.nativeIndex=kObservedIndex;
    o.nativeInstance=o.nestedInstance=o.currentInstance=kObservedInstance;
    o.questType=6u; o.questFlags=0x801u; o.nestedActive=true;
    assert(Review(o,false)==Verdict::kReady);
    assert(Review(o,true)==Verdict::kAlready);
    o.formID=kVerifiedQuestFormID+1;
    assert(Review(o,false)==Verdict::kReject);
    o.formID=kVerifiedQuestFormID;
    o.nativeIndex=51;
    assert(Review(o,false)==Verdict::kReject);
    o.nativeIndex=kObservedIndex;
    o.exactlyOneNativeDisplayed=false;
    assert(Review(o,false)==Verdict::kReject);
    o.exactlyOneNativeDisplayed=true;
    o.currentInstance=2;
    assert(Review(o,false)==Verdict::kReject);
}
