// Included inside udm after Form, prompt and queueNames.
inline std::optional<DownloadQueueChoice> chooseDownloadQueue(CWnd* owner,Manager& manager,std::string initial,bool start=false,const std::function<bool(CWnd*)>& schedule={},const QueueChoiceView* prepared=nullptr){
 QueueChoiceTask task;DownloadQueueChoice choice{initial,start};std::vector<std::string> names,original;bool loaded=false;CWnd* retry=nullptr;std::function<void()> begin;
 Form dialog("Select a queue",161,schedule?108:89,owner);dialog.dialogUnits=true;dialog.pulsePeriod=40;
 dialog.init=[&]{
  auto label=dialog.label("Reading queues...",7,7,143);auto queue=dialog.queueCombo(names,QueueLabels{},initial,7,18,126);
  auto create=dialog.button("+",138,17,16,[&,queue]{if(!loaded)return;auto name=prompt(&dialog,"Create queue");if(name.empty())return;for(auto& value:names)if(lower(value)==lower(name))throw std::runtime_error("That queue already exists.");names.push_back(name);queue->addQueue(name,name);queue->selectQueue(name);});
  auto run=dialog.check("Start queue processing",start,7,34,147);auto remember=dialog.check("Don't ask me again",false,7,74,147);
  dialog.accept=[&,queue,run,remember]{if(!loaded)return;choice.name=queue->queueName();if(choice.name.empty())throw std::runtime_error("Choose a queue.");choice.start=dialog.checked(run);choice.dontAsk=dialog.checked(remember);choice.create=std::find(original.begin(),original.end(),choice.name)==original.end();dialog.close();};
  auto ok=dialog.button("OK",55,53,50,dialog.accept);dialog.defaultButton(ok);
  dialog.cancel=[&]{task.cancelRead();dialog.close(IDCANCEL);};dialog.button("Cancel",111,53,43,dialog.cancel);
  CWnd* scheduleButton=nullptr;if(schedule)scheduleButton=dialog.button("&Schedule...",7,90,64,[&]{if(loaded&&schedule(&dialog))dialog.close(IDCANCEL);});
  const auto enable=[=](bool enabled){for(auto control:std::vector<CWnd*>{queue,create,run,remember,ok,scheduleButton})if(control)control->EnableWindow(enabled);};
  const auto failed=[label,&retry]{label->SetWindowText(L"Queue list unavailable.");retry->ShowWindow(SW_SHOW);};
  begin=[&,enable,label,failed]{enable(false);retry->ShowWindow(SW_HIDE);label->SetWindowText(L"Reading queues...");try{
#ifdef UDM_TOOLBAR_COMPONENT_TEST
   task.start(manager,dialog.taskNotifier(),queueChoiceTestObserver);
#else
   task.start(manager,dialog.taskNotifier());
#endif
  }catch(const std::exception&){failed();}};
  retry=dialog.button("Retry",schedule?82:7,schedule?90:53,schedule?72:43,[&]{if(!loaded&&!task.valid())begin();});retry->ShowWindow(SW_HIDE);
  const auto populate=[&,queue,label,enable](const QueueChoiceView& view){names=view.names;original=names;for(const auto& name:names)queue->addQueue(name,queueDisplayLabel(view.labels,name));if(!queue->selectQueue(initial)&&queue->GetCount()>0)queue->SetCurSel(0);loaded=true;label->SetWindowText(L"Add files to the queue:");enable(true);retry->ShowWindow(SW_HIDE);};
  if(prepared)populate(*prepared);else{
   // Retain callable state by value: pulse runs after init returns.
   dialog.pulse=[&,populate,failed]{if(!task.ready())return;auto result=task.take();if(result.cancelled){dialog.cancel();return;}if(!result.error.empty()){failed();return;}populate(result.view);};begin();
  }
 };const auto result=dialog.DoModal();task.finish();if(result!=IDOK)return std::nullopt;return choice;
}
