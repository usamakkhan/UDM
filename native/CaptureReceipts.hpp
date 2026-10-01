#pragma once
#include "Core.hpp"
#include <regex>
namespace udm {
// Persist only opaque handoff identity and the saved job ID, never request credentials.
inline i64 captureTime(const std::string& token){
 if(!std::regex_match(token,std::regex("[0-9]{13}-[a-f0-9]{8}-[a-f0-9]{4}-[a-f0-9]{4}-[a-f0-9]{4}-[a-f0-9]{12}")))
  throw std::runtime_error("Invalid browser capture token.");
 return std::stoll(token.substr(0,13));
}
inline Json captureStore(const Manager& m){
 auto rows=m.state.value("BrowserCaptures",Json::object());
 if(!rows.is_object()||rows.size()>2048)throw std::runtime_error("Invalid browser capture journal.");
 return rows;
}
inline void saveCaptureStore(Manager& m,const Json& rows){
 const bool existed=m.state.contains("BrowserCaptures");auto previous=m.state.value("BrowserCaptures",Json());
 m.state["BrowserCaptures"]=rows;
 try{m.save();}catch(...){if(existed)m.state["BrowserCaptures"]=previous;else m.state.erase("BrowserCaptures");throw;}
}
inline void captureSpace(Json& rows,i64 now){
 for(auto it=rows.begin();it!=rows.end();)if(num(it.value(),"created")<now-86400000&&!it.value().contains("presentation")&&!(num(it.value(),"protocol")==2&&(str(it.value(),"status")=="prepared"||str(it.value(),"status")=="pending"||str(it.value(),"status")=="review")))it=rows.erase(it);else ++it;
 if(rows.size()>=2048)throw std::runtime_error("Browser capture journal is full. Leave this download in the browser.");
}
inline Json reconcileCapture(Manager& m,const std::string& token){
 Lock lock(m.mutex);auto created=captureTime(token),now=epoch();auto rows=captureStore(m);
 if(rows.contains(token)){
  auto row=rows[token];auto status=str(row,"status");
  if(status=="accepted"&&!str(row,"id").empty())return {{"ok",true},{"status","accepted"},{"id",str(row,"id")}};
  if(status=="released")return {{"ok",true},{"status","released"}};
  return {{"ok",true},{"status","uncertain"}};
 }
 // A missing, old receipt cannot prove that no job was saved.
 if(created<now-600000||created>now+60000)return {{"ok",true},{"status","uncertain"}};
 // This durable barrier also rejects an Add that arrives after reconciliation.
 captureSpace(rows,now);rows[token]={{"created",created},{"status","released"}};saveCaptureStore(m,rows);
 return {{"ok",true},{"status","released"}};
}
// An explicit legacy recovery decision closes the old Add path before the
// browser copy can resume. This never deletes or changes a possible saved job.
inline Json releaseLegacyCapture(Manager& m,const Json& message){
 if(!yes(message,"confirmed"))throw std::runtime_error("Review the interrupted download before choosing its owner.");
 Lock lock(m.mutex);auto token=str(message,"captureToken");auto created=captureTime(token),now=epoch();
 if(created>now+60000)throw std::runtime_error("Invalid browser capture time.");
 auto rows=captureStore(m);
 if(rows.contains(token)){
  const auto row=rows[token];auto status=str(row,"status");
  if(num(row,"protocol")==2)throw std::runtime_error("Use the saved captured-link review for this download.");
  if(status=="accepted"&&!str(row,"id").empty())return {{"ok",true},{"status","accepted"},{"id",str(row,"id")}};
  if(status=="released")return {{"ok",true},{"status","released"}};
  if(status!="pending")throw std::runtime_error("This capture cannot be resolved through legacy recovery.");
 }else captureSpace(rows,now);
 rows[token]={{"created",created},{"status","released"},{"manualReview",true}};
 saveCaptureStore(m,rows);return {{"ok",true},{"status","released"}};
}
inline JobPtr receiveCapture(Manager& m,const Json& message){
 Lock lock(m.mutex);auto token=str(message,"captureToken");auto created=captureTime(token),now=epoch();auto rows=captureStore(m);
 if(rows.contains(token)){
  const auto row=rows[token];if(str(row,"status")=="accepted"){
   for(auto job:m.jobs)if(job->id()==str(row,"id"))return job;
   throw std::runtime_error("UDM already accepted this capture; its history record was later removed.");
  }
  throw std::runtime_error("This browser capture was released or needs reconciliation. Do not resubmit it.");
 }
 if(created<now-600000||created>now+60000)throw std::runtime_error("Browser capture token has expired.");
 captureSpace(rows,now);rows[token]={{"created",created},{"status","pending"}};saveCaptureStore(m,rows);
 auto request=message;request.erase("captureToken");
 // A process or disk failure after reservation stays uncertain, never a guessed replay.
 auto job=m.receive(request);
 rows=captureStore(m);rows[token]={{"created",created},{"status","accepted"},{"id",job->id()}};saveCaptureStore(m,rows);
 try{m.rememberAutomaticCapture(job,token,str(request,"url"));}catch(...){} // Receipt is already accepted.
 return job;
}
}
