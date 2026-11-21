#include "Core.hpp"
#include "ManagedModelWork.hpp"
#include <iostream>
using namespace udm;
template<class F>static bool until(F test){const auto end=GetTickCount64()+5000;while(!test()&&GetTickCount64()<end)Sleep(1);return test();}
struct JoinWork {std::shared_ptr<ManagedModelWork> work;~JoinWork(){work->cancel();work->join();}};
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array();std::string error;const auto check=[&](bool value,const char* name){checks.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);};
 try{int owner=0,other=0;auto work=std::make_shared<ManagedModelWork>();JoinWork joined{work};bool rejected=false;try{work->start([]{});}catch(...){rejected=true;}check(rejected,"A worker cannot start before its lifetime owner is bound");work->bindOwner(&owner);rejected=false;try{work->bindOwner(&other);}catch(...){rejected=true;}check(rejected,"A retained worker cannot change lifetime owners");
  struct Probe {std::atomic_bool entered=false,correctOwner=false,selfJoinRejected=false;};auto probe=std::make_shared<Probe>();work->start([work,probe,&owner,&other]{probe->correctOwner=ManagedModelWork::executingFor(&owner)&&!ManagedModelWork::executingFor(&other);try{work->join();}catch(const std::logic_error&){probe->selfJoinRejected=true;}probe->entered=true;while(!work->isCancelled())work->waitForDecision();});
  check(until([&]{return probe->entered.load();})&&probe->correctOwner&&probe->selfJoinRejected,"Dedicated work records its owner and rejects self join");rejected=false;try{work->start([]{});}catch(...){rejected=true;}check(rejected,"One retained owner cannot launch a second thread");rejected=false;try{work->bindOwner(&owner);}catch(...){rejected=true;}check(rejected,"A running worker cannot be rebound");
  work->cancel();check(until([&]{return work->isComplete();}),"Cancellation wakes a deferred decision wait");work->join();check(work->joined()&&work->isComplete(),"Joining acknowledges all thread work");work->join();check(work->joined(),"Repeated joins are safe");
  auto retired=std::make_shared<ManagedModelWork>();retired->bindOwner(&owner);retired->cancel();retired->join();rejected=false;try{retired->start([]{});}catch(...){rejected=true;}check(rejected&&retired->joined(),"Retiring before launch prevents a worker escaping shutdown");
  auto failed=std::make_shared<ManagedModelWork>();JoinWork failureJoin{failed};failed->bindOwner(&owner);failed->start([]{throw std::runtime_error("Owned worker failure");});check(until([&]{return failed->isComplete();}),"An unexpected action failure still completes its thread owner");failed->join();check(failed->joined()&&!ManagedModelWork::executingFor(&owner),"Worker identity never leaks into the calling thread");
 }catch(const std::exception& failed){error=failed.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"checks",checks},{"error",error}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
