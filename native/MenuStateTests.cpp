#define UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
#include "App.cpp"
#include "TransferFixture.hpp"
namespace udm {
static Json checks=Json::array();
static fs::path checkpointRoot;
static void expect(bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!checkpointRoot.empty())atomicText(checkpointRoot/L"progress.json",checks.dump(2),false);if(!ok)throw std::runtime_error(name);}
class CapturePresentationComponentTest {
public:static void run(Manager& manager,MainWindow& frame){
 auto updateHelp=frame.GetMenu()->GetSubMenu(4);expect(updateHelp->GetMenuItemID(9)==CMD_CHECK_UPDATES,"Help menu contains the update check command at reference position");frame.SendMessage(WM_INITMENUPOPUP,(WPARAM)updateHelp->GetSafeHmenu(),MAKELPARAM(4,FALSE));expect((updateHelp->GetMenuState(CMD_CHECK_UPDATES,MF_BYCOMMAND)&(MF_DISABLED|MF_GRAYED))==0,"Update check command is enabled without download selection");
 expect(updateHelp->GetMenuItemCount()==13,"Help menu matches reference entry count including separators");
 expect(updateHelp->GetMenuItemID(6)==CMD_HOME&&updateHelp->GetMenuItemID(7)==CMD_SUPPORT&&updateHelp->GetMenuItemID(11)==CMD_ABOUT&&updateHelp->GetMenuItemID(12)==CMD_SHARE,"Help product and sharing actions occupy reference positions");
 expect((updateHelp->GetMenuState(CMD_HOME,MF_BYCOMMAND)&(MF_DISABLED|MF_GRAYED))==0&&(updateHelp->GetMenuState(CMD_SUPPORT,MF_BYCOMMAND)&(MF_DISABLED|MF_GRAYED))==0&&(updateHelp->GetMenuState(CMD_SHARE,MF_BYCOMMAND)&(MF_DISABLED|MF_GRAYED))==0,"Help product actions remain available without a download selection");
 auto paused=manager.add("https://example.invalid/paused.zip","","paused.zip","Main queue",true);
 auto complete=manager.add("https://example.invalid/complete.txt","","complete.txt","Main queue",true);
 paused->data["Status"]="Paused";complete->data["Status"]="Complete";complete->data["QueueMember"]=false;fs::create_directories(complete->target().parent_path());writeBytes(complete->target(),Bytes{'x'});
 frame.refresh();
 auto select=[&](const std::vector<JobPtr>& jobs){frame.table.SetItemState(-1,0,LVIS_SELECTED);for(int i=0;i<(int)frame.visible.size();++i)if(std::find(jobs.begin(),jobs.end(),frame.visible[i])!=jobs.end())frame.table.SetItemState(i,LVIS_SELECTED,LVIS_SELECTED);};
 // Queue views and the Q column must describe membership, not a remembered destination.
 {
 auto savedPaused=paused->data,savedComplete=complete->data,savedQueues=manager.state["Queues"];
 auto audit=defaultQueue("Membership audit");audit["Enabled"]=false;manager.setQueue(audit);
 paused->data["Queue"]="Membership audit";paused->data["QueueMember"]=true;
 complete->data["Queue"]="Membership audit";complete->data["QueueMember"]=false;
 frame.filter="queue:Membership audit";frame.refresh();
 expect(frame.visible==std::vector<JobPtr>{paused},"Queue view excludes completed records that only remember the queue name");
 setQueueMembershipBatch(manager,{paused},false);frame.refresh();
 expect(frame.visible.empty(),"Removing the last member empties its queue view immediately");
 frame.filter="all";frame.refresh();select({paused});
 auto rowOf=[&](JobPtr job){return (int)(std::find(frame.visible.begin(),frame.visible.end(),job)-frame.visible.begin());};
 expect(frame.textValue(rowOf(paused),1).empty()&&frame.textValue(rowOf(complete),1).empty(),"Nonmembers have an empty Q column despite remembered queue names");
 CMenu addMenu;frame.fillDownloadMenu(addMenu);CString membershipLabel;addMenu.GetMenuString(CMD_MOVEQUEUE,membershipLabel,MF_BYCOMMAND);
 expect(membershipLabel==L"Add to queue","Removed selection offers Add to queue");
 setQueueMembershipBatch(manager,{paused},true,"Membership audit");frame.refresh();select({paused});
 expect(frame.queueColumnText(paused->data)=="Membership audit"&&frame.queueImageFor(paused->data)>=0,"Re-enrolled member regains its queue indicator");
 frame.sortColumn=1;frame.ascending=true;frame.refresh();expect(frame.visible.front()==complete,"Q sorting uses visible membership rather than a remembered destination");frame.sortColumn=-1;frame.refresh();select({paused});
 CMenu moveMenu;frame.fillDownloadMenu(moveMenu);moveMenu.GetMenuString(CMD_MOVEQUEUE,membershipLabel,MF_BYCOMMAND);
 expect(membershipLabel==L"Move to queue","Existing member offers Move to queue");
 paused->data.erase("QueueMember");frame.filter="queue:Membership audit";frame.refresh();
 expect(frame.visible==std::vector<JobPtr>{paused},"Legacy unfinished records retain implicit membership");
 complete->data.erase("QueueMember");frame.refresh();
 expect(frame.visible==std::vector<JobPtr>{paused},"Legacy completed records do not acquire implicit membership");
 paused->data["QueueMember"]=false;complete->data["QueueMember"]=true;audit["Synchronize"]=true;manager.setQueue(audit);frame.refresh();
 expect(frame.visible==std::vector<JobPtr>{complete},"Explicit completed synchronization member remains in queue view");
 frame.filter="all";frame.refresh();select({paused,complete});CMenu mixedMenu;frame.fillDownloadMenu(mixedMenu);mixedMenu.GetMenuString(CMD_MOVEQUEUE,membershipLabel,MF_BYCOMMAND);
 expect(membershipLabel==L"Move to queue","Mixed membership selection offers a single Move to queue action");
 paused->data=savedPaused;complete->data=savedComplete;manager.setQueues(savedQueues);frame.filter="all";frame.refresh();select({});
 }
 // Real subitem and tree icons follow queue configuration and membership.
 {
 auto previousQueues=manager.state["Queues"],previousData=paused->data;auto oldFilter=frame.filter;
 std::set<int> kinds;
 for(int kind=0;kind<8;++kind){
  auto queue=defaultQueue(kind&1?"Indicator queue":"Main queue");queue["Enabled"]=false;queue["Synchronize"]=(kind&2)!=0;queue["Scheduled"]=(kind&4)!=0;manager.setQueue(queue);
  paused->data["Queue"]=str(queue,"Name");paused->data["QueueMember"]=true;frame.filter="all";frame.refresh();int row=(int)(std::find(frame.visible.begin(),frame.visible.end(),paused)-frame.visible.begin());
  LVITEMW item{};item.mask=LVIF_IMAGE;item.iItem=row;item.iSubItem=1;expect(frame.table.GetItem(&item)!=FALSE&&item.iImage==frame.queueImages[kind],"Q cell has the correct main/additional, download/sync, scheduled image");kinds.insert(item.iImage);
  expect(frame.textValue(row,1).empty(),"Q cell shows only its indicator without clipped queue-name text");
  auto root=frame.tree.GetRootItem();while(auto next=frame.tree.GetNextSiblingItem(root))root=next;bool found=false;
  for(auto child=frame.tree.GetChildItem(root);child;child=frame.tree.GetNextSiblingItem(child))if(frame.tree.GetItemText(child)==cs(str(queue,"Name"))){int image=-1,selected=-1;frame.tree.GetItemImage(child,image,selected);found=image==item.iImage&&selected==item.iImage;}
  expect(found,"Queue tree uses the same state indicator as the download cell");
  wchar_t tipText[512]{};NMLVGETINFOTIPW tip{};tip.hdr={frame.table.GetSafeHwnd(),502,LVN_GETINFOTIPW};tip.iItem=row;tip.pszText=tipText;tip.cchTextMax=512;frame.SendMessage(WM_NOTIFY,502,(LPARAM)&tip);
  auto tipValue=utf8(tipText);expect(tipValue.find(str(queue,"Name"))!=std::string::npos&&tipValue.find(kind&2?"Synchronization queue":"Download queue")!=std::string::npos&&((tipValue.find("scheduled start")!=std::string::npos)==bool(kind&4)),"Native infotip identifies queue name, type and configured schedule");
 }
 expect(kinds.size()==8,"All eight queue states have distinct native image slots");
 auto row=(int)(std::find(frame.visible.begin(),frame.visible.end(),paused)-frame.visible.begin());paused->data["QueueMember"]=false;frame.refresh();LVITEMW removed{};removed.mask=LVIF_IMAGE;removed.iItem=row;removed.iSubItem=1;frame.table.GetItem(&removed);expect(removed.iImage==I_IMAGENONE,"Removing membership immediately clears the native subitem image");
 paused->data["QueueMember"]=true;paused->data["Queue"]="Missing queue";frame.refresh();frame.table.GetItem(&removed);expect(removed.iImage==I_IMAGENONE,"Unknown queue does not display a misleading membership image");
 CClientDC screen(&frame);for(int side:{16,20,32}){std::set<std::string> hashes;for(int kind=0;kind<8;++kind){CImage image;image.Attach(queueIndicatorBitmap(screen,side,kind));auto path=checkpointRoot/wide("queue-"+std::to_string(side)+"-"+std::to_string(kind)+".png");expect(SUCCEEDED(image.Save(path.c_str())),"Queue indicator renders at the requested scale");hashes.insert(fileHash(path));}expect(hashes.size()==8,"Rendered queue states remain visually distinct at each tested scale");}
 int side=frame.px(16);CImage board;board.Create(side*8,side*2,32);auto dc=board.GetDC();RECT top{0,0,side*8,side},bottom{0,side,side*8,side*2};FillRect(dc,&top,(HBRUSH)GetStockObject(WHITE_BRUSH));FillRect(dc,&bottom,(HBRUSH)GetStockObject(BLACK_BRUSH));for(int kind=0;kind<8;++kind){ImageList_Draw(frame.images.GetSafeHandle(),frame.queueImages[kind],dc,side*kind,0,ILD_TRANSPARENT);ImageList_Draw(frame.images.GetSafeHandle(),frame.queueImages[kind],dc,side*kind,side,ILD_TRANSPARENT);}board.ReleaseDC();expect(SUCCEEDED(board.Save((checkpointRoot/L"queue-indicators.png").c_str())),"Actual masked image-list glyphs render on light and dark backgrounds");
 paused->data=previousData;manager.setQueues(previousQueues);frame.filter=oldFilter;frame.refresh();select({});
 }
 auto enabled=[&](int menu,UINT command){
  std::function<std::pair<CMenu*,UINT>(CMenu*,UINT)> locate=[&](CMenu* popup,UINT index)->std::pair<CMenu*,UINT>{
   for(int i=0;i<popup->GetMenuItemCount();++i){if(popup->GetMenuItemID(i)==command)return {popup,index};if(auto child=popup->GetSubMenu(i)){auto found=locate(child,(UINT)i);if(found.first)return found;}}return {nullptr,0};
  };
  auto target=locate(frame.GetMenu()->GetSubMenu(menu),(UINT)menu);UINT state=UINT(-1);
  if(target.first){frame.SendMessage(WM_INITMENUPOPUP,(WPARAM)target.first->GetSafeHmenu(),MAKELPARAM(target.second,FALSE));state=target.first->GetMenuState(command,MF_BYCOMMAND);}
  if(menu==1&&state==UINT(-1)){CMenu context;frame.fillDownloadMenu(context);state=context.GetMenuState(command,MF_BYCOMMAND);}
  return state!=UINT(-1)&&!(state&(MF_DISABLED|MF_GRAYED));
 };



 // Real tree-control coordinate routing, including monitors left of the primary.
 CRect originalWindow;frame.GetWindowRect(&originalWindow);
 frame.SetWindowPos(nullptr,-1200,100,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
 auto firstTree=frame.tree.GetRootItem();expect(firstTree!=nullptr,"Context fixture has a tree root");
 auto otherTree=frame.tree.GetNextSiblingItem(firstTree);expect(otherTree!=nullptr,"Context fixture has a second tree item");
 frame.tree.SelectItem(firstTree);frame.tree.EnsureVisible(otherTree);CRect itemRect;expect(frame.tree.GetItemRect(otherTree,&itemRect,TRUE)!=FALSE,"Second tree item has a hit rectangle");
 CPoint negativeClick=itemRect.CenterPoint();frame.tree.ClientToScreen(&negativeClick);expect(negativeClick.x<0,"Mouse context fixture uses negative desktop coordinates");
 expect(frame.treeMenuPoint(negativeClick)&&frame.tree.GetSelectedItem()==otherTree,"Negative-screen right click selects the clicked tree item");
 auto selectedBefore=frame.tree.GetSelectedItem();CPoint keyboardPoint(-1,-1);
 expect(frame.treeMenuPoint(keyboardPoint)&&frame.tree.GetSelectedItem()==selectedBefore,"Keyboard context preserves selected tree item");
 CPoint localKeyboard=keyboardPoint;frame.tree.ScreenToClient(&localKeyboard);
 expect(frame.tree.HitTest(localKeyboard)==selectedBefore,"Keyboard context anchors at the selected item");
 CPoint outside(-5000,-5000);expect(!frame.treeMenuPoint(outside)&&frame.tree.GetSelectedItem()==selectedBefore,"Off-tree mouse point does not reuse selected queue");
 frame.tree.SelectItem(nullptr);CPoint noSelection(-1,-1);expect(!frame.treeMenuPoint(noSelection),"Keyboard context without a selection is ignored");
 frame.tree.SelectItem(firstTree);frame.SetWindowPos(nullptr,originalWindow.left,originalWindow.top,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);frame.refresh();

 auto tasksMenu=frame.GetMenu()->GetSubMenu(0);const wchar_t* taskLabels[]={L"&Add new download",L"Add &batch download",L"Add batch download from &clipboard",L"Run site &grabber",L"",L"Show &drop target",L"",L"E&xport",L"&Import",L"",L"&Exit"};
 expect(tasksMenu->GetMenuItemCount()==11,"Tasks menu has reference eleven-position shape");
 for(int i=0;i<11;++i){CString label;tasksMenu->GetMenuString(i,label,MF_BYPOSITION);expect(label==taskLabels[i],"Tasks label and position match reference");}
 auto exportMenu=tasksMenu->GetSubMenu(7),importMenu=tasksMenu->GetSubMenu(8);
 expect(exportMenu&&exportMenu->GetMenuItemID(0)==CMD_EXPORT_IDM&&exportMenu->GetMenuItemID(1)==CMD_EXPORT_TEXT,"Export retains reference format routes first");
 expect(importMenu&&importMenu->GetMenuItemID(0)==CMD_IMPORT_IDM&&importMenu->GetMenuItemID(1)==CMD_IMPORT_TEXT,"Import retains reference format routes first");
 expect(exportMenu->GetMenuItemID(4)==CMD_RECOVERY,"Backup and recovery remains accessible within Export");
 auto fileMenu=frame.GetMenu()->GetSubMenu(1);
 expect(fileMenu->GetMenuItemCount()==4,"File menu matches four-command reference shape");
 const wchar_t* fileLabels[]={L"&Stop Download",L"&Remove",L"&Download Now",L"R&edownload"};const UINT fileCommands[]={CMD_STOP,CMD_DELETE,CMD_RESUME,CMD_REDOWNLOAD};
 for(int i=0;i<4;++i){CString label;fileMenu->GetMenuString(i,label,MF_BYPOSITION);expect(label==fileLabels[i]&&fileMenu->GetMenuItemID(i)==fileCommands[i],"File menu reference label and route match");}
 CMenu retainedContext;frame.fillDownloadMenu(retainedContext);
 for(auto id:{CMD_RECYCLE,CMD_PROPERTIES,CMD_REFRESH_ADDRESS,CMD_PROGRESS,CMD_OPEN,CMD_FOLDER,CMD_OPENWITH,CMD_ZIP,CMD_RELOCATE})expect(retainedContext.GetMenuState(id,MF_BYCOMMAND)!=UINT(-1),"Former File-menu action remains in download context menu");
 auto downloadsMenu=frame.GetMenu()->GetSubMenu(2);
 const wchar_t* expectedLabels[]={L"&Pause All",L"&Stop All",L"",L"&Delete All Completed",L"",L"Find (Ctrl-F)",L"Find Next (F3)",L"",L"S&cheduler",L"Start &queue",L"S&top queue",L"",L"Speed &Limiter",L"",L"&Options"};
 expect(downloadsMenu->GetMenuItemCount()==15,"Downloads menu has reference item count");
 for(int i=0;i<15;++i){CString label;downloadsMenu->GetMenuString(i,label,MF_BYPOSITION);expect(label==expectedLabels[i],"Downloads item label and position match reference");if(!*expectedLabels[i])expect((downloadsMenu->GetMenuState(i,MF_BYPOSITION)&MF_SEPARATOR)!=0,"Downloads separator matches reference position");}
 auto startMenu=downloadsMenu->GetSubMenu(9),stopMenu=downloadsMenu->GetSubMenu(10);
 expect(startMenu&&startMenu->GetMenuItemID(0)==CMD_STARTQUEUE,"Start queue submenu routes first item to selected queue");
 expect(stopMenu&&stopMenu->GetMenuItemID(0)==CMD_STOPQUEUE,"Stop queue submenu routes first item to selected queue");
 expect(startMenu->GetMenuItemID(1)==CMD_STARTQUEUEMENU&&stopMenu->GetMenuItemID(1)==CMD_STOPQUEUEMENU,"Queue choosers remain accessible within submenus");

 auto originalQueues=manager.state["Queues"];auto firstQueue=defaultQueue("Research & media"),secondQueue=defaultQueue("Second queue");firstQueue["Enabled"]=false;secondQueue["Enabled"]=false;manager.setQueue(firstQueue);manager.setQueue(secondQueue);
 frame.SendMessage(WM_INITMENUPOPUP,(WPARAM)startMenu->GetSafeHmenu(),MAKELPARAM(9,FALSE));
 auto firstIndex=std::find(frame.startQueueNames.begin(),frame.startQueueNames.end(),"Research & media")-frame.startQueueNames.begin();auto firstCommand=32000+(UINT)firstIndex;
 expect(firstIndex<frame.startQueueNames.size(),"Opened queue submenu includes newly created queue");
 CString literalQueue;startMenu->GetMenuString(firstCommand,literalQueue,MF_BYCOMMAND);expect(literalQueue==L"Research && media","Queue names escape literal ampersands in menu");
 expect((startMenu->GetMenuState(firstCommand,MF_BYCOMMAND)&3)==0,"Dynamic queue command remains enabled after MFC menu update");
 auto reordered=manager.state["Queues"];std::reverse(reordered.begin(),reordered.end());manager.setQueues(reordered);
 frame.SendMessage(WM_COMMAND,firstCommand);
 auto queueEnabled=[&](const std::string& name){for(auto q:manager.state["Queues"])if(str(q,"Name")==name)return yes(q,"Enabled");return false;};
 expect(queueEnabled("Research & media")&&!queueEnabled("Second queue"),"Named queue command retains identity after queue list reorder");
 frame.SendMessage(WM_INITMENUPOPUP,(WPARAM)stopMenu->GetSafeHmenu(),MAKELPARAM(10,FALSE));auto stopIndex=std::find(frame.stopQueueNames.begin(),frame.stopQueueNames.end(),"Research & media")-frame.stopQueueNames.begin();frame.SendMessage(WM_COMMAND,33000+(UINT)stopIndex);
 expect(!queueEnabled("Research & media"),"Named Stop queue command stops its corresponding queue");
 manager.deleteQueue("Research & media");auto beforeStale=manager.state["Queues"];bool staleRejected=false;try{frame.runQueueMenuCommand(firstCommand);}catch(...){staleRejected=true;}
 expect(staleRejected&&manager.state["Queues"]==beforeStale,"Deleted queue command is rejected without acting on another queue");
 frame.SendMessage(WM_INITMENUPOPUP,(WPARAM)startMenu->GetSafeHmenu(),MAKELPARAM(9,FALSE));
 expect(std::find(frame.startQueueNames.begin(),frame.startQueueNames.end(),"Research & media")==frame.startQueueNames.end(),"Reopened queue submenu removes deleted queue");
 manager.setQueues(originalQueues);
 auto viewMenu=frame.GetMenu()->GetSubMenu(3);const wchar_t* viewLabels[]={L"Hide &categories",L"&Arrange files",L"&Toolbar",L"&UDM tray icon",L"C&ustomize URL List...",L"Dark Mode support",L"Font",L"",L"&Language"};
 expect(viewMenu->GetMenuItemCount()==9,"View menu has reference nine-position grouping");
 for(int i=0;i<9;++i){CString label;viewMenu->GetMenuString(i,label,MF_BYPOSITION);expect(label==viewLabels[i],"View label and position match reference with UDM branding");}
 auto trayMenu=viewMenu->GetSubMenu(3);expect(trayMenu&&trayMenu->GetMenuItemCount()==3,"Tray choices are grouped in View submenu");
 for(auto id:{CMD_TRAY_COLOR,CMD_TRAY_SYSTEM,CMD_TRAY_HIDE}){frame.SendMessage(WM_COMMAND,id);frame.SendMessage(WM_INITMENUPOPUP,(WPARAM)trayMenu->GetSafeHmenu(),MAKELPARAM(3,FALSE));expect((trayMenu->GetMenuState(id,MF_BYCOMMAND)&MF_CHECKED)!=0,"Selected tray choice is checked");}
 frame.SendMessage(WM_COMMAND,CMD_TRAY_COLOR);
 auto languageMenu=viewMenu->GetSubMenu(8);auto beforeEnglish=manager.state["Settings"];frame.SendMessage(WM_COMMAND,CMD_LANGUAGE_ENGLISH);frame.SendMessage(WM_INITMENUPOPUP,(WPARAM)languageMenu->GetSafeHmenu(),MAKELPARAM(8,FALSE));
 expect(languageMenu->GetMenuItemCount()==1&&(languageMenu->GetMenuState(CMD_LANGUAGE_ENGLISH,MF_BYCOMMAND)&MF_CHECKED)!=0,"English is the supported checked language");
 expect(manager.state["Settings"]==beforeEnglish,"Selecting current English language preserves preferences");
 auto fontMenu=frame.GetMenu()->GetSubMenu(3)->GetSubMenu(6);expect(fontMenu&&fontMenu->GetMenuItemCount()==2,"Font menu exposes Choose and Reset");
 auto custom=manager.state["Settings"];custom["FontName"]="Arial";custom["FontHeight"]=17;custom["FontWeight"]=700;manager.setSettings(custom);frame.applyAppearance();auto recordsBeforeFont=manager.snapshot()["Downloads"];
 frame.SendMessage(WM_INITMENUPOPUP,(WPARAM)fontMenu->GetSafeHmenu(),MAKELPARAM(6,FALSE));expect((fontMenu->GetMenuState(CMD_FONT_RESET,MF_BYCOMMAND)&3)==0,"Font Reset stays enabled in opened menu");
 frame.SendMessage(WM_COMMAND,CMD_FONT_RESET);auto expectedFont=custom;auto defaults=defaultSettings();for(auto key:{"FontName","FontHeight","FontWeight"})expectedFont[key]=defaults[key];
 expect(manager.state["Settings"]==expectedFont,"Reset restores only the three default font preferences");expect(manager.snapshot()["Downloads"]==recordsBeforeFont,"Font Reset preserves download records");
 LOGFONTW actual{};frame.font.GetLogFont(&actual);expect(std::wstring(actual.lfFaceName)==wide(str(defaults,"FontName"))&&actual.lfHeight==-frame.px((int)num(defaults,"FontHeight"))&&actual.lfWeight==num(defaults,"FontWeight"),"Reset applies the default native font immediately");
 expect(frame.table.GetFont()->GetSafeHandle()==frame.font.GetSafeHandle()&&frame.tree.GetFont()->GetSafeHandle()==frame.font.GetSafeHandle(),"List and category tree receive the reset font");
 auto fontSaved=Json::parse(readText(manager.root/L"state.json"));expect(fontSaved["Settings"]==expectedFont,"Reset font preferences are saved");
 auto limiter=frame.GetMenu()->GetSubMenu(2)->GetSubMenu(12);expect(limiter&&limiter->GetMenuItemCount()==4,"Speed Limiter exposes On Off separator and Settings");
 auto rateSettings=manager.state["Settings"];rateSettings["LimitKbps"]=321;rateSettings["GlobalLimitMode"]="Each download";manager.setSettings(rateSettings);auto records=manager.snapshot()["Downloads"];
 frame.SendMessage(WM_COMMAND,CMD_LIMIT_OFF);expect(num(manager.state["Settings"],"LimitKbps")==0&&num(manager.state["Settings"],"GlobalLimitRememberedKbps")==321,"Turn off remembers the previous global rate");
 frame.SendMessage(WM_INITMENUPOPUP,(WPARAM)limiter->GetSafeHmenu(),MAKELPARAM(12,FALSE));expect((limiter->GetMenuState(CMD_LIMIT_OFF,MF_BYCOMMAND)&MF_CHECKED)!=0,"Turn off is checked while disabled");
 frame.SendMessage(WM_COMMAND,CMD_LIMIT_OFF);frame.SendMessage(WM_COMMAND,CMD_LIMIT_ON);expect(num(manager.state["Settings"],"LimitKbps")==321,"Turn on restores rate after repeated off");
 frame.SendMessage(WM_INITMENUPOPUP,(WPARAM)limiter->GetSafeHmenu(),MAKELPARAM(12,FALSE));expect((limiter->GetMenuState(CMD_LIMIT_ON,MF_BYCOMMAND)&MF_CHECKED)!=0,"Turn on is checked while enabled");
 expect(str(manager.state["Settings"],"GlobalLimitMode")=="Each download","On Off preserves aggregate versus per-download mode");expect(manager.snapshot()["Downloads"]==records,"Global toggles preserve individual download records");
 frame.SendMessage(WM_COMMAND,CMD_LIMIT_OFF);auto saved=Json::parse(readText(manager.root/L"state.json"));expect(num(saved["Settings"],"GlobalLimitRememberedKbps")==321&&num(saved["Settings"],"LimitKbps")==0,"Off and remembered rate persist together");
 for(auto bad:{i64(0),i64(-1),i64(1000001)}){bool rejected=false;try{globalLimiterSettings(rateSettings,false,bad);}catch(...){rejected=true;}expect(rejected,"Invalid remembered speed is rejected even while off");}

 auto optionsRate=manager.state["Settings"];optionsRate["LimitKbps"]=777;manager.setSettings(optionsRate);
 auto optionsOff=manager.state["Settings"];optionsOff["LimitKbps"]=0;manager.setSettings(optionsOff);
 frame.SendMessage(WM_COMMAND,CMD_LIMIT_ON);expect(num(manager.state["Settings"],"LimitKbps")==777,"Options rate survives disabling there and enabling from menu");
 frame.SendMessage(WM_COMMAND,CMD_LIMIT_OFF);
 manager.setSettings(globalLimiterSettings(manager.state["Settings"],false,456));frame.SendMessage(WM_COMMAND,CMD_LIMIT_ON);
 expect(num(manager.state["Settings"],"LimitKbps")==456,"Explicit disabled dialog rate replaces remembered Options rate");
 auto otherSettings=manager.state["Settings"];otherSettings["Sound"]=!yes(otherSettings,"Sound");manager.setSettings(otherSettings);frame.SendMessage(WM_COMMAND,CMD_LIMIT_OFF);frame.SendMessage(WM_COMMAND,CMD_LIMIT_ON);
 expect(num(manager.state["Settings"],"LimitKbps")==456,"Unrelated settings changes preserve remembered rate");
 frame.SendMessage(WM_COMMAND,CMD_LIMIT_OFF);

 frame.showProgress(paused);frame.showProgress(complete);
 auto pausedWindow=frame.progress[frame.progress.size()-2]->GetSafeHwnd(),completeWindow=frame.progress.back()->GetSafeHwnd();
 auto terminalBefore=complete->data;frame.SendMessage(WM_COMMAND,CMD_STOPALL);
 expect(::IsWindow(pausedWindow)&&::IsWindow(completeWindow),"Pause all retains paused and complete progress windows");
 frame.SendMessage(WM_COMMAND,CMD_STOP_CLOSE_ALL);
 expect(!::IsWindow(pausedWindow),"Stop all closes previously paused download window");
 expect(::IsWindow(completeWindow)&&complete->data==terminalBefore,"Stop all preserves completed window and record");
 frame.showProgress(paused);auto toolbarWindow=frame.progress.back()->GetSafeHwnd();frame.refresh();
 expect(frame.toolbar.IsButtonEnabled(CMD_STOPALL),"Toolbar Stop All enables for paused progress window");
 frame.SendMessage(WM_COMMAND,CMD_STOPALL,(LPARAM)frame.toolbar.GetSafeHwnd());
 expect(!::IsWindow(toolbarWindow),"Toolbar Stop All closes paused progress window");
 expect(!enabled(2,CMD_STOP_CLOSE_ALL),"Only completed progress does not enable Stop all");for(auto& window:frame.progress)if(window->GetSafeHwnd()==completeWindow)window->DestroyWindow();
 auto arrange=frame.GetMenu()->GetSubMenu(3)->GetSubMenu(1);expect(arrange&&arrange->GetMenuItemCount()==11,"Arrange files exposes all eleven reference choices");
 paused->data["Added"]="/Date(1000)/";complete->data["Added"]="/Date(2000)/";paused->data["Size"]=20;complete->data["Size"]=10;
 select({paused});
 auto sortHeaderMatches=[&](int column,bool up){auto header=frame.table.GetHeaderCtrl();if(!header)return false;for(int i=0;i<header->GetItemCount();++i){HDITEMW item{};item.mask=HDI_FORMAT;if(!header->GetItem(i,&item))return false;int expected=i==column?(up?HDF_SORTUP:HDF_SORTDOWN):0;if((item.fmt&(HDF_SORTUP|HDF_SORTDOWN))!=expected)return false;}return true;};
 const int columns[]={8,0,2,3,4,5,6,7,9,10,11};
 for(UINT i=0;i<11;++i){frame.SendMessage(WM_COMMAND,CMD_SORT_ADDED+i);expect(frame.sortColumn==columns[i]&&frame.ascending,"Arrange command selects its corresponding column");expect(sortHeaderMatches(columns[i],true),"Arrange selection displays one ascending header indicator");frame.SendMessage(WM_INITMENUPOPUP,(WPARAM)arrange->GetSafeHmenu(),MAKELPARAM(1,FALSE));expect((arrange->GetMenuState(CMD_SORT_ADDED+i,MF_BYCOMMAND)&MF_CHECKED)!=0,"Selected Arrange choice is checked");}
 frame.SendMessage(WM_COMMAND,CMD_SORT_NAME);expect(frame.visible.front()==complete,"Name menu sorts actual rows alphabetically");expect(frame.selected()==std::vector<JobPtr>{paused},"Sorting preserves selected download identity");
 frame.SendMessage(WM_COMMAND,CMD_SORT_SIZE);expect(frame.visible.front()==complete,"Size menu sorts actual rows numerically");
 frame.SendMessage(WM_COMMAND,CMD_SORT_ADDED);expect(frame.visible.front()==paused,"Addition-order menu restores chronological order");expect(num(manager.state["Settings"]["ListLayout"],"SortColumn")==8,"Arrange choice is stored in list layout");
 NMLISTVIEW clicked{};clicked.hdr={frame.table.GetSafeHwnd(),(UINT_PTR)frame.table.GetDlgCtrlID(),LVN_COLUMNCLICK};clicked.iSubItem=2;
 frame.SendMessage(WM_NOTIFY,clicked.hdr.idFrom,(LPARAM)&clicked);expect(frame.sortColumn==2&&frame.ascending&&sortHeaderMatches(2,true),"Clicking another column moves the ascending indicator");
 frame.SendMessage(WM_NOTIFY,clicked.hdr.idFrom,(LPARAM)&clicked);expect(!frame.ascending&&sortHeaderMatches(2,false),"Clicking the sorted column reverses the indicator");
 expect(frame.selected()==std::vector<JobPtr>{paused},"Header-direction changes preserve selected download identity");
 frame.rememberLayout();frame.sortColumn=-1;frame.ascending=true;frame.refresh();expect(sortHeaderMatches(-1,true),"Unsorted list clears all header indicators");
 frame.restoreLayout();frame.refresh();expect(frame.sortColumn==2&&!frame.ascending&&sortHeaderMatches(2,false),"Restored layout restores its descending header indicator");
 frame.SendMessage(WM_COMMAND,CMD_SORT_ADDED);
 select({});expect(enabled(0,CMD_RECOVERY),"Recovery enabled in opened Tasks menu");
 expect(!enabled(1,CMD_PROPERTIES),"Empty selection disables Properties");expect(!enabled(1,CMD_DELETE),"Empty selection disables Delete");expect(!enabled(1,CMD_OPEN),"Empty selection disables Open");expect(!enabled(1,CMD_RESUME),"Empty selection disables Resume");
 select({paused});expect(enabled(1,CMD_PROPERTIES),"Paused selection enables Properties");expect(enabled(1,CMD_RESUME),"Paused selection enables Resume");expect(!enabled(1,CMD_OPEN),"Paused selection disables Open");expect(enabled(1,CMD_DELETE),"Paused selection enables Delete");
 auto categoryPrefsBefore=manager.state["Settings"];
 manager.editCategory("","Video exceptions","mp4","media.example","");auto categoryRulesBefore=manager.state["Settings"]["CategoryRules"];
 manager.editCategory("Video","Video",categoryExtensions(manager.state["Settings"],"Video"),"",utf8((manager.root/L"category-output").wstring()));
 expect(manager.state["Settings"]["CategoryRules"]==categoryRulesBefore,"Folder-only category edit preserves rule order and definitions");
 expect(downloadCategory("movie.mp4","media.example",manager.state["Settings"])=="Video exceptions","Folder-only category edit preserves site-specific classification");

 manager.editCategory("Video","Video","mkv","","");
 expect(categoryExtensions(manager.state["Settings"],"Video")=="mkv","Category editor stores replacement built-in extension list");
 expect(downloadCategory("movie.mp4","example.invalid",manager.state["Settings"])=="Other"&&downloadCategory("movie.mkv","example.invalid",manager.state["Settings"])=="Video","Replaced category types stop classifying removed extensions");
 manager.editCategory("Video","Video","","","");
 expect(categoryExtensions(manager.state["Settings"],"Video").empty()&&downloadCategory("movie.mkv","example.invalid",manager.state["Settings"])=="Other","Empty category list disables built-in automatic types");
 auto categoryBeforeFailure=manager.state["Settings"];auto categoryDiskBefore=fileHash(manager.root/L"state.json");bool categorySaveFailed=false;
 {Handle blocked(CreateFileW((manager.root/L"state.json").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));try{manager.editCategory("Video","Video","mp4","","");}catch(...){categorySaveFailed=true;}}
 expect(categorySaveFailed&&manager.state["Settings"]==categoryBeforeFailure,"Failed category edit restores all category preferences");
 expect(fileHash(manager.root/L"state.json")==categoryDiskBefore,"Failed category edit preserves durable catalog");
 saveCategoryProperties(manager,"","Remembered","pdf","","",true);
 expect(yes(manager.state["Settings"]["CategoryRememberLast"],"Remembered"),"Category properties creates remembered destination preference");
 saveCategoryProperties(manager,"Remembered","Renamed remembered","pdf","","",true);
 expect(yes(manager.state["Settings"]["CategoryRememberLast"],"Renamed remembered")&&!manager.state["Settings"]["CategoryRememberLast"].contains("Remembered"),"Category rename carries destination memory preference");
 auto memoryBefore=manager.state["Settings"];auto memoryDisk=fileHash(manager.root/L"state.json");bool memoryFailed=false;
 {Handle blocked(CreateFileW((manager.root/L"state.json").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));try{saveCategoryProperties(manager,"Renamed remembered","Renamed remembered","mkv","","",false);}catch(...){memoryFailed=true;}}
 expect(memoryFailed&&manager.state["Settings"]==memoryBefore&&fileHash(manager.root/L"state.json")==memoryDisk,"Failed category properties save restores memory and type settings together");
 saveCategoryProperties(manager,"Renamed remembered","Renamed remembered","pdf","","",false);
 expect(!yes(manager.state["Settings"]["CategoryRememberLast"],"Renamed remembered"),"Category properties turns destination memory off");
 manager.state["Settings"]=categoryPrefsBefore;manager.save();
 auto categoryJobBefore=complete->data;auto categoryProjectsBefore=manager.state["Projects"];
 complete->data["Category"]="Video";auto categoryFileBefore=fileHash(complete->target());auto categoryTargetBefore=complete->target();
 manager.state["Projects"].push_back({{"Name","Category fixture"},{"SaveCategory","Video"}});
 auto builtinBefore=manager.state["Settings"];auto builtinJobBefore=complete->data;auto builtinProjectsBefore=manager.state["Projects"];manager.save();auto builtinDiskBefore=fileHash(manager.root/L"state.json");bool builtinFailed=false;
 {Handle blocked(CreateFileW((manager.root/L"state.json").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));try{saveCategoryProperties(manager,"Video","Movies","mp4 mkv","","",true);}catch(...){builtinFailed=true;}}
 expect(builtinFailed&&manager.state["Settings"]==builtinBefore&&complete->data==builtinJobBefore&&manager.state["Projects"]==builtinProjectsBefore&&fileHash(manager.root/L"state.json")==builtinDiskBefore,"Predefined category rename rolls back all references when save fails");
 saveCategoryProperties(manager,"Video","Movies","mp4 mkv","",utf8((manager.root/L"movies").wstring()),true);
 expect(!activeCategory(manager.state["Settings"],"Video")&&activeCategory(manager.state["Settings"],"Movies")&&manager.categories()==optionCategories(manager.state["Settings"]),"Predefined category rename updates both active category lists");
 expect(str(complete->data,"Category")=="Movies"&&str(manager.state["Projects"].back(),"SaveCategory")=="Movies","Predefined rename updates downloads and Grabber project references");
 expect(downloadCategory("movie.mp4","example.invalid",manager.state["Settings"])=="Movies"&&categoryForPreferences("movie.mp4",manager.state["Settings"])=="Movies","Renamed predefined category receives future matching files");
 auto routedMovie=manager.add("https://example.invalid/new-category.mp4","","new-category.mp4","Main queue",true);expect(str(routedMovie->data,"Category")=="Movies"&&routedMovie->target().parent_path()==manager.root/L"movies","New engine admission uses renamed predefined category and folder");manager.remove(routedMovie);
 auto movieDestination=grabberDestination(Json{{"SaveMode","Categories"}},manager.state["Settings"],"https://example.invalid/grabbed.mp4");expect(movieDestination.category=="Movies"&&fs::path(wide(movieDestination.folder))==manager.root/L"movies","Grabber automatic destination agrees with renamed category");
 expect(yes(manager.state["Settings"]["CategoryRememberLast"],"Movies")&&categoryFolder(manager.state["Settings"],"Movies")==utf8((manager.root/L"movies").wstring()),"Renamed predefined category retains folder and path memory");
 expect(complete->target()==categoryTargetBefore&&fileHash(complete->target())==categoryFileBefore,"Predefined rename preserves existing saved file location and bytes");
 auto persistedCategory=Json::parse(readText(manager.root/L"state.json"));expect(!activeCategory(persistedCategory["Settings"],"Video")&&activeCategory(persistedCategory["Settings"],"Movies"),"Predefined rename persists the hidden default and new category");
 manager.deleteCategory("Movies");expect(str(complete->data,"Category")=="Other"&&str(manager.state["Projects"].back(),"SaveCategory")=="Other"&&downloadCategory("movie.mp4","example.invalid",manager.state["Settings"])=="Other","Deleting renamed category reassigns records without resurrecting default classification");
 auto deletionBefore=manager.state["Settings"];bool deletionFailed=false;
 {Handle blocked(CreateFileW((manager.root/L"state.json").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));try{manager.deleteCategory("Music");}catch(...){deletionFailed=true;}}
 expect(deletionFailed&&manager.state["Settings"]==deletionBefore,"Predefined deletion rolls back its hidden-category entry when save fails");
 manager.deleteCategory("Music");expect(!activeCategory(manager.state["Settings"],"Music")&&downloadCategory("track.mp3","example.invalid",manager.state["Settings"])=="Other","Deleting predefined category removes its automatic file classification");
 manager.editCategory("","Video","webm","","");expect(activeCategory(manager.state["Settings"],"Video")&&downloadCategory("movie.webm","example.invalid",manager.state["Settings"])=="Video"&&downloadCategory("movie.mp4","example.invalid",manager.state["Settings"])=="Other","Reusing deleted predefined name does not restore its old file types");
 validateOptionsModel(manager.state["Settings"]);expect(true,"Edited predefined category settings satisfy options validation");
 auto invalidHidden=manager.state["Settings"];invalidHidden["HiddenBuiltinCategories"].push_back("Other");bool hiddenRejected=false;try{validateOptionsModel(invalidHidden);}catch(...){hiddenRejected=true;}expect(hiddenRejected,"Hidden-category input cannot remove the fallback category");
 complete->data=categoryJobBefore;manager.state["Projects"]=categoryProjectsBefore;manager.state["Settings"]=categoryPrefsBefore;manager.save();

 auto batchPaused=paused->data,batchComplete=complete->data;manager.save();auto batchDiskHash=fileHash(manager.root/L"state.json");
 bool lateInvalid=false;try{setQueueMembershipBatch(manager,{paused,nullptr},false);}catch(...){lateInvalid=true;}
 expect(lateInvalid&&paused->data==batchPaused&&complete->data==batchComplete,"Invalid later batch member leaves all records unchanged");
 expect(fileHash(manager.root/L"state.json")==batchDiskHash,"Invalid batch does not write a partial catalog");
 bool batchSaveFailed=false;
 {Handle blocked(CreateFileW((manager.root/L"state.json").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));try{setQueueMembershipBatch(manager,{paused,complete},false);}catch(...){batchSaveFailed=true;}}
 expect(batchSaveFailed&&paused->data==batchPaused&&complete->data==batchComplete,"Failed batch save rolls back every record");
 expect(fileHash(manager.root/L"state.json")==batchDiskHash,"Failed batch save preserves durable catalog");
 setQueueMembershipBatch(manager,{paused,paused,complete},false);
 expect(!yes(paused->data,"QueueMember")&&!yes(complete->data,"QueueMember"),"Batch removal handles duplicate selections and removes all members");
 auto durableBatch=Json::parse(readText(manager.root/L"state.json"));bool durableRemoved=true;for(auto& row:durableBatch["Downloads"])if(str(row,"Id")==paused->id()||str(row,"Id")==complete->id())durableRemoved&=!yes(row,"QueueMember");
 expect(durableRemoved,"Batch removal persists every selected member together");
 auto emptyHash=fileHash(manager.root/L"state.json");setQueueMembershipBatch(manager,{},false);expect(fileHash(manager.root/L"state.json")==emptyHash,"Empty batch leaves durable catalog untouched");
 paused->data=batchPaused;complete->data=batchComplete;manager.save();frame.refresh();
 auto enrollmentOriginal=complete->data;auto enrollmentQueues=manager.state["Queues"];
 select({complete});expect(enabled(1,CMD_MOVEQUEUE)&&!frame.membershipQueues({complete}).empty(),"Default synchronization queue is offered for completed enrollment");
 auto withoutSync=manager.state["Queues"];withoutSync.erase(std::remove_if(withoutSync.begin(),withoutSync.end(),[](const Json& q){return yes(q,"Synchronize");}),withoutSync.end());manager.setQueues(withoutSync);
 select({complete});expect(!enabled(1,CMD_MOVEQUEUE),"Completed enrollment disabled when no synchronization queue exists");
 auto checkQueue=defaultQueue("File checks");checkQueue["Synchronize"]=true;checkQueue["Enabled"]=false;manager.setQueue(checkQueue);
 expect(enabled(1,CMD_MOVEQUEUE)&&frame.membershipQueues({complete})==std::vector<std::string>{"File checks"},"Completed enrollment offers only synchronization queues");
 bool ordinaryRejected=false;try{manager.setMembership(complete,true,"Main queue");}catch(...){ordinaryRejected=true;}
 expect(ordinaryRejected&&complete->data==enrollmentOriginal,"Completed enrollment in ordinary queue is rejected without mutation");
 complete->data["DuplicateOf"]="fixture-original";auto unsupportedEnrollment=complete->data;bool duplicateRejected=false;try{manager.setMembership(complete,true,"File checks");}catch(...){duplicateRejected=true;}
 expect(duplicateRejected&&complete->data==unsupportedEnrollment,"Duplicate placeholder cannot enroll for synchronization");complete->data=enrollmentOriginal;
 complete->data["Folder"]=utf8((manager.root/L"missing-enrollment-folder").wstring());auto missingEnrollment=complete->data;bool missingRejected=false;try{manager.setMembership(complete,true,"File checks");}catch(...){missingRejected=true;}
 expect(missingRejected&&complete->data==missingEnrollment,"Missing saved file cannot enroll and preserves its record");complete->data=enrollmentOriginal;
 manager.save();bool enrollmentFailed=false;
 {Handle blocked(CreateFileW((manager.root/L"state.json").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));try{manager.setMembership(complete,true,"File checks");}catch(...){enrollmentFailed=true;}}
 expect(enrollmentFailed&&complete->data==enrollmentOriginal,"Failed completed enrollment save restores source queue and membership");
 auto enrollmentHash=fileHash(complete->target());manager.setMembership(complete,true,"File checks");
 expect(str(complete->data,"Queue")=="File checks"&&yes(complete->data,"QueueMember")&&str(complete->data,"Status")=="Complete"&&!yes(complete->data,"SyncPending"),"Completed enrollment preserves status and waits for queue start");
 expect(fileHash(complete->target())==enrollmentHash,"Completed enrollment preserves saved bytes");
 auto orderBeforeDrop=manager.jobs;auto dataBeforeDrop=complete->data;auto diskBeforeDrop=fileHash(manager.root/L"state.json");bool invalidAnchor=false;
 try{manager.reorder(complete,"File checks",paused);}catch(...){invalidAnchor=true;}
 expect(invalidAnchor&&manager.jobs==orderBeforeDrop&&complete->data==dataBeforeDrop,"Cross-queue drop anchor rejects without changing completed record or order");
 expect(fileHash(manager.root/L"state.json")==diskBeforeDrop,"Rejected drop preserves durable catalog");
 auto anchorBeforeDrop=paused->data;manager.setMembership(paused,true,"File checks");
 bool failedDrop=false;
 {Handle blocked(CreateFileW((manager.root/L"state.json").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));try{manager.reorder(complete,"File checks",paused);}catch(...){failedDrop=true;}}
 expect(failedDrop&&manager.jobs==orderBeforeDrop&&complete->data==dataBeforeDrop,"Failed drop save restores order and membership together");
 complete->data["SyncPending"]=true;manager.reorder(complete,"File checks",paused);
 expect(manager.jobs.front()==complete&&yes(complete->data,"SyncPending")&&str(complete->data,"Status")=="Complete","Completed same-queue drop preserves pending synchronization");
 expect(fileHash(complete->target())==enrollmentHash,"Completed drop preserves saved bytes");
 manager.jobs=orderBeforeDrop;complete->data=dataBeforeDrop;paused->data=anchorBeforeDrop;manager.save();
 manager.queueRun("File checks",true);expect(yes(complete->data,"SyncPending"),"Enrolled completed file becomes eligible when synchronization queue starts");manager.queueRun("File checks",false);
 complete->data=enrollmentOriginal;manager.setQueues(enrollmentQueues);manager.save();frame.refresh();
 auto completedBeforeMembership=complete->data;auto completedFileHash=fileHash(complete->target());
 complete->data["QueueMember"]=true;select({complete});
 expect(enabled(1,CMD_REMOVEQUEUE),"Completed queue member enables removal from queue");
 expect(enabled(1,CMD_UP)&&enabled(1,CMD_DOWN),"Completed queue member enables queue reordering");
 auto orderBeforeMember=manager.jobs;frame.SendMessage(WM_COMMAND,CMD_UP);
 expect(manager.jobs!=orderBeforeMember&&str(complete->data,"Status")=="Complete","Completed queue member can actually move earlier without restarting");
 manager.jobs=orderBeforeMember;manager.save();
 complete->data["SyncPending"]=true;auto beforeFailedRemoval=complete->data;manager.save();bool membershipFailed=false;
 {Handle blocked(CreateFileW((manager.root/L"state.json").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));expect((bool)blocked,"Queue membership rollback fixture locks catalog");try{manager.setMembership(complete,false);}catch(...){membershipFailed=true;}}
 expect(membershipFailed&&complete->data==beforeFailedRemoval,"Failed membership save preserves pending synchronization and membership");
 frame.SendMessage(WM_COMMAND,CMD_REMOVEQUEUE);
 expect(!yes(complete->data,"SyncPending"),"Removing completed queue member clears pending synchronization");
 auto queuesBeforeMembership=manager.state["Queues"];auto pausedBeforeMembership=paused->data;
 auto syncQueue=manager.state["Queues"][0];syncQueue["Synchronize"]=true;syncQueue["Enabled"]=false;manager.setQueue(syncQueue);
 complete->data["QueueMember"]=true;complete->data["SyncPending"]=false;manager.queueRun("Main queue",true);
 expect(yes(complete->data,"SyncPending"),"Synchronization still schedules retained completed queue members");
 manager.queueRun("Main queue",false);manager.setMembership(complete,false);manager.queueRun("Main queue",true);
 expect(!yes(complete->data,"SyncPending"),"Starting synchronization queue does not requeue removed completed record");
 manager.queueRun("Main queue",false);manager.setQueues(queuesBeforeMembership);paused->data=pausedBeforeMembership;
 expect(!yes(complete->data,"QueueMember")&&str(complete->data,"Status")=="Complete","Removing completed queue member preserves completion status");
 expect(fileHash(complete->target())==completedFileHash,"Removing completed queue member preserves downloaded bytes");
 expect(!enabled(1,CMD_REMOVEQUEUE)&&!enabled(1,CMD_UP)&&!enabled(1,CMD_DOWN),"Removed queue member disables membership actions");
 complete->data.erase("QueueMember");
 expect(!enabled(1,CMD_REMOVEQUEUE)&&!enabled(1,CMD_UP),"Legacy completed record without membership is not treated as queued");
 complete->data=completedBeforeMembership;manager.save();frame.refresh();
 select({complete});expect(enabled(1,CMD_PROPERTIES),"Completed selection enables Properties");expect(enabled(1,CMD_OPEN),"Completed selection enables Open");expect(!enabled(1,CMD_RESUME),"Completed selection disables Resume");expect(enabled(1,CMD_RELOCATE),"Completed selection enables Move/Rename");
 select({paused,complete});expect(!enabled(1,CMD_PROPERTIES),"Mixed selection disables single-file Properties");expect(!enabled(1,CMD_OPEN),"Mixed selection disables Open");expect(enabled(1,CMD_RESUME),"Mixed selection can resume paused records");expect(enabled(1,CMD_DELETE),"Inactive mixed selection enables Delete");

 WSADATA wsa{};if(WSAStartup(MAKEWORD(2,2),&wsa))throw std::runtime_error("Cannot start loopback fixture");
 TransferFixture server(1024*1024,20,20);auto running=manager.add(server.url("/steady"),"","running.bin","Main queue",true);
 running->data["Connections"]=1;running->data["LimitKbps"]=64;manager.resume(running);manager.tick();
 auto deadline=GetTickCount64()+5000;
 while(GetTickCount64()<deadline){bool ready;{Lock lock(manager.mutex);ready=manager.isActive(running)&&num(running->data,"Received")>0;}if(ready)break;Sleep(20);}
 expect(manager.isActive(running),"Menu fixture has an actual active transfer");
 auto beforeActiveBatch=complete->data;bool activeBatchRejected=false;try{setQueueMembershipBatch(manager,{complete,running},false);}catch(...){activeBatchRejected=true;}
 expect(activeBatchRejected&&complete->data==beforeActiveBatch&&manager.isActive(running),"Active later member rejects entire batch without stopping its transfer");

 {Lock lock(manager.mutex);expect(num(running->data,"Received")>0,"Active menu fixture received network bytes");}
 frame.showProgress(running);auto activeWindow=frame.progress.back()->GetSafeHwnd();frame.SendMessage(WM_COMMAND,CMD_STOPALL);
 deadline=GetTickCount64()+5000;while(manager.isActive(running)&&GetTickCount64()<deadline)Sleep(20);
 expect(!manager.isActive(running)&&::IsWindow(activeWindow),"Pause all stops real transfer and retains its window");
 manager.resume(running);manager.tick();deadline=GetTickCount64()+5000;while(!manager.isActive(running)&&GetTickCount64()<deadline){manager.tick();Sleep(20);}
 expect(manager.isActive(running),"Paused-all transfer can resume");
 frame.SendMessage(WM_COMMAND,CMD_STOP_CLOSE_ALL);deadline=GetTickCount64()+5000;while(manager.isActive(running)&&GetTickCount64()<deadline)Sleep(20);
 expect(!manager.isActive(running)&&!::IsWindow(activeWindow),"Stop all stops real transfer and closes its window");
 {std::lock_guard<std::mutex> lock(frame.eventsMutex);expect(std::none_of(frame.events.begin(),frame.events.end(),[&](const auto& e){return !e.second&&e.first==running;}),"Stop all discards pending progress-open events");}
 manager.resume(running);manager.tick();deadline=GetTickCount64()+5000;while(!manager.isActive(running)&&GetTickCount64()<deadline){manager.tick();Sleep(20);}
 expect(manager.isActive(running),"Stopped-all transfer can resume");
 frame.refresh();select({running});
 expect(!enabled(0,CMD_RECOVERY),"Active transfer disables Recovery");
 expect(enabled(1,CMD_STOP),"Active selection enables Stop");expect(!enabled(1,CMD_RESUME),"Active selection disables Resume");expect(!enabled(1,CMD_DELETE),"Active selection disables Delete");expect(enabled(1,CMD_PROPERTIES),"Active selection keeps Properties available");expect(!enabled(1,CMD_RELOCATE),"Active selection disables Move/Rename");
 select({running,complete});expect(!enabled(1,CMD_DELETE),"Active/completed mixed selection disables Delete");expect(!enabled(1,CMD_PROPERTIES),"Active/completed mixed selection disables Properties");expect(enabled(1,CMD_STOP),"Active/completed mixed selection enables Stop");
 auto completeBefore=complete->data;auto completeHash=fileHash(complete->target());frame.command(CMD_STOP);expect(complete->data==completeBefore,"Mixed Stop preserves completed record");expect(fileHash(complete->target())==completeHash,"Mixed Stop preserves completed file");deadline=GetTickCount64()+5000;while(manager.isActive(running)&&GetTickCount64()<deadline)Sleep(20);
 expect(!manager.isActive(running),"Stop command pauses real transfer");frame.refresh();select({running});
 expect(enabled(0,CMD_RECOVERY),"Paused transfer reenables Recovery");expect(enabled(1,CMD_RESUME),"Paused transfer reenables Resume");expect(enabled(1,CMD_DELETE),"Paused transfer reenables Delete");expect(!enabled(1,CMD_STOP),"Paused transfer disables Stop");
 {Lock lock(manager.mutex);running->data["LimitKbps"]=0;complete->data["RequiresMediaCapture"]=true;completeBefore=complete->data;}select({complete,running});frame.command(CMD_RESUME);expect(complete->data==completeBefore,"Mixed Resume skips completed media record");expect(fileHash(complete->target())==completeHash,"Mixed Resume preserves completed file");
 deadline=GetTickCount64()+15000;while(GetTickCount64()<deadline){manager.tick();bool done;{Lock lock(manager.mutex);done=str(running->data,"Status")=="Complete"||str(running->data,"Status")=="Failed";}if(done)break;Sleep(20);}
 {Lock lock(manager.mutex);expect(str(running->data,"Status")=="Complete","Resume command completes real transfer");}
 server.expected(manager.root.parent_path()/L"expected.bin");expect(fileHash(running->target())==fileHash(manager.root.parent_path()/L"expected.bin"),"Menu-driven paused/resumed transfer has exact bytes");
 frame.refresh();select({running});expect(enabled(1,CMD_OPEN),"Completed real transfer enables Open");expect(!enabled(1,CMD_RESUME),"Completed real transfer disables Resume");

 auto multiPrefs=manager.state["Settings"];multiPrefs["Parallel"]=2;manager.setSettings(multiPrefs);
 std::vector<JobPtr> batch;std::vector<HWND> batchWindows;
 for(int i=0;i<3;++i){auto j=manager.add(server.url("/steady"),"","batch-"+std::to_string(i)+".bin","Main queue",true);j->data["Connections"]=1;j->data["LimitKbps"]=64;manager.resume(j);batch.push_back(j);frame.showProgress(j);batchWindows.push_back(frame.progress.back()->GetSafeHwnd());}
 manager.tick();deadline=GetTickCount64()+5000;
 while(GetTickCount64()<deadline){int receiving=0;{Lock lock(manager.mutex);for(auto j:batch)if(manager.isActive(j)&&num(j->data,"Received")>0)++receiving;}if(receiving==2)break;manager.tick();Sleep(20);}
 {Lock lock(manager.mutex);int receiving=0,queued=0;for(auto j:batch){if(manager.isActive(j)&&num(j->data,"Received")>0)++receiving;if(str(j->data,"Status")=="Queued")++queued;}expect(receiving==2&&queued==1,"Stop-all batch has two receiving transfers and one queued");}
 frame.refresh();select({batch[0],running});auto completedBeforeSelected=running->data;frame.SendMessage(WM_COMMAND,CMD_STOP);
 deadline=GetTickCount64()+5000;while(manager.isActive(batch[0])&&GetTickCount64()<deadline)Sleep(20);
 expect(!manager.isActive(batch[0])&&!::IsWindow(batchWindows[0]),"Selected Stop pauses transfer and closes only its progress window");
 expect(manager.isActive(batch[1])&&::IsWindow(batchWindows[1]),"Selected Stop leaves other receiving transfer and window running");
 expect(str(batch[2]->data,"Status")=="Queued"&&::IsWindow(batchWindows[2]),"Selected Stop leaves unrelated queued transfer and window unchanged");
 expect(running->data==completedBeforeSelected,"Mixed selected Stop preserves completed record");
 {std::lock_guard<std::mutex> lock(frame.eventsMutex);expect(std::none_of(frame.events.begin(),frame.events.end(),[&](const auto& e){return !e.second&&e.first==batch[0];}),"Selected Stop removes its pending progress-open events");}
 manager.resume(batch[0]);manager.tick();frame.showProgress(batch[0]);batchWindows[0]=frame.progress.back()->GetSafeHwnd();
 deadline=GetTickCount64()+5000;while(!manager.isActive(batch[0])&&GetTickCount64()<deadline){manager.tick();Sleep(20);}
 expect(manager.isActive(batch[0]),"Selected stopped transfer can resume before batch stop");
 ::EnableWindow(batchWindows[0],FALSE);auto completeBatchBefore=running->data;
 frame.SendMessage(WM_COMMAND,CMD_STOP_CLOSE_ALL);
 expect(::IsWindow(batchWindows[0]),"Stop all defers window close while its child dialog owns input");
 expect(!::IsWindow(batchWindows[1])&&!::IsWindow(batchWindows[2]),"Stop all closes other active and queued progress windows");
 ::EnableWindow(batchWindows[0],TRUE);::SendMessageW(batchWindows[0],WM_TIMER,1,0);
 expect(!::IsWindow(batchWindows[0]),"Deferred progress closes after its child dialog releases input");
 deadline=GetTickCount64()+5000;while(GetTickCount64()<deadline){bool any=false;for(auto j:batch)any|=manager.isActive(j);if(!any)break;Sleep(20);}
 {Lock lock(manager.mutex);for(auto j:batch)expect(!manager.isActive(j)&&str(j->data,"Status")=="Paused","Stop all leaves each active or queued download paused");}
 for(int i=0;i<15;++i){manager.tick();Sleep(20);}
 {Lock lock(manager.mutex);for(auto j:batch)expect(!manager.isActive(j)&&str(j->data,"Status")=="Paused","Scheduler ticks do not restart stopped batch");}
 expect(running->data==completeBatchBefore,"Batch stop preserves previously completed download metadata");
 for(auto j:batch){{Lock lock(manager.mutex);j->data["LimitKbps"]=0;}manager.resume(j);}
 deadline=GetTickCount64()+20000;while(GetTickCount64()<deadline){manager.tick();bool all=true;{Lock lock(manager.mutex);for(auto j:batch)all&=str(j->data,"Status")=="Complete";}if(all)break;Sleep(20);}
 for(auto j:batch){expect(str(j->data,"Status")=="Complete","Stopped batch download completes on explicit resume");expect(fileHash(j->target())==fileHash(manager.root.parent_path()/L"expected.bin"),"Resumed batch download has exact expected bytes");}

 }
};
class MenuTestApp:public CWinApp {
 int result=1;
public:BOOL InitInstance()override{
 CWinApp::InitInstance();INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_WIN95_CLASSES|ICC_DATE_CLASSES|ICC_PROGRESS_CLASS};InitCommonControlsEx(&controls);AfxOleInit();
 int argc;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);fs::path root;std::unique_ptr<Manager> manager;MainWindow* frame=nullptr;std::string failure;
 try{if(argc!=2)throw std::runtime_error("Fresh fixture required");root=argv[1];if(fs::exists(root))throw std::runtime_error("Fixture exists");fs::create_directories(root);checkpointRoot=root;manager=std::make_unique<Manager>(root/L"data");auto prefs=manager->state["Settings"];prefs["DownloadFolder"]=utf8((root/L"files").wstring());prefs["CategoryFolders"]=false;prefs["ProxyMode"]="Connect directly";prefs["SuppressCompletionDialog"]=true;prefs["SuppressProgressDialog"]=true;manager->setSettings(prefs);frame=new MainWindow(*manager);m_pMainWnd=frame;CapturePresentationComponentTest::run(*manager,*frame);result=0;}catch(const std::exception& e){failure=e.what();}
 if(frame){frame->SendMessage(WM_CLOSE);m_pMainWnd=nullptr;}manager.reset();if(argv)LocalFree(argv);
 if(!root.empty())atomicText(root/L"results.json",Json{{"passed",result==0},{"error",failure},{"checks",checks}}.dump(2),false);return FALSE;
 }
 int ExitInstance()override{AfxOleTerm(FALSE);return result;}
};MenuTestApp app;
}
