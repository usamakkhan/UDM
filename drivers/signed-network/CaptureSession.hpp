#pragma once
#include <memory>
#include <vector>
#include <cstdint>
#include <stdexcept>
namespace udmbroker {
// Only the broker command thread changes configuration. A generation identifies
// one gateway lifetime; observation IDs must also be bound to the pipe nonce.
template<class Gateway> class CaptureSession {
    std::unique_ptr<Gateway> gateway;
    std::vector<unsigned> ports;
    uint64_t generation=0;
public:
    template<class Factory> void configure(const std::vector<unsigned>& requested,bool hasRoots,Factory create) {
        if(requested==ports)return;
        if(!requested.empty()&&!hasRoots)throw std::runtime_error("Start process monitoring before HTTP capture.");
        if(generation==UINT64_MAX)throw std::runtime_error("Restart the capture session before changing its scope.");
        auto nextPorts=requested;
        gateway.reset();ports.clear();++generation;
        if(!requested.empty()){
            auto next=create(requested);if(!next)throw std::runtime_error("Cannot create the HTTP capture gateway.");
            gateway=std::move(next);ports=std::move(nextPorts);
        }
    }
    void stop(){configure({},true,[](const auto&){return std::unique_ptr<Gateway>{};});}
    Gateway* get()const{return gateway.get();}
    const std::vector<unsigned>& destinationPorts()const{return ports;}
    uint64_t currentGeneration()const{return generation;}
};
}
