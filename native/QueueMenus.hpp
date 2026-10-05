// Included inside MainWindow.
 std::vector<std::string> startQueueNames,stopQueueNames;
 static bool isQueueMenuCommand(UINT id){return id>=32000&&id<34000;}
 void fillQueueCommands(CMenu& popup,bool start){
  auto& names=start?startQueueNames:stopQueueNames;names=queueNames(manager);
  while(popup.GetMenuItemCount()>0)popup.DeleteMenu(0,MF_BYPOSITION);
  popup.AppendMenuW(MF_STRING,start?CMD_STARTQUEUE:CMD_STOPQUEUE,start?L"Start":L"Stop");
  if(!names.empty())popup.AppendMenuW(MF_SEPARATOR);
  for(size_t i=0;i<names.size()&&i<1000;++i){auto label=wide(names[i]);size_t at=0;while((at=label.find(L'&',at))!=std::wstring::npos){label.insert(at,1,L'&');at+=2;}popup.AppendMenuW(MF_STRING,(start?32000:33000)+(UINT)i,label.c_str());}
  if(names.size()>1000)popup.AppendMenuW(MF_STRING,start?CMD_STARTQUEUEMENU:CMD_STOPQUEUEMENU,L"Choose queue...");
 }
 bool runQueueMenuCommand(UINT id){
  if(!isQueueMenuCommand(id))return false;bool start=id<33000;auto& names=start?startQueueNames:stopQueueNames;auto index=id-(start?32000:33000);
  if(index>=names.size())throw std::runtime_error("Open the queue menu again.");
  manager.queueRun(names[index],start);return true;
 }
 void updateQueuePopup(CMenu* popup){
  if(!popup||!menu.GetSafeHmenu())return;auto downloads=menu.GetSubMenu(2);if(!downloads)return;
  for(int position:{9,10}){auto child=downloads->GetSubMenu(position);if(child&&child->GetSafeHmenu()==popup->GetSafeHmenu()){fillQueueCommands(*popup,position==9);return;}}
 }
