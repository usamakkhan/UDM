#pragma once
// Original UDM classic tray glyph, drawn at icon resolution and embedded in code.
namespace udm {
inline HICON classicTrayIcon(){
 struct Icon {
  HICON value=nullptr;
  Icon(){
   BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=32;info.bmiHeader.biHeight=-32;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
   void* bits=nullptr;auto color=CreateDIBSection(nullptr,&info,DIB_RGB_COLORS,&bits,nullptr,0);if(!color)return;
   auto pixels=static_cast<DWORD*>(bits);for(int y=0;y<32;++y)for(int x=0;x<32;++x){
    DWORD pixel=0;
    if(x>=2&&x<30&&y>=2&&y<30)pixel=(x==2||x==29||y==2||y==29)?0xff444444:0xffe8e8e8;
    const bool shaft=x>=14&&x<=17&&y>=6&&y<=18;
    const bool tip=y>=14&&y<=22&&x>=7+(y-14)&&x<=24-(y-14);
    const bool tray=(y>=25&&y<=27&&x>=6&&x<=25)||((x>=6&&x<=8||x>=23&&x<=25)&&y>=21&&y<=27);
    if(shaft||tip||tray)pixel=0xff2455a6;pixels[y*32+x]=pixel;
   }
   BYTE maskBits[128]{};auto mask=CreateBitmap(32,32,1,1,maskBits);
   if(mask){ICONINFO icon{};icon.fIcon=TRUE;icon.hbmColor=color;icon.hbmMask=mask;value=CreateIconIndirect(&icon);DeleteObject(mask);}DeleteObject(color);
  }
  ~Icon(){if(value)DestroyIcon(value);}
 };
 static Icon icon;return icon.value;
}
inline bool classicTrayStyle(const std::string& value){return value=="Classic"||value=="System";}
}
