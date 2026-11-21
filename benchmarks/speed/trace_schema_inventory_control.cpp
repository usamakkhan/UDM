#define wmain unusedReaderEntry
#include "finish_etl_reader.cpp"
#undef wmain
#include "TraceSchemaInventory.hpp"
int wmain(int argc,wchar_t** argv){try{
 unsigned checks=0;auto require=[&](bool value){++checks;if(!value)throw std::runtime_error("Inventory control failed");};
 EVENT_RECORD event{};event.EventHeader.ProviderId={0x3d6fa8d1,0xfe05,0x11d0,{0x9d,0xda,0x00,0xc0,0x4f,0xd7,0xba,0x7c}};event.EventHeader.EventDescriptor.Opcode=36;event.EventHeader.EventDescriptor.Version=5;event.EventHeader.Flags=EVENT_HEADER_FLAG_CLASSIC_HEADER|EVENT_HEADER_FLAG_64_BIT_HEADER;event.EventHeader.TimeStamp.QuadPart=123;BYTE payload[129]{};payload[0]=0xab;event.UserData=payload;event.UserDataLength=24;
 TraceSchemaInventory inventory;inventory.consume(&event,false);require(inventory.schemas.size()==1&&inventory.threadWindowRecords.empty());
 inventory.consume(&event,true);require(inventory.threadWindowEvents==1&&inventory.threadWindowRecords.size()==1);require(inventory.schemas.begin()->second["count"]==2);require(inventory.threadWindowRecords[0]["payloadHex"].get<std::string>().substr(0,2)=="ab");require(inventory.threadWindowRecords[0]["payloadHex"].get<std::string>().size()==48);
 event.EventHeader.EventDescriptor.Opcode=37;inventory.consume(&event,true);require(inventory.threadWindowEvents==1&&inventory.schemas.size()==2);
 event.EventHeader.EventDescriptor.Opcode=36;event.EventHeader.ProviderId.Data1=0;inventory.consume(&event,true);require(inventory.threadWindowEvents==1);
 event.EventHeader.ProviderId.Data1=0x3d6fa8d1;event.UserDataLength=129;inventory.consume(&event,true);require(inventory.capped&&inventory.threadWindowEvents==2&&inventory.threadWindowRecords.size()==1);
 event.UserDataLength=24;TraceSchemaInventory capacity;for(int i=0;i<16385;++i)capacity.consume(&event,true);require(capacity.capped&&capacity.threadWindowEvents==16385&&capacity.threadWindowRecords.size()==16384);require(capacity.schemas.begin()->second["count"]==16385);
 TraceSchemaInventory keys;for(int i=0;i<65;++i){event.EventHeader.EventDescriptor.Id=(USHORT)i;keys.consume(&event,false);}require(keys.capped&&keys.schemas.size()==64&&keys.threadWindowRecords.empty());
 event.EventHeader.EventDescriptor.Id=0;event.UserData=nullptr;bool threw=false;try{TraceSchemaInventory missing;missing.consume(&event,true);}catch(const std::runtime_error&){threw=true;}require(threw);
 event.EventHeader.EventDescriptor.Id=0;event.EventHeader.EventDescriptor.Opcode=36;event.EventHeader.EventDescriptor.Version=5;event.EventHeader.Flags=848;event.UserData=payload;event.UserDataLength=28;std::memset(payload,0,sizeof(payload));
 auto ids=[&](DWORD next,DWORD old){std::memcpy(payload,&next,4);std::memcpy(payload+4,&old,4);};auto owns=[](DWORD tid){return tid==99;};
 TraceSchemaInventory filtered(true);ids(99,7);filtered.consume(&event,true,owns);require(filtered.threadWindowRecords.size()==1);
 ids(7,99);filtered.consume(&event,true,owns);require(filtered.threadWindowRecords.size()==2);
 ids(7,8);filtered.consume(&event,true,owns);require(filtered.unmatchedTargetThreadEvents==1&&filtered.threadWindowRecords.size()==2);
 filtered.consume(&event,false,owns);require(filtered.threadWindowEvents==3);
 event.EventHeader.EventDescriptor.Version=6;filtered.consume(&event,true,owns);event.EventHeader.EventDescriptor.Version=5;
 event.UserDataLength=24;filtered.consume(&event,true,owns);event.UserDataLength=28;
 event.EventHeader.Flags=320;filtered.consume(&event,true,owns);event.EventHeader.Flags=848;
 require(filtered.unsupportedTargetThreadForms==3&&filtered.threadWindowRecords.size()==2);
 require(filtered.threadWindowEvents==filtered.retainedTargetThreadEvents+filtered.unmatchedTargetThreadEvents+filtered.unsupportedTargetThreadForms);
 TraceSchemaInventory unrelated(true);for(int i=0;i<20000;++i)unrelated.consume(&event,true,owns);require(!unrelated.capped&&unrelated.threadWindowRecords.empty()&&unrelated.unmatchedTargetThreadEvents==20000);
 ids(99,7);TraceSchemaInventory targetCapacity(true);for(int i=0;i<16385;++i)targetCapacity.consume(&event,true,owns);require(targetCapacity.capped&&targetCapacity.retainedTargetThreadEvents==16385&&targetCapacity.threadWindowRecords.size()==16384);
 auto filteredReport=filtered.report();require(filteredReport["targetThreadFilter"]==true&&filteredReport["payloadsFullyDecoded"]==false&&!filteredReport.contains("payloadsDecoded"));
 Json result={{"passed",true},{"checks",checks},{"liveCoverageProven",false},{"kernelSessionStarted",false}};
 if(argc==3){
  size_t used=0;const auto selected=std::stoul(argv[2],&used);require(selected&&used==wcslen(argv[2]));
  const fs::path path(argv[1]);require(fs::file_size(path)<=32*1048576);std::ifstream stream(path,std::ios::binary);Json input;stream>>input;
  const auto& inventory=input.at("unknownSchemaInventory");TraceSchemaInventory replay(true);
  for(const auto& row:inventory.at("threadWindowRecords")){
   const auto& schema=inventory.at("schemas").at(row.at("schemaKey").get<std::string>());
   require(schema["provider"]=="3d6fa8d1-fe05-11d0-9dda-00c04fd7ba7c"&&schema["eventId"]==0&&schema["opcode"]==36&&schema["version"]==5&&schema["headerFlags"]==848&&schema["payloadBytes"]==28);
   auto hex=row.at("payloadHex").get<std::string>();require(hex.size()==56&&hex.find_first_not_of("0123456789abcdef")==std::string::npos);BYTE data[28]{};for(size_t i=0;i<28;++i)data[i]=(BYTE)std::stoul(hex.substr(i*2,2),nullptr,16);
   EVENT_RECORD item{};item.EventHeader.ProviderId={0x3d6fa8d1,0xfe05,0x11d0,{0x9d,0xda,0x00,0xc0,0x4f,0xd7,0xba,0x7c}};item.EventHeader.EventDescriptor.Opcode=36;item.EventHeader.EventDescriptor.Version=5;item.EventHeader.Flags=848;item.EventHeader.TimeStamp.QuadPart=row.at("counter").get<LONGLONG>();item.EventHeader.ProcessId=row.at("headerProcess").get<DWORD>();item.EventHeader.ThreadId=row.at("headerThread").get<DWORD>();item.UserData=data;item.UserDataLength=28;
   replay.consume(&item,true,[&](DWORD tid){return tid==selected;});
  }
  result["replay"]=replay.report();result["selectedThread"]=selected;
 }else require(argc==1);
 std::cout<<result.dump()<<"\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 2;}}
