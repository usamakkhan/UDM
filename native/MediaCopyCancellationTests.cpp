// Manual cross-volume regression: pass an existing test-output directory on
// another volume. The source and catalog use a unique Windows temporary folder.
#include "MediaPublication.hpp"
#include <iostream>
#include <future>

int wmain(int argc,wchar_t** argv){
 using namespace udm;
 if(argc!=2){std::cerr<<"Pass a cross-volume test-output directory.\n";return 2;}
 const auto root=fs::temp_directory_path()/(L"udm-copy-cancel-"+wide(guid()));
 const auto destination=fs::absolute(argv[1])/(L"udm-copy-cancel-"+wide(guid()));
 fs::path source=root/L"verified-source.bin";
 try{
  if(!fs::create_directories(root)||!fs::create_directories(destination))throw std::runtime_error("Test directories must be new.");
  constexpr i64 size=512LL*1024*1024;
  {
   Handle output(CreateFileW(source.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,0,nullptr));
   LARGE_INTEGER end{};end.QuadPart=size;
   if(!output||!SetFilePointerEx(output.h,end,nullptr,FILE_BEGIN)||!SetEndOfFile(output.h))throw std::runtime_error("Cannot create source fixture.");
  }
  auto volume=[](const fs::path& path){
   Handle file(CreateFileW(path.c_str(),0,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,nullptr));
   BY_HANDLE_FILE_INFORMATION info{};
   if(!file||!GetFileInformationByHandle(file.h,&info))throw std::runtime_error("Cannot identify test volume.");
   return info.dwVolumeSerialNumber;
  };
  if(volume(source)==volume(destination))throw std::runtime_error("The fixture requires different volumes.");
  const auto digest=fileHash(source);
  Manager manager(root/L"state");
  auto job=manager.add("https://example.invalid/test.mp4",utf8(destination.wstring()),"final.mp4");
  auto cancel=std::make_shared<Cancel>();
  auto worker=std::async(std::launch::async,[&]{
   try{publishMediaOutput(manager,job,source,digest,cancel);return false;}
   catch(const Cancelled&){return true;}
  });
  i64 observed=0;ULONGLONG cancelledAt=0;
  try{
   const auto deadline=GetTickCount64()+15000;
   while(GetTickCount64()<deadline&&worker.wait_for(std::chrono::milliseconds(0))!=std::future_status::ready){
    for(const auto& entry:fs::directory_iterator(destination))if(entry.path().extension()==L".publishing"){
     std::error_code ec;auto copied=fs::file_size(entry.path(),ec);
     if(!ec&&copied>0&&copied<(uintmax_t)size){observed=(i64)copied;cancelledAt=GetTickCount64();cancel->stop=true;break;}
    }
    if(observed)break;
    Sleep(1);
   }
  }catch(...){cancel->stop=true;worker.wait();throw;}
  cancel->stop=true;
  const bool cancelled=worker.get();
  const auto latency=cancelledAt?GetTickCount64()-cancelledAt:0;
  bool leftover=false;for(const auto& entry:fs::directory_iterator(destination))leftover|=entry.path().extension()==L".publishing";
  const bool unchanged=fs::exists(source)&&fs::file_size(source)==size&&fileHash(source)==digest;
  const bool passed=observed>0&&observed<size&&cancelled&&latency<2000&&unchanged&&!leftover&&!fs::exists(job->target());
  std::cout<<Json{{"passed",passed},{"sourceBytes",size},{"observedCopiedBytes",observed},{"cancelLatencyMs",latency},{"cancelled",cancelled},{"sourceUnchanged",unchanged},{"stagingRemoved",!leftover},{"finalAbsent",!fs::exists(job->target())}}.dump(2)<<std::endl;
  std::error_code ec;fs::remove(source,ec);
  return passed?0:1;
 }catch(const std::exception& error){std::cerr<<error.what()<<std::endl;std::error_code ec;fs::remove(source,ec);return 1;}
}
