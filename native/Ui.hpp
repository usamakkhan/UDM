#include "ProgressPresentation.hpp"
#pragma once
#include "SchedulerNumber.hpp"
#include "SchedulerTime.hpp"
#include <limits>
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
#include "QueueLabels.hpp"
#include "QueueRetry.hpp"
#include "CaptureExclusions.hpp"
#include "GuiModels.hpp"
#include "ServerConnections.hpp"
#include "SiteLogins.hpp"
#include "DownloadPreview.hpp"
#include "PreviewFilename.hpp"
#include "AsyncFileIcons.hpp"
#include "ZipPreview.hpp"
#include "BrowserSession.hpp"
#include "Scanner.hpp"
#include "StreamProgress.hpp"
#include "DialogGeometry.hpp"
#include "CompletionPolicy.hpp"
#include "OptionsModel.hpp"
#include "CategoryEditor.hpp"
#include <algorithm>
#include <array>
#include <sstream>
#include <iomanip>
#include "MenuBarTheme.hpp"
namespace udm {
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
inline Json formInitializationTiming=Json::array();
inline std::function<void(const std::string&)> formRefreshPageObserver;
inline std::function<void(const char*)> capturePresentationTrace;
inline void traceCapturePresentation(const char* stage){if(capturePresentationTrace)capturePresentationTrace(stage);}
inline double formClockMs(){LARGE_INTEGER value,frequency;QueryPerformanceCounter(&value);QueryPerformanceFrequency(&frequency);return 1000.0*value.QuadPart/frequency.QuadPart;}
inline void formStage(const std::string& caption,UINT id,const char* stage,double start){if(caption=="Download File Info")formInitializationTiming.push_back({{"id",id},{"stage",stage},{"ms",formClockMs()-start}});}
#endif
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
inline int themedMessage(CWnd* owner,LPCWSTR message,LPCWSTR title,UINT flags=MB_OK);
inline void error(CWnd* owner,const std::exception& e){themedMessage(owner,cs(e.what()),L"UDM",MB_OK|MB_ICONWARNING);}
inline void openFile(CWnd* owner,const fs::path& path){auto result=ShellExecuteW(owner->GetSafeHwnd(),L"open",path.c_str(),nullptr,nullptr,SW_SHOWNORMAL);if((INT_PTR)result<=32)throw std::runtime_error("Windows could not open this file.");}
inline bool completedFileOpenReady(const Json& record,bool busy){
 if(busy||str(record,"Status")!="Complete")return false;
 const auto scan=record.find("ScanResult");
 if(scan==record.end())return true;
 const auto status=str(*scan,"Status");
 return status!="Pending"&&status!="Running";
}
inline bool confirmDownloadedFilePresent(CWnd* owner,const fs::path& path){
 std::error_code error;
 const auto status=fs::status(path,error);
 if(fs::is_regular_file(status))return true;
 if(error&&error!=std::errc::no_such_file_or_directory&&error!=std::errc::not_a_directory)
  throw fs::filesystem_error("Cannot inspect the downloaded file",path,error);
 themedMessage(owner,cs("The\r\n"+utf8(path.wstring())+" file has been moved."),L"Opening downloaded file",MB_OK|MB_ICONWARNING);
 return false;
}
inline bool openDownloadedFile(CWnd* owner,const fs::path& path){
 if(!confirmDownloadedFilePresent(owner,path))return false;
 openFile(owner,path);
 return true;
}
inline std::wstring chooseFolder(CWnd* owner,const std::wstring& initial){CFolderPickerDialog dialog(initial.c_str(),OFN_PATHMUSTEXIST,owner);return dialog.DoModal()==IDOK?std::wstring(dialog.GetPathName()):L"";}
#include "ThemeControls.hpp"
class QueueCombo:public ThemeCombo {
 std::vector<std::string> names;
public:
 void addQueue(const std::string& name,std::string label){
  if(std::find(names.begin(),names.end(),name)!=names.end())return;
  while(FindStringExact(-1,cs(label))>=0)label+=" (custom)";
  const int index=AddString(cs(label));
  if(index<0)throw std::runtime_error("Cannot add queue to the selector.");
  names.insert(names.begin()+index,name);
 }
 bool selectQueue(const std::string& name){
  const auto found=std::find(names.begin(),names.end(),name);
  if(found==names.end())return false;
  SetCurSel(static_cast<int>(found-names.begin()));
  return true;
 }
 std::string queueName(){
  int index=GetCurSel();
  if(index<0){const auto typed=text(this);index=FindStringExact(-1,cs(typed));if(index<0)return typed;}
  return names.at(static_cast<size_t>(index));
 }
};
enum class ResizeAnchor {StretchWidth,StretchBoth,BottomLeft,BottomRight};
class Form:public CDialog {
 DECLARE_MESSAGE_MAP()
 MenuBarTheme popupTheme;
 std::string caption;int width,height;DialogUnits units;std::map<HWND,CRect> logicalRects;
 std::map<HWND,ResizeAnchor> resizeAnchors;
 CRect preferredWindowRect(){
  auto bounds=rect(0,0,width,height);
  AdjustWindowRectExForDpi(&bounds,GetStyle(),::GetMenu(m_hWnd)!=nullptr,GetExStyle(),GetDpiForWindow(m_hWnd));
  return bounds;
 }
 void layoutAnchoredControls(){
  if(resizeAnchors.empty()||IsIconic())return;
  CRect client;GetClientRect(&client);
  const auto baseline=rect(0,0,width,height);
  const int extraWidth=std::max(0,client.Width()-baseline.Width());
  const int extraHeight=std::max(0,client.Height()-baseline.Height());
  for(const auto& [window,anchor]:resizeAnchors){
   const auto& logical=logicalRects.at(window);
   auto bounds=rect(logical.left,logical.top,logical.Width(),logical.Height());
   switch(anchor){
    case ResizeAnchor::StretchWidth:bounds.right+=extraWidth;break;
    case ResizeAnchor::StretchBoth:bounds.right+=extraWidth;bounds.bottom+=extraHeight;break;
    case ResizeAnchor::BottomLeft:bounds.OffsetRect(0,extraHeight);break;
    case ResizeAnchor::BottomRight:bounds.OffsetRect(extraWidth,extraHeight);break;
   }
   ::MoveWindow(window,bounds.left,bounds.top,bounds.Width(),bounds.Height(),FALSE);
  }
  RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_ERASE);
 }
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
 LRESULT WindowProc(UINT message,WPARAM w,LPARAM l)override{
  if(message==WM_NOTIFY&&l&&((NMHDR*)l)->code==DTN_DROPDOWN&&uiDark){auto calendar=(HWND)::SendMessageW(((NMHDR*)l)->hwndFrom,DTM_GETMONTHCAL,0,0);if(calendar){SetWindowTheme(calendar,L"",L"");for(int part:{MCSC_BACKGROUND,MCSC_MONTHBK,MCSC_TITLEBK})::SendMessageW(calendar,MCM_SETCOLOR,part,uiBackground());for(int part:{MCSC_TEXT,MCSC_TITLETEXT})::SendMessageW(calendar,MCM_SETCOLOR,part,uiForeground());::SendMessageW(calendar,MCM_SETCOLOR,MCSC_TRAILINGTEXT,RGB(145,145,145));}}
  if(message==WM_INITMENUPOPUP){auto result=CDialog::WindowProc(message,w,l);popupTheme.popup((HMENU)w,uiDark);return result;}
  if(message==WM_UNINITMENUPOPUP)popupTheme.releasePopup((HMENU)w);
  if(message==WM_MEASUREITEM&&popupTheme.measure((MEASUREITEMSTRUCT*)l))return TRUE;
  if(message==WM_DRAWITEM&&popupTheme.draw((DRAWITEMSTRUCT*)l))return TRUE;
  if(message==WM_MENUCHAR){LRESULT result=0;if(popupTheme.mnemonic((HMENU)l,LOWORD(w),result))return result;}
  return CDialog::WindowProc(message,w,l);
 }
 afx_msg HBRUSH OnCtlColor(CDC* dc,CWnd* wnd,UINT type){auto brush=CDialog::OnCtlColor(dc,wnd,type);if(type==CTLCOLOR_STATIC||type==CTLCOLOR_BTN||type==CTLCOLOR_EDIT||type==CTLCOLOR_LISTBOX){bool surface=type==CTLCOLOR_STATIC||type==CTLCOLOR_BTN;dc->SetTextColor(surface&&!uiDark?GetSysColor(COLOR_BTNTEXT):uiForeground());dc->SetBkColor(surface&&!uiDark?GetSysColor(COLOR_BTNFACE):uiBackground());return (HBRUSH)(surface?dialogBrush():uiBrush()).GetSafeHandle();}return brush;}
 afx_msg BOOL OnEraseBkgnd(CDC* dc){CRect area;GetClientRect(&area);dc->FillSolidRect(area,uiDark?uiBackground():GetSysColor(COLOR_BTNFACE));return TRUE;}
 afx_msg void OnTimer(UINT_PTR id){if(pulse)try{pulse();}catch(...){}CDialog::OnTimer(id);}
 afx_msg void OnSize(UINT type,int cx,int cy){
  CDialog::OnSize(type,cx,cy);
  if(type!=SIZE_MINIMIZED)layoutAnchoredControls();
 }
 afx_msg void OnGetMinMaxInfo(MINMAXINFO* info){
  CDialog::OnGetMinMaxInfo(info);
  if(!resizable||!font.GetSafeHandle())return;
  const auto minimum=preferredWindowRect();
  info->ptMinTrackSize.x=std::max<LONG>(info->ptMinTrackSize.x,minimum.Width());
  info->ptMinTrackSize.y=std::max<LONG>(info->ptMinTrackSize.y,minimum.Height());
 }
 UINT nextId=1000;std::vector<std::unique_ptr<CWnd>> controls;std::map<UINT,std::function<void()>> actions;
 CFont font;float scale=1;
 void measureFont(UINT dpi){scale=dpi/96.0f;font.DeleteObject();font.CreateFontW(dialogUnits?-MulDiv(8,dpi,72):-(int)(11*scale),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,L"Tahoma");SetFont(&font);CClientDC dc(this);units=DialogUnits::measure(dc.m_hDC,(HFONT)font.GetSafeHandle());}
 afx_msg LRESULT OnDialogDpiChanged(WPARAM dpi,LPARAM position){
  if(!HIWORD(dpi)||!position)return 0;
  // Preserve logical widths; rescaling rounded pixels repeatedly accumulates drift.
  const double oldMetric=dialogUnits?units.x:scale;
  for(auto& item:controls)if(auto list=dynamic_cast<CListCtrl*>(item.get()))if(list->GetSafeHwnd())rememberListColumns(*list,oldMetric);
  measureFont(HIWORD(dpi));auto suggested=reinterpret_cast<RECT*>(position);
  if(!resizable)SetWindowPos(nullptr,suggested->left,suggested->top,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
  for(auto& item:controls){auto wnd=item.get();if(!wnd->GetSafeHwnd())continue;wnd->SetFont(&font);auto found=logicalRects.find(wnd->GetSafeHwnd());if(found!=logicalRects.end()&&!resizeAnchors.count(wnd->GetSafeHwnd())){auto r=found->second;wnd->MoveWindow(rect(r.left,r.top,r.Width(),r.Height()));}
   if(auto list=dynamic_cast<CListCtrl*>(wnd))scaleListColumns(*list,dialogUnits?units.x:scale);
  }
  if(resizable){
   CRect bounds(*suggested);
   if(!IsZoomed()&&!IsIconic()){
    const auto minimum=preferredWindowRect();
    bounds.right=bounds.left+std::max(bounds.Width(),minimum.Width());
    bounds.bottom=bounds.top+std::max(bounds.Height(),minimum.Height());
   }
   SetWindowPos(nullptr,bounds.left,bounds.top,bounds.Width(),bounds.Height(),SWP_NOZORDER|SWP_NOACTIVATE);
   layoutAnchoredControls();
  }else resizeClient(width,height);
  popupTheme.apply(m_hWnd,nullptr,uiDark,HIWORD(dpi));Invalidate(TRUE);return 0;
 }
 BOOL OnInitDialog()override{
  CDialog::OnInitDialog();
  SetDialogDpiChangeBehavior(m_hWnd,DDC_DISABLE_ALL,DDC_DISABLE_ALL);
  measureFont(GetDpiForWindow(m_hWnd));
  SetWindowText(cs(caption));
  if(resizable){
   ModifyStyleEx(WS_EX_DLGMODALFRAME,0);
   ModifyStyle(DS_MODALFRAME,WS_THICKFRAME|WS_MINIMIZEBOX|WS_MAXIMIZEBOX,SWP_FRAMECHANGED);
  }
  themeFrame(m_hWnd);
  const auto r=preferredWindowRect();
  SetWindowPos(nullptr,0,0,r.Width(),r.Height(),SWP_NOMOVE|SWP_NOZORDER);
  SetIcon(AfxGetApp()->LoadIcon(1),TRUE);
  CenterWindow();
  keepOnScreen();
  popupTheme.apply(m_hWnd,nullptr,uiDark,GetDpiForWindow(m_hWnd));
  if(init)init();
  layoutAnchoredControls();
  refreshTheme(false);
  if(afterInit)afterInit();
  if(pulse)SetTimer(pulseTimer,pulseInterval,nullptr);
  return TRUE;
 }
 BOOL OnCommand(WPARAM w,LPARAM l)override{auto it=actions.find(LOWORD(w));const auto code=HIWORD(w);if(it!=actions.end()&&code==BN_CLICKED){try{it->second();}catch(const std::exception& e){error(this,e);}return TRUE;}auto changed=changes.find(LOWORD(w));if(changed!=changes.end()&&(code==EN_CHANGE||code==CBN_SELCHANGE||code==CBN_EDITCHANGE)){try{changed->second(code);}catch(const std::exception& e){error(this,e);}return TRUE;}return CDialog::OnCommand(w,l);}
 void OnOK()override{if(accept){try{accept();}catch(const std::exception& e){error(this,e);}}}
 void OnCancel()override{if(cancel){try{cancel();}catch(const std::exception& e){error(this,e);}}else if(modeless)DestroyWindow();else CDialog::OnCancel();}
public:
 static constexpr UINT_PTR pulseTimer=7;
 static constexpr UINT pulseInterval=250;
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
 void refreshTheme(bool includeFrame=true){
  if(!GetSafeHwnd())return;
  if(includeFrame){themeFrame(m_hWnd);popupTheme.apply(m_hWnd,nullptr,uiDark,GetDpiForWindow(m_hWnd));}
  for(auto& control:controls){auto window=control->GetSafeHwnd();if(!window)continue;
   wchar_t kind[32]{};GetClassNameW(window,kind,32);
   if(auto button=dynamic_cast<ActionButton*>(control.get()))button->ModifyStyle(BS_TYPEMASK,uiDark||button->hyperlink||!button->glyph.empty()?BS_OWNERDRAW:button->primary?BS_DEFPUSHBUTTON:BS_PUSHBUTTON);
   else if(!_wcsicmp(kind,L"BUTTON"))SetWindowTheme(window,uiDark?L"":nullptr,uiDark?L"":nullptr);
   if(auto tree=dynamic_cast<CTreeCtrl*>(control.get())){themeScrollbars(window);tree->SetBkColor(uiBackground());tree->SetTextColor(uiForeground());themeTooltip((HWND)tree->SendMessage(TVM_GETTOOLTIPS));}
   const bool edit=!_wcsicmp(kind,L"EDIT");
   if(edit||!_wcsicmp(kind,L"LISTBOX")||!_wcsicmp(kind,L"COMBOBOX"))themeScrollbars(window);
   if(edit)SetWindowSubclass(window,themeEditMenu,1,(DWORD_PTR)m_hWnd);
   if(auto combo=dynamic_cast<CComboBox*>(control.get())){COMBOBOXINFO info{sizeof(info)};if(GetComboBoxInfo(window,&info)){if(info.hwndList)themeScrollbars(info.hwndList);if(info.hwndItem){themeScrollbars(info.hwndItem);SetWindowSubclass(info.hwndItem,themeEditMenu,1,(DWORD_PTR)m_hWnd);}}}
   if(auto progress=dynamic_cast<CProgressCtrl*>(control.get())){SetWindowTheme(window,uiDark?L"":nullptr,uiDark?L"":nullptr);progress->SetBkColor(uiDark?RGB(48,51,56):CLR_DEFAULT);progress->SetBarColor(uiDark?RGB(35,139,63):CLR_DEFAULT);}
   if(auto list=dynamic_cast<CListCtrl*>(control.get())){list->SetBkColor(uiBackground());list->SetTextBkColor(uiBackground());list->SetTextColor(uiForeground());themeTooltip((HWND)list->SendMessage(LVM_GETTOOLTIPS));if(auto themed=dynamic_cast<ThemeList*>(list))themed->theme();}
  }
  RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_ERASE);
 }
 int MessageBox(LPCTSTR message,LPCTSTR title=nullptr,UINT flags=MB_OK){return themedMessage(this,message,title?title:L"UDM",flags);}
 bool modeless=false,dialogUnits=false,resizable=false;std::map<UINT,std::function<void(UINT)>> changes;std::function<void()> init,afterInit,accept,cancel,pulse;
 Form(std::string title,int w,int h,CWnd* parent=nullptr):CDialog(100,parent),caption(std::move(title)),width(w),height(h){}
 void keepOnScreen(){CRect r;GetWindowRect(&r);MONITORINFO monitor{sizeof(monitor)};if(!GetMonitorInfoW(MonitorFromWindow(m_hWnd,MONITOR_DEFAULTTONEAREST),&monitor))return;const auto& work=monitor.rcWork;int x=std::max<int>(work.left,std::min<int>(r.left,work.right-r.Width())),y=std::max<int>(work.top,std::min<int>(r.top,work.bottom-r.Height()));SetWindowPos(nullptr,x,y,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);}
 void resizeClient(int w,int h){
  width=w;height=h;
  const auto desired=rect(0,0,w,h);
  auto outer=desired;
  constexpr UINT flags=SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE;
  AdjustWindowRectExForDpi(&outer,GetStyle(),::GetMenu(m_hWnd)!=nullptr,GetExStyle(),(UINT)std::lround(scale*96));
  SetWindowPos(nullptr,0,0,outer.Width(),outer.Height(),flags);
  // The frame can still be using its previous non-client metrics during a DPI transition.
  CRect actual;GetClientRect(&actual);
  const int missingWidth=std::max(0,desired.Width()-actual.Width());
  const int missingHeight=std::max(0,desired.Height()-actual.Height());
  if(missingWidth||missingHeight){
   GetWindowRect(&outer);
   SetWindowPos(nullptr,0,0,outer.Width()+missingWidth,outer.Height()+missingHeight,flags);
  }
  keepOnScreen();
 }
 void placeControl(CWnd* control,int x,int y,int w,int h){if(!control||!control->GetSafeHwnd())return;logicalRects[control->GetSafeHwnd()]=CRect(x,y,x+w,y+h);control->MoveWindow(rect(x,y,w,h));}
 void anchorControl(CWnd* control,ResizeAnchor anchor){resizeAnchors[control->GetSafeHwnd()]=anchor;}
 void defaultButton(CWnd* control){if(auto button=dynamic_cast<ActionButton*>(control)){button->primary=true;if(!uiDark&&!button->hyperlink)button->ModifyStyle(BS_TYPEMASK,BS_DEFPUSHBUTTON);button->Invalidate();}}
 int pixelsX(int value)const{return dialogUnits?units.px(value):(int)(value*scale);}
 CRect rect(int x,int y,int w,int h){if(dialogUnits)return CRect(units.rect(x,y,w,h));return CRect((int)(x*scale),(int)(y*scale),(int)((x+w)*scale),(int)((y+h)*scale));}
 template<class T>T* make(DWORD style,int x,int y,int w,int h,UINT id=0,bool redrawFont=true){using Native=std::conditional_t<std::is_same_v<T,CListCtrl>,ThemeList,T>;auto control=std::make_unique<Native>();if(!control->Create(style|WS_CHILD|WS_VISIBLE,rect(x,y,w,h),this,id?id:nextId++))throw std::runtime_error("Cannot create interface control.");
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
 const double fontStart=formClockMs();
#endif
 control->SetFont(&font,redrawFont&&control->IsWindowVisible());
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
 formStage(caption,control->GetDlgCtrlID(),"font",fontStart);
#endif
if constexpr(std::is_same_v<T,CTabCtrl>)SetWindowTheme(control->GetSafeHwnd(),L"",L"");auto p=control.get();logicalRects[p->GetSafeHwnd()]=CRect(x,y,x+w,y+h);controls.push_back(std::move(control));return p;}
 CWnd* control(const wchar_t* type,std::string value,DWORD style,int x,int y,int w,int h,DWORD ex=0){auto c=std::make_unique<CWnd>();if(!c->CreateEx(ex,type,cs(value),style|WS_CHILD|WS_VISIBLE,rect(x,y,w,h),this,nextId++))throw std::runtime_error("Cannot create interface control.");if(uiDark&&!_wcsicmp(type,L"BUTTON")&&(style&BS_TYPEMASK)==BS_GROUPBOX)SetWindowTheme(c->GetSafeHwnd(),L"",L"");c->SetFont(&font,c->IsWindowVisible());auto p=c.get();logicalRects[p->GetSafeHwnd()]=CRect(x,y,x+w,y+h);controls.push_back(std::move(c));return p;}
 CWnd* label(std::string s,int x,int y,int w,int h=-1){return control(L"STATIC",s,SS_LEFT,x,y,w,h<0?(dialogUnits?9:18):h);}
 CWnd* edit(std::string s,int x,int y,int w,int h=-1,bool readonly=false,bool multi=false,bool secret=false){if(h<0)h=dialogUnits?14:23;auto c=control(L"EDIT",s,WS_TABSTOP|ES_AUTOHSCROLL|(readonly?ES_READONLY:0)|(multi?ES_MULTILINE|ES_AUTOVSCROLL|WS_VSCROLL:0)|(secret?ES_PASSWORD:0),x,y,w,h,WS_EX_CLIENTEDGE);c->SendMessage(EM_SETLIMITTEXT,multi?1024*1024:32768);return c;}
 CWnd* check(std::string s,bool value,int x,int y,int w,int h=-1){auto c=control(L"BUTTON",s,WS_TABSTOP|BS_AUTOCHECKBOX|BS_MULTILINE,x,y,w,h<0?(dialogUnits?10:21):h);if(uiDark)SetWindowTheme(c->GetSafeHwnd(),L"",L"");c->SendMessage(BM_SETCHECK,value?BST_CHECKED:BST_UNCHECKED);return c;}
 CWnd* button(std::string s,int x,int y,int w,std::function<void()> action,int h=-1){if(h<0)h=dialogUnits?14:25;auto c=std::make_unique<ActionButton>();if(!c->Create(cs(s),WS_CHILD|WS_VISIBLE|WS_TABSTOP|(uiDark?BS_OWNERDRAW:BS_PUSHBUTTON),rect(x,y,w,h),this,nextId++))throw std::runtime_error("Cannot create interface button.");c->SetFont(&font,c->IsWindowVisible());auto p=c.get();actions[p->GetDlgCtrlID()]=std::move(action);logicalRects[p->GetSafeHwnd()]=CRect(x,y,x+w,y+h);controls.push_back(std::move(c));return p;}
 CWnd* link(std::string s,int x,int y,int w,std::function<void()> action,int h=-1){auto value=button(s,x,y,w,std::move(action),h<0?(dialogUnits?9:18):h);auto button=static_cast<ActionButton*>(value);button->hyperlink=true;button->ModifyStyle(BS_TYPEMASK,BS_OWNERDRAW);return value;}
 CWnd* staticLink(std::string value,int x,int y,int w,std::function<void()> action,int h=-1){if(h<0)h=dialogUnits?9:18;auto control=std::make_unique<StaticHyperlink>();if(!control->Create(cs(value),WS_CHILD|WS_VISIBLE|WS_TABSTOP|SS_OWNERDRAW|SS_NOTIFY,rect(x,y,w,h),this,nextId++))throw std::runtime_error("Cannot create source-page link");control->SetFont(&font);auto p=control.get();actions[p->GetDlgCtrlID()]=std::move(action);logicalRects[p->GetSafeHwnd()]=CRect(x,y,x+w,y+h);controls.push_back(std::move(control));return p;}
 CWnd* glyphCheck(std::string name,bool value,int x,int y,int w,int h=10){auto c=std::make_unique<GlyphCheckBox>(cs(name));if(!c->Create(L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX,rect(x,y,w,h),this,nextId++))throw std::runtime_error("Cannot create checkbox.");c->SetFont(&font);if(uiDark)SetWindowTheme(c->GetSafeHwnd(),L"",L"");c->SendMessage(BM_SETCHECK,value?BST_CHECKED:BST_UNCHECKED);auto p=c.get();logicalRects[p->GetSafeHwnd()]=CRect(x,y,x+w,y+h);controls.push_back(std::move(c));return p;}
 template<class Combo=ThemeCombo> Combo* combo(const std::vector<std::string>& values,std::string current,int x,int y,int w,bool free=false){
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
  const double createStart=formClockMs();
#endif
  auto c=make<Combo>(WS_TABSTOP|WS_VSCROLL|(free?CBS_DROPDOWN|CBS_AUTOHSCROLL:CBS_DROPDOWNLIST),x,y,w,230,0,false);
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
  formStage(caption,c->GetDlgCtrlID(),"create-and-font",createStart);const double populateStart=formClockMs();
#endif
  const bool redraw=c->IsWindowVisible()!=FALSE;
  if(redraw)c->SetRedraw(FALSE);
  for(const auto& v:values)c->AddString(cs(v));
  const int index=values.empty()?CB_ERR:c->FindStringExact(-1,cs(current));
  if(index!=CB_ERR)c->SetCurSel(index);
  else if(free)c->SetWindowText(cs(current));
  else if(!values.empty())c->SetCurSel(0);
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
  formStage(caption,c->GetDlgCtrlID(),"populate-and-select",populateStart);const double redrawStart=formClockMs();
#endif
  if(redraw){c->SetRedraw(TRUE);c->Invalidate();}
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
  formStage(caption,c->GetDlgCtrlID(),"restore-redraw",redrawStart);
#endif
  return c;
 }
 QueueCombo* queueCombo(Manager& manager,const std::vector<std::string>& names,const std::string& current,int x,int y,int w,bool free=false){
  const auto labels=queueDisplayLabels(manager);
  auto control=combo<QueueCombo>({},"",x,y,w,free);
  for(const auto& name:names)control->addQueue(name,queueDisplayLabel(labels,name));
  if(!control->selectQueue(current)){
   if(free)control->SetWindowText(cs(queueDisplayLabel(labels,current)));
   else if(control->GetCount()>0)control->SetCurSel(0);
  }
  return control;
 }
 bool checked(CWnd* c){return c->SendMessage(BM_GETCHECK)==BST_CHECKED;}
 void bind(CWnd* c,std::function<void()> action){actions[c->GetDlgCtrlID()]=std::move(action);}
 void bindChange(CWnd* c,std::function<void(UINT)> action){changes[c->GetDlgCtrlID()]=std::move(action);}
 void bindChange(CWnd* c,std::function<void()> action){bindChange(c,[action=std::move(action)](UINT){action();});}
 void close(int code=IDOK){if(modeless)DestroyWindow();else EndDialog(code);}
};
BEGIN_MESSAGE_MAP(Form,CDialog)
 ON_WM_TIMER()
 ON_WM_SIZE()
 ON_WM_GETMINMAXINFO()
 ON_WM_CTLCOLOR()
 ON_WM_ERASEBKGND()
 ON_MESSAGE(WM_DPICHANGED,OnDialogDpiChanged)
END_MESSAGE_MAP()
inline int themedMessage(CWnd* owner,LPCWSTR message,LPCWSTR title,UINT flags){
 const UINT type=flags&MB_TYPEMASK;
 if(!uiDark||type>MB_CANCELTRYCONTINUE||(flags&(MB_HELP|MB_SERVICE_NOTIFICATION|MB_DEFAULT_DESKTOP_ONLY)))return ::MessageBoxW(owner?owner->GetSafeHwnd():nullptr,message,title,flags);
 std::vector<std::pair<int,std::string>> choices;
 switch(type){case MB_OK:choices={{IDOK,"OK"}};break;case MB_OKCANCEL:choices={{IDOK,"OK"},{IDCANCEL,"Cancel"}};break;case MB_YESNO:choices={{IDYES,"Yes"},{IDNO,"No"}};break;case MB_YESNOCANCEL:choices={{IDYES,"Yes"},{IDNO,"No"},{IDCANCEL,"Cancel"}};break;case MB_RETRYCANCEL:choices={{IDRETRY,"Retry"},{IDCANCEL,"Cancel"}};break;case MB_ABORTRETRYIGNORE:choices={{IDABORT,"Abort"},{IDRETRY,"Retry"},{IDIGNORE,"Ignore"}};break;default:choices={{IDCANCEL,"Cancel"},{IDTRYAGAIN,"Try again"},{IDCONTINUE,"Continue"}};break;}
 class MessageForm:public Form{public:CWnd* initial=nullptr;using Form::Form;BOOL OnInitDialog()override{Form::OnInitDialog();if(initial){initial->SetFocus();return FALSE;}return TRUE;}};
 const auto value=utf8(message?message:L"");const int lines=1+(int)std::count(value.begin(),value.end(),'\n');const int bodyHeight=std::clamp(std::max(lines*17,((int)value.size()/72+1)*17),62,300);
 MessageForm dialog(utf8(title?title:L"UDM"),540,bodyHeight+78,owner);int defaultIndex=std::min<int>((flags&MB_DEFMASK)>>8,(int)choices.size()-1);int cancelId=0;for(const auto& choice:choices)if(choice.first==IDCANCEL)cancelId=IDCANCEL;if(type==MB_OK)cancelId=IDOK;
 dialog.init=[&]{
  LPCWSTR iconName=(flags&MB_ICONMASK)==MB_ICONERROR?IDI_ERROR:(flags&MB_ICONMASK)==MB_ICONWARNING?IDI_WARNING:(flags&MB_ICONMASK)==MB_ICONQUESTION?IDI_QUESTION:(flags&MB_ICONMASK)==MB_ICONINFORMATION?IDI_INFORMATION:nullptr;int inset=14;if(iconName){auto icon=dialog.control(L"STATIC","",SS_ICON,14,16,32,32);icon->SendMessage(STM_SETICON,(WPARAM)LoadIconW(nullptr,iconName));inset=56;}auto body=dialog.edit(value,inset,14,526-inset,bodyHeight,true,true);body->ModifyStyle(WS_TABSTOP,0);
  for(size_t i=0;i<choices.size();++i){auto id=choices[i].first;auto button=dialog.button(choices[i].second,526-(int)(choices.size()-i)*100,bodyHeight+38,90,[&,id]{dialog.close(id);});if((int)i==defaultIndex){dialog.defaultButton(button);dialog.initial=button;}}
  dialog.accept=[&]{dialog.close(choices[defaultIndex].first);};dialog.cancel=[&]{if(cancelId)dialog.close(cancelId);};
 };return (int)dialog.DoModal();
}
inline void refreshOpenForms(){
 EnumThreadWindows(GetCurrentThreadId(),[](HWND window,LPARAM)->BOOL{if(auto form=dynamic_cast<Form*>(CWnd::FromHandlePermanent(window)))form->refreshTheme();return TRUE;},0);
}
#include "DragUi.hpp"
#include "QueueDragUi.hpp"
inline bool showBrowserOwnedDialog(Form& dialog,CWnd* owner,bool applyTheme=true){
 if(!owner||!GetPropW(owner->GetSafeHwnd(),L"UDM.CaptureDialogOwner"))return false;
 // The hidden owner has no taskbar entry. Keep the confirmation reachable
 // even if Windows denies the foreground request.
 if(applyTheme)dialog.refreshTheme();
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
 traceCapturePresentation("owned-theme-ready");
#endif
 dialog.ModifyStyleEx(WS_EX_TOOLWINDOW,WS_EX_APPWINDOW);
 dialog.SetWindowPos(&CWnd::wndTopMost,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
 traceCapturePresentation("owned-show-returned");
#endif
 dialog.UpdateWindow();
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
 traceCapturePresentation("owned-paint-returned");
#endif
 dialog.SetForegroundWindow();
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
 traceCapturePresentation("owned-foreground-returned");
#endif
 return true;
}
inline void openRefreshPage(HWND owner,const std::string& value){Url url(trim(value));if(url.scheme!="http"&&url.scheme!="https")throw std::runtime_error("Enter the original HTTP or HTTPS download page.");
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
 if(formRefreshPageObserver){formRefreshPageObserver(url.full);return;}
#endif
 auto result=ShellExecuteW(owner,L"open",wide(url.full).c_str(),nullptr,nullptr,SW_SHOWNORMAL);if((INT_PTR)result<=32)throw std::runtime_error("Windows could not open the download page. Use the Open page button to try again.");}
enum class DownloadRefreshMode { AllowResume, FileInfo };
class RefreshAddressDialog:public Form {
 DECLARE_MESSAGE_MAP()
 Manager& manager;JobPtr job;bool allowResume;CWnd *address=nullptr,*page=nullptr,*status=nullptr;Json candidate,displayed=Json::array(),previousLinkChoices;std::string seen;
 Json refreshPresentations()const{auto offers=manager.pendingBrowserPresentations(job);if(!offers.empty()&&str(offers.front(),"kind")!="refresh")offers.clear();return offers;}
 void poll(bool advanceReview=false){
  Lock lock(manager.mutex);
  auto offer=manager.addressRefreshCandidate(job);
  auto pending=refreshPresentations();
  if(offer.is_object()){
   if(offer.dump()!=seen||pending!=displayed){
    candidate=offer;seen=offer.dump();displayed=pending;
    address->SetWindowText(cs(str(offer,"url")));
    if(!str(offer,"page").empty())page->SetWindowText(cs(str(offer,"page")));
    status->SetWindowText(allowResume?L"A matching browser link arrived. Review the address, then choose Save or Resume.":L"A matching browser link arrived. Save it to return to Download File Info.");
   }
  }else if(advanceReview&&IsWindowEnabled()){
   const auto reviews=manager.pendingBrowserCaptureReviews(job);
   const bool newChoice=std::any_of(reviews.begin(),reviews.end(),[&](const auto& token){
    return std::find(previousLinkChoices.begin(),previousLinkChoices.end(),token)==previousLinkChoices.end();
   });
   if(newChoice){
    if(trim(text(address)).empty())close(IDCANCEL);
    else{
     const std::string message="A different link arrived. Close this dialog to review it, or continue with the address you entered.";
     if(text(status)!=message)status->SetWindowText(cs(message));
    }
   }
  }
 }
 void apply(bool resume){
  Lock lock(manager.mutex);auto latest=manager.addressRefreshCandidate(job);auto pending=refreshPresentations();
  if(latest!=candidate||pending!=displayed){
   if(latest.is_object())poll();else{candidate=latest;seen=latest.dump();displayed=pending;}
   status->SetWindowText(allowResume?L"The browser link changed or expired. Review the address, then choose Save or Resume again.":L"The browser link changed or expired. Review the address, then choose Save again.");return;
  }
  std::optional<Headers> headers;if(candidate.is_object()&&text(address)==str(candidate,"url"))headers=candidate["headers"].get<Headers>();manager.refreshAddress(job,trim(text(address)),headers,trim(text(page)));if(resume)manager.resume(job);close();
 }
 afx_msg void OnTimer(UINT_PTR id){try{if(id==2){KillTimer(2);if(!trim(text(page)).empty())openRefreshPage(m_hWnd,text(page));}else poll(true);}catch(const std::exception& e){status->SetWindowText(cs(e.what()));}Form::OnTimer(id);}
public:
 const Json& displayedPresentations()const{return displayed;}
 RefreshAddressDialog(Manager& m,JobPtr j,CWnd* owner,bool reopenPage,Json oldChoices,DownloadRefreshMode mode):Form("Refresh download address",610,302,owner),manager(m),job(j),allowResume(mode==DownloadRefreshMode::AllowResume),previousLinkChoices(std::move(oldChoices)){init=[this,owner,reopenPage]{
  Json data;{Lock lock(manager.mutex);data=job->data;}
  label(str(data,"FileName"),14,12,580,23);
  const bool ftp=Url(str(data,"Url")).scheme=="ftp";
  label(ftp?"Paste a replacement FTP address for the same file. UDM checks the saved bytes before resuming.":"Open the download page, then send the same file to UDM from the browser. You can also paste a fresh direct link below.",14,42,580,34);
  label("Download page",14,91,96);page=edit(recoveryPage(data),116,87,359);
  button("Open page",485,86,109,[this]{openRefreshPage(m_hWnd,text(page));});
  label("New address",14,126,96);address=edit("",116,122,478,47,false,true);
  status=label(ftp?"Enter the replacement address to continue.":"Waiting for a matching link from the browser (up to 10 minutes).",14,181,580,30);
  label("Saved parts: "+bytes(num(data,"Received"))+". Resume checks the file size and server validator before reusing them.",14,215,580,29);
  button("Save address",allowResume?219:364,262,116,[this]{apply(false);});
  if(allowResume)button("Save and resume",345,262,135,[this]{apply(true);});
  button("Cancel",490,262,104,[this]{close(IDCANCEL);});accept=[this]{apply(allowResume);};
  if(!ftp){poll();SetTimer(1,250,nullptr);if(reopenPage)SetTimer(2,100,nullptr);}
  showBrowserOwnedDialog(*this,owner);
 };}
};
BEGIN_MESSAGE_MAP(RefreshAddressDialog,Form)
 ON_WM_TIMER()
END_MESSAGE_MAP()
class RefreshMediaDialog:public Form {
 DECLARE_MESSAGE_MAP()
 Manager& manager;JobPtr job;CWnd* page=nullptr;CWnd* status=nullptr;CWnd* saveButton=nullptr;CWnd* resumeButton=nullptr;std::string launchError;Json candidate,displayed=Json::array();
 std::optional<std::string> observedOffer;
 Json refreshPresentations()const{auto offers=manager.pendingBrowserPresentations(job);if(!offers.empty()&&str(offers.front(),"kind")!="refresh")offers.clear();return offers;}
 void poll(bool force=false){
  {
   Lock lock(manager.mutex);
   const std::string empty;
   const auto value=job->data.find("ProtectedRefreshOffer");
   const auto& encrypted=value!=job->data.end()&&value->is_string()?value->get_ref<const std::string&>():empty;
   if(!force&&observedOffer&&*observedOffer==encrypted&&
      (encrypted.empty()||addressRefreshCandidateCurrent(candidate,epoch())))return;
   auto latest=manager.addressRefreshCandidate(job);
   auto pending=refreshPresentations();
   observedOffer=encrypted;
   candidate=std::move(latest);
   displayed=std::move(pending);
  }
  const auto kind=str(candidate,"kind");
  const bool ready=kind=="sabr"||kind=="adaptive"||kind=="direct-media";
  if((saveButton->IsWindowEnabled()!=FALSE)!=ready)saveButton->EnableWindow(ready);
  if(resumeButton&&(resumeButton->IsWindowEnabled()!=FALSE)!=ready)resumeButton->EnableWindow(ready);
  const std::string message=ready?(resumeButton?"Matching streams received. Review the selection above, then save or resume.":"Matching streams received. Save the session to return to Download File Info."):launchError.empty()?"Waiting for the same video, quality and audio track from the browser (10 minutes).":launchError;
  if(text(status)!=message)status->SetWindowText(cs(message));
 }
 void apply(bool start){Lock lock(manager.mutex);
  if(manager.addressRefreshCandidate(job)!=candidate||refreshPresentations()!=displayed){poll(true);status->SetWindowText(L"The captured session changed or expired. Review the selection and try again.");return;}
  manager.applyMediaRefresh(job);if(start)manager.resume(job);close();
 }
 void openPage(){openRefreshPage(m_hWnd,manager.mediaRefreshPage(job));}
 afx_msg void OnTimer(UINT_PTR id){try{if(id==2){KillTimer(2);openPage();}else poll();}catch(const std::exception& e){launchError=e.what();status->SetWindowText(cs(launchError));}Form::OnTimer(id);}
public:
 const Json& displayedPresentations()const{return displayed;}
 RefreshMediaDialog(Manager& m,JobPtr j,CWnd* owner,bool reopenPage,DownloadRefreshMode mode):Form("Refresh media session",574,290,owner),manager(m),job(j){init=[this,owner,reopenPage,mode]{Json data;{Lock lock(manager.mutex);data=job->data;}
  label(str(data,"FileName"),14,12,546,24);
  const bool fixedVideo=!str(data,"ProtectedSabr").empty()&&str(data,"MediaOutput")!="audio";
  label(fixedVideo?
   "1. Open the original video and let it play.\r\n2. Click Download this video in the UDM panel to request the saved quality.\r\n3. Return here to save the matching session.":
   "1. Open the original video and let it play.\r\n2. In its UDM panel, select the same quality or audio track.\r\n3. Return here to apply the matching session to this download.",14,43,546,55);
  label("Original page",14,109,97);page=edit(str(data,"SourceUrl",str(data,"Url")),114,105,446,23,true);
  button("Open video page",14,143,142,[this]{openPage();launchError.clear();poll(true);});
  const auto retained=num(data,"SabrRetainedBytes");
  label(str(data,"FormatDescription")+(!str(data,"ProtectedSabr").empty()?"\r\nRetained data: "+bytes(retained)+
   (retained>0&&num(data,"StreamCompletedMs")<=0?". No playable progress yet.":". Filename and history are preserved."):
   "\r\nSaved segments are checked against the fresh capture before reuse."),170,141,390,38);
  status=label("",14,187,546,43);
  const bool allowResume=mode==DownloadRefreshMode::AllowResume;
  saveButton=button("Save session",allowResume?171:321,246,117,[this]{apply(false);});
  if(allowResume)resumeButton=button("Save and resume",298,246,140,[this]{apply(true);});
  button("Cancel",448,246,112,[this]{close(IDCANCEL);});
  accept=[this,allowResume]{apply(allowResume);};
  poll();SetTimer(1,250,nullptr);if(reopenPage)SetTimer(2,100,nullptr);
  showBrowserOwnedDialog(*this,owner);
 };}
};
BEGIN_MESSAGE_MAP(RefreshMediaDialog,Form)
 ON_WM_TIMER()
END_MESSAGE_MAP()
inline bool refreshDownloadAddress(CWnd* owner,Manager& manager,JobPtr job,bool reopenPage=true,DownloadRefreshMode mode=DownloadRefreshMode::AllowResume){
 Json previousLinkChoices;
 {
  Lock lock(manager.mutex);
  previousLinkChoices=manager.pendingBrowserCaptureReviews(job);
  manager.beginAddressRefresh(job,!reopenPage);
 }
 try{
  bool streaming;{Lock lock(manager.mutex);streaming=!str(job->data,"SourceUrl").empty()||!str(job->data,"ProtectedAdaptive").empty();}
  INT_PTR result;Json displayed=Json::array();
  if(streaming){RefreshMediaDialog dialog(manager,job,owner,reopenPage,mode);result=dialog.DoModal();displayed=dialog.displayedPresentations();}
  else{RefreshAddressDialog dialog(manager,job,owner,reopenPage,std::move(previousLinkChoices),mode);result=dialog.DoModal();displayed=dialog.displayedPresentations();}
  if(result==-1)throw std::runtime_error("Cannot open the download refresh dialog.");
  manager.cancelAddressRefresh(job);
  // Cancel acknowledges only links actually shown by this dialog. A newer
  // receipt committed after its last poll remains available for presentation.
  for(const auto& offer:displayed)manager.finishBrowserPresentation(offer);
  return result==IDOK;
 }catch(...){manager.cancelAddressRefresh(job);throw;}
}
inline std::string prompt(CWnd* parent,std::string title,std::string value=""){Form d(title,380,95,parent);CWnd* input=nullptr;std::string result;d.init=[&]{d.label("Name",10,12,48);input=d.edit(value,60,9,309);d.accept=[&]{result=trim(text(input));if(result.empty()||result.size()>80)throw std::runtime_error("Enter a name up to 80 characters.");d.close();};d.button("OK",205,58,78,d.accept);d.button("Cancel",291,58,78,[&]{d.close(IDCANCEL);});input->SetFocus();};return d.DoModal()==IDOK?result:"";}
inline std::vector<std::string> queueNames(Manager& m){Lock l(m.mutex);std::vector<std::string> names;for(auto q:m.state["Queues"])names.push_back(str(q,"Name"));return names;}
inline std::string primaryDownloadQueueName(const Json& queues){
 std::string fallback;
 for(const auto& queue:queues){
  if(yes(queue,"Synchronize"))continue;
  auto name=str(queue,"Name");
  if(str(queue,"DefaultQueueRole")=="Download")return name;
  if(fallback.empty()||name=="Main queue")fallback=std::move(name);
 }
 return fallback;
}
#include "QueueChoiceUi.hpp"
class RelocationWorkDialog:public Form {
 using Move=std::pair<JobPtr,fs::path>;
 Manager& manager;std::vector<Move> moves;Cancel cancellation;
 std::thread worker;std::atomic_bool done{false};std::mutex progressMutex;
 RelocationProgress progress{RelocationPhase::Preparing};std::string failure;bool cancelled=false;
 CWnd *status=nullptr,*location=nullptr,*detail=nullptr,*cancelButton=nullptr,*rateInfo=nullptr,*timeInfo=nullptr;CProgressCtrl* bar=nullptr;bool marquee=true;
 SpeedMeter meter;RelocationPhase measuredPhase=RelocationPhase::Preparing;i64 measuredTotal=0;
 size_t currentFile=0,displayedFile=0,finished=0;
 std::chrono::steady_clock::time_point began=std::chrono::steady_clock::now(),lastSample=began;
 std::string moveLocation(size_t index)const{
  auto path=utf8(moves[index].second.wstring());
  if(moves.size()==1)return path;
  return std::to_string(index+1)+" of "+std::to_string(moves.size())+"  "+path;
 }
 static std::string moveTime(double seconds){
  if(!std::isfinite(seconds)||seconds<0)return "Unknown";
  if(seconds>7*86400)return "Over 7 days";
  const auto value=(i64)std::ceil(seconds);return value>=3600?std::to_string(value/3600)+" hr "+std::to_string(value/60%60)+" min":value>=60?std::to_string(value/60)+" min "+std::to_string(value%60)+" sec":std::to_string(value)+" sec";
 }
 static const char* phaseText(RelocationPhase phase){switch(phase){
  case RelocationPhase::Preparing:return "Preparing the file move...";
  case RelocationPhase::VerifyingSource:return "Verifying the original file...";
  case RelocationPhase::Moving:return "Moving the file...";
  case RelocationPhase::VerifyingOriginal:return "Checking the original file after interruption...";
  case RelocationPhase::VerifyingDestination:return "Verifying the new location...";
  case RelocationPhase::SavingHistory:return "Saving download history...";
  case RelocationPhase::RestoringOriginal:return "Restoring the original location...";
  case RelocationPhase::Complete:return "File move complete.";
 }return "Working...";}
 void requestCancel(){
  RelocationProgress current;{std::lock_guard<std::mutex> guard(progressMutex);current=progress;}
  if(!current.cancellable)return;cancellation.stop=true;cancelButton->EnableWindow(FALSE);
  status->SetWindowText(L"Cancelling; waiting for file recovery...");
 }
public:
 void OnOK()override{} // Enter must not close a live worker dialog.
 RelocationWorkDialog(CWnd* owner,Manager& manager,JobPtr job,fs::path destination)
  :RelocationWorkDialog(owner,manager,{{std::move(job),std::move(destination)}}){}
 RelocationWorkDialog(CWnd* owner,Manager& manager,std::vector<Move> moves):Form("Move/Rename",480,240,owner),manager(manager),moves(std::move(moves)){
  if(this->moves.empty())throw std::invalid_argument("Select at least one completed download to move.");
  init=[this]{
   status=label("Preparing the file move...",16,16,448,24);
   location=label(moveLocation(0),16,47,448,22);location->ModifyStyle(0,SS_PATHELLIPSIS);
   detail=label("",16,76,448,22);
   rateInfo=label("Speed: measuring...",16,103,448,22);timeInfo=label("Elapsed: 0 sec",16,130,448,22);
   bar=make<CProgressCtrl>(PBS_MARQUEE,16,161,448,16);bar->SetRange32(0,1000);bar->SendMessage(PBM_SETMARQUEE,TRUE,40);
   cancel=[this]{requestCancel();};cancelButton=button("Cancel",364,200,100,cancel);cancelButton->EnableWindow(FALSE);
   began=lastSample=std::chrono::steady_clock::now();
   worker=std::thread([this]{
    try{
     for(const auto& [job,destination]:this->moves){
      {std::lock_guard<std::mutex> guard(progressMutex);currentFile=finished;progress={RelocationPhase::Preparing};}
      this->manager.relocate(job,destination,&cancellation,[this](const RelocationProgress& value){std::lock_guard<std::mutex> guard(progressMutex);progress=value;});
      ++finished;
     }
    }
    catch(const RelocationCancelled&){cancelled=true;}
    catch(const std::exception& e){failure=e.what();}
    catch(...){failure="The file move failed. Preserve any recovery journal.";}
    done.store(true,std::memory_order_release);
   });
  };
  pulse=[this]{
   if(done.load(std::memory_order_acquire)){if(worker.joinable())worker.join();close(cancelled?IDCANCEL:IDOK);return;}
   RelocationProgress current;size_t index;
   {std::lock_guard<std::mutex> guard(progressMutex);current=progress;index=currentFile;}
   const bool nextFile=index!=displayedFile;
   if(nextFile){displayedFile=index;location->SetWindowText(cs(moveLocation(index)));}
   const auto now=std::chrono::steady_clock::now();double speed=0;
   if(nextFile||current.phase!=measuredPhase||current.total!=measuredTotal||current.completed<meter.previous){meter.reset(current.completed);measuredPhase=current.phase;measuredTotal=current.total;}
   else speed=meter.update(current.completed,std::chrono::duration<double>(now-lastSample).count(),current.total>0);
   lastSample=now;
   const bool measured=current.total>0&&meter.duration>=0.5;
   rateInfo->SetWindowText(cs(current.total<=0?"Speed: unavailable":measured?"Speed: "+bytes(speed)+"/sec":"Speed: measuring..."));
   const double remaining=measured&&speed>0?std::max(0.0,(double)current.total-current.completed)/speed:-1;
   timeInfo->SetWindowText(cs("Elapsed: "+moveTime(std::chrono::duration<double>(now-began).count())+"   Phase left: "+moveTime(remaining)));
   status->SetWindowText(cs(cancellation.cancelled()&&current.cancellable?"Cancelling; waiting for file recovery...":phaseText(current.phase)));
   cancelButton->EnableWindow(current.cancellable&&!cancellation.cancelled());
   const bool indeterminate=current.total<=0;if(indeterminate!=marquee){bar->SendMessage(PBM_SETMARQUEE,FALSE,0);marquee=indeterminate;bar->ModifyStyle(marquee?0:PBS_MARQUEE,marquee?PBS_MARQUEE:0);if(marquee)bar->SendMessage(PBM_SETMARQUEE,TRUE,40);}
   if(!indeterminate){bar->SetPos((int)std::clamp<long double>((long double)current.completed*1000/current.total,0,1000));detail->SetWindowText(cs(bytes((double)current.completed)+" of "+bytes((double)current.total)));}
   else detail->SetWindowText(L"");
  };
 }
 ~RelocationWorkDialog(){cancellation.stop=true;if(worker.joinable())worker.join();}
 void run(){
  const auto result=DoModal();
  if(!done.load(std::memory_order_acquire))cancellation.stop=true;
  if(worker.joinable())worker.join();
  if(result==-1)throw std::runtime_error("Cannot open file move progress.");
  if(moves.size()==1){if(!failure.empty())throw std::runtime_error(failure);return;}
  const auto summary=std::to_string(finished)+" of "+std::to_string(moves.size())+" files processed.";
  if(!failure.empty())throw std::runtime_error(summary+"\r\nCould not move "+utf8(moves[finished].second.filename().wstring())+".\r\n"+failure);
  if(cancelled&&finished)AfxMessageBox(cs(summary+"\r\nThe remaining moves were cancelled."),MB_ICONINFORMATION);
 }
};
inline void moveCompleted(CWnd* owner,Manager& manager,JobPtr job){
 fs::path source;{Lock lock(manager.mutex);source=job->target();}
 auto path=source.wstring(),extension=source.extension().wstring();if(!extension.empty()&&extension.front()==L'.')extension.erase(0,1);
 CFileDialog chooser(FALSE,extension.empty()?nullptr:extension.c_str(),path.c_str(),OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR,L"All Files|*.*||",owner);
 if(chooser.DoModal()==IDOK){RelocationWorkDialog progress(owner,manager,job,fs::path((LPCWSTR)chooser.GetPathName()));progress.run();}
}
inline void moveCompletedToFolder(CWnd* owner,Manager& manager,const std::vector<JobPtr>& jobs){
 std::wstring initial;
 {Lock lock(manager.mutex);initial=jobs.front()->target().parent_path().wstring();}
 const auto folder=chooseFolder(owner,initial);
 if(folder.empty())return;
 std::vector<std::pair<JobPtr,fs::path>> moves;
 {Lock lock(manager.mutex);for(const auto& job:jobs)moves.emplace_back(job,fs::path(folder)/job->target().filename());}
 RelocationWorkDialog progress(owner,manager,std::move(moves));
 progress.run();
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
  showBrowserOwnedDialog(dialog,parent);
 };if(dialog.DoModal()==-1)throw std::runtime_error("Cannot open the duplicate download dialog.");return result;
}
#include "CaptureExclusionsUi.hpp"
#include "ZipPreviewUi.hpp"
inline std::optional<std::string> showCategoryProperties(CWnd* owner,Manager& manager,const Json& p,const std::string& original){
  std::optional<std::string> accepted;
  Form d("UDM categories",322,177,owner);d.dialogUnits=true;
  d.init=[&]{
   d.label("Category name",9,3,235,10);auto name=d.edit(original=="Archives"?"Compressed":original,9,13,238,14,original=="Other");
   auto paths=dictionary(p["CategoryPaths"]);std::string extensions=original.empty()?"":categoryExtensions(p,original),hosts;
   for(const auto& rule:p["CategoryRules"])if(str(rule,"Category")==original){extensions=str(rule,"Extensions");hosts=str(rule,"Hosts");break;}
   d.label("Automatically put in this category the following file types:",9,33,238,10);auto ex=d.edit(extensions,9,43,238,14);
   d.label("Note: type file extensions separated by space (e.g. avi mpg mpeg)",9,58,302,10);
   d.control(L"STATIC","",SS_ETCHEDHORZ,9,72,302,1);
   auto restrictSites=d.check("Automatically put in this category the files from the following sites only:",!hosts.empty(),9,78,302,10);
   auto sites=d.edit(hosts,9,90,302,14);sites->EnableWindow(!hosts.empty());
   d.bind(restrictSites,[&,restrictSites,sites]{sites->EnableWindow(d.checked(restrictSites));});
   d.label("Separate sites by spaces. Use *.example.com for a domain and its subdomains.",9,106,302,10);
   d.control(L"STATIC","",SS_ETCHEDHORZ,9,121,302,1);
   d.label("Save future downloads of this category to the following folder:",9,127,302,10);auto folder=d.edit(str(paths,original.c_str()),9,138,302,14);
   auto remember=d.check("Remember last save path",yes(p.value("CategoryRememberLast",Json::object()),original.c_str()),9,155,228,10);
   d.button("Browse...",261,155,50,[&,folder]{auto value=chooseFolder(&d,wide(text(folder)));if(!value.empty())folder->SetWindowText(value.c_str());},14);
   d.accept=[&,name,ex,sites,folder,restrictSites,remember]{auto value=trim(text(name));if(original=="Archives"&&value=="Compressed")value=original;auto hosts=d.checked(restrictSites)?trim(text(sites)):std::string();if(d.checked(restrictSites)&&hosts.empty())throw std::runtime_error("Enter at least one site or turn off the site restriction.");saveCategoryProperties(manager,original,value,text(ex),hosts,trim(text(folder)),d.checked(remember));accepted=std::move(value);d.close();};
   auto ok=d.button("OK",261,13,50,d.accept,14);d.defaultButton(ok);d.button("Cancel",261,34,50,[&]{d.close(IDCANCEL);},14);
  };d.DoModal();return accepted;
 }
#include "ScheduleDownloadUi.hpp"
#include "DownloadInfoUi.hpp"

class CaptureDialogOwner {
 CWnd window;CWnd* main=nullptr;bool enabled=false;
public:
 explicit CaptureDialogOwner(CWnd* original,bool isolate):main(isolate?original:nullptr){
  if(!isolate)return;
  CRect rect(0,0,1,1);if(auto foreground=::GetForegroundWindow())::GetWindowRect(foreground,&rect);
  // Keep the hidden owner in the same topmost group as its confirmation.
  // Activating a later modal must not demote File Info through its owner.
  if(!window.CreateEx(WS_EX_TOOLWINDOW|WS_EX_TOPMOST,AfxRegisterWndClass(0),L"UDM capture owner",WS_POPUP,rect,nullptr,0))throw std::runtime_error("Cannot create download dialog owner.");
  SetPropW(window.GetSafeHwnd(),L"UDM.CaptureDialogOwner",(HANDLE)1);
  if(main&&main->GetSafeHwnd()){enabled=main->IsWindowEnabled()!=FALSE;if(enabled)main->EnableWindow(FALSE);}
 }
 ~CaptureDialogOwner(){if(window.GetSafeHwnd())window.DestroyWindow();if(enabled&&main&&main->GetSafeHwnd())main->EnableWindow(TRUE);}
 CWnd* get(CWnd* original){return window.GetSafeHwnd()?&window:original;}
};
inline void presentDownload(CWnd* owner,Manager& manager,JobPtr job,bool automatic=false){
 CaptureDialogOwner captureOwner(owner,automatic);owner=captureOwner.get(owner);
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
 traceCapturePresentation("dialog-owner-ready");
#endif
 auto context=automatic?manager.takeAutomaticCapture(job):Json::object();bool cancelled=false;
 try{
  job=chooseDuplicate(owner,manager,job);
  if(!job)cancelled=true;
  else{auto presentation=manager.presentOffer(job);
   if(presentation==OfferPresentation::Complete){if(manager.showCompletedDownload)manager.showCompletedDownload(job);}
   else if(presentation==OfferPresentation::Progress){if(manager.event)manager.event(job,false);}
   else{
    bool freshBrowserOffer=false;
    if(automatic){Lock lock(manager.mutex);freshBrowserOffer=str(job->data,"Status")=="Awaiting confirmation";}
    cancelled=!downloadInfo(owner,manager,job,false,freshBrowserOffer);
   }
  }
 }catch(...){manager.finishAutomaticCapture(context,false);throw;}
 auto offer=manager.finishAutomaticCapture(context,cancelled);if(!offer.empty()){CaptureExclusionOffer dialog(owner,manager,offer);dialog.DoModal();}
}
#include "CaptureReviewUi.hpp"
inline bool presentBrowserCapture(CWnd* owner,Manager& manager,const Json& offer){
 auto job=manager.restoreBrowserPresentation(offer);if(!job)return false;
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
 traceCapturePresentation("offer-restored");
#endif
 if(str(offer,"kind")=="refresh"){
  CaptureDialogOwner captureOwner(owner,true);
  refreshDownloadAddress(captureOwner.get(owner),manager,job,false);
 }
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
  auto loginLabel=d.label("Login",13,36,60);auto user=d.edit("",77,34,78);
  auto passwordLabel=d.label("Password",160,36,64);auto password=d.edit("",228,34,75,14,false,false,true);
  auto toggle=[&d,auth,loginLabel,user,passwordLabel,password]{
   const BOOL enabled=d.checked(auth);
   for(auto control:{loginLabel,user,passwordLabel,password})control->EnableWindow(enabled);
  };d.bind(auth,toggle);toggle();
  d.accept=[&,address,auth,user,password]{Url url(trim(text(address)));if(hostIs(url.host,"youtube.com")||url.host=="youtu.be")throw std::runtime_error("Open this video in the browser and choose its quality using the UDM panel.");Headers headers;if(d.checked(auth))setBasicLogin(headers,text(user),text(password));job=manager.offerDownload(url.full,"","","Main queue",true,headers);d.close();};
  d.defaultButton(d.button("O&K",316,7,50,d.accept,13));d.button("&Cancel",316,24,50,[&]{d.close(IDCANCEL);});
  address->SetFocus();address->SetEditSel(0,-1);
 };if(d.DoModal()==IDOK&&job)presentDownload(parent,manager,job);
}
#include "SelectionUi.hpp"
class RangeMap:public CWnd {
public:
 using Span=std::pair<i64,i64>; // First byte and exclusive end in the displayed coordinate space.
 static void appendCoverage(std::vector<Span>& spans,const Json& segments,i64 size,i64 offset=0){
  if(size<=0)return;
  for(const auto& segment:segments){
   const auto begin=std::clamp<i64>(num(segment,"Start"),0,size);
   const auto end=std::clamp<i64>(num(segment,"End"),begin-1,size-1)+1;
   const auto done=std::clamp<i64>(num(segment,"Done"),0,end-begin);
   if(done>0)spans.emplace_back(offset+begin,offset+begin+done);
  }
 }
private:
 DECLARE_MESSAGE_MAP()
 std::vector<Span> coverageSpans;
 i64 total=0;std::vector<i64> starts;
 void draw(CDC& dc){
  CRect frame;GetClientRect(&frame);
  const bool highContrast=windowsHighContrast();
  const COLORREF background=highContrast?GetSysColor(COLOR_WINDOW):uiDark?RGB(33,42,54):RGB(235,242,250);
  const COLORREF coverage=highContrast?GetSysColor(COLOR_HIGHLIGHT):uiDark?RGB(53,151,206):RGB(28,111,180);
  const COLORREF marker=highContrast?GetSysColor(COLOR_WINDOWTEXT):uiDark?RGB(255,210,114):RGB(159,68,0);
  const COLORREF border=highContrast?GetSysColor(COLOR_WINDOWFRAME):uiDark?RGB(83,105,126):RGB(149,172,196);
  dc.FillSolidRect(frame,background);dc.Draw3dRect(frame,border,border);
  auto area=frame;area.DeflateRect(1,1);
  if(total<=0||area.Width()<=0||area.Height()<=0)return;
  auto x=[&](i64 offset){return (int)area.left+(int)((double)std::clamp<i64>(offset,0,total)*area.Width()/total);};
  for(int i=1;i<4;++i){const int tick=area.left+area.Width()*i/4;dc.FillSolidRect(tick,area.top,1,area.Height(),border);}
  for(const auto& [begin,end]:coverageSpans){
   const int left=x(begin),right=x(end);
   if(right>left)dc.FillSolidRect(left,area.top,right-left,area.Height(),coverage);
  }
  for(const auto offset:starts)if(offset>=0&&offset<total){
   const int start=std::min(x(offset),(int)area.right-1);
   dc.FillSolidRect(start,area.top,1,area.Height(),marker);
   dc.FillSolidRect(start,area.top,std::min<int>(3,area.right-start),std::min<int>(3,area.Height()),marker);
  }
 }
 afx_msg void OnPaint(){CPaintDC dc(this);draw(dc);}
 afx_msg LRESULT OnPrintClient(WPARAM dc,LPARAM){draw(*CDC::FromHandle(reinterpret_cast<HDC>(dc)));return 0;}
public:
 BOOL Create(DWORD style,const RECT& rect,CWnd* parent,UINT id){return CWnd::Create(AfxRegisterWndClass(0),L"Downloaded file pieces",style,rect,parent,id);}
 void update(std::vector<Span> spans,i64 size,std::vector<i64> offsets){
  if(total==size&&coverageSpans==spans&&starts==offsets)return;
  coverageSpans=std::move(spans);total=size;starts=std::move(offsets);Invalidate(FALSE);
 }
};
BEGIN_MESSAGE_MAP(RangeMap,CWnd)
 ON_WM_PAINT()
 ON_MESSAGE(WM_PRINTCLIENT,OnPrintClient)
END_MESSAGE_MAP()
#include "DialogPreferences.hpp"
#include "ProgressUi.hpp"
}
