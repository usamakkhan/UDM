#define UDM_TOOLBAR_COMPONENT_TEST
#include "Core.hpp"
#include "Ui.hpp"
namespace udm {
static fs::path output;static Json checks=Json::array();static std::string failure;
static void check(bool value,const char* name){checks.push_back({{"name",name},{"passed",value}});if(!value&&failure.empty())failure=name;}
static void run(){
 Form form("Owned command dispatch",460,180);CWnd *action=nullptr,*other=nullptr,*bound=nullptr,*link=nullptr,*removed=nullptr;int calls=0,boundCalls=0,linkCalls=0,removedCalls=0;bool ran=false;
 form.init=[&]{action=form.button("Run",12,12,90,[&]{++calls;});other=form.button("Other",110,12,90,[]{});bound=form.check("Bound checkbox",false,12,50,180);form.bind(bound,[&]{++boundCalls;});link=form.staticLink("Owned link",12,85,180,[&]{++linkCalls;});removed=form.button("Removed",220,12,90,[&]{++removedCalls;});};
 const auto send=[&](CWnd* target,HWND source){form.SendMessage(WM_COMMAND,MAKEWPARAM(target->GetDlgCtrlID(),BN_CLICKED),reinterpret_cast<LPARAM>(source));};
 form.pulse=[&]{if(ran)return;ran=true;try{
  send(action,action->GetSafeHwnd());check(calls==1,"An enabled owned button dispatches its current action");calls=0;
  action->EnableWindow(FALSE);send(action,action->GetSafeHwnd());check(calls==0,"A queued command cannot execute a disabled button action");calls=0;
  send(action,nullptr);check(calls==0,"An originless command cannot bypass a disabled owned button");calls=0;
  action->EnableWindow(TRUE);send(action,action->GetSafeHwnd());check(calls==1,"Reenabling the button restores its ordinary action");calls=0;
  send(action,other->GetSafeHwnd());check(calls==0,"A command from a different child cannot borrow an action ID");calls=0;
  send(action,nullptr);check(calls==1,"An enabled owned action retains originless command routing");calls=0;
  form.EnableWindow(FALSE);send(action,action->GetSafeHwnd());check(calls==0,"A disabled dialog cannot dispatch one of its button actions");calls=0;form.EnableWindow(TRUE);
  bound->EnableWindow(FALSE);send(bound,bound->GetSafeHwnd());check(boundCalls==0,"A queued checkbox notification cannot execute its disabled binding");boundCalls=0;
  bound->EnableWindow(TRUE);bound->SendMessage(BM_CLICK);check(boundCalls==1&&form.checked(bound),"An enabled checkbox still toggles and invokes its binding");
  send(link,link->GetSafeHwnd());check(linkCalls==1,"An enabled static hyperlink retains click routing");linkCalls=0;
  link->EnableWindow(FALSE);send(link,link->GetSafeHwnd());check(linkCalls==0,"A disabled static hyperlink cannot execute its old click");
  const auto id=removed->GetDlgCtrlID();const auto oldWindow=removed->GetSafeHwnd();removed->DestroyWindow();form.SendMessage(WM_COMMAND,MAKEWPARAM(id,BN_CLICKED),reinterpret_cast<LPARAM>(oldWindow));check(removedCalls==0,"A destroyed child cannot execute its remaining action binding");
  form.SendMessage(WM_COMMAND,MAKEWPARAM(id,BN_CLICKED),0);check(removedCalls==0,"An originless command cannot resurrect a destroyed control");
 }catch(const std::exception& error){failure=error.what();}form.close(IDCANCEL);};
 form.DoModal();check(ran&&!form.GetSafeHwnd(),"The owned dispatch dialog closes normally");
}
class FormCommandApplication:public CWinApp{int result=1;public:BOOL InitInstance()override{CWinApp::InitInstance();INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_WIN95_CLASSES|ICC_DATE_CLASSES};InitCommonControlsEx(&controls);AfxOleInit();int argc=0;auto args=CommandLineToArgvW(GetCommandLineW(),&argc);try{if(!args||argc!=2)throw std::runtime_error("Pass one fresh output directory");output=args[1];if(fs::exists(output)){output.clear();throw std::runtime_error("Fresh output required");}fs::create_directories(output);run();result=failure.empty()?0:1;}catch(const std::exception& error){failure=error.what();}if(args)LocalFree(args);if(!output.empty())atomicText(output/L"results.json",Json{{"passed",result==0},{"checks",checks},{"error",failure}}.dump(2),false);return FALSE;}int ExitInstance()override{AfxOleTerm(FALSE);return result;}};FormCommandApplication application;
}
