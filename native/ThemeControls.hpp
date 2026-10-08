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
 void paintDark(CDC& dc){CRect area;GetClientRect(&area);dc.FillSolidRect(area,RGB(43,46,51));dc.SetBkMode(TRANSPARENT);dc.SetTextColor(uiForeground());auto old=dc.SelectObject(GetFont());for(int i=0;i<GetItemCount();++i){CRect r;GetItemRect(i,&r);if(r.Width()<=0)continue;dc.FillSolidRect(r.right-1,r.top,1,r.Height(),RGB(74,78,85));wchar_t caption[256]{};HDITEMW item{};item.mask=HDI_TEXT|HDI_FORMAT;item.pszText=caption;item.cchTextMax=256;GetItem(i,&item);r.DeflateRect(5,0);dc.DrawText(caption,r,DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS|(item.fmt&HDF_RIGHT?DT_RIGHT:item.fmt&HDF_CENTER?DT_CENTER:DT_LEFT));}dc.SelectObject(old);}
};
BEGIN_MESSAGE_MAP(ThemeHeader,CHeaderCtrl)
 ON_WM_PAINT()
 ON_MESSAGE(WM_PRINTCLIENT,OnPrintClient)
END_MESSAGE_MAP()
class ThemeList:public CListCtrl {
 ThemeHeader header;
public:
 void theme(){SetBkColor(uiBackground());SetTextBkColor(uiBackground());SetTextColor(uiForeground());if(!header.GetSafeHwnd()&&GetHeaderCtrl())header.SubclassWindow(GetHeaderCtrl()->GetSafeHwnd());if(header.GetSafeHwnd())header.Invalidate();}
};

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
 BOOL PreTranslateMessage(MSG* message)override{if(IsWindowEnabled()&&(message->wParam==VK_RETURN||message->wParam==VK_SPACE)&&(message->message==WM_KEYDOWN||message->message==WM_KEYUP)){if(message->message==WM_KEYDOWN&&!(message->lParam&(1L<<30)))GetParent()->SendMessage(WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(),STN_CLICKED),(LPARAM)m_hWnd);return TRUE;}return CStatic::PreTranslateMessage(message);}
};
BEGIN_MESSAGE_MAP(StaticHyperlink,CStatic)
 ON_WM_SETCURSOR()
 ON_WM_SETFOCUS()
 ON_WM_KILLFOCUS()
END_MESSAGE_MAP()
