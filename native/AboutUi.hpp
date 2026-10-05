#pragma once
#include "ProductLinks.hpp"
namespace udm {
inline void showAboutDialog(CWnd* owner,const Json& settings,std::function<void(CWnd*)> update={},std::function<void(HWND,const std::string&)> open=openProductLink){
 if(!update)update=[settings](CWnd* parent){showUpdateDialog(parent,settings);};
 Form dialog("About UDM Download Manager",260,170,owner);dialog.dialogUnits=true;
 dialog.accept=[&]{dialog.close();};
 dialog.init=[&]{
  auto icon=dialog.control(L"STATIC","",SS_ICON,7,6,20,20);icon->SendMessage(STM_SETICON,(WPARAM)AfxGetApp()->LoadIcon(1));
  dialog.label("UDM Download Manager",33,3,208,16);
  dialog.label(std::string("Version: ")+currentProductVersion,32,19,208,9);
  dialog.button("Check for &Update",75,31,99,[&]{update(&dialog);});
  dialog.edit("UDM Download Manager\r\nDownload files and manage queues.\r\nPause and resume supported downloads.\r\n\r\nNo activation required.",7,54,245,50,true,true);
  dialog.label("Home Site:",7,112,81,9);
  dialog.link("UDM project",93,112,150,[&]{open(dialog.GetSafeHwnd(),productHome);});
  dialog.label("Support:",7,125,81,9);
  dialog.link("Report an issue",93,125,150,[&]{open(dialog.GetSafeHwnd(),productSupport);});
  auto close=dialog.button("&Close",102,148,55,[&]{dialog.close();});dialog.defaultButton(close);
 };
 dialog.DoModal();
}
}
