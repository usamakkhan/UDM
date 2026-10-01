// Included inside udm, after Form.
inline DROPEFFECT dragSavedFiles(const std::vector<fs::path>& paths){
 std::wstring names;for(auto& path:paths)if(fs::is_regular_file(path)){names+=fs::absolute(path).wstring();names.push_back(0);}if(names.empty())return DROPEFFECT_NONE;names.push_back(0);auto size=sizeof(DROPFILES)+names.size()*sizeof(wchar_t);HGLOBAL memory=GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT,size);if(!memory)throw std::runtime_error("Cannot prepare the file drag.");auto data=(DROPFILES*)GlobalLock(memory);if(!data){GlobalFree(memory);throw std::runtime_error("Cannot prepare the file drag.");}data->pFiles=sizeof(DROPFILES);data->fWide=TRUE;memcpy((char*)data+sizeof(DROPFILES),names.data(),names.size()*sizeof(wchar_t));GlobalUnlock(memory);COleDataSource source;source.CacheGlobalData(CF_HDROP,memory);return source.DoDragDrop(DROPEFFECT_COPY|DROPEFFECT_LINK);
}
class FileDragIcon:public CWnd {
 DECLARE_MESSAGE_MAP()
 afx_msg void OnPaint(){CPaintDC dc(this);CRect r;GetClientRect(&r);dc.FillSolidRect(r,uiDark?uiBackground():GetSysColor(COLOR_BTNFACE));DrawIconEx(dc.m_hDC,0,0,AfxGetApp()->LoadIcon(1),r.Width(),r.Height(),0,nullptr,DI_NORMAL);}
 afx_msg void OnLButtonDown(UINT flags,CPoint point){CWnd::OnLButtonDown(flags,point);if(::DragDetect(m_hWnd,point)){try{dragSavedFiles({path});}catch(const std::exception& e){error(this,e);}}}
public:fs::path path;BOOL Create(DWORD style,const RECT& rect,CWnd* owner,UINT id){return CWnd::Create(AfxRegisterWndClass(CS_DBLCLKS,LoadCursor(nullptr,IDC_HAND)),L"Drag downloaded file",style,rect,owner,id);}
};
BEGIN_MESSAGE_MAP(FileDragIcon,CWnd)
 ON_WM_PAINT()
 ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()
