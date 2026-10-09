#pragma once
#include "CaptureObservations.hpp"
#include "third_party/json.hpp"
namespace udmnet {
using ObservationJson=nlohmann::json;
// HTTP field octets are not necessarily UTF-8. Preserve non-UTF-8 fields as
// the JSON library's binary {bytes:[...],subtype:null} representation.
inline ObservationJson captureWireBytes(const std::string& bytes) {
    ObservationJson value=bytes;
    try{value.dump();return value;}
    catch(const ObservationJson::type_error&){return ObservationJson::binary(std::vector<uint8_t>(bytes.begin(),bytes.end()));}
}
inline ObservationJson captureObservationsJson(const CaptureObservationSnapshot& snapshot) {
    ObservationJson rows=ObservationJson::array();
    for(const auto& observation:snapshot.entries){const auto& c=observation.candidate;
        rows.push_back({{"Id",observation.id},{"ConnectionId",observation.connection},{"ProcessId",observation.process},
            {"ProcessCreated100ns",observation.processCreated},{"Method",captureWireBytes(c.request.method)},{"Host",captureWireBytes(c.request.host)},
            {"Target",captureWireBytes(c.request.target)},{"Range",captureWireBytes(c.request.range)},{"Status",c.status},{"ContentType",captureWireBytes(c.contentType)},
            {"ContentDisposition",captureWireBytes(c.contentDisposition)},{"ContentRange",captureWireBytes(c.contentRange)},{"Location",captureWireBytes(c.location)},
            {"Length",c.lengthKnown?ObservationJson(c.length):ObservationJson(nullptr)},
            {"WasInterceptable",c.canIntercept},{"RequestBodyFramed",c.request.bodyFramed},{"RequestBodyRetained",false},{"ResponseRetained",false}});
    }
    return {{"Observed",snapshot.observed},{"Evicted",snapshot.evicted},{"Rejected",snapshot.rejected},
        {"RetainedFieldBytes",snapshot.retainedBytes},{"Items",rows}};
}
}
