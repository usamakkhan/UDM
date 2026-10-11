#pragma once
#include "CaptureSession.hpp"
#include "ResponseAdmissions.hpp"
namespace udmbroker {
// The gateway owns a parser configured for one admission mode. Reset pending
// admissions before replacing it; publish the new mode only after startup.
template<class Gateway,class Factory>
void configureResponseCapture(CaptureSession<Gateway>& capture,udmnet::ResponseAdmissions& admissions,
    bool& enabled,const std::vector<unsigned>& ports,bool hasRoots,bool requested,Factory create){
    requested=requested&&!ports.empty();
    if(!ports.empty()&&!hasRoots)throw std::runtime_error("Start process monitoring before HTTP capture.");
    if(ports==capture.destinationPorts()&&requested==enabled)return;
    admissions.reset(false);enabled=false;
    capture.stop();
    capture.configure(ports,hasRoots,[&](const auto& selected){return create(selected,requested);});
    admissions.reset(requested);enabled=requested;
}
}
