#pragma once
#include "DialUp.hpp"
#include "DialCredentialsUi.hpp"
#include <ras.h>
#include <rasdlg.h>
namespace udm {
inline int dialEditorSelection(const std::vector<DialEntry>& entries,const DialEntry& desired){
 int exact=-1,exactCount=0,name=-1,nameCount=0;for(int i=0;i<(int)entries.size();++i)if(entries[i].name==desired.name){name=i;++nameCount;if(entries[i].phonebook==desired.phonebook){exact=i;++exactCount;}}
 if(exactCount==1)return exact;if(exactCount>1)return -1;if(desired.phonebook.empty()&&nameCount==1)return name;return entries.size()==1?0:-1;
}
enum class DialEditorAction {New,Properties,Connect};
using DialEditorProvider=std::function<std::vector<DialEntry>()>;
using DialEditorNativeAction=std::function<void(DialEditorAction,CWnd*,const std::optional<DialEntry>&)>;
inline void nativeDialEditorAction(DialEditorAction action,CWnd* owner,const std::optional<DialEntry>& entry){
 if(action!=DialEditorAction::New&&!entry)throw std::runtime_error("Select a Windows connection.");
 if(action==DialEditorAction::Connect){auto name=wide(entry->name),book=wide(entry->phonebook);RASDIALDLG info{};info.dwSize=sizeof(info);info.hwndOwner=owner->GetSafeHwnd();RasDialDlgW(book.empty()?nullptr:book.data(),name.data(),nullptr,&info);if(info.dwError)throw std::runtime_error("Windows connection failed ("+std::to_string(info.dwError)+").");return;}
 RASENTRYDLGW info{};info.dwSize=sizeof(info);info.hwndOwner=owner->GetSafeHwnd();if(action==DialEditorAction::New)info.dwFlags=RASEDFLAG_NewEntry;auto name=entry?wide(entry->name):L"",book=entry?wide(entry->phonebook):L"";RasEntryDlgW(book.empty()?nullptr:book.data(),entry?name.data():nullptr,&info);if(info.dwError)throw std::runtime_error(std::string(action==DialEditorAction::New?"Windows connection setup failed (":"Windows connection properties failed (")+std::to_string(info.dwError)+").");
}
inline bool dialUpOptions(CWnd* owner,Json& prefs,DialEditorProvider provider=dialEntries,DialEditorNativeAction native=nativeDialEditorAction){
 struct State {AsyncDialLookup read;std::vector<DialEntry> entries;DialEntry desired;bool ready=false,active=false;};
 auto state=std::make_shared<State>();state->desired={str(prefs,"DialEntry"),str(prefs,"DialPhonebook")};Json accepted;const Json baseline={{"DialEnabled",yes(prefs,"DialEnabled")},{"DialEntry",str(prefs,"DialEntry")},{"DialPhonebook",str(prefs,"DialPhonebook")},{"DialAttempts",num(prefs,"DialAttempts",3)},{"DialRetrySeconds",num(prefs,"DialRetrySeconds",10)},{"DialTimeoutSeconds",num(prefs,"DialTimeoutSeconds",120)}};
 Form form("Windows connection options",602,410,owner);
 form.cancel=[&form,state]{if(state->active)return;state->read.stop();form.close(IDCANCEL);};
 form.init=[&]{
  auto enabled=form.check("Connect using Windows dial-up / VPN before downloads",yes(prefs,"DialEnabled"),26,50,541);
  form.label("Connection",26,89,132);auto choice=form.combo({},"",166,85,403);auto status=form.label("",26,327,542,35);
  auto selected=[state,choice]{auto index=choice->GetCurSel();if(!state->ready||index<0||index>=(int)state->entries.size())throw std::runtime_error("Select a Windows connection.");return state->entries.at(index);};
  auto newButton=form.button("New...",26,122,97,[]{});auto properties=form.button("Properties...",134,122,112,[]{});auto connect=form.button("Connect...",257,122,100,[]{});
  auto update=[state,choice,properties,connect]{bool available=state->ready&&choice->GetCurSel()>=0;properties->EnableWindow(available);connect->EnableWindow(available);};
  const auto wake=form.taskNotifier();
  auto refresh=[state,choice,status,provider,wake,update]{if(state->active)return;
   auto index=choice->GetCurSel();if(state->ready&&index>=0&&index<(int)state->entries.size())state->desired=state->entries[index];
   state->ready=false;state->entries.clear();choice->ResetContent();choice->EnableWindow(FALSE);update();status->SetWindowText(L"Reading Windows connections...");
   try{state->read.request([provider]{return AsyncDialLookup::Value(provider());},wake);}catch(const std::exception& e){status->SetWindowText(cs(e.what()));}
  };
  auto invoke=[&form,state,selected,refresh,native](DialEditorAction action){if(state->active)return;std::optional<DialEntry> entry;if(action!=DialEditorAction::New)entry=selected();state->active=true;std::exception_ptr failure;try{native(action,&form,entry);}catch(...){failure=std::current_exception();}state->active=false;if(action!=DialEditorAction::Connect)refresh();if(failure){try{std::rethrow_exception(failure);}catch(const std::exception&){throw;}catch(...){throw std::runtime_error("Windows connection action failed.");}}};
  form.bind(newButton,[invoke]{invoke(DialEditorAction::New);});form.bind(properties,[invoke]{invoke(DialEditorAction::Properties);});form.bind(connect,[invoke]{invoke(DialEditorAction::Connect);});form.button("Refresh",368,122,97,refresh);form.bindChange(choice,update);
  form.label("Dial attempts (0 = until stopped)",26,167,301);auto attempts=form.edit(std::to_string(num(prefs,"DialAttempts",3)),431,163,138);
  form.label("Seconds between attempts",26,201,301);auto delay=form.edit(std::to_string(num(prefs,"DialRetrySeconds",10)),431,197,138);
  form.label("Connection timeout (seconds)",26,235,301);auto timeout=form.edit(std::to_string(num(prefs,"DialTimeoutSeconds",120)),431,231,138);
  form.label("Queued and manual downloads wait for this connection. Pause cancels dialing. Existing connections are reused; connections UDM opens automatically close when UDM exits.",26,270,543,50);
  form.accept=[&form,&accepted,&prefs,baseline,state,selected,enabled,attempts,delay,timeout]{if(state->active)return;
   auto whole=[](CWnd* control){auto raw=trim(text(control));if(raw.empty()||raw.find_first_not_of("0123456789")!=std::string::npos)throw std::runtime_error("Use whole-number dial settings.");try{return std::stoll(raw);}catch(...){throw std::runtime_error("Use whole-number dial settings.");}};
   auto entry=state->desired;if(state->ready)try{entry=selected();}catch(...){if(form.checked(enabled))throw;}
   Json next={{"DialEnabled",form.checked(enabled)},{"DialEntry",entry.name},{"DialPhonebook",entry.phonebook},{"DialAttempts",whole(attempts)},{"DialRetrySeconds",whole(delay)},{"DialTimeoutSeconds",whole(timeout)}};validateDialSettings(next);Json patch=Json::object(),merged=prefs;for(auto i=next.begin();i!=next.end();++i)if(i.value()!=baseline.at(i.key())){patch[i.key()]=i.value();merged[i.key()]=i.value();}validateDialSettings(merged);accepted=std::move(patch);state->read.stop();form.close();
  };
  form.defaultButton(form.button("OK",407,376,83,form.accept));form.button("Cancel",502,376,83,form.cancel);
  form.pulse=[state,choice,status,update]{if(state->active)return;if(auto result=state->read.take()){
   if(!result->value){status->SetWindowText(cs(result->error));return;}state->entries=std::get<std::vector<DialEntry>>(*result->value);state->ready=true;choice->ResetContent();int selected=dialEditorSelection(state->entries,state->desired);
   for(int i=0;i<(int)state->entries.size();++i){const auto& entry=state->entries[i];auto title=entry.name;int duplicates=0;for(const auto& other:state->entries)if(other.name==entry.name)++duplicates;if(duplicates>1)title+=" ("+entry.phonebook+")";choice->AddString(cs(title));}
   choice->SetCurSel(selected);choice->EnableWindow(!state->entries.empty());update();status->SetWindowText(cs(state->entries.empty()?"No Windows connections found. Choose New to set one up.":"Passwords stay in Windows. Use Connect to enter or save credentials."));
  }};refresh();
 };
 auto result=form.DoModal();state->read.stop();if(result!=IDOK)return false;
 if(!accepted.is_object()||accepted.size()>6)throw std::runtime_error("The connection settings were not accepted.");auto next=prefs;for(auto i=accepted.begin();i!=accepted.end();++i)next[i.key()]=i.value();validateDialSettings(next);if(!accepted.empty())prefs=std::move(next);return true;
}
}
