#pragma once
#include "ProtocolFallback.hpp"
#include <bcrypt.h>
#include <deque>

namespace udm {
inline bool httpAdmissionRefusal(DWORD code){return code==ERROR_WINHTTP_CANNOT_CONNECT||code==ERROR_WINHTTP_CONNECTION_ERROR;}
inline std::string httpAdmissionDigest(const std::string& bytes){
 unsigned char digest[32]{};
 if(bytes.size()>BrowserRequestMessageLimit||BCryptHash(BCRYPT_SHA256_ALG_HANDLE,nullptr,0,
    (PUCHAR)bytes.data(),(ULONG)bytes.size(),digest,sizeof(digest))<0)
  throw std::runtime_error("Cannot identify HTTP admission context.");
 std::string key;const char* hex="0123456789abcdef";
 for(const auto byte:digest){key+=hex[byte>>4];key+=hex[byte&15];}
 return key;
}
inline std::string httpAdmissionCredential(const std::string& protectedValue){
 auto plaintext=reveal(protectedValue);
 struct Clear {std::string& value;~Clear(){if(!value.empty())SecureZeroMemory(value.data(),value.size());}} clear{plaintext};
 return httpAdmissionDigest(plaintext);
}
// Capacity evidence is private to one origin, resolved route and request identity.
// Automatic Windows routes remain pool-local until their selected route is known.
inline std::string httpAdmissionKey(const Url& url,const Json& route,const Headers& headers,const Json& request=Json::object()){
 Json identity={{"origin",url.origin},{"route",Json::object()}};
 for(const auto* field:{"ProxyMode","Proxy","ProxyBypass","ProxyUser","ProxyAuthMode","ResolvedPacProxyScheme","CapturedProxyDNS","WindowsLoginIntranet","UserAgent","UseHttp3"})
  if(route.contains(field))identity["route"][field]=route[field];
 // DPAPI blobs can differ for the same password. Capacity belongs to the
 // credential identity, not the randomized representation used for storage.
 if(route.contains("ProxySecret"))identity["route"]["ProxySecret"]=httpAdmissionCredential(str(route,"ProxySecret"));
 if(route.contains("ActiveBrowserSession"))identity["browser"]=route["ActiveBrowserSession"];
 for(const auto* field:{"DisableHttp2","DisableHttp3"})identity["policy"][field]=yes(route,field)||yes(request,field);
 const auto mode=str(route,"ProxyMode");
 if(mode!="Connect directly"&&mode!="Use a proxy server"&&!isSocksProxy(route)){
  const auto session=str(route,"HttpAdmissionSession");if(session.empty())return {};
  identity["opaqueRoute"]=session;
 }
 auto serialized=identity.dump();
 std::vector<std::pair<std::string,std::string>> normalized;
 for(const auto& header:headers)normalized.emplace_back(lower(header.first),header.second);
 std::stable_sort(normalized.begin(),normalized.end(),[](const auto& left,const auto& right){return left.first<right.first;});
 for(const auto& header:normalized)for(const auto* field:{&header.first,&header.second}){
  serialized+=std::to_string(field->size());serialized.push_back(':');serialized+=*field;
 }
 return httpAdmissionDigest(serialized);
}

class HttpAdmissionContext {
 friend class HttpAdmissionLease;
 struct Slot {std::weak_ptr<void> transport;ULONGLONG progress=0;bool accepted=false,idle=false,probe=false,multiplexed=false;};
 std::mutex mutex;std::condition_variable changed;
 std::map<uint64_t,Slot> slots;std::deque<uint64_t> waiting;
 uint64_t next=0;size_t limit=0,refusalCapacity=0;unsigned refusals=0;ULONGLONG lastRefusal=0,refusalSince=0,probeAfter=0,recoveryAfter=0;
 static constexpr ULONGLONG EvidenceWindow=20000,ProbeDelay=5000,RecoveryDelay=100,LowerCapacityEvidence=500;
 void prune(){
  for(auto it=slots.begin();it!=slots.end();)if(it->second.idle&&it->second.transport.expired())it=slots.erase(it);else ++it;
 }
 size_t count(bool acceptedOnly=false,bool recentOnly=false,uint64_t except=0)const{
  size_t result=0;std::set<const void*> transports;const auto now=GetTickCount64();
  for(const auto& entry:slots){
   const auto& slot=entry.second;
   if(entry.first==except||(acceptedOnly&&!slot.accepted)||(recentOnly&&(!slot.progress||now-slot.progress>EvidenceWindow)))continue;
   const auto owner=slot.transport.lock();
   if(!slot.multiplexed||!owner||transports.insert(owner.get()).second)++result;
  }
  return result;
 }
 bool reusable(const std::shared_ptr<void>& owner)const{
  return owner&&std::any_of(slots.begin(),slots.end(),[&](const auto& entry){return (entry.second.idle||entry.second.multiplexed)&&entry.second.transport.lock()==owner;});
 }
 bool producer(const std::shared_ptr<void>& owner)const{
  return owner&&std::any_of(slots.begin(),slots.end(),[&](const auto& entry){return entry.second.accepted&&entry.second.transport.lock()==owner;});
 }
 bool probing()const{return std::any_of(slots.begin(),slots.end(),[](const auto& entry){return entry.second.probe;});}
 void abandon(uint64_t id){
  slots.erase(id);
  waiting.erase(std::remove(waiting.begin(),waiting.end(),id),waiting.end());changed.notify_all();
 }
 uint64_t enter(const std::shared_ptr<void>& transport,const Cancel& cancel){
  std::unique_lock<std::mutex> lock(mutex);const auto id=++next;waiting.push_back(id);
  try{for(;;){
   cancel.check();prune();
   auto idle=std::find_if(slots.begin(),slots.end(),[&](const auto& entry){return entry.second.idle&&entry.second.transport.lock()==transport;});
   // An existing session can reuse its own already-open connection. It does
   // not take a new server slot away from a queued independent session.
   if(idle!=slots.end()){
    auto slot=std::move(idle->second);slots.erase(idle);slot.idle=false;slot.probe=false;
    slots.emplace(id,std::move(slot));waiting.erase(std::find(waiting.begin(),waiting.end(),id));return id;
   }
   if(reusable(transport)){
    slots.emplace(id,Slot{transport,0,false,false,false,true});waiting.erase(std::find(waiting.begin(),waiting.end(),id));return id;
   }
   // Existing workers retain the transfer's own split/refusal recovery policy.
   if(producer(transport)){
    slots.emplace(id,Slot{transport});waiting.erase(std::find(waiting.begin(),waiting.end(),id));return id;
   }
   const auto now=GetTickCount64();
   if(waiting.front()==id&&now>=recoveryAfter&&(!limit||count()<limit||(!probing()&&now>=probeAfter))){
    const bool probe=limit&&count()>=limit;
    slots.emplace(id,Slot{transport,0,false,false,probe});waiting.pop_front();
    changed.notify_all();return id;
   }
   changed.wait_for(lock,std::chrono::milliseconds(25));
  }}catch(...){abandon(id);throw;}
 }
 void progress(uint64_t id,bool payload,bool multiplexed=false){
  std::lock_guard<std::mutex> lock(mutex);auto found=slots.find(id);if(found==slots.end())return;
  auto& slot=found->second;const bool newAcceptance=!slot.accepted;
  const auto acceptedBefore=newAcceptance?count(true):0;slot.accepted=true;if(payload)slot.progress=GetTickCount64();
  if(multiplexed){
   const auto owner=slot.transport.lock();
   for(auto& entry:slots)if(owner&&entry.second.transport.lock()==owner)entry.second.multiplexed=true;
  }
  const auto acceptedNow=newAcceptance?count(true):0;
  if(newAcceptance&&acceptedNow>acceptedBefore){refusals=0;refusalCapacity=0;refusalSince=0;recoveryAfter=0;changed.notify_all();}
  if(slot.probe||(limit&&acceptedNow>limit)){slot.probe=false;limit=0;refusals=0;changed.notify_all();}
 }
 bool refused(uint64_t id){
  std::lock_guard<std::mutex> lock(mutex);prune();const auto now=GetTickCount64();
  const auto failed=slots.find(id);
  if(failed!=slots.end()&&(failed->second.multiplexed||producer(failed->second.transport.lock())))return false;
  const auto recent=count(true,true,id),accepted=count(true,false,id);
  if(!accepted)return false;
  if(now-lastRefusal>EvidenceWindow||refusalCapacity!=accepted){refusals=0;refusalCapacity=accepted;refusalSince=now;}
  lastRefusal=now;++refusals;
  // Keep earlier refusals while matching responses are open, but only learn
  // capacity once at least one peer has delivered actual payload bytes.
  if(!limit&&!recent)return false;
  if(!limit&&refusals<2)return false;
  if(limit&&accepted<limit){
   if(!recent)return false;
   // Closing a productive transport can precede the server releasing its
   // slot. Retain the proven bound and pace recovery opens; lowering it needs
   // fresh, sustained refusal evidence at the smaller productive capacity.
   recoveryAfter=now+RecoveryDelay;
   if(refusals<2||now-refusalSince<LowerCapacityEvidence)return true;
  }
  limit=accepted;probeAfter=now+ProbeDelay;return true;
 }
 void leave(uint64_t id,bool drained){
  std::lock_guard<std::mutex> lock(mutex);auto found=slots.find(id);
  if(found!=slots.end()&&(drained||found->second.multiplexed)&&!found->second.probe){found->second.idle=true;}
  else abandon(id);
  changed.notify_all();
 }
public:
 void wait(const Cancel& cancel,const std::weak_ptr<void>& owner={}){
  std::unique_lock<std::mutex> lock(mutex);
  for(;;){
   cancel.check();prune();
   const auto now=GetTickCount64();
   if(reusable(owner.lock())||(now>=recoveryAfter&&(!limit||count()<limit||(!probing()&&now>=probeAfter))))return;
   changed.wait_for(lock,std::chrono::milliseconds(25));
  }
 }
};

class HttpAdmissionLease {
 std::shared_ptr<HttpAdmissionContext> context;uint64_t id=0;bool drained=false;
public:
 HttpAdmissionLease(std::shared_ptr<HttpAdmissionContext> value,const std::shared_ptr<void>& transport,const Cancel& cancel):context(std::move(value)){
  id=context->enter(transport,cancel);
 }
 ~HttpAdmissionLease(){context->leave(id,drained);}
 HttpAdmissionLease(const HttpAdmissionLease&)=delete;HttpAdmissionLease& operator=(const HttpAdmissionLease&)=delete;
 void accepted(bool multiplexed=false){context->progress(id,false,multiplexed);}
 void progress(){context->progress(id,true);}
 void eof(){drained=true;}
 std::shared_ptr<HttpAdmissionContext> refused(){return context->refused(id)?context:std::shared_ptr<HttpAdmissionContext>{};}
};

inline std::unique_ptr<HttpAdmissionLease> httpAdmission(const std::string& key,const std::shared_ptr<void>& transport,const Cancel& cancel){
 if(key.empty())return {};
 struct Entry {std::shared_ptr<HttpAdmissionContext> context;ULONGLONG used;};
 static std::mutex mutex;static std::map<std::string,Entry> entries;
 std::shared_ptr<HttpAdmissionContext> context;
 {
  std::lock_guard<std::mutex> lock(mutex);auto found=entries.find(key);
  if(found==entries.end()){
   if(entries.size()>=256){
    auto oldest=entries.end();
    for(auto it=entries.begin();it!=entries.end();++it)if(it->second.context.use_count()==1&&
       (oldest==entries.end()||it->second.used<oldest->second.used))oldest=it;
    if(oldest==entries.end())return {};
    entries.erase(oldest);
   }
   found=entries.emplace(key,Entry{std::make_shared<HttpAdmissionContext>(),GetTickCount64()}).first;
  }
  found->second.used=GetTickCount64();context=found->second.context;
 }
 return std::make_unique<HttpAdmissionLease>(std::move(context),transport,cancel);
}

struct HttpAdmissionFailure:InternetFailure {
 std::shared_ptr<HttpAdmissionContext> capacity;
 std::weak_ptr<void> transport;
 HttpAdmissionFailure(const InternetFailure& failure,std::shared_ptr<HttpAdmissionContext> context,const std::shared_ptr<void>& owner):InternetFailure(failure),capacity(std::move(context)),transport(owner){}
};
}
