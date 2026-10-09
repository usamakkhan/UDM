#pragma once
#include "HttpStream.hpp"
#include <mutex>
#include <vector>
namespace udmnet {
struct CaptureObservation {
    uint64_t id=0,connection=0,processCreated=0;
    uint32_t process=0;
    DownloadCandidate candidate;
};
struct CaptureObservationSnapshot {
    std::vector<CaptureObservation> entries;
    uint64_t observed=0,evicted=0,rejected=0;
    size_t retainedBytes=0;
};
// Observation is not ownership: the original response continues unless the
// existing synchronous decision callback explicitly accepts it.
class CaptureObservations {
    mutable std::mutex mutex;
    std::deque<CaptureObservation> entries;
    uint64_t sequence=0,observed=0,evicted=0,rejected=0;
    size_t retainedBytes=0;
    static size_t bytes(const DownloadCandidate& c) {
        return c.request.method.size()+c.request.target.size()+c.request.host.size()+c.request.range.size()+
            c.contentType.size()+c.contentDisposition.size()+c.contentRange.size()+c.location.size();
    }
public:
    static constexpr size_t MaxEntries=64,MaxBytes=48*1024;
    uint64_t append(uint32_t process,uint64_t created,uint64_t connection,const DownloadCandidate& candidate) {
        std::lock_guard<std::mutex> lock(mutex);
        const auto size=bytes(candidate);
        if(process<=4||!created||!connection||size>MaxBytes||sequence==UINT64_MAX){++rejected;return 0;}
        // Allocate first: failure must not evict previously captured evidence.
        CaptureObservation next{sequence+1,connection,created,process,candidate};
        entries.push_back(std::move(next));++sequence;++observed;retainedBytes+=size;
        while(entries.size()>MaxEntries||retainedBytes>MaxBytes){retainedBytes-=bytes(entries.front().candidate);entries.pop_front();++evicted;}
        return sequence;
    }
    CaptureObservationSnapshot snapshot()const {
        std::lock_guard<std::mutex> lock(mutex);
        return {{entries.begin(),entries.end()},observed,evicted,rejected,retainedBytes};
    }
};
}
