// Included inside udm, after Form.
inline DROPEFFECT dragSavedFiles(const std::vector<fs::path>& paths){
 std::wstring names;for(auto& path:paths)if(fs::is_regular_file(path)){names+=fs::absolute(path).wstring();names.push_back(0);}if(names.empty())return DROPEFFECT_NONE;names.push_back(0);auto size=sizeof(DROPFILES)+names.size()*sizeof(wchar_t);HGLOBAL memory=GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT,size);if(!memory)throw std::runtime_error("Cannot prepare the file drag.");auto data=(DROPFILES*)GlobalLock(memory);if(!data){GlobalFree(memory);throw std::runtime_error("Cannot prepare the file drag.");}data->pFiles=sizeof(DROPFILES);data->fWide=TRUE;memcpy((char*)data+sizeof(DROPFILES),names.data(),names.size()*sizeof(wchar_t));GlobalUnlock(memory);COleDataSource source;source.CacheGlobalData(CF_HDROP,memory);return source.DoDragDrop(DROPEFFECT_COPY|DROPEFFECT_LINK);
}
class FileDragIcon:public CStatic {
 DECLARE_MESSAGE_MAP()
 afx_msg void OnPaint(){CPaintDC dc(this);CRect r;GetClientRect(&r);dc.FillSolidRect(r,uiDark?uiBackground():GetSysColor(COLOR_BTNFACE));DrawIconEx(dc.m_hDC,0,0,AfxGetApp()->LoadIcon(1),r.Width(),r.Height(),0,nullptr,DI_NORMAL);}
 afx_msg void OnLButtonDown(UINT flags,CPoint point){CStatic::OnLButtonDown(flags,point);if(::DragDetect(m_hWnd,point)){try{dragSavedFiles({path});}catch(const std::exception& e){error(this,e);}}}
 afx_msg BOOL OnSetCursor(CWnd*,UINT,UINT){::SetCursor(::LoadCursor(nullptr,IDC_HAND));return TRUE;}
public:fs::path path;BOOL Create(DWORD style,const RECT& rect,CWnd* owner,UINT id){return CStatic::Create(L"Drag downloaded file",style|SS_ICON|SS_NOTIFY,rect,owner,id);}
};
BEGIN_MESSAGE_MAP(FileDragIcon,CStatic)
 ON_WM_PAINT()
 ON_WM_LBUTTONDOWN()
 ON_WM_SETCURSOR()
END_MESSAGE_MAP()
class FileDragList:public CListCtrl {
 DECLARE_MESSAGE_MAP()
 CImageList icons;
 afx_msg void OnBeginDrag(NMHDR*,LRESULT* result){*result=0;try{dragSavedFiles({path});}catch(const std::exception& e){error(this,e);}}
public:
 fs::path path;
 BOOL Create(DWORD style,const RECT& rect,CWnd* owner,UINT id){
  if(!CListCtrl::Create(style|WS_BORDER|LVS_SMALLICON|LVS_NOSCROLL|LVS_NOCOLUMNHEADER,rect,owner,id))return FALSE;
  icons.Create(16,16,ILC_COLOR32|ILC_MASK,1,1);icons.Add(AfxGetApp()->LoadIcon(1));SetImageList(&icons,LVSIL_SMALL);SetBkColor(uiDark?uiBackground():GetSysColor(COLOR_BTNFACE));SetTextBkColor(CLR_NONE);SetTextColor(uiForeground());InsertItem(0,L"",0);return TRUE;
 }
 void setPath(const fs::path& value){path=value;SetItemText(0,0,cs(path.filename().string()));}
};
BEGIN_MESSAGE_MAP(FileDragList,CListCtrl)
 ON_NOTIFY_REFLECT(LVN_BEGINDRAG,OnBeginDrag)
END_MESSAGE_MAP()
