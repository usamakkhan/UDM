#pragma once
#include "Core.hpp"
#include "PostResponse.hpp"
#include <algorithm>
namespace udm {
inline void clearServerRetry(Json& job){
 job.erase("ServerRetryAfterUtc");job.erase("ServerRetryRequiresReview");job.erase("BrowserVerificationRequired");
}
inline void recordServerRetry(Json& job,const HttpRejected& rejection){
 if(dynamic_cast<const BrowserVerificationRequired*>(&rejection))job["BrowserVerificationRequired"]=true;
 else if(rejection.retryAfterMs>=0&&rejection.retryAfterUntilUtc>parseDate(job.value("ServerRetryAfterUtc",Json())))
  job["ServerRetryAfterUtc"]=date(rejection.retryAfterUntilUtc);
 else if(rejection.retryAfterMs==-2)job["ServerRetryRequiresReview"]=true;
}
inline bool serverRetryReady(const Json& job,i64 now){
 return !yes(job,"ServerRetryRequiresReview")&&parseDate(job.value("ServerRetryAfterUtc",Json()))<=now;
}
inline bool fileInfoRequiresExplicitHttpRetry(const Json& job){
 const auto status=num(job,"LastHttpStatus");
 return yes(job,"BrowserVerificationRequired")||
  (status>=400&&status<500&&status!=408&&status!=425&&status!=429);
}
inline i64 queueRetryNotBefore(const Json& queue,const Json& job,i64 now){
 return std::max(now+num(queue,"RetryDelaySeconds",30)*1000,parseDate(job.value("ServerRetryAfterUtc",Json())));
}
inline bool queueTransferBlocked(const Json& job){
 if(!str(job,"DuplicateOf").empty()||!str(job,"RecycledAt").empty()||yes(job,"ConfirmationPending")
  ||yes(job,"RequiresRequestCapture")||yes(job,"RequiresMediaCapture")||yes(job,"RequiresBrowserProxyCapture")
  ||yes(job,"RequiresBrowserSessionCapture")||yes(job,"SabrSessionRefreshRequired")||yes(job,"AuthenticationPromptPending")||yes(job,"ServerRetryRequiresReview")||yes(job,"BrowserVerificationRequired")
  ||yes(job,"WebPageReceived")||yes(job,"ContentPrefixPending"))return true;
 if(yes(job,"PostAttempted")&&str(job,"ProtectedPostGetUrl").empty()&&!completedPostResponseCandidate(job))return true;
 const auto response=job.find("OriginalResponse");
 return response!=job.end()&&str(*response,"State")!="Published";
}
inline bool queueFailureCanRetry(const Json& queue,const Json& job,bool cycling){
 if(!cycling||!yes(queue,"Enabled",true)||str(job,"Status")!="Failed"||!yes(job,"QueueOrigin")||!yes(job,"QueueMember"))return false;
 if(!str(job,"ProtectedRequest").empty()||queueTransferBlocked(job))return false;
 bool limited=yes(queue,"FileRetryLimitEnabled",true);
 if(limited&&num(job,"QueueAttempts")>=num(queue,"FileRetries"))return false;
 // Old saved queues retain their former permanent-error policy until edited.
 auto code=num(job,"LastHttpStatus");if(code==401||code==407||num(job,"LastFtpStatus")==530)return false;if(!queue.contains("FileRetryLimitEnabled")&&code>=400&&code<500&&code!=408&&code!=425&&code!=429)return false;
 return true;
}
inline bool queueFailureExhausted(const Json& q,const Json& job){
 if(!q.contains("FileRetryLimitEnabled")||!yes(q,"FileRetryLimitEnabled")||!yes(job,"QueueOrigin")||!yes(job,"QueueMember")||num(job,"QueueAttempts")<num(q,"FileRetries"))return false;
 if(queueTransferBlocked(job))return false;
 bool sync=str(job,"Status")=="Complete"&&yes(job,"SyncRetryFailed");auto code=num(job,sync?"SyncLastHttpStatus":"LastHttpStatus");
 if(code==401||code==407||num(job,sync?"SyncLastFtpStatus":"LastFtpStatus")==530)return false;
 return str(job,"Status")=="Failed"||sync;
}
inline bool queueItemFinished(const Json& q,const Json& job){
 return (str(job,"Status")=="Complete"&&!yes(job,"SyncRetryFailed"))||queueFailureExhausted(q,job);
}}
