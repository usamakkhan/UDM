// Manual, local-origin-only reference probe. This uses IDM's published COM API,
// not its browser extension internals. No queue, quit or configuration commands.
#include <windows.h>
#include <atlbase.h>
#include <iostream>
#import "C:\Program Files (x86)\Internet Download Manager\idmantypeinfo.tlb" no_namespace raw_interfaces_only named_guids
int wmain(int argc,wchar_t** argv){
 if(argc!=5||std::wstring(argv[1]).rfind(L"http://127.0.0.1:",0)!=0)return 2;
 HRESULT hr=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);if(FAILED(hr))return 3;
 {
  CLSID cls{};hr=CLSIDFromProgID(L"IDMan.CIDMLinkTransmitter",&cls);
  CComPtr<ICIDMLinkTransmitter2> transmitter;
  if(SUCCEEDED(hr))hr=transmitter.CoCreateInstance(cls,nullptr,CLSCTX_LOCAL_SERVER);
  VARIANT empty;VariantInit(&empty);
  if(SUCCEEDED(hr))hr=transmitter->SendLinkToIDM2(CComBSTR(argv[1]),nullptr,nullptr,CComBSTR(argv[2]),nullptr,nullptr,CComBSTR(argv[3]),CComBSTR(argv[4]),1,empty,empty);
 }
 CoUninitialize();std::cout<<"COM HRESULT "<<std::hex<<hr<<"\n";return FAILED(hr)?1:0;
}
