#pragma once
#include "Core.hpp"
namespace udm {
// Internal player protocol: keep this small and fail closed when its contract changes.
Json youtubePlayerRequest(const Json&);
Json youtubePlayerPair(const Json& response,const Json& request,i64 now=0);
void validatePlayerProbe(const Json& stream,DWORD status,const std::string& range,const std::string& type,size_t received);
Json retrieveYouTubePlayer(const Json&,const Json& preferences);
}
