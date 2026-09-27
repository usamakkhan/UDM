// Included inside udm, after Form.
class ProtocolProxyForm:public Form {
public:
 using Form::Form;CWnd* protocolControl=nullptr;std::function<void()> selectionChanged;
 BOOL OnCommand(WPARAM w,LPARAM l)override{if(protocolControl&&LOWORD(w)==protocolControl->GetDlgCtrlID()&&HIWORD(w)==CBN_SELCHANGE){if(selectionChanged)selectionChanged();return TRUE;}return Form::OnCommand(w,l);}
};
inline void protocolProxyDialog(CWnd* owner,Json& prefs){
 ProtocolProxyForm d("Proxy settings by protocol",562,380,owner);auto draft=prefs;std::string selected="http";std::function<void()> save,load;bool changing=false;
 d.init=[&]{
  d.label("Protocol",14,18,97);auto protocol=d.combo({"HTTP","HTTPS","FTP"},"HTTP",117,14,429);
  d.label("Connection",14,58,97);auto mode=d.combo({"Use default settings","Use Windows proxy / PAC settings","Connect directly","Use a proxy server","Use a SOCKS5 proxy","Use a SOCKS4 / 4a proxy"},"Use default settings",117,54,429);
  d.label("Proxy host:port",14,98,97);auto address=d.edit("",117,94,429);
  d.label("Bypass hosts",14,138,97);auto bypass=d.edit("",117,134,429);
  d.label("User name",14,178,97);auto user=d.edit("",117,174,429);
  d.label("Password",14,218,97);auto password=d.edit("",117,214,429);password->SendMessage(EM_SETPASSWORDCHAR,L'*');
  d.label("Each protocol can use its own route and login. FTP through a proxy uses passive mode. An HTTP proxy must permit CONNECT to FTP control and data ports.",14,257,532,50);
  save=[&,mode,address,bypass,user,password]{auto next=draft;if(!next.contains("ProtocolProxies"))next["ProtocolProxies"]=Json::object();
   if(text(mode)=="Use default settings")next["ProtocolProxies"].erase(selected);
   else next["ProtocolProxies"][selected]={{"ProxyMode",text(mode)},{"Proxy",trim(text(address))},{"ProxyBypass",trim(text(bypass))},{"ProxyUser",text(user)},{"ProxySecret",protect(text(password))}};
   validateProtocolProxies(next);draft=std::move(next);
  };
  load=[&,mode,address,bypass,user,password]{auto table=draft.value("ProtocolProxies",Json::object());auto row=table.value(selected,Json::object());
   mode->SelectString(-1,cs(str(row,"ProxyMode","Use default settings")));address->SetWindowText(cs(str(row,"Proxy")));bypass->SetWindowText(cs(str(row,"ProxyBypass")));user->SetWindowText(cs(str(row,"ProxyUser")));password->SetWindowText(cs(reveal(str(row,"ProxySecret"))));
  };
  d.protocolControl=protocol;load();d.selectionChanged=[&,protocol]{auto next=lower(text(protocol));if(next==selected||changing)return;changing=true;
   try{save();selected=next;load();}catch(const std::exception& e){protocol->SetCurSel(selected=="http"?0:selected=="https"?1:2);MessageBoxW(d.m_hWnd,wide(e.what()).c_str(),L"Proxy settings",MB_OK|MB_ICONWARNING);}changing=false;
  };
  d.accept=[&]{save();prefs["ProtocolProxies"]=draft.value("ProtocolProxies",Json::object());d.close();};
  d.defaultButton(d.button("OK",348,334,92,d.accept));d.button("Cancel",454,334,92,[&]{d.close(IDCANCEL);});
 };d.DoModal();
}
