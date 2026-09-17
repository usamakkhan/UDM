#pragma once
#include <afxwin.h>
#include <afxcmn.h>
#include <afxdlgs.h>
#include <afxdtctl.h>
#include <atlimage.h>
#include <shlobj.h>
#include "Core.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
namespace udm {
inline bool uiDark=false;
inline COLORREF uiBackground(){return uiDark?RGB(32,34,38):GetSysColor(COLOR_WINDOW);}
inline COLORREF uiForeground(){return uiDark?RGB(232,234,238):GetSysColor(COLOR_WINDOWTEXT);}
inline CBrush& uiBrush(){static CBrush dark(RGB(32,34,38));static CBrush light(GetSysColor(COLOR_WINDOW));return uiDark?dark:light;}
inline void openWith(CWnd* owner,const fs::path& path){if(!fs::is_regular_file(path))throw std::runtime_error("The saved file is missing.");auto normalized=fs::absolute(path).lexically_normal().make_preferred();OPENASINFO info{normalized.c_str(),nullptr,OAIF_EXEC};auto filter=AfxOleGetMessageFilter();if(filter){filter->EnableBusyDialog(FALSE);filter->EnableNotRespondingDialog(FALSE);}auto result=SHOpenWithDialog(owner->GetSafeHwnd(),&info);if(filter){filter->EnableBusyDialog(TRUE);filter->EnableNotRespondingDialog(TRUE);}if(FAILED(result)&&result!=HRESULT_FROM_WIN32(ERROR_CANCELLED))throw std::runtime_error("Windows could not open the app chooser (HRESULT "+std::to_string((unsigned long)result)+").");}
inline CString cs(const std::string& s){return CString(wide(s).c_str());}
inline std::string text(CWnd* w){CString s;w->GetWindowText(s);return utf8((LPCWSTR)s);}
inline void error(CWnd* owner,const std::exception& e){owner->MessageBox(cs(e.what()),L"UDM",MB_OK|MB_ICONWARNING);}
inline void openFile(CWnd* owner,const fs::path& path){auto result=ShellExecuteW(owner->GetSafeHwnd(),L"open",path.c_str(),nullptr,nullptr,SW_SHOWNORMAL);if((INT_PTR)result<=32)throw std::runtime_error("Windows could not open this file.");}
inline std::wstring chooseFolder(CWnd* owner,const std::wstring& initial){CFolderPickerDialog dialog(initial.c_str(),OFN_PATHMUSTEXIST,owner);return dialog.DoModal()==IDOK?std::wstring(dialog.GetPathName()):L"";}
class Form:public CDialog {
 DECLARE_MESSAGE_MAP()
 std::string caption;int width,height;
protected:
 afx_msg HBRUSH OnCtlColor(CDC* dc,CWnd* wnd,UINT type){auto brush=CDialog::OnCtlColor(dc,wnd,type);if(type==CTLCOLOR_STATIC||type==CTLCOLOR_BTN||type==CTLCOLOR_EDIT||type==CTLCOLOR_LISTBOX){dc->SetTextColor(uiForeground());dc->SetBkColor(uiBackground());return (HBRUSH)uiBrush().GetSafeHandle();}return brush;}
 afx_msg BOOL OnEraseBkgnd(CDC* dc){CRect area;GetClientRect(&area);dc->FillSolidRect(area,uiBackground());return TRUE;}
 afx_msg void OnTimer(UINT_PTR id){if(pulse)try{pulse();}catch(...){}CDialog::OnTimer(id);}
 UINT nextId=1000;std::vector<std::unique_ptr<CWnd>> controls;std::map<UINT,std::function<void()>> actions;
 CFont font;float scale=1;
 BOOL OnInitDialog()override{CDialog::OnInitDialog();scale=GetDpiForWindow(m_hWnd)/96.0f;font.CreateFontW(-(int)(11*scale),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,L"Tahoma");SetFont(&font);SetWindowText(cs(caption));CRect r(0,0,(int)(width*scale),(int)(height*scale));AdjustWindowRectEx(&r,GetStyle(),FALSE,GetExStyle());SetWindowPos(nullptr,0,0,r.Width(),r.Height(),SWP_NOMOVE|SWP_NOZORDER);SetIcon(AfxGetApp()->LoadIcon(1),TRUE);CenterWindow();if(init)init();if(pulse)SetTimer(7,250,nullptr);return TRUE;}
 BOOL OnCommand(WPARAM w,LPARAM l)override{auto it=actions.find(LOWORD(w));if(it!=actions.end()&&HIWORD(w)==BN_CLICKED){try{it->second();}catch(const std::exception& e){error(this,e);}return TRUE;}return CDialog::OnCommand(w,l);}
 void OnOK()override{if(accept){try{accept();}catch(const std::exception& e){error(this,e);}}}
 void OnCancel()override{if(cancel){try{cancel();}catch(const std::exception& e){error(this,e);}}else if(modeless)DestroyWindow();else CDialog::OnCancel();}
public:
 bool modeless=false;std::function<void()> init,accept,cancel,pulse;
 Form(std::string title,int w,int h,CWnd* parent=nullptr):CDialog(100,parent),caption(std::move(title)),width(w),height(h){}
 CRect rect(int x,int y,int w,int h){return CRect((int)(x*scale),(int)(y*scale),(int)((x+w)*scale),(int)((y+h)*scale));}
 template<class T>T* make(DWORD style,int x,int y,int w,int h,UINT id=0){auto control=std::make_unique<T>();if(!control->Create(style|WS_CHILD|WS_VISIBLE,rect(x,y,w,h),this,id?id:nextId++))throw std::runtime_error("Cannot create interface control.");control->SetFont(&font);auto p=control.get();controls.push_back(std::move(control));return p;}
 CWnd* control(const wchar_t* type,std::string value,DWORD style,int x,int y,int w,int h,DWORD ex=0){auto c=std::make_unique<CWnd>();if(!c->CreateEx(ex,type,cs(value),style|WS_CHILD|WS_VISIBLE,rect(x,y,w,h),this,nextId++))throw std::runtime_error("Cannot create interface control.");c->SetFont(&font);auto p=c.get();controls.push_back(std::move(c));return p;}
 CWnd* label(std::string s,int x,int y,int w,int h=18){return control(L"STATIC",s,SS_LEFT,x,y,w,h);}
 CWnd* edit(std::string s,int x,int y,int w,int h=23,bool readonly=false,bool multi=false,bool secret=false){auto c=control(L"EDIT",s,WS_TABSTOP|ES_AUTOHSCROLL|(readonly?ES_READONLY:0)|(multi?ES_MULTILINE|ES_AUTOVSCROLL|WS_VSCROLL:0)|(secret?ES_PASSWORD:0),x,y,w,h,WS_EX_CLIENTEDGE);c->SendMessage(EM_SETLIMITTEXT,multi?1024*1024:32768);return c;}
 CWnd* check(std::string s,bool value,int x,int y,int w){auto c=control(L"BUTTON",s,WS_TABSTOP|BS_AUTOCHECKBOX,x,y,w,21);c->SendMessage(BM_SETCHECK,value?BST_CHECKED:BST_UNCHECKED);return c;}
 CWnd* button(std::string s,int x,int y,int w,std::function<void()> action){auto c=control(L"BUTTON",s,WS_TABSTOP|BS_PUSHBUTTON,x,y,w,25);actions[c->GetDlgCtrlID()]=std::move(action);return c;}
 CComboBox* combo(const std::vector<std::string>& values,std::string current,int x,int y,int w,bool free=false){auto c=make<CComboBox>(WS_TABSTOP|WS_VSCROLL|(free?CBS_DROPDOWN:CBS_DROPDOWNLIST),x,y,w,230);for(const auto& v:values)c->AddString(cs(v));int index=c->FindStringExact(-1,cs(current));if(index>=0)c->SetCurSel(index);else if(free)c->SetWindowText(cs(current));else if(!values.empty())c->SetCurSel(0);return c;}
 bool checked(CWnd* c){return c->SendMessage(BM_GETCHECK)==BST_CHECKED;}
 void bind(CWnd* c,std::function<void()> action){actions[c->GetDlgCtrlID()]=std::move(action);}
 void close(int code=IDOK){if(modeless)DestroyWindow();else EndDialog(code);}
};
BEGIN_MESSAGE_MAP(Form,CDialog)
 ON_WM_TIMER()
 ON_WM_CTLCOLOR()
 ON_WM_ERASEBKGND()
END_MESSAGE_MAP()
class RefreshAddressDialog:public Form {
 DECLARE_MESSAGE_MAP()
 Manager& manager;JobPtr job;CWnd *address=nullptr,*page=nullptr,*status=nullptr;Json candidate;std::string seen;
 void poll(){auto offer=manager.addressRefreshCandidate(job);if(offer.is_object()&&offer.dump()!=seen){candidate=offer;seen=offer.dump();address->SetWindowText(cs(str(offer,"url")));if(!str(offer,"page").empty())page->SetWindowText(cs(str(offer,"page")));status->SetWindowText(L"A matching browser link arrived. Review the address, then choose Save or Resume.");}}
 void apply(bool resume){std::optional<Headers> headers;if(candidate.is_object()&&text(address)==str(candidate,"url"))headers=candidate["headers"].get<Headers>();manager.refreshAddress(job,trim(text(address)),headers,trim(text(page)));if(resume)manager.resume(job);close();}
 afx_msg void OnTimer(UINT_PTR id){try{poll();}catch(const std::exception& e){status->SetWindowText(cs(e.what()));}Form::OnTimer(id);}
public:
 RefreshAddressDialog(Manager& m,JobPtr j,CWnd* owner):Form("Refresh download address",610,302,owner),manager(m),job(j){init=[this]{
  Json data;{Lock lock(manager.mutex);data=job->data;}
  label(str(data,"FileName"),14,12,580,23);
  label("Open the download page, then send the same file to UDM from the browser. You can also paste a fresh direct link below.",14,42,580,34);
  label("Download page",14,91,96);page=edit(recoveryPage(data),116,87,359);
  button("Open page",485,86,109,[this]{Url url(trim(text(page)));if(url.scheme!="http"&&url.scheme!="https")throw std::runtime_error("Enter the original HTTP or HTTPS download page.");auto result=ShellExecuteW(m_hWnd,L"open",wide(url.full).c_str(),nullptr,nullptr,SW_SHOWNORMAL);if((INT_PTR)result<=32)throw std::runtime_error("Windows could not open the download page.");});
  label("New address",14,126,96);address=edit("",116,122,478,47,false,true);
  status=label("Waiting for a matching link from the browser (up to 10 minutes).",14,181,580,30);
  label("Saved parts: "+bytes(num(data,"Received"))+". Resume checks the file size and server validator before reusing them.",14,215,580,29);
  button("Save address",219,262,116,[this]{apply(false);});button("Save and resume",345,262,135,[this]{apply(true);});button("Cancel",490,262,104,[this]{close(IDCANCEL);});accept=[this]{apply(true);};poll();SetTimer(1,250,nullptr);
 };}
};
BEGIN_MESSAGE_MAP(RefreshAddressDialog,Form)
 ON_WM_TIMER()
END_MESSAGE_MAP()
inline void refreshDownloadAddress(CWnd* owner,Manager& manager,JobPtr job){manager.beginAddressRefresh(job);try{RefreshAddressDialog dialog(manager,job,owner);dialog.DoModal();}catch(...){manager.cancelAddressRefresh(job);throw;}manager.cancelAddressRefresh(job);}
inline std::string prompt(CWnd* parent,std::string title,std::string value=""){Form d(title,380,95,parent);CWnd* input=nullptr;std::string result;d.init=[&]{d.label("Name",10,12,48);input=d.edit(value,60,9,309);d.accept=[&]{result=trim(text(input));if(result.empty()||result.size()>80)throw std::runtime_error("Enter a name up to 80 characters.");d.close();};d.button("OK",205,58,78,d.accept);d.button("Cancel",291,58,78,[&]{d.close(IDCANCEL);});input->SetFocus();};return d.DoModal()==IDOK?result:"";}
inline std::vector<std::string> queueNames(Manager& m){Lock l(m.mutex);std::vector<std::string> names;for(auto q:m.state["Queues"])names.push_back(str(q,"Name"));return names;}
inline void moveCompleted(CWnd* owner,Manager& manager,JobPtr job){
 Form d("Move / Rename",605,151,owner);d.init=[&]{
  d.label("New file location",13,17,116);auto path=d.edit(utf8(job->target().wstring()),132,13,460);
  d.button("Choose folder...",13,53,128,[&d,path]{auto value=chooseFolder(&d,fs::path(wide(text(path))).parent_path().wstring());if(!value.empty())path->SetWindowText(cs(utf8((fs::path(value)/fs::path(wide(text(path))).filename()).wstring())));});
  d.label("Enter a new name or location. Existing files will not be overwritten.",155,57,435,34);
  d.accept=[&,path]{manager.relocate(job,fs::path(wide(trim(text(path)))));d.close();};d.button("Move / Rename",351,110,134,d.accept);d.button("Cancel",496,110,96,[&]{d.close(IDCANCEL);});
 };d.DoModal();
}
inline void completedProperties(CWnd* parent,Manager& manager,JobPtr job){
 Json data;{Lock lock(manager.mutex);data=job->data;}auto headers=readHeaders(data);Form d("File Properties",650,410,parent);
 d.init=[&]{
  d.label("File name",14,17,88);auto name=d.edit(str(data,"FileName"),110,13,526,23,true);
  d.label("Status: Complete     Size: "+bytes(num(data,"Size"))+" ("+std::to_string(num(data,"Size"))+" bytes)",14,49,622);
  d.label("Address",14,80,88);auto url=d.edit(str(data,"Url"),110,76,526);
  d.label("Save to",14,113,88);auto path=d.edit(utf8(job->target().wstring()),110,109,412,23,true);
  d.button("Move...",532,108,104,[&,name,path]{moveCompleted(&d,manager,job);name->SetWindowText(cs(str(job->data,"FileName")));path->SetWindowText(cs(utf8(job->target().wstring())));});
  d.label("Description",14,147,88);auto desc=d.edit(str(data,"Description"),110,143,526);
  d.label("Parent page",14,181,88);auto page=d.edit(recoveryPage(data),110,177,526);
  d.label("Referer",14,215,88);auto referer=d.edit(headers.count("Referer")?headers["Referer"]:"",110,211,526);
  d.label("Authorization",14,249,93);auto auth=d.edit(headers.count("Authorization")?headers["Authorization"]:"",110,245,526,23,false,false,true);
  d.label("SHA-256",14,283,88);d.edit(str(data,"Sha256"),110,279,526,23,true);
  d.label("Category: "+str(data,"Category")+"     Queue: "+str(data,"Queue"),14,315,622);
  d.button("Open",14,367,87,[&]{openFile(&d,job->target());});d.button("Open with...",111,367,105,[&]{openWith(&d,job->target());});d.button("Open folder",226,367,109,[&]{openFile(&d,job->target().parent_path());});
  d.accept=[&,url,desc,page,referer,auth]{auto h=headers;for(auto it=h.begin();it!=h.end();)if(lower(it->first)=="authorization"||lower(it->first)=="referer")it=h.erase(it);else ++it;if(!text(auth).empty())h["Authorization"]=text(auth);if(!text(referer).empty())h["Referer"]=text(referer);validateHeaders(h);manager.updateCompleted(job,{{"Url",trim(text(url))},{"Description",text(desc)},{"DownloadPage",trim(text(page))},{"ProtectedHeaders",h.empty()?"":protect(legacyDictionary(Json(h)).dump())}});d.close();};
  d.button("OK",444,367,91,d.accept);d.button("Cancel",545,367,91,[&]{d.close(IDCANCEL);});
 };d.DoModal();
}
inline JobPtr chooseDuplicate(CWnd* parent,Manager& manager,JobPtr candidate){
 Json original;{Lock lock(manager.mutex);if(str(candidate->data,"DuplicateOf").empty())return candidate;for(auto j:manager.jobs)if(j->id()==str(candidate->data,"DuplicateOf"))original=j->data;}
 Form dialog("Duplicate download link",650,327,parent);JobPtr result;
 dialog.init=[&]{
  dialog.label("This address is already in your download history.",15,14,620,25);
  dialog.label("Existing: "+str(original,"FileName","Record no longer available")+"   |   "+str(original,"Status"),15,47,620,25);
  dialog.edit(str(candidate->data,"Url"),15,79,620,23,true);
  dialog.label("What would you like to do?",15,117,620);
  std::vector<std::string> choices={"Show existing download / resume saved parts","Download another numbered copy"};
  if(str(original,"Status")=="Complete"&&Url(str(candidate->data,"Url")).scheme!="ftp")choices.push_back("Replace completed file; keep its previous version");
  auto choice=dialog.combo(choices,choices[0],15,142,620);
  dialog.label("Replacement keeps the original until the new file is verified. The previous version remains in history under a numbered name.",15,180,620,39);
  auto remember=dialog.check("Remember choice (existing or numbered copy only)",false,15,232,620);
  dialog.accept=[&,choice,remember]{auto mode=choice->GetCurSel()==0?"Existing":choice->GetCurSel()==1?"Numbered":"Replace";result=manager.resolveDuplicate(candidate,mode);if(dialog.checked(remember)&&std::string(mode)!="Replace"){Lock lock(manager.mutex);auto prefs=manager.state["Settings"];prefs["DuplicatePolicy"]=mode;manager.setSettings(prefs);}dialog.close();};
  dialog.button("Continue",427,281,100,dialog.accept);dialog.cancel=[&]{manager.resolveDuplicate(candidate,"Cancel");dialog.close(IDCANCEL);};dialog.button("Cancel",539,281,96,dialog.cancel);
 };dialog.DoModal();return result;
}
inline void downloadInfo(CWnd* parent,Manager& m,JobPtr job,bool properties=false){Form d(properties?"File Properties":"Download File Info",560,properties?367:266,parent);Json j;{Lock l(m.mutex);j=job->data;}d.init=[&]{d.label("URL",10,13,57);auto url=d.edit(str(j,"Url"),78,9,472,23,!properties);d.label("Category",10,47,60);auto cat=d.combo(m.categories(),str(j,"Category"),78,43,160);auto remember=d.check("Remember this path for this category",false,247,43,303);d.label("Save As",10,81,60);auto destination=d.edit(utf8(job->target().wstring()),78,77,431);d.button("...",518,76,32,[&d,destination]{auto folder=chooseFolder(&d,fs::path(wide(text(destination))).parent_path().wstring());if(!folder.empty())destination->SetWindowText(cs(utf8((fs::path(folder)/fs::path(wide(text(destination))).filename()).wstring())));});d.label("Description",10,115,67);auto desc=d.edit(str(j,"Description"),78,111,472);d.label("Queue",10,149,60);auto queue=d.combo(queueNames(m),str(j,"Queue","Main queue"),78,145,190);d.label("Size: "+bytes(num(j,"Size",-1)),288,149,253);CWnd *connections=nullptr,*expected=nullptr,*authorization=nullptr;if(properties){d.label("Connections",10,186,80);connections=d.edit(std::to_string(num(j,"Connections",8)),99,182,64);d.label("SHA-256",10,220,80);expected=d.edit(str(j,"ExpectedSha256"),99,216,451);d.label("Authorization",10,254,84);auto h=readHeaders(j);authorization=d.edit(h.count("Authorization")?h["Authorization"]:"",99,250,451,23,false,false,true);d.label("Error: "+str(j,"Error"),10,284,540,35);}else{auto live=d.label(str(j,"FormatDescription"),10,180,540,36);d.pulse=[&m,job,live]{Lock lock(m.mutex);if(yes(job->data,"ConfirmationPending"))live->SetWindowText(cs("Downloading in background: "+bytes(num(job->data,"Received"))+" / "+bytes(num(job->data,"Size",-1))+". Waiting for your confirmation."));};}auto apply=[&,url,cat,remember,destination,desc,queue,connections,expected,authorization]{if(!properties)m.endPrefetch(job);auto path=fs::path(wide(text(destination)));Json e={{"Url",text(url)},{"Folder",utf8(path.parent_path().wstring())},{"FileName",utf8(path.filename().wstring())},{"Category",text(cat)},{"Description",text(desc)},{"Queue",text(queue)}};if(properties){e["Connections"]=std::stoll(text(connections));e["ExpectedSha256"]=trim(text(expected));auto h=readHeaders(j);if(text(authorization).empty())h.erase("Authorization");else h["Authorization"]=text(authorization);validateHeaders(h);e["ProtectedHeaders"]=h.empty()?"":protect(legacyDictionary(Json(h)).dump());}m.configure(job,e);if(d.checked(remember)){Lock l(m.mutex);auto prefs=m.state["Settings"];auto paths=dictionary(prefs["CategoryPaths"]);paths[text(cat)]=utf8(path.parent_path().wstring());prefs["CategoryPaths"]=legacyDictionary(paths);m.setSettings(prefs);}};int bottom=properties?331:228;if(properties){d.accept=[&,apply]{apply();d.close();};d.button("OK",380,bottom,80,d.accept);}else{d.accept=[&,apply]{apply();m.resume(job);d.close();};d.button("Download Later",195,bottom,114,[&,apply]{apply();m.pause(job);d.close();});d.button("Start Download",317,bottom,123,d.accept);}d.cancel=[&]{if(!properties){m.endPrefetch(job);m.pause(job);}d.close(IDCANCEL);};d.button("Cancel",468,bottom,82,d.cancel);if(!properties)m.beginPrefetch(job);};try{d.DoModal();}catch(...){if(!properties)m.endPrefetch(job);throw;}if(!properties)m.endPrefetch(job);}
inline void presentDownload(CWnd* owner,Manager& manager,JobPtr job){
 job=chooseDuplicate(owner,manager,job);if(!job)return;
 std::string status;bool active;{Lock lock(manager.mutex);status=str(job->data,"Status");active=manager.isActive(job);}
 if(status=="Complete")completedProperties(owner,manager,job);
 else if(active||status=="Queued"){if(manager.event)manager.event(job,false);}
 else downloadInfo(owner,manager,job);
}
inline void addAddress(CWnd* parent,Manager& m,const std::string& initial=""){Form d("Enter new address to download",523,108,parent);JobPtr job;d.init=[&]{d.label("Address",10,13,49);auto address=d.edit(initial,63,9,367);auto auth=d.check("Use authorization",false,10,43,153);d.label("Login",10,78,45);auto user=d.edit("",63,74,156);d.label("Password",233,78,62);auto password=d.edit("",300,74,130,23,false,false,true);user->EnableWindow(FALSE);password->EnableWindow(FALSE);d.button("OK",441,9,72,[&,address,auth,user,password]{Url u(text(address));if(hostIs(u.host,"youtube.com")||u.host=="youtu.be")throw std::runtime_error("Open this video in the browser and choose its quality using the UDM panel.");Headers h;if(d.checked(auth)){auto s=text(user)+":"+text(password);h["Authorization"]="Basic "+b64(Bytes(s.begin(),s.end()));}job=m.offerDownload(text(address),"","","Main queue",true,h);d.close();});d.button("Cancel",441,43,72,[&]{d.close(IDCANCEL);});d.accept=[&,address,auth,user,password]{Url u(text(address));if(hostIs(u.host,"youtube.com")||u.host=="youtu.be")throw std::runtime_error("Use the UDM browser panel for video capture.");Headers h;if(d.checked(auth)){auto s=text(user)+":"+text(password);h["Authorization"]="Basic "+b64(Bytes(s.begin(),s.end()));}job=m.offerDownload(text(address),"","","Main queue",true,h);d.close();};d.bind(auth,[&d,auth,user,password]{user->EnableWindow(d.checked(auth));password->EnableWindow(d.checked(auth));});
 address->SetFocus();};if(d.DoModal()==IDOK&&job)presentDownload(parent,m,job);}
inline void batchDialog(CWnd* parent,Manager& m,std::string initial=""){Form d("Add batch download",585,362,parent);d.init=[&]{d.label("Enter URLs, one per line. Use [001-100] or [a-z] for a sequence.",12,12,558);auto input=d.edit(initial,12,37,560,208,false,true);d.label("Queue",12,263,48);auto queue=d.combo(queueNames(m),"Main queue",70,259,225);auto paused=d.check("Add paused",true,321,260,190);auto report=d.label("",12,299,250);d.accept=[&,input,queue,paused,report]{auto urls=expand(text(input));if(urls.empty())throw std::runtime_error("Enter at least one URL.");for(const auto& url:urls)m.offerDownload(url,"","",text(queue),d.checked(paused));report->SetWindowText(cs("Added "+std::to_string(urls.size())+" downloads."));d.close();};d.button("Add downloads",346,319,133,d.accept);d.button("Cancel",488,319,84,[&]{d.close(IDCANCEL);});};d.DoModal();}
class RangeMap:public CWnd {
 DECLARE_MESSAGE_MAP()
 Json segments=Json::array();i64 total=0;
 afx_msg void OnPaint(){CPaintDC dc(this);CRect r;GetClientRect(&r);dc.FillSolidRect(r,RGB(228,235,242));if(total>0){for(const auto& s:segments){double a=(double)num(s,"Start"),b=(double)num(s,"End")+1,done=std::clamp((double)num(s,"Done"),0.0,std::max(0.0,b-a));int left=(int)(a*r.Width()/total),right=(int)((a+done)*r.Width()/total);if(right>left)dc.FillSolidRect(left,1,right-left,r.Height()-2,RGB(35,139,63));if(left>0)dc.FillSolidRect(left,0,1,r.Height(),RGB(145,159,173));}}dc.Draw3dRect(r,RGB(145,159,173),RGB(145,159,173));}
public:
 BOOL Create(DWORD style,const RECT& rect,CWnd* parent,UINT id){return CWnd::Create(AfxRegisterWndClass(0),L"Downloaded file pieces",style,rect,parent,id);}
 void update(Json value,i64 size){segments=std::move(value);total=size;Invalidate(FALSE);}
};
BEGIN_MESSAGE_MAP(RangeMap,CWnd)
 ON_WM_PAINT()
END_MESSAGE_MAP()
class Progress:public Form {
 DECLARE_MESSAGE_MAP()
 Manager& manager;JobPtr job;CWnd *status=nullptr,*received=nullptr,*rate=nullptr,*remaining=nullptr,*resume=nullptr,*limit=nullptr,*remember=nullptr,*limitEnabled=nullptr,*information=nullptr,*refreshAddressButton=nullptr;CProgressCtrl* bar=nullptr;RangeMap* rangeMap=nullptr;CListCtrl* workers=nullptr;CTabCtrl* tabs=nullptr;std::vector<CWnd*> page0,page1,page2;
 void update(){Json j;std::vector<Worker> rows;double speed;{Lock l(manager.mutex);j=job->data;speed=job->speed;rows=job->workers;if(job->video){rows=job->video->workers;if(job->audio)rows.insert(rows.end(),job->audio->workers.begin(),job->audio->workers.end());}}auto total=num(j,"Size",-1),done=num(j,"Received");auto state=str(j,"Status");if(refreshAddressButton)refreshAddressButton->EnableWindow(manager.canRefreshAddress(job));SetWindowText(cs((total>0?std::to_string((int)std::min(100.0,100.0*done/total))+"% ":"")+str(j,"FileName")));status->SetWindowText(cs("Status: "+state+(str(j,"Error").empty()?"":" - "+str(j,"Error"))));received->SetWindowText(cs("Downloaded: "+bytes(done)+" / "+bytes(total)+(num(j,"AdaptiveTotalSegments")>0?"   Segments: "+std::to_string(num(j,"AdaptiveCompletedSegments"))+" / "+std::to_string(num(j,"AdaptiveTotalSegments")):"")));rate->SetWindowText(cs("Transfer rate: "+bytes(speed)+"/s"));remaining->SetWindowText(cs("Time left: "+(speed>0&&total>done?std::to_string((i64)((total-done)/speed))+" seconds":"--")));resume->SetWindowText(cs(std::string("Resume capability: ")+(yes(j,"RangeSupported")?"Yes":"Fresh transfer required")));bar->SetPos(num(j,"AdaptiveTotalSegments")>0&&state!="Complete"?(int)(1000*num(j,"AdaptiveCompletedSegments")/num(j,"AdaptiveTotalSegments")):total>0?(int)std::min(1000.0,1000.0*done/total):state=="Complete"?1000:0);rangeMap->update(j.value("Segments",Json::array()),total);workers->SetRedraw(FALSE);while(workers->GetItemCount()>(int)rows.size())workers->DeleteItem(workers->GetItemCount()-1);for(int i=0;i<(int)rows.size();++i){auto& w=rows[i];if(i>=workers->GetItemCount())workers->InsertItem(i,cs(std::to_string(i+1)));workers->SetItemText(i,1,cs(bytes(w.received)));workers->SetItemText(i,2,cs(w.state));workers->SetItemText(i,3,cs(w.end>=w.start?std::to_string((int)std::clamp(100.0*(w.position-w.start)/(w.end-w.start+1),0.0,100.0))+"%":"--"));}workers->SetRedraw(TRUE);workers->Invalidate();information->SetWindowText(cs("Saved as: "+utf8(job->target().wstring())+"\r\n\r\nSHA-256: "+str(j,"Sha256")+"\r\n\r\n"+str(j,"FormatDescription")+"\r\nTransferred: "+bytes(num(j,"TransferredBytes"))+"\r\nNetwork time: "+std::to_string(real(j,"TransferSeconds"))+" s\r\nMerge time: "+std::to_string(real(j,"MergeSeconds"))+" s"));}
 void page(){int n=tabs->GetCurSel();for(auto w:page0)w->ShowWindow(n==0?SW_SHOW:SW_HIDE);for(auto w:page1)w->ShowWindow(n==1?SW_SHOW:SW_HIDE);for(auto w:page2)w->ShowWindow(n==2?SW_SHOW:SW_HIDE);}
 afx_msg void OnTimer(UINT_PTR id){try{update();}catch(const std::exception& e){status->SetWindowText(cs(e.what()));}Form::OnTimer(id);}
 BOOL OnNotify(WPARAM w,LPARAM l,LRESULT* result)override{auto hdr=(NMHDR*)l;if(hdr->hwndFrom==tabs->m_hWnd&&hdr->code==TCN_SELCHANGE){page();*result=0;return TRUE;}return Form::OnNotify(w,l,result);}
public:
 std::string downloadId()const{return job->id();}
 Progress(Manager& m,JobPtr j,CWnd* owner):Form("Download status",535,408,owner),manager(m),job(j){modeless=true;init=[this]{tabs=make<CTabCtrl>(WS_TABSTOP,8,8,519,345);tabs->InsertItem(0,L"Download status");tabs->InsertItem(1,L"Speed Limiter");tabs->InsertItem(2,L"Download information");status=label("",22,43,491,34);received=label("",22,80,491);rate=label("",22,104,250);remaining=label("",290,104,210);resume=label("",22,128,491);bar=make<CProgressCtrl>(PBS_SMOOTH,22,153,491,18);bar->SetRange32(0,1000);rangeMap=make<RangeMap>(0,22,181,491,16);workers=make<CListCtrl>(LVS_REPORT|LVS_SINGLESEL|LVS_NOSORTHEADER,22,205,491,132);workers->SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_DOUBLEBUFFER);workers->InsertColumn(0,L"N.",LVCFMT_LEFT,32);workers->InsertColumn(1,L"Downloaded",LVCFMT_LEFT,90);workers->InsertColumn(2,L"Info",LVCFMT_LEFT,259);workers->InsertColumn(3,L"Progress",LVCFMT_LEFT,79);page0={status,received,rate,remaining,resume,bar,rangeMap,workers};limitEnabled=check("Use Speed Limiter",num(job->data,"LimitKbps")>0,25,54,330);auto hint=label("Maximum download speed (KB/s)",25,94,285);limit=edit(std::to_string(num(job->data,"LimitKbps",1000)),318,90,110);remember=check("Remember this limit for this download",false,25,129,458);auto apply=button("Apply",345,169,83,[this]{auto kb=checked(limitEnabled)?std::stoll(text(limit)):0;if(kb<0||kb>1000000)throw std::runtime_error("Use a speed limit from 0 to 1,000,000 KB/s.");Lock l(manager.mutex);if(checked(remember)){job->data["LimitKbps"]=kb;job->sessionLimit.reset();manager.save();}else job->sessionLimit=kb;});page1={limitEnabled,hint,limit,remember,apply};information=edit("",24,47,486,283,true,true);page2={information};refreshAddressButton=button("Refresh address",15,369,139,[this]{refreshDownloadAddress(this,manager,job);});button("Pause",253,369,83,[this]{manager.pause(job);});button("Resume",345,369,83,[this]{manager.resume(job);});button("Hide",437,369,83,[this]{DestroyWindow();});page();update();SetTimer(1,500,nullptr);};}
};
BEGIN_MESSAGE_MAP(Progress,Form)
 ON_WM_TIMER()
END_MESSAGE_MAP()
}
