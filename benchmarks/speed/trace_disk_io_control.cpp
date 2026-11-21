#define wmain unusedDiskReaderEntry
#include "finish_etl_reader.cpp"
#undef wmain
int main(){try{
 unsigned checks=0;auto require=[&](bool value){++checks;if(!value)throw std::runtime_error("Disk correlation control failed");};
 TargetDiskIo tracker;auto init=tracker.observe(true,L"flushinit",1,10,99);require(init.retain&&!init.linked&&tracker.pending.size()==1);
 auto end=tracker.observe(false,L"flushbuffers",1,20,0);require(end.retain&&end.linked&&end.origin->thread==99&&end.origin->counter==10&&tracker.pending.empty());
 require(!tracker.observe(false,L"flushbuffers",2,20,0).retain);require(!tracker.observe(false,L"flushinit",2,20,0).retain&&tracker.pending.empty());
 require(tracker.observe(true,L"read",2,30,99).retain&&tracker.unmatchedOwnedCompletions==1);
 tracker.observe(true,L"writeinit",1,40,99);auto reused=tracker.observe(false,L"write",1,50,0);require(reused.linked&&reused.origin->counter==40&&tracker.pending.empty());
 for(const auto& pair:{std::make_pair(L"readinit",L"read"),std::make_pair(L"writeinit",L"write"),std::make_pair(L"flushinit",L"flushbuffers")}){TargetDiskIo each;each.observe(true,pair.first,1,1,7);require(each.observe(false,pair.second,1,2,0).linked&&!each.ambiguous);}
 TargetDiskIo wrong;wrong.observe(true,L"flushinit",1,10,7);require(!wrong.observe(false,L"write",1,20,0).retain&&wrong.ambiguous&&wrong.pending.size()==1);
 TargetDiskIo early;early.observe(true,L"flushinit",1,10,7);require(!early.observe(false,L"flushbuffers",1,9,0).retain&&early.ambiguous);
 TargetDiskIo duplicate;duplicate.observe(true,L"flushinit",1,10,7);require(!duplicate.observe(true,L"flushinit",1,11,7).retain&&duplicate.ambiguous&&duplicate.pending.size()==1);
 TargetDiskIo zero;require(!zero.observe(true,L"flushinit",0,10,7).retain&&zero.ambiguous);
 TargetDiskIo capacity;for(uint64_t i=1;i<=4097;++i)capacity.observe(true,L"readinit",i,i,7);require(capacity.limit&&capacity.pending.size()==4096);
 TargetDiskIo foreign;for(uint64_t i=1;i<=10000;++i)foreign.observe(false,L"writeinit",i,i,0);require(!foreign.limit&&foreign.pending.empty());
 for(BYTE version:{BYTE(2),BYTE(3)}){
  Reader reader(GetCurrentProcessId(),L"synthetic",fs::current_path());reader.threads.insert(99);
  const GUID provider={0x3d6fa8d4,0xfe05,0x11d0,{0x9d,0xda,0x00,0xc0,0x4f,0xd7,0xba,0x7c}};
  const uint64_t irp=0x1122334455667788ULL;BYTE beginData[12]{},endData[28]{};std::memcpy(beginData,&irp,8);std::memcpy(endData+16,&irp,8);DWORD disk=7,missing=MAXDWORD;std::memcpy(endData,&disk,4);std::memcpy(beginData+8,&missing,4);std::memcpy(endData+24,&missing,4);
  EVENT_RECORD begin{};begin.EventHeader.ProviderId=provider;begin.EventHeader.Flags=EVENT_HEADER_FLAG_CLASSIC_HEADER|EVENT_HEADER_FLAG_64_BIT_HEADER;begin.EventHeader.EventDescriptor.Version=version;begin.EventHeader.EventDescriptor.Opcode=15;begin.EventHeader.ProcessId=reader.pid;begin.EventHeader.ThreadId=99;begin.EventHeader.TimeStamp.QuadPart=10;begin.UserData=beginData;begin.UserDataLength=version==2?8:12;
  require(reader.info(&begin)!=nullptr&&reader.number(&begin,L"Irp")==irp);reader.consume(&begin);require(reader.diskEvents.size()==1&&reader.diskIo.pending.size()==1);
  EVENT_RECORD complete=begin;complete.EventHeader.EventDescriptor.Opcode=14;complete.EventHeader.ProcessId=MAXDWORD;complete.EventHeader.ThreadId=MAXDWORD;complete.EventHeader.TimeStamp.QuadPart=20;complete.UserData=endData;complete.UserDataLength=version==2?24:28;
  require(reader.info(&complete)!=nullptr&&reader.number(&complete,L"Irp")==irp&&reader.number(&complete,L"IssuingThreadId")== (version==2?0:MAXDWORD));
  reader.consume(&complete);require(reader.diskEvents.size()==2&&reader.diskEvents[1]["opcode"]=="flushbuffers"&&reader.diskEvents[1]["linkedToTargetInit"]==true);
  require(reader.diskEvents[1]["targetInitThread"]==99&&reader.diskEvents[1]["targetInitCounter"]==10&&reader.diskIo.pending.empty()&&!reader.coverage.ambiguous);
 }
 std::cout<<nlohmann::json{{"passed",true},{"checks",checks},{"liveCoverageProven",false},{"kernelSessionStarted",false}}.dump()<<"\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 2;}}
