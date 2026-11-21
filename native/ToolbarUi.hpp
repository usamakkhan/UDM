// Included inside MainWindow.
#define UDM_TOOLBAR_SETTINGS_COMMAND_AVAILABLE
 const AppearanceResult* toolbarPreparedRead=nullptr;
 static int toolbarCommand(int index){return index==11?CMD_SHARE:CMD_ADD+index;}
 static int toolbarIndex(int command){return command==CMD_SHARE?11:(command>=CMD_ADD&&command<=CMD_GRABBER?command-CMD_ADD:-2);}
 ToolbarSourceCache toolbarSources;ToolbarRasterCache toolbarRasters;std::optional<ToolbarRasterKey> toolbarImageKey;
#ifdef UDM_TOOLBAR_COMPONENT_TEST
 std::function<void(int)> toolbarSourceObserver;std::function<void(const ToolbarRasterKey&)> toolbarRasterObserver;
#endif
 void invalidateToolbarImages(){toolbarSources.clear();toolbarRasters.clear();toolbarImageKey.reset();}
 std::shared_ptr<const ToolbarBitmap> builtInToolbarSource(int index){return toolbarSources.get(index,[&](int value){
#ifdef UDM_TOOLBAR_COMPONENT_TEST
  if(toolbarSourceObserver)toolbarSourceObserver(value);
#endif
  return loadBuiltInToolbarPng(appDir()/L"assets",value);});}
 CToolBarCtrl toolbar;CMenu toolbarMenu;CImageList toolbarNormal,toolbarHot,toolbarDisabled;
 bool toolbarReferenceHeight=false;
 bool toolbarBuilding=false,toolbarAdjusting=false;std::vector<int> toolbarBefore;std::vector<fs::path> toolbarSkins;std::string toolbarSkinError,toolbarSkinName;int toolbarButtonHeight=60,toolbarButtonWidth=67;
 TBBUTTON toolbarDefinition(int index) {
  TBBUTTON button{};if(index<0){button.iBitmap=px(8);button.fsStyle=BTNS_SEP;return button;}
  auto p=repaintAppearancePreferences();button.idCommand=toolbarCommand(index);button.iBitmap=str(p,"ToolbarStyle","Icons and text")=="Text only"?I_IMAGENONE:index;button.fsState=TBSTATE_ENABLED;
  button.fsStyle=BTNS_BUTTON|BTNS_SHOWTEXT|((index==8||index==9)?BTNS_DROPDOWN:0);button.dwData=(DWORD_PTR)index;button.iString=(INT_PTR)toolbarLabel(index);return button;
 }
 std::vector<int> currentToolbarLayout(){
  std::vector<int> values;for(int i=0;i<toolbar.GetButtonCount();++i){TBBUTTON b{};if(!toolbar.GetButton(i,&b))throw std::runtime_error("Cannot read toolbar layout.");values.push_back(b.fsStyle&BTNS_SEP?-1:toolbarIndex(b.idCommand));}
  if(!validToolbarLayout(Json(values)))throw std::runtime_error("Toolbar supports at most 64 entries with each command once.");return values;
 }
 void setToolbarButtons(const std::vector<int>& items){
  auto settings=repaintAppearancePreferences();struct Restore{const Json*& slot;const Json* previous;~Restore(){slot=previous;}} restore{appearanceReadSnapshot,appearanceReadSnapshot};appearanceReadSnapshot=&settings;
  bool prior=toolbarBuilding;toolbarBuilding=true;while(toolbar.GetButtonCount()>0)toolbar.DeleteButton(0);
  for(int i:items){auto button=toolbarDefinition(i);if(!toolbar.SendMessage(TB_ADDBUTTONSW,1,(LPARAM)&button)){toolbarBuilding=prior;throw std::runtime_error("Cannot create toolbar buttons.");}}
  toolbar.SetMaxTextRows(str(appearancePreferences(),"ToolbarStyle","Icons and text")=="Icons only"?0:1);toolbar.SetButtonWidth(toolbarButtonWidth,toolbarButtonWidth);toolbar.SetButtonSize(CSize(toolbarButtonWidth,toolbarButtonHeight));
  toolbarBuilding=prior;updateToolbarCommands();
 }
 void restoreToolbarLayoutOwned(const std::vector<int>& value){
  auto settings=renderedAppearance;saveToolbarLayout(settings,value);struct Restore{const Json*& slot;const Json* previous;~Restore(){slot=previous;}} restore{appearanceReadSnapshot,appearanceReadSnapshot};appearanceReadSnapshot=&settings;setToolbarButtons(value);CRect area;GetClientRect(&area);layout(area.Width(),area.Height());renderedAppearance=std::move(settings);
 }
 void saveToolbarButtons(){
  if(renderedAppearance.empty())return;
  auto previous=toolbarLayout(renderedAppearance);try{auto value=currentToolbarLayout();if(value!=previous){AppearanceDraft draft;draft.change=AppearanceChange::ToolbarLayout;draft.toolbarButtons=std::move(value);startAppearanceCommand(std::move(draft));return;}}
  catch(...){restoreToolbarLayoutOwned(previous);throw;}
  CRect area;GetClientRect(&area);layout(area.Width(),area.Height());
 }
 void loadToolbarImages(){
  auto prefs=appearancePreferences();auto size=str(prefs,"ToolbarSize","Large");int dpi=windowDpi(),w=MulDiv(size=="Small"?16:size=="Medium"?24:32,dpi,96),h=w;
  std::shared_ptr<const ToolbarSkin> skin;std::string group;toolbarSkinError.clear();toolbarSkinName.clear();
  if(!str(prefs,"ToolbarSkin").empty())try{if(toolbarPreparedRead&&str(toolbarPreparedRead->settings,"ToolbarSkin")==str(prefs,"ToolbarSkin")){skin=toolbarPreparedRead->toolbarSkin;if(!skin)throw std::runtime_error(toolbarPreparedRead->toolbarSkinError.empty()?"Cannot prepare the saved toolbar skin.":toolbarPreparedRead->toolbarSkinError);}else skin=std::make_shared<const ToolbarSkin>(loadToolbarSkin(fs::path(wide(str(prefs,"ToolbarSkin")))));toolbarSkinName=skin->name;group=toolbarSkinGroup(*skin,size,dpi);
   if(!group.empty()){
    const auto& base=skin->images.at(group=="small"?"small":"large");double factor=size=="Medium"?0.75:1.0;
    w=std::clamp((int)std::lround(base.width/ToolbarSkinColumns*factor*dpi/96.0),1,512);h=std::clamp((int)std::lround(base.height*factor*dpi/96.0),1,512);
   }
  }catch(const std::exception& e){toolbarSkinError=e.what();skin.reset();}
  const bool customImages=skin&&!group.empty();
  const bool raised=!customImages&&(bool(skin)||str(prefs,"ToolbarLook","Neon")=="3D Style");
  const ToolbarRasterKey key{w,h,raised,toolbarSources.generation()};
  const bool reuse=!customImages&&toolbarImageKey&&*toolbarImageKey==key&&toolbarNormal.GetSafeHandle()&&toolbarHot.GetSafeHandle()&&toolbarDisabled.GetSafeHandle();
  if(!reuse){
   std::shared_ptr<const ToolbarRasterSet> cached;
   if(!customImages&&toolbarRasters.cacheable(key))cached=toolbarRasters.get(key,[&]{
#ifdef UDM_TOOLBAR_COMPONENT_TEST
    if(toolbarRasterObserver)toolbarRasterObserver(key);
#endif
    ToolbarRasterSet result;for(int i=0;i<ToolbarCommandCount;++i){auto source=builtInToolbarSource(i);for(int state=0;state<3;++state)result.pixels[i][state]=toolbarFrame(*source,0,1,w,h,state,raised);}
    if(toolbarSources.generation()!=key.generation)throw std::runtime_error("Toolbar images changed during preparation. Please reload again.");return result;
   });
   CImageList normal,hot,disabled;if(!normal.Create(w,h,ILC_COLOR32,ToolbarCommandCount,0)||!hot.Create(w,h,ILC_COLOR32,ToolbarCommandCount,0)||!disabled.Create(w,h,ILC_COLOR32,ToolbarCommandCount,0))throw std::runtime_error("Cannot create toolbar images.");
   auto add=[&](CImageList& list,const Bytes& pixels){CBitmap bitmap;bitmap.Attach(toolbarDib(pixels,w,h));if(list.Add(&bitmap,(CBitmap*)nullptr)<0)throw std::runtime_error("Cannot add toolbar image.");};
   for(int i=0;i<ToolbarCommandCount;++i){
    std::shared_ptr<const ToolbarBitmap> builtIn;if(!customImages&&!cached)builtIn=builtInToolbarSource(i);
    for(int state=0;state<3;++state){
     if(cached){add(state==0?normal:state==1?hot:disabled,cached->pixels[i][state]);continue;}
     const ToolbarBitmap* input=builtIn.get();int effect=state;
     if(customImages){auto name=group+(state==1?"hot":state==2?"disabled":"");auto at=skin->images.find(name);if(at!=skin->images.end()){input=&at->second;effect=0;}else input=&skin->images.at(group);}
     auto pixels=toolbarFrame(*input,customImages?i:0,customImages?ToolbarSkinColumns:1,w,h,effect,raised);add(state==0?normal:state==1?hot:disabled,pixels);
    }
   }
   if(!customImages&&toolbarSources.generation()!=key.generation)throw std::runtime_error("Toolbar images changed during preparation. Please reload again.");
   toolbar.SetImageList(&normal);toolbar.SetHotImageList(&hot);toolbar.SetDisabledImageList(&disabled);
   toolbarNormal.DeleteImageList();toolbarHot.DeleteImageList();toolbarDisabled.DeleteImageList();toolbarNormal.Attach(normal.Detach());toolbarHot.Attach(hot.Detach());toolbarDisabled.Attach(disabled.Detach());
   if(customImages)toolbarImageKey.reset();else toolbarImageKey=key;
  }else{toolbar.SetImageList(&toolbarNormal);toolbar.SetHotImageList(&toolbarHot);toolbar.SetDisabledImageList(&toolbarDisabled);}
  const auto style=str(prefs,"ToolbarStyle","Icons and text");toolbar.SetBitmapSize(CSize(w,h));toolbar.SetMaxTextRows(style=="Icons only"?0:1);
  CClientDC dc(this);auto fontBefore=dc.SelectObject(&font);TEXTMETRIC tm{};dc.GetTextMetrics(&tm);dc.SelectObject(fontBefore);
  toolbarButtonHeight=style=="Text only"?std::max<int>(px(33),tm.tmHeight+px(14)):h+px(10)+(style=="Icons and text"?tm.tmHeight:0);
  toolbarReferenceHeight=!skin&&size=="Large"&&style=="Icons and text";
  if(toolbarReferenceHeight)toolbarButtonHeight=std::max<int>(MulDiv(68,dpi,120),h+tm.tmHeight+MulDiv(8,dpi,120));
  // The themed control adds its own border and dropdown widths.
  toolbarButtonWidth=style=="Text only"?px(98):style=="Icons only"?w+px(18):std::max(px(55),w+px(14));toolbar.SetButtonWidth(toolbarButtonWidth,toolbarButtonWidth);
 }
 void rebuildToolbar(){
  if(!toolbar.GetSafeHwnd())return;toolbarBuilding=true;
  try{themeTooltip((HWND)toolbar.SendMessage(TB_GETTOOLTIPS));if(reinterpret_cast<HFONT>(toolbar.SendMessage(WM_GETFONT))!=font.GetSafeHandle())toolbar.SetFont(&font);loadToolbarImages();setToolbarButtons(toolbarLayout(appearancePreferences()));toolbarBuilding=false;}catch(...){toolbarBuilding=false;throw;}
 }
 int layoutToolbar(int width,const Json& settings){
  if(!toolbar.GetSafeHwnd())return 0;const bool shown=!yes(settings,"HideToolbar")&&toolbar.GetButtonCount()>0;::ShowWindow(toolbar.GetSafeHwnd(),shown?SW_SHOW:SW_HIDE);
  if(!shown)return 0;
  toolbar.MoveWindow(0,0,std::max(1,width),toolbarButtonHeight,FALSE);toolbar.AutoSize();int bottom=0;
  for(int i=0;i<toolbar.GetButtonCount();++i){CRect r;toolbar.GetItemRect(i,&r);bottom=std::max<int>(bottom,r.bottom);}
  // Native item rectangles can grow for font and theme metrics.
  const int padding=toolbarReferenceHeight?MulDiv(3,windowDpi(),120):px(3);
  bottom=std::max(bottom,toolbarButtonHeight)+padding;toolbar.MoveWindow(0,0,std::max(1,width),bottom,FALSE);toolbar.Invalidate(FALSE);return bottom;
 }
 void createToolbar(){
  if(!toolbar.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|TBSTYLE_FLAT|TBSTYLE_TOOLTIPS|CCS_ADJUSTABLE|CCS_NORESIZE|CCS_NOPARENTALIGN|CCS_NODIVIDER,CRect(0,0,1,1),this,506))throw std::runtime_error("Cannot create the download toolbar.");
  toolbar.SetButtonStructSize(sizeof(TBBUTTON));toolbar.SendMessage(CCM_SETUNICODEFORMAT,TRUE);toolbar.SetExtendedStyle(TBSTYLE_EX_DRAWDDARROWS|TBSTYLE_EX_MIXEDBUTTONS);
  toolbarMenu.CreatePopupMenu();auto view=menu.GetSubMenu(3);view->DeleteMenu(CMD_TOOLBAR,MF_BYCOMMAND);view->InsertMenuW(2,MF_BYPOSITION|MF_POPUP,(UINT_PTR)toolbarMenu.GetSafeHmenu(),L"&Toolbar");
 }
 void fillToolbarMenu(CMenu& target){
  while(target.GetMenuItemCount()>0)target.DeleteMenu(0,MF_BYPOSITION);auto p=repaintAppearancePreferences();
  auto item=[&](UINT id,const wchar_t* title,bool checked=false){target.AppendMenuW(MF_STRING|(checked?MF_CHECKED:0),id,title);};
  // IDM's order: Customize, sizes, Look for new toolbars, then the looks; UDM's own entries follow.
  item(CMD_TOOLBAR,L"&Customize...");target.AppendMenuW(MF_SEPARATOR);
  item(CMD_TOOLBAR_LARGE,L"&Large Buttons",str(p,"ToolbarSize","Large")=="Large");item(CMD_TOOLBAR_SMALL,L"&Small Buttons",str(p,"ToolbarSize","Large")=="Small");item(CMD_TOOLBAR_MEDIUM,L"&Medium Buttons",str(p,"ToolbarSize","Large")=="Medium");target.AppendMenuW(MF_SEPARATOR);
  item(CMD_TOOLBAR_DISCOVER,L"Look for &new toolbars (IDM gallery)");target.AppendMenuW(MF_SEPARATOR);
  const bool ownIcons=str(p,"ToolbarSkin").empty();item(CMD_TOOLBAR_NEON,L"Neon",ownIcons&&str(p,"ToolbarLook","Neon")!="3D Style");item(CMD_TOOLBAR_3D,L"3D Style",ownIcons&&str(p,"ToolbarLook","Neon")=="3D Style");target.AppendMenuW(MF_SEPARATOR);
  item(CMD_TOOLBAR_ICONSTEXT,L"Icons and text",str(p,"ToolbarStyle","Icons and text")=="Icons and text");item(CMD_TOOLBAR_ICONSONLY,L"Icons only",str(p,"ToolbarStyle","Icons and text")=="Icons only");item(CMD_TOOLBAR_TEXTONLY,L"Text only",str(p,"ToolbarStyle","Icons and text")=="Text only");target.AppendMenuW(MF_SEPARATOR);
  toolbarSkins=toolbarSkinFiles({appDir()/L"Toolbar",manager.root/L"Toolbar"});auto current=fs::path(wide(str(p,"ToolbarSkin")));
  if(!current.empty()&&std::find(toolbarSkins.begin(),toolbarSkins.end(),current)==toolbarSkins.end()&&toolbarSkins.size()<64)toolbarSkins.push_back(current);
  for(size_t i=0;i<toolbarSkins.size();++i){
   auto label=utf8(toolbarSkins[i].stem().wstring());
   try{label=toolbarInformation(toolbarSkins[i]).at("name");}catch(const std::exception&){if(toolbarSkins[i]==current&&!toolbarSkinName.empty())label=toolbarSkinName;}
   item(31000+(UINT)i,menuLabel(label).c_str(),toolbarSkins[i]==current);
  }
  if(!toolbarSkinError.empty())target.AppendMenuW(MF_STRING|MF_GRAYED,0,L"Saved skin unavailable; using UDM icons");
  item(CMD_TOOLBAR_LOAD,L"Load toolbar skin...");item(CMD_TOOLBAR_FOLDER,L"Open toolbar folder");item(CMD_TOOLBAR_RELOAD,L"Reload toolbar skins");target.AppendMenuW(MF_SEPARATOR);
  item(CMD_TOOLBAR_HIDE,L"Hide toolbar",yes(p,"HideToolbar"));
 }
 void toolbarPopup(CPoint point){
  if(appearanceCommandTask.valid())return;
  if(point.x<0){CRect r;toolbar.GetWindowRect(&r);point=CPoint(r.left+px(8),r.bottom);}
  CMenu popup;popup.CreatePopupMenu();fillToolbarMenu(popup);menuTheme.popup(popup.GetSafeHmenu(),uiDark);auto id=popup.TrackPopupMenu(TPM_RETURNCMD|TPM_NONOTIFY|TPM_RIGHTBUTTON,point.x,point.y,this);menuTheme.releasePopup(popup.GetSafeHmenu());if(id)command(id);
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
  if(!(id==CMD_TOOLBAR_DISCOVER||(id>=CMD_TOOLBAR_SMALL&&id<=CMD_TOOLBAR_3D)||(id>=31000&&id<31064)))return false;
  if(!commandEnabled(id))return true;AppearanceDraft draft;
  if(id>=31000){auto index=id-31000;if(index>=toolbarSkins.size())return true;draft.change=AppearanceChange::ToolbarSkin;draft.toolbarValue=utf8(toolbarSkins[index].wstring());}
  else switch(id){
   case CMD_TOOLBAR_SMALL:case CMD_TOOLBAR_MEDIUM:case CMD_TOOLBAR_LARGE:draft.change=AppearanceChange::ToolbarSize;draft.toolbarValue=id==CMD_TOOLBAR_SMALL?"Small":id==CMD_TOOLBAR_MEDIUM?"Medium":"Large";break;
   case CMD_TOOLBAR_ICONSTEXT:case CMD_TOOLBAR_ICONSONLY:case CMD_TOOLBAR_TEXTONLY:draft.change=AppearanceChange::ToolbarStyle;draft.toolbarValue=id==CMD_TOOLBAR_ICONSTEXT?"Icons and text":id==CMD_TOOLBAR_ICONSONLY?"Icons only":"Text only";break;
   case CMD_TOOLBAR_HIDE:draft.change=AppearanceChange::ToggleToolbar;break;
   case CMD_TOOLBAR_BUILTIN:draft.change=AppearanceChange::ToolbarSkin;break;
   case CMD_TOOLBAR_NEON:case CMD_TOOLBAR_3D:draft.change=AppearanceChange::ToolbarLook;draft.toolbarValue=id==CMD_TOOLBAR_NEON?"Neon":"3D Style";break;
   case CMD_TOOLBAR_FOLDER:{auto path=manager.root/L"Toolbar";fs::create_directories(path);openFile(this,path);return true;}
   case CMD_TOOLBAR_DISCOVER:openProductLink(m_hWnd,toolbarGallery);return true;
   case CMD_TOOLBAR_RELOAD:draft.change=AppearanceChange::ToolbarReload;break;
   case CMD_TOOLBAR_LOAD:{CFileDialog chooser(TRUE,L"tbi",nullptr,OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST,L"Toolbar information|*.tbi||",this);if(chooser.DoModal()!=IDOK)return true;draft.change=AppearanceChange::ToolbarSkin;draft.toolbarValue=utf8(std::wstring((LPCWSTR)chooser.GetPathName()));break;}
  }
  startAppearanceCommand(std::move(draft));return true;
 }

 BOOL toolbarNotification(NMHDR* header,LRESULT* result){
  if(header->hwndFrom!=toolbar.GetSafeHwnd())return FALSE;*result=0;
  if(appearanceCommandTask.valid()&&!toolbarBuilding&&header->code!=NM_CUSTOMDRAW){if(header->code==TBN_ENDADJUST)toolbarAdjusting=false;return TRUE;}
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
   case TBN_GETINFOTIPW:{auto info=(NMTBGETINFOTIPW*)header;auto index=toolbarIndex(info->iItem);if(index>=0&&index<ToolbarCommandCount&&info->pszText&&info->cchTextMax>0)wcsncpy_s(info->pszText,info->cchTextMax,toolbarLabel(index),_TRUNCATE);return TRUE;}
   case NM_CUSTOMDRAW:{auto draw=(NMTBCUSTOMDRAW*)header;if(uiDark){if(draw->nmcd.dwDrawStage==CDDS_PREPAINT){CDC dc;dc.Attach(draw->nmcd.hdc);CRect r;toolbar.GetClientRect(&r);dc.FillSolidRect(r,mainDarkSurface);dc.Detach();*result=CDRF_NOTIFYITEMDRAW;}else if(draw->nmcd.dwDrawStage==CDDS_ITEMPREPAINT){draw->clrText=uiForeground();draw->clrBtnFace=mainDarkSurface;draw->clrBtnHighlight=RGB(64,71,80);draw->clrHighlightHotTrack=RGB(64,71,80);*result=TBCDRF_USECDCOLORS|TBCDRF_HILITEHOTTRACK|CDRF_NOTIFYPOSTPAINT;}else if(draw->nmcd.dwDrawStage==CDDS_ITEMPOSTPAINT)drawDarkToolbarArrow(draw);}return TRUE;}
  }}catch(const std::exception& e){toolbarAdjusting=false;error(this,e);return TRUE;}
  return FALSE;
 }
 afx_msg void OnInitMenuPopup(CMenu* popup,UINT index,BOOL systemMenu){
  const bool queuePopup=!appearanceCommandTask.valid()&&!systemMenu&&updateQueuePopup(popup);
  if(!appearanceCommandTask.valid()&&popup&&popup->GetSafeHmenu()==toolbarMenu.GetSafeHmenu())fillToolbarMenu(*popup);
  {struct UpdateScope {bool& flag;bool previous;UpdateScope(bool& value,bool enabled):flag(value),previous(value){flag=enabled;}~UpdateScope(){flag=previous;}} updating(queuePopupUpdating,queuePopup);
   CFrameWnd::OnInitMenuPopup(popup,index,systemMenu);
  }
  if(!systemMenu&&popup)menuTheme.popup(popup->GetSafeHmenu(),uiDark);
 }
 afx_msg void OnUpdateToolbarSkin(CCmdUI* ui){ui->Enable(!appearanceCommandTask.valid());}
