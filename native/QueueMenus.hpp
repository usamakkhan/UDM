// Included inside MainWindow.
 std::vector<std::string> startQueueNames,stopQueueNames,moveQueueNames;
 int moveQueuePosition=-1;// position of the "Move to queue" submenu in the last download context menu (popup items cannot be found by command id)
 static bool isQueueMenuCommand(UINT id){return id>=32000&&id<35000;}
 static std::wstring menuLabel(const std::string& name){auto label=wide(name);size_t at=0;while((at=label.find(L'&',at))!=std::wstring::npos){label.insert(at,1,L'&');at+=2;}return label;}
 // IDM lists the destination queues directly under "Move to queue"; ids 34000+ pick one for the current selection.
 void fillMoveQueueCommands(CMenu& popup,const std::vector<JobPtr>& selection){
  moveQueueNames=membershipQueues(selection);while(popup.GetMenuItemCount()>0)popup.DeleteMenu(0,MF_BYPOSITION);
  for(size_t i=0;i<moveQueueNames.size()&&i<1000;++i)popup.AppendMenuW(MF_STRING,34000+(UINT)i,menuLabel(moveQueueNames[i]).c_str());
  if(moveQueueNames.size()>1000)popup.AppendMenuW(MF_STRING,CMD_MOVEQUEUE,L"Choose queue...");
 }
 void fillQueueCommands(CMenu& popup,bool start){
  auto& names=start?startQueueNames:stopQueueNames;names=queueNames(manager);
  while(popup.GetMenuItemCount()>0)popup.DeleteMenu(0,MF_BYPOSITION);
  for(size_t i=0;i<names.size()&&i<1000;++i)popup.AppendMenuW(MF_STRING,(start?32000:33000)+(UINT)i,menuLabel(std::string(start?"Start ":"Stop ")+names[i]).c_str());
  if(names.size()>1000)popup.AppendMenuW(MF_STRING,start?CMD_STARTQUEUEMENU:CMD_STOPQUEUEMENU,L"Choose queue...");
 }
 bool runQueueMenuCommand(UINT id){
  if(!isQueueMenuCommand(id))return false;
  if(id>=34000){auto index=id-34000;if(index>=moveQueueNames.size())throw std::runtime_error("Open the queue menu again.");auto chosen=selected();if(chosen.empty()||!commandEnabled(CMD_MOVEQUEUE))return true;setQueueMembershipBatch(manager,chosen,true,moveQueueNames[index]);return true;}
  bool start=id<33000;auto& names=start?startQueueNames:stopQueueNames;auto index=id-(start?32000:33000);
  if(index>=names.size())throw std::runtime_error("Open the queue menu again.");
  if(start||manager.canStopQueue(names[index]))manager.queueRun(names[index],start);return true;
 }
 void updateQueuePopup(CMenu* popup){
  if(!popup||!menu.GetSafeHmenu())return;auto downloads=menu.GetSubMenu(2);if(!downloads)return;
  for(int position:{9,10}){auto child=downloads->GetSubMenu(position);if(child&&child->GetSafeHmenu()==popup->GetSafeHmenu()){fillQueueCommands(*popup,position==9);return;}}
 }
