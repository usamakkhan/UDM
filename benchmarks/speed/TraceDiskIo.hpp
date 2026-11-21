#pragma once
#include <optional>
#include <cstdint>
#include <map>
#include <string>
// Correlate only operations whose initiation was independently attributed to a
// target. Completion header process/thread IDs need not identify the issuer.
struct TargetDiskIo {
 struct Origin {uint64_t counter;DWORD thread;std::wstring opcode;};
 struct Decision {bool retain=false,linked=false;std::optional<Origin> origin;};
 std::map<uint64_t,Origin> pending;
 bool ambiguous=false,limit=false;
 uint64_t unmatchedOwnedCompletions=0;
 Decision observe(bool owned,const std::wstring& op,uint64_t irp,uint64_t time,DWORD thread){
  const bool init=op==L"readinit"||op==L"writeinit"||op==L"flushinit";
  const bool end=op==L"read"||op==L"write"||op==L"flushbuffers";
  auto found=pending.find(irp);
  if(end&&found!=pending.end()){
   const auto origin=found->second;
   const bool matching=(origin.opcode==L"readinit"&&op==L"read")||(origin.opcode==L"writeinit"&&op==L"write")||(origin.opcode==L"flushinit"&&op==L"flushbuffers");
   if(!matching||time<origin.counter){ambiguous=true;return {};}
   pending.erase(found);return {true,true,origin};
  }
  if(!owned)return {};
  if(init){
   if(!irp||found!=pending.end()){ambiguous=true;return {};}
   if(pending.size()>=4096){limit=true;return {};}
   Origin origin{time,thread,op};pending.emplace(irp,origin);return {true,false,origin};
  }
  if(end)++unmatchedOwnedCompletions;
  return {true,false,std::nullopt};
 }
};
