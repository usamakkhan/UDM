#include "Core.hpp"
#include "CatalogAdmissionReservation.hpp"
#include "BrowserAdmissionContext.hpp"
#include <iostream>
using namespace udm;
static Json checks=Json::array();
static void check(bool ok,const std::string& name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);}
template<class Action>static void rejects(Action&& action,const std::string& name){bool rejected=false;try{action();}catch(const std::exception&){rejected=true;}check(rejected,name);}
#define UDM_MEDIA_CRASH_FIXTURE_NAME L"Udm.CatalogAdmissionReservationTests.exe"
#include "MediaAdmissionChecks.hpp"
int wmain(int argc,wchar_t** argv){
 BrowserAdmissionContext admissionContext;
 if(argc==4&&std::wstring(argv[1])==L"--media-crash-fixture")return mediaCrashChild(argv[2],utf8(argv[3]));
 if(argc!=2)return 2;const fs::path root=fs::absolute(argv[1]);if(fs::exists(root))return 3;fs::create_directories(root);std::string error;
 try{
  Json job={{"Id","reserved-job"},{"Folder",utf8((root/L"files").wstring())},{"FileName","video.mp4"},{"Status","Awaiting confirmation"},{"Video",nullptr},{"Audio",nullptr}};
  Json receipt={{"protocol",3},{"status","accepted"},{"id","reserved-job"},{"fingerprint","opaque-protected-fixture"}};
  const CatalogAdmissionReservation reservation("operation-a",job,receipt);
  Json base={{"Schema",1},{"Downloads",Json::array()},{"Settings",{{"Keep",true}}}};
  auto merged=base;reservation.overlay(merged);
  check(merged["Downloads"].size()==1&&merged["Downloads"][0]==job&&merged["BrowserCaptures"]["operation-a"]==receipt,"Overlay adds the exact reserved job and receipt together");
  auto once=merged;reservation.overlay(merged);check(merged==once,"Repeating the same reservation is idempotent");
  check(base["Downloads"].empty()&&!base.contains("BrowserCaptures"),"Preparing a persistence overlay leaves the source catalog unchanged");
  auto conflict=once;conflict["Downloads"][0]["FileName"]="changed.mp4";auto previous=conflict;
  rejects([&]{reservation.overlay(conflict);},"Changed job content cannot reuse a reserved identity");check(conflict==previous,"Job conflict leaves the detached snapshot unchanged");
  conflict=once;conflict["BrowserCaptures"]["operation-a"]["fingerprint"]="different";previous=conflict;
  rejects([&]{reservation.overlay(conflict);},"Changed receipt fingerprint cannot reuse a reserved token");check(conflict==previous,"Receipt conflict leaves the detached snapshot unchanged");
  conflict=once;conflict["Downloads"].push_back(job);rejects([&]{reservation.overlay(conflict);},"Duplicate reserved IDs in a snapshot are rejected");
  conflict=base;auto other=job;other["Id"]="other-job";conflict["Downloads"].push_back(other);previous=conflict;
  rejects([&]{reservation.overlay(conflict);},"An unrelated job cannot take a missing reservation's destination");check(conflict==previous,"Destination conflict publishes neither receipt nor reserved job");
  auto secondJob=job;secondJob["Id"]="second-job";secondJob["FileName"]="second.mp4";auto secondReceipt=receipt;secondReceipt["id"]="second-job";
  CatalogAdmissionReservation second("operation-b",secondJob,secondReceipt);second.overlay(merged);reservation.overlay(merged);
  check(merged["Downloads"].size()==2&&merged["BrowserCaptures"].size()==2&&yes(merged["Settings"],"Keep"),"Independent overlays preserve other reservations and unrelated settings");
  auto originalJob=job;job["FileName"]="outside-change.mp4";receipt["fingerprint"]="outside-change";auto detached=base;reservation.overlay(detached);
  check(detached["Downloads"][0]==originalJob&&str(detached["BrowserCaptures"]["operation-a"],"fingerprint")=="opaque-protected-fixture","Reservation owns immutable copies of its supplied job and receipt");
  conflict=base;conflict["BrowserCaptures"]=Json::array();rejects([&]{reservation.overlay(conflict);},"Malformed receipt stores are rejected");
  rejects([&]{CatalogAdmissionReservation invalid("",originalJob,secondReceipt);},"An invalid reservation identity is rejected before overlay");
  // Exercise the actual integrated admission, rollback, lost acknowledgement,
  // restart and refresh paths using the existing maintained crash fixtures.
  mediaAdmissionChecks(root/L"integrated");
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"error",error},{"checks",checks}}.dump(2),false);
 std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
