#pragma once
#include <afxwin.h>
#include <afxcmn.h>
#include <afxdlgs.h>
#include <afxdtctl.h>
#include <afxole.h>
#include <atlimage.h>
#include <shlobj.h>
#include <mmsystem.h>
#include <uxtheme.h>
#include <dwmapi.h>
#pragma comment(lib,"dwmapi.lib")
#include <type_traits>
#include "Core.hpp"
#include "GuiModels.hpp"
#include "StreamProgress.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
namespace udm {
inline bool uiDark=false;
inline COLORREF uiBackground(){return uiDark?RGB(32,34,38):GetSysColor(COLOR_WINDOW);}
inline COLORREF uiForeground(){return uiDark?RGB(232,234,238):GetSysColor(COLOR_WINDOWTEXT);}
inline void themeFrame(HWND window){BOOL dark=uiDark;DwmSetWindowAttribute(window,DWMWA_USE_IMMERSIVE_DARK_MODE,&dark,sizeof(dark));}
class ActionButton:public CButton {
public:
 bool primary=false;
 void DrawItem(LPDRAWITEMSTRUCT item)override{CDC dc;dc.Attach(item->hDC);CRect r=item->rcItem;bool pressed=(item->itemState&ODS_SELECTED)!=0,disabled=(item->itemState&ODS_DISABLED)!=0;dc.FillSolidRect(r,uiDark?(pressed?RGB(72,76,83):RGB(49,52,58)):GetSysColor(COLOR_BTNFACE));if(uiDark)dc.Draw3dRect(r,primary?RGB(99,169,224):RGB(100,104,110),primary?RGB(99,169,224):RGB(72,76,82));else dc.DrawFrameControl(r,DFC_BUTTON,DFCS_BUTTONPUSH|(pressed?DFCS_PUSHED:0)|(disabled?DFCS_INACTIVE:0));CString value;GetWindowText(value);dc.SetBkMode(TRANSPARENT);dc.SetTextColor(disabled?(uiDark?RGB(148,151,157):GetSysColor(COLOR_GRAYTEXT)):uiForeground());auto old=dc.SelectObject(GetFont());dc.DrawText(value,r,DT_CENTER|DT_VCENTER|DT_SINGLELINE);dc.SelectObject(old);if(item->itemState&ODS_FOCUS){r.DeflateRect(3,3);dc.DrawFocusRect(r);}dc.Detach();}
};
inline CBrush& dialogBrush(){static CBrush light(GetSysColor(COLOR_BTNFACE));static CBrush dark(RGB(32,34,38));return uiDark?dark:light;}
inline CBrush& uiBrush(){static CBrush dark(RGB(32,34,38));static CBrush light(GetSysColor(COLOR_WINDOW));return uiDark?dark:light;}
inline void openWith(CWnd* owner,const fs::path& path){if(!fs::is_regular_file(path))throw std::runtime_error("The saved file is missing.");auto normalized=fs::absolute(path).lexically_normal().make_preferred();OPENASINFO info{normalized.c_str(),nullptr,OAIF_EXEC};auto filter=AfxOleGetMessageFilter();if(filter){filter->EnableBusyDialog(FALSE);filter->EnableNotRespondingDialog(FALSE);}auto result=SHOpenWithDialog(owner->GetSafeHwnd(),&info);if(filter){filter->EnableBusyDialog(TRUE);filter->EnableNotRespondingDialog(TRUE);}if(FAILED(result)&&result!=HRESULT_FROM_WIN32(ERROR_CANCELLED))throw std::runtime_error("Windows could not open the app chooser (HRESULT "+std::to_string((unsigned long)result)+").");}
inline CString cs(const std::string& s){return CString(wide(s).c_str());}
inline std::string text(CWnd* w){CString s;w->GetWindowText(s);return utf8((LPCWSTR)s);}
inline void error(CWnd* owner,const std::exception& e){owner->MessageBox(cs(e.what()),L"UDM",MB_OK|MB_ICONWARNING);}
inline void openFile(CWnd* owner,const fs::path& path){auto result=ShellExecuteW(owner->GetSafeHwnd(),L"open",path.c_str(),nullptr,nullptr,SW_SHOWNORMAL);if((INT_PTR)result<=32)throw std::runtime_error("Windows could not open this file.");}
inline std::wstring chooseFolder(CWnd* owner,const std::wstring& initial){CFolderPickerDialog dialog(initial.c_str(),OFN_PATHMUSTEXIST,owner);return dialog.DoModal()==IDOK?std::wstring(dialog.GetPathName()):L"";}
#include "ThemeControls.hpp"
class Form:public CDialog {
 DECLARE_MESSAGE_MAP()
 std::string caption;int width,height;
protected:
 afx_msg HBRUSH OnCtlColor(CDC* dc,CWnd* wnd,UINT type){auto brush=CDialog::OnCtlColor(dc,wnd,type);if(type==CTLCOLOR_STATIC||type==CTLCOLOR_BTN||type==CTLCOLOR_EDIT||type==CTLCOLOR_LISTBOX){dc->SetTextColor(uiForeground());bool surface=type==CTLCOLOR_STATIC||type==CTLCOLOR_BTN;dc->SetBkColor(surface&&!uiDark?GetSysColor(COLOR_BTNFACE):uiBackground());return (HBRUSH)(surface?dialogBrush():uiBrush()).GetSafeHandle();}return brush;}
 afx_msg BOOL OnEraseBkgnd(CDC* dc){CRect area;GetClientRect(&area);dc->FillSolidRect(area,uiDark?uiBackground():GetSysColor(COLOR_BTNFACE));return TRUE;}
 afx_msg void OnTimer(UINT_PTR id){if(pulse)try{pulse();}catch(...){}CDialog::OnTimer(id);}
 UINT nextId=1000;std::vector<std::unique_ptr<CWnd>> controls;std::map<UINT,std::function<void()>> actions;
 CFont font;float scale=1;
 BOOL OnInitDialog()override{CDialog::OnInitDialog();scale=GetDpiForWindow(m_hWnd)/96.0f;font.CreateFontW(-(int)(11*scale),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,L"Tahoma");SetFont(&font);SetWindowText(cs(caption));themeFrame(m_hWnd);CRect r(0,0,(int)(width*scale),(int)(height*scale));AdjustWindowRectEx(&r,GetStyle(),FALSE,GetExStyle());SetWindowPos(nullptr,0,0,r.Width(),r.Height(),SWP_NOMOVE|SWP_NOZORDER);SetIcon(AfxGetApp()->LoadIcon(1),TRUE);CenterWindow();keepOnScreen();if(init)init();if(pulse)SetTimer(7,250,nullptr);return TRUE;}
 BOOL OnCommand(WPARAM w,LPARAM l)override{auto it=actions.find(LOWORD(w));if(it!=actions.end()&&HIWORD(w)==BN_CLICKED){try{it->second();}catch(const std::exception& e){error(this,e);}return TRUE;}return CDialog::OnCommand(w,l);}
 void OnOK()override{if(accept){try{accept();}catch(const std::exception& e){error(this,e);}}}
 void OnCancel()override{if(cancel){try{cancel();}catch(const std::exception& e){error(this,e);}}else if(modeless)DestroyWindow();else CDialog::OnCancel();}
public:
 bool modeless=false;std::function<void()> init,accept,cancel,pulse;
 Form(std::string title,int w,int h,CWnd* parent=nullptr):CDialog(100,parent),caption(std::move(title)),width(w),height(h){}
 void keepOnScreen(){CRect r;GetWindowRect(&r);MONITORINFO monitor{sizeof(monitor)};if(!GetMonitorInfoW(MonitorFromWindow(m_hWnd,MONITOR_DEFAULTTONEAREST),&monitor))return;const auto& work=monitor.rcWork;int x=std::max<int>(work.left,std::min<int>(r.left,work.right-r.Width())),y=std::max<int>(work.top,std::min<int>(r.top,work.bottom-r.Height()));SetWindowPos(nullptr,x,y,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);}
 void resizeClient(int w,int h){CRect r=rect(0,0,w,h);AdjustWindowRectEx(&r,GetStyle(),FALSE,GetExStyle());SetWindowPos(nullptr,0,0,r.Width(),r.Height(),SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);keepOnScreen();}
 void defaultButton(CWnd* control){if(auto button=dynamic_cast<ActionButton*>(control)){button->primary=true;button->Invalidate();}}
 CRect rect(int x,int y,int w,int h){return CRect((int)(x*scale),(int)(y*scale),(int)((x+w)*scale),(int)((y+h)*scale));}
 template<class T>T* make(DWORD style,int x,int y,int w,int h,UINT id=0){auto control=std::make_unique<T>();if(!control->Create(style|WS_CHILD|WS_VISIBLE,rect(x,y,w,h),this,id?id:nextId++))throw std::runtime_error("Cannot create interface control.");control->SetFont(&font);if constexpr(std::is_same_v<T,CTabCtrl>)SetWindowTheme(control->GetSafeHwnd(),L"",L"");auto p=control.get();controls.push_back(std::move(control));return p;}
 CWnd* control(const wchar_t* type,std::string value,DWORD style,int x,int y,int w,int h,DWORD ex=0){auto c=std::make_unique<CWnd>();if(!c->CreateEx(ex,type,cs(value),style|WS_CHILD|WS_VISIBLE,rect(x,y,w,h),this,nextId++))throw std::runtime_error("Cannot create interface control.");c->SetFont(&font);auto p=c.get();controls.push_back(std::move(c));return p;}
 CWnd* label(std::string s,int x,int y,int w,int h=18){return control(L"STATIC",s,SS_LEFT,x,y,w,h);}
 CWnd* edit(std::string s,int x,int y,int w,int h=23,bool readonly=false,bool multi=false,bool secret=false){auto c=control(L"EDIT",s,WS_TABSTOP|ES_AUTOHSCROLL|(readonly?ES_READONLY:0)|(multi?ES_MULTILINE|ES_AUTOVSCROLL|WS_VSCROLL:0)|(secret?ES_PASSWORD:0),x,y,w,h,WS_EX_CLIENTEDGE);c->SendMessage(EM_SETLIMITTEXT,multi?1024*1024:32768);return c;}
 CWnd* check(std::string s,bool value,int x,int y,int w){auto c=control(L"BUTTON",s,WS_TABSTOP|BS_AUTOCHECKBOX,x,y,w,21);if(uiDark)SetWindowTheme(c->GetSafeHwnd(),L"",L"");c->SendMessage(BM_SETCHECK,value?BST_CHECKED:BST_UNCHECKED);return c;}
 CWnd* button(std::string s,int x,int y,int w,std::function<void()> action){auto c=std::make_unique<ActionButton>();if(!c->Create(cs(s),WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,rect(x,y,w,25),this,nextId++))throw std::runtime_error("Cannot create interface button.");c->SetFont(&font);auto p=c.get();actions[p->GetDlgCtrlID()]=std::move(action);controls.push_back(std::move(c));return p;}
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
#include "DragUi.hpp"
#include "QueueDragUi.hpp"
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
#include "DownloadDetailsUi.hpp"
#include "PropertiesUi.hpp"
inline void completedProperties(CWnd* parent,Manager& manager,JobPtr job){fileProperties(parent,manager,job);}
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
#include "DownloadInfoUi.hpp"

inline void presentDownload(CWnd* owner,Manager& manager,JobPtr job){
 job=chooseDuplicate(owner,manager,job);if(!job)return;
 std::string status;bool active;{Lock lock(manager.mutex);status=str(job->data,"Status");active=manager.isActive(job);}
 if(status=="Complete")completedProperties(owner,manager,job);
 else if(active||status=="Queued"){if(manager.event)manager.event(job,false);}
 else downloadInfo(owner,manager,job);
}
inline void addAddress(CWnd* parent,Manager& m,const std::string& initial=""){Form d("Enter new address to download",523,108,parent);JobPtr job;d.init=[&]{d.label("Address",10,13,49);std::vector<std::string> history;{Lock lock(m.mutex);for(auto it=m.jobs.rbegin();it!=m.jobs.rend()&&history.size()<25;++it){auto value=str((*it)->data,"Url");if(std::find(history.begin(),history.end(),value)==history.end())history.push_back(value);}}auto address=d.combo(history,initial,63,9,367,true);address->SetWindowText(cs(initial));auto auth=d.check("Use authorization",false,10,43,153);d.label("Login",10,78,45);auto user=d.edit("",63,74,156);d.label("Password",233,78,62);auto password=d.edit("",300,74,130,23,false,false,true);user->EnableWindow(FALSE);password->EnableWindow(FALSE);d.button("OK",441,9,72,[&,address,auth,user,password]{Url u(text(address));if(hostIs(u.host,"youtube.com")||u.host=="youtu.be")throw std::runtime_error("Open this video in the browser and choose its quality using the UDM panel.");Headers h;if(d.checked(auth)){setBasicLogin(h,text(user),text(password));}job=m.offerDownload(text(address),"","","Main queue",true,h);d.close();});d.button("Cancel",441,43,72,[&]{d.close(IDCANCEL);});d.accept=[&,address,auth,user,password]{Url u(text(address));if(hostIs(u.host,"youtube.com")||u.host=="youtu.be")throw std::runtime_error("Use the UDM browser panel for video capture.");Headers h;if(d.checked(auth)){setBasicLogin(h,text(user),text(password));}job=m.offerDownload(text(address),"","","Main queue",true,h);d.close();};d.bind(auth,[&d,auth,user,password]{user->EnableWindow(d.checked(auth));password->EnableWindow(d.checked(auth));});
 address->SetFocus();};if(d.DoModal()==IDOK&&job)presentDownload(parent,m,job);}
#include "SelectionUi.hpp"
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
#include "ProgressUi.hpp"
}
