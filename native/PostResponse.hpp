#pragma once
#include "Core.hpp"
#include "StreamHash.hpp"
#include <string_view>
namespace udm {
inline std::string_view postResponseText(const Json& data,const char* key){
 const auto found=data.find(key);return found!=data.end()&&found->is_string()?std::string_view(found->get_ref<const std::string&>()):std::string_view{};
}
inline std::string postResponseRequestBinding(const Json& data){
 const auto found=data.find("ProtectedRequest");if(found==data.end()||!found->is_string())return {};
 const auto& request=found->get_ref<const std::string&>();if(request.empty())return {};
 StreamHash hash;hash.update(request.data(),request.size());return hash.finish();
}
// A matching catalog receipt is only a candidate. The worker must verify the
// saved files before using them; UI/queue checks never establish file integrity.
inline bool completedPostResponseCandidate(const Json& data){
 if(!hasNonemptyString(data,"ProtectedRequest")||!yes(data,"PostResponseComplete")||yes(data,"RefreshPendingValidation"))return false;
 const auto found=data.find("CompletedPrefetch");if(found==data.end()||!found->is_object())return false;
 const auto& receipt=*found;const auto size=num(data,"Size",-1);
 static const Json empty=Json::array();
 const auto parts=data.find("Segments"),saved=receipt.find("Plan"),hashes=receipt.find("Hashes");
 const auto& plan=parts==data.end()?empty:*parts;
 const auto& savedPlan=saved==receipt.end()?empty:*saved;
 return num(receipt,"Version")==1&&size>=0&&num(data,"Received")==size&&num(receipt,"Size",-1)==size&&
  postResponseText(receipt,"Url")==postResponseText(data,"Url")&&postResponseText(receipt,"Headers")==postResponseText(data,"ProtectedHeaders")&&
  !postResponseText(data,"PostResponseRequestSha256").empty()&&postResponseText(receipt,"RequestSha256")==postResponseText(data,"PostResponseRequestSha256")&&postResponseText(receipt,"ContentEncoding")==postResponseText(data,"HttpContentEncoding")&&
  postResponseText(receipt,"PostGetUrl")==postResponseText(data,"ProtectedPostGetUrl")&&postResponseText(receipt,"PostGetHeaders")==postResponseText(data,"ProtectedPostGetHeaders")&&
  savedPlan==plan&&hashes!=receipt.end()&&hashes->is_array()&&hashes->size()==plan.size();
}
}
