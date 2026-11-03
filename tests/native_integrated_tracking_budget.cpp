#include "../research/native_one_shot/OneShotOgIntegratedQA.h"
#include <atomic>
#include <cassert>
#include <thread>
#include <vector>
using namespace K2040::OneShotResearch;
int main() {
    static_assert(kIntegratedMaxAttempts == 8);
    std::atomic_uint32_t used{0};
    std::atomic_uint32_t successes{0};
    std::vector<std::thread> threads;
    for (int i=0;i<12;++i) {
        threads.emplace_back([&] {
            for (int j=0;j<100;++j)
                if(ReserveIntegratedAttempt(used))
                    successes.fetch_add(1,std::memory_order_relaxed);
        });
    }
    for (auto& t:threads)t.join();
    assert(used.load()==kIntegratedMaxAttempts);
    assert(successes.load()==kIntegratedMaxAttempts);
    assert(!ReserveIntegratedAttempt(used));
    // A new Journal/save must NOT reset the process limit.
    Gate gate;
    gate.BeginSession(1,1);
    auto a=gate.Offer(42,true,1,1);
    assert(a && gate.Settle(*a));
    gate.BeginSession(2,3);
    assert(!ReserveIntegratedAttempt(used));
}
