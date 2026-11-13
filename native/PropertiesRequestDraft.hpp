#pragma once
#include "GuiModels.hpp"

namespace udm {
struct PropertiesRequestInput {std::string user,password,referer;};
// Carry only logical header changes across the unlocked staging/move interval.
class PropertiesHeaderPatch {
 using Entry=std::pair<std::string,std::string>;
 using Logical=std::map<std::string,Entry>;
 Logical assigned;std::set<std::string> removed;std::string requiredOrigin;bool sensitiveAssignment=false;
 static Logical logical(const Headers& headers){
  Logical result;for(const auto& entry:headers)result.emplace(lower(entry.first),entry);return result;
 }
public:
 PropertiesHeaderPatch(const Headers& before,const Headers& after,const std::string& target=""){
  if(!target.empty())requiredOrigin=Url(target).origin;
  validateHeaders(before);validateHeaders(after);const auto original=logical(before),desired=logical(after);
  for(const auto& entry:original)if(!desired.count(entry.first))removed.insert(entry.first);
  for(const auto& entry:desired){auto found=original.find(entry.first);if(found==original.end()||found->second.second!=entry.second.second)assigned.emplace(entry);}
  for(const auto& entry:assigned)if(!entry.second.second.empty()&&(entry.first=="authorization"||entry.first=="cookie"||entry.first=="referer"||entry.first=="origin"))sensitiveAssignment=true;
 }
 void apply(Json& edit,const Json& current,std::string(*seal)(const std::string&)=protect)const{
  if(assigned.empty()&&removed.empty())return;
  if(sensitiveAssignment&&!requiredOrigin.empty()&&Url(str(current,"Url")).origin!=requiredOrigin)
   throw std::runtime_error("The download address changed to another site. Reopen Properties before saving request headers. Your edits are still in this dialog.");
  const auto before=readHeaders(current);auto next=before;const auto available=logical(before);
  for(const auto& name:removed)setHeader(next,name,"");
  for(const auto& entry:assigned){
   const auto found=available.find(entry.first);
   if(found!=available.end()&&found->second.second==entry.second.second)continue;
   setHeader(next,entry.second.first,"");next[entry.second.first]=entry.second.second;
  }
  validateHeaders(next);if(next==before)return;
  Json protectedValue=next.empty()?"":seal(legacyDictionary(Json(next)).dump());
  edit["ProtectedHeaders"].swap(protectedValue);
 }
};
// The baseline describes the controls, not a replacement for a refreshed session.
class PropertiesRequestDraft {
 PropertiesRequestInput displayed;
 using Seal=std::string(*)(const std::string&);
 Seal seal;
public:
 explicit PropertiesRequestDraft(PropertiesRequestInput initial,Seal protector=protect):displayed(std::move(initial)),seal(protector){}
 void reset(PropertiesRequestInput input){displayed=std::move(input);}
 void apply(Json& draft,PropertiesRequestInput input){
  auto headers=readHeaders(draft);const auto previous=headers;
  if(input.referer!=displayed.referer)setHeader(headers,"Referer",trim(input.referer));
  if(input.user!=displayed.user||input.password!=displayed.password)
   setBasicLogin(headers,input.user,input.password,!input.user.empty()||!input.password.empty());
  validateHeaders(headers);
  if(headers!=previous){
   Json protectedHeaders=headers.empty()?"":seal(legacyDictionary(Json(headers)).dump());
   draft["ProtectedHeaders"].swap(protectedHeaders);
  }
  // Rejecting validation or protection leaves both the draft and its control
  // baseline unchanged. Retrying must still attempt every typed field.
  displayed=std::move(input);
 }
};
}
