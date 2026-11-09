// Included inside udm after Form, prompt and queueNames.
inline std::optional<DownloadQueueChoice> chooseDownloadQueue(CWnd* owner,Manager& manager,std::string initial,bool start=false,const std::function<bool(CWnd*)>& schedule={}){
 DownloadQueueChoice choice{initial,start};auto names=queueNames(manager);auto original=names;Form dialog("Select a queue",161,schedule?108:89,owner);dialog.dialogUnits=true;
 dialog.init=[&]{
  dialog.label("Add files to the queue:",7,7,143);auto queue=dialog.queueCombo(manager,names,initial,7,18,126);
  dialog.button("+",138,17,16,[&,queue]{auto name=prompt(&dialog,"Create queue");if(name.empty())return;for(auto& value:names)if(lower(value)==lower(name))throw std::runtime_error("That queue already exists.");names.push_back(name);queue->addQueue(name,name);queue->selectQueue(name);});
  auto run=dialog.check("Start queue processing",start,7,34,147);auto remember=dialog.check("Don't ask me again",false,7,74,147);
  dialog.accept=[&,queue,run,remember]{choice.name=queue->queueName();if(choice.name.empty())throw std::runtime_error("Choose a queue.");choice.start=dialog.checked(run);choice.dontAsk=dialog.checked(remember);choice.create=std::find(original.begin(),original.end(),choice.name)==original.end();dialog.close();};
  dialog.defaultButton(dialog.button("OK",55,53,50,dialog.accept));
  if(schedule)dialog.button("&Schedule...",7,90,64,[&]{if(schedule(&dialog))dialog.close(IDCANCEL);});
 };if(dialog.DoModal()!=IDOK)return std::nullopt;return choice;
}
