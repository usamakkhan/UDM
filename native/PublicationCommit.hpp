#pragma once
#include "Core.hpp"
namespace udm {
// Correlation for ordinary file publication. A runtime ID is not itself a
// durable receipt: snapshot-bound acknowledgements supply that proof.
class PublicationCommit {
public:
 static std::string identity(const Json& operation){
  if(!operation.contains("CommitId"))return {};
  const auto id=str(operation,"CommitId");if(!operation["CommitId"].is_string()||!std::regex_match(id,std::regex("[a-f0-9]{32}"))||!operation.contains("After")||str(operation["After"],"PublicationCommitId")!=id)throw std::runtime_error("Publication completion identity changed. Keep the file and recovery record.");return id;
 }
 static Json apply(const Json& before,const Json& after,const Json& current){
  auto result=current;for(auto it=before.begin();it!=before.end();++it)if(!after.contains(it.key()))result.erase(it.key());
  for(auto it=after.begin();it!=after.end();++it)if(!before.contains(it.key())||before.at(it.key())!=it.value())result[it.key()]=it.value();
  for(const char* key:{"Status","Size","Received","Sha256","Finished","Error","QueueMember","MediaPhase","MediaTracksReady","SabrRetainedBytes","PublicationCommitId"})if(after.contains(key))result[key]=after.at(key);
  if(str(before,"HttpContentEncoding")=="gzip"||(yes(before,"PostResponseComplete")&&yes(before,"StripContentPrefix")&&num(before,"ContentPrefixBytes")>0))for(const char* key:{"HttpWireSize","HttpWireReceived","HttpDecodedSize","HttpDecodedReceived","Segments"})if(after.contains(key))result[key]=after.at(key);
  return result;
 }
 static Json rollback(const Json& before,const Json& applied,const Json& current){auto result=current;for(auto it=applied.begin();it!=applied.end();++it)if((!before.contains(it.key())||before.at(it.key())!=it.value())&&result.contains(it.key())&&result.at(it.key())==it.value()){if(before.contains(it.key()))result[it.key()]=before.at(it.key());else result.erase(it.key());}for(auto it=before.begin();it!=before.end();++it)if(!applied.contains(it.key())&&!result.contains(it.key()))result[it.key()]=it.value();return result;}
 static bool accepted(const Json& operation,const Json& current,const std::string& verifiedHash){
  auto id=identity(operation);const auto& after=operation.at("After");return !id.empty()&&str(current,"PublicationCommitId")==id&&str(current,"Status")=="Complete"&&num(current,"Size",-1)==num(after,"Size",-2)&&num(current,"Received",-1)==num(after,"Size",-2)&&str(current,"Finished")==str(after,"Finished")&&(str(current,"Sha256").empty()||str(current,"Sha256")==verifiedHash);
 }
 static void validateCurrent(const Json& operation,const Json& current){auto id=identity(operation);if(id.empty())return;const auto currentId=str(current,"PublicationCommitId");if(currentId!=id&&currentId!=str(operation.at("Before"),"PublicationCommitId"))throw std::runtime_error("A newer publication owns this download. Keep the file and recovery record.");}
};
}
