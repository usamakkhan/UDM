#include "ProgressPresentation.hpp"
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
#include "CaptureExclusions.hpp"
#include "GuiModels.hpp"
#include "ServerConnections.hpp"
#include "SiteLogins.hpp"
#include "DownloadPreview.hpp"
#include "ZipPreview.hpp"
#include "BrowserSession.hpp"
#include "Scanner.hpp"
#include "StreamProgress.hpp"
#include "DialogGeometry.hpp"
#include "CompletionPolicy.hpp"
#include "OptionsModel.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
namespace udm {
inline bool uiDark=false;
inline bool useDarkUi(bool requested,bool highContrast){return requested&&!highContrast;}
inline bool windowsHighContrast(){HIGHCONTRASTW contrast{sizeof(contrast)};return SystemParametersInfoW(SPI_GETHIGHCONTRAST,sizeof(contrast),&contrast,0)&&(contrast.dwFlags&HCF_HIGHCONTRASTON);}

inline COLORREF uiBackground(){return uiDark?RGB(32,34,38):GetSysColor(COLOR_WINDOW);}
inline COLORREF uiForeground(){return uiDark?RGB(232,234,238):GetSysColor(COLOR_WINDOWTEXT);}
inline void themeFrame(HWND window){BOOL dark=uiDark;DwmSetWindowAttribute(window,DWMWA_USE_IMMERSIVE_DARK_MODE,&dark,sizeof(dark));}
class ActionButton:public CButton {
public:
 bool primary=false,hyperlink=false;std::wstring glyph;
 void setGlyph(const wchar_t* value){glyph=value;ModifyStyle(BS_TYPEMASK,BS_OWNERDRAW);Invalidate();}
 void DrawItem(LPDRAWITEMSTRUCT item)override{CDC dc;dc.Attach(item->hDC);CRect r=item->rcItem;bool pressed=(item->itemState&ODS_SELECTED)!=0,disabled=(item->itemState&ODS_DISABLED)!=0;if(hyperlink){dc.FillSolidRect(r,uiDark?uiBackground():GetSysColor(COLOR_BTNFACE));CString value;GetWindowText(value);if(!glyph.empty())value=glyph.c_str();dc.SetBkMode(TRANSPARENT);dc.SetTextColor(disabled?GetSysColor(COLOR_GRAYTEXT):uiDark?RGB(125,186,240):GetSysColor(COLOR_HOTLIGHT));auto old=dc.SelectObject(GetFont());dc.DrawText(value,r,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);dc.SelectObject(old);if(item->itemState&ODS_FOCUS)dc.DrawFocusRect(r);dc.Detach();return;}dc.FillSolidRect(r,uiDark?(pressed?RGB(72,76,83):RGB(49,52,58)):GetSysColor(COLOR_BTNFACE));if(uiDark)dc.Draw3dRect(r,primary?RGB(99,169,224):RGB(100,104,110),primary?RGB(99,169,224):RGB(72,76,82));else dc.DrawFrameControl(r,DFC_BUTTON,DFCS_BUTTONPUSH|(pressed?DFCS_PUSHED:0)|(disabled?DFCS_INACTIVE:0));CString value;GetWindowText(value);if(!glyph.empty())value=glyph.c_str();dc.SetBkMode(TRANSPARENT);dc.SetTextColor(disabled?(uiDark?RGB(148,151,157):GetSysColor(COLOR_GRAYTEXT)):(uiDark?uiForeground():GetSysColor(COLOR_BTNTEXT)));auto old=dc.SelectObject(GetFont());dc.DrawText(value,r,DT_CENTER|DT_VCENTER|DT_SINGLELINE);dc.SelectObject(old);if(item->itemState&ODS_FOCUS){r.DeflateRect(3,3);dc.DrawFocusRect(r);}dc.Detach();}
};
inline CBrush& dialogBrush(){static CBrush dark(RGB(32,34,38));return uiDark?dark:*CBrush::FromHandle(GetSysColorBrush(COLOR_BTNFACE));}
inline CBrush& uiBrush(){static CBrush dark(RGB(32,34,38));return uiDark?dark:*CBrush::FromHandle(GetSysColorBrush(COLOR_WINDOW));}
inline void openWith(CWnd* owner,const fs::path& path){if(!fs::is_regular_file(path))throw std::runtime_error("The saved file is missing.");auto normalized=fs::absolute(path).lexically_normal().make_preferred();OPENASINFO info{normalized.c_str(),nullptr,OAIF_EXEC};auto filter=AfxOleGetMessageFilter();if(filter){filter->EnableBusyDialog(FALSE);filter->EnableNotRespondingDialog(FALSE);}auto result=SHOpenWithDialog(owner->GetSafeHwnd(),&info);if(filter){filter->EnableBusyDialog(TRUE);filter->EnableNotRespondingDialog(TRUE);}if(FAILED(result)&&result!=HRESULT_FROM_WIN32(ERROR_CANCELLED))throw std::runtime_error("Windows could not open the app chooser (HRESULT "+std::to_string((unsigned long)result)+").");}
inline CString cs(const std::string& s){return CString(wide(s).c_str());}
inline std::string text(CWnd* w){CString s;w->GetWindowText(s);return utf8((LPCWSTR)s);}
inline void error(CWnd* owner,const std::exception& e){owner->MessageBox(cs(e.what()),L"UDM",MB_OK|MB_ICONWARNING);}
inline void openFile(CWnd* owner,const fs::path& path){auto result=ShellExecuteW(owner->GetSafeHwnd(),L"open",path.c_str(),nullptr,nullptr,SW_SHOWNORMAL);if((INT_PTR)result<=32)throw std::runtime_error("Windows could not open this file.");}
inline bool openDownloadedFile(CWnd* owner,const fs::path& path){std::error_code error;if(!fs::exists(path,error)&&!error){owner->MessageBox(cs("The\r\n"+utf8(path.wstring())+" file has been moved."),L"Opening downloaded file",MB_OK|MB_ICONWARNING);return false;}openFile(owner,path);return true;}
inline std::wstring chooseFolder(CWnd* owner,const std::wstring& initial){CFolderPickerDialog dialog(initial.c_str(),OFN_PATHMUSTEXIST,owner);return dialog.DoModal()==IDOK?std::wstring(dialog.GetPathName()):L"";}
#include "ThemeControls.hpp"
class Form:public CDialog {
 DECLARE_MESSAGE_MAP()
 std::string caption;int width,height;DialogUnits units;std::map<HWND,CRect> logicalRects;
 struct ListColumnLayout {std::vector<double> logical;std::vector<int> rendered;};
 std::map<HWND,ListColumnLayout> listColumnLayouts;
 void rememberListColumns(CListCtrl& list,double metric){
  auto header=list.GetHeaderCtrl();if(!header||metric<=0)return;const int count=header->GetItemCount();if(count<0)return;
  auto& layout=listColumnLayouts[list.GetSafeHwnd()];if(layout.logical.size()!=(size_t)count){layout.logical.assign(count,0);layout.rendered.assign(count,-1);}
  for(int i=0;i<count;++i){const int actualWidth=list.GetColumnWidth(i);if(actualWidth>=0&&actualWidth!=layout.rendered[i])layout.logical[i]=actualWidth/metric;}
 }
 void scaleListColumns(CListCtrl& list,double metric){
  auto found=listColumnLayouts.find(list.GetSafeHwnd());if(found==listColumnLayouts.end())return;auto& layout=found->second;
  for(size_t i=0;i<layout.logical.size();++i){list.SetColumnWidth((int)i,(int)std::lround(layout.logical[i]*metric));layout.rendered[i]=list.GetColumnWidth((int)i);}
 }

protected:
 afx_msg HBRUSH OnCtlColor(CDC* dc,CWnd* wnd,UINT type){auto brush=CDialog::OnCtlColor(dc,wnd,type);if(type==CTLCOLOR_STATIC||type==CTLCOLOR_BTN||type==CTLCOLOR_EDIT||type==CTLCOLOR_LISTBOX){bool surface=type==CTLCOLOR_STATIC||type==CTLCOLOR_BTN;dc->SetTextColor(surface&&!uiDark?GetSysColor(COLOR_BTNTEXT):uiForeground());dc->SetBkColor(surface&&!uiDark?GetSysColor(COLOR_BTNFACE):uiBackground());return (HBRUSH)(surface?dialogBrush():uiBrush()).GetSafeHandle();}return brush;}
 afx_msg BOOL OnEraseBkgnd(CDC* dc){CRect area;GetClientRect(&area);dc->FillSolidRect(area,uiDark?uiBackground():GetSysColor(COLOR_BTNFACE));return TRUE;}
 afx_msg void OnTimer(UINT_PTR id){if(pulse)try{pulse();}catch(...){}CDialog::OnTimer(id);}
 UINT nextId=1000;std::vector<std::unique_ptr<CWnd>> controls;std::map<UINT,std::function<void()>> actions;
 CFont font;float scale=1;
 void measureFont(UINT dpi){scale=dpi/96.0f;font.DeleteObject();font.CreateFontW(dialogUnits?-MulDiv(8,dpi,72):-(int)(11*scale),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,L"Tahoma");SetFont(&font);CClientDC dc(this);units=DialogUnits::measure(dc.m_hDC,(HFONT)font.GetSafeHandle());}
 afx_msg LRESULT OnDialogDpiChanged(WPARAM dpi,LPARAM position){
  if(!HIWORD(dpi)||!position)return 0;
  // Preserve logical widths; rescaling rounded pixels repeatedly accumulates drift.
  const double oldMetric=dialogUnits?units.x:scale;
  for(auto& item:controls)if(auto list=dynamic_cast<CListCtrl*>(item.get()))if(list->GetSafeHwnd())rememberListColumns(*list,oldMetric);
  measureFont(HIWORD(dpi));auto suggested=reinterpret_cast<RECT*>(position);SetWindowPos(nullptr,suggested->left,suggested->top,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
  for(auto& item:controls){auto wnd=item.get();if(!wnd->GetSafeHwnd())continue;wnd->SetFont(&font);auto found=logicalRects.find(wnd->GetSafeHwnd());if(found!=logicalRects.end()){auto r=found->second;wnd->MoveWindow(rect(r.left,r.top,r.Width(),r.Height()));}
   if(auto list=dynamic_cast<CListCtrl*>(wnd))scaleListColumns(*list,dialogUnits?units.x:scale);
  }
  resizeClient(width,height);Invalidate(TRUE);return 0;
 }
 BOOL OnInitDialog()override{CDialog::OnInitDialog();SetDialogDpiChangeBehavior(m_hWnd,DDC_DISABLE_ALL,DDC_DISABLE_ALL);measureFont(GetDpiForWindow(m_hWnd));SetWindowText(cs(caption));themeFrame(m_hWnd);CRect r=rect(0,0,width,height);AdjustWindowRectExForDpi(&r,GetStyle(),FALSE,GetExStyle(),(UINT)std::lround(scale*96));SetWindowPos(nullptr,0,0,r.Width(),r.Height(),SWP_NOMOVE|SWP_NOZORDER);SetIcon(AfxGetApp()->LoadIcon(1),TRUE);CenterWindow();keepOnScreen();if(init)init();if(pulse)SetTimer(7,250,nullptr);return TRUE;}
 BOOL OnCommand(WPARAM w,LPARAM l)override{auto it=actions.find(LOWORD(w));if(it!=actions.end()&&HIWORD(w)==BN_CLICKED){try{it->second();}catch(const std::exception& e){error(this,e);}return TRUE;}auto changed=changes.find(LOWORD(w));if(changed!=changes.end()&&(HIWORD(w)==EN_CHANGE||HIWORD(w)==CBN_SELCHANGE||HIWORD(w)==CBN_EDITCHANGE)){try{changed->second();}catch(const std::exception& e){error(this,e);}return TRUE;}return CDialog::OnCommand(w,l);}
 void OnOK()override{if(accept){try{accept();}catch(const std::exception& e){error(this,e);}}}
 void OnCancel()override{if(cancel){try{cancel();}catch(const std::exception& e){error(this,e);}}else if(modeless)DestroyWindow();else CDialog::OnCancel();}
public:
 BOOL PreTranslateMessage(MSG* message)override{
  if(message->message==WM_KEYDOWN&&(message->wParam==VK_TAB||message->wParam==VK_PRIOR||message->wParam==VK_NEXT)&&(GetKeyState(VK_CONTROL)&0x8000)&&!(GetKeyState(VK_MENU)&0x8000)&&(message->hwnd==m_hWnd||::IsChild(m_hWnd,message->hwnd))){
   for(auto& control:controls)if(auto tabs=dynamic_cast<CTabCtrl*>(control.get());tabs&&tabs->IsWindowVisible()&&tabs->IsWindowEnabled()&&tabs->GetItemCount()>1){
    const int count=tabs->GetItemCount(),current=tabs->GetCurSel();if(current<0)continue;const bool backwards=message->wParam==VK_PRIOR||(message->wParam==VK_TAB&&(GetKeyState(VK_SHIFT)&0x8000));
    NMHDR notice{tabs->GetSafeHwnd(),(UINT_PTR)tabs->GetDlgCtrlID(),TCN_SELCHANGING};if(SendMessage(WM_NOTIFY,notice.idFrom,(LPARAM)&notice))return TRUE;
    const auto focused=::GetFocus();tabs->SetCurSel((current+(backwards?count-1:1))%count);notice.code=TCN_SELCHANGE;SendMessage(WM_NOTIFY,notice.idFrom,(LPARAM)&notice);
    if(!focused||!::IsWindowVisible(focused)||!::IsWindowEnabled(focused))tabs->SetFocus();return TRUE;
   }
  }
  return CDialog::PreTranslateMessage(message);
 }
 void refreshTheme(){
  if(!GetSafeHwnd())return;themeFrame(m_hWnd);
  for(auto& control:controls){auto window=control->GetSafeHwnd();if(!window)continue;
   if(auto button=dynamic_cast<ActionButton*>(control.get()))button->ModifyStyle(BS_TYPEMASK,uiDark||button->hyperlink||!button->glyph.empty()?BS_OWNERDRAW:button->primary?BS_DEFPUSHBUTTON:BS_PUSHBUTTON);
   else{wchar_t kind[32]{};GetClassNameW(window,kind,32);if(!_wcsicmp(kind,L"BUTTON"))SetWindowTheme(window,uiDark?L"":nullptr,uiDark?L"":nullptr);}
   if(auto tree=dynamic_cast<CTreeCtrl*>(control.get())){tree->SetBkColor(uiBackground());tree->SetTextColor(uiForeground());}
   if(auto list=dynamic_cast<CListCtrl*>(control.get())){list->SetBkColor(uiBackground());list->SetTextBkColor(uiBackground());list->SetTextColor(uiForeground());if(auto themed=dynamic_cast<ThemeList*>(list))themed->theme();}
  }
  RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_ERASE);
 }
 bool modeless=false,dialogUnits=false;std::map<UINT,std::function<void()>> changes;std::function<void()> init,accept,cancel,pulse;
 Form(std::string title,int w,int h,CWnd* parent=nullptr):CDialog(100,parent),caption(std::move(title)),width(w),height(h){}
 void keepOnScreen(){CRect r;GetWindowRect(&r);MONITORINFO monitor{sizeof(monitor)};if(!GetMonitorInfoW(MonitorFromWindow(m_hWnd,MONITOR_DEFAULTTONEAREST),&monitor))return;const auto& work=monitor.rcWork;int x=std::max<int>(work.left,std::min<int>(r.left,work.right-r.Width())),y=std::max<int>(work.top,std::min<int>(r.top,work.bottom-r.Height()));SetWindowPos(nullptr,x,y,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);}
 void resizeClient(int w,int h){width=w;height=h;CRect r=rect(0,0,w,h);AdjustWindowRectExForDpi(&r,GetStyle(),FALSE,GetExStyle(),(UINT)std::lround(scale*96));SetWindowPos(nullptr,0,0,r.Width(),r.Height(),SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);keepOnScreen();}
 void placeControl(CWnd* control,int x,int y,int w,int h){if(!control||!control->GetSafeHwnd())return;logicalRects[control->GetSafeHwnd()]=CRect(x,y,x+w,y+h);control->MoveWindow(rect(x,y,w,h));}
 void defaultButton(CWnd* control){if(auto button=dynamic_cast<ActionButton*>(control)){button->primary=true;if(!uiDark&&!button->hyperlink)button->ModifyStyle(BS_TYPEMASK,BS_DEFPUSHBUTTON);button->Invalidate();}}
 int pixelsX(int value)const{return dialogUnits?units.px(value):(int)(value*scale);}
 CRect rect(int x,int y,int w,int h){if(dialogUnits)return CRect(units.rect(x,y,w,h));return CRect((int)(x*scale),(int)(y*scale),(int)((x+w)*scale),(int)((y+h)*scale));}
 template<class T>T* make(DWORD style,int x,int y,int w,int h,UINT id=0){auto control=std::make_unique<T>();if(!control->Create(style|WS_CHILD|WS_VISIBLE,rect(x,y,w,h),this,id?id:nextId++))throw std::runtime_error("Cannot create interface control.");control->SetFont(&font);if constexpr(std::is_same_v<T,CTabCtrl>)SetWindowTheme(control->GetSafeHwnd(),L"",L"");auto p=control.get();logicalRects[p->GetSafeHwnd()]=CRect(x,y,x+w,y+h);controls.push_back(std::move(control));return p;}
 CWnd* control(const wchar_t* type,std::string value,DWORD style,int x,int y,int w,int h,DWORD ex=0){auto c=std::make_unique<CWnd>();if(!c->CreateEx(ex,type,cs(value),style|WS_CHILD|WS_VISIBLE,rect(x,y,w,h),this,nextId++))throw std::runtime_error("Cannot create interface control.");if(uiDark&&!_wcsicmp(type,L"BUTTON")&&(style&BS_TYPEMASK)==BS_GROUPBOX)SetWindowTheme(c->GetSafeHwnd(),L"",L"");c->SetFont(&font);auto p=c.get();logicalRects[p->GetSafeHwnd()]=CRect(x,y,x+w,y+h);controls.push_back(std::move(c));return p;}
 CWnd* label(std::string s,int x,int y,int w,int h=-1){return control(L"STATIC",s,SS_LEFT,x,y,w,h<0?(dialogUnits?9:18):h);}
 CWnd* edit(std::string s,int x,int y,int w,int h=-1,bool readonly=false,bool multi=false,bool secret=false){if(h<0)h=dialogUnits?14:23;auto c=control(L"EDIT",s,WS_TABSTOP|ES_AUTOHSCROLL|(readonly?ES_READONLY:0)|(multi?ES_MULTILINE|ES_AUTOVSCROLL|WS_VSCROLL:0)|(secret?ES_PASSWORD:0),x,y,w,h,WS_EX_CLIENTEDGE);c->SendMessage(EM_SETLIMITTEXT,multi?1024*1024:32768);return c;}
 CWnd* check(std::string s,bool value,int x,int y,int w,int h=-1){auto c=control(L"BUTTON",s,WS_TABSTOP|BS_AUTOCHECKBOX|BS_MULTILINE,x,y,w,h<0?(dialogUnits?10:21):h);if(uiDark)SetWindowTheme(c->GetSafeHwnd(),L"",L"");c->SendMessage(BM_SETCHECK,value?BST_CHECKED:BST_UNCHECKED);return c;}
 CWnd* button(std::string s,int x,int y,int w,std::function<void()> action,int h=-1){if(h<0)h=dialogUnits?14:25;auto c=std::make_unique<ActionButton>();if(!c->Create(cs(s),WS_CHILD|WS_VISIBLE|WS_TABSTOP|(uiDark?BS_OWNERDRAW:BS_PUSHBUTTON),rect(x,y,w,h),this,nextId++))throw std::runtime_error("Cannot create interface button.");c->SetFont(&font);auto p=c.get();actions[p->GetDlgCtrlID()]=std::move(action);logicalRects[p->GetSafeHwnd()]=CRect(x,y,x+w,y+h);controls.push_back(std::move(c));return p;}
 CWnd* link(std::string s,int x,int y,int w,std::function<void()> action,int h=-1){auto value=button(s,x,y,w,std::move(action),h<0?(dialogUnits?9:18):h);auto button=static_cast<ActionButton*>(value);button->hyperlink=true;button->ModifyStyle(BS_TYPEMASK,BS_OWNERDRAW);return value;}
 CWnd* staticLink(std::string value,int x,int y,int w,std::function<void()> action,int h=-1){if(h<0)h=dialogUnits?9:18;auto control=std::make_unique<StaticHyperlink>();if(!control->Create(cs(value),WS_CHILD|WS_VISIBLE|WS_TABSTOP|SS_OWNERDRAW|SS_NOTIFY,rect(x,y,w,h),this,nextId++))throw std::runtime_error("Cannot create source-page link");control->SetFont(&font);auto p=control.get();actions[p->GetDlgCtrlID()]=std::move(action);logicalRects[p->GetSafeHwnd()]=CRect(x,y,x+w,y+h);controls.push_back(std::move(control));return p;}
 CWnd* glyphCheck(std::string name,bool value,int x,int y,int w,int h=10){auto c=std::make_unique<GlyphCheckBox>(cs(name));if(!c->Create(L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX,rect(x,y,w,h),this,nextId++))throw std::runtime_error("Cannot create checkbox.");c->SetFont(&font);if(uiDark)SetWindowTheme(c->GetSafeHwnd(),L"",L"");c->SendMessage(BM_SETCHECK,value?BST_CHECKED:BST_UNCHECKED);auto p=c.get();logicalRects[p->GetSafeHwnd()]=CRect(x,y,x+w,y+h);controls.push_back(std::move(c));return p;}
 CComboBox* combo(const std::vector<std::string>& values,std::string current,int x,int y,int w,bool free=false){auto c=make<CComboBox>(WS_TABSTOP|WS_VSCROLL|(free?CBS_DROPDOWN|CBS_AUTOHSCROLL:CBS_DROPDOWNLIST),x,y,w,230);for(const auto& v:values)c->AddString(cs(v));int index=c->FindStringExact(-1,cs(current));if(index>=0)c->SetCurSel(index);else if(free)c->SetWindowText(cs(current));else if(!values.empty())c->SetCurSel(0);return c;}
 bool checked(CWnd* c){return c->SendMessage(BM_GETCHECK)==BST_CHECKED;}
 void bind(CWnd* c,std::function<void()> action){actions[c->GetDlgCtrlID()]=std::move(action);}
 void bindChange(CWnd* c,std::function<void()> action){changes[c->GetDlgCtrlID()]=std::move(action);}
 void close(int code=IDOK){if(modeless)DestroyWindow();else EndDialog(code);}
};
BEGIN_MESSAGE_MAP(Form,CDialog)
 ON_WM_TIMER()
 ON_WM_CTLCOLOR()
 ON_WM_ERASEBKGND()
 ON_MESSAGE(WM_DPICHANGED,OnDialogDpiChanged)
END_MESSAGE_MAP()
inline void refreshOpenForms(){
 EnumThreadWindows(GetCurrentThreadId(),[](HWND window,LPARAM)->BOOL{if(auto form=dynamic_cast<Form*>(CWnd::FromHandlePermanent(window)))form->refreshTheme();return TRUE;},0);
}
#include "DragUi.hpp"
#include "QueueDragUi.hpp"
inline void openRefreshPage(HWND owner,const std::string& value){Url url(trim(value));if(url.scheme!="http"&&url.scheme!="https")throw std::runtime_error("Enter the original HTTP or HTTPS download page.");auto result=ShellExecuteW(owner,L"open",wide(url.full).c_str(),nullptr,nullptr,SW_SHOWNORMAL);if((INT_PTR)result<=32)throw std::runtime_error("Windows could not open the download page. Use the Open page button to try again.");}
class RefreshAddressDialog:public Form {
 DECLARE_MESSAGE_MAP()
 Manager& manager;JobPtr job;CWnd *address=nullptr,*page=nullptr,*status=nullptr;Json candidate,displayed=Json::array();std::string seen;
 Json refreshPresentations()const{Json offers=Json::array();for(const auto& offer:manager.pendingBrowserPresentations())if(str(offer,"id")==job->id()&&str(offer,"kind")=="refresh")offers.push_back(offer);return offers;}
 void poll(){Lock lock(manager.mutex);auto offer=manager.addressRefreshCandidate(job);auto pending=refreshPresentations();if(offer.is_object()&&(offer.dump()!=seen||pending!=displayed)){candidate=offer;seen=offer.dump();displayed=pending;address->SetWindowText(cs(str(offer,"url")));if(!str(offer,"page").empty())page->SetWindowText(cs(str(offer,"page")));status->SetWindowText(L"A matching browser link arrived. Review the address, then choose Save or Resume.");}}
 void apply(bool resume){
  Lock lock(manager.mutex);auto latest=manager.addressRefreshCandidate(job);auto pending=refreshPresentations();
  if(latest!=candidate||pending!=displayed){
   if(latest.is_object())poll();else{candidate=latest;seen=latest.dump();displayed=pending;}
   status->SetWindowText(L"The browser link changed or expired. Review the address, then choose Save or Resume again.");return;
  }
  std::optional<Headers> headers;if(candidate.is_object()&&text(address)==str(candidate,"url"))headers=candidate["headers"].get<Headers>();manager.refreshAddress(job,trim(text(address)),headers,trim(text(page)));if(resume)manager.resume(job);close();
 }
 afx_msg void OnTimer(UINT_PTR id){try{if(id==2){KillTimer(2);if(!trim(text(page)).empty())openRefreshPage(m_hWnd,text(page));}else poll();}catch(const std::exception& e){status->SetWindowText(cs(e.what()));}Form::OnTimer(id);}
public:
 const Json& displayedPresentations()const{return displayed;}
 RefreshAddressDialog(Manager& m,JobPtr j,CWnd* owner,bool reopenPage=true):Form("Refresh download address",610,302,owner),manager(m),job(j){init=[this,reopenPage]{
  Json data;{Lock lock(manager.mutex);data=job->data;}
  label(str(data,"FileName"),14,12,580,23);
  label("Open the download page, then send the same file to UDM from the browser. You can also paste a fresh direct link below.",14,42,580,34);
  label("Download page",14,91,96);page=edit(recoveryPage(data),116,87,359);
  button("Open page",485,86,109,[this]{openRefreshPage(m_hWnd,text(page));});
  label("New address",14,126,96);address=edit("",116,122,478,47,false,true);
  status=label("Waiting for a matching link from the browser (up to 10 minutes).",14,181,580,30);
  label("Saved parts: "+bytes(num(data,"Received"))+". Resume checks the file size and server validator before reusing them.",14,215,580,29);
  button("Save address",219,262,116,[this]{apply(false);});button("Save and resume",345,262,135,[this]{apply(true);});button("Cancel",490,262,104,[this]{close(IDCANCEL);});accept=[this]{apply(true);};poll();SetTimer(1,250,nullptr);if(reopenPage)SetTimer(2,100,nullptr);
 };}
};
BEGIN_MESSAGE_MAP(RefreshAddressDialog,Form)
 ON_WM_TIMER()
END_MESSAGE_MAP()
class RefreshMediaDialog:public Form {
 DECLARE_MESSAGE_MAP()
 Manager& manager;JobPtr job;CWnd* page=nullptr;CWnd* status=nullptr;CWnd* saveButton=nullptr;CWnd* resumeButton=nullptr;std::string launchError;Json candidate,displayed=Json::array();
 Json refreshPresentations()const{Json offers=Json::array();for(const auto& offer:manager.pendingBrowserPresentations())if(str(offer,"id")==job->id()&&str(offer,"kind")=="refresh")offers.push_back(offer);return offers;}
 void poll(){Lock lock(manager.mutex);candidate=manager.addressRefreshCandidate(job);displayed=refreshPresentations();const bool ready=str(candidate,"kind")=="sabr"||str(candidate,"kind")=="adaptive"||str(candidate,"kind")=="direct-media";saveButton->EnableWindow(ready);resumeButton->EnableWindow(ready);status->SetWindowText(cs(ready?"Matching streams received. Review the selection above, then save or resume.":launchError.empty()?"Waiting for the same video, quality and audio track from the browser (10 minutes).":launchError));}
 void apply(bool start){Lock lock(manager.mutex);
  if(manager.addressRefreshCandidate(job)!=candidate||refreshPresentations()!=displayed){poll();status->SetWindowText(L"The captured session changed or expired. Review the selection, then choose Save or Resume again.");return;}
  manager.applyMediaRefresh(job);if(start)manager.resume(job);close();
 }
 afx_msg void OnTimer(UINT_PTR id){try{if(id==2){KillTimer(2);openRefreshPage(m_hWnd,text(page));}else poll();}catch(const std::exception& e){launchError=e.what();status->SetWindowText(cs(launchError));}Form::OnTimer(id);}
public:
 const Json& displayedPresentations()const{return displayed;}
 RefreshMediaDialog(Manager& m,JobPtr j,CWnd* owner,bool reopenPage=true):Form("Refresh media session",574,290,owner),manager(m),job(j){init=[this,reopenPage]{Json data;{Lock lock(manager.mutex);data=job->data;}
  label(str(data,"FileName"),14,12,546,24);
  label("1. Open the original video and let it play.\r\n2. In its UDM panel, select the same quality or audio track.\r\n3. Return here to apply the matching session to this download.",14,43,546,55);
  label("Original page",14,109,97);page=edit(str(data,"SourceUrl",str(data,"Url")),114,105,446,23,true);
  button("Open video page",14,143,142,[this]{openRefreshPage(m_hWnd,text(page));launchError.clear();poll();});
  label(str(data,"FormatDescription")+(!str(data,"ProtectedSabr").empty()?"\r\nRetained segments: "+bytes(num(data,"SabrRetainedBytes"))+". Filename and history are preserved.":"\r\nSaved segments are checked against the fresh capture before reuse."),170,141,390,38);
  status=label("",14,187,546,43);
  saveButton=button("Save session",171,246,117,[this]{apply(false);});resumeButton=button("Save and resume",298,246,140,[this]{apply(true);});button("Cancel",448,246,112,[this]{close(IDCANCEL);});accept=[this]{apply(true);};poll();SetTimer(1,250,nullptr);if(reopenPage)SetTimer(2,100,nullptr);
 };}
};
BEGIN_MESSAGE_MAP(RefreshMediaDialog,Form)
 ON_WM_TIMER()
END_MESSAGE_MAP()
inline void refreshDownloadAddress(CWnd* owner,Manager& manager,JobPtr job,bool reopenPage=true){
 manager.beginAddressRefresh(job,!reopenPage);
 try{
  bool streaming;{Lock lock(manager.mutex);streaming=!str(job->data,"SourceUrl").empty()||!str(job->data,"ProtectedAdaptive").empty();}
  INT_PTR result;Json displayed=Json::array();
  if(streaming){RefreshMediaDialog dialog(manager,job,owner,reopenPage);result=dialog.DoModal();displayed=dialog.displayedPresentations();}
  else{RefreshAddressDialog dialog(manager,job,owner,reopenPage);result=dialog.DoModal();displayed=dialog.displayedPresentations();}
  if(result==-1)throw std::runtime_error("Cannot open the download refresh dialog.");
  manager.cancelAddressRefresh(job);
  // Cancel acknowledges only links actually shown by this dialog. A newer
  // receipt committed after its last poll remains available for presentation.
  for(const auto& offer:displayed)manager.finishBrowserPresentation(offer);
 }catch(...){manager.cancelAddressRefresh(job);throw;}
}
inline std::string prompt(CWnd* parent,std::string title,std::string value=""){Form d(title,380,95,parent);CWnd* input=nullptr;std::string result;d.init=[&]{d.label("Name",10,12,48);input=d.edit(value,60,9,309);d.accept=[&]{result=trim(text(input));if(result.empty()||result.size()>80)throw std::runtime_error("Enter a name up to 80 characters.");d.close();};d.button("OK",205,58,78,d.accept);d.button("Cancel",291,58,78,[&]{d.close(IDCANCEL);});input->SetFocus();};return d.DoModal()==IDOK?result:"";}
inline std::vector<std::string> queueNames(Manager& m){Lock l(m.mutex);std::vector<std::string> names;for(auto q:m.state["Queues"])names.push_back(str(q,"Name"));return names;}
#include "QueueChoiceUi.hpp"
inline void moveCompleted(CWnd* owner,Manager& manager,JobPtr job){
 Form d("Move / Rename",605,151,owner);d.init=[&]{
  d.label("New file location",13,17,116);auto path=d.edit(utf8(job->target().wstring()),132,13,460);
  d.button("Choose folder...",13,53,128,[&d,path]{auto value=chooseFolder(&d,fs::path(wide(text(path))).parent_path().wstring());if(!value.empty())path->SetWindowText(cs(utf8((fs::path(value)/fs::path(wide(text(path))).filename()).wstring())));});
  d.label("Enter a new name or location. Existing files will not be overwritten.",155,57,435,34);
  d.accept=[&,path]{manager.relocate(job,fs::path(wide(trim(text(path)))));d.close();};d.button("Move / Rename",351,110,134,d.accept);d.button("Cancel",496,110,96,[&]{d.close(IDCANCEL);});
 };d.DoModal();
}
#include "DownloadDetailsUi.hpp"
#include "ScannerUi.hpp"
#include "PropertiesUi.hpp"
inline void completedProperties(CWnd* parent,Manager& manager,JobPtr job){fileProperties(parent,manager,job);}
inline JobPtr chooseDuplicate(CWnd* parent,Manager& manager,JobPtr candidate){
 Json original;{Lock lock(manager.mutex);if(str(candidate->data,"DuplicateOf").empty())return candidate;for(auto j:manager.jobs)if(j->id()==str(candidate->data,"DuplicateOf"))original=j->data;}
 Form dialog("Duplicate download link",286,154,parent);dialog.dialogUnits=true;JobPtr result;
 dialog.init=[&]{
  dialog.edit(str(candidate->data,"Url"),5,7,274,14,true);
  dialog.label("This file already exists in your download list. Choose an option below, or Cancel to skip this file.",7,26,272,18);
  auto numbered=dialog.check("Add the duplicate with a numbered file name",false,7,56,272);
  auto replace=dialog.check("Add the duplicate and overwrite the existing file",false,7,73,272);
  auto existing=dialog.check("If existing file is complete, show Download complete; otherwise resume it.",true,7,86,272,18);
  for(auto item:{numbered,replace,existing})item->ModifyStyle(BS_TYPEMASK,BS_AUTORADIOBUTTON);
  replace->EnableWindow((str(original,"Status")=="Complete"||str(original,"Status")=="Paused"||str(original,"Status")=="Failed")&&str(original,"ReplacementOf").empty());
  auto remember=dialog.check("Remember my selection and do not show this dialog again.\r\nChange this later in UDM Options > Downloads.",false,11,132,260,18);
  for(auto selected:{numbered,replace,existing})dialog.bind(selected,[&,selected,numbered,replace,existing,remember]{for(auto item:{numbered,replace,existing})item->SendMessage(BM_SETCHECK,item==selected?BST_CHECKED:BST_UNCHECKED);});
  dialog.accept=[&,numbered,replace,remember]{auto mode=dialog.checked(numbered)?"Numbered":dialog.checked(replace)?"Replace":"Existing";result=manager.resolveDuplicate(candidate,mode,dialog.checked(remember));dialog.close();};
  dialog.defaultButton(dialog.button("OK",84,112,50,dialog.accept));dialog.cancel=[&]{manager.resolveDuplicate(candidate,"Cancel");dialog.close(IDCANCEL);};dialog.button("Cancel",152,112,50,dialog.cancel);
 };if(dialog.DoModal()==-1)throw std::runtime_error("Cannot open the duplicate download dialog.");return result;
}
#include "CaptureExclusionsUi.hpp"
#include "ZipPreviewUi.hpp"
#include "DownloadInfoUi.hpp"

inline void presentDownload(CWnd* owner,Manager& manager,JobPtr job,bool automatic=false){
 auto context=automatic?manager.takeAutomaticCapture(job):Json::object();bool cancelled=false;
 try{
  job=chooseDuplicate(owner,manager,job);
  if(!job)cancelled=true;
  else{auto presentation=manager.presentOffer(job);
   if(presentation==OfferPresentation::Complete){if(manager.showCompletedDownload)manager.showCompletedDownload(job);}
   else if(presentation==OfferPresentation::Progress){if(manager.event)manager.event(job,false);}
   else cancelled=!downloadInfo(owner,manager,job);
  }
 }catch(...){manager.finishAutomaticCapture(context,false);throw;}
 auto offer=manager.finishAutomaticCapture(context,cancelled);if(!offer.empty()){CaptureExclusionOffer dialog(owner,manager,offer);dialog.DoModal();}
}
#include "CaptureReviewUi.hpp"
inline bool presentBrowserCapture(CWnd* owner,Manager& manager,const Json& offer){
 auto job=manager.restoreBrowserPresentation(offer);if(!job)return false;
 if(owner)owner->ShowWindow(SW_SHOW);
 if(str(offer,"kind")=="refresh")refreshDownloadAddress(owner,manager,job,false);
 else presentDownload(owner,manager,job,true);
 // Only the tokens offered to this dialog are acknowledged. A later request
 // must retain its own pending presentation if it arrived during the modal loop.
 manager.finishBrowserPresentation(offer);return true;
}
inline void addAddress(CWnd* parent,Manager& manager,const std::string& initial=""){
 Form d("Enter new address to download",373,57,parent);d.dialogUnits=true;JobPtr job;
 d.init=[&]{
  d.label("Address",8,9,35);std::vector<std::string> history;{Lock lock(manager.mutex);for(auto it=manager.jobs.rbegin();it!=manager.jobs.rend()&&history.size()<25;++it){auto value=str((*it)->data,"Url");if(std::find(history.begin(),history.end(),value)==history.end())history.push_back(value);}}
  auto address=d.combo(history,initial,45,7,264,true);address->SetWindowText(cs(initial));
  d.control(L"BUTTON","",BS_GROUPBOX,7,24,302,29);auto auth=d.check("Use &authorization",false,13,24,70,8);
  d.label("Login",13,36,60);auto user=d.edit("",77,34,78);d.label("Password",160,36,64);auto password=d.edit("",228,34,75,14,false,false,true);
  auto toggle=[&d,auth,user,password]{user->EnableWindow(d.checked(auth));password->EnableWindow(d.checked(auth));};d.bind(auth,toggle);toggle();
  d.accept=[&,address,auth,user,password]{Url url(trim(text(address)));if(hostIs(url.host,"youtube.com")||url.host=="youtu.be")throw std::runtime_error("Open this video in the browser and choose its quality using the UDM panel.");Headers headers;if(d.checked(auth))setBasicLogin(headers,text(user),text(password));job=manager.offerDownload(url.full,"","","Main queue",true,headers);d.close();};
  d.defaultButton(d.button("O&K",316,7,50,d.accept,13));d.button("&Cancel",316,24,50,[&]{d.close(IDCANCEL);});address->SetFocus();
 };if(d.DoModal()==IDOK&&job)presentDownload(parent,manager,job);
}
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
#include "DialogPreferences.hpp"
#include "ProgressUi.hpp"
}
