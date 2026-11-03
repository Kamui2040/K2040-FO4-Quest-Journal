#include "../src/GenericQuestRowIdentity.h"
#include <array>
#include <cassert>
#include <string_view>
#include <vector>

namespace G=K2040::GenericQuestRowIdentity;
using G::Kind;
using G::Row;
using G::Native;
using G::Status;
void Match(Status wanted, const std::vector<G::Result>& result,
           std::size_t row=0) {
    assert(row<result.size());
    assert(result[row].status==wanted);
}
int main(){
    // All IDs are synthetic. No whitelist or specific runtime FormID.
    constexpr std::uint32_t a=0x010F1020, b=0xFE000AAA, c=0x030000E4;
    const std::array<Native,6> native{{
        {a,10,7,true},{a,11,7,true},{b,0,2,true},
        {c,10,17,true},{c,10,18,true},{a,12,7,false}
    }};
    const std::array<Row,5> rows{{
        {a,Kind::kMiscObjective,10,7,true},
        {a,Kind::kMiscObjective,11,7,true},
        {b,Kind::kMiscObjective,std::nullopt,2,true},
        {c,Kind::kMiscObjective,10,18,true},
        {a,Kind::kQuest,std::nullopt,std::nullopt,true}
    }};
    const auto mapped=G::Resolve(rows,native,true);
    assert(mapped.size()==rows.size());
    for(std::size_t i=0;i<4;++i){
        Match(Status::kUniqueCandidateNotVerified,mapped,i);
        assert(mapped[i].nativeOrdinal.has_value());
    }
    assert(*mapped[0].nativeOrdinal!=*mapped[1].nativeOrdinal);
    Match(Status::kQuestWideOnly,mapped,4);
    // A multi-objective Misc row without an objective index is ambiguous.
    Match(Status::kAmbiguous,G::Resolve(
        std::array<Row,1>{{{a,Kind::kMiscObjective,std::nullopt,7,true}}},
        native,true));
    // A radiant instance must match the live instance, not just the quest.
    Match(Status::kUniqueCandidateNotVerified,G::Resolve(
        std::array<Row,1>{{{c,Kind::kMiscObjective,10,17,true}}},native,true));
    Match(Status::kNoCandidate,G::Resolve(
        std::array<Row,1>{{{c,Kind::kMiscObjective,10,19,true}}},native,true));
    // Two Pip-Boy entries for precisely the same objective must NOT become
    // two independent writable commands; duplicate identity fails closed.
    const std::array<Row,2> duplicate{{rows[0],rows[0]}};
    const auto duplicates=G::Resolve(duplicate,native,true);
    Match(Status::kDuplicatePipboyRow,duplicates,0);
    Match(Status::kDuplicatePipboyRow,duplicates,1);
    assert(!duplicates[0].nativeOrdinal && !duplicates[1].nativeOrdinal);
    const std::array<Row,3> triple{{rows[0],rows[0],rows[0]}};
    const auto tripled=G::Resolve(triple,native,true);
    for(std::size_t i=0;i<3;++i){
        Match(Status::kDuplicatePipboyRow,tripled,i);
        assert(!tripled[i].nativeOrdinal);
    }
    Match(Status::kInvalidRow,G::Resolve(
        std::array<Row,1>{{{a,static_cast<Kind>(99),10,7,true}}},
        native,true));
    Match(Status::kMissingInstance,G::Resolve(
        std::array<Row,1>{{{a,Kind::kMiscObjective,10,std::nullopt,true}}},
        native,true));
    Match(Status::kInvalidRow,G::Resolve(
        std::array<Row,1>{{{0,Kind::kMiscObjective,10,7,true}}},native,true));
    Match(Status::kInvalidRow,G::Resolve(
        std::array<Row,1>{{{a,Kind::kMiscObjective,10,7,false}}},native,true));
    Match(Status::kIncomplete,G::Resolve(
        std::array<Row,1>{{rows[0]}},native,false));
    Match(Status::kIncomplete,G::Resolve(std::vector<Row>(513,rows[0]),native,true));
    Match(Status::kIncomplete,G::Resolve(
        std::array<Row,1>{{rows[0]}},std::vector<Native>(513,native[0]),true));
    // A hidden/non-displayed objective cannot be treated as a selectable row.
    Match(Status::kNoCandidate,G::Resolve(
        std::array<Row,1>{{{a,Kind::kMiscObjective,12,7,true}}},native,true));
    // Objective index zero is valid, as are dynamically allocated FormIDs.
    Match(Status::kUniqueCandidateNotVerified,G::Resolve(
        std::array<Row,1>{{{b,Kind::kMiscObjective,0,2,true}}},native,true));
    assert(std::string_view(G::StatusName(Status::kUniqueCandidateNotVerified))==
           "candidate-NOT-native-command");
}
