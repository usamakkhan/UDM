#pragma once
#include "ToolbarModel.hpp"
#include <wincodec.h>
#include <atlbase.h>
#pragma comment(lib,"windowscodecs.lib")
namespace udm {
inline void toolbarImageResult(HRESULT value){if(FAILED(value))throw std::runtime_error("Windows could not decode the toolbar image.");}
inline ToolbarBitmap loadToolbarPng(const fs::path& path){
 CComPtr<IWICImagingFactory> factory;toolbarImageResult(factory.CoCreateInstance(CLSID_WICImagingFactory));
 CComPtr<IWICBitmapDecoder> decoder;toolbarImageResult(factory->CreateDecoderFromFilename(path.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&decoder));
 CComPtr<IWICBitmapFrameDecode> frame;toolbarImageResult(decoder->GetFrame(0,&frame));UINT w=0,h=0;toolbarImageResult(frame->GetSize(&w,&h));if(!w||!h||w>256||h>256)throw std::runtime_error("Toolbar icon dimensions are invalid.");
 CComPtr<IWICFormatConverter> converter;toolbarImageResult(factory->CreateFormatConverter(&converter));toolbarImageResult(converter->Initialize(frame,GUID_WICPixelFormat32bppBGRA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom));
 ToolbarBitmap result{(int)w,(int)h,Bytes((size_t)w*h*4)};toolbarImageResult(converter->CopyPixels(nullptr,w*4,(UINT)result.pixels.size(),result.pixels.data()));return result;
}
inline Bytes toolbarFrame(const ToolbarBitmap& strip,int index,int columns,int width,int height,int state=0){
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
 Bytes output((size_t)width*height*4);toolbarImageResult(converter->CopyPixels(nullptr,width*4,(UINT)output.size(),output.data()));return output;
}
inline HBITMAP toolbarDib(const Bytes& pixels,int width,int height){
 if(pixels.size()!=(size_t)width*height*4)throw std::runtime_error("Invalid toolbar pixel data.");
 BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=width;info.bmiHeader.biHeight=-height;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;void* bits=nullptr;
 auto bitmap=CreateDIBSection(nullptr,&info,DIB_RGB_COLORS,&bits,nullptr,0);if(!bitmap||!bits){if(bitmap)DeleteObject(bitmap);throw std::runtime_error("Cannot allocate toolbar image.");}memcpy(bits,pixels.data(),pixels.size());return bitmap;
}
}

