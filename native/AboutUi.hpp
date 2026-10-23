#pragma once
#include "ProductLinks.hpp"
namespace udm {
// Build date in IDM's "(MM/DD/YYYY)" form, from the compiler's "Mmm dd yyyy".
inline std::string productBuildDate(){
 const char* months="JanFebMarAprMayJunJulAugSepOctNovDec";const std::string stamp=__DATE__;
 auto month=(std::string_view(months).find(stamp.substr(0,3))/3)+1;auto day=stamp[4]==' '?stamp.substr(5,1):stamp.substr(4,2);
 char text[16];sprintf_s(text,"%02u/%02u/%s",(unsigned)month,(unsigned)std::stoul(day),stamp.substr(7,4).c_str());return text;
}
// IDM 6.43 "About": 461x503 window with the 40x40 icon, name and version line,
// Check for Update, the "This product is licensed to:" group with two buttons,
// the legal notice, Home Site and Support Team lines, copyright and Close
// (gui-parity-2026-10-18). UDM's licensing group states that the program is
// free software; its buttons open the license and the source instead of a store.
inline void showAboutDialog(CWnd* owner,const Json& settings,std::function<void(CWnd*)> update={},std::function<void(HWND,const std::string&)> open=openProductLink){
 if(!update)update=[settings](CWnd* parent){showUpdateDialog(parent,settings);};
 Form dialog("About UDM Download Manager",253,228,owner);dialog.dialogUnits=true;
 dialog.accept=[&]{dialog.close();};
 dialog.init=[&]{
  auto icon=dialog.control(L"STATIC","",SS_ICON,4,3,23,20);icon->SendMessage(STM_SETICON,(WPARAM)AfxGetApp()->LoadIcon(1));
  dialog.label("UDM Download Manager",30,0,182,16);
  dialog.label(std::string("Version: ")+currentProductVersion+" ("+productBuildDate()+")",29,16,207,8);
  dialog.button("Check for &Update",72,28,99,[&]{update(&dialog);});
  dialog.control(L"BUTTON","This product is licensed to:",BS_GROUPBOX,4,45,245,49);
  dialog.label("Everyone. UDM is free and open-source software.",15,57,222,9);
  dialog.label("No purchase or registration is needed.",15,69,222,8);
  dialog.label("The license terms are in LICENSE.",15,80,222,8);
  dialog.button("&License",106,99,61,[&]{open(dialog.GetSafeHwnd(),productLicense);});
  dialog.button("&Source code",176,99,61,[&]{open(dialog.GetSafeHwnd(),productHome);});
  dialog.label("UDM is free software: you may use, copy, modify and redistribute it under the terms of its license. It is provided without any warranty. UDM is an independent project and is not affiliated with Tonec FZE or Internet Download Manager.",4,115,245,50);
  dialog.label("Home Site:",4,171,81,9);
  dialog.link(productHome,90,171,136,[&]{open(dialog.GetSafeHwnd(),productHome);});
  dialog.label("Support Team:",4,182,81,9);
  dialog.link(productSupport,91,182,158,[&]{open(dialog.GetSafeHwnd(),productSupport);});
  dialog.label("Copyright \xc2\xa9 2026 UDM contributors. All rights reserved.",4,196,230,8);
  auto close=dialog.button("&Close",99,209,55,[&]{dialog.close();});dialog.defaultButton(close);
 };
 dialog.DoModal();
}
}
