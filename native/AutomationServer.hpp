#pragma once
// COM class objects for UDM download automation, hosted either by
// Udm.Automation.exe (launched by COM, forwards over the local pipe) or by the
// running desktop itself (serves calls directly). A running desktop's class
// object is found before any registry lookup, so automation works whenever UDM
// is running even where per-user LocalServer32 activation is unavailable.
#include "Automation.hpp"
#include "Udm.Automation_h.h"
#include <atlbase.h>
namespace udm {
constexpr wchar_t AutomationClassKey[]=L"Software\\Classes\\CLSID\\{84987627-1793-4A9D-AEC6-E31AF7D6942A}";
constexpr wchar_t AutomationInterfaceKey[]=L"Software\\Classes\\Interface\\{14C30E3F-910A-4B44-907F-2D39249C1D0C}";
constexpr wchar_t AutomationLibraryId[]=L"{FFCA5D22-0EAB-4193-8EC2-1EE439450B58}";
constexpr wchar_t AutomationClassId[]=L"{84987627-1793-4A9D-AEC6-E31AF7D6942A}";
struct AutomationRuntime {
 std::function<void(const Json&)> transmit;std::function<void()> changed;std::atomic<long> active{0};std::atomic<ULONGLONG> idleSince{0};CComPtr<ITypeInfo> info;
 void acquired(){++active;CoAddRefServerProcess();}
 void released(){if(--active==0)idleSince=GetTickCount64();CoReleaseServerProcess();if(changed)changed();}
};
inline AutomationRuntime& automationRuntime(){static AutomationRuntime runtime;return runtime;}
namespace automation {
inline void checked(HRESULT hr){if(FAILED(hr))throw std::runtime_error("Windows COM error "+std::to_string((unsigned long)hr)+".");}
inline std::string string(BSTR text,size_t maximum=16384){
 auto length=SysStringLen(text);if(length>maximum)throw std::invalid_argument("Automation string is too long.");
 if(!length)return {};if(wmemchr(text,L'\0',length))throw std::invalid_argument("Automation strings cannot contain NUL characters.");
 if(!WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text,length,nullptr,0,nullptr,nullptr))throw std::invalid_argument("Automation text is not valid Unicode.");
 return utf8(std::wstring(text,length));
}
inline HRESULT failure(const std::exception& error,HRESULT code){
 CComPtr<ICreateErrorInfo> create;if(SUCCEEDED(CreateErrorInfo(&create))){create->SetGUID(IID_IUdmLinkTransmitter);create->SetSource(const_cast<LPOLESTR>(L"UDM.LinkTransmitter"));auto message=wide(error.what());create->SetDescription(message.data());CComPtr<IErrorInfo> e;if(SUCCEEDED(create.QueryInterface(&e)))SetErrorInfo(0,e);}return code;
}
template<class F> HRESULT invoke(F work){try{work();return S_OK;}catch(const std::invalid_argument& e){return failure(e,E_INVALIDARG);}catch(const std::bad_alloc&){return E_OUTOFMEMORY;}catch(const std::exception& e){return failure(e,E_FAIL);}catch(...){return E_UNEXPECTED;}}
class Transmitter final:public IUdmLinkTransmitter,public ISupportErrorInfo {
 std::atomic<ULONG> refs{1};
 static void transmit(const Json& message){auto& runtime=automationRuntime();if(!runtime.transmit)throw std::runtime_error("UDM automation is not ready.");runtime.transmit(message);}
public:
 Transmitter(){automationRuntime().acquired();}~Transmitter(){automationRuntime().released();}
 HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void** out)override{if(!out)return E_POINTER;*out=nullptr;if(iid==IID_IUnknown||iid==IID_IDispatch||iid==IID_IUdmLinkTransmitter)*out=static_cast<IUdmLinkTransmitter*>(this);else if(iid==IID_ISupportErrorInfo)*out=static_cast<ISupportErrorInfo*>(this);else return E_NOINTERFACE;AddRef();return S_OK;}
 ULONG STDMETHODCALLTYPE AddRef()override{return ++refs;}ULONG STDMETHODCALLTYPE Release()override{auto n=--refs;if(!n)delete this;return n;}
 HRESULT STDMETHODCALLTYPE InterfaceSupportsErrorInfo(REFIID iid)override{return iid==IID_IUdmLinkTransmitter?S_OK:S_FALSE;}
 HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT* count)override{if(!count)return E_POINTER;*count=1;return S_OK;}
 HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT index,LCID,ITypeInfo** out)override{if(!out)return E_POINTER;*out=nullptr;if(index)return DISP_E_BADINDEX;*out=automationRuntime().info;if(!*out)return E_UNEXPECTED;(*out)->AddRef();return S_OK;}
 HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID iid,LPOLESTR* names,UINT count,LCID,DISPID* ids)override{if(iid!=IID_NULL)return DISP_E_UNKNOWNINTERFACE;return DispGetIDsOfNames(automationRuntime().info,names,count,ids);}
 HRESULT STDMETHODCALLTYPE Invoke(DISPID member,REFIID iid,LCID,WORD flags,DISPPARAMS* params,VARIANT* result,EXCEPINFO* ex,UINT* arg)override{if(iid!=IID_NULL)return DISP_E_UNKNOWNINTERFACE;return DispInvoke(static_cast<IUdmLinkTransmitter*>(this),automationRuntime().info,member,flags,params,result,ex,arg);}
 HRESULT STDMETHODCALLTYPE SendLinkToUDM(BSTR url,BSTR referer,BSTR cookies,BSTR data,BSTR user,BSTR password,BSTR path,BSTR filename,LONG flags)override{VARIANT empty;VariantInit(&empty);return SendLinkToUDM2(url,referer,cookies,data,user,password,path,filename,flags,empty,empty);}
 HRESULT STDMETHODCALLTYPE SendLinkToUDM2(BSTR url,BSTR referer,BSTR cookies,BSTR data,BSTR user,BSTR password,BSTR path,BSTR filename,LONG flags,VARIANT agent,VARIANT reserved)override{return invoke([&]{
  CComVariant actualAgent,actualReserved;checked(VariantCopyInd(&actualAgent,&agent));checked(VariantCopyInd(&actualReserved,&reserved));
  if((actualAgent.vt!=VT_EMPTY&&actualAgent.vt!=VT_BSTR)||actualReserved.vt!=VT_EMPTY)throw std::invalid_argument("User agent must be a string or empty; the reserved argument must be empty.");
  Json link={{"url",string(url)},{"referrer",string(referer)},{"cookies",string(cookies)},{"data",string(data,MaxBrowserPostBytes)},{"user",string(user)},{"password",string(password)},{"folder",string(path)},{"filename",string(filename)},{"flags",flags},{"userAgent",actualAgent.vt==VT_BSTR?string(actualAgent.bstrVal):""}};
  transmit({{"action","automation-add"},{"link",validateAutomationLink(link)}});
 });}
 HRESULT STDMETHODCALLTYPE SendLinksArray(BSTR location,VARIANT array)override{return invoke([&]{
  CComVariant value;checked(VariantCopyInd(&value,&array));if(value.vt!=(VT_ARRAY|VT_BSTR)&&value.vt!=(VT_ARRAY|VT_VARIANT))throw std::invalid_argument("Use a two-dimensional array of strings.");
  auto a=value.parray;if(!a||SafeArrayGetDim(a)!=2)throw std::invalid_argument("The links array must have two dimensions.");LONG first[2]{},last[2]{};
  for(UINT d=1;d<=2;++d){checked(SafeArrayGetLBound(a,d,&first[d-1]));checked(SafeArrayGetUBound(a,d,&last[d-1]));}
  auto count=(i64)last[0]-first[0]+1;if(count<1||count>1000||(i64)last[1]-first[1]+1!=4)throw std::invalid_argument("Use N rows and four columns: URL, cookies, description, user agent.");
  auto cell=[&](LONG row,LONG column){LONG at[]={row,column};if(value.vt==(VT_ARRAY|VT_BSTR)){CComBSTR s;checked(SafeArrayGetElement(a,at,&s));return string(s);}CComVariant v;checked(SafeArrayGetElement(a,at,&v));if(v.vt==VT_EMPTY||v.vt==VT_NULL)return std::string();if(v.vt!=VT_BSTR)throw std::invalid_argument("Every array cell must be a string or empty.");return string(v.bstrVal);};
  Json links=Json::array();auto agent=cell(first[0],first[1]+3);
  for(i64 n=0;n<count;++n){auto r=(LONG)(first[0]+n);if(n&&!cell(r,first[1]+3).empty())throw std::invalid_argument("Only the first row may supply a user agent.");links.push_back({{"url",cell(r,first[1])},{"cookies",cell(r,first[1]+1)},{"description",cell(r,first[1]+2)}});}
  Json request={{"action","automation-batch"},{"referrer",string(location)},{"userAgent",agent},{"links",links}};validateAutomationBatch(request);transmit(request);
 });}
};
class Factory final:public IClassFactory {
 std::atomic<ULONG> refs{1};std::atomic<long> locks{0};
public:
 HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void** out)override{if(!out)return E_POINTER;*out=nullptr;if(iid!=IID_IUnknown&&iid!=IID_IClassFactory)return E_NOINTERFACE;*out=static_cast<IClassFactory*>(this);AddRef();return S_OK;}
 ULONG STDMETHODCALLTYPE AddRef()override{return ++refs;}ULONG STDMETHODCALLTYPE Release()override{auto n=--refs;if(!n)delete this;return n;}
 HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown* outer,REFIID iid,void** out)override{if(!out)return E_POINTER;*out=nullptr;if(outer)return CLASS_E_NOAGGREGATION;auto p=new(std::nothrow) Transmitter;if(!p)return E_OUTOFMEMORY;auto hr=p->QueryInterface(iid,out);p->Release();return hr;}
 HRESULT STDMETHODCALLTYPE LockServer(BOOL lock)override{if(lock){++locks;automationRuntime().acquired();}else{if(locks==0)return E_UNEXPECTED;--locks;automationRuntime().released();}return S_OK;}
};
inline fs::path serverPath(){return appDir()/L"Udm.Automation.exe";}
inline fs::path libraryPath(){return appDir()/L"Udm.Automation.tlb";}
inline void reg(HKEY root,const std::wstring& path,const wchar_t* name,const std::wstring& value,REGSAM view){HKEY key=nullptr;auto code=RegCreateKeyExW(root,path.c_str(),0,nullptr,0,KEY_SET_VALUE|view,nullptr,&key,nullptr);if(code)throw std::runtime_error("Cannot register UDM automation ("+std::to_string(code)+").");code=RegSetValueExW(key,name,0,REG_SZ,(const BYTE*)value.c_str(),(DWORD)((value.size()+1)*sizeof(wchar_t)));RegCloseKey(key);if(code)throw std::runtime_error("Cannot write UDM automation registration ("+std::to_string(code)+").");}
inline std::wstring registered(HKEY root,REGSAM view){HKEY key=nullptr;auto path=std::wstring(AutomationClassKey)+L"\\LocalServer32";if(RegOpenKeyExW(root,path.c_str(),0,KEY_READ|view,&key))return {};wchar_t text[32768]{};DWORD size=sizeof(text),type=0;auto result=RegQueryValueExW(key,nullptr,nullptr,&type,(BYTE*)text,&size);RegCloseKey(key);if(result||type!=REG_SZ||size>sizeof(text))throw std::runtime_error("Existing UDM registration could not be read.");return text;}
// machine=true writes HKLM (needs administrator rights); otherwise the current user's classes.
inline void unregisterServer(bool machine){
 HKEY root=machine?HKEY_LOCAL_MACHINE:HKEY_CURRENT_USER;auto executable=quote(serverPath().wstring());
 for(REGSAM view:{KEY_WOW64_64KEY,KEY_WOW64_32KEY}){auto old=registered(root,view);if(!old.empty()&&old.rfind(executable+L" ",0)!=0)throw std::runtime_error("Another UDM installation owns this automation registration.");}
 for(REGSAM view:{KEY_WOW64_64KEY,KEY_WOW64_32KEY}){HKEY classes=nullptr;if(RegOpenKeyExW(root,L"Software\\Classes",0,KEY_WRITE|view,&classes))continue;for(auto key:{L"CLSID\\{84987627-1793-4A9D-AEC6-E31AF7D6942A}",L"Interface\\{14C30E3F-910A-4B44-907F-2D39249C1D0C}",L"UDM.LinkTransmitter",L"UDM.LinkTransmitter.1"}){auto code=RegDeleteTreeW(classes,key);if(code&&code!=ERROR_FILE_NOT_FOUND){RegCloseKey(classes);throw std::runtime_error("Cannot remove UDM automation registration ("+std::to_string(code)+").");}}RegCloseKey(classes);}
 auto hr=machine?UnRegisterTypeLib(LIBID_UdmAutomation,1,0,0,SYS_WIN64):UnRegisterTypeLibForUser(LIBID_UdmAutomation,1,0,0,SYS_WIN64);if(FAILED(hr)&&hr!=TYPE_E_REGISTRYACCESS&&hr!=TYPE_E_LIBNOTREGISTERED)checked(hr);
}
inline void registerServer(const std::wstring& arguments,bool machine){
 HKEY root=machine?HKEY_LOCAL_MACHINE:HKEY_CURRENT_USER;auto executable=quote(serverPath().wstring()),command=executable+L" -Embedding"+arguments;
 for(REGSAM view:{KEY_WOW64_64KEY,KEY_WOW64_32KEY}){auto old=registered(root,view);if(!old.empty()&&old.rfind(executable+L" ",0)!=0)throw std::runtime_error("Another UDM installation owns this automation registration.");}
 auto library=libraryPath().wstring();CComPtr<ITypeLib> typeLibrary;checked(LoadTypeLibEx(library.c_str(),REGKIND_NONE,&typeLibrary));
 try{
  for(REGSAM view:{KEY_WOW64_64KEY,KEY_WOW64_32KEY}){
   reg(root,std::wstring(AutomationClassKey)+L"\\LocalServer32",nullptr,command,view);reg(root,std::wstring(AutomationClassKey)+L"\\LocalServer32",L"ServerExecutable",serverPath().wstring(),view);
   reg(root,AutomationClassKey,nullptr,L"UDM Link Transmitter",view);reg(root,std::wstring(AutomationClassKey)+L"\\ProgID",nullptr,L"UDM.LinkTransmitter.1",view);reg(root,std::wstring(AutomationClassKey)+L"\\VersionIndependentProgID",nullptr,L"UDM.LinkTransmitter",view);reg(root,std::wstring(AutomationClassKey)+L"\\TypeLib",nullptr,AutomationLibraryId,view);
   for(auto prog:{L"UDM.LinkTransmitter",L"UDM.LinkTransmitter.1"}){reg(root,std::wstring(L"Software\\Classes\\")+prog,nullptr,L"UDM Link Transmitter",view);reg(root,std::wstring(L"Software\\Classes\\")+prog+L"\\CLSID",nullptr,AutomationClassId,view);}reg(root,L"Software\\Classes\\UDM.LinkTransmitter\\CurVer",nullptr,L"UDM.LinkTransmitter.1",view);
   reg(root,AutomationInterfaceKey,nullptr,L"IUdmLinkTransmitter",view);reg(root,std::wstring(AutomationInterfaceKey)+L"\\ProxyStubClsid32",nullptr,L"{00020424-0000-0000-C000-000000000046}",view);reg(root,std::wstring(AutomationInterfaceKey)+L"\\TypeLib",nullptr,AutomationLibraryId,view);reg(root,std::wstring(AutomationInterfaceKey)+L"\\TypeLib",L"Version",L"1.0",view);
  }
  checked(machine?RegisterTypeLib(typeLibrary,const_cast<LPOLESTR>(library.c_str()),nullptr):RegisterTypeLibForUser(typeLibrary,const_cast<LPOLESTR>(library.c_str()),nullptr));
 }catch(...){try{unregisterServer(machine);}catch(...){}throw;}
}
}
// Registers the class object in the calling (STA) apartment. Calls arrive on
// that thread's message loop.
class AutomationHost {
 DWORD registration=0;
public:
 AutomationHost(std::function<void(const Json&)> transmit,std::function<void()> changed={}){
  auto& runtime=automationRuntime();runtime.transmit=std::move(transmit);runtime.changed=std::move(changed);
  if(!runtime.info){CComPtr<ITypeLib> library;automation::checked(LoadTypeLibEx(automation::libraryPath().c_str(),REGKIND_NONE,&library));automation::checked(library->GetTypeInfoOfGuid(IID_IUdmLinkTransmitter,&runtime.info));}
  CComPtr<IClassFactory> factory;factory.Attach(new automation::Factory);
  automation::checked(CoRegisterClassObject(CLSID_UdmLinkTransmitter,factory,CLSCTX_LOCAL_SERVER,REGCLS_MULTIPLEUSE|REGCLS_SUSPENDED,&registration));automation::checked(CoResumeClassObjects());runtime.idleSince=GetTickCount64();
 }
 ~AutomationHost(){if(registration)CoRevokeClassObject(registration);auto& runtime=automationRuntime();runtime.transmit=nullptr;runtime.changed=nullptr;}
 AutomationHost(const AutomationHost&)=delete;AutomationHost& operator=(const AutomationHost&)=delete;
};
}
