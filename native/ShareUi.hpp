#pragma once
#include "ProductLinks.hpp"
namespace udm {
using ProductLinkOpener=std::function<void(HWND,const std::string&)>;
using ProductLinkCopier=std::function<void(HWND,const std::wstring&)>;
inline void showShareDialog(CWnd* owner,ProductLinkOpener opener=openProductLink,ProductLinkCopier copier=copyProductLink){
 Form dialog("Tell a friend",425,152,owner);dialog.init=[&]{
  dialog.label("Share UDM Download Manager",13,12,399);auto address=dialog.edit(productHome,13,39,399,23,true);
  auto status=dialog.label("Copy the project link or open a draft in your email app.",13,76,399,31);
  dialog.button("Copy link",13,113,110,[&,status]{copier(dialog.GetSafeHwnd(),wide(productHome));status->SetWindowText(L"Project link copied.");});
  dialog.button("Email...",133,113,110,[&,status]{opener(dialog.GetSafeHwnd(),productShareMail);status->SetWindowText(L"Email draft opened. Review it in your email app.");});
  dialog.button("Close",302,113,110,[&]{dialog.close();});address->SendMessage(EM_SETSEL,0,-1);
 };dialog.DoModal();
}
}
