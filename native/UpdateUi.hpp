#pragma once
#include "Updates.hpp"
#include "UpdateManifest.hpp"
namespace udm {
// Shown at startup when a verified manifest names a newer version (unless that version was
// skipped), and from Help > Check for Update. Download hands the setup to UDM's own transfer.
inline void showUpdateAvailableDialog(CWnd* owner,const UpdateManifest& manifest,const Json& settings,std::function<void(const Json&)> saveSettings,std::function<void()> download){
 Form dialog("Update available",330,122,owner);dialog.dialogUnits=true;CWnd* startup=nullptr;
 // The button actions run from the form's OnCommand long after init has returned, so persist lives at function scope.
 auto persist=[&](const std::string& skip){auto next=settings;if(startup)next["CheckForUpdates"]=dialog.checked(startup);if(!skip.empty())next["SkippedUpdateVersion"]=skip;saveSettings(next);};
 dialog.init=[&]{
  dialog.label("UDM "+manifest.version+" is available. Installed version: "+std::string(currentProductVersion)+".",10,8,310,10);
  dialog.label("The setup ("+bytes((double)manifest.setupSize)+(manifest.published.empty()?"":", published "+manifest.published)+") is downloaded by UDM, checked against the signed manifest and its publisher signature, and then started. UDM closes while it installs.",10,22,310,30);
  if(!manifest.notes.empty())dialog.link("Release notes",10,58,100,[&]{ShellExecuteW(dialog.GetSafeHwnd(),L"open",wide(manifest.notes).c_str(),nullptr,nullptr,SW_SHOWNORMAL);});
  startup=dialog.check("Check for updates at startup",yes(settings,"CheckForUpdates",true),10,76,160,10);
  dialog.defaultButton(dialog.button("Download and install",10,98,96,[&]{persist("");dialog.close();download();}));
  dialog.button("Skip this version",112,98,86,[&]{persist(manifest.version);dialog.close();});
  dialog.button("Later",204,98,56,[&]{persist("");dialog.close();});
  dialog.cancel=[&]{persist("");dialog.close();};
 };dialog.DoModal();
}
inline void showUpdateDialog(CWnd* owner,const Json& settings,ReleaseLoader loader={}){
 if(!loader)loader=[settings](const Cancel& cancel){return fetchPublishedRelease(settings,cancel);};
 struct Result{std::mutex mutex;bool done=false;std::string error;PublishedRelease release;};
 auto result=std::make_shared<Result>();std::shared_ptr<Cancel> cancel;std::thread worker;
 Form dialog("Check for updates",430,170,owner);CWnd *status=nullptr,*retry=nullptr,*open=nullptr;std::string page;
 auto stop=[&]{if(cancel)cancel->stop=true;if(worker.joinable())worker.join();};
 auto start=[&]{stop();result=std::make_shared<Result>();cancel=std::make_shared<Cancel>();cancel->deadline=GetTickCount64()+15000;page.clear();status->SetWindowText(L"Checking published UDM releases...");retry->EnableWindow(FALSE);open->EnableWindow(FALSE);
  worker=std::thread([state=result,token=cancel,loader]{PublishedRelease release;std::string error;try{release=loader(*token);}catch(const Cancelled&){error="The update check was canceled or timed out.";}catch(const std::exception& e){error=e.what();}std::lock_guard<std::mutex> lock(state->mutex);state->release=std::move(release);state->error=std::move(error);state->done=true;});
 };
 dialog.init=[&]{dialog.label(std::string("Installed version: ")+currentProductVersion,13,12,404);status=dialog.label("",13,42,404,63);retry=dialog.button("Check again",13,125,106,start);open=dialog.button("Open release page",129,125,163,[&]{if(!page.empty())openRefreshPage(dialog.GetSafeHwnd(),page);});dialog.button("Close",302,125,115,[&]{if(cancel)cancel->stop=true;dialog.close();});start();};
 dialog.pulse=[&]{std::lock_guard<std::mutex> lock(result->mutex);if(!result->done)return;result->done=false;retry->EnableWindow(TRUE);if(!result->error.empty()){status->SetWindowText(cs("Could not check for updates.\r\n"+result->error));page.clear();open->EnableWindow(FALSE);return;}page=result->release.page;status->SetWindowText(cs(result->release.newer?"UDM "+result->release.version+" is available. Open its release page to review and download it.":"No newer stable release was found. Latest published version: "+result->release.version+"."));open->EnableWindow(TRUE);};
 try{dialog.DoModal();}catch(...){stop();throw;}stop();
}
}
