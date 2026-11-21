// Normal-token metadata lookup controls, not live event/decoder qualification.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <evntrace.h>
#include <evntcons.h>
#include <tdh.h>
#include <shlobj.h>
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include "third_party/json.hpp"
using Json=nlohmann::json;
static std::string label(const std::vector<BYTE>& bytes,ULONG offset){
 if(!offset)return {};if(offset>=bytes.size()||offset%sizeof(wchar_t))throw std::runtime_error("Bad metadata offset");
 const auto p=reinterpret_cast<const wchar_t*>(bytes.data()+offset);const auto capacity=(bytes.size()-offset)/sizeof(wchar_t);const auto length=wcsnlen_s(p,capacity);if(length==capacity)throw std::runtime_error("Bad metadata string");
 std::string result;for(size_t i=0;i<length;++i){if(p[i]>127)throw std::runtime_error("Unexpected non-ASCII schema label");result.push_back((char)p[i]);}return result;
}
static uint64_t field(EVENT_RECORD* event,const wchar_t* name,ULONG expected){
 PROPERTY_DATA_DESCRIPTOR descriptor{};descriptor.PropertyName=(ULONGLONG)name;descriptor.ArrayIndex=ULONG_MAX;ULONG size=0;
 auto status=TdhGetPropertySize(event,0,nullptr,1,&descriptor,&size);if(status!=ERROR_SUCCESS||size!=expected||size>8)throw std::runtime_error("Unexpected property size/status");
 BYTE bytes[8]{};status=TdhGetProperty(event,0,nullptr,1,&descriptor,size,bytes);if(status!=ERROR_SUCCESS)throw std::runtime_error("Property read failed");uint64_t value=0;std::memcpy(&value,bytes,size);return value;
}
int main(){try{
 if(IsUserAnAdmin())return 77;
 const GUID provider={0x3d6fa8d1,0xfe05,0x11d0,{0x9d,0xda,0x00,0xc0,0x4f,0xd7,0xba,0x7c}};
 Json rows=Json::array();BYTE payload[24]{};
 for(BYTE version:{BYTE(1),BYTE(2),BYTE(3),BYTE(4),BYTE(5)})
 for(USHORT architecture:{USHORT(0),USHORT(EVENT_HEADER_FLAG_32_BIT_HEADER),USHORT(EVENT_HEADER_FLAG_64_BIT_HEADER)}){
  EVENT_RECORD event{};event.EventHeader.Size=sizeof(EVENT_HEADER)+sizeof(payload);event.EventHeader.Flags=EVENT_HEADER_FLAG_CLASSIC_HEADER|architecture;event.EventHeader.ProviderId=provider;event.EventHeader.EventDescriptor.Version=version;event.EventHeader.EventDescriptor.Opcode=36;event.UserData=payload;event.UserDataLength=sizeof(payload);
  ULONG size=0;auto status=TdhGetEventInformation(&event,0,nullptr,nullptr,&size);Json row={{"version",version},{"headerFlags",event.EventHeader.Flags},{"payloadBytes",sizeof(payload)},{"firstStatus",status},{"metadataBytes",size}};
  if(status==ERROR_INSUFFICIENT_BUFFER&&size>=sizeof(TRACE_EVENT_INFO)&&size<=65536){std::vector<BYTE> bytes(size);status=TdhGetEventInformation(&event,0,nullptr,reinterpret_cast<TRACE_EVENT_INFO*>(bytes.data()),&size);if(status==ERROR_SUCCESS){const auto* info=reinterpret_cast<const TRACE_EVENT_INFO*>(bytes.data());row["task"]=label(bytes,info->TaskNameOffset);row["opcode"]=label(bytes,info->OpcodeNameOffset);}}
  row["status"]=status;rows.push_back(std::move(row));
 }
 Json controls=Json::array();unsigned fieldsVerified=0;
 for(BYTE version:{BYTE(2),BYTE(3),BYTE(4)})for(USHORT length:{USHORT(24),USHORT(28)})for(unsigned pattern:{0u,1u}){
  BYTE data[28]{};const uint32_t next=pattern?0x87654321:0x12345678,old=pattern?0x10203040:0x76543210;const BYTE reason=pattern?0:37,state=pattern?1:5;
  std::memcpy(data,&next,4);std::memcpy(data+4,&old,4);data[12]=reason;data[14]=state;std::memset(data+24,0xa5,4);
  EVENT_RECORD event{};event.EventHeader.Size=sizeof(EVENT_HEADER)+length;event.EventHeader.Flags=EVENT_HEADER_FLAG_CLASSIC_HEADER|EVENT_HEADER_FLAG_64_BIT_HEADER|EVENT_HEADER_FLAG_PROCESSOR_INDEX|EVENT_HEADER_FLAG_NO_CPUTIME;event.EventHeader.ProviderId=provider;event.EventHeader.EventDescriptor.Version=version;event.EventHeader.EventDescriptor.Opcode=36;event.UserData=data;event.UserDataLength=length;
  auto n=field(&event,L"NewThreadId",4),o=field(&event,L"OldThreadId",4),r=field(&event,L"OldThreadWaitReason",1),s=field(&event,L"OldThreadState",1);if(n!=next||o!=old||r!=reason||s!=state)throw std::runtime_error("TDH prefix mismatch");fieldsVerified+=4;
  controls.push_back({{"version",version},{"headerFlags",event.EventHeader.Flags},{"payloadBytes",length},{"pattern",pattern},{"newThread",n},{"oldThread",o},{"oldWaitReason",r},{"oldState",s},{"passed",true}});
 }
 std::cout<<Json{{"passed",true},{"elevated",false},{"syntheticMetadataQueriesOnly",true},{"liveCoverageProven",false},{"provider","3d6fa8d1-fe05-11d0-9dda-00c04fd7ba7c"},{"opcode",36},{"queries",rows},{"knownSchemaPrefixControls",controls},{"prefixFieldsVerified",fieldsVerified},{"extraTrailingBytesInterpreted",false}}.dump(2)<<"\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 2;}}
