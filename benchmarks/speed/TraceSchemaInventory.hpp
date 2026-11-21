#pragma once
// Diagnostic metadata evidence. Optional filtering reads only the independently
// checked ID prefix; no wait fields or extension bytes are decoded here.
struct TraceSchemaInventory {
 std::map<std::string,Json> schemas;
 Json threadWindowRecords=Json::array();
 uint64_t threadWindowEvents=0,retainedTargetThreadEvents=0,unmatchedTargetThreadEvents=0,unsupportedTargetThreadForms=0;
 bool capped=false;
 bool targetThreadFilter=false;
 explicit TraceSchemaInventory(bool filtered=false):targetThreadFilter(filtered){}
 void consume(EVENT_RECORD* event,bool inWindow){consume(event,inWindow,[](DWORD){return false;});}
 template<class OwnsThread> void consume(EVENT_RECORD* event,bool inWindow,OwnsThread ownsThread){
  const auto& h=event->EventHeader;
  const auto key=guid(h.ProviderId)+":"+std::to_string(h.EventDescriptor.Id)+":"+std::to_string(h.EventDescriptor.Opcode)+":"+std::to_string(h.EventDescriptor.Version)+":"+std::to_string(h.Flags)+":"+std::to_string(event->UserDataLength);
  auto found=schemas.find(key);
  if(found!=schemas.end())found->second["count"]=found->second["count"].get<uint64_t>()+1;
  else if(schemas.size()<64){
   ULONG bytes=0;auto status=TdhGetEventInformation(event,0,nullptr,nullptr,&bytes);
   if(status==ERROR_INSUFFICIENT_BUFFER&&bytes>=sizeof(TRACE_EVENT_INFO)&&bytes<=65536){std::vector<BYTE> data(bytes);status=TdhGetEventInformation(event,0,nullptr,reinterpret_cast<TRACE_EVENT_INFO*>(data.data()),&bytes);}
   schemas.emplace(key,Json{{"provider",guid(h.ProviderId)},{"eventId",h.EventDescriptor.Id},{"opcode",h.EventDescriptor.Opcode},{"version",h.EventDescriptor.Version},{"headerFlags",h.Flags},{"payloadBytes",event->UserDataLength},{"requeryStatus",status},{"metadataBytes",bytes},{"count",1}});
  }else capped=true;
  static const GUID thread={0x3d6fa8d1,0xfe05,0x11d0,{0x9d,0xda,0x00,0xc0,0x4f,0xd7,0xba,0x7c}};
  if(!inWindow||!IsEqualGUID(h.ProviderId,thread)||h.EventDescriptor.Opcode!=36)return;
  ++threadWindowEvents;
  if(targetThreadFilter){
   if(h.EventDescriptor.Id!=0||h.EventDescriptor.Version!=5||h.Flags!=848||event->UserDataLength!=28){++unsupportedTargetThreadForms;return;}
   if(!event->UserData)throw std::runtime_error("Missing context-switch ID prefix");
   DWORD next=0,old=0;std::memcpy(&next,event->UserData,4);std::memcpy(&old,static_cast<const BYTE*>(event->UserData)+4,4);
   if(!ownsThread(next)&&!ownsThread(old)){++unmatchedTargetThreadEvents;return;}
  }
  ++retainedTargetThreadEvents;
  if(threadWindowRecords.size()>=16384||event->UserDataLength>128){capped=true;return;}
  if(event->UserDataLength&&!event->UserData)throw std::runtime_error("Missing unknown event payload");
  std::string hex;const auto* data=static_cast<const BYTE*>(event->UserData);const char digits[]="0123456789abcdef";
  for(USHORT i=0;i<event->UserDataLength;++i){hex.push_back(digits[data[i]>>4]);hex.push_back(digits[data[i]&15]);}
  threadWindowRecords.push_back({{"schemaKey",key},{"counter",h.TimeStamp.QuadPart},{"headerProcess",h.ProcessId},{"headerThread",h.ThreadId},{"payloadHex",hex}});
 }
 Json report()const{Json value={{"schemas",schemas},{"threadWindowEvents",threadWindowEvents},{"threadWindowRecords",threadWindowRecords},{"capped",capped},{"payloadsFullyDecoded",false},{"targetThreadFilter",targetThreadFilter},{"selectedIdPrefixReadForFiltering",targetThreadFilter},{"retainedTargetThreadEvents",retainedTargetThreadEvents},{"unmatchedTargetThreadEvents",unmatchedTargetThreadEvents},{"unsupportedTargetThreadForms",unsupportedTargetThreadForms}};if(!targetThreadFilter)value["payloadsDecoded"]=false;return value;}
};
