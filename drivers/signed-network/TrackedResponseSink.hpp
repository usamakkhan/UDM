#pragma once
#include "HttpStream.hpp"
#include <atomic>
#include <chrono>
#include <stdexcept>
#include <mutex>
#include <condition_variable>
namespace udmnet {
enum class ResponsePhase {Receiving,Finalizing,Complete,Cancelled,Failed};
struct ResponseProgress {
    std::atomic<ResponsePhase> phase{ResponsePhase::Receiving};
    std::atomic<uint64_t> received{0};
    const bool flowControlled;
    mutable std::mutex creditMutex;std::condition_variable creditChanged;uint64_t granted=0;
    explicit ResponseProgress(bool controlled=false):flowControlled(controlled){}
    uint64_t credits()const{std::lock_guard<std::mutex> lock(creditMutex);return granted;}
    bool grant(uint64_t total){
        std::lock_guard<std::mutex> lock(creditMutex);
        if(!flowControlled||phase.load()!=ResponsePhase::Receiving||total<granted)return false;
        granted=total;creditChanged.notify_all();return true;
    }
    size_t permitted(size_t requested){
        if(!flowControlled)return requested;
        std::unique_lock<std::mutex> lock(creditMutex);
        creditChanged.wait(lock,[&]{return phase.load()!=ResponsePhase::Receiving||granted>received.load();});
        if(phase.load()!=ResponsePhase::Receiving)throw std::runtime_error("Original response stopped while waiting for transfer allowance.");
        return static_cast<size_t>(std::min<uint64_t>(requested,granted-received.load()));
    }
    static uint64_t clockNs(){return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}
    const uint64_t startedNs=clockNs();std::atomic<uint64_t> finishedNs{0};
    void finishTiming(){uint64_t empty=0;finishedNs.compare_exchange_strong(empty,clockNs());}
    double seconds()const{auto end=finishedNs.load();return double((end?end:clockNs())-startedNs)/1000000000.0;}
    bool cancel(){std::lock_guard<std::mutex> lock(creditMutex);auto expected=ResponsePhase::Receiving;bool result=phase.compare_exchange_strong(expected,ResponsePhase::Cancelled)||expected==ResponsePhase::Cancelled;if(result){finishTiming();creditChanged.notify_all();}return result;}
};
class TrackedResponseSink final:public ResponseSink {
    std::unique_ptr<ResponseSink> sink;std::shared_ptr<ResponseProgress> progress;bool aborted=false;
public:
    TrackedResponseSink(std::unique_ptr<ResponseSink> value,std::shared_ptr<ResponseProgress> state):sink(std::move(value)),progress(std::move(state)){}
    ~TrackedResponseSink(){abort();}
    bool cancelled()const noexcept override{return progress->phase.load()==ResponsePhase::Cancelled;}
    void write(std::string_view bytes)override{
        if(progress->phase.load()!=ResponsePhase::Receiving)throw std::runtime_error("Original response is no longer receiving.");
        try{while(!bytes.empty()){
            auto count=progress->permitted(bytes.size());
            if(progress->phase.load()!=ResponsePhase::Receiving)throw std::runtime_error("Original response was stopped before writing.");
            sink->write(bytes.substr(0,count));progress->received.fetch_add(count);bytes.remove_prefix(count);
        }}catch(...){abort();throw;}
    }
    void complete()override{
        auto expected=ResponsePhase::Receiving;
        if(!progress->phase.compare_exchange_strong(expected,ResponsePhase::Finalizing))throw std::runtime_error("Original response was stopped before completion.");
        try{sink->complete();progress->finishTiming();progress->phase=ResponsePhase::Complete;}catch(...){abort();throw;}
    }
    void abort()noexcept override{
        auto phase=progress->phase.load();if(phase==ResponsePhase::Complete||aborted)return;
        while(phase==ResponsePhase::Receiving||phase==ResponsePhase::Finalizing)if(progress->phase.compare_exchange_weak(phase,ResponsePhase::Failed))break;
        progress->finishTiming();aborted=true;sink->abort();
    }
};
}
