#pragma once
#include "Ui.hpp"
#include "CategoryEditor.hpp"
#include "SiteLogins.hpp"
#include "DialUpUi.hpp"
#include <future>
#include <tlhelp32.h>
#include "Startup.hpp"
#include "SchedulerTime.hpp"
#include "QueueCompletion.hpp"
#include "QueueMembership.hpp"
#include "BrowserSettingsUi.hpp"
#include "BrowserIdentity.hpp"
#include "OriginalResponsePolicy.hpp"
#include "OptionsModel.hpp"
#include "GrabberDestinations.hpp"
#include "GrabberProject.hpp"
#include "GrabberBrowserSession.hpp"
#include "ProxyPolicy.hpp"
namespace udm {
#include "ProxyPolicyUi.hpp"
inline std::optional<std::string> showCategoryProperties(CWnd* owner,Manager& manager,const Json& p,const std::string& original){
  std::optional<std::string> accepted;
  Form d("UDM categories",322,177,owner);d.dialogUnits=true;
  d.init=[&]{
   d.label("Category name",9,3,235,10);auto name=d.edit(original=="Archives"?"Compressed":original,9,13,238,14,original=="Other");
   auto paths=dictionary(p["CategoryPaths"]);std::string extensions=original.empty()?"":categoryExtensions(p,original),hosts;
   for(const auto& rule:p["CategoryRules"])if(str(rule,"Category")==original){extensions=str(rule,"Extensions");hosts=str(rule,"Hosts");break;}
   d.label("Automatically put in this category the following file types:",9,33,238,10);auto ex=d.edit(extensions,9,43,238,14);
   d.label("Note: type file extensions separated by space (e.g. avi mpg mpeg)",9,58,302,10);
   d.control(L"STATIC","",SS_ETCHEDHORZ,9,72,302,1);
   auto restrictSites=d.check("Automatically put in this category the files from the following sites only:",!hosts.empty(),9,78,302,10);
   auto sites=d.edit(hosts,9,90,302,14);sites->EnableWindow(!hosts.empty());
   d.bind(restrictSites,[&,restrictSites,sites]{sites->EnableWindow(d.checked(restrictSites));});
   d.label("Separate sites by spaces. Use *.example.com for a domain and its subdomains.",9,106,302,10);
   d.control(L"STATIC","",SS_ETCHEDHORZ,9,121,302,1);
   d.label("Save future downloads of this category to the following folder:",9,127,302,10);auto folder=d.edit(str(paths,original.c_str()),9,138,302,14);
   auto remember=d.check("Remember last save path",yes(p.value("CategoryRememberLast",Json::object()),original.c_str()),9,155,228,10);
   d.button("Browse...",261,155,50,[&,folder]{auto value=chooseFolder(&d,wide(text(folder)));if(!value.empty())folder->SetWindowText(value.c_str());},14);
   d.accept=[&,name,ex,sites,folder,restrictSites,remember]{auto value=trim(text(name));if(original=="Archives"&&value=="Compressed")value=original;auto hosts=d.checked(restrictSites)?trim(text(sites)):std::string();if(d.checked(restrictSites)&&hosts.empty())throw std::runtime_error("Enter at least one site or turn off the site restriction.");saveCategoryProperties(manager,original,value,text(ex),hosts,trim(text(folder)),d.checked(remember));accepted=std::move(value);d.close();};
   auto ok=d.button("OK",261,13,50,d.accept,14);d.defaultButton(ok);d.button("Cancel",261,34,50,[&]{d.close(IDCANCEL);},14);
  };d.DoModal();return accepted;
 }
#include "DialogPreferences.hpp"
inline void showNetworkIntegration(CWnd* owner);
#include "OptionsIcons.hpp"
#include "OptionsUi.hpp"
#include "OptionsPages.hpp"
inline SYSTEMTIME toLocal(i64 ms){return localScheduleTime(ms);}
inline i64 fromLocal(SYSTEMTIME t){return utcScheduleTime(t);}
#include "SchedulerUi.hpp"
#include "GrabberUi.hpp"
inline std::set<DWORD> networkProcessRoots(){
 std::map<DWORD,DWORD> parents;DWORD currentSession=0;ProcessIdToSessionId(GetCurrentProcessId(),&currentSession);
 Handle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0));PROCESSENTRY32W entry{sizeof(entry)};
 if(!snapshot)throw std::runtime_error("Cannot enumerate browser process roots.");
 if(Process32FirstW(snapshot.h,&entry))do{
  auto name=lower(utf8(entry.szExeFile));DWORD session=0;
  if(originalResponseBrowser(name)&&ProcessIdToSessionId(entry.th32ProcessID,&session)&&session==currentSession)parents[entry.th32ProcessID]=entry.th32ParentProcessID;
 }while(Process32NextW(snapshot.h,&entry));
 std::set<DWORD> roots={GetCurrentProcessId()};for(auto& [pid,parent]:parents)if(!parents.count(parent))roots.insert(pid);return roots;
}
class NetworkCaptureSession {
public:
 Manager* catalog=nullptr;bool capturing=false;std::unique_ptr<Monitor> monitor;std::map<DWORD,FILETIME> watched;
 explicit NetworkCaptureSession(Manager* manager=nullptr):catalog(manager){}
 ~NetworkCaptureSession(){try{stop();}catch(...){monitor.reset();}}
 void stop(){if(monitor){if(catalog)catalog->stopOriginalResponses(*monitor);else monitor->watch({});}monitor.reset();watched.clear();capturing=false;}
 void start(){if(monitor&&monitor->reconnectExisting())return;const bool resumeCapture=capturing;stop();auto driver=std::make_unique<Monitor>();auto pids=networkProcessRoots();std::map<DWORD,FILETIME> next;
  for(auto pid:pids){if(next.size()==32)break;Handle process(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid));FILETIME a{},b{},c{},d{};if(process&&GetProcessTimes(process.h,&a,&b,&c,&d))next[pid]=a;}
  std::vector<DWORD> live;for(auto& item:next)live.push_back(item.first);if(live.empty())throw std::runtime_error("Open a browser before starting the signed monitor.");driver->watch(live);if(resumeCapture)driver->capture({80},true);watched=std::move(next);monitor=std::move(driver);capturing=resumeCapture;
 }
 void capture(){if(!monitor)throw std::runtime_error("Start the signed monitor first.");monitor->capture({80},true);capturing=true;}
 Json poll(){if(!monitor)return Json();try{std::vector<DWORD> live;std::map<DWORD,FILETIME> next;
  for(auto pid:networkProcessRoots()){
   if(next.size()==32)break;
   Handle process(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION|SYNCHRONIZE,FALSE,pid));FILETIME a{},b{},c{},d{};
   if(process&&WaitForSingleObject(process.h,0)==WAIT_TIMEOUT&&GetProcessTimes(process.h,&a,&b,&c,&d)){next[pid]=a;live.push_back(pid);}
  }
  monitor->watchExisting(live);watched=std::move(next);if(capturing&&!live.empty())monitor->capture({80},true);auto snap=monitor->snapshot();
  if(capturing&&catalog){if(snap.contains("Capture")){auto result=catalog->processOriginalResponses(*monitor,snap["Capture"]);if(!result["Errors"].empty())throw std::runtime_error(str(result["Errors"][0],"Message"));}else catalog->recoverOriginalResponses(true);}
  return snap;
  }catch(...){if(catalog)catalog->recoverOriginalResponses(true);throw;}
 }
};
class NetworkDialog:public Form {
 DECLARE_MESSAGE_MAP()
 NetworkCaptureSession local;NetworkCaptureSession& session;bool shared=false;CListCtrl* table=nullptr;CWnd* status=nullptr;
 void update(){Json rows;auto snap=session.poll();if(!snap.is_null()){rows=snap["Flows"];status->SetWindowText(cs((session.capturing?"HTTP download capture active. ":"Signed monitor watching ")+std::to_string(num(snap,"Watched"))+" process trees. Dropped observations: "+std::to_string(num(snap,"Dropped"))));}else{rows=endpoints();status->SetWindowText(cs("Windows TCP / UDP endpoints. "+std::to_string(rows.size())+" connections."));}table->SetRedraw(FALSE);table->DeleteAllItems();int i=0;for(auto row:rows){table->InsertItem(i,cs(str(row,"Process","Process")+" ("+std::to_string(num(row,"ProcessId"))+")"));int column=1;for(auto key:{"Local","Remote","Transport","State"})table->SetItemText(i,column++,cs(str(row,key)));table->SetItemText(i,5,cs(row.contains("Received")&&row["Received"].is_number()?bytes(num(row,"Received")):"--"));table->SetItemText(i,6,cs(row.contains("Sent")&&row["Sent"].is_number()?bytes(num(row,"Sent")):"--"));++i;}table->SetRedraw(TRUE);table->Invalidate();}
 afx_msg void OnTimer(UINT_PTR id){try{update();}catch(const std::exception& e){status->SetWindowText(cs(e.what()));}Form::OnTimer(id);}
public:explicit NetworkDialog(CWnd* parent,Manager* manager=nullptr,NetworkCaptureSession* external=nullptr):Form("UDM network integration",805,484,parent),local(manager),session(external?*external:local),shared(external!=nullptr){init=[this]{
 label("Browser and UDM connections",12,12,770);status=label("Reading connections...",12,40,778,39);table=make<CListCtrl>(WS_TABSTOP|WS_BORDER|LVS_SHOWSELALWAYS|LVS_REPORT|LVS_SINGLESEL,12,83,780,320);table->SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER);
 const wchar_t* names[]={L"Process",L"Local endpoint",L"Remote endpoint",L"Protocol",L"State",L"Received",L"Sent"};int widths[]={130,175,175,58,79,74,64};for(int i=0;i<7;++i)table->InsertColumn(i,names[i],LVCFMT_LEFT,widths[i]);
 button("Start signed monitor",12,418,157,[this]{session.start();update();});button("Stop monitor",184,418,136,[this]{session.stop();update();});
 button("Driver status",337,418,128,[this]{auto info=diagnostics();if(session.monitor&&session.monitor->running())info["DriverStatus"]="Available; monitoring is active.";MessageBox(cs(info.dump(2)),L"Driver status",MB_OK|MB_ICONINFORMATION);});
 if(session.catalog)button("Capture HTTP downloads",480,418,205,[this]{session.capture();update();});
 button("Close",693,445,99,[this]{if(!shared)session.stop();close();});label(shared?"Capture continues after closing this window. Stop monitor or exit UDM to stop it. HTTPS downloads use the browser extension.":"The signed monitor observes connection metadata. HTTPS downloads use the browser extension.",12,454,670,26);
 update();SetTimer(1,1000,nullptr);};}
};
BEGIN_MESSAGE_MAP(NetworkDialog,Form)
 ON_WM_TIMER()
END_MESSAGE_MAP()
inline void showNetworkIntegration(CWnd* owner){NetworkDialog dialog(owner);dialog.DoModal();}
}
