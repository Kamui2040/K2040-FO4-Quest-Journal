#define K2040_SINGLE_MISC_WRITE_QA 1
#include "../research/native_one_shot/SingleMiscWriteGate.h"
#include <cassert>
using namespace K2040::SingleMiscQA;
int main() {
 Observation o{};
 assert(Review(o,false)==Verdict::kReject);
 o.formID=kVerifiedQuestFormID;
 o.validSession=true;o.selectedAndObserved=true;o.complete=true;
 o.rootRowAbsent=true;o.exactlyOneNestedRow=true;
 o.nestedActiveTyped=true;o.nestedEnabledTyped=true;o.nestedEnabled=true;
 o.nestedInstanceTyped=true;o.nestedIndexAbsent=true;
 o.exactlyOneNativeDisplayed=true;o.nativeIndex=50;o.nativeInstance=1;
 o.nestedInstance=1;o.currentInstance=1;o.questType=6;o.questFlags=0x801;
 o.nestedActive=true;
 assert(Review(o,false)==Verdict::kReady);
 assert(Review(o,true)==Verdict::kAlready);
 o.formID=0x0006DFAEu;
 assert(Review(o,false)==Verdict::kReject);
 o.formID=kVerifiedQuestFormID;
 o.nativeIndex=51;assert(Review(o,false)==Verdict::kReject);o.nativeIndex=50;
 o.exactlyOneNativeDisplayed=false;assert(Review(o,false)==Verdict::kReject);o.exactlyOneNativeDisplayed=true;
 o.nestedIndexAbsent=false;assert(Review(o,false)==Verdict::kReject);o.nestedIndexAbsent=true;
 o.nestedInstance=2;assert(Review(o,false)==Verdict::kReject);o.nestedInstance=1;
 o.currentInstance=2;assert(Review(o,false)==Verdict::kReject);o.currentInstance=1;
 o.nestedEnabled=false;assert(Review(o,false)==Verdict::kReject);o.nestedEnabled=true;
 o.rootRowAbsent=false;assert(Review(o,false)==Verdict::kReject);o.rootRowAbsent=true;
 o.exactlyOneNestedRow=false;assert(Review(o,false)==Verdict::kReject);o.exactlyOneNestedRow=true;
 o.truncated=true;assert(Review(o,false)==Verdict::kReject);o.truncated=false;
 o.complete=false;assert(Review(o,false)==Verdict::kReject);o.complete=true;
 o.questFlags|=0x2;assert(Review(o,false)==Verdict::kReject);o.questFlags&=~0x2u;
 o.questFlags|=0x40;assert(Review(o,false)==Verdict::kReject);o.questFlags&=~0x40u;
 o.questFlags=1;assert(Review(o,false)==Verdict::kReject);o.questFlags=0x801;
 o.nestedActive=false;assert(Review(o,false)==Verdict::kReject);
}
