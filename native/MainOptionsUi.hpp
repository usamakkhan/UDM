// Included inside MainWindow. All state here belongs to its UI thread.
 bool optionsUiActive=false,optionsRefreshPending=false,optionsQueueDispatchPending=false;
 std::optional<Json> optionsAppearanceSnapshot;
 std::optional<std::map<UINT,bool>> optionsAppearanceCommands;
 std::function<void(Options&)> configureOptionsDialog;
 void dispatchOptionsQueue(){
  std::unique_lock<std::recursive_mutex> lock(manager.mutex,std::try_to_lock);if(!lock.owns_lock())return;
  manager.dispatchQueueWork();optionsQueueDispatchPending=false;
 }
 void tickOptionsModel(){
  std::string monitorFailure,tickFailure;
  {std::unique_lock<std::recursive_mutex> lock(manager.mutex,std::try_to_lock);if(!lock.owns_lock())return;
   try{if(networkCapture)networkCapture->poll();}catch(const std::exception& e){monitorFailure=e.what();}
   try{if(optionsQueueDispatchPending){manager.dispatchQueueWork();optionsQueueDispatchPending=false;}manager.tick();}catch(const std::exception& e){tickFailure=e.what();}
  }
  if(!monitorFailure.empty())showStatus(monitorFailure);if(!tickFailure.empty())showStatus(tickFailure);
 }
 bool finishOptionsAppearance(){
  if(!optionsRefreshPending)return true;if(optionsUiActive||optionsAppearanceSnapshot)return false;
  // Read Windows controls before taking the model lock. Validate their JobPtrs
  // only inside that lock; the UI snapshots never mutate the catalog.
  std::vector<JobPtr> chosen;for(int i=-1;(i=table.GetNextItem(i,LVNI_SELECTED))>=0;)if(i<(int)visible.size())chosen.push_back(visible[i]);
  std::vector<UINT> commands;for(int i=0;i<ToolbarCommandCount;++i)commands.push_back(i==3?CMD_STOP_CLOSE_ALL:toolbarCommand(i));commands.push_back(CMD_ZIP);commands.push_back(CMD_RECYCLE);
  Json settings;std::map<UINT,bool> availability;
  {std::unique_lock<std::recursive_mutex> lock(manager.mutex,std::try_to_lock);if(!lock.owns_lock())return false;
   chosen.erase(std::remove_if(chosen.begin(),chosen.end(),[&](const JobPtr& job){return std::find(manager.jobs.begin(),manager.jobs.end(),job)==manager.jobs.end();}),chosen.end());settings=manager.state["Settings"];availability=commandStates(commands,&chosen);
  }
  optionsAppearanceSnapshot=std::move(settings);optionsAppearanceCommands=std::move(availability);optionsRefreshPending=false;
  struct Restore{MainWindow& owner;~Restore(){owner.optionsAppearanceCommands.reset();owner.optionsAppearanceSnapshot.reset();}} restore{*this};
  applyAppearance();historyRefreshDeferred=true;requestDeferredHistoryRefresh();PostMessage(PRESENT_CAPTURE);PostMessage(TRANSFER_EVENTS);PostMessage(DISPATCH_QUEUE_WORK);return true;
 }
 void openMainOptions(){
  if(optionsUiActive||optionsAppearanceSnapshot)return;optionsUiActive=true;
  try{
   Options dialog(manager,this);if(configureOptionsDialog)configureOptionsDialog(dialog);
   dialog.help=[this]{showHelpTopic(0);};dialog.networkIntegration=[this](CWnd* owner){if(!networkCapture)networkCapture=std::make_unique<NetworkCaptureSession>(&manager);NetworkDialog network(owner,&manager,networkCapture.get());network.DoModal();};dialog.DoModal();
  }catch(...){optionsUiActive=false;optionsRefreshPending=true;throw;}
  optionsUiActive=false;optionsRefreshPending=true;finishOptionsAppearance();
 }
