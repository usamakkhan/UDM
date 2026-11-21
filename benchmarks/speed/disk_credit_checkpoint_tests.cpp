#include "disk_credit_checkpoint.hpp"
#include "CatalogCommitCoordinator.hpp"
#include <future>
#include <iostream>
using namespace udm;
using namespace udm::experiment;
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;fs::path root=argv[1];if(fs::exists(root))return 3;fs::create_directories(root);
 Json checks=Json::array();bool passed=false;std::string error;
 auto check=[&](bool v,const char* name){checks.push_back({{"name",name},{"passed",v}});if(!v)throw std::runtime_error(name);};
 auto refused=[&](auto&& action){try{action();return false;}catch(const std::exception&){return true;}};
 try{
  constexpr i64 total=65536;
  auto file=std::make_shared<Handle>(CreateFileW((root/L"body.data").c_str(),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_FLAG_OPEN_REPARSE_POINT,nullptr));
  check(static_cast<bool>(*file),"The fixture owns an ordinary disk file with delete sharing denied");
  auto cancel=std::make_shared<Cancel>();std::shared_ptr<const Cancel> generation=cancel;
  Json live={{"Id","disk-credit-job"},{"Url","http://127.0.0.1/body"},{"Size",total},{"Folder","fixture"},{"FileName","body.bin"},{"ETag","v1"},{"Status","Downloading"},{"Received",total},{"TransferredBytes",total+4096},{"Description","live metadata"},{"Segments",Json::array({{{"Index",0},{"Start",0},{"End",32767},{"Done",32768}},{{"Index",1},{"Start",32768},{"End",65535},{"Done",32768}}})}};
  const auto original=live;
  check(refused([&]{auto row=live;row["Size"]=0;DiskCreditCheckpoint bad(row,generation,file);}),"Zero-sized representations are refused");
  check(refused([&]{auto row=live;row["Size"]=BodyBufferStorage::maxBytes+1;DiskCreditCheckpoint bad(row,generation,file);}),"The bounded body size is enforced");
  check(refused([&]{auto row=live;row["Size"]=1.5;DiskCreditCheckpoint bad(row,generation,file);}),"Fractional representation sizes are refused");
  check(refused([&]{auto row=live;row["Size"]=std::numeric_limits<unsigned long long>::max();DiskCreditCheckpoint bad(row,generation,file);}),"Unsigned size overflow is refused before narrowing");
  check(refused([&]{DiskCreditCheckpoint bad(live,{},file);}),"A generation must have an owning cancellation identity");
  check(refused([&]{DiskCreditCheckpoint bad(live,generation,{});}),"A generation must have a held file lifetime");
  for(const auto field:{"HttpPartsResetPending","RefreshPendingValidation"}){
   auto row=live;row[field]=true;
   check(refused([&]{DiskCreditCheckpoint bad(row,generation,file);}),"A pending reset or unvalidated representation cannot admit a writer generation");
  }
  auto credit=std::make_unique<DiskCreditCheckpoint>(live,generation,file);
  auto write=[&](i64 offset,i64 length,bool success=true){
   auto lease=credit->beginWrite(generation,file,offset,length);
   LARGE_INTEGER at{};at.QuadPart=offset;if(!SetFilePointerEx(file->h,at,nullptr,FILE_BEGIN))throw std::runtime_error("Fixture seek failed");
   Bytes bytes(static_cast<size_t>(length));for(size_t i=0;i<bytes.size();++i)bytes[i]=static_cast<unsigned char>((offset+i)%251);
   DWORD done=0;if(!WriteFile(file->h,bytes.data(),static_cast<DWORD>(bytes.size()),&done,nullptr)||done!=bytes.size())throw std::runtime_error("Fixture write failed");
   if(success)lease.acknowledge();
  };
  {
   auto snapshot=credit->capture(live,generation,file);
   check(num(snapshot.value(),"Received")==0,"RAM-only received bytes have no persisted disk credit");
   check(num(snapshot.value(),"TransferredBytes")==total+4096,"Checkpoint projection preserves actual network/quota accounting");
   check(live==original,"Disk-credit projection never changes the UI/live row");
   check(!credit->tryRetire(),"A detached checkpoint pins its generation before serialization");
  }
  write(8192,8192);write(32768,8192);
  {
   auto snapshot=credit->capture(live,generation,file);
   check(num(snapshot.value()["Segments"][0],"Done")==0,"Written data beyond a segment gap is not summed into Done");
   check(num(snapshot.value()["Segments"][1],"Done")==8192&&num(snapshot.value(),"Received")==8192,"Each segment receives only its acknowledged contiguous disk prefix");
  }
  write(0,4096,false);
  {
   auto snapshot=credit->capture(live,generation,file);
   check(num(snapshot.value()["Segments"][0],"Done")==0,"A failed or partially failed operation adds no credit even if bytes physically exist");
  }
  write(0,8192);
  {
   auto old=credit->capture(live,generation,file);write(16384,16384);
   auto newer=credit->capture(live,generation,file);
   check(num(old.value()["Segments"][0],"Done")==16384&&num(newer.value()["Segments"][0],"Done")==32768,"An older captured checkpoint remains a conservative immutable lower bound as writes advance");
  }
  {
   auto row=live;row["Segments"][0]["Done"]=512;
   auto snapshot=credit->capture(row,generation,file);
   check(num(snapshot.value()["Segments"][0],"Done")==512,"Disk coverage cannot increase a live segment's claimed receive progress");
  }
  for(const auto name:{"Id","Url","Folder","FileName","ETag"}){
   auto row=live;row[name]="different";
   check(refused([&]{credit->capture(row,generation,file);}),"Changed job, destination or representation identity refuses the old binding");
  }
  for(const auto name:{"Modified","HttpContentEncoding","SparseDataFile","HttpPartsResetPending","RefreshPendingValidation"}){
   auto row=live;row[name]=true;
   check(refused([&]{credit->capture(row,generation,file);}),"Added representation/layout/reset metadata cannot silently reuse a generation");
  }
  check(refused([&]{credit->capture(live,std::make_shared<Cancel>(),file);}),"A newer cancellation object cannot capture the previous generation");
  check(refused([&]{credit->beginWrite(std::make_shared<Cancel>(),file,0,1);}),"A stale writer is refused before starting disk work");
  auto alias=std::shared_ptr<Handle>(file.get(),[](Handle*){});
  check(refused([&]{credit->capture(live,generation,alias);}),"An equal raw handle-owner address with a different ownership block is refused");
  for(auto bounds:{std::pair<i64,i64>{-1,1},{0,0},{total,1},{1,std::numeric_limits<i64>::max()}})
   check(refused([&]{credit->beginWrite(generation,file,bounds.first,bounds.second);}),"Write bounds and addition overflow are refused before IO");
  for(const auto field:{"Start","End","Done"}){
   auto row=live;row["Segments"][0][field]=-1;
   check(refused([&]{credit->capture(row,generation,file);}),"Negative saved-segment bounds are refused");
  }
  for(const auto field:{"Start","End","Done"}){
   auto row=live;row["Segments"][0][field]=2.5;
   check(refused([&]{credit->capture(row,generation,file);}),"Fractional saved-segment bounds are refused");
  }
  {
   auto row=live;row["Segments"][1]["Start"]=32767;
   check(refused([&]{credit->capture(row,generation,file);}),"Overlapping segments cannot double-count saved Received");
   row=live;row["Segments"][1]["Done"]=32769;
   check(refused([&]{credit->capture(row,generation,file);}),"Done cannot exceed its segment length");
   row=live;row["Segments"][1]["End"]=total;
   check(refused([&]{credit->capture(row,generation,file);}),"Segments cannot exceed the representation length");
   row=live;row["Segments"]=Json::array();for(int i=0;i<4097;++i)row["Segments"].push_back({{"Index",i},{"Start",i},{"End",i},{"Done",1}});
   check(refused([&]{credit->capture(row,generation,file);}),"Unbounded segment metadata is refused");
   row=live;row["Status"]="Complete";
   check(refused([&]{credit->capture(row,generation,file);}),"A live Complete label cannot publish buffered-only bytes as complete");
   row=live;row["Description"]="new description";row["TransferredBytes"]=total*3;
   auto snapshot=credit->capture(row,generation,file);
   check(str(snapshot.value(),"Description")=="new description"&&num(snapshot.value(),"TransferredBytes")==total*3,"Unrelated metadata and quota changes survive disk-credit projection");
  }
  {
   auto row=live;row["Segments"][1]["Index"]=0;
   check(refused([&]{credit->capture(row,generation,file);}),"Duplicate saved segment identifiers cannot create ambiguous resume credit");
   row=live;row["Segments"][0].erase("Index");
   check(refused([&]{credit->capture(row,generation,file);}),"Missing saved segment identifiers are refused");
   row=live;row["Id"]="0123456789abcdef0123456789abcdef";
   Manager manager(root/L"manager-view");auto job=std::make_shared<Job>(row);manager.jobs.push_back(job);
   const auto before=manager.snapshot();DiskCreditCheckpoint separate(job->snapshot(),generation,file);
   {auto snapshot=separate.capture(job->snapshot(),generation,file);check(num(snapshot.value(),"Received")==0&&manager.snapshot()==before&&num(job->snapshot(),"Received")==total,"The actual Job and Manager UI snapshot APIs remain unchanged by a separate persisted projection");}
   check(separate.tryRetire(),"The separate UI-view fixture releases its checkpoint pin");
  }
  {
   auto lease=credit->beginWrite(generation,file,40960,total-40960);
   check(!credit->tryRetire(),"An in-flight write pins the generation even before acknowledgment");
   LARGE_INTEGER at{};at.QuadPart=40960;
   if(!SetFilePointerEx(file->h,at,nullptr,FILE_BEGIN))throw std::runtime_error("Fixture seek failed");
   Bytes tail(total-40960);for(size_t i=0;i<tail.size();++i)tail[i]=static_cast<unsigned char>((40960+i)%251);
   DWORD saved=0;if(!WriteFile(file->h,tail.data(),static_cast<DWORD>(tail.size()),&saved,nullptr)||saved!=tail.size())throw std::runtime_error("Fixture tail write failed");
   cancel->stop=true;
   check(refused([&]{credit->beginWrite(generation,file,0,1);}),"Cancellation blocks new IO admission");
   lease.acknowledge();
   auto snapshot=credit->capture(live,generation,file);
   check(num(snapshot.value(),"Received")==total,"Already issued successful writes can be acknowledged and checkpointed after cancellation");
   cancel->stop=false;
  }
  check(FlushFileBuffers(file->h)!=FALSE,"The real body retains an explicit caller-owned durability barrier");
  Bytes expected(total);for(size_t i=0;i<expected.size();++i)expected[i]=static_cast<unsigned char>(i%251);
  writeBytes(root/L"expected.bin",expected);
  check(fileHash(file->h)==fileHash(root/L"expected.bin"),"Actual disk bytes match the independently generated complete body hash");
  {
   std::mutex gate;unsigned long long written=0;CatalogCommitCoordinator coordinator(gate,written);
   const auto catalog=root/L"checkpoint.json";
   auto old=credit->capture(live,generation,file);auto newerLive=live;newerLive["Description"]="newer checkpoint";
   auto newer=credit->capture(newerLive,generation,file);
   auto result=coordinator.commit(2,[&]{atomicText(catalog,newer.value().dump(),false);});
   bool staleWrite=false;auto stale=coordinator.commit(1,[&]{staleWrite=true;atomicText(catalog,old.value().dump(),false);});
   check(result.published()&&!stale.published()&&!staleWrite&&written==2,"A newer real catalog commit supersedes an older captured write");
   check(str(Json::parse(readText(catalog)),"Description")=="newer checkpoint","The newer actual on-disk catalog remains authoritative");
   check(!credit->tryRetire(),"Superseded snapshots still pin file reset until their callers release them");
  }
  {
   std::mutex gate;unsigned long long written=0;CatalogCommitCoordinator coordinator(gate,written);
   const auto catalog=root/L"held.json";atomicText(catalog,Json{{"original",true}}.dump(),false);
   std::promise<void> held,release;auto ready=held.get_future();auto resume=release.get_future();
   auto snapshot=credit->capture(live,generation,file);Json failure;
   std::thread writer([&,snapshot=std::move(snapshot)]()mutable{
    try{coordinator.commit(1,[&]{atomicText(catalog,snapshot.value().dump(),false,[&](const char* point){if(std::string_view(point)=="temporary-flushed"){held.set_value();resume.wait();throw std::runtime_error("injected checkpoint interruption");}});});}
    catch(const std::exception& e){failure=e.what();}
   });
   const bool reached=ready.wait_for(std::chrono::seconds(10))==std::future_status::ready;
   const bool refusedReset=reached&&!credit->tryRetire();release.set_value();writer.join();
   check(reached&&refusedReset,"A detached save held after its temporary-file flush prevents file reset through actual catalog IO");
   check(failure=="injected checkpoint interruption"&&written==0&&Json::parse(readText(catalog))==Json{{"original",true}},"Failed durable commit preserves the original catalog and sequence");
  }
  {
   auto snapshot=credit->capture(live,generation,file);auto row=snapshot.value();
   check(num(row,"Received")==total,"Failed catalog publication does not erase previously acknowledged disk credit");
  }
  {
   std::mutex gate;unsigned long long written=0;CatalogCommitCoordinator coordinator(gate,written);
   const auto catalog=root/L"held-success.json";
   std::promise<void> held,release;auto ready=held.get_future();auto resume=release.get_future();
   auto snapshot=credit->capture(live,generation,file);std::exception_ptr failure;
   std::thread writer([&,snapshot=std::move(snapshot)]()mutable{
    try{coordinator.commit(1,[&]{atomicText(catalog,snapshot.value().dump(),false,[&](const char* point){if(std::string_view(point)=="temporary-flushed"){held.set_value();resume.wait();}});});}
    catch(...){failure=std::current_exception();}
   });
   const bool reached=ready.wait_for(std::chrono::seconds(10))==std::future_status::ready;
   const bool refusedReset=reached&&!credit->tryRetire();release.set_value();writer.join();
   check(reached&&refusedReset&&!failure&&written==1,"Successful detached catalog IO also keeps reset blocked until its snapshot is released");
   check(num(Json::parse(readText(catalog)),"Received")==total,"The successful actual catalog contains exact complete written credit");
  }
  {
   DiskCreditCheckpoint parallel(live,generation,file);std::mutex fileMutex;std::atomic<unsigned> finished{0};
   std::vector<std::future<void>> producers;
   for(unsigned worker=0;worker<8;++worker)producers.push_back(std::async(std::launch::async,[&,worker]{
    struct Finish{std::atomic<unsigned>& count;~Finish(){++count;}} finish{finished};
    for(unsigned index=worker;index<16;index+=8){
     const i64 offset=index*4096;auto lease=parallel.beginWrite(generation,file,offset,4096);
     Bytes bytes(4096);for(size_t i=0;i<bytes.size();++i)bytes[i]=static_cast<unsigned char>((offset+i)%251);
     {std::lock_guard<std::mutex> lock(fileMutex);LARGE_INTEGER at{};at.QuadPart=offset;DWORD done=0;
      if(!SetFilePointerEx(file->h,at,nullptr,FILE_BEGIN)||!WriteFile(file->h,bytes.data(),4096,&done,nullptr)||done!=4096)throw std::runtime_error("Concurrent fixture IO failed");}
     lease.acknowledge();
    }
   }));
   i64 last=0;bool monotonic=true;
   while(finished<8){auto snapshot=parallel.capture(live,generation,file);const auto received=num(snapshot.value(),"Received");monotonic=monotonic&&received>=last&&received<=total;last=received;std::this_thread::yield();}
   for(auto& producer:producers)producer.get();
   {auto snapshot=parallel.capture(live,generation,file);check(monotonic&&num(snapshot.value(),"Received")==total,"Eight concurrent disk-write acknowledgers preserve monotonic conservative checkpoint credit");}
   check(parallel.tryRetire(),"Concurrent producers release every write and snapshot pin");
   check(FlushFileBuffers(file->h)&&fileHash(file->h)==fileHash(root/L"expected.bin"),"Concurrent real writes retain exact complete bytes and the explicit final flush");
  }
  {
   auto heldFile=std::make_shared<Handle>(CreateFileW((root/L"lifetime.data").c_str(),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_FLAG_OPEN_REPARSE_POINT,nullptr));
   check(static_cast<bool>(*heldFile),"The snapshot-lifetime fixture owns a separate held file");
   std::weak_ptr<Handle> weak=heldFile;auto owner=std::make_unique<DiskCreditCheckpoint>(live,generation,heldFile);
   {auto snapshot=owner->capture(live,generation,heldFile);owner.reset();heldFile.reset();check(!weak.expired()&&num(snapshot.value(),"Received")==0,"Destroying the controller cannot free a file still held by a detached checkpoint");}
   check(weak.expired(),"Releasing the final checkpoint destroys its remaining file-lifetime owner");
  }
  check(credit->tryRetire(),"Retirement succeeds after all write and checkpoint leases have settled");
  check(refused([&]{credit->capture(live,generation,file);})&&refused([&]{credit->beginWrite(generation,file,0,1);}),"A retired generation cannot start another checkpoint or write");
  auto nextCancel=std::make_shared<Cancel>();
  DiskCreditCheckpoint next(live,nextCancel,file);
  {auto snapshot=next.capture(live,nextCancel,file);check(num(snapshot.value(),"Received")==0,"A successor generation begins with zero credit despite the same physical file length");}
  check(next.tryRetire(),"An unused successor can retire without waiting");
  passed=true;
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",passed},{"error",error},{"checks",checks},{"productionIntegrated",false},{"durabilityOwnedByCaller",true}}.dump(2),false);
 std::cout<<checks.size()<<" checks; "<<(passed?"passed":error)<<"\n";return passed?0:1;
}
