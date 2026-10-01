#pragma once
#include "Core.hpp"
namespace udm {
// Presentation metadata contains only opaque download IDs and a UI action.
// Callers publish changes together with the user's download decision.
inline bool eraseCapturePresentations(Json& state,const std::string& id){
 if(!state.contains("BrowserCaptures")||!state["BrowserCaptures"].is_object())return false;
 bool changed=false;for(auto& row:state["BrowserCaptures"])if(str(row.value("presentation",Json::object()),"id")==id){row.erase("presentation");changed=true;}
 return changed;
}
inline void retargetCapturePresentations(Json& state,const std::string& from,const std::string& to,const char* kind){
 if(!state.contains("BrowserCaptures")||!state["BrowserCaptures"].is_object())return;
 for(auto& row:state["BrowserCaptures"])if(str(row.value("presentation",Json::object()),"id")==from)row["presentation"]={{"id",to},{"kind",kind}};
}
inline void restoreCaptureRows(Json& state,const Json& previous){if(previous.is_null())state.erase("BrowserCaptures");else state["BrowserCaptures"]=previous;}
}
