#define NOMINMAX
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <chrono>
#include <algorithm>
#include <vector>
#include <string>
#include "../../native/third_party/json.hpp"
namespace fs=std::filesystem;using Json=nlohmann::json;using Clock=std::chrono::steady_clock;
static double elapsed(Clock::time_point start){return std::chrono::duration<double,std::milli>(Clock::now()-start).count();}
struct File {HANDLE h=INVALID_HANDLE_VALUE;~File(){if(h!=INVALID_HANDLE_VALUE)CloseHandle(h);}void close(){if(!CloseHandle(h))throw std::runtime_error("Close failed: "+std::to_string(GetLastError()));h=INVALID_HANDLE_VALUE;}};
struct Memory {void* p=nullptr;~Memory(){if(p)VirtualFree(p,0,MEM_RELEASE);}};
static void require(bool ok,const char* message){if(!ok)throw std::runtime_error(std::string(message)+": "+std::to_string(GetLastError()));}
static Json run(const fs::path& root,const std::string& mode,size_t requested,int sequence){
 Json row={{"mode",mode},{"requestedBytes",requested},{"sequence",sequence}};auto temp=root/(std::to_wstring(sequence)+L".tmp"),target=root/(std::to_wstring(sequence)+L".json");Json value={{"kind","owned-intent-probe"},{"sequence",sequence},{"data",""}};value["data"]=std::string(requested-value.dump().size(),'x');const auto bytes=value.dump();require(bytes.size()==requested,"Payload construction");const auto began=Clock::now();DWORD flags=FILE_ATTRIBUTE_NORMAL;if(mode!="cached-flush")flags|=FILE_FLAG_WRITE_THROUGH;if(mode=="unbuffered-write-through")flags|=FILE_FLAG_NO_BUFFERING;
 try{
  File file;auto mark=Clock::now();file.h=CreateFileW(temp.c_str(),GENERIC_WRITE|FILE_READ_ATTRIBUTES,0,nullptr,CREATE_NEW,flags,nullptr);require(file.h!=INVALID_HANDLE_VALUE,"Create");row["openMs"]=elapsed(mark);size_t count=bytes.size();const void* buffer=bytes.data();Memory memory;mark=Clock::now();
  if(mode=="unbuffered-write-through"){
   FILE_STORAGE_INFO storage{};FILE_ALIGNMENT_INFO alignment{};require(GetFileInformationByHandleEx(file.h,FileStorageInfo,&storage,sizeof(storage))!=FALSE,"Storage alignment query");require(GetFileInformationByHandleEx(file.h,FileAlignmentInfo,&alignment,sizeof(alignment))!=FALSE,"File alignment query");const size_t align=std::max<size_t>({storage.LogicalBytesPerSector,storage.PhysicalBytesPerSectorForAtomicity,storage.PhysicalBytesPerSectorForPerformance,storage.FileSystemEffectivePhysicalBytesPerSectorForAtomicity,static_cast<size_t>(alignment.AlignmentRequirement)+1});require(align&&!(align&(align-1))&&align<=1024*1024,"Unsupported alignment");count=(count+align-1)&~(align-1);memory.p=VirtualAlloc(nullptr,count+align,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);require(memory.p!=nullptr,"Aligned allocation");auto pointer=(reinterpret_cast<uintptr_t>(memory.p)+align-1)&~(static_cast<uintptr_t>(align)-1);auto output=reinterpret_cast<char*>(pointer);memset(output,' ',count);memcpy(output,bytes.data(),bytes.size());buffer=output;row["alignment"]=align;row["logicalSector"]=storage.LogicalBytesPerSector;row["physicalSector"]=storage.PhysicalBytesPerSectorForAtomicity;row["paddingBytes"]=count-bytes.size();
  }
  row["alignmentAndBufferMs"]=elapsed(mark);row["storedBytes"]=count;mark=Clock::now();DWORD written=0;require(count<=MAXDWORD&&WriteFile(file.h,buffer,static_cast<DWORD>(count),&written,nullptr)&&written==count,"Complete write");row["writeMs"]=elapsed(mark);mark=Clock::now();if(mode=="cached-flush")require(FlushFileBuffers(file.h)!=FALSE,"Required flush");row["explicitFlushMs"]=elapsed(mark);mark=Clock::now();file.close();row["closeMs"]=elapsed(mark);mark=Clock::now();require(MoveFileExW(temp.c_str(),target.c_str(),MOVEFILE_WRITE_THROUGH)!=FALSE,"Publish");row["renameMs"]=elapsed(mark);row["primaryWallMs"]=elapsed(began);
  mark=Clock::now();{File barrier;barrier.h=CreateFileW(target.c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr);require(barrier.h!=INVALID_HANDLE_VALUE&&FlushFileBuffers(barrier.h)!=FALSE,"Symmetric post-publication barrier");barrier.close();}row["postPublicationBarrierMs"]=elapsed(mark);row["throughPostPublicationBarrierMs"]=elapsed(began);
  std::ifstream input(target,std::ios::binary);std::string saved((std::istreambuf_iterator<char>(input)),{});require(saved.size()==count&&saved.substr(0,bytes.size())==bytes&&std::all_of(saved.begin()+bytes.size(),saved.end(),[](char c){return c==' ';})&&Json::parse(saved)==value,"Exact prefix padding and parsed metadata");row["bytesAndJsonVerified"]=true;row["passed"]=true;
 }catch(const std::exception& e){row["passed"]=false;row["error"]=e.what();}return row;
}
int wmain(int argc,wchar_t** argv){if(argc!=2)return 2;auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);std::ofstream(root/L".udm-intent-probe-owned")<<"Owned standalone metadata IO probe";Json rows=Json::array();bool passed=true;int sequence=0;for(const size_t size:{8191,32769})for(const std::string mode:{"cached-flush","buffered-write-through","unbuffered-write-through","unbuffered-write-through","buffered-write-through","cached-flush"}){auto row=run(root,mode,size,sequence++);passed=passed&&row["passed"].get<bool>();std::cout<<row.dump()<<"\n";rows.push_back(std::move(row));}std::ofstream(root/L"results.json")<<Json{{"passed",passed},{"diagnosticOnly",true},{"noPowerLossTest",true},{"rows",rows}}.dump(2);return passed?0:1;}
