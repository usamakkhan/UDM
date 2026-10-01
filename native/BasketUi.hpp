// Included after DropTarget, inside udm.
class Basket:public CWnd {
 DECLARE_MESSAGE_MAP()
 CWnd* owner;DropTarget target;CFont font;CButton menuButton;float dpi=1;
 afx_msg void OnPaint(){CPaintDC dc(this);CRect r;GetClientRect(&r);dc.FillSolidRect(r,GetSysColor(COLOR_BTNFACE));dc.Draw3dRect(r,RGB(90,144,184),RGB(37,82,125));int n=(int)(32*dpi);DrawIconEx(dc.GetSafeHdc(),(r.Width()-n)/2,(int)(8*dpi),AfxGetApp()->LoadIcon(1),n,n,0,nullptr,DI_NORMAL);dc.SetBkMode(TRANSPARENT);dc.SetTextColor(GetSysColor(COLOR_BTNTEXT));auto old=dc.SelectObject(&font);r.top=(int)(45*dpi);dc.DrawText(L"UDM",r,DT_CENTER|DT_SINGLELINE);dc.SelectObject(old);}
 afx_msg void OnRButtonUp(UINT,CPoint point){ClientToScreen(&point);OnContextMenu(this,point);}
 BOOL OnCommand(WPARAM w,LPARAM l)override{if(LOWORD(w)==601){CRect r;GetWindowRect(&r);OnContextMenu(this,r.BottomRight());return TRUE;}return CWnd::OnCommand(w,l);}
 afx_msg void OnLButtonDown(UINT,CPoint){SendMessage(WM_NCLBUTTONDOWN,HTCAPTION,0);if(moved){CRect r;GetWindowRect(&r);moved(r.left,r.top);}}
 afx_msg void OnLButtonDblClk(UINT,CPoint){owner->PostMessage(SHOW_APP);}
 afx_msg void OnContextMenu(CWnd*,CPoint p){CMenu menu;menu.CreatePopupMenu();menu.AppendMenuW(MF_STRING,1,L"Open UDM");menu.AppendMenuW(MF_STRING,2,L"Add URL...");menu.AppendMenuW(MF_STRING,3,L"Paste download link");menu.AppendMenuW(MF_SEPARATOR);menu.AppendMenuW(MF_STRING,4,L"Hide drop basket");if(p.x<0){CRect r;GetWindowRect(&r);p=r.BottomRight();}owner->SetForegroundWindow();auto choice=menu.TrackPopupMenu(TPM_RETURNCMD|TPM_NONOTIFY|TPM_RIGHTBUTTON,p.x,p.y,owner);if(choice==1)owner->PostMessage(SHOW_APP);if(choice==2)owner->PostMessage(WM_COMMAND,CMD_ADD);if(choice==3)owner->PostMessage(WM_COMMAND,CMD_PASTE);if(choice==4)owner->PostMessage(WM_COMMAND,CMD_BASKET);}
public:
 std::function<void(int,int)> moved;
 Basket(CWnd* main):owner(main),target(main){}
 bool open(int x,int y){dpi=GetDpiForWindow(owner->GetSafeHwnd())/96.0f;int size=(int)(68*dpi);RECT work{};SystemParametersInfoW(SPI_GETWORKAREA,0,&work,0);POINT location{x,y};HMONITOR monitor=MonitorFromPoint(location,MONITOR_DEFAULTTONEAREST);MONITORINFO mi{sizeof(mi)};if(GetMonitorInfoW(monitor,&mi))work=mi.rcWork;x=std::clamp(x,(int)work.left,(int)work.right-size);y=std::clamp(y,(int)work.top,(int)work.bottom-size);if(!CreateEx(WS_EX_TOOLWINDOW|WS_EX_TOPMOST,AfxRegisterWndClass(CS_DBLCLKS,LoadCursor(nullptr,IDC_ARROW)),L"UDM Drop Basket",WS_POPUP|WS_VISIBLE,CRect(x,y,x+size,y+size),owner,0))return false;font.CreateFontW(-(int)(11*dpi),0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,L"Tahoma");menuButton.Create(L"≡",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON,CRect(size-(int)(17*dpi),(int)(2*dpi),size-(int)(2*dpi),(int)(17*dpi)),this,601);menuButton.SetFont(&font);target.Register(this);return true;}
 ~Basket(){target.Revoke();if(GetSafeHwnd())DestroyWindow();}
};
BEGIN_MESSAGE_MAP(Basket,CWnd)
 ON_WM_RBUTTONUP()
 ON_WM_PAINT()
 ON_WM_LBUTTONDOWN()
 ON_WM_LBUTTONDBLCLK()
 ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()
