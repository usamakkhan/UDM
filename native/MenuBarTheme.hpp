#pragma once
// Original owner painting; native menu handles, labels and command routing are retained.
class MenuBarTheme {
 struct Entry { HMENU menu=nullptr; UINT position=0,type=0; ULONG_PTR data=0; std::wstring text; bool popup=false; };
 std::vector<std::unique_ptr<Entry>> entries;
 CBrush background;
 CFont font;
 UINT dpi=96;
 const Entry* find(ULONG_PTR data)const{for(const auto& entry:entries)if(reinterpret_cast<ULONG_PTR>(entry.get())==data)return entry.get();return nullptr;}
 void restore(const Entry& entry){
  if(!IsMenu(entry.menu))return;MENUITEMINFOW current{sizeof(current)};current.fMask=MIIM_DATA;
  if(!GetMenuItemInfoW(entry.menu,entry.position,TRUE,&current)||current.dwItemData!=reinterpret_cast<ULONG_PTR>(&entry))return;
  current.fMask=MIIM_DATA|MIIM_FTYPE;current.dwItemData=entry.data;current.fType=entry.type;SetMenuItemInfoW(entry.menu,entry.position,TRUE,&current);
 }
public:
 void releasePopup(HMENU menu){for(auto it=entries.begin();it!=entries.end();)if((*it)->popup&&(*it)->menu==menu){restore(**it);it=entries.erase(it);}else ++it;}
 void popup(HMENU menu,bool dark){
  releasePopup(menu);if(!menu)return;MENUINFO info{sizeof(info)};info.fMask=MIM_BACKGROUND;info.hbrBack=dark?(HBRUSH)background.GetSafeHandle():nullptr;SetMenuInfo(menu,&info);if(!dark)return;
  for(int i=0;i<GetMenuItemCount(menu);++i){MENUITEMINFOW item{sizeof(item)};item.fMask=MIIM_DATA|MIIM_FTYPE|MIIM_STRING;GetMenuItemInfoW(menu,i,TRUE,&item);if(item.fType&MFT_OWNERDRAW)continue;
   auto entry=std::make_unique<Entry>();entry->menu=menu;entry->position=i;entry->type=item.fType;entry->data=item.dwItemData;entry->popup=true;
   std::vector<wchar_t> label(item.cch+1);item.dwTypeData=label.data();item.cch=(UINT)label.size();GetMenuItemInfoW(menu,i,TRUE,&item);entry->text=label.data();
   item.fMask=MIIM_FTYPE|MIIM_DATA;item.fType|=MFT_OWNERDRAW;item.dwItemData=reinterpret_cast<ULONG_PTR>(entry.get());if(SetMenuItemInfoW(menu,i,TRUE,&item))entries.push_back(std::move(entry));
  }
 }
 void apply(HWND window,HMENU menu,bool dark,UINT nextDpi){
  for(const auto& entry:entries){restore(*entry);if(entry->popup&&IsMenu(entry->menu)){MENUINFO old{sizeof(old)};old.fMask=MIM_BACKGROUND;SetMenuInfo(entry->menu,&old);}}
  entries.clear();dpi=nextDpi?nextDpi:96;
  MENUINFO info{sizeof(info)};info.fMask=MIM_BACKGROUND;
  if(dark){
   if(!background.GetSafeHandle())background.CreateSolidBrush(RGB(32,32,32));info.hbrBack=(HBRUSH)background.GetSafeHandle();
   NONCLIENTMETRICSW metrics{sizeof(metrics)};SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS,sizeof(metrics),&metrics,0,dpi);font.DeleteObject();font.CreateFontIndirectW(&metrics.lfMenuFont);
   for(int i=0;i<GetMenuItemCount(menu);++i){
    MENUITEMINFOW item{sizeof(item)};item.fMask=MIIM_DATA|MIIM_FTYPE|MIIM_STRING;GetMenuItemInfoW(menu,i,TRUE,&item);
    if(item.fType&(MFT_OWNERDRAW|MFT_SEPARATOR))continue;
    auto entry=std::make_unique<Entry>();entry->menu=menu;entry->position=i;entry->type=item.fType;entry->data=item.dwItemData;
    std::vector<wchar_t> label(item.cch+1);item.dwTypeData=label.data();item.cch=(UINT)label.size();GetMenuItemInfoW(menu,i,TRUE,&item);entry->text=label.data();
    item.fMask=MIIM_FTYPE|MIIM_DATA;item.fType|=MFT_OWNERDRAW;item.dwItemData=reinterpret_cast<ULONG_PTR>(entry.get());
    if(SetMenuItemInfoW(menu,i,TRUE,&item))entries.push_back(std::move(entry));
   }
  }else info.hbrBack=nullptr;
  if(menu){SetMenuInfo(menu,&info);DrawMenuBar(window);}
 }
 bool measure(MEASUREITEMSTRUCT* item){
  if(!item||item->CtlType!=ODT_MENU)return false;auto entry=find(item->itemData);if(!entry)return false;
  if(entry->popup&&(entry->type&MFT_SEPARATOR)){item->itemWidth=MulDiv(60,dpi,96);item->itemHeight=MulDiv(8,dpi,96);return true;}
  CWindowDC dc(nullptr);auto old=dc.SelectObject(&font);CRect text(0,0,0,0);dc.DrawText(entry->text.c_str(),text,DT_CALCRECT|DT_SINGLELINE);dc.SelectObject(old);
  if(entry->popup){int labelWidth=0,shortcutWidth=0;auto selected=dc.SelectObject(&font);for(const auto& peer:entries)if(peer->menu==entry->menu){auto tab=peer->text.find(L'\t');CRect a(0,0,0,0),b(0,0,0,0);dc.DrawText(peer->text.substr(0,tab).c_str(),a,DT_CALCRECT|DT_SINGLELINE);if(tab!=std::wstring::npos)dc.DrawText(peer->text.substr(tab+1).c_str(),b,DT_CALCRECT|DT_SINGLELINE);labelWidth=std::max(labelWidth,a.Width());shortcutWidth=std::max(shortcutWidth,b.Width());}dc.SelectObject(selected);text.right=labelWidth+shortcutWidth+MulDiv(shortcutWidth?24:0,dpi,96);}
  item->itemWidth=text.Width()+MulDiv(entry->popup?52:12,dpi,96);item->itemHeight=std::max(text.Height()+MulDiv(4,dpi,96),GetSystemMetricsForDpi(SM_CYMENU,dpi)-1);return true;
 }
 bool draw(DRAWITEMSTRUCT* item){
  if(!item||item->CtlType!=ODT_MENU)return false;auto entry=find(item->itemData);if(!entry)return false;
  auto dc=CDC::FromHandle(item->hDC);const int saved=dc->SaveDC();CRect area(item->rcItem);bool submenu=false;
  const bool hot=(item->itemState&(ODS_SELECTED|ODS_HOTLIGHT))!=0,disabled=(item->itemState&(ODS_DISABLED|ODS_GRAYED))!=0;
  dc->FillSolidRect(area,hot?RGB(64,67,72):RGB(32,32,32));dc->SetBkMode(TRANSPARENT);dc->SetTextColor(disabled?RGB(145,145,145):RGB(235,235,235));dc->SelectObject(&font);
  if(entry->popup){
   if(entry->type&MFT_SEPARATOR){const int inset=MulDiv(7,dpi,96);dc->FillSolidRect(area.left+inset,area.CenterPoint().y,area.Width()-2*inset,1,RGB(64,67,72));}
   else{
    MENUITEMINFOW state{sizeof(state)};state.fMask=MIIM_STATE|MIIM_SUBMENU;GetMenuItemInfoW(entry->menu,entry->position,TRUE,&state);
    const auto foreground=disabled?RGB(145,145,145):RGB(235,235,235);const int gutter=MulDiv(24,dpi,96),radius=std::max(2,MulDiv(3,dpi,96)),cy=area.CenterPoint().y;
    if(state.fState&MFS_CHECKED){CPen pen(PS_SOLID,std::max(1,MulDiv(2,dpi,96)),foreground);auto previous=dc->SelectObject(&pen);int x=area.left+gutter/2;if(entry->type&MFT_RADIOCHECK){CBrush brush(foreground);auto oldBrush=dc->SelectObject(&brush);dc->Ellipse(x-radius,cy-radius,x+radius+1,cy+radius+1);dc->SelectObject(oldBrush);}else{dc->MoveTo(x-radius,cy);dc->LineTo(x,cy+radius);dc->LineTo(x+radius*2,cy-radius*2);}dc->SelectObject(previous);}
    // Paint one contrasting arrow and suppress the system's black overdraw below.
    submenu=state.hSubMenu!=nullptr;
    if(submenu){CPen pen(PS_SOLID,1,foreground);CBrush brush(foreground);auto oldPen=dc->SelectObject(&pen);auto oldBrush=dc->SelectObject(&brush);int x=area.right-MulDiv(10,dpi,96);POINT points[]={{x-radius,cy-radius},{x,cy},{x-radius,cy+radius}};dc->Polygon(points,3);dc->SelectObject(oldBrush);dc->SelectObject(oldPen);}
    CRect label=area;label.left+=gutter;label.right-=MulDiv(20,dpi,96);auto tab=entry->text.find(L'\t');UINT flags=DT_VCENTER|DT_SINGLELINE|((item->itemState&ODS_NOACCEL)?DT_HIDEPREFIX:0);dc->DrawText(entry->text.substr(0,tab).c_str(),label,flags|DT_LEFT);if(tab!=std::wstring::npos)dc->DrawText(entry->text.substr(tab+1).c_str(),label,flags|DT_RIGHT|DT_NOPREFIX);
   }
  }else dc->DrawText(entry->text.c_str(),area,DT_CENTER|DT_VCENTER|DT_SINGLELINE|((item->itemState&ODS_NOACCEL)?DT_HIDEPREFIX:0));dc->RestoreDC(saved);if(submenu)dc->ExcludeClipRect(item->rcItem.left,item->rcItem.top,item->rcItem.right,item->rcItem.bottom);return true;
 }
 bool mnemonic(HMENU menu,wchar_t key,LRESULT& result)const{
  key=(wchar_t)towupper(key);std::vector<UINT> matches;int highlighted=-1;
  for(const auto& entry:entries)if(entry->menu==menu){MENUITEMINFOW state{sizeof(state)};state.fMask=MIIM_STATE;GetMenuItemInfoW(menu,entry->position,TRUE,&state);if(state.fState&MFS_HILITE)highlighted=(int)entry->position;if(state.fState&(MFS_DISABLED|MFS_GRAYED))continue;
   for(size_t i=0;i+1<entry->text.size();++i)if(entry->text[i]==L'&'){if(entry->text[i+1]==L'&'){++i;continue;}if(towupper(entry->text[i+1])==key){matches.push_back(entry->position);break;}}
  }
  if(!matches.empty()){auto next=std::find_if(matches.begin(),matches.end(),[&](UINT position){return (int)position>highlighted;});result=MAKELRESULT(next==matches.end()?matches.front():*next,matches.size()==1?MNC_EXECUTE:MNC_SELECT);return true;}
  return false;
 }
};
