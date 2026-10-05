#pragma once
#include "DialUp.hpp"
#include "DialCredentialsUi.hpp"
#include <ras.h>
#include <rasdlg.h>
namespace udm {
inline void dialUpOptions(Form& form,Json& prefs,std::vector<std::function<void()>>& bindings){
 auto enabled=form.check("Connect using Windows dial-up / VPN before downloads",yes(prefs,"DialEnabled"),26,50,541);
 form.label("Connection",26,89,132);auto choice=form.combo({},"",166,85,403);auto entries=std::make_shared<std::vector<DialEntry>>();auto status=form.label("",26,327,542,35);
 auto refresh=[entries,choice,status,&prefs]{try{*entries=dialEntries();choice->ResetContent();int selected=-1,index=0;for(auto& e:*entries){auto label=e.name;int duplicates=0;for(auto& other:*entries)if(other.name==e.name)++duplicates;if(duplicates>1)label+=" ("+e.phonebook+")";choice->AddString(cs(label));if(e.name==str(prefs,"DialEntry")&&(str(prefs,"DialPhonebook").empty()||e.phonebook==str(prefs,"DialPhonebook")))selected=index;++index;}choice->SetCurSel(selected<0&&entries->size()==1?0:selected);status->SetWindowText(cs(entries->empty()?"No Windows connections found. Choose New to set one up.":"Passwords stay in Windows. Use Connect to enter or save credentials."));}catch(const std::exception& e){status->SetWindowText(cs(e.what()));}};
 auto selected=[entries,choice]{auto index=choice->GetCurSel();if(index<0||index>=(int)entries->size())throw std::runtime_error("Select a Windows connection.");return entries->at(index);};
 form.button("New...",26,122,97,[&form,refresh]{RASENTRYDLGW info{};info.dwSize=sizeof(info);info.hwndOwner=form.GetSafeHwnd();info.dwFlags=RASEDFLAG_NewEntry;RasEntryDlgW(nullptr,nullptr,&info);refresh();if(info.dwError)throw std::runtime_error("Windows connection setup failed ("+std::to_string(info.dwError)+").");});
 form.button("Properties...",134,122,112,[&form,selected,refresh]{auto e=selected();auto name=wide(e.name),book=wide(e.phonebook);RASENTRYDLGW info{};info.dwSize=sizeof(info);info.hwndOwner=form.GetSafeHwnd();RasEntryDlgW(book.empty()?nullptr:book.data(),name.data(),&info);refresh();if(info.dwError)throw std::runtime_error("Windows connection properties failed ("+std::to_string(info.dwError)+").");});
 form.button("Connect...",257,122,100,[&form,selected]{auto e=selected();auto name=wide(e.name),book=wide(e.phonebook);RASDIALDLG info{};info.dwSize=sizeof(info);info.hwndOwner=form.GetSafeHwnd();RasDialDlgW(book.empty()?nullptr:book.data(),name.data(),nullptr,&info);if(info.dwError)throw std::runtime_error("Windows connection failed ("+std::to_string(info.dwError)+").");});
 form.button("Refresh",368,122,97,refresh);
 form.label("Dial attempts (0 = until stopped)",26,167,301);auto attempts=form.edit(std::to_string(num(prefs,"DialAttempts",3)),431,163,138);
 form.label("Seconds between attempts",26,201,301);auto delay=form.edit(std::to_string(num(prefs,"DialRetrySeconds",10)),431,197,138);
 form.label("Connection timeout (seconds)",26,235,301);auto timeout=form.edit(std::to_string(num(prefs,"DialTimeoutSeconds",120)),431,231,138);
 form.label("Queued and manual downloads wait for this connection. Pause cancels dialing. Existing connections are reused; connections UDM opens automatically close when UDM exits.",26,270,543,50);
 bindings.push_back([enabled,selected,attempts,delay,timeout,&prefs,&form]{prefs["DialEnabled"]=form.checked(enabled);try{auto e=selected();prefs["DialEntry"]=e.name;prefs["DialPhonebook"]=e.phonebook;}catch(...){if(form.checked(enabled))throw;}prefs["DialAttempts"]=std::stoll(text(attempts));prefs["DialRetrySeconds"]=std::stoll(text(delay));prefs["DialTimeoutSeconds"]=std::stoll(text(timeout));validateDialSettings(prefs);});refresh();
}
}
