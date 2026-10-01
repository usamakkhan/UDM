#pragma once
inline CLIPFORMAT queueDragFormat(){static auto format=(CLIPFORMAT)RegisterClipboardFormatW(L"UDM.DownloadQueue.1");return format;}
inline void dragQueueJob(const std::string& id){COleDataSource source;HGLOBAL block=GlobalAlloc(GMEM_MOVEABLE,id.size()+1);if(!block)throw std::bad_alloc();void* p=GlobalLock(block);if(!p){GlobalFree(block);throw std::bad_alloc();}memcpy(p,id.c_str(),id.size()+1);GlobalUnlock(block);source.CacheGlobalData(queueDragFormat(),block);source.DoDragDrop(DROPEFFECT_MOVE);}
class QueueDropTarget:public COleDropTarget {
 CWnd* owner=nullptr;
public:
 std::function<void(const std::string&,CPoint)> apply;
 void attach(CWnd* window){owner=window;Register(window);}
 DROPEFFECT OnDragEnter(CWnd*,COleDataObject* data,DWORD,CPoint)override{return data->IsDataAvailable(queueDragFormat())?DROPEFFECT_MOVE:DROPEFFECT_NONE;}
 DROPEFFECT OnDragOver(CWnd*,COleDataObject* data,DWORD,CPoint)override{return data->IsDataAvailable(queueDragFormat())?DROPEFFECT_MOVE:DROPEFFECT_NONE;}
 BOOL OnDrop(CWnd*,COleDataObject* data,DROPEFFECT,CPoint point)override{auto h=data->GetGlobalData(queueDragFormat());if(!h)return FALSE;auto p=(const char*)GlobalLock(h);std::string id;if(p){auto n=strnlen_s(p,GlobalSize(h));if(n>0&&n<100)id.assign(p,n);GlobalUnlock(h);}GlobalFree(h);if(id.empty()||!apply)return FALSE;try{apply(id,point);return TRUE;}catch(const std::exception& e){error(owner,e);return FALSE;}}
};
