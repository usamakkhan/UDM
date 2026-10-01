// Included inside udm after Form, prompt and queueNames.
struct DownloadQueueChoice {std::string name;bool start=false,dontAsk=false,create=false;};
inline std::optional<DownloadQueueChoice> chooseDownloadQueue(CWnd* owner,Manager& manager,std::string initial,bool start=false){
 DownloadQueueChoice choice{initial,start};auto names=queueNames(manager);auto original=names;Form dialog("Select a queue",161,89,owner);dialog.dialogUnits=true;
 dialog.init=[&]{
  dialog.label("Add files to the queue:",7,7,143);auto queue=dialog.combo(names,initial,7,18,126);
  dialog.button("+",138,17,16,[&,queue]{auto name=prompt(&dialog,"Create queue");if(name.empty())return;for(auto& value:names)if(lower(value)==lower(name))throw std::runtime_error("That queue already exists.");names.push_back(name);queue->AddString(cs(name));queue->SetCurSel(queue->FindStringExact(-1,cs(name)));});
  auto run=dialog.check("Start queue processing",start,7,34,147);auto remember=dialog.check("Do not ask again",false,7,74,147);
  dialog.accept=[&,queue,run,remember]{choice.name=text(queue);if(choice.name.empty())throw std::runtime_error("Choose a queue.");choice.start=dialog.checked(run);choice.dontAsk=dialog.checked(remember);choice.create=std::find(original.begin(),original.end(),choice.name)==original.end();dialog.close();};
  dialog.defaultButton(dialog.button("OK",55,53,50,dialog.accept));
 };if(dialog.DoModal()!=IDOK)return std::nullopt;return choice;
}
inline void ensureDownloadQueue(Manager& manager,const DownloadQueueChoice& choice){
 Lock lock(manager.mutex);bool found=false;for(auto& queue:manager.state["Queues"])if(lower(str(queue,"Name"))==lower(choice.name)){if(str(queue,"Name")!=choice.name)throw std::runtime_error("A queue with that name already exists.");found=true;}
 if(!found){if(!choice.create)throw std::runtime_error("The selected queue no longer exists.");auto queue=defaultQueue(choice.name);queue["Enabled"]=false;manager.setQueue(queue);}
}
inline void finishDownloadQueue(Manager& manager,const DownloadQueueChoice& choice,const char* preference){
 if(choice.start)manager.queueRun(choice.name,true);
 if(choice.dontAsk){Lock lock(manager.mutex);auto prefs=manager.state["Settings"];prefs[preference]=false;manager.setSettings(prefs);}
}
