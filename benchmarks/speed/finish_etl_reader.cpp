// Read-only, normal-token consumer for the owned WPR finishing investigation.
// It never starts/stops a logger or executes the captured application.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <evntrace.h>
#include <evntcons.h>
#include <tdh.h>
#include <shellapi.h>
#include <shlobj.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <limits>
#include "TraceDiskIo.hpp"
#include "third_party/json.hpp"
using Json=nlohmann::json;
namespace fs=std::filesystem;

static std::string utf8(const std::wstring& text){
 if(text.empty())return {};int n=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),nullptr,0,nullptr,nullptr);
 if(!n)throw std::runtime_error("Invalid Unicode metadata");std::string out(n,0);
 if(!WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),out.data(),n,nullptr,nullptr))throw std::runtime_error("Unicode conversion failed");return out;
}
static std::wstring lower(std::wstring text){for(auto& c:text)c=static_cast<wchar_t>(towlower(c));return text;}
static std::string guid(const GUID& value){char text[48]{};sprintf_s(text,"%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",value.Data1,value.Data2,value.Data3,value.Data4[0],value.Data4[1],value.Data4[2],value.Data4[3],value.Data4[4],value.Data4[5],value.Data4[6],value.Data4[7]);return text;}
static bool ends(const std::string& value,const std::string& suffix){return value.size()>=suffix.size()&&value.compare(value.size()-suffix.size(),suffix.size(),suffix)==0;}

struct Flush {uint64_t irp=0,begin=0,end=0;DWORD tid=0;uint64_t status=0;std::string relative;};
struct Coverage {
 uint64_t begin=0,end=0,lastDropped=0;unsigned starts=0,ends=0;bool imageMatches=true,droppedMetadata=false,dropDecodeError=false,ambiguous=false,limit=false;
 uint64_t eventsLost=0,buffersLost=0,targetSchemaErrors=0;
 std::map<uint64_t,Flush> pending;std::vector<Flush> completed;
 void start(uint64_t time,bool image){++starts;begin=time;imageMatches=imageMatches&&image;}
 void stop(uint64_t time){++ends;end=time;}
 void flush(uint64_t irp,uint64_t time,DWORD tid,const std::string& relative){
  if(!irp||pending.count(irp)){ambiguous=true;return;}if(pending.size()>=512||completed.size()>=4096){limit=true;return;}
  pending.emplace(irp,Flush{irp,time,0,tid,0,relative});
 }
 void finish(uint64_t irp,uint64_t time,uint64_t status){auto found=pending.find(irp);if(found==pending.end())return;auto row=found->second;pending.erase(found);row.end=time;row.status=status;if(time<row.begin)ambiguous=true;completed.push_back(std::move(row));}
 Json report(uint64_t frequency)const{
  Json rows=Json::array();unsigned dataFlushes=0;bool valid=true;
  for(const auto& row:completed){bool trusted=!row.relative.empty()&&row.end>=row.begin&&row.begin>=begin&&row.end<=end&&row.status==0;
   if(trusted&&endsWithData(row.relative))++dataFlushes;valid=valid&&trusted;
   rows.push_back({{"thread",row.tid},{"beginCounter",row.begin},{"endCounter",row.end},{"ntStatus",row.status},{"relativePath",row.relative},{"pathAndIntervalVerified",trusted},{"wallMs",frequency&&row.end>=row.begin?Json(1000.0*static_cast<double>(row.end-row.begin)/frequency):Json(nullptr)}});
  }
  bool passed=frequency&&starts==1&&ends==1&&begin<end&&imageMatches&&droppedMetadata&&!dropDecodeError&&lastDropped<begin&&!eventsLost&&!buffersLost&&!targetSchemaErrors&&!ambiguous&&!limit&&pending.empty()&&valid&&dataFlushes==2;
  return {{"structuralCoveragePassed",passed},{"processStarts",starts},{"processEnds",ends},{"imageMatches",imageMatches},{"processBeginCounter",begin},{"processEndCounter",end},{"lastDroppedMetadataDecoded",droppedMetadata&&!dropDecodeError},{"latestDroppedCounter",lastDropped},{"overwriteAtOrAfterTargetStart",begin&&lastDropped>=begin},{"ordinaryEventsLost",eventsLost},{"buffersLost",buffersLost},{"targetSchemaErrors",targetSchemaErrors},{"ambiguousIrpOrTime",ambiguous},{"limitReached",limit},{"unmatchedFlushStarts",pending.size()},{"verifiedDataFlushPairs",dataFlushes},{"requiredDataFlushPairs",2},{"flushPairs",rows},{"rootCauseVerified",false}};
 }
 static bool endsWithData(const std::string& path){return ::ends(path,".data");}
};

struct Metadata {std::vector<BYTE> bytes;bool ok=false;};
struct Reader {
 DWORD pid;std::wstring expectedImage,dosRoot,ntRoot;Coverage coverage;
 std::map<std::string,Metadata> schemas;std::map<uint64_t,std::string> files;std::set<DWORD> threads;
 uint64_t eventCount=0,otherSchemaErrors=0,decodedEvents=0,targetHeaderEvents=0;
 std::map<std::string,uint64_t> targetTaskCounts;Json diskEvents=Json::array();TargetDiskIo diskIo;bool callbackFailed=false;
 Reader(DWORD process,std::wstring image,const fs::path& root):pid(process),expectedImage(lower(image)){
  dosRoot=lower(root.wstring());while(!dosRoot.empty()&&(dosRoot.back()==L'\\'||dosRoot.back()==L'/'))dosRoot.pop_back();
  if(dosRoot.size()<3||dosRoot[1]!=L':')throw std::runtime_error("Owned root must use a local drive path");
  wchar_t device[32768]{};const auto drive=dosRoot.substr(0,2);if(!QueryDosDeviceW(drive.c_str(),device,32768))throw std::runtime_error("Cannot resolve owned volume device path");ntRoot=lower(device)+dosRoot.substr(2);
 }
 const TRACE_EVENT_INFO* info(EVENT_RECORD* event){
  const auto& h=event->EventHeader;auto key=guid(h.ProviderId)+":"+std::to_string(h.EventDescriptor.Id)+":"+std::to_string(h.EventDescriptor.Opcode)+":"+std::to_string(h.EventDescriptor.Version)+":"+std::to_string(h.Flags);
  auto it=schemas.find(key);if(it==schemas.end()){
   if(schemas.size()>=512){coverage.limit=true;return nullptr;}Metadata meta;ULONG size=0;auto status=TdhGetEventInformation(event,0,nullptr,nullptr,&size);
   if(status==ERROR_INSUFFICIENT_BUFFER&&size>=sizeof(TRACE_EVENT_INFO)&&size<=65536){meta.bytes.resize(size);status=TdhGetEventInformation(event,0,nullptr,reinterpret_cast<TRACE_EVENT_INFO*>(meta.bytes.data()),&size);meta.ok=status==ERROR_SUCCESS;}
   it=schemas.emplace(key,std::move(meta)).first;
  }
  if(!it->second.ok)return nullptr;return reinterpret_cast<const TRACE_EVENT_INFO*>(it->second.bytes.data());
 }
 std::wstring label(const TRACE_EVENT_INFO* info,ULONG offset)const{
  if(!offset)return {};const Metadata* owner=nullptr;for(const auto& pair:schemas)if(pair.second.bytes.data()==reinterpret_cast<const BYTE*>(info)){owner=&pair.second;break;}
  if(!owner||offset>=owner->bytes.size()||offset%sizeof(wchar_t))throw std::runtime_error("Invalid metadata string offset");
  const auto text=reinterpret_cast<const wchar_t*>(owner->bytes.data()+offset);auto available=(owner->bytes.size()-offset)/sizeof(wchar_t);auto length=wcsnlen_s(text,available);if(length==available)throw std::runtime_error("Unterminated metadata string");return std::wstring(text,length);
 }
 std::vector<BYTE> property(EVENT_RECORD* event,const wchar_t* name){
  PROPERTY_DATA_DESCRIPTOR descriptor{};descriptor.PropertyName=reinterpret_cast<ULONGLONG>(name);descriptor.ArrayIndex=ULONG_MAX;ULONG size=0;
  if(TdhGetPropertySize(event,0,nullptr,1,&descriptor,&size)!=ERROR_SUCCESS||!size||size>65536)return {};
  std::vector<BYTE> bytes(size);if(TdhGetProperty(event,0,nullptr,1,&descriptor,size,bytes.data())!=ERROR_SUCCESS)return {};return bytes;
 }
 uint64_t number(EVENT_RECORD* event,const wchar_t* name){auto raw=property(event,name);uint64_t value=0;if(raw.size()!=1&&raw.size()!=2&&raw.size()!=4&&raw.size()!=8)return 0;memcpy(&value,raw.data(),raw.size());return value;}
 std::wstring text(EVENT_RECORD* event,const wchar_t* name,bool wide){
  auto raw=property(event,name);if(raw.empty())return {};
  if(wide){if(raw.size()%2)return {};auto p=reinterpret_cast<const wchar_t*>(raw.data());auto n=raw.size()/2;return std::wstring(p,wcsnlen_s(p,n));}
  auto n=strnlen_s(reinterpret_cast<const char*>(raw.data()),raw.size());if(!n)return {};int length=MultiByteToWideChar(CP_ACP,MB_ERR_INVALID_CHARS,reinterpret_cast<const char*>(raw.data()),static_cast<int>(n),nullptr,0);if(!length)return {};std::wstring result(length,0);MultiByteToWideChar(CP_ACP,MB_ERR_INVALID_CHARS,reinterpret_cast<const char*>(raw.data()),static_cast<int>(n),result.data(),length);return result;
 }
 std::string owned(std::wstring path)const{
  path=lower(std::move(path));std::replace(path.begin(),path.end(),L'/',L'\\');if(path.rfind(L"\\??\\",0)==0||path.rfind(L"\\\\?\\",0)==0)path.erase(0,4);
  for(const auto* root:{&dosRoot,&ntRoot})if(path.size()>root->size()&&path.compare(0,root->size(),*root)==0&&path[root->size()]==L'\\')return utf8(path.substr(root->size()+1));return {};
 }
 void consume(EVENT_RECORD* event){
  if(++eventCount>2000000){coverage.limit=true;return;}const auto* schema=info(event);if(!schema){if(event->EventHeader.ProcessId==pid)++coverage.targetSchemaErrors;else ++otherSchemaErrors;return;}
  ++decodedEvents;auto task=lower(label(schema,schema->TaskNameOffset));auto op=lower(label(schema,schema->OpcodeNameOffset));uint64_t time=static_cast<uint64_t>(event->EventHeader.TimeStamp.QuadPart);
  if(event->EventHeader.ProcessId==pid){++targetHeaderEvents;auto key=utf8(task)+"/"+utf8(op);if(targetTaskCounts.size()<512||targetTaskCounts.count(key))++targetTaskCounts[key];else coverage.limit=true;}
  if(op==L"lastdroppedtimes"){
   auto countBytes=property(event,L"TimeStampCount");auto count=number(event,L"TimeStampCount");auto raw=property(event,L"TimeStamp");coverage.droppedMetadata=true;
   if(countBytes.size()!=4||count>256||raw.size()!=count*sizeof(uint64_t)){coverage.dropDecodeError=true;return;}
   for(size_t at=0;at<raw.size();at+=8){uint64_t stamp=0;memcpy(&stamp,raw.data()+at,8);coverage.lastDropped=std::max(coverage.lastDropped,stamp);}return;
  }
  if(task==L"process"){
   auto process=number(event,L"ProcessId");if(!process)process=number(event,L"ProcessID");if(process!=pid)return;
   if(op==L"start"){auto image=lower(text(event,L"ImageFileName",false));coverage.start(time,!image.empty()&&fs::path(image).filename().wstring()==expectedImage);}
   if(op==L"end"||op==L"terminate")coverage.stop(time);return;
  }
  if(task==L"thread"&&(op==L"start"||op==L"dcstart")){
   if(number(event,L"ProcessId")==pid){auto tid=number(event,L"TThreadId");if(!tid)tid=number(event,L"ThreadId");if(tid&&tid<=MAXDWORD){if(threads.size()>=512)coverage.limit=true;else threads.insert(static_cast<DWORD>(tid));}}return;
  }
  if(task==L"fileio"){
   auto object=number(event,L"FileObject");auto path=text(event,L"OpenPath",true);if(path.empty())path=text(event,L"FileName",true);
   if(!path.empty()&&object){auto relative=owned(path);if(relative.empty())files.erase(object);else if(files.size()<4096)files[object]=relative;else coverage.limit=true;}
   auto tid=number(event,L"TTID");bool target=event->EventHeader.ProcessId==pid||(tid&&threads.count(static_cast<DWORD>(tid)));
   if(op==L"flush"&&target){auto found=files.find(object);coverage.flush(number(event,L"IrpPtr"),time,tid?static_cast<DWORD>(tid):event->EventHeader.ThreadId,found==files.end()?"":found->second);}
   if(op==L"operationend"){
    auto irp=number(event,L"IrpPtr");auto status=property(event,L"NtStatus");
    if(coverage.pending.count(irp)&&status.size()!=4)++coverage.targetSchemaErrors;
    else coverage.finish(irp,time,number(event,L"NtStatus"));
   }
   if(op==L"close"&&object)files.erase(object);return;
  }
  if(task==L"diskio"){
   auto object=number(event,L"FileObject"),tid=number(event,L"IssuingThreadId");auto found=files.find(object);
   const bool knownIssuer=tid&&tid<=MAXDWORD&&threads.count((DWORD)tid),knownHeaderThread=threads.count(event->EventHeader.ThreadId)!=0;
   const bool owner=found!=files.end()||event->EventHeader.ProcessId==pid||knownIssuer||knownHeaderThread;
   auto irp=number(event,L"Irp");if(!irp)irp=number(event,L"IrpPtr");
   const DWORD sourceThread=knownIssuer?(DWORD)tid:(knownHeaderThread?event->EventHeader.ThreadId:0);
   auto decision=diskIo.observe(owner,op,irp,time,sourceThread);
   if(diskIo.ambiguous)coverage.ambiguous=true;if(diskIo.limit)coverage.limit=true;
   if(!decision.retain)return;
   if(diskEvents.size()>=16384){coverage.limit=true;return;}
   Json row={{"opcode",utf8(op)},{"counter",time},{"thread",event->EventHeader.ThreadId},{"issuingThread",tid},{"relativePath",found==files.end()?"":found->second},{"linkedToTargetInit",decision.linked}};
   if(decision.origin){row["targetInitCounter"]=decision.origin->counter;row["targetInitThread"]=decision.origin->thread;row["targetInitOpcode"]=utf8(decision.origin->opcode);}
   for(const auto* key:{L"Irp",L"IrpPtr",L"DiskNumber",L"TransferSize",L"ByteOffset",L"HighResResponseTime",L"ResponseTime"}){auto value=property(event,key);if(!value.empty()&&value.size()<=8){uint64_t number=0;memcpy(&number,value.data(),value.size());row[utf8(key)]=number;}}
   diskEvents.push_back(std::move(row));
  }
 }
 static void WINAPI eventCallback(EVENT_RECORD* event){auto* reader=static_cast<Reader*>(event->UserContext);try{reader->consume(event);}catch(...){reader->callbackFailed=true;}}
};

static int selfTest(){
 Json checks=Json::array();auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 auto complete=[](){Coverage c;c.start(100,true);c.stop(300);c.droppedMetadata=true;c.flush(1,110,7,"parts/body.data");c.finish(1,150,0);c.flush(2,200,7,"parts/body.data");c.finish(2,250,0);return c;};
 auto good=[](const Coverage& c){return c.report(1000)["structuralCoveragePassed"].get<bool>();};
 auto c=complete();check(good(c),"Complete normalized lifetime and two owned data-flush pairs pass structural coverage");
 c=complete();c.starts=0;check(!good(c),"Missing target process start rejects partial history");
 c=complete();c.ends=0;check(!good(c),"Missing target process end rejects partial history");
 c=complete();c.starts=2;check(!good(c),"Repeated PID lifetimes cannot be silently combined");
 c=complete();c.imageMatches=false;check(!good(c),"Wrong image identity rejects the target lifetime");
 c=complete();c.eventsLost=1;check(!good(c),"Ordinary lost events reject structural coverage");
 c=complete();c.buffersLost=1;check(!good(c),"Lost buffers reject structural coverage");
 c=complete();c.lastDropped=50;check(good(c),"Older overwritten history may precede a fully retained target interval");
 c=complete();c.lastDropped=120;check(!good(c),"Circular overwrite intersecting target history rejects coverage");
 c=complete();c.droppedMetadata=false;check(!good(c),"Missing memory-history metadata is not treated as zero loss");
 c=complete();c.dropDecodeError=true;check(!good(c),"Incomplete overwrite arrays reject coverage");
 c=complete();c.flush(3,260,7,"parts/body.data");check(!good(c),"Unfinished flush starts reject coverage");
 c=complete();c.flush(3,260,7,"parts/body.data");c.flush(3,261,7,"parts/body.data");c.finish(3,280,0);check(!good(c),"Reused outstanding IRP identities reject ambiguous pairing");
 c=complete();c.completed[0].status=1;check(!good(c),"Failed IO completion cannot be labeled a confirmed flush");
 c=complete();c.completed[0].relative.clear();check(!good(c),"Missing contemporaneous file identity rejects attribution");
 c=complete();c.completed[0].end=99;check(!good(c),"Reversed timestamps reject timing claims");
 c=complete();c.completed.pop_back();check(!good(c),"One data-flush pair cannot prove the controlled split scenario");
 c=complete();c.flush(3,260,7,"parts/body.data");c.finish(3,280,0);check(!good(c),"Unexpected extra data-flush pairs require scenario review");
 c=complete();c.limit=true;check(!good(c),"Capacity exhaustion cannot silently truncate a passing report");
 c=complete();c.targetSchemaErrors=1;check(!good(c),"Target schema failures reject coverage");
 std::cout<<Json{{"passed",true},{"checks",checks},{"method","Synthetic normalized-event safety checks; not real ETL/TDH integration validation"},{"realTraceConsumed",false}}.dump(2)<<"\n";return 0;
}

int wmain(int argc,wchar_t** argv){
 try{
  if(argc==2&&std::wstring(argv[1])==L"--self-test")return selfTest();
  if(argc!=5&&argc!=6)throw std::runtime_error("Usage: reader input.etl output.json target-pid owned-root [expected-image]");
  if(IsUserAnAdmin())throw std::runtime_error("Run the read-only consumer unelevated");
  fs::path input=argv[1],output=argv[2],root=argv[4];if(!input.is_absolute()||!output.is_absolute()||!root.is_absolute()||!fs::is_regular_file(input)||!fs::is_directory(root)||fs::exists(output))throw std::runtime_error("Require an existing absolute ETL/root and a fresh absolute output file");
  if(fs::file_size(input)>256*1048576ULL)throw std::runtime_error("ETL input exceeds the 256 MiB offline-reader limit");
  size_t used=0;auto process=std::stoull(argv[3],&used);if(!process||process>MAXDWORD||used!=wcslen(argv[3]))throw std::runtime_error("Invalid target PID");
  auto parent=fs::canonical(output.parent_path());wchar_t local[32768]{};auto localLength=GetEnvironmentVariableW(L"LOCALAPPDATA",local,32768);if(!localLength||localLength>=32768)throw std::runtime_error("Missing private development root");auto privateRoot=fs::canonical(fs::path(local)/L"UDM-Development");auto relative=parent.lexically_relative(privateRoot);if(relative.empty()||relative.is_absolute()||*relative.begin()==L"..")throw std::runtime_error("Output must remain inside private UDM-Development");
  Reader reader(static_cast<DWORD>(process),argc==6?argv[5]:L"UDM.exe",root);EVENT_TRACE_LOGFILEW log{};auto file=input.wstring();log.LogFileName=file.data();log.ProcessTraceMode=PROCESS_TRACE_MODE_EVENT_RECORD|PROCESS_TRACE_MODE_RAW_TIMESTAMP;log.EventRecordCallback=&Reader::eventCallback;log.Context=&reader;
  auto handle=OpenTraceW(&log);if(handle==INVALID_PROCESSTRACE_HANDLE)throw std::runtime_error("OpenTrace failed: "+std::to_string(GetLastError()));
  struct TraceHandle {PROCESSTRACE_HANDLE value;~TraceHandle(){CloseTrace(value);}} owner{handle};if(log.LogfileHeader.ReservedFlags!=1||log.LogfileHeader.PerfFreq.QuadPart<=0)throw std::runtime_error("Only explicitly identified QPC-clock traces are supported");
  auto status=ProcessTrace(&handle,1,nullptr,nullptr);reader.coverage.eventsLost=log.LogfileHeader.EventsLost+static_cast<uint64_t>(log.EventsLost);reader.coverage.buffersLost=log.LogfileHeader.BuffersLost;if(status!=ERROR_SUCCESS||reader.callbackFailed)reader.coverage.ambiguous=true;
  Json report=reader.coverage.report(static_cast<uint64_t>(log.LogfileHeader.PerfFreq.QuadPart));report["schema"]=1;report["processedEvents"]=reader.eventCount;report["metadataDecodedEvents"]=reader.decodedEvents;report["targetHeaderEvents"]=reader.targetHeaderEvents;report["targetTaskCounts"]=reader.targetTaskCounts;report["processTraceStatus"]=status;report["callbackFailed"]=reader.callbackFailed;report["otherSchemaErrors"]=reader.otherSchemaErrors;report["clockType"]=log.LogfileHeader.ReservedFlags;report["counterFrequency"]=log.LogfileHeader.PerfFreq.QuadPart;report["relatedDiskEvents"]=reader.diskEvents;report["limits"]=Json::array({"Coverage is a necessary gate, not a disk/filter/scheduler root-cause conclusion.","Only the declared PID/image and contemporaneously mapped owned file paths are associated. Unknown identities reject flush coverage.","Disk response-time fields are raw provider values; no undocumented time-unit conversion or parent-IRP relationship is inferred.","Requires a complete target lifetime, decoded memory-overwrite metadata and two successful owned .data flush pairs for this controlled ordinary split scenario.","The kernel TDH adapter requires live ETL validation; normalized self-tests or a process-local fixture alone do not prove kernel integration."});
  auto text=report.dump(2)+"\n";if(text.size()>32*1048576)throw std::runtime_error("Bounded report size exceeded");HANDLE out=CreateFileW(output.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);if(out==INVALID_HANDLE_VALUE)throw std::runtime_error("Fresh private report could not be created");DWORD count=0;bool ok=WriteFile(out,text.data(),static_cast<DWORD>(text.size()),&count,nullptr)&&count==text.size();CloseHandle(out);if(!ok)throw std::runtime_error("Report write failed");std::cout<<Json{{"processedEvents",reader.eventCount},{"structuralCoveragePassed",report["structuralCoveragePassed"]},{"rootCauseVerified",false}}.dump()<<"\n";return report["structuralCoveragePassed"].get<bool>()?0:1;
 }catch(const std::exception& e){std::cerr<<Json{{"error",e.what()},{"rootCauseVerified",false}}.dump()<<"\n";return 2;}
}
