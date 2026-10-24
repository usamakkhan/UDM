#pragma once
#include "ToolbarModel.hpp"
#include <wincodec.h>
#include <atlbase.h>
#pragma comment(lib,"windowscodecs.lib")
namespace udm {
inline void toolbarImageResult(HRESULT value){if(FAILED(value))throw std::runtime_error("Windows could not decode the toolbar image.");}
inline ToolbarBitmap decodeToolbarPng(IWICImagingFactory* factory,IWICBitmapDecoder* decoder){
 CComPtr<IWICBitmapFrameDecode> frame;toolbarImageResult(decoder->GetFrame(0,&frame));UINT w=0,h=0;toolbarImageResult(frame->GetSize(&w,&h));if(!w||!h||w>256||h>256)throw std::runtime_error("Toolbar icon dimensions are invalid.");
 CComPtr<IWICFormatConverter> converter;toolbarImageResult(factory->CreateFormatConverter(&converter));toolbarImageResult(converter->Initialize(frame,GUID_WICPixelFormat32bppBGRA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom));
 ToolbarBitmap result{(int)w,(int)h,Bytes((size_t)w*h*4)};toolbarImageResult(converter->CopyPixels(nullptr,w*4,(UINT)result.pixels.size(),result.pixels.data()));return result;
}
inline ToolbarBitmap loadToolbarPng(const fs::path& path){
 CComPtr<IWICImagingFactory> factory;toolbarImageResult(factory.CoCreateInstance(CLSID_WICImagingFactory));
 CComPtr<IWICBitmapDecoder> decoder;toolbarImageResult(factory->CreateDecoderFromFilename(path.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&decoder));
 return decodeToolbarPng(factory,decoder);
}
inline ToolbarBitmap loadEmbeddedToolbarPng(int index){
 if(index<0||index>=ToolbarCommandCount)throw std::runtime_error("Invalid toolbar icon index.");
 auto module=GetModuleHandleW(nullptr);
 auto resource=FindResourceW(module,MAKEINTRESOURCEW(1000+index),RT_RCDATA);auto count=resource?SizeofResource(module,resource):0;
 auto loaded=resource?LoadResource(module,resource):nullptr;auto data=loaded?LockResource(loaded):nullptr;
 if(!data||!count)throw std::runtime_error("The built-in toolbar icon is unavailable.");
 CComPtr<IWICImagingFactory> factory;toolbarImageResult(factory.CoCreateInstance(CLSID_WICImagingFactory));
 CComPtr<IWICStream> stream;toolbarImageResult(factory->CreateStream(&stream));toolbarImageResult(stream->InitializeFromMemory(static_cast<BYTE*>(data),count));
 CComPtr<IWICBitmapDecoder> decoder;toolbarImageResult(factory->CreateDecoderFromStream(stream,nullptr,WICDecodeMetadataCacheOnLoad,&decoder));
 return decodeToolbarPng(factory,decoder);
}
inline ToolbarBitmap loadBuiltInToolbarPng(const fs::path& directory,int index){
 try{return loadToolbarPng(directory/(wide(toolbarAsset(index))+L".png"));}
 catch(const std::runtime_error&){return loadEmbeddedToolbarPng(index);}
}
// "3D Style": the icon on a raised, rounded tile with a vertical gradient, a 1 px border and a
// shadow at the bottom right (IDM offers Neon and 3D icon sets; UDM keeps its own icons and
// changes their setting). Pixels are premultiplied BGRA; the icon is composited over the tile.
inline void toolbarRaisedTile(Bytes& pixels,int width,int height,int state){
 if(pixels.size()!=(size_t)width*height*4||width<4||height<4)return;
 const int radius=std::max(2,width/8),inset=1;Bytes tile((size_t)width*height*4,0);
 auto cornerOutside=[&](int x,int y){int cx=x<inset+radius?inset+radius:x>=width-inset-radius?width-inset-radius-1:x,cy=y<inset+radius?inset+radius:y>=height-inset-radius?height-inset-radius-1:y;if(cx==x&&cy==y)return false;const int dx=x-cx,dy=y-cy;return dx*dx+dy*dy>radius*radius;};
 for(int y=0;y<height;++y)for(int x=0;x<width;++x){
  const bool inside=x>=inset&&y>=inset&&x<width-inset&&y<height-inset&&!cornerOutside(x,y);
  const bool shadow=!inside&&x>=inset+1&&y>=inset+1&&x<width&&y<height&&!(x>=inset+1&&y>=inset+1&&x<width-inset+1&&y<height-inset+1&&cornerOutside(x-1,y-1));
  BYTE* at=&tile[((size_t)y*width+x)*4];
  if(inside){
   const bool border=x==inset||y==inset||x==width-inset-1||y==height-inset-1||cornerOutside(x-(x<width/2?-1:1),y-(y<height/2?-1:1));
   const double t=(double)(y-inset)/std::max(1,height-2*inset-1);
   int r,g,b;if(state==2){r=g=b=(int)(236-20*t);}else if(state==1){r=(int)(255-10*t);g=(int)(250-16*t);b=(int)(236-28*t);}else{r=(int)(253-30*t);g=(int)(253-28*t);b=(int)(255-24*t);}
   if(border){r=state==2?180:150;g=state==2?180:156;b=state==2?180:166;}
   at[0]=(BYTE)b;at[1]=(BYTE)g;at[2]=(BYTE)r;at[3]=255;
  }else if(shadow){at[0]=at[1]=at[2]=0;at[3]=56;}
 }
 for(size_t i=0;i<pixels.size();i+=4){const unsigned a=pixels[i+3];for(int c=0;c<4;++c){const unsigned top=pixels[i+c],under=tile[i+c];pixels[i+c]=(BYTE)std::min<unsigned>(255,top+under*(255-a)/255);}}
}
inline Bytes toolbarFrame(const ToolbarBitmap& strip,int index,int columns,int width,int height,int state=0,bool raised=false){
 if(columns<1||strip.width%columns||index<0||index>=columns||width<1||height<1||width>512||height>512||strip.pixels.size()!=(size_t)strip.width*strip.height*4)throw std::runtime_error("Invalid toolbar image frame.");
 const int originalWidth=strip.width/columns;ToolbarBitmap frame{originalWidth,strip.height,Bytes((size_t)originalWidth*strip.height*4)};
 for(int y=0;y<strip.height;++y)for(int x=0;x<originalWidth;++x){auto src=((size_t)y*strip.width+index*originalWidth+x)*4,at=((size_t)y*originalWidth+x)*4;
  for(int c=0;c<4;++c)frame.pixels[at+c]=strip.pixels[src+c];
  if(state==1){for(int c=0;c<3;++c)frame.pixels[at+c]=(BYTE)(frame.pixels[at+c]+(255-frame.pixels[at+c])/6);}
  if(state==2){auto grey=(frame.pixels[at]*11+frame.pixels[at+1]*59+frame.pixels[at+2]*30)/100;for(int c=0;c<3;++c)frame.pixels[at+c]=(BYTE)grey;frame.pixels[at+3]=(BYTE)(frame.pixels[at+3]*3/5);}
 }
 CComPtr<IWICImagingFactory> factory;toolbarImageResult(factory.CoCreateInstance(CLSID_WICImagingFactory));
 CComPtr<IWICBitmap> bitmap;toolbarImageResult(factory->CreateBitmapFromMemory(originalWidth,strip.height,GUID_WICPixelFormat32bppBGRA,originalWidth*4,(UINT)frame.pixels.size(),frame.pixels.data(),&bitmap));
 CComPtr<IWICBitmapScaler> scaler;toolbarImageResult(factory->CreateBitmapScaler(&scaler));toolbarImageResult(scaler->Initialize(bitmap,width,height,WICBitmapInterpolationModeFant));
 CComPtr<IWICFormatConverter> converter;toolbarImageResult(factory->CreateFormatConverter(&converter));toolbarImageResult(converter->Initialize(scaler,GUID_WICPixelFormat32bppPBGRA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom));
 Bytes output((size_t)width*height*4);toolbarImageResult(converter->CopyPixels(nullptr,width*4,(UINT)output.size(),output.data()));if(raised)toolbarRaisedTile(output,width,height,state);return output;
}
inline HBITMAP toolbarDib(const Bytes& pixels,int width,int height){
 if(pixels.size()!=(size_t)width*height*4)throw std::runtime_error("Invalid toolbar pixel data.");
 BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=width;info.bmiHeader.biHeight=-height;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;void* bits=nullptr;
 auto bitmap=CreateDIBSection(nullptr,&info,DIB_RGB_COLORS,&bits,nullptr,0);if(!bitmap||!bits){if(bitmap)DeleteObject(bitmap);throw std::runtime_error("Cannot allocate toolbar image.");}memcpy(bits,pixels.data(),pixels.size());return bitmap;
}
}
