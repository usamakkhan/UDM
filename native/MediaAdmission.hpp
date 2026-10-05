#pragma once
#include "CaptureReceipts.hpp"
#include <bcrypt.h>
namespace udm {
inline std::string mediaAdmissionDigest(const Json& request){
 auto serialized=request.dump();if(serialized.size()>200000)throw std::runtime_error("Media admission exceeds the message limit.");
 BCRYPT_ALG_HANDLE algorithm=nullptr;if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)throw std::runtime_error("Cannot identify media admission.");
 unsigned char bytes[32]{};auto result=BCryptHash(algorithm,nullptr,0,(PUCHAR)serialized.data(),(ULONG)serialized.size(),bytes,sizeof(bytes));BCryptCloseAlgorithmProvider(algorithm,0);
 if(result<0)throw std::runtime_error("Cannot identify media admission.");std::string out;const char* hex="0123456789abcdef";for(auto value:bytes){out+=hex[value>>4];out+=hex[value&15];}return out;
}
inline Json mediaAdmissionStatus(Manager& manager,const std::string& token){
 Lock lock(manager.mutex);auto created=captureTime(token),now=epoch();auto rows=captureStore(manager);
 if(rows.contains(token)){
  const auto& row=rows[token];if(num(row,"protocol")!=3)throw std::runtime_error("This token belongs to another browser operation.");
  auto status=str(row,"status");if(status=="released")return {{"ok",true},{"status","released"}};
  if(status!="accepted"||str(row,"id").empty())throw std::runtime_error("Invalid media admission receipt.");
  bool present=false;for(auto job:manager.jobs)if(job->id()==str(row,"id"))present=true;
  return {{"ok",true},{"status","accepted"},{"id",str(row,"id")},{"present",present}};
 }
 if(created<now-600000||created>now+60000)return {{"ok",true},{"status","uncertain"}};
 // A durable missing-receipt barrier wins against any later arriving submission.
 captureSpace(rows,now);rows[token]={{"created",created},{"protocol",3},{"status","released"}};saveCaptureStore(manager,rows);
 return {{"ok",true},{"status","released"}};
}
}
