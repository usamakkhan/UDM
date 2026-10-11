#pragma once
#include "Ui.hpp"
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
inline void categoryRule(CWnd* owner,Json& prefs){Form d("Category settings",463,198,owner);d.init=[&]{auto categories=optionCategories(prefs);d.label("Category",10,15,87);auto cat=d.combo(categories,"Archives",105,11,348);d.label("Extensions",10,51,87);auto extensions=d.edit("",105,47,348);d.label("Sites (optional)",10,87,94);auto hosts=d.edit("",105,83,348);d.label("Separate extensions and host names with spaces. Use * for all file types.",10,122,443,34);d.button("Load rule",10,161,87,[cat,extensions,hosts,&prefs]{for(auto r:prefs["CategoryRules"])if(str(r,"Category")==text(cat)){extensions->SetWindowText(cs(str(r,"Extensions")));hosts->SetWindowText(cs(str(r,"Hosts")));return;}extensions->SetWindowText(L"");hosts->SetWindowText(L"");});d.accept=[&,cat,extensions,hosts]{auto ex=words(text(extensions));for(auto s:ex)if(s!="*"&&!std::regex_match(s,std::regex("[a-z0-9_-]{1,30}")))throw std::runtime_error("Use plain extensions such as zip mp4 pdf.");for(auto h:words(text(hosts)))Url("https://"+h+"/");auto& rules=prefs["CategoryRules"];rules.erase(std::remove_if(rules.begin(),rules.end(),[&](const Json& r){return str(r,"Category")==text(cat);}),rules.end());if(!ex.empty()||!trim(text(hosts)).empty())rules.push_back({{"Category",text(cat)},{"Extensions",text(extensions)},{"Hosts",text(hosts)}});if(trim(text(hosts)).empty())prefs["CategoryTypeOverrides"][text(cat)]=lower(trim(text(extensions)));else if(prefs.contains("CategoryTypeOverrides"))prefs["CategoryTypeOverrides"].erase(text(cat));d.close();};d.button("Save",273,161,83,d.accept);d.button("Cancel",366,161,87,[&]{d.close(IDCANCEL);});};d.DoModal();}
#include "DialogPreferences.hpp"
inline void showNetworkIntegration(CWnd* owner);
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
  if(capturing&&catalog){if(snap.contains("Capture")){auto result=catalog->processOriginalResponses(*monitor,snap["Capture"]);if(!result["Errors"].empty())throw std::runtime_error(str(result["Errors"][0],"Message"));}else catalog->recoverOriginalResponses();}
  return snap;
  }catch(...){if(catalog)catalog->recoverOriginalResponses();throw;}
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
