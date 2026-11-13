#define UDM_TOOLBAR_COMPONENT_TEST
#include "App.cpp"
#include "../drivers/signed-network/DurableResponseSink.hpp"
namespace udm {
static Json checks=Json::array();static fs::path output,referencePath;static std::string failure;
static void expect(bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!output.empty())atomicText(output/L"progress.json",checks.dump(2),false);if(!ok)throw std::runtime_error(name);}
static std::wstring caption(HWND w){wchar_t value[2048]{};GetWindowTextW(w,value,2048);return value;}
static HWND named(HWND w,const wchar_t* text){for(auto c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT))if(caption(c)==text)return c;throw std::runtime_error("Missing completion fixture control");}
static std::vector<fs::path> shellDropPaths(const std::vector<fs::path>& files){
 const auto memory=savedFileDropData(files);
 std::unique_ptr<std::remove_pointer_t<HDROP>,decltype(&DragFinish)> drop(reinterpret_cast<HDROP>(memory),DragFinish);
 std::vector<fs::path> paths;
 if(!drop)return paths;
 const auto count=DragQueryFileW(drop.get(),UINT_MAX,nullptr,0);
 for(UINT index=0;index<count;++index){
  const auto length=DragQueryFileW(drop.get(),index,nullptr,0);
  std::wstring name(length+1,L'\0');
  if(!length||DragQueryFileW(drop.get(),index,name.data(),length+1)!=length)
   throw std::runtime_error("Windows could not read the file-drop path.");
  name.resize(length);
  paths.emplace_back(std::move(name));
 }
 return paths;
}
static CRect bounds(HWND w,HWND parent=nullptr){CRect r;GetWindowRect(w,&r);if(parent)MapWindowPoints(nullptr,parent,(POINT*)&r,2);return r;}
static bool render(HWND w,const fs::path& path,const std::vector<HWND>& edits={}){
 ShowWindow(w,SW_SHOWNOACTIVATE);
 RedrawWindow(w,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_UPDATENOW|RDW_ERASE);
 const auto r=bounds(w);
 CImage image;image.Create(r.Width(),r.Height(),32);
 const auto dc=image.GetDC();RECT fill{0,0,r.Width(),r.Height()};FillRect(dc,&fill,GetSysColorBrush(COLOR_BTNFACE));
 SendMessageW(w,WM_PRINT,(WPARAM)dc,PRF_NONCLIENT|PRF_CLIENT|PRF_ERASEBKGND|PRF_CHILDREN);
 bool flatBorders=true;
 for(auto edit:edits){
  auto field=bounds(edit);field.OffsetRect(-r.left,-r.top);
  const auto x=field.CenterPoint().x;
  const auto top=GetPixel(dc,x,field.top),bottom=GetPixel(dc,x,field.bottom-1);
  // The themed edit can paint its outline inside the client area. Check the
  // rendered edges for a uniform outline with contrasting interior pixels.
  flatBorders&=top!=CLR_INVALID&&top==bottom&&top!=GetPixel(dc,x,field.top+1);
 }
 image.ReleaseDC();
 if(FAILED(image.Save(path.c_str())))throw std::runtime_error("Could not render completion fixture");
 return flatBorders;
}
struct MissingWarning {
 HWND owner=nullptr;
 std::wstring title,body;
 static inline MissingWarning* active=nullptr;
 static void CALLBACK inspect(HWND,UINT,UINT_PTR,DWORD){
  if(!active||!active->title.empty())return;
  // Run on the dialog thread during its modal message loop. Child control text
  // is read directly and the real OK button handles dismissal.
  EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM)->BOOL{
   if(GetWindow(w,GW_OWNER)!=active->owner||!IsWindowVisible(w))return TRUE;
   wchar_t type[80]{};GetClassNameW(w,type,80);
   if(wcscmp(type,L"#32770"))return TRUE;
   HWND button=nullptr;
   for(auto c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){
    const auto value=caption(c);
    if(value==L"OK")button=c;
    else if(!value.empty())active->body=value;
   }
   if(!button)return TRUE;
   active->title=caption(w);
   PostMessageW(button,BM_CLICK,0,0);
   return FALSE;
  },0);
 }
 static MissingWarning observe(HWND owner,const std::function<void()>& action){
  MissingWarning result;result.owner=owner;
  const auto timer=SetTimer(nullptr,0,20,inspect);
  if(!timer)throw std::runtime_error("Cannot observe completion warning.");
  active=&result;
  action();
  KillTimer(nullptr,timer);
  active=nullptr;
  return result;
 }
 static MissingWarning observe(HWND owner,const wchar_t* action){
  const auto button=named(owner,action);
  return observe(owner,[button]{SendMessageW(button,BM_CLICK,0,0);});
 }
 bool explainsMove(const fs::path& path)const{
  return title==L"Opening downloaded file"&&body.find(path.wstring())!=std::wstring::npos&&body.find(L"file has been moved.")!=std::wstring::npos;
 }
};
static LRESULT CALLBACK expandNonClientFrame(HWND w,UINT message,WPARAM param,LPARAM data,UINT_PTR,DWORD_PTR){
 const auto result=DefSubclassProc(w,message,param,data);
 if(message==WM_NCCALCSIZE&&data){
  auto& client=param?reinterpret_cast<NCCALCSIZE_PARAMS*>(data)->rgrc[0]:*reinterpret_cast<RECT*>(data);
  client.bottom=std::max(client.top,client.bottom-48);
 }
 return result;
}
class ToolbarComponentTest {
public:static void run(MainWindow& frame){
 auto& manager=frame.manager;
 {
  Json offer={{"Id",uint64_t(1)},{"Method","GET"},{"Status",200},{"WasInterceptable",true},{"Host","example.invalid"},{"Target","/captured-complete.bin"},{"Length",uint64_t(4)}};
  Json capture={{"Session",std::string(32,'a')},{"Generation",uint64_t(1)},{"ResponseAdmissionEnabled",true},{"PendingResponses",Json::array({offer})}};JobPtr captured;
  {auto reserved=manager.reserveOriginalResponse(capture,offer);captured=reserved.job;auto body=reserved.body.h,receipt=reserved.receipt.h;reserved.body.h=reserved.receipt.h=INVALID_HANDLE_VALUE;udmnet::DurableResponseSink sink(body,receipt,true,4);sink.write("abcd");sink.complete();}
  manager.recoverOriginalResponses();auto previous=manager.event;unsigned events=0;
  manager.event=[&](JobPtr job,bool completed){expect(completed&&job==captured&&job->speed==0&&readText(job->target())=="abcd","Captured completion callback observes published bytes and zero live speed");++events;frame.complete(job,true);};
  try{captured->speed=1024;manager.publishOriginalResponse(captured);manager.publishOriginalResponse(captured);}catch(...){manager.event=previous;throw;}manager.event=previous;
  expect(events==1,"Repeated captured publication emits one completion dialog callback");auto window=frame.completions.back()->GetSafeHwnd();expect(window!=nullptr,"Captured response publication opens the native completion dialog");
  bool destination=false;for(auto child=GetWindow(window,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT))destination|=caption(child)==captured->target().wstring();expect(destination,"Captured completion dialog displays the published destination");
  for(auto label:{L"Open",L"Open with...",L"Open folder",L"Close"})expect(IsWindowEnabled(named(window,label))!=FALSE,"Captured completed file enables its standard file actions");render(window,output/L"captured-completion.png");SendMessageW(named(window,L"Close"),BM_CLICK,0,0);expect(!IsWindow(window),"Captured completion dialog closes normally");
 }
auto job=manager.add("https://example.invalid/completed.bin","","completed.bin","Main queue",true);fs::create_directories(job->target().parent_path());writeBytes(job->target(),Bytes{1,2,3,4});job->data["Status"]="Complete";job->data["Size"]=4;job->data["Received"]=4;job->data["TransferredBytes"]=4;job->data["TransferSeconds"]=2;manager.save();
 {
  frame.ShowWindow(SW_SHOWNOACTIVATE);
  frame.refresh();
  const auto row=std::find(frame.visible.begin(),frame.visible.end(),job);
  expect(row!=frame.visible.end(),"Completed file is visible in the main download list");
  frame.table.SetItemState(-1,0,LVIS_SELECTED|LVIS_FOCUSED);
  frame.table.SetItemState((int)std::distance(frame.visible.begin(),row),LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
  const std::vector<JobPtr> selected={job};
  const auto enter=[&]{
   NMHDR notice{frame.table.GetSafeHwnd(),(UINT_PTR)frame.table.GetDlgCtrlID(),NM_RETURN};
   SendMessageW(frame.GetSafeHwnd(),WM_NOTIFY,notice.idFrom,(LPARAM)&notice);
  };
  const auto path=job->target(),moved=path.parent_path()/L"temporary-moved.bin";
  expect(shellDropPaths(frame.completedDragPaths(frame.selected()))==std::vector<fs::path>{path},
   "An idle completed selection supplies its saved file to the main-list drag");
  job->data["ScanResult"]={{"Status","Pending"}};
  expect(frame.completedDragPaths(frame.selected()).empty(),
   "A scan starting before the next list refresh blocks the selected file drag");
  fs::rename(path,moved);
  fs::create_directory(path);
  auto available=frame.commandStates({CMD_OPEN,CMD_OPENWITH,CMD_FOLDER},&selected);
  expect(!available.at(CMD_OPEN)&&!available.at(CMD_OPENWITH)&&available.at(CMD_FOLDER),
   "Pending scanner work disables both file actions while the folder remains accessible");
  for(auto id:{CMD_OPEN,CMD_OPENWITH}){
   const auto notice=MissingWarning::observe(frame.GetSafeHwnd(),[&]{frame.command(id);});
   expect(notice.title.empty(),"Invoking a pending-scan command does not reach the file action");
  }
  job->data["ScanResult"]={{"Status","Interrupted"}};
  available=frame.commandStates({CMD_OPEN,CMD_OPENWITH},&selected);
  expect(available.at(CMD_OPEN)&&available.at(CMD_OPENWITH),"An idle completed job still offers explicit file actions after an interrupted scan");
  for(auto id:{CMD_OPEN,CMD_OPENWITH}){
   const auto notice=MissingWarning::observe(frame.GetSafeHwnd(),[&]{frame.command(id);});
   expect(notice.explainsMove(path)&&fs::is_directory(path),"Main-list file actions reject a directory substituted for the saved file");
  }
  frame.command(CMD_DOUBLE_OPEN);
  auto notice=MissingWarning::observe(frame.GetSafeHwnd(),enter);
  expect(notice.explainsMove(path),"Enter on the selected row uses the configured Open action");
  frame.table.SetItemState(-1,0,LVIS_SELECTED);
  notice=MissingWarning::observe(frame.GetSafeHwnd(),enter);
  expect(notice.title.empty()&&frame.selected().empty(),"Enter does not activate a focused but unselected row");
  fs::remove(path);
  fs::rename(moved,path);
  job->data["QueueMember"]=false;
  frame.command(CMD_DOUBLE_PROPERTIES);
  const auto currentRow=std::find(frame.visible.begin(),frame.visible.end(),job);
  expect(currentRow!=frame.visible.end(),"The same row remains available for the properties action");
  frame.table.SetItemState((int)std::distance(frame.visible.begin(),currentRow),LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
  notice=MissingWarning::observe(frame.GetSafeHwnd(),enter);
  expect(notice.title==L"File Properties","Enter opens File Properties when it is the configured action");
  job->data.erase("ScanResult");
  auto paused=manager.add("https://example.invalid/keyboard-progress.bin","","keyboard-progress.bin","Main queue",true);
  frame.refresh();
  const auto pausedRow=std::find(frame.visible.begin(),frame.visible.end(),paused);
  expect(pausedRow!=frame.visible.end(),"The paused row is available for keyboard activation");
  frame.table.SetItemState(-1,0,LVIS_SELECTED|LVIS_FOCUSED);
  frame.table.SetItemState((int)std::distance(frame.visible.begin(),pausedRow),LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
  enter();
  const auto progress=std::find_if(frame.progress.begin(),frame.progress.end(),[&](const auto& window){
   return window->GetSafeHwnd()&&window->downloadId()==paused->id();
  });
  expect(progress!=frame.progress.end(),"Enter opens the progress window for the paused row");
  (*progress)->DestroyWindow();
  manager.remove(paused);
  frame.ShowWindow(SW_HIDE);
 }
 {
  auto first=manager.add("https://example.invalid/move-first.bin","","move-first.bin","Main queue",true);
  auto second=manager.add("https://example.invalid/move-second.bin","","move-second.bin","Main queue",true);
  first->data["Status"]=second->data["Status"]="Complete";
  const auto firstSource=first->target(),secondSource=second->target();
  writeBytes(firstSource,Bytes{'a'});writeBytes(secondSource,Bytes{'b'});
  const std::vector<JobPtr> chosen={first,second};
  const auto states=frame.commandStates({CMD_RELOCATE,CMD_OPEN},&chosen);
  expect(states.at(CMD_RELOCATE)&&!states.at(CMD_OPEN),"Completed selection enables batch Move/Rename and keeps Open single-file");
  const auto folder=manager.root.parent_path()/L"moved";
  const auto firstMoved=folder/firstSource.filename(),secondMoved=folder/secondSource.filename();
  RelocationWorkDialog success(&frame,manager,{{first,firstMoved},{second,secondMoved}});success.run();
  expect(!fs::exists(firstSource)&&!fs::exists(secondSource)&&first->target()==firstMoved&&second->target()==secondMoved,
   "Batch Move/Rename updates both saved paths and preserves names");
  expect(readText(firstMoved)=="a"&&readText(secondMoved)=="b","Batch Move/Rename preserves both files' bytes");
  second->data["ScanResult"]={{"Status","Running"}};
  expect(shellDropPaths(frame.completedDragPaths(chosen))==std::vector<fs::path>{firstMoved},
   "A mixed main-list drag includes the ready relocated file and skips the running scan");
  second->data["ScanResult"]={{"Status","Finished"}};
  expect(shellDropPaths(frame.completedDragPaths(chosen))==std::vector<fs::path>{firstMoved,secondMoved},
   "A finished scan restores the relocated file to the drag in selection order");
  second->data.erase("ScanResult");
  writeBytes(secondSource,Bytes{'x'});
  RelocationWorkDialog collision(&frame,manager,{{first,firstSource},{second,secondSource}});
  std::string failure;try{collision.run();}catch(const std::exception& e){failure=e.what();}
  expect(failure.find("1 of 2 files processed")!=std::string::npos&&failure.find("move-second.bin")!=std::string::npos,
   "Batch Move/Rename identifies the failing file and reports prior work");
  expect(first->target()==firstSource&&readText(firstSource)=="a"&&!fs::exists(firstMoved)&&second->target()==secondMoved&&
   readText(secondMoved)=="b"&&readText(secondSource)=="x","A later collision leaves the earlier move committed and overwrites nothing");
  {Lock lock(manager.mutex);second->data["Status"]="Paused";}
  expect(!frame.commandStates({CMD_RELOCATE},&chosen).at(CMD_RELOCATE),"Mixed completed and unfinished selection disables Move/Rename");
  expect(shellDropPaths(frame.completedDragPaths(chosen))==std::vector<fs::path>{firstSource},
   "A paused file cannot enter a mixed completed-file drag even when its path exists");
  manager.remove(first);manager.remove(second);
 }

 auto reference=Json::parse(readText(referencePath));Json dialog;for(auto& d:reference["dialogs"])if(num(d,"id")==286)dialog=d;expect(!dialog.is_null(),"Reference contains Download complete dialog 286");
 {
  auto media=std::make_shared<Job>(job->data);
  media->data["SourceUrl"]="https://example.invalid/watch";
  media->video=std::make_shared<Job>(Json{{"Id",guid()},{"FileName","video.mp4"},{"Url","https://example.invalid/video.mp4"}});
  media->audio=std::make_shared<Job>(Json{{"Id",guid()},{"FileName","audio.m4a"},{"Url","https://example.invalid/audio.m4a"}});
  for(auto track:{media->video,media->audio}){
   auto& segments=track->data["Segments"];
   for(int index=0;index<4096;++index)segments.push_back({{"Index",index},{"Start",4096*index},{"Done",4096}});
  }
  const auto inputs=downloadAddressInputs(*media);
  expect(inputs.size()==2&&inputs["Video"].size()==1&&!inputs.contains("Audio"),
   "Split completion address copies only the selected primary URL even with populated journals");
  frame.complete(media,true);
  const auto window=frame.completions.back()->GetSafeHwnd();
  expect(named(window,L"https://example.invalid/video.mp4")&&named(window,media->target().c_str()),
   "Split completion displays the selected video URL and final saved path");
  SendMessageW(named(window,L"Close"),BM_CLICK,0,0);

  media->audio->data["Url"]="https://";frame.complete(media,true);
  const auto primaryWindow=frame.completions.back()->GetSafeHwnd();
  expect(named(primaryWindow,L"Address")&&named(primaryWindow,L"https://example.invalid/video.mp4"),"Invalid secondary audio cannot hide a valid completion video address");
  SendMessageW(named(primaryWindow,L"Close"),BM_CLICK,0,0);
  media->audio->data["Url"]="https://example.invalid/audio.m4a";

  media->data["ProtectedAdaptive"]=protect(Json{{"type","hls"},{"manifestUrl","https://example.invalid/legacy.m3u8"}}.dump());
  expect(downloadAddress(downloadAddressInputs(*media))=="https://example.invalid/legacy.m3u8","Legacy adaptive completion retains its manifest address");
  media->data["ProtectedAdaptiveDisplayAddress"]=protect("https://example.invalid/cached.m3u8");
  const auto cached=downloadAddressInputs(*media);
  expect(cached.size()==1&&downloadAddress(cached)=="https://example.invalid/cached.m3u8","Cached manifest display excludes the full adaptive plan");
  media->data["ProtectedSabr"]=protect(Json{{"url","https://example.invalid/sabr"}}.dump());
  expect(downloadAddress(downloadAddressInputs(*media))=="https://example.invalid/sabr","SABR keeps address precedence over cached adaptive metadata");
 }
 {
  auto redirected=std::make_shared<Job>(job->data);redirected->data["ProtectedResolvedUrl"]="not-a-valid-encrypted-context";
  frame.complete(redirected,true);const auto window=frame.completions.back()->GetSafeHwnd();
  expect(named(window,L"Address")&&named(window,wide(str(redirected->data,"Url")).c_str()),"Unreadable secondary redirect does not hide the completed primary address");
  expect(IsWindowEnabled(named(window,L"Open"))!=FALSE,"Primary-only address display keeps verified local Open enabled");
  render(window,output/L"primary-address.png");SendMessageW(named(window,L"Close"),BM_CLICK,0,0);
 }
 const std::vector<std::pair<std::string,std::string>> unreadableContexts={
  {"ProtectedSabr","not-a-valid-encrypted-context"},{"ProtectedAdaptive","not-a-valid-encrypted-context"},
  {"ProtectedAdaptiveDisplayAddress","not-a-valid-encrypted-context"},
  {"ProtectedSabr",protect("{")},{"ProtectedAdaptive",protect("{")},
  {"ProtectedAdaptiveDisplayAddress",protect("https://")}};
 for(const auto& [key,value]:unreadableContexts){
  auto unreadable=std::make_shared<Job>(job->data);unreadable->data[key]=value;
  const auto count=frame.completions.size();frame.complete(unreadable,true);
  expect(frame.completions.size()==count+1&&frame.completions.back()->GetSafeHwnd(),"Unreadable saved address context cannot prevent completion from opening");
  const auto window=frame.completions.back()->GetSafeHwnd();
  expect(named(window,L"Address (unavailable)")&&named(window,unreadable->target().c_str()),"Unreadable address is identified without hiding the verified destination");
  HWND addressField=nullptr;
  for(auto child=GetWindow(window,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT)){
   wchar_t type[80]{};GetClassNameW(child,type,80);if(!_wcsicmp(type,L"Edit")){addressField=child;break;}
  }
  expect(addressField&&caption(addressField).empty()&&(GetWindowLongW(addressField,GWL_STYLE)&ES_READONLY),"Unavailable address stays empty and read-only rather than guessing a different source");
  for(const auto* title:{L"Open",L"Open with...",L"Open folder",L"Close"})expect(IsWindowEnabled(named(window,title))!=FALSE,"Unreadable remote metadata leaves local completion actions enabled");
  expect(str(unreadable->data,key.c_str())==value,"Viewing completion does not erase unreadable saved request context");
  if(&key==&unreadableContexts.front().first)render(window,output/L"unavailable-address.png");
  SendMessageW(named(window,L"Close"),BM_CLICK,0,0);expect(!IsWindow(window),"Completion with unreadable metadata closes normally");
 }
 frame.complete(job,true);expect(!frame.completions.empty()&&frame.completions.back()->GetSafeHwnd(),"Actual completed record opens its completion dialog");auto w=frame.completions.back()->GetSafeHwnd();
 for(auto title:{L"Open",L"Open with...",L"Open folder",L"Close"})expect(IsWindowEnabled(named(w,title))!=FALSE,"Completion action is enabled for a saved file");
 auto drag=named(w,L"completed.bin");wchar_t dragText[128]{};LVITEMW dragItem{};dragItem.iSubItem=0;dragItem.pszText=dragText;dragItem.cchTextMax=128;SendMessageW(drag,LVM_GETITEMTEXTW,0,(LPARAM)&dragItem);expect(SendMessageW(drag,LVM_GETITEMCOUNT,0,0)==1&&dragText[0]==0,"Drag area contains one icon without clipped filename text");
 std::vector<HWND> edits;for(auto c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){wchar_t type[80]{};GetClassNameW(c,type,80);if(!_wcsicmp(type,L"Edit"))edits.push_back(c);}
 expect(edits.size()==2&&caption(edits[0])==L"https://example.invalid/completed.bin"&&caption(edits[1])==job->target().wstring(),"Completion shows exact source and saved destination");for(auto field:edits)expect((GetWindowLongW(field,GWL_STYLE)&ES_READONLY)!=0,"Completion source and destination are read-only");
 const std::array<HWND,8> tabOrder={named(w,L"Open"),named(w,L"Open with..."),named(w,L"Open folder"),
  named(w,L"Close"),named(w,L"Don't show this dialog again"),edits[0],edits[1],drag};
 bool ordered=true;
 HWND next=nullptr;
 for(auto control:tabOrder){next=GetNextDlgTabItem(w,next,FALSE);ordered&=next==control;}
 expect(ordered&&GetNextDlgTabItem(w,next,FALSE)==tabOrder.front()&&
  GetNextDlgTabItem(w,tabOrder.front(),TRUE)==tabOrder.back(),"Completion tab order and wrap match the reference");
 bool referenceBorders=true;
 for(UINT dpi:{96u,144u,192u}){auto suggested=bounds(w);SendMessageW(w,WM_DPICHANGED,MAKEWPARAM(dpi,dpi),(LPARAM)&suggested);CFont font;font.CreateFontW(-MulDiv(8,dpi,72),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,L"Tahoma");auto dc=GetDC(w);auto units=DialogUnits::measure(dc,(HFONT)font.GetSafeHandle());ReleaseDC(w,dc);
  for(auto& ref:dialog["controls"]){const auto target=CRect(units.rect((int)ref["x"],(int)ref["y"],(int)ref["width"],(int)ref["height"]));std::wstring type;if(ref["class"].is_string())type=wide(ref["class"]);else{int role=ref["class"]["ordinal"];type=role==128?L"Button":role==129?L"Edit":L"Static";}bool found=false;
   for(auto c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){wchar_t actual[80]{};GetClassNameW(c,actual,80);if(_wcsicmp(type.c_str(),actual)||bounds(c,w)!=target)continue;if(!ref["title"].is_string()||str(ref,"title").empty()||str(ref,"title")=="List1"||str(ref,"title")=="Downloaded xx bytes."||caption(c)==wide(str(ref,"title")))found=true;}
   if(!found)atomicText(output/L"geometry-failure.json",Json{{"dpi",dpi},{"reference",ref}}.dump(2),false);expect(found,"Completion reference control class, label and geometry match");
  }referenceBorders&=render(w,output/wide("completion-"+std::to_string(dpi)+".png"),edits);
 }
 const auto savedPath=job->target(),movedPath=output/L"moved-fixture.bin";
 fs::rename(savedPath,movedPath);
 auto missingOpen=MissingWarning::observe(w,L"Open");
 auto missingOpenWith=MissingWarning::observe(w,L"Open with...");
 fs::create_directory(savedPath);
 const auto folderOpen=MissingWarning::observe(w,L"Open");
 const auto folderOpenWith=MissingWarning::observe(w,L"Open with...");
 expect(folderOpen.explainsMove(savedPath)&&folderOpenWith.explainsMove(savedPath)&&IsWindow(w),
  "A directory at the saved file path is rejected by both completion file actions");
 expect(fs::is_directory(savedPath),"Rejecting the substituted directory leaves it untouched");
 fs::remove(savedPath);
 fs::rename(movedPath,savedPath);
 atomicText(output/L"reference-behavior.json",Json{{"plainEditBorders",referenceBorders},{"openMovedWarning",missingOpen.explainsMove(savedPath)},{"openWithMovedWarning",missingOpenWith.explainsMove(savedPath)}}.dump(2),false);
 expect(missingOpen.explainsMove(savedPath)&&IsWindow(w),"Open explains the moved file and retains completion");
 expect(missingOpenWith.explainsMove(savedPath)&&IsWindow(w),"Open with explains the moved file and retains completion");
 expect(referenceBorders,"Completion text fields use the reference plain border");
 FileDragIcon* icon=nullptr;
 for(auto control=GetWindow(w,GW_CHILD);control&&!icon;control=GetWindow(control,GW_HWNDNEXT))
  icon=dynamic_cast<FileDragIcon*>(CWnd::FromHandlePermanent(control));
 const auto dragControl=dynamic_cast<FileDragList*>(CWnd::FromHandlePermanent(drag));
 expect(icon&&dragControl,"The completion dialog exposes both native drag controls");
 const auto intermediatePath=output/L"relocated"/L"Intermediate.bin";
 manager.relocate(job,intermediatePath);
 writeBytes(savedPath,Bytes{9,9,9});
 expect(icon->path==savedPath&&caption(edits[1])==savedPath.wstring(),
  "The old destination is still displayed before a timer tick");
 expect(shellDropPaths({icon->dragPath()})==std::vector<fs::path>{intermediatePath},
  "The large icon resolves the current file when dragged before a timer tick");
 fs::remove(savedPath);

 const auto relocatedPath=output/L"relocated"/L"Renamed \u2603.zip";
 manager.relocate(job,relocatedPath);
 writeBytes(intermediatePath,Bytes{8,8,8});
 expect(dragControl->path==intermediatePath,"The list handle still displays the previous path before its next drag");
 expect(shellDropPaths({dragControl->dragPath()})==std::vector<fs::path>{relocatedPath},
  "The list handle resolves the renamed Unicode file before a timer tick");
 fs::remove(intermediatePath);
 SendMessageW(w,WM_TIMER,7,0);
 expect(IsWindow(w)&&caption(edits[1])==relocatedPath.wstring()&&caption(edits[0])==L"https://example.invalid/completed.bin",
  "A committed Move/Rename updates the read-only saved path without replacing the completion window");
 expect(icon&&dragControl&&icon->path==relocatedPath&&dragControl->path==relocatedPath&&caption(drag)==relocatedPath.filename().wstring(),
  "Both drag controls follow the same Unicode destination as the path field");
 expect(shellDropPaths({icon->dragPath()})==std::vector<fs::path>{relocatedPath},
  "Windows reads the exact renamed Unicode path from the large icon's file drop");
 expect(shellDropPaths({dragControl->dragPath()})==std::vector<fs::path>{relocatedPath},
  "Windows reads the exact renamed Unicode path from the list handle's file drop");
 const auto secondPath=output/L"second-drag.bin";
 writeBytes(secondPath,Bytes{5,6,7});
 expect(shellDropPaths({savedPath,icon->path,relocatedPath.parent_path(),secondPath})==std::vector<fs::path>{relocatedPath,secondPath},
  "The shared multi-file drop preserves file order and skips missing files and folders");
 job->data["ScanResult"]={{"Status","Pending"}};
 const auto open=named(w,L"Open"),openWith=named(w,L"Open with...");
 expect(IsWindowEnabled(open)&&IsWindowEnabled(openWith),"A new scan can arrive before the completion controls refresh");
 fs::rename(relocatedPath,movedPath);
 for(auto button:{open,openWith}){
  const auto warning=MissingWarning::observe(w,[&]{SendMessageW(w,WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(button),BN_CLICKED),(LPARAM)button);});
  expect(warning.title.empty()&&IsWindow(w),"A queued completion command rechecks the pending scan before using its path");
 }
 expect(!IsWindowEnabled(open)&&!IsWindowEnabled(openWith)&&IsWindowEnabled(named(w,L"Open folder"))&&IsWindowEnabled(named(w,L"Close")),
  "Pending scanning disables completion file actions while the folder and Close remain available");
 expect(icon->dragPath().empty()&&dragControl->dragPath().empty(),"Neither completion drag control supplies a path during a scan");
 job->data.erase("ScanResult");
 SendMessageW(w,WM_TIMER,7,0);
 expect(IsWindowEnabled(open)&&IsWindowEnabled(openWith)&&icon->dragPath()==relocatedPath&&dragControl->dragPath()==relocatedPath,
  "After scanning the existing dialog restores both actions and both drag paths");
 expect(shellDropPaths({icon->dragPath()}).empty(),"A file moved outside UDM yields no stale file-drop data");
 auto missingRelocated=MissingWarning::observe(w,L"Open");
 fs::rename(movedPath,relocatedPath);
 expect(missingRelocated.explainsMove(relocatedPath)&&IsWindow(w),"Open reports the updated destination if the relocated file is later moved externally");
 const auto suppress=named(w,L"Don't show this dialog again");
 frame.complete(job,true);
 const auto peer=frame.completions.back()->GetSafeHwnd();
 const auto peerSuppress=named(peer,L"Don't show this dialog again");
 {
  Handle locked(CreateFileW((manager.root/L"state.json.tmp").c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,0,nullptr));
  expect(static_cast<bool>(locked),"Hold the catalog write lock for a failed suppression change");
  const auto warning=MissingWarning::observe(w,[&]{SendMessageW(suppress,BM_CLICK,0,0);});
  expect(!warning.title.empty()&&SendMessageW(suppress,BM_GETCHECK,0,0)==BST_UNCHECKED&&
   !yes(manager.state["Settings"],"SuppressCompletionDialog")&&IsWindow(w),
   "A failed suppression write restores the checkbox and keeps completion open");
 }
 SendMessageW(suppress,BM_CLICK,0,0);
 expect(IsWindow(w)&&IsWindow(peer)&&yes(manager.state["Settings"],"SuppressCompletionDialog"),
  "Checking Don't show again saves the preference before either completion dialog closes");
 {Manager reopened(manager.root);expect(yes(reopened.state["Settings"],"SuppressCompletionDialog"),
  "Completion suppression survives catalog reopen before closing the dialog");}
 const auto count=frame.completions.size();
 frame.complete(job,false);
 expect(frame.completions.size()==count,"A later automatic completion is suppressed while the first dialog is still open");
 SendMessageW(peer,WM_TIMER,7,0);
 expect(SendMessageW(peerSuppress,BM_GETCHECK,0,0)==BST_CHECKED,"Another open completion reflects the shared preference");
 SendMessageW(peerSuppress,BM_CLICK,0,0);
 SendMessageW(w,WM_TIMER,7,0);
 expect(!yes(manager.state["Settings"],"SuppressCompletionDialog")&&SendMessageW(suppress,BM_GETCHECK,0,0)==BST_UNCHECKED,
  "Unchecking restores automatic completion and synchronizes the first dialog");
 frame.complete(job,false);
 expect(frame.completions.size()==count+1,"Automatic completion opens again after unchecking suppression");
 frame.completions.back()->SendMessage(WM_CLOSE);
 SetActiveWindow(w);
 SendMessageW(suppress,BM_CLICK,0,0);
 expect(yes(manager.state["Settings"],"SuppressCompletionDialog"),
  "The reactivated completion saves suppression again before Close");
 SendMessageW(named(w,L"Close"),BM_CLICK,0,0);
 expect(!IsWindow(w)&&IsWindow(peer)&&yes(manager.state["Settings"],"SuppressCompletionDialog"),
  "Close dismisses only its own completion and preserves the saved preference");
 SendMessageW(peer,WM_CLOSE,0,0);
 frame.complete(job,true);
 const auto explicitWindow=frame.completions.back()->GetSafeHwnd();
 expect(explicitWindow&&SendMessageW(named(explicitWindow,L"Don't show this dialog again"),BM_GETCHECK,0,0)==BST_CHECKED,
  "An explicit completion still opens and shows the current suppression preference");
 SendMessageW(explicitWindow,WM_CLOSE,0,0);
 const bool originalDark=uiDark;
 for(const bool dark:{false,true}){
  uiDark=dark;
  frame.complete(job,true);
  auto& completion=*frame.completions.back();
  const auto window=completion.GetSafeHwnd();
  SetActiveWindow(window);
  auto press=[&](HWND control,WPARAM key){
   SetFocus(control);
   MSG input{};input.hwnd=control;input.message=WM_KEYDOWN;input.wParam=key;input.lParam=1;
   if(!static_cast<CWnd&>(completion).PreTranslateMessage(&input)){
    TranslateMessage(&input);
    DispatchMessageW(&input);
   }
  };
  fs::rename(relocatedPath,movedPath);
  const auto warning=MissingWarning::observe(window,[&]{press(named(window,L"Open"),VK_RETURN);});
  const auto chooserWarning=IsWindow(window)?
   MissingWarning::observe(window,[&]{press(named(window,L"Open with..."),VK_RETURN);}):MissingWarning{};
  fs::rename(movedPath,relocatedPath);
  expect(warning.explainsMove(relocatedPath)&&IsWindow(window),
   dark?"Enter invokes the focused dark completion Open button":"Enter invokes the focused light completion Open button");
  expect(chooserWarning.explainsMove(relocatedPath)&&IsWindow(window),
   dark?"Enter invokes the focused dark completion Open with button":"Enter invokes the focused light completion Open with button");
  press(named(window,L"Close"),VK_RETURN);
  expect(!IsWindow(window),dark?"Enter invokes the focused dark Close button":"Enter invokes the focused light Close button");
 }
 uiDark=originalDark;
 {
  auto& suppression=manager.state["Settings"]["SuppressCompletionDialog"];
  const auto originalSuppression=suppression;
  suppression=false;
  const auto before=frame.completions.size();
  manager.filePublished(job);frame.presentTransferEvents();
  expect(frame.completions.size()==before+1&&IsWindowVisible(frame.completions.back()->GetSafeHwnd()),
   "The publication event shows completion before the transfer finishes its cleanup");
  auto publishedWindow=frame.completions.back()->GetSafeHwnd();
  SendMessageW(named(publishedWindow,L"Close"),BM_CLICK,0,0);
  manager.event(job,true);frame.presentTransferEvents();
  expect(frame.completions.size()==before+1&&!IsWindow(publishedWindow),
   "Finishing cleanup does not reopen a dismissed publication dialog");
  job->data["ScanResult"]={{"Status","Pending"}};
  manager.filePublished(job);frame.presentTransferEvents();
  expect(frame.completions.size()==before+1,"A pending scan prevents early completion presentation");
  job->data.erase("ScanResult");
  job->data["SuppressCompletionDialog"]=true;
  manager.filePublished(job);frame.presentTransferEvents();
  expect(frame.completions.size()==before+1,"Early publication respects completion suppression");
  job->data.erase("SuppressCompletionDialog");
  suppression=originalSuppression;
 }
 {
  const auto progressCount=frame.progress.size();
  manager.event(job,false);frame.presentTransferEvents();
  expect(frame.progress.size()==progressCount,"A finished transfer with automatic close skips a stale Start event");
  job->data["CloseProgressOnCompletion"]=false;
  job->data["PrefetchedBytesAtStart"]=4;job->data["PrefetchPeakRate"]=1024;
  manager.event(job,false);manager.event(job,true);frame.presentTransferEvents();
  expect(frame.progress.size()==progressCount+1&&frame.progress.back()->GetSafeHwnd(),"A completed prefetch honors the keep-open progress preference");
  bool peakVisible=false;
  for(auto child=GetWindow(frame.progress.back()->GetSafeHwnd(),GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT))
   peakVisible|=caption(child).find(L"Prefetch peak 1.000 KB/sec")!=std::wstring::npos;
  expect(peakVisible,"The retained progress window shows the measured prefetch peak");
  frame.progress.back()->DestroyWindow();
  job->data.erase("CloseProgressOnCompletion");job->data.erase("PrefetchedBytesAtStart");job->data.erase("PrefetchPeakRate");
 }
 const std::vector<std::pair<std::string,std::string>> scanCases={
  {"Finished","Microsoft Defender completed the scan: no malware found or detected malware remediated. Review Windows Security for details."},
  {"Attention","Microsoft Defender reports an unresolved detection or a scanning error. Review Windows Security before opening the file."},
  {"Interrupted","UDM closed before the scanner result was recorded. Review your antivirus or run a new check."},
  {"Failed","Cannot save scanner result: Access is denied for C:\\Users\\Example\\Downloads\\"+std::string(160,'x')+"\\state.json. The antivirus may still be running. Review its result and the destination permissions before trying again."}
 };
 for(size_t scenario=0;scenario<scanCases.size();++scenario){
  job->data["ScanResult"]={{"Status",scanCases[scenario].first},{"Message",scanCases[scenario].second},{"ExitCode",scenario?2:0}};
  frame.complete(job,true);auto scanWindow=frame.completions.back()->GetSafeHwnd();expect(scanWindow!=nullptr,"Scanner result opens the expanded completion dialog");
  auto expected=wide("Virus check: "+scannerSummary(job->data));auto summary=named(scanWindow,expected.c_str());
  auto actionsFit=[&](UINT dpi){
   RECT client{};GetClientRect(scanWindow,&client);
   for(auto title:{L"Open",L"Open with...",L"Open folder",L"Close",L"Don't show this dialog again"}){
    const auto control=bounds(named(scanWindow,title),scanWindow);
    if(control.left>=0&&control.top>=0&&control.right<=client.right&&control.bottom<=client.bottom)continue;
    atomicText(output/L"scanner-geometry-failure.json",Json{{"scenario",scanCases[scenario].first},{"control",utf8(title)},{"dpi",dpi},{"windowDpi",GetDpiForWindow(scanWindow)},{"clientRight",client.right},{"clientBottom",client.bottom},{"controlRight",control.right},{"controlBottom",control.bottom}}.dump(2),false);
    return false;
   }
   return true;
  };
  expect(actionsFit(GetDpiForWindow(scanWindow)),"Initial scanner actions and suppression control fit");
  if(scenario==0){
   expect(SetWindowSubclass(scanWindow,expandNonClientFrame,1,0)!=FALSE,"Install the enlarged frame metrics fixture");
   auto suggested=bounds(scanWindow);
   SendMessageW(scanWindow,WM_DPICHANGED,MAKEWPARAM(120,120),(LPARAM)&suggested);
   expect(actionsFit(120),"Scanner actions fit when the actual frame exceeds the calculated frame");
   RemoveWindowSubclass(scanWindow,expandNonClientFrame,1);
  }
  for(UINT dpi:{96u,120u,144u,192u}){
   auto suggested=bounds(scanWindow);SendMessageW(scanWindow,WM_DPICHANGED,MAKEWPARAM(dpi,dpi),(LPARAM)&suggested);
   auto summaryRect=bounds(summary,scanWindow),openRect=bounds(named(scanWindow,L"Open"),scanWindow);RECT client{};GetClientRect(scanWindow,&client);
   expect(summaryRect.bottom<=openRect.top&&summaryRect.left>=0&&summaryRect.right<=client.right,"Scanner summary fits above actions without overlap");
   expect(actionsFit(dpi),"Expanded scanner actions and suppression control stay inside the dialog");
   wchar_t type[80]{};GetClassNameW(summary,type,80);auto style=GetWindowLongW(summary,GWL_STYLE);
   bool scrollable=!_wcsicmp(type,L"Edit")&&(style&ES_READONLY)&&(style&ES_MULTILINE)&&(style&WS_VSCROLL);
   auto dc=GetDC(summary);auto old=SelectObject(dc,(HFONT)SendMessageW(summary,WM_GETFONT,0,0));RECT needed{0,0,summaryRect.Width(),0};DrawTextW(dc,expected.c_str(),(int)expected.size(),&needed,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);SelectObject(dc,old);ReleaseDC(summary,dc);
   if(scrollable&&scenario==3){SendMessageW(summary,EM_LINESCROLL,0,10000);expect(SendMessageW(summary,EM_GETFIRSTVISIBLELINE,0,0)>0&&caption(summary)==expected,"Long scanner diagnostic scrolls while retaining its complete text");}
   render(scanWindow,output/wide("scanner-"+std::to_string(scenario)+"-"+std::to_string(dpi)+".png"));
   expect(scrollable||(needed.bottom<=summaryRect.Height()&&needed.right<=summaryRect.Width()),"Complete scanner diagnostic is readable or scrollable at each DPI");
  }
  SendMessageW(named(scanWindow,L"Close"),BM_CLICK,0,0);expect(!IsWindow(scanWindow),"Expanded scanner dialog closes normally");
 }
 }
};
class CompletionDialogApplication:public CWinApp {
 int result=1;bool started=false;std::unique_ptr<Manager> manager;MainWindow* frame=nullptr;
 void finish(){
  if(frame){frame->SendMessage(WM_CLOSE);frame=nullptr;m_pMainWnd=nullptr;}manager.reset();
  if(!output.empty())atomicText(output/L"results.json",Json{{"passed",result==0},{"error",failure},{"checks",checks}}.dump(2),false);
 }
public:BOOL InitInstance()override{
 CWinApp::InitInstance();INITCOMMONCONTROLSEX common{sizeof(common),ICC_WIN95_CLASSES|ICC_DATE_CLASSES};InitCommonControlsEx(&common);AfxOleInit();
 int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);
 try{
  if(!argv||argc!=3)throw std::runtime_error("Pass fresh output and reference file paths");output=argv[1];referencePath=argv[2];
  if(fs::exists(output)){output.clear();throw std::runtime_error("Fresh output required");}fs::create_directories(output);
  manager=std::make_unique<Manager>(output/L"state");auto prefs=manager->state["Settings"];prefs["DownloadFolder"]=utf8((output/L"files").wstring());prefs["CategoryFolders"]=false;prefs["ClipboardMonitor"]=false;prefs["Sound"]=false;prefs["CloseToTray"]=false;manager->setSettings(prefs);
  frame=new MainWindow(*manager);m_pMainWnd=frame;frame->KillTimer(1);
 }catch(const std::exception& e){failure=e.what();if(argv)LocalFree(argv);finish();return FALSE;}
 LocalFree(argv);return TRUE;
 }
 BOOL OnIdle(LONG)override{
  if(started)return FALSE;started=true;
  try{ToolbarComponentTest::run(*frame);result=0;}catch(const std::exception& e){failure=e.what();}
  finish();PostQuitMessage(result);return FALSE;
 }
 int ExitInstance()override{AfxOleTerm(FALSE);return result;}
};CompletionDialogApplication application;
}
