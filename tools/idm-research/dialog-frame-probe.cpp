#include <windows.h>
#include <commctrl.h>
#include <cstdio>
#include <vector>
#include <cstring>
static INT_PTR CALLBACK dialogProc(HWND,UINT,WPARAM,LPARAM){return FALSE;}
static void word(std::vector<BYTE>& b,WORD value){b.push_back((BYTE)value);b.push_back((BYTE)(value>>8));}
static void string(std::vector<BYTE>& b,const wchar_t* s){do{word(b,*s);}while(*s++);}
static void report(HWND w,const char* phase){
 RECT outer{},client{};GetWindowRect(w,&outer);GetClientRect(w,&client);POINT origin{};ClientToScreen(w,&origin);
 RECT computed=client;AdjustWindowRectExForDpi(&computed,GetWindowLongW(w,GWL_STYLE),FALSE,GetWindowLongW(w,GWL_EXSTYLE),GetDpiForWindow(w));
 printf("%s dpi=%u style=%08lx ex=%08lx outer=%ldx%ld client=%ldx%ld top=%ld adjusted=%ldx%ld\n",phase,GetDpiForWindow(w),GetWindowLongW(w,GWL_STYLE),GetWindowLongW(w,GWL_EXSTYLE),outer.right-outer.left,outer.bottom-outer.top,client.right,client.bottom,origin.y-outer.top,computed.right-computed.left,computed.bottom-computed.top);
}
int wmain(int argc,wchar_t** argv){
 SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);INITCOMMONCONTROLSEX cc{sizeof cc,ICC_LISTVIEW_CLASSES};InitCommonControlsEx(&cc);
 for(auto context:{DPI_AWARENESS_CONTEXT_UNAWARE,DPI_AWARENESS_CONTEXT_SYSTEM_AWARE,DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2}){SetThreadDpiAwarenessContext(context);printf("context=%lld\n",(long long)context);DLGTEMPLATE t{};t.style=0x80C800C0;t.cx=236;t.cy=177;
 std::vector<BYTE> data(sizeof t);memcpy(data.data(),&t,sizeof t);word(data,0);word(data,0);string(data,L"UDM frame fixture");word(data,8);string(data,L"Tahoma");
 auto w=CreateDialogIndirectParamW(GetModuleHandleW(nullptr),(DLGTEMPLATE*)data.data(),nullptr,dialogProc,0);if(!w){printf("create-error=%lu\n",GetLastError());return 1;}
 report(w,"native");SetWindowLongW(w,GWL_EXSTYLE,GetWindowLongW(w,GWL_EXSTYLE)|WS_EX_TOOLWINDOW);SetWindowPos(w,nullptr,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED);report(w,"tool-frame");RECT client{};GetClientRect(w,&client);RECT r=client;
 AdjustWindowRectExForDpi(&r,GetWindowLongW(w,GWL_STYLE),FALSE,GetWindowLongW(w,GWL_EXSTYLE),GetDpiForWindow(w));
 SetWindowPos(w,nullptr,0,0,r.right-r.left,r.bottom-r.top,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);report(w,"resized");DestroyWindow(w);
if(argc<2)continue; // Optional local reference, loaded as data only.
auto module=LoadLibraryExW(argv[1],nullptr,LOAD_LIBRARY_AS_DATAFILE|LOAD_LIBRARY_AS_IMAGE_RESOURCE);
if(!module){printf("reference-load-error=%lu\n",GetLastError());return 1;}
auto resource=FindResourceW(module,MAKEINTRESOURCEW(209),RT_DIALOG);auto block=LoadResource(module,resource);auto nativeTemplate=(DLGTEMPLATE*)LockResource(block);
if(nativeTemplate){printf("resource cx=%d cy=%d style=%08lx controls=%u\n",nativeTemplate->cx,nativeTemplate->cy,nativeTemplate->style,nativeTemplate->cdit);auto reference=CreateDialogIndirectParamW(GetModuleHandleW(nullptr),nativeTemplate,nullptr,dialogProc,0);if(reference){report(reference,"reference-resource");DestroyWindow(reference);}else printf("reference-create-error=%lu\n",GetLastError());}FreeLibrary(module);
}return 0;
}

