#pragma once
#include "CaptureObservations.hpp"
#include "TrackedResponseSink.hpp"
#include <condition_variable>
#include <chrono>
namespace udmnet {
// Requests wait only while the browser still owns the response. IDs never repeat
// within a broker lifetime; the RPC also binds them to session and generation.
class ResponseAdmissions {
    using Clock=std::chrono::steady_clock;
    struct Entry {CaptureObservation offer;Clock::time_point deadline;bool decided=false;std::unique_ptr<ResponseSink> sink;};
    mutable std::mutex mutex;std::condition_variable changed;
    std::map<uint64_t,std::shared_ptr<Entry>> entries;
    uint64_t sequence=0;bool enabled=false;
    std::map<uint64_t,bool> decisions;
    std::map<uint64_t,std::shared_ptr<ResponseProgress>> active;
    void remember(uint64_t id,bool accepted){decisions[id]=accepted;while(decisions.size()>64)decisions.erase(decisions.begin());}
    static size_t fieldBytes(const DownloadCandidate& c){size_t headers=0;for(const auto& field:c.request.replayHeaders)headers+=field.first.size()+field.second.size();return headers+ c.request.method.size()+c.request.target.size()+c.request.host.size()+c.request.range.size()+c.contentType.size()+c.contentDisposition.size()+c.contentRange.size()+c.location.size()+c.etag.size()+c.lastModified.size();}
public:
    struct Progress {uint64_t id,received;ResponsePhase phase;double seconds;bool flowControlled;uint64_t granted;};
    std::vector<Progress> progress()const{std::lock_guard<std::mutex> lock(mutex);std::vector<Progress> result;for(const auto& [id,state]:active){auto phase=state->phase.load();result.push_back({id,state->received.load(),phase,state->seconds(),state->flowControlled,state->credits()});}return result;}
    bool grant(uint64_t id,uint64_t total){std::lock_guard<std::mutex> lock(mutex);auto it=active.find(id);return it!=active.end()&&it->second->grant(total);}
    bool cancel(uint64_t id){std::lock_guard<std::mutex> lock(mutex);auto it=active.find(id);return it!=active.end()&&it->second->cancel();}
    void reset(bool on){std::lock_guard<std::mutex> lock(mutex);enabled=on;for(auto& item:active)item.second->cancel();for(auto& [id,e]:entries){e->decided=true;if(e->sink)e->sink->abort();e->sink.reset();}entries.clear();changed.notify_all();}
    std::vector<CaptureObservation> pending()const{std::lock_guard<std::mutex> lock(mutex);std::vector<CaptureObservation> result;for(const auto& [id,e]:entries)if(!e->decided&&Clock::now()<e->deadline)result.push_back(e->offer);return result;}
    std::unique_ptr<ResponseSink> offer(uint32_t process,uint64_t created,uint64_t connection,const DownloadCandidate& candidate,std::chrono::milliseconds wait=std::chrono::seconds(3)){
        std::unique_lock<std::mutex> lock(mutex);
        if(!enabled||!candidate.canIntercept||process<=4||!created||!connection||entries.size()>=16||sequence==UINT64_MAX)return {};
        size_t retained=fieldBytes(candidate);for(const auto& item:entries)retained+=fieldBytes(item.second->offer.candidate);
        if(retained>48*1024)return {};
        auto e=std::make_shared<Entry>();e->offer={++sequence,connection,created,process,candidate};e->deadline=Clock::now()+wait;entries.emplace(sequence,e);
        changed.wait_until(lock,e->deadline,[&]{return e->decided;});
        if(!e->decided)remember(e->offer.id,false);
        entries.erase(e->offer.id);return std::move(e->sink);
    }
    bool reject(uint64_t id){std::lock_guard<std::mutex> lock(mutex);auto it=entries.find(id);if(it==entries.end()||it->second->decided)return false;it->second->decided=true;remember(id,false);changed.notify_all();return true;}
    template<class Factory> bool admit(uint64_t id,Factory factory,bool flowControlled=false){
        std::lock_guard<std::mutex> lock(mutex);auto it=entries.find(id);
        auto prior=decisions.find(id);if(prior!=decisions.end())return prior->second;
        if(it==entries.end()||it->second->decided||Clock::now()>=it->second->deadline)return false;
        for(auto old=active.begin();active.size()>=64&&old!=active.end();){auto phase=old->second->phase.load();if(phase!=ResponsePhase::Receiving&&phase!=ResponsePhase::Finalizing)old=active.erase(old);else ++old;}
        if(active.size()>=64)return false;
        auto e=it->second;
        // Serialize against expiry/reconfiguration while the receipt is flushed.
        // Never acknowledge admission if durable preparation missed its deadline.
        try{auto sink=factory(e->offer.candidate);if(!sink||Clock::now()>=e->deadline){if(sink)sink->abort();e->decided=true;remember(id,false);changed.notify_all();return false;}auto state=std::make_shared<ResponseProgress>(flowControlled);e->sink=std::make_unique<TrackedResponseSink>(std::move(sink),state);active.emplace(id,std::move(state));}
        catch(...){if(e->sink){e->sink->abort();e->sink.reset();}active.erase(id);e->decided=true;remember(id,false);changed.notify_all();throw;}
        e->decided=true;remember(id,true);changed.notify_all();return true;
    }
};
}
