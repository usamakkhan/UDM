#pragma once
#include "CaptureReceipts.hpp"
#include "BrowserRequest.hpp"
#include "BrowserProxy.hpp"
namespace udm {
inline Json preparedCaptureEnvelope(const Json& receipt){
 auto stored=Json::parse(reveal(str(receipt,"request")));
 // Earlier unshipped protocol-2 preparations stored only the download object.
 return num(receipt,"envelope")==1?stored:Json{{"download",stored},{"context",Json::object()}};
}
// A prepared request is protected at rest and has no job, UI or transfer effects.
// Only the browser may commit after acknowledging cancellation of its response.
inline Json captureTransactionStatus(Manager& m,const std::string& token){
 Lock lock(m.mutex);captureTime(token);auto rows=captureStore(m);
 if(!rows.contains(token))return reconcileCapture(m,token);
 const auto& row=rows[token];auto status=str(row,"status");
 if(status=="accepted")return {{"ok",true},{"status","accepted"},{"id",str(row,"id")}};
 if(status=="released")return {{"ok",true},{"status","released"}};
 if(num(row,"protocol")==2&&(status=="review"||status=="discarded"))return {{"ok",true},{"status",status}};
 if(num(row,"protocol")==2&&status=="prepared")return {{"ok",true},{"status","prepared"}};
 return {{"ok",true},{"status","uncertain"}};
}
inline Json prepareCapture(Manager& m,const Json& message){
 Lock lock(m.mutex);auto token=str(message,"captureToken");auto created=captureTime(token),now=epoch();
 auto request=message.at("download");
 if(!request.is_object()||str(request,"action")!="add"||request.contains("captureToken"))throw std::runtime_error("Only ordinary browser downloads can be prepared.");
 auto serialized=request.dump();if(serialized.size()>BrowserRequestMessageLimit)throw std::runtime_error("Prepared browser request exceeds the size limit.");
 auto address=str(request,"url");Url url(address);if(url.scheme!="http"&&url.scheme!="https")throw std::runtime_error("Only HTTP and HTTPS downloads can be prepared.");
 auto headers=browserHeaders(request);validatePostRequest(request.value("request",Json::object()),address);
 validateBrowserProxy(request.value("browserProxy",Json::object()),address);
 browserDownloadSession(request.value("browserSession",Json::object()),address,headers);
 auto rows=captureStore(m);
 if(rows.contains(token)){
  const auto& row=rows[token];if(num(row,"protocol")==2&&str(row,"status")=="prepared"){
   if(preparedCaptureEnvelope(row).at("download").dump()!=serialized)throw std::runtime_error("The prepared browser request changed. Do not resubmit it.");
   return {{"ok",true},{"status","prepared"}};
  }
  return captureTransactionStatus(m,token);
 }
 if(created<now-600000||created>now+60000)throw std::runtime_error("Browser capture token has expired.");
 captureSpace(rows,now);auto encrypted=protect(Json{{"download",request},{"context",m.browserCaptureContext(request)}}.dump());size_t bytes=encrypted.size(),pending=1;
 for(auto it=rows.begin();it!=rows.end();++it)if(num(it.value(),"protocol")==2&&!str(it.value(),"request").empty()){bytes+=str(it.value(),"request").size();++pending;}
 // Two maximum-size POST submissions can coexist while the catalog's 32 MiB
 // durable size check remains the final admission boundary.
 if(bytes>16*1024*1024||pending>128)throw std::runtime_error("Prepared browser downloads need recovery before more can be captured.");
 rows[token]={{"created",created},{"protocol",2},{"envelope",1},{"status","prepared"},{"request",encrypted}};saveCaptureStore(m,rows);
 return {{"ok",true},{"status","prepared"}};
}
inline Json releasePreparedCapture(Manager& m,const std::string& token){
 Lock lock(m.mutex);auto status=captureTransactionStatus(m,token);
 if(str(status,"status")!="prepared")return status;
 auto rows=captureStore(m);auto created=num(rows[token],"created");
 rows[token]={{"created",created},{"protocol",2},{"status","released"}};saveCaptureStore(m,rows);
 return {{"ok",true},{"status","released"}};
}
inline Json commitPreparedCapture(Manager& m,const std::string& token){
 return m.commitBrowserCapture(token);
}
}
