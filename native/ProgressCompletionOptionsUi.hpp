// Included inside udm. Workers receive values only; the modal owns its task.
struct ProgressCompletionOptionsDraft {
 bool close=true,folder=false,open=false,wait=true;
 int seconds=30;
};
inline FileInfoDecisionResult applyProgressCompletionOptions(Manager& manager,const JobPtr& job,const ProgressCompletionOptionsDraft& draft){
 if(draft.seconds<15||draft.seconds>3600)throw std::runtime_error("Use a countdown from 15 to 3600 seconds.");
 Lock lock(manager.mutex);
 if(std::find(manager.jobs.begin(),manager.jobs.end(),job)==manager.jobs.end())throw std::runtime_error("Unknown download.");
 const auto before=job->data;
 if(str(before,"Status")=="Complete")throw std::runtime_error("Set completion actions before the download finishes.");
 auto actions=completionSteps(before);actions.erase(std::remove(actions.begin(),actions.end(),"Open downloaded file"),actions.end());
 if(showDownloadCompletionDialog(before,yes(manager.state["Settings"],"SuppressCompletionDialog")))actions.clear();
 if(draft.open)actions.push_back("Open downloaded file");
 try{
  job->data["CloseProgressOnCompletion"]=draft.close;job->data["OpenFolderOnCompletion"]=draft.folder;
  manager.setCompletionPlan(job,actions,yes(before,"CompletionForce")&&completionCanForce(actions),draft.seconds,draft.wait);
 }catch(...){job->data=before;throw;}
 return {};
}
inline void progressCompletionOptions(CWnd* owner,Manager& manager,JobPtr job,const Json& record){
 if(str(record,"Status")=="Complete")return;
 Form dialog("Additional completion options",455,269,owner);
 FileInfoDecisionTask task;bool handling=false;std::vector<std::pair<HWND,BOOL>> enablement;
 const auto restore=[&]{for(const auto& item:enablement)if(IsWindow(item.first))::EnableWindow(item.first,item.second);enablement.clear();};
 dialog.cancel=[&]{if(!task.valid()&&!handling)dialog.close(IDCANCEL);};
 dialog.init=[&]{
  auto close=dialog.check("Close this progress window when done",yes(record,"CloseProgressOnCompletion",true),14,15,427);
  auto folder=dialog.check("Open the download folder when done",yes(record,"OpenFolderOnCompletion"),14,47,427);
  const auto steps=completionSteps(record);auto open=dialog.check("Open the downloaded file after the countdown",std::find(steps.begin(),steps.end(),"Open downloaded file")!=steps.end(),14,79,427);
  dialog.label("Countdown (seconds)",14,117,240);auto delay=dialog.edit(std::to_string(num(record,"CompletionActionDelay",30)),282,113,159);
  auto wait=dialog.check("Wait for other active and queued downloads",yes(record,"CompletionWaitForOthers",true),14,151,427);
  dialog.label("Power, disconnect and exit choices are on the completion tab.",14,185,427,30);
  dialog.accept=[&,close,folder,open,delay,wait]{
   if(task.valid()||handling)return;
   const auto seconds=std::stoll(text(delay));if(seconds<15||seconds>3600)throw std::runtime_error("Use a countdown from 15 to 3600 seconds.");
   ProgressCompletionOptionsDraft draft{dialog.checked(close),dialog.checked(folder),dialog.checked(open),dialog.checked(wait),static_cast<int>(seconds)};
   enablement.clear();
   EnumChildWindows(dialog.GetSafeHwnd(),[](HWND control,LPARAM value)->BOOL{wchar_t kind[32]{};GetClassNameW(control,kind,32);if((GetWindowLongPtrW(control,GWL_STYLE)&WS_TABSTOP)||!_wcsicmp(kind,L"BUTTON")||!_wcsicmp(kind,L"EDIT"))reinterpret_cast<std::vector<std::pair<HWND,BOOL>>*>(value)->push_back({control,IsWindowEnabled(control)});return TRUE;},reinterpret_cast<LPARAM>(&enablement));
   task.startAction(manager,[source=&manager,target=job,draft]{return applyProgressCompletionOptions(*source,target,draft);},dialog.taskNotifier());
   for(const auto& item:enablement)::EnableWindow(item.first,FALSE);
  };
  dialog.defaultButton(dialog.button("OK",261,229,85,dialog.accept));dialog.button("Cancel",356,229,85,dialog.cancel);
  dialog.pulse=[&]{
   if(handling||!task.valid()||!task.ready())return;
   handling=true;bool saved=false;
   try{task.get();saved=true;}
   catch(const std::exception& failure){error(&dialog,failure);}
   catch(...){error(&dialog,std::runtime_error("The completion options could not be saved. Your entries are still available."));}
   task.release();restore();handling=false;
   if(saved)dialog.close();
  };
 };
 dialog.DoModal();
}
