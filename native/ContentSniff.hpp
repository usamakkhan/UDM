#pragma once
// UDM's own content recognition for downloaded bytes. It classifies the leading
// bytes of a download so UDM can stop when a server sends a web page instead of
// the requested file, and can add a missing extension before publication.
// The signature table is written from public format specifications; no third-party
// signature database is included.
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
namespace udm {
enum class SniffVerdict {None,Empty,Match,MatchAfterPrefix,Html,Text,NeedMore};
struct SniffResult {
 SniffVerdict verdict=SniffVerdict::None;
 std::string mime,extension,description;
 // Other extensions that are consistent with this content (e.g. docx for ZIP).
 std::vector<std::string> compatible;
 // True only when the content identifies one specific extension.
 bool specific=false;
 size_t prefixLength=0;
};
// Text and HTML decisions use this many leading bytes; the ISO 9660 descriptor
// is read from its fixed offset when the caller supplies it.
constexpr size_t contentSniffWindow=5000;
constexpr size_t contentSniffReadLimit=0x8806;
inline const char* sniffVerdictName(SniffVerdict v){
 switch(v){case SniffVerdict::Empty:return "Empty";case SniffVerdict::Match:return "Match";case SniffVerdict::MatchAfterPrefix:return "MatchAfterPrefix";case SniffVerdict::Html:return "Html";case SniffVerdict::Text:return "Text";case SniffVerdict::NeedMore:return "NeedMore";default:return "None";}
}
namespace sniff {
inline std::string lowerAscii(std::string s){for(auto& c:s)if(c>='A'&&c<='Z')c=(char)(c-'A'+'a');return s;}
inline bool at(const uint8_t* d,size_t n,size_t offset,const char* magic,size_t length){return offset+length<=n&&std::memcmp(d+offset,magic,length)==0;}
inline bool contains(const uint8_t* d,size_t n,const char* text,size_t limit){const size_t length=std::strlen(text);limit=std::min(limit,n);if(length>limit)return false;return std::search(d,d+limit,text,text+length)!=d+limit;}
inline uint32_t le32(const uint8_t* d){return d[0]|(d[1]<<8)|(d[2]<<16)|((uint32_t)d[3]<<24);}
inline SniffResult found(const char* mime,const char* ext,const char* description,std::vector<std::string> compatible={},bool specific=true){
 SniffResult r;r.verdict=SniffVerdict::Match;r.mime=mime;r.extension=ext;r.description=description;r.specific=specific;r.compatible=std::move(compatible);
 if(*ext&&std::find(r.compatible.begin(),r.compatible.end(),ext)==r.compatible.end())r.compatible.insert(r.compatible.begin(),ext);return r;
}
inline SniffResult zipFamily(const uint8_t* d,size_t n){
 const std::vector<std::string> family={"zip","docx","xlsx","pptx","odt","ods","odp","epub","jar","apk","xpi","crx","vsix","nupkg","whl","appx","msix","appxbundle","msixbundle","aab","ipa","kmz","3mf","cbz"};
 if(contains(d,n,"mimetypeapplication/epub+zip",120))return found("application/epub+zip","epub","EPUB document",family);
 if(contains(d,n,"mimetypeapplication/vnd.oasis.opendocument.text",120))return found("application/vnd.oasis.opendocument.text","odt","OpenDocument text",family);
 if(contains(d,n,"mimetypeapplication/vnd.oasis.opendocument.spreadsheet",120))return found("application/vnd.oasis.opendocument.spreadsheet","ods","OpenDocument spreadsheet",family);
 if(contains(d,n,"mimetypeapplication/vnd.oasis.opendocument.presentation",120))return found("application/vnd.oasis.opendocument.presentation","odp","OpenDocument presentation",family);
 if(contains(d,n,"[Content_Types].xml",n)){
  if(contains(d,n,"word/",n))return found("application/vnd.openxmlformats-officedocument.wordprocessingml.document","docx","Word document",family);
  if(contains(d,n,"xl/",n))return found("application/vnd.openxmlformats-officedocument.spreadsheetml.sheet","xlsx","Excel workbook",family);
  if(contains(d,n,"ppt/",n))return found("application/vnd.openxmlformats-officedocument.presentationml.presentation","pptx","PowerPoint presentation",family);
 }
 if(contains(d,n,"AndroidManifest.xml",n))return found("application/vnd.android.package-archive","apk","Android package",family);
 if(contains(d,n,"META-INF/MANIFEST.MF",n))return found("application/java-archive","jar","Java archive",family);
 return found("application/zip","zip","ZIP archive",family);
}
inline SniffResult portableExecutable(const uint8_t* d,size_t n){
 const std::vector<std::string> family={"exe","dll","sys","scr","com","cpl","ocx","efi","mui","ax"};
 if(n<0x40)return found("application/vnd.microsoft.portable-executable","exe","DOS/Windows executable",family,false);
 const uint32_t pe=le32(d+0x3c);
 if(pe+24<=n&&at(d,n,pe,"PE\0\0",4)){const uint16_t characteristics=(uint16_t)(d[pe+22]|(d[pe+23]<<8));if(characteristics&0x2000)return found("application/vnd.microsoft.portable-executable","dll","Windows library",family);return found("application/vnd.microsoft.portable-executable","exe","Windows executable",family);}
 return found("application/vnd.microsoft.portable-executable","exe","DOS/Windows executable",family,false);
}
inline SniffResult isoBrand(const uint8_t* d,size_t n){
 std::string brand(reinterpret_cast<const char*>(d+8),4);
 const std::vector<std::string> mp4={"mp4","m4v","m4a","m4b","m4p","mov","3gp","3g2","f4v","heic","avif","mp4v"};
 if(brand=="qt  ")return found("video/quicktime","mov","QuickTime movie",mp4);
 if(brand=="M4A "||brand=="M4B "||brand=="M4P ")return found("audio/mp4","m4a","MPEG-4 audio",mp4);
 if(brand=="M4V "||brand=="M4VH"||brand=="M4VP")return found("video/x-m4v","m4v","MPEG-4 video",mp4);
 if(brand.compare(0,3,"3gp")==0)return found("video/3gpp","3gp","3GPP media",mp4);
 if(brand.compare(0,3,"3g2")==0)return found("video/3gpp2","3g2","3GPP2 media",mp4);
 if(brand=="avif"||brand=="avis")return found("image/avif","avif","AVIF image",mp4);
 if(brand=="heic"||brand=="heix"||brand=="mif1"||brand=="msf1")return found("image/heic","heic","HEIF image",mp4);
 if(brand=="f4v ")return found("video/mp4","f4v","Flash MP4 video",mp4);
 (void)n;return found("video/mp4","mp4","MPEG-4 media",mp4);
}
// Recognizes content at offset zero. Returns verdict None when unknown.
inline SniffResult signature(const uint8_t* d,size_t n){
 if(at(d,n,0,"PK\x03\x04",4)||at(d,n,0,"PK\x05\x06",4)||at(d,n,0,"PK\x07\x08",4))return zipFamily(d,n);
 if(at(d,n,0,"Rar!\x1a\x07",6))return found("application/vnd.rar","rar","RAR archive",{"rar","cbr"});
 if(at(d,n,0,"7z\xbc\xaf\x27\x1c",6))return found("application/x-7z-compressed","7z","7-Zip archive");
 if(at(d,n,0,"\x1f\x8b\x08",3))return found("application/gzip","gz","gzip data",{"gz","tgz","gzip","svgz"});
 if(at(d,n,0,"BZh",3)&&n>3&&d[3]>='1'&&d[3]<='9')return found("application/x-bzip2","bz2","bzip2 data",{"bz2","tbz2","tbz"});
 if(at(d,n,0,"\xfd" "7zXZ\0",6))return found("application/x-xz","xz","XZ data",{"xz","txz"});
 if(at(d,n,0,"\x28\xb5\x2f\xfd",4))return found("application/zstd","zst","Zstandard data",{"zst","tzst"});
 if(at(d,n,0,"MSCF\0\0\0\0",8))return found("application/vnd.ms-cab-compressed","cab","Microsoft Cabinet",{"cab","msu"});
 if(at(d,n,257,"ustar",5))return found("application/x-tar","tar","tar archive");
 if(n>=7&&d[2]=='-'&&d[3]=='l'&&d[6]=='-'&&(d[4]=='h'||d[4]=='z'))return found("application/x-lzh-compressed","lzh","LHA archive",{"lzh","lha"});
 if(at(d,n,0,"\x60\xea",2)&&n>=10)return found("application/x-arj","arj","ARJ archive",{},false);
 if(at(d,n,0,"MSWIM\0\0\0",8))return found("application/x-ms-wim","wim","Windows imaging archive",{"wim","esd"});
 if(at(d,n,0,"!<arch>\ndebian",14))return found("application/vnd.debian.binary-package","deb","Debian package");
 if(at(d,n,0,"\xed\xab\xee\xdb",4))return found("application/x-rpm","rpm","RPM package");
 if(at(d,n,0x8001,"CD001",5))return found("application/x-iso9660-image","iso","ISO 9660 image",{"iso","img"});
 if(at(d,n,0,"%PDF-",5))return found("application/pdf","pdf","PDF document",{"pdf","ai"});
 if(at(d,n,0,"%!PS",4))return found("application/postscript","ps","PostScript document",{"ps","eps","ai"});
 if(at(d,n,0,"{\\rtf",5))return found("application/rtf","rtf","Rich Text document");
 if(at(d,n,0,"\xd0\xcf\x11\xe0\xa1\xb1\x1a\xe1",8))return found("application/x-ole-storage","","Compound document",{"msi","msp","mst","doc","xls","ppt","pps","msg","vsd","pub"},false);
 if(at(d,n,0,"MZ",2))return portableExecutable(d,n);
 if(at(d,n,0,"\x7f" "ELF",4))return found("application/x-executable","","ELF executable",{"so","bin","run","elf","appimage"},false);
 if(at(d,n,4,"ftyp",4)&&n>=12)return isoBrand(d,n);
 if(at(d,n,0,"\x1a\x45\xdf\xa3",4))return contains(d,n,"webm",64)?found("video/webm","webm","WebM media",{"webm","mkv","weba"}):found("video/x-matroska","mkv","Matroska media",{"mkv","mka","mks","mk3d","webm"});
 if(at(d,n,0,"RIFF",4)&&at(d,n,8,"AVI ",4))return found("video/x-msvideo","avi","AVI video");
 if(at(d,n,0,"RIFF",4)&&at(d,n,8,"WAVE",4))return found("audio/wav","wav","WAVE audio");
 if(at(d,n,0,"RIFF",4)&&at(d,n,8,"WEBP",4))return found("image/webp","webp","WebP image");
 if(at(d,n,0,"FORM",4)&&(at(d,n,8,"AIFF",4)||at(d,n,8,"AIFC",4)))return found("audio/aiff","aif","AIFF audio",{"aif","aiff","aifc"});
 // FLV: version 1, reserved flag bits clear, at least one stream present.
 if(at(d,n,0,"FLV\x01",4)&&n>=9&&(d[4]&0xfa)==0&&(d[4]&0x05)!=0)return found("video/x-flv","flv","Flash video");
 if(at(d,n,0,"\x30\x26\xb2\x75\x8e\x66\xcf\x11",8))return found("video/x-ms-asf","wmv","ASF media",{"wmv","wma","asf"},false);
 if(at(d,n,0,".RMF",4))return found("application/vnd.rn-realmedia","rm","RealMedia",{"rm","rmvb","ra"});
 if(at(d,n,0,"OggS",4))return contains(d,n,"\x01video",120)||contains(d,n,"theora",120)?found("video/ogg","ogv","Ogg video",{"ogv","ogg"}):found("audio/ogg","ogg","Ogg audio",{"ogg","oga","opus","spx","ogv"});
 if(at(d,n,0,"fLaC",4))return found("audio/flac","flac","FLAC audio");
 if(at(d,n,0,"ID3",3)&&n>=10&&d[3]<5)return found("audio/mpeg","mp3","MP3 audio with ID3",{"mp3","mpga","aac"},false);
 if(n>=4&&d[0]==0xff&&(d[1]==0xfb||d[1]==0xf3||d[1]==0xf2||d[1]==0xfa))return found("audio/mpeg","mp3","MPEG audio",{"mp3","mpga","mp2","mpa"},false);
 if(n>=4&&d[0]==0xff&&(d[1]==0xf1||d[1]==0xf9))return found("audio/aac","aac","AAC audio",{"aac","adts"},false);
 if(at(d,n,0,"\0\0\x01\xba",4))return found("video/mpeg","mpg","MPEG program stream",{"mpg","mpeg","mpe","vob","m2p"});
 if(n>=377&&d[0]==0x47&&d[188]==0x47&&d[376]==0x47)return found("video/mp2t","ts","MPEG transport stream",{"ts","m2ts","mts","tsv"});
 if(at(d,n,0,"\x89PNG\r\n\x1a\n",8))return found("image/png","png","PNG image");
 if(at(d,n,0,"\xff\xd8\xff",3))return found("image/jpeg","jpg","JPEG image",{"jpg","jpeg","jpe","jfif"});
 if(at(d,n,0,"GIF87a",6)||at(d,n,0,"GIF89a",6))return found("image/gif","gif","GIF image");
 if(at(d,n,0,"II*\0",4)||at(d,n,0,"MM\0*",4))return found("image/tiff","tif","TIFF image",{"tif","tiff","dng","cr2","nef","arw"});
 if(at(d,n,0,"8BPS",4))return found("image/vnd.adobe.photoshop","psd","Photoshop image");
 if(at(d,n,0,"BM",2)&&n>=26&&le32(d+14)>=12&&le32(d+14)<=124)return found("image/bmp","bmp","BMP image",{"bmp","dib"},false);
 if(at(d,n,0,"FWS",3)||at(d,n,0,"CWS",3)||at(d,n,0,"ZWS",3))return found("application/x-shockwave-flash","swf","Flash movie");
 if(at(d,n,0,"SQLite format 3\0",16))return found("application/vnd.sqlite3","sqlite","SQLite database",{"sqlite","db","sqlite3"},false);
 if(at(d,n,0,"d8:announce",11)||at(d,n,0,"d13:announce-list",17))return found("application/x-bittorrent","torrent","BitTorrent file");
 return {};
}
// Text classes follow the usual "text data" definition: printable ASCII,
// common whitespace, ESC, and well-formed UTF-8 sequences.
inline bool textByte(uint8_t c){return c>=0x20&&c!=0x7f?true:c=='\t'||c=='\n'||c=='\r'||c=='\f'||c==0x1b;}
// A valid UTF-8 prefix may end at the sniff-window boundary, but not at EOF.
inline size_t textRun(const uint8_t* d,size_t n,bool allowIncomplete=true){
 size_t i=0;
 while(i<n){const uint8_t c=d[i];
  if(c<0x80){if(!textByte(c))return i;++i;continue;}
  size_t extra=(c&0xe0)==0xc0&&c>=0xc2?1:(c&0xf0)==0xe0?2:(c&0xf8)==0xf0&&c<=0xf4?3:0;if(!extra)return i;
  size_t j=1;for(;j<=extra&&i+j<n;++j){
   const auto next=d[i+j];if((next&0xc0)!=0x80)return i;
   // RFC 3629: exclude overlong encodings, surrogates and values above U+10FFFF.
   if(j==1&&((c==0xe0&&next<0xa0)||(c==0xed&&next>=0xa0)||(c==0xf0&&next<0x90)||(c==0xf4&&next>=0x90)))return i;
  }
  if(j<=extra&&!allowIncomplete)return i;
  i+=std::min(extra+1,n-i);
 }
 return n;
}
inline size_t skipBom(const uint8_t* d,size_t n){return at(d,n,0,"\xef\xbb\xbf",3)?3:0;}
inline bool utf16Text(const uint8_t* d,size_t n){
 if(n<4||!(at(d,n,0,"\xff\xfe",2)||at(d,n,0,"\xfe\xff",2)))return false;
 const bool little=d[0]==0xff;size_t zeros=0,pairs=0;for(size_t i=2;i+1<n&&i<1024;i+=2){++pairs;const uint8_t high=little?d[i+1]:d[i],low=little?d[i]:d[i+1];if(high==0){++zeros;if(!textByte(low))return false;}}
 return pairs&&zeros*10>=pairs*8;
}
inline bool htmlMarkup(const uint8_t* d,size_t n){
 size_t i=skipBom(d,n);while(i<n&&(d[i]==' '||d[i]=='\t'||d[i]=='\r'||d[i]=='\n'))++i;
 if(i>=n||d[i]!='<')return false;
 std::string head=lowerAscii(std::string(reinterpret_cast<const char*>(d+i),std::min<size_t>(n-i,1024)));
 for(const char* tag:{"<!doctype html","<html","<head","<body","<script","<title","<meta","<iframe","<style","<link","<div","<p>","<p ","<h1","<table","<center","<form","<a ","<br","<noscript","<frameset"})if(head.compare(0,std::strlen(tag),tag)==0)return true;
 if(head.compare(0,4,"<!--")==0||head.compare(0,5,"<?xml")==0)return head.find("<html")!=std::string::npos||head.find("<!doctype html")!=std::string::npos;
 return false;
}
}
// Classifies the leading bytes of a download. complete reports whether data
// holds the whole file (so a short buffer is final rather than pending).
inline SniffResult sniffContent(const uint8_t* data,size_t size,bool complete){
 SniffResult r;
 if(!size){if(complete){r.verdict=SniffVerdict::Empty;r.mime="application/x-empty";r.description="Empty file";}else r.verdict=SniffVerdict::NeedMore;return r;}
 auto match=sniff::signature(data,size);if(match.verdict==SniffVerdict::Match)return match;
 const size_t window=std::min(size,contentSniffWindow);
 if(!complete&&size<contentSniffWindow){r.verdict=SniffVerdict::NeedMore;return r;}
 const size_t text=sniff::textRun(data,window,!complete||window<size);
 if(text==window||sniff::utf16Text(data,window)){
  if(sniff::htmlMarkup(data,window)){r.verdict=SniffVerdict::Html;r.mime="text/html";r.extension="html";r.description="HTML document";r.compatible={"html","htm","xhtml","shtml","mht","mhtml","php","asp","aspx","jsp"};return r;}
  r.verdict=SniffVerdict::Text;r.mime="text/plain";r.extension="txt";r.description="Text";return r;
 }
 // Binary content after a text or HTML prefix (for example a notice inserted
 // ahead of an archive). Only strong signatures after a text-only prefix count.
 if(text>=16){
  const size_t limit=std::min<size_t>({window,size,2000});
  for(size_t offset=std::max<size_t>(1,text>16?text-16:1);offset<=std::min(text,limit);++offset){
   auto later=sniff::signature(data+offset,size-offset);
   if(later.verdict!=SniffVerdict::Match||!later.specific)continue;
   later.verdict=SniffVerdict::MatchAfterPrefix;later.prefixLength=offset;return later;
  }
 }
 r.mime="application/octet-stream";return r;
}
// Extensions whose content is never a web page or plain text. A text or HTML
// body served under one of these names is treated as a server error page.
// Generic names such as .bin or .dat carry arbitrary bytes and are excluded.
inline bool binaryExtension(const std::string& extension){
 static const std::vector<std::string> list={"3gp","3g2","7z","aac","ace","aif","aiff","apk","appx","appxbundle","arj","asf","avi","bz2","cab","cbr","cbz","deb","dll","dmg","doc","docx","epub","exe","f4v","flac","flv","gz","gzip","img","iso","jar","lha","lzh","m4a","m4b","m4v","mkv","mov","mp3","mp4","mpa","mpe","mpeg","mpg","msi","msix","msixbundle","msp","msu","odp","ods","odt","ogg","ogv","opus","pdf","pps","ppt","pptx","qt","ra","rar","rm","rmvb","rpm","sit","sitx","swf","tar","tbz2","tgz","tif","tiff","ts","txz","vob","wav","webm","wim","wma","wmv","xls","xlsx","xz","z","zip","zst"};
 auto e=sniff::lowerAscii(extension);
 if(e.size()==3&&(e[0]=='r'||e[0]=='z')&&e[1]>='0'&&e[1]<='9'&&e[2]>='0'&&e[2]<='9')return true; // split archive volumes
 return std::find(list.begin(),list.end(),e)!=list.end();
}
inline std::string fileExtension(const std::string& name){
 auto slash=name.find_last_of("\\/");auto base=slash==std::string::npos?name:name.substr(slash+1);auto dot=base.rfind('.');
 if(dot==std::string::npos||dot==0||dot+1==base.size())return "";return sniff::lowerAscii(base.substr(dot+1));
}
// True when the body is a web page or plain text but the download was
// expected to be a file format that is never text.
inline bool webPageInsteadOfFile(const SniffResult& r,const std::string& expectedExtension){
 const auto e=sniff::lowerAscii(expectedExtension);
 if(r.verdict==SniffVerdict::Html)return !e.empty()&&std::find(r.compatible.begin(),r.compatible.end(),e)==r.compatible.end()&&e!="txt"&&e!="xml"&&e!="svg";
 if(r.verdict==SniffVerdict::Text)return binaryExtension(e);
 return false;
}
// Returns the corrected file name, or an empty string when the name stays.
// Only names without a usable extension change; an explicit extension that
// disagrees with the content is left alone.
inline std::string correctedFileName(const std::string& name,const SniffResult& r){
 if(name.empty())return "";
 std::string target;
 if(r.verdict==SniffVerdict::Match&&r.specific&&!r.extension.empty())target=r.extension;
 else if(r.verdict==SniffVerdict::Html)target="html";
 else return "";
 static const std::vector<std::string> weak={"action","ashx","asp","aspx","bin","cgi","crdownload","dat","do","download","jsp","part","php","pl","tmp","unknown"};
 auto e=fileExtension(name);
 const bool numeric=!e.empty()&&std::all_of(e.begin(),e.end(),[](char c){return c>='0'&&c<='9';});
 const bool plausible=!e.empty()&&e.size()<=8&&!numeric&&std::all_of(e.begin(),e.end(),[](char c){return (c>='a'&&c<='z')||(c>='0'&&c<='9');});
 if(plausible&&std::find(weak.begin(),weak.end(),e)==weak.end())return "";
 if(e==target)return "";
 if(plausible&&std::find(r.compatible.begin(),r.compatible.end(),e)!=r.compatible.end())return "";
 auto stem=plausible?name.substr(0,name.size()-e.size()-1):name;
 while(!stem.empty()&&(stem.back()=='.'||stem.back()==' '))stem.pop_back();
 if(stem.empty())return "";
 return stem+"."+target;
}
}
