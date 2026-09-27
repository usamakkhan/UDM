#pragma once
#include "BrowserSettings.hpp"
namespace udm {
inline bool browserIntegration(CWnd* owner,Json& prefs){
 Form d("Browser integration and video panels",594,518,owner);Json draft=prefs;
 d.init=[&]{
  auto capture=d.check("Allow automatic browser download capture",yes(draft,"BrowserCaptureEnabled",true),16,14,561);
  d.label("Automatic capture also needs to be enabled in your browser extension.",34,41,543,30);
  auto enabled=d.check("Show Download this video on supported players",yes(draft,"VideoPanelEnabled",true),16,77,561);
  auto compact=d.check("Use a compact icon",yes(draft,"VideoPanelCompact"),34,107,258);
  auto hover=d.check("Show only while pointing at a video",yes(draft,"VideoPanelHover"),298,107,279);
  d.label("Panel position",34,149,127);auto position=d.combo({"Top right","Top left","Bottom right","Bottom left"},str(draft,"VideoPanelPosition","Top right"),166,145,155);
  d.label("Menu width",336,149,97);auto width=d.edit(std::to_string(num(draft,"VideoPanelMenuWidth",420)),450,145,127);
  d.button("Reset saved panel positions",34,180,243,[&]{draft["VideoPanelReset"]=epoch();});
  d.label("Hold a modifier when clicking a download link:",16,223,561);
  std::vector<std::string> keys={"None","Alt","Ctrl","Shift","Ctrl+Shift","Alt+Shift"};
  d.label("Capture any file type",34,258,158);auto force=d.combo(keys,str(draft,"CaptureForceKey","Alt"),207,254,101);
  d.label("Keep in browser",324,258,140);auto bypass=d.combo(keys,str(draft,"CaptureBypassKey","Ctrl"),474,254,103);
  d.label("Keys never override excluded sites or browser permissions.",34,289,543,28);
  d.label("Selected-link panel",16,333,172);auto selected=d.combo({"All allowed sites","Listed sites only","Off"},str(draft,"SelectedLinksMode","all")=="off"?"Off":str(draft,"SelectedLinksMode","all")=="sites"?"Listed sites only":"All allowed sites",207,329,213);
  auto mini=d.check("Compact",yes(draft,"SelectedLinksCompact"),435,329,142);
  d.label("Listed hosts",34,370,157);auto hosts=d.edit(str(draft,"SelectedLinkHosts"),207,366,370);
  d.button("Address exceptions...",16,410,184,[&]{Form e("Excluded web addresses",566,303,&d);e.init=[&]{e.label("One HTTP/HTTPS address pattern per line. * matches a path; *. before a host includes its subdomains.",14,14,538,43);auto value=e.edit(str(draft,"CaptureExcludedUrls"),14,65,538,172,false,true);e.accept=[&,value]{addressExceptions(text(value));draft["CaptureExcludedUrls"]=text(value);e.close();};e.button("Save",362,262,90,e.accept);e.button("Cancel",462,262,90,[&]{e.close(IDCANCEL);});};e.DoModal();});
  d.label("Site access and cookies are controlled by the browser. Changes reach open pages within 30 seconds.",214,410,363,48);
  d.accept=[&,capture,enabled,compact,hover,position,width,force,bypass,selected,hosts,mini]{draft["BrowserCaptureEnabled"]=d.checked(capture);draft["VideoPanelEnabled"]=d.checked(enabled);draft["VideoPanelCompact"]=d.checked(compact);draft["VideoPanelHover"]=d.checked(hover);draft["VideoPanelPosition"]=text(position);draft["VideoPanelMenuWidth"]=std::stoll(text(width));draft["CaptureForceKey"]=text(force);draft["CaptureBypassKey"]=text(bypass);draft["SelectedLinksMode"]=selected->GetCurSel()==2?"off":selected->GetCurSel()==1?"sites":"all";draft["SelectedLinkHosts"]=text(hosts);draft["SelectedLinksCompact"]=d.checked(mini);validateBrowserSettings(draft);prefs=draft;d.close();};
  d.button("OK",383,478,92,d.accept);d.button("Cancel",485,478,92,[&]{d.close(IDCANCEL);});
 };return d.DoModal()==IDOK;
}
}
