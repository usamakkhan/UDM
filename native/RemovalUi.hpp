#pragma once
#include "Ui.hpp"
namespace udm {
struct RemovalChoice {bool accepted=false,recycle=false,suppress=false;};
inline RemovalChoice confirmRemoval(CWnd* owner,bool completed,bool suppress){
 if(suppress)return {true,false,true};
 RemovalChoice choice;Form dialog("Confirm deletion of downloads",completed?278:249,completed?86:74,owner);dialog.dialogUnits=true;
 dialog.init=[&]{
  dialog.label(completed?"Are you sure you want to delete selected downloads from UDM list of downloads?":"The selected downloads are not complete. Are you sure you want to delete them from UDM list of downloads?",7,completed?6:7,completed?264:235,completed?18:27);
  CWnd* files=completed?dialog.check("Move completely downloaded files to the Recycle Bin as well.",false,12,26,259,18):nullptr;
  auto remember=dialog.check("Don't show this dialog again",false,completed?12:16,completed?47:57,completed?247:220,10);
  if(files)dialog.bind(files,[&dialog,files,remember]{bool selected=dialog.checked(files);remember->EnableWindow(!selected);if(selected)remember->SendMessage(BM_SETCHECK,BST_UNCHECKED);});
  dialog.accept=[&,files,remember]{choice={true,files&&dialog.checked(files),dialog.checked(remember)};dialog.close();};
  dialog.button("Yes",completed?76:62,completed?65:38,50,dialog.accept,14);
  dialog.defaultButton(dialog.button("No",completed?151:137,completed?65:38,50,[&]{dialog.close(IDCANCEL);},14));
 };dialog.DoModal();return choice;
}
inline void removeDownloadSelection(CWnd* owner,Manager& manager,const std::vector<JobPtr>& selected,
 std::function<RemovalChoice(CWnd*,bool,bool)> confirm=confirmRemoval,
 std::function<void(JobPtr,HWND)> recycle={}){
 if(selected.empty())return;
 bool completed=false;Json prefs;
 {Lock lock(manager.mutex);prefs=manager.state["Settings"];for(auto job:selected){
  if(!job||std::find(manager.jobs.begin(),manager.jobs.end(),job)==manager.jobs.end())throw std::runtime_error("A selected download is no longer in the list.");
  if(manager.isActive(job))throw std::runtime_error("Pause the download and wait until it stops before removing it.");
  for(auto other:manager.jobs)if(str(other->data,"ReplacementOf")==job->id())throw std::runtime_error("A pending replacement depends on this record. Remove that replacement first.");
  completed|=str(job->data,"Status")=="Complete";
 }}
 const auto key=completed?"SkipCompletedRemovalConfirmation":"SkipIncompleteRemovalConfirmation";
 auto choice=confirm(owner,completed,yes(prefs,key));if(!choice.accepted)return;
 if(!recycle)recycle=[&manager](JobPtr job,HWND window){manager.recycleCompleted(job,window);};
 for(auto job:selected){
  if(choice.recycle){bool eligible=false;{Lock lock(manager.mutex);eligible=str(job->data,"Status")=="Complete";}if(eligible)recycle(job,owner?owner->GetSafeHwnd():nullptr);}
  manager.remove(job);
 }
 // Never remember file deletion: a suppressed confirmation removes list entries only.
 if(choice.suppress&&!choice.recycle){Json current;{Lock lock(manager.mutex);current=manager.state["Settings"];}current[key]=true;manager.setSettings(current);}
}
}