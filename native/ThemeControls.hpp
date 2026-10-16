// Request Windows' per-control Explorer scrollbar theme where available.
inline void themeScrollbars(HWND window){
 using AllowWindow=bool(WINAPI*)(HWND,bool);
 static auto allow=[]()->AllowWindow{
  using Version=LONG(WINAPI*)(OSVERSIONINFOW*);auto version=reinterpret_cast<Version>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"),"RtlGetVersion"));OSVERSIONINFOW info{sizeof(info)};
  if(!version||version(&info)!=0||info.dwMajorVersion!=10||info.dwBuildNumber<17763)return nullptr;
  auto module=LoadLibraryExW(L"uxtheme.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);return module?reinterpret_cast<AllowWindow>(GetProcAddress(module,MAKEINTRESOURCEA(133))):nullptr;
 }();
 if(allow)allow(window,uiDark);SetWindowTheme(window,uiDark?L"DarkMode_Explorer":L"Explorer",nullptr);
}
inline void buildEditContextMenu(HWND window,CMenu& menu){
 const auto style=GetWindowLongPtrW(window,GWL_STYLE);DWORD begin=0,end=0;SendMessageW(window,EM_GETSEL,(WPARAM)&begin,(LPARAM)&end);const bool selected=begin!=end,writable=IsWindowEnabled(window)&&!(style&ES_READONLY),secret=(style&ES_PASSWORD)!=0;
 menu.CreatePopupMenu();auto add=[&](UINT id,LPCWSTR label,bool enabled){menu.AppendMenu(MF_STRING|(enabled?MF_ENABLED:MF_GRAYED),id,label);};add(1,L"&Undo",writable&&SendMessageW(window,EM_CANUNDO,0,0));menu.AppendMenu(MF_SEPARATOR);add(2,L"Cu&t",writable&&selected&&!secret);add(3,L"&Copy",selected&&!secret);add(4,L"&Paste",writable&&IsClipboardFormatAvailable(CF_UNICODETEXT));add(5,L"&Delete",writable&&selected);menu.AppendMenu(MF_SEPARATOR);add(6,L"Select &all",GetWindowTextLengthW(window)>0);menu.AppendMenu(MF_STRING|((GetWindowLongPtrW(window,GWL_EXSTYLE)&WS_EX_RTLREADING)?MF_CHECKED:0),7,L"Right-to-left reading order");
}
inline void editContextCommand(HWND window,UINT command){switch(command){case 1:SendMessageW(window,EM_UNDO,0,0);break;case 2:SendMessageW(window,WM_CUT,0,0);break;case 3:SendMessageW(window,WM_COPY,0,0);break;case 4:SendMessageW(window,WM_PASTE,0,0);break;case 5:SendMessageW(window,EM_REPLACESEL,TRUE,(LPARAM)L"");break;case 6:SendMessageW(window,EM_SETSEL,0,-1);break;case 7:SetWindowLongPtrW(window,GWL_EXSTYLE,GetWindowLongPtrW(window,GWL_EXSTYLE)^WS_EX_RTLREADING);InvalidateRect(window,nullptr,TRUE);break;}}
// Edit controls own their standard context menus; route only menu painting to the dialog.
inline LRESULT CALLBACK themeEditMenu(HWND window,UINT message,WPARAM w,LPARAM l,UINT_PTR id,DWORD_PTR owner){
 if(message==WM_NCDESTROY){RemoveWindowSubclass(window,themeEditMenu,id);return DefSubclassProc(window,message,w,l);}
 if(uiDark&&IsWindow((HWND)owner)){
  if(message==WM_CONTEXTMENU){CMenu menu;buildEditContextMenu(window,menu);POINT point{(short)LOWORD(l),(short)HIWORD(l)};if(point.x==-1&&point.y==-1){RECT area{};GetWindowRect(window,&area);point={area.left+8,area.bottom};}SetFocus(window);SendMessageW((HWND)owner,WM_INITMENUPOPUP,(WPARAM)menu.GetSafeHmenu(),0);auto command=::TrackPopupMenu(menu.GetSafeHmenu(),TPM_RETURNCMD|TPM_NONOTIFY|TPM_RIGHTBUTTON,point.x,point.y,0,(HWND)owner,nullptr);SendMessageW((HWND)owner,WM_UNINITMENUPOPUP,(WPARAM)menu.GetSafeHmenu(),0);if(command)editContextCommand(window,command);return 0;}
  if(message==WM_INITMENUPOPUP||message==WM_UNINITMENUPOPUP||message==WM_MENUCHAR)return SendMessageW((HWND)owner,message,w,l);
  if(message==WM_MEASUREITEM&&l&&((MEASUREITEMSTRUCT*)l)->CtlType==ODT_MENU)return SendMessageW((HWND)owner,message,w,l);
  if(message==WM_DRAWITEM&&l&&((DRAWITEMSTRUCT*)l)->CtlType==ODT_MENU)return SendMessageW((HWND)owner,message,w,l);
 }
 return DefSubclassProc(window,message,w,l);
}
inline void themeTooltip(HWND window){if(!window)return;SetWindowTheme(window,uiDark?L"":nullptr,uiDark?L"":nullptr);SendMessageW(window,TTM_SETTIPBKCOLOR,uiDark?RGB(43,46,51):GetSysColor(COLOR_INFOBK),0);SendMessageW(window,TTM_SETTIPTEXTCOLOR,uiDark?uiForeground():GetSysColor(COLOR_INFOTEXT),0);}
class ThemeCombo:public CComboBox {
 DECLARE_MESSAGE_MAP()
 void paint(CDC& dc){CRect area;GetClientRect(&area);COMBOBOXINFO info{sizeof(info)};::GetComboBoxInfo(m_hWnd,&info);dc.FillSolidRect(area,uiBackground());dc.Draw3dRect(area,RGB(100,104,110),RGB(72,76,82));CRect arrow(info.rcButton);dc.FillSolidRect(arrow,GetDroppedState()?RGB(65,69,76):RGB(49,52,58));const auto color=IsWindowEnabled()?uiForeground():RGB(145,145,145);CBrush brush(color);CPen pen(PS_SOLID,1,color);auto oldBrush=dc.SelectObject(&brush);auto oldPen=dc.SelectObject(&pen);const int radius=std::max(2,MulDiv(3,GetDpiForWindow(m_hWnd),96)),x=arrow.CenterPoint().x,y=arrow.CenterPoint().y;POINT points[]={{x-radius,y-radius/2},{x+radius,y-radius/2},{x,y+radius}};dc.Polygon(points,3);dc.SelectObject(oldBrush);dc.SelectObject(oldPen);
  if((GetStyle()&3)==CBS_DROPDOWNLIST){CString label;int selected=GetCurSel();if(selected>=0)GetLBText(selected,label);CRect text(info.rcItem);text.DeflateRect(3,0);dc.SetBkMode(TRANSPARENT);dc.SetTextColor(color);auto font=dc.SelectObject(GetFont());dc.DrawText(label,text,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);dc.SelectObject(font);if(::GetFocus()==m_hWnd){text.InflateRect(1,0);dc.DrawFocusRect(text);}}
 }
 afx_msg void OnPaint(){if(!uiDark){Default();return;}CPaintDC dc(this);paint(dc);}
 afx_msg LRESULT OnPrintClient(WPARAM target,LPARAM){if(!uiDark)return Default();if(target){auto dc=CDC::FromHandle((HDC)target);const int saved=dc->SaveDC();paint(*dc);dc->RestoreDC(saved);}return 0;}
 LRESULT WindowProc(UINT message,WPARAM w,LPARAM l)override{if(uiDark&&(message==WM_CTLCOLOREDIT||message==WM_CTLCOLORLISTBOX||message==WM_CTLCOLORSTATIC)){SetTextColor((HDC)w,uiForeground());SetBkColor((HDC)w,uiBackground());return (LRESULT)uiBrush().GetSafeHandle();}auto result=CComboBox::WindowProc(message,w,l);if(message==CB_SETCURSEL||message==WM_SETFOCUS||message==WM_KILLFOCUS||message==WM_ENABLE||message==CB_SHOWDROPDOWN)Invalidate();return result;}
};
BEGIN_MESSAGE_MAP(ThemeCombo,CComboBox)
 ON_WM_PAINT()
 ON_MESSAGE(WM_PRINTCLIENT,OnPrintClient)
END_MESSAGE_MAP()
class ThemeSpin:public CSpinButtonCtrl {
 DECLARE_MESSAGE_MAP()
 void paint(CDC& dc){CRect area;GetClientRect(&area);dc.FillSolidRect(area,RGB(49,52,58));const bool horizontal=(GetStyle()&UDS_HORZ)!=0;auto color=IsWindowEnabled()?uiForeground():RGB(145,145,145);CBrush brush(color);CPen pen(PS_SOLID,1,color);auto oldBrush=dc.SelectObject(&brush);auto oldPen=dc.SelectObject(&pen);for(int i=0;i<2;++i){CRect half=area;if(horizontal){half.left=i?area.CenterPoint().x:area.left;half.right=i?area.right:area.CenterPoint().x;}else{half.top=i?area.CenterPoint().y:area.top;half.bottom=i?area.bottom:area.CenterPoint().y;}dc.Draw3dRect(half,RGB(100,104,110),RGB(72,76,82));int x=half.CenterPoint().x,y=half.CenterPoint().y,r=std::max(2,MulDiv(2,GetDpiForWindow(m_hWnd),96));POINT points[3];if(horizontal){int direction=i?1:-1;points[0]={x+direction*r,y};points[1]={x-direction*r,y-r};points[2]={x-direction*r,y+r};}else{int direction=i?1:-1;points[0]={x,y+direction*r};points[1]={x-r,y-direction*r};points[2]={x+r,y-direction*r};}dc.Polygon(points,3);}dc.SelectObject(oldBrush);dc.SelectObject(oldPen);}
 afx_msg void OnPaint(){if(!uiDark){Default();return;}CPaintDC dc(this);paint(dc);}
 afx_msg LRESULT OnPrintClient(WPARAM target,LPARAM){if(!uiDark)return Default();if(target){auto dc=CDC::FromHandle((HDC)target);const int saved=dc->SaveDC();paint(*dc);dc->RestoreDC(saved);}return 0;}
};
BEGIN_MESSAGE_MAP(ThemeSpin,CSpinButtonCtrl)
 ON_WM_PAINT()
 ON_MESSAGE(WM_PRINTCLIENT,OnPrintClient)
END_MESSAGE_MAP()
// Retain native date editing and selection; adapt only the monochrome client palette.
class ThemeDateTime:public CDateTimeCtrl {
 DECLARE_MESSAGE_MAP()
 void paint(CDC& target){CRect area;GetClientRect(&area);if(area.IsRectEmpty())return;CImage image;image.Create(area.Width(),area.Height(),32);auto dc=image.GetDC();FillRect(dc,&area,GetSysColorBrush(COLOR_WINDOW));DefWindowProc(WM_PRINTCLIENT,(WPARAM)dc,PRF_CLIENT);image.ReleaseDC();const auto selection=GetSysColor(COLOR_HIGHLIGHT);
  for(int y=0;y<area.Height();++y){int left=area.Width(),right=-1;for(int x=0;x<area.Width();++x){auto p=(BYTE*)image.GetPixelAddress(x,y);if(p[2]==GetRValue(selection)&&p[1]==GetGValue(selection)&&p[0]==GetBValue(selection)){left=std::min(left,x);right=x;}}
   for(int x=0;x<area.Width();++x){auto p=(BYTE*)image.GetPixelAddress(x,y);if(x>=left&&x<=right)continue;int brightness=(p[0]+p[1]+p[2])/3;auto mapped=(BYTE)(232-brightness*200/255);p[0]=mapped;p[1]=mapped;p[2]=mapped;}
  }image.BitBlt(target.GetSafeHdc(),0,0);
 }
 afx_msg void OnPaint(){if(!uiDark){Default();return;}CPaintDC dc(this);paint(dc);}
 afx_msg LRESULT OnPrintClient(WPARAM target,LPARAM){if(!uiDark)return Default();if(target)paint(*CDC::FromHandle((HDC)target));return 0;}
 LRESULT WindowProc(UINT message,WPARAM w,LPARAM l)override{auto result=CDateTimeCtrl::WindowProc(message,w,l);if(message==DTM_SETSYSTEMTIME||message==DTM_SETFORMATW||message==WM_SETFOCUS||message==WM_KILLFOCUS||message==WM_ENABLE||message==WM_KEYDOWN||message==WM_LBUTTONUP)Invalidate();return result;}
};
BEGIN_MESSAGE_MAP(ThemeDateTime,CDateTimeCtrl)
 ON_WM_PAINT()
 ON_MESSAGE(WM_PRINTCLIENT,OnPrintClient)
END_MESSAGE_MAP()
class ThemeStatusBar:public CStatusBarCtrl {
 DECLARE_MESSAGE_MAP()
 void paint(CDC& dc){CRect area;GetClientRect(&area);dc.FillSolidRect(area,uiBackground());dc.SetBkMode(TRANSPARENT);dc.SetTextColor(uiForeground());auto old=dc.SelectObject(GetFont());int count=GetParts(0,nullptr);for(int i=0;i<std::max(1,count);++i){CRect part;if(!GetRect(i,&part))part=area;CString label=GetText(i);part.DeflateRect(3,0);dc.DrawText(label,part,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX|DT_END_ELLIPSIS);}dc.SelectObject(old);
  if(GetStyle()&SBARS_SIZEGRIP){CPen pen(PS_SOLID,1,RGB(115,118,123));auto oldPen=dc.SelectObject(&pen);for(int offset:{4,8,12}){dc.MoveTo(area.right-offset,area.bottom-2);dc.LineTo(area.right-2,area.bottom-offset);}dc.SelectObject(oldPen);}
 }
 afx_msg void OnPaint(){if(!uiDark){Default();return;}CPaintDC dc(this);paint(dc);}
 afx_msg LRESULT OnPrintClient(WPARAM target,LPARAM){if(!uiDark)return Default();if(target){auto dc=CDC::FromHandle((HDC)target);int saved=dc->SaveDC();paint(*dc);dc->RestoreDC(saved);}return 0;}
};
BEGIN_MESSAGE_MAP(ThemeStatusBar,CStatusBarCtrl)
 ON_WM_PAINT()
 ON_MESSAGE(WM_PRINTCLIENT,OnPrintClient)
END_MESSAGE_MAP()
// Original native painting for common controls whose default theme stays light.
class ThemeTabs:public CTabCtrl {
 DECLARE_MESSAGE_MAP()
 afx_msg void OnPaint(){if(!uiDark){Default();return;}CPaintDC dc(this);paintDark(dc);}
 afx_msg LRESULT OnPrintClient(WPARAM target,LPARAM){if(!uiDark)return Default();if(target){auto dc=CDC::FromHandle((HDC)target);const auto saved=dc->SaveDC();paintDark(*dc);dc->RestoreDC(saved);}return 0;}
 void paintDark(CDC& dc){CRect area;GetClientRect(&area);dc.FillSolidRect(area,uiBackground());dc.SetBkMode(TRANSPARENT);auto old=dc.SelectObject(GetFont());for(int i=0;i<GetItemCount();++i){CRect r;GetItemRect(i,&r);dc.FillSolidRect(r,i==GetCurSel()?uiBackground():RGB(44,47,52));dc.Draw3dRect(r,RGB(99,103,110),RGB(64,67,72));wchar_t caption[256]{};TCITEMW item{};item.mask=TCIF_TEXT;item.pszText=caption;item.cchTextMax=256;GetItem(i,&item);dc.SetTextColor(uiForeground());dc.DrawText(caption,r,DT_CENTER|DT_VCENTER|DT_SINGLELINE);if(i==GetCurSel()&&GetFocus()==this){r.DeflateRect(3,3);dc.DrawFocusRect(r);}}dc.SelectObject(old);CRect body=area;AdjustRect(FALSE,&body);body.InflateRect(2,2);dc.Draw3dRect(body,RGB(99,103,110),RGB(64,67,72));}
};
BEGIN_MESSAGE_MAP(ThemeTabs,CTabCtrl)
 ON_WM_PAINT()
 ON_MESSAGE(WM_PRINTCLIENT,OnPrintClient)
END_MESSAGE_MAP()
class ThemeHeader:public CHeaderCtrl {
 DECLARE_MESSAGE_MAP()
 afx_msg void OnPaint(){if(!uiDark){Default();return;}CPaintDC dc(this);paintDark(dc);}
 afx_msg LRESULT OnPrintClient(WPARAM target,LPARAM){if(!uiDark)return Default();if(target){auto dc=CDC::FromHandle((HDC)target);const auto saved=dc->SaveDC();paintDark(*dc);dc->RestoreDC(saved);}return 0;}
 void paintDark(CDC& dc){
  CRect area;GetClientRect(&area);dc.FillSolidRect(area,RGB(43,46,51));dc.SetBkMode(TRANSPARENT);dc.SetTextColor(uiForeground());auto old=dc.SelectObject(GetFont());const int dpi=(int)GetDpiForWindow(m_hWnd),radius=std::max(3,MulDiv(3,dpi,96)),height=std::max(2,MulDiv(2,dpi,96));
  for(int i=0;i<GetItemCount();++i){
   CRect r;GetItemRect(i,&r);if(r.Width()<=0)continue;const int saved=dc.SaveDC();dc.IntersectClipRect(r);
   dc.FillSolidRect(r.right-1,r.top,1,r.Height(),RGB(74,78,85));wchar_t caption[256]{};HDITEMW item{};item.mask=HDI_TEXT|HDI_FORMAT;item.pszText=caption;item.cchTextMax=256;GetItem(i,&item);r.DeflateRect(5,0);
   if((item.fmt&(HDF_SORTUP|HDF_SORTDOWN))&&r.Width()>radius*2+6){
    const int x=r.CenterPoint().x,y=r.top+height;const bool up=(item.fmt&HDF_SORTUP)!=0;
    CPen pen(PS_SOLID,std::max(1,MulDiv(1,dpi,96)),uiForeground());auto previousPen=dc.SelectObject(&pen);
    dc.MoveTo(x-radius,y+(up?height:0));dc.LineTo(x,y+(up?0:height));dc.LineTo(x+radius,y+(up?height:0));dc.SelectObject(previousPen);
   }
   dc.DrawText(caption,r,DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX|(item.fmt&HDF_RIGHT?DT_RIGHT:item.fmt&HDF_CENTER?DT_CENTER:DT_LEFT));dc.RestoreDC(saved);
  }
  dc.SelectObject(old);
 }
};
BEGIN_MESSAGE_MAP(ThemeHeader,CHeaderCtrl)
 ON_WM_PAINT()
 ON_MESSAGE(WM_PRINTCLIENT,OnPrintClient)
END_MESSAGE_MAP()
class ThemeList:public CListCtrl {
 DECLARE_MESSAGE_MAP()
 ThemeHeader header;
 bool darkGrid=false;
 afx_msg void OnCustomDraw(NMHDR* notification,LRESULT* result){
  auto draw=reinterpret_cast<NMLVCUSTOMDRAW*>(notification);*result=CDRF_DODEFAULT;
  if(!uiDark||!darkGrid)return;
  if(draw->nmcd.dwDrawStage==CDDS_PREPAINT){*result=CDRF_NOTIFYPOSTPAINT;return;}
  if(draw->nmcd.dwDrawStage!=CDDS_POSTPAINT)return;
  auto dc=CDC::FromHandle(draw->nmcd.hdc);const int saved=dc->SaveDC();
  CRect area;GetClientRect(&area);auto columns=GetHeaderCtrl();
  if(columns&&columns->IsWindowVisible()){CRect bounds;columns->GetWindowRect(&bounds);ScreenToClient(&bounds);area.top=std::max(area.top,bounds.bottom);}
  dc->IntersectClipRect(area);CPen pen(PS_SOLID,1,RGB(64,67,72));auto old=dc->SelectObject(&pen);
  if(columns)for(int i=0;i<columns->GetItemCount();++i){CRect column;columns->GetItemRect(i,&column);if(column.Width()<=0)continue;columns->MapWindowPoints(this,&column);const int x=column.right-1;dc->MoveTo(x,area.top);dc->LineTo(x,area.bottom);}
  const int first=GetTopIndex(),last=std::min(GetItemCount(),first+GetCountPerPage()+2);
  for(int i=first;i<last;++i){CRect row;if(GetItemRect(i,&row,LVIR_BOUNDS)){const int y=row.bottom-1;dc->MoveTo(area.left,y);dc->LineTo(area.right,y);}}
  dc->SelectObject(old);dc->RestoreDC(saved);
 }
public:
 void sortIndicator(int column,bool ascending){
  if(!GetSafeHwnd())return;auto control=GetHeaderCtrl();if(!control)return;
  for(int i=0;i<control->GetItemCount();++i){HDITEMW item{};item.mask=HDI_FORMAT;if(!control->GetItem(i,&item))continue;int format=(item.fmt&~(HDF_SORTUP|HDF_SORTDOWN))|(i==column?(ascending?HDF_SORTUP:HDF_SORTDOWN):0);if(format!=item.fmt){item.fmt=format;control->SetItem(i,&item);}}
 }
 void theme(){
  themeScrollbars(m_hWnd);
  const auto styles=GetExtendedStyle();
  if(uiDark){darkGrid=darkGrid||(styles&LVS_EX_GRIDLINES)!=0;if(styles&LVS_EX_GRIDLINES)SetExtendedStyle(styles&~LVS_EX_GRIDLINES);}
  else if(darkGrid){SetExtendedStyle(styles|LVS_EX_GRIDLINES);darkGrid=false;}
  SetBkColor(uiBackground());SetTextBkColor(uiBackground());SetTextColor(uiForeground());if(!header.GetSafeHwnd()&&GetHeaderCtrl())header.SubclassWindow(GetHeaderCtrl()->GetSafeHwnd());if(header.GetSafeHwnd())header.Invalidate();}
};

BEGIN_MESSAGE_MAP(ThemeList,CListCtrl)
 ON_NOTIFY_REFLECT(NM_CUSTOMDRAW,OnCustomDraw)
END_MESSAGE_MAP()

// Keep an explicit accessible caption while painting the compact panel-close glyph.
class PanelCloseButton:public CButton {
 void DrawItem(LPDRAWITEMSTRUCT item)override{
  CDC dc;dc.Attach(item->hDC);CRect r(item->rcItem);dc.FillSolidRect(r,uiBackground());
  const bool pressed=(item->itemState&ODS_SELECTED)!=0;
  if(pressed)dc.Draw3dRect(r,uiForeground(),uiForeground());
  const int pad=std::max(4,r.Width()/3),offset=pressed?1:0;CRect glyph=r;glyph.DeflateRect(pad,pad);glyph.OffsetRect(offset,offset);
  CPen pen(PS_SOLID,std::max(1,r.Width()/15),uiForeground());auto old=dc.SelectObject(&pen);
  dc.MoveTo(glyph.left,glyph.top);dc.LineTo(glyph.right,glyph.bottom);dc.MoveTo(glyph.right-1,glyph.top);dc.LineTo(glyph.left-1,glyph.bottom);
  dc.SelectObject(old);if(item->itemState&ODS_FOCUS){r.DeflateRect(2,2);dc.DrawFocusRect(r);}dc.Detach();
 }
};
// Reference-style static hyperlink, retaining focus and keyboard activation.
// A standalone checkbox glyph gets its visible text from a separate label.
// Keep its accessible name without asking the native button to paint that text.
class GlyphCheckBox:public CButton {
 CString accessibleName;
public:
 explicit GlyphCheckBox(const CString& name):accessibleName(name){EnableActiveAccessibility();}
 HRESULT get_accName(VARIANT child,BSTR* value)override{
  if(child.vt==VT_I4&&child.lVal==CHILDID_SELF){if(!value)return E_POINTER;*value=accessibleName.AllocSysString();return *value?S_OK:E_OUTOFMEMORY;}
  return CButton::get_accName(child,value);
 }
};
class StaticHyperlink:public CStatic {
 DECLARE_MESSAGE_MAP()
 void DrawItem(LPDRAWITEMSTRUCT item)override{CDC dc;dc.Attach(item->hDC);CRect r=item->rcItem;dc.FillSolidRect(r,uiDark?uiBackground():GetSysColor(COLOR_BTNFACE));CString value;GetWindowText(value);dc.SetBkMode(TRANSPARENT);dc.SetTextColor(IsWindowEnabled()?(uiDark?RGB(125,186,240):GetSysColor(COLOR_HOTLIGHT)):GetSysColor(COLOR_GRAYTEXT));auto old=dc.SelectObject(GetFont());dc.DrawText(value,r,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);dc.SelectObject(old);if(::GetFocus()==m_hWnd)dc.DrawFocusRect(r);dc.Detach();}
 afx_msg BOOL OnSetCursor(CWnd*,UINT,UINT){if(!IsWindowEnabled())return FALSE;::SetCursor(::LoadCursor(nullptr,IDC_HAND));return TRUE;}
 afx_msg void OnSetFocus(CWnd* old){CStatic::OnSetFocus(old);Invalidate();}
 afx_msg void OnKillFocus(CWnd* next){CStatic::OnKillFocus(next);Invalidate();}
public:
 StaticHyperlink(){EnableActiveAccessibility();}
 HRESULT get_accRole(VARIANT child,VARIANT* value)override{
  if(child.vt!=VT_I4||child.lVal!=CHILDID_SELF)return CStatic::get_accRole(child,value);
  if(!value)return E_POINTER;VariantInit(value);value->vt=VT_I4;value->lVal=ROLE_SYSTEM_LINK;return S_OK;
 }
 HRESULT get_accState(VARIANT child,VARIANT* value)override{
  auto result=CStatic::get_accState(child,value);
  if(SUCCEEDED(result)&&value&&value->vt==VT_I4&&child.vt==VT_I4&&child.lVal==CHILDID_SELF){value->lVal|=STATE_SYSTEM_LINKED|STATE_SYSTEM_FOCUSABLE;value->lVal&=~(STATE_SYSTEM_UNAVAILABLE|STATE_SYSTEM_FOCUSED);if(!IsWindowEnabled())value->lVal|=STATE_SYSTEM_UNAVAILABLE;if(::GetFocus()==m_hWnd)value->lVal|=STATE_SYSTEM_FOCUSED;}
  return result;
 }
 HRESULT get_accDefaultAction(VARIANT child,BSTR* value)override{
  if(child.vt!=VT_I4||child.lVal!=CHILDID_SELF)return CStatic::get_accDefaultAction(child,value);
  if(!value)return E_POINTER;*value=SysAllocString(L"Open");return *value?S_OK:E_OUTOFMEMORY;
 }
 HRESULT accDoDefaultAction(VARIANT child)override{
  if(child.vt!=VT_I4||child.lVal!=CHILDID_SELF)return CStatic::accDoDefaultAction(child);
  if(!GetSafeHwnd()||!IsWindowEnabled())return E_FAIL;
  auto parent=GetParent();if(!parent)return E_FAIL;
  parent->SendMessage(WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(),STN_CLICKED),(LPARAM)m_hWnd);return S_OK;
 }
 BOOL PreTranslateMessage(MSG* message)override{if(IsWindowEnabled()&&(message->wParam==VK_RETURN||message->wParam==VK_SPACE)&&(message->message==WM_KEYDOWN||message->message==WM_KEYUP)){if(message->message==WM_KEYDOWN&&!(message->lParam&(1L<<30)))GetParent()->SendMessage(WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(),STN_CLICKED),(LPARAM)m_hWnd);return TRUE;}return CStatic::PreTranslateMessage(message);}
};
BEGIN_MESSAGE_MAP(StaticHyperlink,CStatic)
 ON_WM_SETCURSOR()
 ON_WM_SETFOCUS()
 ON_WM_KILLFOCUS()
END_MESSAGE_MAP()
