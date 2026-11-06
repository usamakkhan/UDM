// Included after DropTarget, inside udm.
class Basket:public CWnd {
 DECLARE_MESSAGE_MAP()
 MenuBarTheme popupTheme;
 LRESULT WindowProc(UINT message,WPARAM w,LPARAM l)override{if(message==WM_MEASUREITEM&&popupTheme.measure((MEASUREITEMSTRUCT*)l))return TRUE;if(message==WM_DRAWITEM&&popupTheme.draw((DRAWITEMSTRUCT*)l))return TRUE;if(message==WM_MENUCHAR){LRESULT result=0;if(popupTheme.mnemonic((HMENU)l,LOWORD(w),result))return result;}return CWnd::WindowProc(message,w,l);}
 CWnd* owner;DropTarget target;
 void place(int x,int y,UINT dpi){
  const int width=MulDiv(59,dpi,120),height=MulDiv(70,dpi,120);
  MONITORINFO monitor{sizeof(monitor)};
  if(GetMonitorInfoW(MonitorFromPoint(POINT{x,y},MONITOR_DEFAULTTONEAREST),&monitor)){
   const auto& work=monitor.rcWork;
   x=std::clamp(x,(int)work.left,std::max((int)work.left,(int)work.right-width));
   y=std::clamp(y,(int)work.top,std::max((int)work.top,(int)work.bottom-height));
  }
  SetWindowPos(nullptr,x,y,width,height,SWP_NOZORDER|SWP_NOACTIVATE);
 }
 afx_msg LRESULT OnDpiChanged(WPARAM value,LPARAM position){
  if(HIWORD(value)&&position){const auto& r=*reinterpret_cast<const RECT*>(position);place(r.left,r.top,HIWORD(value));Invalidate();}
  return 0;
 }
 afx_msg void OnPaint(){
  CPaintDC dc(this);CRect area;GetClientRect(&area);
  dc.FillSolidRect(area,uiDark?uiBackground():GetSysColor(COLOR_WINDOW));
  const int icon=std::min({area.Width(),area.Height(),MulDiv(35,GetDpiForWindow(m_hWnd),120)});
  if(icon>0)DrawIconEx(dc.GetSafeHdc(),(area.Width()-icon)/2,(area.Height()-icon)/2,AfxGetApp()->LoadIcon(1),icon,icon,0,nullptr,DI_NORMAL);
 }
 afx_msg void OnRButtonUp(UINT,CPoint point){ClientToScreen(&point);OnContextMenu(this,point);}
 afx_msg void OnExitSizeMove(){if(moved){CRect r;GetWindowRect(&r);moved(r.left,r.top);}}
 afx_msg void OnClose(){owner->PostMessage(WM_COMMAND,CMD_HIDE_BASKET);}
 afx_msg void OnLButtonDblClk(UINT,CPoint){owner->PostMessage(SHOW_APP);}
 afx_msg void OnContextMenu(CWnd*,CPoint p){CMenu menu;menu.CreatePopupMenu();menu.AppendMenuW(MF_STRING,1,L"Open UDM");menu.AppendMenuW(MF_STRING,2,L"Add URL...");menu.AppendMenuW(MF_STRING,3,L"Paste download link");menu.AppendMenuW(MF_SEPARATOR);menu.AppendMenuW(MF_STRING,4,L"Hide drop target");if(p==CPoint(-1,-1)){CRect r;GetWindowRect(&r);p=r.BottomRight();}SetForegroundWindow();popupTheme.apply(m_hWnd,nullptr,uiDark,GetDpiForWindow(m_hWnd));popupTheme.popup(menu.GetSafeHmenu(),uiDark);auto choice=menu.TrackPopupMenu(TPM_RETURNCMD|TPM_NONOTIFY|TPM_RIGHTBUTTON,p.x,p.y,this);popupTheme.releasePopup(menu.GetSafeHmenu());if(choice==1)owner->PostMessage(SHOW_APP);if(choice==2)owner->PostMessage(WM_COMMAND,CMD_ADD);if(choice==3)owner->PostMessage(WM_COMMAND,CMD_PASTE);if(choice==4)OnClose();}
public:
 void refreshTheme(){themeFrame(m_hWnd);RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_FRAME);}
 std::function<void(int,int)> moved;
 Basket(CWnd* main):owner(main),target(main){}
 bool open(int x,int y){
  if(!CreateEx(WS_EX_TOOLWINDOW|WS_EX_TOPMOST,AfxRegisterWndClass(CS_DBLCLKS,LoadCursor(nullptr,IDC_ARROW)),
     L"UDM drop target. Drop web-links for downloading here",WS_POPUP|WS_CAPTION|WS_SYSMENU,CRect(x,y,x+1,y+1),owner,0))return false;
  place(x,y,GetDpiForWindow(m_hWnd));refreshTheme();
  if(target.Register(this))return true;
  DestroyWindow();return false;
 }
 ~Basket(){target.Revoke();if(GetSafeHwnd())DestroyWindow();}
};
BEGIN_MESSAGE_MAP(Basket,CWnd)
 ON_WM_RBUTTONUP()
 ON_WM_PAINT()
 ON_MESSAGE(WM_DPICHANGED,OnDpiChanged)
 ON_WM_EXITSIZEMOVE()
 ON_WM_CLOSE()
 ON_WM_LBUTTONDBLCLK()
 ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()
