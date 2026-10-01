// Included inside MainWindow.
 CToolBarCtrl toolbar;CMenu toolbarMenu;CImageList toolbarNormal,toolbarHot,toolbarDisabled;
 bool toolbarBuilding=false,toolbarAdjusting=false;std::vector<int> toolbarBefore;std::vector<fs::path> toolbarSkins;std::string toolbarSkinError,toolbarSkinName;int toolbarButtonHeight=60,toolbarButtonWidth=67;
 TBBUTTON toolbarDefinition(int index) {
  TBBUTTON button{};if(index<0){button.iBitmap=px(8);button.fsStyle=BTNS_SEP;return button;}
  auto p=preferences();button.idCommand=CMD_ADD+index;button.iBitmap=str(p,"ToolbarStyle","Icons and text")=="Text only"?I_IMAGENONE:index;button.fsState=TBSTATE_ENABLED;
  button.fsStyle=BTNS_BUTTON|BTNS_SHOWTEXT|((index==8||index==9)?BTNS_DROPDOWN:0);button.dwData=(DWORD_PTR)index;button.iString=(INT_PTR)toolbarLabel(index);return button;
 }
 std::vector<int> currentToolbarLayout(){
  std::vector<int> values;for(int i=0;i<toolbar.GetButtonCount();++i){TBBUTTON b{};if(!toolbar.GetButton(i,&b))throw std::runtime_error("Cannot read toolbar layout.");values.push_back(b.fsStyle&BTNS_SEP?-1:b.idCommand-CMD_ADD);}
  if(!validToolbarLayout(Json(values)))throw std::runtime_error("Toolbar supports at most 64 entries with each command once.");return values;
 }
 void setToolbarButtons(const std::vector<int>& items){
  bool prior=toolbarBuilding;toolbarBuilding=true;while(toolbar.GetButtonCount()>0)toolbar.DeleteButton(0);
  for(int i:items){auto button=toolbarDefinition(i);if(!toolbar.SendMessage(TB_ADDBUTTONSW,1,(LPARAM)&button)){toolbarBuilding=prior;throw std::runtime_error("Cannot create toolbar buttons.");}}
  toolbar.SetMaxTextRows(str(preferences(),"ToolbarStyle","Icons and text")=="Icons only"?0:2);toolbar.SetButtonWidth(toolbarButtonWidth,toolbarButtonWidth);toolbar.SetButtonSize(CSize(toolbarButtonWidth,toolbarButtonHeight));
  toolbarBuilding=prior;for(int i=0;i<ToolbarCommandCount;++i)toolbar.EnableButton(CMD_ADD+i,commandEnabled(CMD_ADD+i));
 }
 void saveToolbarButtons(){
  auto previous=toolbarLayout(preferences());try{auto value=currentToolbarLayout();if(value!=previous){auto p=preferences();saveToolbarLayout(p,value);manager.setSettings(p);}}
  catch(...){setToolbarButtons(previous);throw;}
  CRect area;GetClientRect(&area);layout(area.Width(),area.Height());
 }
 void loadToolbarImages(){
  auto prefs=preferences();auto size=str(prefs,"ToolbarSize","Large");int dpi=GetDpiForWindow(m_hWnd),w=MulDiv(size=="Small"?16:size=="Medium"?24:32,dpi,96),h=w;
  std::optional<ToolbarSkin> skin;std::string group;toolbarSkinError.clear();toolbarSkinName.clear();
  if(!str(prefs,"ToolbarSkin").empty())try{skin=loadToolbarSkin(fs::path(wide(str(prefs,"ToolbarSkin"))));toolbarSkinName=skin->name;group=toolbarSkinGroup(*skin,size,dpi);
   const auto& base=skin->images.at(size=="Small"&&skin->images.count("small")?"small":"large");double factor=size=="Medium"?0.75:1.0;
   w=std::clamp((int)std::lround(base.width/ToolbarSkinColumns*factor*dpi/96.0),1,512);h=std::clamp((int)std::lround(base.height*factor*dpi/96.0),1,512);
  }catch(const std::exception& e){toolbarSkinError=e.what();skin.reset();}
  CImageList normal,hot,disabled;if(!normal.Create(w,h,ILC_COLOR32,ToolbarCommandCount,0)||!hot.Create(w,h,ILC_COLOR32,ToolbarCommandCount,0)||!disabled.Create(w,h,ILC_COLOR32,ToolbarCommandCount,0))throw std::runtime_error("Cannot create toolbar images.");
  auto add=[&](CImageList& list,const Bytes& pixels){CBitmap bitmap;bitmap.Attach(toolbarDib(pixels,w,h));if(list.Add(&bitmap,(CBitmap*)nullptr)<0)throw std::runtime_error("Cannot add toolbar image.");};
  for(int i=0;i<ToolbarCommandCount;++i){
   ToolbarBitmap builtIn;if(!skin)builtIn=loadToolbarPng(appDir()/L"assets"/(wide(toolbarAsset(i))+L".png"));
   for(int state=0;state<3;++state){const ToolbarBitmap* input=&builtIn;int effect=state;
    if(skin){auto key=group+(state==1?"hot":state==2?"disabled":"");auto at=skin->images.find(key);if(at!=skin->images.end()){input=&at->second;effect=0;}else input=&skin->images.at(group);}
    auto pixels=toolbarFrame(*input,skin?i:0,skin?ToolbarSkinColumns:1,w,h,effect);add(state==0?normal:state==1?hot:disabled,pixels);
   }
  }
  toolbar.SetImageList(&normal);toolbar.SetHotImageList(&hot);toolbar.SetDisabledImageList(&disabled);
  toolbarNormal.DeleteImageList();toolbarHot.DeleteImageList();toolbarDisabled.DeleteImageList();toolbarNormal.Attach(normal.Detach());toolbarHot.Attach(hot.Detach());toolbarDisabled.Attach(disabled.Detach());
  const auto style=str(prefs,"ToolbarStyle","Icons and text");toolbar.SetBitmapSize(CSize(w,h));toolbar.SetMaxTextRows(style=="Icons only"?0:2);
  CClientDC dc(this);auto fontBefore=dc.SelectObject(&font);TEXTMETRIC tm{};dc.GetTextMetrics(&tm);dc.SelectObject(fontBefore);
  toolbarButtonHeight=style=="Text only"?std::max<int>(px(33),tm.tmHeight+px(14)):h+px(10)+(style=="Icons and text"?tm.tmHeight*2:0);
  // The themed control adds its own border and dropdown widths.
  toolbarButtonWidth=style=="Text only"?px(98):style=="Icons only"?w+px(18):std::max(px(61),w+px(14));toolbar.SetButtonWidth(toolbarButtonWidth,toolbarButtonWidth);
 }
 void rebuildToolbar(){
  if(!toolbar.GetSafeHwnd())return;toolbarBuilding=true;
  try{toolbar.SetFont(&font);loadToolbarImages();setToolbarButtons(toolbarLayout(preferences()));toolbarBuilding=false;}catch(...){toolbarBuilding=false;throw;}
 }
 int layoutToolbar(int width){
  if(!toolbar.GetSafeHwnd())return 0;const bool shown=!yes(preferences(),"HideToolbar")&&toolbar.GetButtonCount()>0;::ShowWindow(toolbar.GetSafeHwnd(),shown?SW_SHOW:SW_HIDE);
  if(!shown)return 0;
  toolbar.MoveWindow(0,0,std::max(1,width),toolbarButtonHeight,FALSE);toolbar.AutoSize();int bottom=0;
  for(int i=0;i<toolbar.GetButtonCount();++i){CRect r;toolbar.GetItemRect(i,&r);bottom=std::max<int>(bottom,r.bottom);}
  bottom=std::max(bottom,toolbarButtonHeight)+px(3);toolbar.MoveWindow(0,0,std::max(1,width),bottom,FALSE);toolbar.Invalidate(FALSE);return bottom;
 }
 void createToolbar(){
  if(!toolbar.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|TBSTYLE_FLAT|TBSTYLE_TOOLTIPS|TBSTYLE_WRAPABLE|CCS_ADJUSTABLE|CCS_NORESIZE|CCS_NOPARENTALIGN|CCS_NODIVIDER,CRect(0,0,1,1),this,506))throw std::runtime_error("Cannot create the download toolbar.");
  toolbar.SetButtonStructSize(sizeof(TBBUTTON));toolbar.SendMessage(CCM_SETUNICODEFORMAT,TRUE);toolbar.SetExtendedStyle(TBSTYLE_EX_DRAWDDARROWS|TBSTYLE_EX_MIXEDBUTTONS);
  toolbarMenu.CreatePopupMenu();auto view=menu.GetSubMenu(3);view->DeleteMenu(CMD_TOOLBAR,MF_BYCOMMAND);view->InsertMenuW(2,MF_BYPOSITION|MF_POPUP,(UINT_PTR)toolbarMenu.GetSafeHmenu(),L"Toolbar");
 }
 void fillToolbarMenu(CMenu& target){
  while(target.GetMenuItemCount()>0)target.DeleteMenu(0,MF_BYPOSITION);auto p=preferences();
  auto item=[&](UINT id,const wchar_t* title,bool checked=false){target.AppendMenuW(MF_STRING|(checked?MF_CHECKED:0),id,title);};
  item(CMD_TOOLBAR_SMALL,L"Small buttons",str(p,"ToolbarSize","Large")=="Small");item(CMD_TOOLBAR_MEDIUM,L"Medium buttons",str(p,"ToolbarSize","Large")=="Medium");item(CMD_TOOLBAR_LARGE,L"Large buttons",str(p,"ToolbarSize","Large")=="Large");target.AppendMenuW(MF_SEPARATOR);
  item(CMD_TOOLBAR_ICONSTEXT,L"Icons and text",str(p,"ToolbarStyle","Icons and text")=="Icons and text");item(CMD_TOOLBAR_ICONSONLY,L"Icons only",str(p,"ToolbarStyle","Icons and text")=="Icons only");item(CMD_TOOLBAR_TEXTONLY,L"Text only",str(p,"ToolbarStyle","Icons and text")=="Text only");target.AppendMenuW(MF_SEPARATOR);
  item(CMD_TOOLBAR_BUILTIN,L"UDM icons",str(p,"ToolbarSkin").empty());
  toolbarSkins=toolbarSkinFiles({appDir()/L"Toolbar",manager.root/L"Toolbar"});auto current=fs::path(wide(str(p,"ToolbarSkin")));
  if(!current.empty()&&std::find(toolbarSkins.begin(),toolbarSkins.end(),current)==toolbarSkins.end()&&toolbarSkins.size()<64)toolbarSkins.push_back(current);
  for(size_t i=0;i<toolbarSkins.size();++i){auto label=utf8(toolbarSkins[i].stem().wstring());if(toolbarSkins[i]==current&&!toolbarSkinName.empty())label=toolbarSkinName;item(31000+(UINT)i,cs(label),toolbarSkins[i]==current);}
  if(!toolbarSkinError.empty())target.AppendMenuW(MF_STRING|MF_GRAYED,0,L"Saved skin unavailable; using UDM icons");
  item(CMD_TOOLBAR_LOAD,L"Load toolbar skin...");item(CMD_TOOLBAR_FOLDER,L"Open toolbar folder");item(CMD_TOOLBAR_RELOAD,L"Reload toolbar skins");target.AppendMenuW(MF_SEPARATOR);
  item(CMD_TOOLBAR,L"Customize...");item(CMD_TOOLBAR_HIDE,L"Hide toolbar",yes(p,"HideToolbar"));
 }
 void toolbarPopup(CPoint point){
  if(point.x<0){CRect r;toolbar.GetWindowRect(&r);point=CPoint(r.left+px(8),r.bottom);}
  CMenu popup;popup.CreatePopupMenu();fillToolbarMenu(popup);auto id=popup.TrackPopupMenu(TPM_RETURNCMD|TPM_NONOTIFY|TPM_RIGHTBUTTON,point.x,point.y,this);if(id)command(id);
 }
 void drawDarkToolbarArrow(NMTBCUSTOMDRAW* draw){
  if(draw->nmcd.dwItemSpec!=CMD_STARTQUEUE&&draw->nmcd.dwItemSpec!=CMD_STOPQUEUE)return;
  CDC dc;dc.Attach(draw->nmcd.hdc);const auto& r=draw->nmcd.rc;int x=r.right-px(8),y=(r.top+r.bottom)/2;
  dc.FillSolidRect(x-px(5),y-px(4),px(10)+1,px(8)+1,(draw->nmcd.uItemState&CDIS_HOT)?RGB(64,71,80):uiBackground());
  auto color=(draw->nmcd.uItemState&CDIS_DISABLED)?RGB(148,151,157):uiForeground();CBrush brush(color);CPen pen(PS_SOLID,1,color);
  auto oldBrush=dc.SelectObject(&brush);auto oldPen=dc.SelectObject(&pen);POINT points[]={{x-px(3),y-1},{x+px(3),y-1},{x,y+px(2)}};dc.Polygon(points,3);
  dc.SelectObject(oldBrush);dc.SelectObject(oldPen);dc.Detach();
 }
 bool toolbarMenuAction(UINT id){
  if(!((id>=CMD_TOOLBAR_SMALL&&id<=CMD_TOOLBAR_RELOAD)||(id>=31000&&id<31064)))return false;
  auto p=preferences();if(id>=31000){auto index=id-31000;if(index>=toolbarSkins.size())return true;loadToolbarSkin(toolbarSkins[index]);p["ToolbarSkin"]=utf8(toolbarSkins[index].wstring());}
  else switch(id){
   case CMD_TOOLBAR_SMALL:p["ToolbarSize"]="Small";break;case CMD_TOOLBAR_MEDIUM:p["ToolbarSize"]="Medium";break;case CMD_TOOLBAR_LARGE:p["ToolbarSize"]="Large";break;
   case CMD_TOOLBAR_ICONSTEXT:p["ToolbarStyle"]="Icons and text";break;case CMD_TOOLBAR_ICONSONLY:p["ToolbarStyle"]="Icons only";break;case CMD_TOOLBAR_TEXTONLY:p["ToolbarStyle"]="Text only";break;
   case CMD_TOOLBAR_HIDE:p["HideToolbar"]=!yes(p,"HideToolbar");break;case CMD_TOOLBAR_BUILTIN:p["ToolbarSkin"]="";break;
   case CMD_TOOLBAR_FOLDER:{auto path=manager.root/L"Toolbar";fs::create_directories(path);openFile(this,path);return true;}
   case CMD_TOOLBAR_RELOAD:applyAppearance();return true;
   case CMD_TOOLBAR_LOAD:{CFileDialog chooser(TRUE,L"tbi",nullptr,OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST,L"Toolbar information|*.tbi||",this);if(chooser.DoModal()!=IDOK)return true;auto path=fs::path((LPCWSTR)chooser.GetPathName());loadToolbarSkin(path);p["ToolbarSkin"]=utf8(path.wstring());break;}
  }
  manager.setSettings(p);applyAppearance();return true;
 }
 BOOL toolbarNotification(NMHDR* header,LRESULT* result){
  if(header->hwndFrom!=toolbar.GetSafeHwnd())return FALSE;*result=0;
  try{switch(header->code){
   case TBN_BEGINADJUST:toolbarAdjusting=true;toolbarBefore=currentToolbarLayout();return TRUE;
   case TBN_INITCUSTOMIZE:*result=TBNRF_HIDEHELP;return TRUE;
   case TBN_QUERYINSERT:*result=TRUE;return TRUE;
   case TBN_QUERYDELETE:*result=TRUE;return TRUE;
   case TBN_GETBUTTONINFOW:{auto info=(NMTOOLBARW*)header;if(info->iItem>=0&&info->iItem<ToolbarCommandCount){info->tbButton=toolbarDefinition(info->iItem);if(info->pszText&&info->cchText>0)wcsncpy_s(info->pszText,info->cchText,toolbarLabel(info->iItem),_TRUNCATE);*result=TRUE;}return TRUE;}
   case TBN_TOOLBARCHANGE:if(!toolbarBuilding){if(!toolbarAdjusting)saveToolbarButtons();else{CRect area;GetClientRect(&area);layout(area.Width(),area.Height());}}return TRUE;
   case TBN_RESET:setToolbarButtons(defaultToolbarLayout());{CRect area;GetClientRect(&area);layout(area.Width(),area.Height());}return TRUE;
   case TBN_ENDADJUST:toolbarAdjusting=false;saveToolbarButtons();toolbarBefore.clear();return TRUE;
   case TBN_DROPDOWN:{auto info=(NMTOOLBARW*)header;if(info->iItem==CMD_STARTQUEUE||info->iItem==CMD_STOPQUEUE){CRect r;toolbar.GetRect(info->iItem,&r);CPoint point(r.left,r.bottom);toolbar.ClientToScreen(&point);queuePopup(info->iItem==CMD_STARTQUEUE,point);}*result=TBDDRET_DEFAULT;return TRUE;}
   case TBN_GETINFOTIPW:{auto info=(NMTBGETINFOTIPW*)header;auto index=info->iItem-CMD_ADD;if(index>=0&&index<ToolbarCommandCount&&info->pszText&&info->cchTextMax>0)wcsncpy_s(info->pszText,info->cchTextMax,toolbarLabel(index),_TRUNCATE);return TRUE;}
   case NM_CUSTOMDRAW:{auto draw=(NMTBCUSTOMDRAW*)header;if(uiDark){if(draw->nmcd.dwDrawStage==CDDS_PREPAINT){CDC dc;dc.Attach(draw->nmcd.hdc);CRect r;toolbar.GetClientRect(&r);dc.FillSolidRect(r,uiBackground());dc.Detach();*result=CDRF_NOTIFYITEMDRAW;}else if(draw->nmcd.dwDrawStage==CDDS_ITEMPREPAINT){draw->clrText=uiForeground();draw->clrBtnFace=uiBackground();draw->clrBtnHighlight=RGB(64,71,80);draw->clrHighlightHotTrack=RGB(64,71,80);*result=TBCDRF_USECDCOLORS|TBCDRF_HILITEHOTTRACK|CDRF_NOTIFYPOSTPAINT;}else if(draw->nmcd.dwDrawStage==CDDS_ITEMPOSTPAINT)drawDarkToolbarArrow(draw);}return TRUE;}
  }}catch(const std::exception& e){toolbarAdjusting=false;error(this,e);return TRUE;}
  return FALSE;
 }
 afx_msg void OnInitMenuPopup(CMenu* popup,UINT index,BOOL systemMenu){if(popup&&popup->GetSafeHmenu()==toolbarMenu.GetSafeHmenu())fillToolbarMenu(*popup);CFrameWnd::OnInitMenuPopup(popup,index,systemMenu);}
 afx_msg void OnUpdateToolbarSkin(CCmdUI* ui){ui->Enable(TRUE);}
