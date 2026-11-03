#include "../research/native_one_shot/OneShotIntent.h"
#include <cassert>
#include <functional>
#include <memory>
#include <utility>
#include <vector>
using namespace K2040::OneShotResearch;

int main() {
    auto gate=std::make_shared<Gate>();
    gate->BeginSession(3,8);
    std::vector<std::function<void()>> jobs;
    int reads=0, reports=0;
    auto observe=[&](const Intent& ticket)->Observation {
        ++reads;
        return {ticket.formId,ticket.formId,ticket.saveEpoch,ticket.uiEpoch,
                true,true,true,true,true,true,true,false,kEnabled,7};
    };
    auto onResult=[&](Result value){
        ++reports;
        assert(value==Result::kWouldWrite);
    };
    auto enqueue=[&](std::function<void()> job) {
        jobs.emplace_back(std::move(job));
        return true;
    };
    assert(QueueOneShot(gate,10,true,3,8,enqueue,observe,onResult));
    assert(jobs.size()==1);
    assert(!QueueOneShot(gate,11,false,3,8,enqueue,observe,onResult));
    assert(jobs.size()==1);
    assert(reads==0 && reports==0);
    std::move(jobs[0])();
    assert(reads==1 && reports==1);
    assert(QueueOneShot(gate,11,false,3,8,enqueue,observe,onResult));
    assert(jobs.size()==2);
    gate->BeginSession(4,9); // Game save loaded while task was pending.
    std::move(jobs[1])();
    assert(reads==1 && reports==1); // stale task dropped with no reads
    assert(!QueueOneShot(gate,12,true,3,8,enqueue,observe,onResult));
    const auto refused=[](std::function<void()>)->bool{return false;};
    assert(!QueueOneShot(gate,12,true,4,9,refused,observe,onResult));
    assert(QueueOneShot(gate,12,true,4,9,enqueue,observe,onResult));
    assert(jobs.size()==3);
    gate->BeginSession(4,10); // Panel replaced while task was pending.
    std::move(jobs[2])();
    assert(reads==1 && reports==1);
    assert(QueueOneShot(gate,12,true,4,10,enqueue,observe,onResult));
    assert(jobs.size()==4);
    std::move(jobs[3])();
    assert(reads==2 && reports==2);
}
