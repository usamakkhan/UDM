// Original native painting for common controls whose default theme stays light.
class ThemeTabs:public CTabCtrl {
 DECLARE_MESSAGE_MAP()
 afx_msg void OnPaint(){if(!uiDark){Default();return;}CPaintDC dc(this);CRect area;GetClientRect(&area);dc.FillSolidRect(area,uiBackground());dc.SetBkMode(TRANSPARENT);auto old=dc.SelectObject(GetFont());for(int i=0;i<GetItemCount();++i){CRect r;GetItemRect(i,&r);dc.FillSolidRect(r,i==GetCurSel()?uiBackground():RGB(44,47,52));dc.Draw3dRect(r,RGB(99,103,110),RGB(64,67,72));wchar_t caption[256]{};TCITEMW item{};item.mask=TCIF_TEXT;item.pszText=caption;item.cchTextMax=256;GetItem(i,&item);dc.SetTextColor(uiForeground());dc.DrawText(caption,r,DT_CENTER|DT_VCENTER|DT_SINGLELINE);if(i==GetCurSel()&&GetFocus()==this){r.DeflateRect(3,3);dc.DrawFocusRect(r);}}dc.SelectObject(old);CRect body=area;AdjustRect(FALSE,&body);body.InflateRect(2,2);dc.Draw3dRect(body,RGB(99,103,110),RGB(64,67,72));}
};
BEGIN_MESSAGE_MAP(ThemeTabs,CTabCtrl)
 ON_WM_PAINT()
END_MESSAGE_MAP()
class ThemeHeader:public CHeaderCtrl {
 DECLARE_MESSAGE_MAP()
 afx_msg void OnPaint(){if(!uiDark){Default();return;}CPaintDC dc(this);CRect area;GetClientRect(&area);dc.FillSolidRect(area,RGB(43,46,51));dc.SetBkMode(TRANSPARENT);dc.SetTextColor(uiForeground());auto old=dc.SelectObject(GetFont());for(int i=0;i<GetItemCount();++i){CRect r;GetItemRect(i,&r);if(r.Width()<=0)continue;dc.FillSolidRect(r.right-1,r.top,1,r.Height(),RGB(74,78,85));wchar_t caption[256]{};HDITEMW item{};item.mask=HDI_TEXT|HDI_FORMAT;item.pszText=caption;item.cchTextMax=256;GetItem(i,&item);r.DeflateRect(5,0);dc.DrawText(caption,r,DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS|(item.fmt&HDF_RIGHT?DT_RIGHT:item.fmt&HDF_CENTER?DT_CENTER:DT_LEFT));}dc.SelectObject(old);}
};
BEGIN_MESSAGE_MAP(ThemeHeader,CHeaderCtrl)
 ON_WM_PAINT()
END_MESSAGE_MAP()
class ThemeList:public CListCtrl {
 ThemeHeader header;
public:
 void theme(){SetBkColor(uiBackground());SetTextBkColor(uiBackground());SetTextColor(uiForeground());if(!header.GetSafeHwnd()&&GetHeaderCtrl())header.SubclassWindow(GetHeaderCtrl()->GetSafeHwnd());if(header.GetSafeHwnd())header.Invalidate();}
};
