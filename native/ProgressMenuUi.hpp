// Included inside udm. Capture under the existing model snapshot lock; render
// and open the menu from values only. Refresh plans decode only when inputs change.
enum class ProgressMenuAction {CopyAddress,OpenFolder,OpenWith,PauseRecording,ResumeRecording};
struct ProgressMenuState {
 bool complete=false,refresh=false,canOpen=false,recording=false,recordingActive=false,recordingCanResume=false,saving=false;
};
class ProgressMenuCache {
 Json refreshInputs,adaptiveCiphertext;
 bool refresh=false;
public:
 ProgressMenuState capture(Manager& manager,const JobPtr& job){
  const auto& data=job->data;const bool active=manager.isActive(job),complete=str(data,"Status")=="Complete";
  Json inputs={{"active",active},{"complete",complete},{"offline",data.contains("OfflineProject")},
   {"request",hasNonemptyString(data,"ProtectedRequest")},{"requiresCapture",yes(data,"RequiresRequestCapture")},
   {"source",hasNonemptyString(data,"SourceUrl")},{"sabr",hasNonemptyString(data,"ProtectedSabr")},
   {"live",yes(data,"LiveRecording")},{"video",static_cast<bool>(job->video)},
   {"output",str(data,"MediaOutput")},{"url",str(data,"Url")}};
  const auto& adaptive=addressInputString(data,"ProtectedAdaptive");
  if(inputs!=refreshInputs||adaptive!=adaptiveCiphertext){
   refresh=manager.canRefreshAddress(job);refreshInputs=std::move(inputs);adaptiveCiphertext=adaptive;
  }
  ProgressMenuState result;result.complete=complete;result.refresh=refresh;
  result.canOpen=!manager.isRelocating(job)&&progressCompletedAction(data,active)==ProgressAction::Open;
  result.recording=yes(data,"LiveRecording")&&!complete;result.recordingActive=active;result.saving=liveSavePending(data);
  result.recordingCanResume=result.saving?(!active&&str(data,"Status")!="Queued"):str(data,"Status")!="Pausing";
  return result;
 }
};
// Presentation actions can remain available while another window saves. Own
// accepted/queued work must not be destroyed or reentered through a popup.
inline bool progressMenuChoiceAllowed(UINT choice,bool modelBusy,bool ownPending){
 if(ownPending)return false;
 if(choice==6||choice==8)return true;
 return choice>=1&&choice<=10&&!modelBusy;
}
inline void populateProgressMenu(CMenu& menu,const ProgressMenuState& state,bool modelBusy=false,bool ownPending=false){
 const auto flags=[&](UINT choice,bool eligible=true){return MF_STRING|((eligible&&progressMenuChoiceAllowed(choice,modelBusy,ownPending))?MF_ENABLED:MF_GRAYED);};
 menu.AppendMenu(flags(1,state.refresh),1,L"Refresh download address...");
 menu.AppendMenu(flags(2),2,L"Download properties...");menu.AppendMenu(flags(3),3,L"Copy download address");menu.AppendMenu(MF_SEPARATOR);
 menu.AppendMenu(flags(4),4,L"Open folder");menu.AppendMenu(flags(5,state.canOpen),5,L"Open with...");menu.AppendMenu(MF_SEPARATOR);
 menu.AppendMenu(flags(6),6,L"Hide progress window");menu.AppendMenu(flags(7),7,L"Customize download progress dialog...");menu.AppendMenu(flags(8),8,L"Download information...");
 menu.AppendMenu(flags(9,!state.complete),9,L"Additional completion options...");
 if(state.recording){menu.AppendMenu(MF_SEPARATOR);menu.AppendMenu(flags(10,state.recordingCanResume),10,state.saving?L"Retry save":state.recordingActive?L"Pause recording":L"Resume recording");}
}
