#pragma once
#include "Ui.hpp"
namespace udm {
struct RemovalChoice {bool accepted=false,recycle=false,suppress=false;};
inline RemovalChoice confirmRemovalGroup(CWnd* owner,bool completed,bool suppress,size_t count,bool mixed){
 if(suppress)return {true,false,true};
 RemovalChoice choice;Form dialog("Confirm deletion of downloads",completed?278:249,completed?86:74,owner);dialog.dialogUnits=true;
 dialog.init=[&]{
  const auto message=completed?"Are you sure you want to delete selected downloads from UDM list of downloads?":mixed?"There are incomplete downloads among selected ones. Are you sure you want to delete them from UDM list of downloads as well?":count==1?"The selected download is not complete. Are you sure you want to delete it from UDM list of downloads?":"The selected downloads are not complete. Are you sure you want to delete them from UDM list of downloads?";
  dialog.label(message,7,completed?6:7,completed?264:235,completed?18:27);
  CWnd* files=completed?dialog.check("Move completely downloaded files to the Recycle Bin as well.",false,12,26,259,18):nullptr;
  auto remember=dialog.check("Don't show this dialog again",false,completed?12:16,completed?47:57,completed?247:220,10);
  if(files)dialog.bind(files,[&dialog,files,remember]{bool selected=dialog.checked(files);remember->EnableWindow(!selected);if(selected)remember->SendMessage(BM_SETCHECK,BST_UNCHECKED);});
  dialog.accept=[&,files,remember]{choice={true,files&&dialog.checked(files),dialog.checked(remember)};dialog.close();};
  dialog.button("Yes",completed?76:62,completed?65:38,50,dialog.accept,14);
  dialog.defaultButton(dialog.button("No",completed?151:137,completed?65:38,50,[&]{dialog.close(IDCANCEL);},14));
 };dialog.DoModal();return choice;
}
inline RemovalChoice confirmRemoval(CWnd* owner,bool completed,bool suppress){return confirmRemovalGroup(owner,completed,suppress,0,false);}
inline void removeDownloadSelection(CWnd* owner,Manager& manager,const std::vector<JobPtr>& selected,
 std::function<RemovalChoice(CWnd*,bool,bool)> confirm={},
 std::function<void(JobPtr,HWND)> recycle={}){
 if(selected.empty())return;
 size_t completed=0,incomplete=0;Json prefs;std::vector<std::pair<JobPtr,bool>> targets;
 {Lock lock(manager.mutex);prefs=manager.state["Settings"];for(auto job:selected){
  if(!job||std::find(manager.jobs.begin(),manager.jobs.end(),job)==manager.jobs.end())throw std::runtime_error("A selected download is no longer in the list.");
  if(manager.isActive(job))throw std::runtime_error("Pause the download and wait until it stops before removing it.");
  for(auto other:manager.jobs)if(str(other->data,"ReplacementOf")==job->id())throw std::runtime_error("A pending replacement depends on this record. Remove that replacement first.");
  const bool complete=str(job->data,"Status")=="Complete";if(complete)++completed;else ++incomplete;targets.emplace_back(job,complete);
 }}
 auto ask=[&](bool complete,size_t count,bool mixed){const auto suppress=yes(prefs,complete?"SkipCompletedRemovalConfirmation":"SkipIncompleteRemovalConfirmation");return confirm?confirm(owner,complete,suppress):confirmRemovalGroup(owner,complete,suppress,count,mixed);};
 RemovalChoice finished,unfinished;
 if(completed){finished=ask(true,completed,false);if(!finished.accepted)return;}
 if(incomplete){unfinished=ask(false,incomplete,completed!=0);if(!unfinished.accepted&&!completed)return;}
 if(!recycle)recycle=[&manager](JobPtr job,HWND window){manager.recycleCompleted(job,window,true);};
 for(const auto& target:targets){const auto& job=target.first;const bool complete=target.second;if(!complete&&!unfinished.accepted)continue;
  {Lock lock(manager.mutex);if((str(job->data,"Status")=="Complete")!=complete)throw std::runtime_error("A selected download changed while removal was being confirmed. Review the selection and try again.");}
  if(complete&&finished.recycle)recycle(job,owner?owner->GetSafeHwnd():nullptr);
  manager.remove(job,complete);
 }
 // Never remember file deletion: a suppressed confirmation removes list entries only.
 const bool rememberFinished=finished.accepted&&finished.suppress&&!finished.recycle;
 const bool rememberUnfinished=unfinished.accepted&&unfinished.suppress;
 if(rememberFinished||rememberUnfinished){Json current;{Lock lock(manager.mutex);current=manager.state["Settings"];}if(rememberFinished)current["SkipCompletedRemovalConfirmation"]=true;if(rememberUnfinished)current["SkipIncompleteRemovalConfirmation"]=true;manager.setSettings(current);}
}
}
