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
#include "NetworkCaptureSession.hpp"
#include "OptionsModel.hpp"
#include "GrabberDestinations.hpp"
#include "GrabberProject.hpp"
#include "GrabberBrowserSession.hpp"
#include "ProxyPolicy.hpp"
namespace udm {
#include "ProxyPolicyUi.hpp"
#include "DialogPreferences.hpp"
inline void showNetworkIntegration(CWnd* owner);
#include "OptionsIcons.hpp"
#include "OptionsUi.hpp"
#include "OptionsPages.hpp"
inline SYSTEMTIME toLocal(i64 ms){return localScheduleTime(ms);}
inline i64 fromLocal(SYSTEMTIME t){return utcScheduleTime(t);}
#include "SchedulerUi.hpp"
#include "GrabberUi.hpp"
class NetworkDialog:public Form {
 DECLARE_MESSAGE_MAP()
 friend struct NetworkDialogTestAccess;
 NetworkDialogOwner presentation;NetworkCaptureSession local;NetworkCaptureSession& session;bool shared=false,closing=false,stopRequested=false;CListCtrl* table=nullptr;CWnd *status=nullptr,*startButton=nullptr,*stopButton=nullptr,*captureButton=nullptr,*driverButton=nullptr,*closeButton=nullptr;
 uint64_t renderedRevision=UINT64_MAX;
 void requestClose(){if(shared){close();return;}closing=true;stopRequested=false;update(false);}
 void update(bool request=true){
  session.pollReady();
  if(closing&&!session.busy()){
   if(stopRequested){if(session.view().error.empty()){close();return;}closing=false;stopRequested=false;}
   else{stopRequested=session.requestStop(taskNotifier(),false);}
  }
  if(request&&!closing)session.requestPoll(true,taskNotifier());
  const auto view=session.view();const auto snap=view.snapshot;const bool busy=session.busy();
  startButton->EnableWindow(!busy&&!closing);stopButton->EnableWindow(!busy&&!closing&&snap->monitorPresent);driverButton->EnableWindow(!busy&&!closing);if(captureButton)captureButton->EnableWindow(!busy&&!closing&&snap->monitorPresent);closeButton->EnableWindow(!closing);
  if(!view.error.empty())status->SetWindowText(cs(view.error));
  else if(closing)status->SetWindowText(L"Stopping the monitor and finishing captured responses...");
  else if(busy&&!snap->loaded)status->SetWindowText(L"Reading connections...");
  else if(snap->monitorPresent)status->SetWindowText(cs((snap->capturing?"HTTP download capture active. ":"Signed monitor watching ")+std::to_string(snap->watched)+" process trees. Dropped observations: "+std::to_string(snap->dropped)));
  else if(snap->loaded)status->SetWindowText(cs("Windows TCP / UDP endpoints. "+std::to_string(snap->flows.size())+" connections."));else status->SetWindowText(L"Reading connections...");
  if(renderedRevision==view.revision)return;renderedRevision=view.revision;
  table->SetRedraw(FALSE);table->DeleteAllItems();int i=0;
  for(const auto& row:snap->flows){table->InsertItem(i,cs(str(row,"Process","Process")+" ("+std::to_string(num(row,"ProcessId"))+")"));int column=1;for(auto key:{"Local","Remote","Transport","State"})table->SetItemText(i,column++,cs(str(row,key)));table->SetItemText(i,5,cs(row.contains("Received")&&row["Received"].is_number()?bytes(num(row,"Received")):"--"));table->SetItemText(i,6,cs(row.contains("Sent")&&row["Sent"].is_number()?bytes(num(row,"Sent")):"--"));++i;}
  table->SetRedraw(TRUE);table->Invalidate();
 }
 afx_msg void OnTimer(UINT_PTR id){try{if(id==1)update();else Form::OnTimer(id);}catch(const std::exception& e){status->SetWindowText(cs(e.what()));}}
public:explicit NetworkDialog(CWnd* parent,Manager* manager=nullptr,NetworkCaptureSession* external=nullptr):Form("UDM network integration",805,484,parent),local(manager),session(external?*external:local),shared(external!=nullptr){init=[this]{
 label("Browser and UDM connections",12,12,770);status=label("Reading connections...",12,40,778,39);table=make<CListCtrl>(WS_TABSTOP|WS_BORDER|LVS_SHOWSELALWAYS|LVS_REPORT|LVS_SINGLESEL,12,83,780,320);table->SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER);
 const wchar_t* names[]={L"Process",L"Local endpoint",L"Remote endpoint",L"Protocol",L"State",L"Received",L"Sent"};int widths[]={130,175,175,58,79,74,64};for(int i=0;i<7;++i)table->InsertColumn(i,names[i],LVCFMT_LEFT,widths[i]);
 startButton=button("Start signed monitor",12,418,157,[this]{session.requestStart(taskNotifier());update(false);});stopButton=button("Stop monitor",184,418,136,[this]{session.requestStop(taskNotifier());update(false);});
 driverButton=button("Driver status",337,418,128,[this]{auto info=diagnostics();if(session.view().snapshot->monitoring)info["DriverStatus"]="Available; monitoring is active.";MessageBox(cs(info.dump(2)),L"Driver status",MB_OK|MB_ICONINFORMATION);});
 if(session.hasCatalog())captureButton=button("Capture HTTP downloads",480,418,205,[this]{session.requestCapture(taskNotifier());update(false);});
 closeButton=button("Close",693,445,99,[this]{requestClose();});defaultButton(closeButton);accept=[this]{requestClose();};cancel=accept;label(shared?"Capture continues after closing this window. Stop monitor or exit UDM to stop it. HTTPS downloads use the browser extension.":"The signed monitor observes connection metadata. HTTPS downloads use the browser extension.",12,454,670,26);
 pulse=[this]{update(false);};pulsePeriod=100;update();SetTimer(1,1000,nullptr);};}
};
BEGIN_MESSAGE_MAP(NetworkDialog,Form)
 ON_WM_TIMER()
END_MESSAGE_MAP()
inline void showNetworkIntegration(CWnd* owner){NetworkDialog dialog(owner);dialog.DoModal();}
}
