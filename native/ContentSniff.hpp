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
 const std::vector<std::string> family={"zip","docx","xlsx","pptx","odt","ods","odp","epub","jar","apk","xpi","crx","vsix","nupkg","whl","appx","msix","appxbundle","msixbundle","aab","ipa","kmz","3mf","cbz","hwpx"};
 if(contains(d,n,"mimetypeapplication/epub+zip",120))return found("application/epub+zip","epub","EPUB document",family);
 if(contains(d,n,"mimetypeapplication/hwp+zip",120))return found("application/hwp+zip","hwpx","Hangul Word Processor document (HWPX)",family);
 if(contains(d,n,"mimetypeapplication/vnd.oasis.opendocument.text",120))return found("application/vnd.oasis.opendocument.text","odt","OpenDocument text",family);
 if(contains(d,n,"mimetypeapplication/vnd.oasis.opendocument.spreadsheet",120))return found("application/vnd.oasis.opendocument.spreadsheet","ods","OpenDocument spreadsheet",family);
 if(contains(d,n,"mimetypeapplication/vnd.oasis.opendocument.presentation",120))return found("application/vnd.oasis.opendocument.presentation","odp","OpenDocument presentation",family);
 if(contains(d,n,"[Content_Types].xml",n)){
  if(contains(d,n,"word/",n))return found("application/vnd.openxmlformats-officedocument.wordprocessingml.document","docx","Word document",family);
  if(contains(d,n,"xl/",n))return found("application/vnd.openxmlformats-officedocument.spreadsheetml.sheet","xlsx","Excel workbook",family);
  if(contains(d,n,"ppt/",n))return found("application/vnd.openxmlformats-officedocument.presentationml.presentation","pptx","PowerPoint presentation",family);
 }
 if(contains(d,n,"mimetypeapplication/vnd.oasis.opendocument.graphics",120))return found("application/vnd.oasis.opendocument.graphics","odg","OpenDocument drawing",family);
 if(contains(d,n,"AndroidManifest.xml",n))return found("application/vnd.android.package-archive","apk","Android package",family);
 if(contains(d,n,"AppxMetadata/AppxBundleManifest.xml",n))return found("application/vnd.ms-appx","appxbundle","Windows app bundle",family);
 if(contains(d,n,"AppxManifest.xml",n))return found("application/vnd.ms-appx","appx","Windows app package",family);
 if(contains(d,n,"extension.vsixmanifest",n))return found("application/vsix","vsix","Visual Studio extension",family);
 if(contains(d,n,"Payload/",n)&&contains(d,n,".app/",n))return found("application/octet-stream","ipa","iOS application archive",family);
 if(contains(d,n,"doc.kml",n))return found("application/vnd.google-earth.kmz","kmz","Google Earth archive",family);
 if(contains(d,n,"META-INF/MANIFEST.MF",n))return found("application/java-archive","jar","Java archive",family);
 if(contains(d,n,"manifest.json",n)&&(contains(d,n,"META-INF/mozilla.rsa",n)||contains(d,n,"META-INF/mozilla.sf",n)))return found("application/x-xpinstall","xpi","Firefox extension",family);
 return found("application/zip","zip","ZIP archive",family);
}
inline uint16_t le16(const uint8_t* d){return (uint16_t)(d[0]|(d[1]<<8));}
inline uint32_t be32(const uint8_t* d){return ((uint32_t)d[0]<<24)|(d[1]<<16)|(d[2]<<8)|d[3];}
// Searches for an ASCII name stored as UTF-16LE, as compound-document directory entries are.
inline bool containsUtf16(const uint8_t* d,size_t n,const char* text){
 std::string wide;for(const char* c=text;*c;++c){wide.push_back(*c);wide.push_back('\0');}
 return n>=wide.size()&&std::search(d,d+n,wide.begin(),wide.end())!=d+n;
}
// OLE compound files (MSI, Office 97-2003, Outlook messages) share one header.
// The root entry's class identifier names the producer when the directory sector
// lies within the sniffed bytes; otherwise well-known stream names decide.
inline SniffResult oleCompound(const uint8_t* d,size_t n){
 const std::vector<std::string> family={"msi","msp","mst","doc","xls","ppt","pps","msg","vsd","pub"};
 if(n>=52){
  const unsigned shift=le16(d+30);const uint32_t directory=le32(d+48);
  if((shift==9||shift==12)&&directory<0x10000){
   const size_t root=((size_t)directory+1)<<shift;
   if(root+128<=n){
    const uint8_t* clsid=d+root+80;
    auto is=[&](const char* guid){return std::memcmp(clsid,guid,16)==0;};
    if(is("\x84\x10\x0c\0\0\0\0\0\xc0\0\0\0\0\0\0\x46"))return found("application/x-msi","msi","Windows Installer package",family);
    if(is("\x86\x10\x0c\0\0\0\0\0\xc0\0\0\0\0\0\0\x46"))return found("application/x-msi","msp","Windows Installer patch",family);
    if(is("\x06\x09\x02\0\0\0\0\0\xc0\0\0\0\0\0\0\x46"))return found("application/msword","doc","Word 97-2003 document",family);
    if(is("\x20\x08\x02\0\0\0\0\0\xc0\0\0\0\0\0\0\x46")||is("\x10\x08\x02\0\0\0\0\0\xc0\0\0\0\0\0\0\x46"))return found("application/vnd.ms-excel","xls","Excel 97-2003 workbook",family);
    if(is("\x10\x8d\x81\x64\x9b\x4f\xcf\x11\x86\xea\0\xaa\0\xb9\x29\xe8"))return found("application/vnd.ms-powerpoint","ppt","PowerPoint 97-2003 presentation",family);
    if(is("\x0b\x0d\x02\0\0\0\0\0\xc0\0\0\0\0\0\0\x46"))return found("application/vnd.ms-outlook","msg","Outlook message",family);
    if(is("\x14\x1a\x02\0\0\0\0\0\xc0\0\0\0\0\0\0\x46"))return found("application/vnd.visio","vsd","Visio drawing",family);
   }
  }
 }
 if(containsUtf16(d,n,"FileHeader")&&(containsUtf16(d,n,"BodyText")||containsUtf16(d,n,"DocInfo")))return found("application/x-hwp","hwp","Hangul Word Processor document (5.x)",{"hwp"});
 if(containsUtf16(d,n,"WordDocument"))return found("application/msword","doc","Word 97-2003 document",family);
 if(containsUtf16(d,n,"Workbook")||containsUtf16(d,n,"Book"))return found("application/vnd.ms-excel","xls","Excel 97-2003 workbook",family);
 if(containsUtf16(d,n,"PowerPoint Document"))return found("application/vnd.ms-powerpoint","ppt","PowerPoint 97-2003 presentation",family);
 if(containsUtf16(d,n,"__properties_version1.0"))return found("application/vnd.ms-outlook","msg","Outlook message",family);
 if(containsUtf16(d,n,"VisioDocument"))return found("application/vnd.visio","vsd","Visio drawing",family);
 return found("application/x-ole-storage","","Compound document",family,false);
}
inline bool pnmSeparator(const uint8_t* d,size_t n){return n>2&&(d[2]=='\n'||d[2]==' '||d[2]=='\r'||d[2]=='\t');}
// Long-tail formats, recognized from their published headers. Each entry names
// the extension the content implies; containers with several uses stay non-specific.
// An SVG document: UTF-8 (optionally with a byte order mark) or UTF-16LE text that
// opens with <svg or an XML declaration and names an <svg element without any <html.
inline bool svgDocument(const uint8_t* d,size_t n){
 const size_t limit=std::min<size_t>(n,4096);
 if(at(d,n,0,"\xff\xfe",2))return containsUtf16(d,limit,"<svg")&&!containsUtf16(d,limit,"<html")&&!containsUtf16(d,limit,"<HTML");
 if(!(at(d,n,0,"<svg",4)||at(d,n,0,"<?xml",5)||at(d,n,0,"\xef\xbb\xbf<?xml",8)||at(d,n,0,"\xef\xbb\xbf<svg",7)))return false;
 if(!contains(d,n,"<svg",limit))return false;
 const auto head=lowerAscii(std::string(reinterpret_cast<const char*>(d),limit));return head.find("<html")==std::string::npos&&head.find("<!doctype html")==std::string::npos;
}
// A binary OpenPGP certificate starts with a public (tag 6) or secret (tag 5) key packet
// in the old or new packet format (RFC 9580); the packet body opens with the key version.
inline SniffResult openPgpKey(const uint8_t* d,size_t n){
 size_t version=0;
 if(n>=4&&(d[0]==0x99||d[0]==0x95))version=3;else if(n>=3&&(d[0]==0x98||d[0]==0x94))version=2;
 else if(n>=7&&(d[0]==0xc6||d[0]==0xc5))version=d[1]<192?2:d[1]<224?3:d[1]==255?6:0;
 if(!version||d[version]<2||d[version]>6)return {};
 return found("application/pgp-keys","pgp","OpenPGP key",{"pgp","gpg","key","asc"},false);
}
inline SniffResult longTail(const uint8_t* d,size_t n){
 // Archives, compressed data and disk images.
 if(at(d,n,7,"**ACE**",7))return found("application/x-ace-compressed","ace","ACE archive");
 if(at(d,n,0,"xar!",4))return found("application/x-xar","xar","XAR archive",{"xar","pkg"});
 if(at(d,n,0,"\x1f\x9d",2))return found("application/x-compress","z","compress data",{"z","taz"});
 if(at(d,n,0,"\x04\x22\x4d\x18",4))return found("application/x-lz4","lz4","LZ4 data");
 if(at(d,n,0,"LZIP",4))return found("application/x-lzip","lz","lzip data");
 if(at(d,n,0,"070701",6)||at(d,n,0,"070702",6)||at(d,n,0,"070707",6))return found("application/x-cpio","cpio","cpio archive");
 if(at(d,n,0,"hsqs",4)||at(d,n,0,"sqsh",4))return found("application/vnd.squashfs","squashfs","SquashFS image",{"squashfs","sqfs","snap"});
 if(at(d,n,0,"StuffIt (c)1997",15)||at(d,n,0,"SIT!",4))return found("application/x-stuffit","sit","StuffIt archive",{"sit","sitx"});
 if(at(d,n,0,"ITSF",4))return found("application/vnd.ms-htmlhelp","chm","Compiled HTML help");
 if(at(d,n,0,"ITOLITLS",8))return found("application/x-ms-reader","lit","Microsoft Reader book");
 if(at(d,n,0,"KDMV",4))return found("application/x-vmdk","vmdk","VMware disk image");
 if(at(d,n,0,"QFI\xfb",4))return found("application/x-qemu-disk","qcow2","QEMU disk image",{"qcow2","qcow"});
 if(at(d,n,0,"vhdxfile",8))return found("application/x-vhdx","vhdx","Hyper-V disk image");
 if(at(d,n,0,"conectix",8))return found("application/x-vhd","vhd","Virtual PC disk image");
 if(at(d,n,0,"<<< Oracle VM VirtualBox Disk Image >>>",39))return found("application/x-virtualbox-vdi","vdi","VirtualBox disk image");
 if(at(d,n,0x8001,"BEA01",5)||at(d,n,0x8801,"CD001",5)||at(d,n,0x9001,"CD001",5))return found("application/x-iso9660-image","iso","Optical disc image",{"iso","img","udf"});
 if(at(d,n,0,"Cr24",4))return found("application/x-chrome-extension","crx","Chrome extension");
 // Executables and code.
 if(at(d,n,0,"\xce\xfa\xed\xfe",4)||at(d,n,0,"\xcf\xfa\xed\xfe",4)||at(d,n,0,"\xfe\xed\xfa\xce",4)||at(d,n,0,"\xfe\xed\xfa\xcf",4))return found("application/x-mach-binary","","Mach-O executable",{"dylib","bundle","o"},false);
 if(at(d,n,0,"\xca\xfe\xba\xbe",4)&&n>=8){if(be32(d+4)<=32)return found("application/x-mach-binary","","Universal Mach-O binary",{"dylib","bundle"},false);return found("application/java-vm","class","Java class file");}
 if(at(d,n,0,"\0asm",4))return found("application/wasm","wasm","WebAssembly module");
 if(at(d,n,0,"dex\n",4))return found("application/vnd.android.dex","dex","Dalvik executable");
 if(at(d,n,0,"L\0\0\0\x01\x14\x02\0\0\0\0\0\xc0\0\0\0\0\0\0\x46",20))return found("application/x-ms-shortcut","lnk","Windows shortcut");
 // Fonts. The TrueType table count is big-endian and small.
 if(at(d,n,0,"\0\x01\0\0",4)&&n>=12&&d[4]==0&&d[5]>=1&&d[5]<=64)return found("font/ttf","ttf","TrueType font");
 if(at(d,n,0,"OTTO",4))return found("font/otf","otf","OpenType font");
 if(at(d,n,0,"ttcf",4))return found("font/collection","ttc","TrueType collection");
 if(at(d,n,0,"wOFF",4))return found("font/woff","woff","Web Open Font");
 if(at(d,n,0,"wOF2",4))return found("font/woff2","woff2","Web Open Font 2");
 // Documents and e-books.
 if(at(d,n,0,"AT&TFORM",8)&&at(d,n,12,"DJV",3))return found("image/vnd.djvu","djvu","DjVu document",{"djvu","djv"});
 if(at(d,n,60,"BOOKMOBI",8))return found("application/x-mobipocket-ebook","mobi","Mobipocket e-book",{"mobi","prc","azw","azw3"});
 if(at(d,n,60,"TEXtREAd",8))return found("application/x-mobipocket-ebook","prc","Palm e-book",{"prc","pdb"});
 // Images.
 if(at(d,n,0,"\0\0\x01\0",4)&&n>=6&&le16(d+4)>=1&&le16(d+4)<=64)return found("image/x-icon","ico","Windows icon");
 if(at(d,n,0,"\0\0\x02\0",4)&&n>=6&&le16(d+4)>=1&&le16(d+4)<=64)return found("image/x-icon","cur","Windows cursor");
 if(at(d,n,0,"\0\0\0\x0cjP  \r\n\x87\n",12))return found("image/jp2","jp2","JPEG 2000 image",{"jp2","j2k","jpx","jpf"});
 if(at(d,n,0,"\xff\x4f\xff\x51",4))return found("image/jp2","j2k","JPEG 2000 codestream",{"j2k","j2c","jp2"});
 if(at(d,n,0,"\xff\x0a",2)||at(d,n,0,"\0\0\0\x0cJXL \r\n\x87\n",12))return found("image/jxl","jxl","JPEG XL image");
 if(at(d,n,0,"gimp xcf ",9))return found("image/x-xcf","xcf","GIMP image");
 if(at(d,n,0,"DDS ",4))return found("image/vnd-ms.dds","dds","DirectDraw surface");
 if(at(d,n,0,"\x76\x2f\x31\x01",4))return found("image/x-exr","exr","OpenEXR image");
 if(at(d,n,0,"#?RADIANCE",10)||at(d,n,0,"#?RGBE",6))return found("image/vnd.radiance","hdr","Radiance HDR image");
 if(at(d,n,0,"\xabKTX 11\xbb\r\n\x1a\n",12))return found("image/ktx","ktx","Khronos texture");
 if(at(d,n,0,"\xabKTX 20\xbb\r\n\x1a\n",12))return found("image/ktx2","ktx2","Khronos texture 2");
 if(at(d,n,0,"qoif",4))return found("image/qoi","qoi","QOI image");
 if((at(d,n,0,"P1",2)||at(d,n,0,"P4",2))&&pnmSeparator(d,n))return found("image/x-portable-bitmap","pbm","Portable bitmap",{"pbm","pnm"},false);
 if((at(d,n,0,"P2",2)||at(d,n,0,"P5",2))&&pnmSeparator(d,n))return found("image/x-portable-graymap","pgm","Portable graymap",{"pgm","pnm"},false);
 if((at(d,n,0,"P3",2)||at(d,n,0,"P6",2))&&pnmSeparator(d,n))return found("image/x-portable-pixmap","ppm","Portable pixmap",{"ppm","pnm"},false);
 if(at(d,n,0,"icns",4))return found("image/x-icns","icns","Apple icon image");
 if(at(d,n,0,"\0\0\0\x1c" "ftypcrx ",12))return found("image/x-canon-cr3","cr3","Canon raw image");
 if(at(d,n,0,"FUJIFILMCCD-RAW",15))return found("image/x-fuji-raf","raf","Fujifilm raw image");
 if(at(d,n,0,"IIRO",4)||at(d,n,0,"IIRS",4))return found("image/x-olympus-orf","orf","Olympus raw image");
 if(at(d,n,0,"\0MRM",4))return found("image/x-minolta-mrw","mrw","Minolta raw image");
 // Audio.
 if(at(d,n,0,"MAC ",4))return found("audio/ape","ape","Monkey's Audio");
 if(at(d,n,0,"wvpk",4))return found("audio/wavpack","wv","WavPack audio");
 if(at(d,n,0,"MPCK",4)||at(d,n,0,"MP+",3))return found("audio/musepack","mpc","Musepack audio");
 if(at(d,n,0,"TTA1",4))return found("audio/tta","tta","True Audio");
 if(at(d,n,0,"DSD ",4))return found("audio/dsf","dsf","DSD stream file");
 if(at(d,n,0,"#!AMR",5))return found("audio/amr","amr","AMR audio");
 if(at(d,n,0,".snd",4))return found("audio/basic","au","Sun audio",{"au","snd"});
 if(at(d,n,0,"MThd",4))return found("audio/midi","mid","MIDI sequence",{"mid","midi","kar"});
 if(at(d,n,0,"RIFF",4)&&at(d,n,8,"RMID",4))return found("audio/midi","rmi","RIFF MIDI",{"rmi","mid"});
 if(at(d,n,0,"caff",4))return found("audio/x-caf","caf","Core Audio file");
 if(at(d,n,0,"\x0b\x77",2))return found("audio/ac3","ac3","Dolby Digital audio",{},false);
 if(at(d,n,0,"\x7f\xfe\x80\x01",4))return found("audio/vnd.dts","dts","DTS audio",{},false);
 if(at(d,n,0,"MMMD",4))return found("audio/x-mmf","mmf","SMAF ringtone");
 if(at(d,n,0,"Extended Module: ",17))return found("audio/x-xm","xm","FastTracker module");
 if(at(d,n,0,"IMPM",4))return found("audio/x-it","it","Impulse Tracker module");
 if(at(d,n,0x2c,"SCRM",4))return found("audio/x-s3m","s3m","Scream Tracker module");
 // Video.
 if(at(d,n,0,"\x06\x0e\x2b\x34\x02\x05\x01\x01\x0d\x01\x02",11))return found("application/mxf","mxf","MXF media");
 if(at(d,n,0,"DKIF",4))return found("video/x-ivf","ivf","IVF video");
 if(at(d,n,0,"YUV4MPEG2",9))return found("video/x-yuv4mpeg","y4m","YUV4MPEG stream");
 if(at(d,n,0,"\0\0\x01\xb3",4))return found("video/mpeg","mpg","MPEG video stream",{"mpg","mpeg","m1v","m2v"},false);
 if(at(d,n,0,"RIFF",4)&&at(d,n,8,"CDXA",4))return found("video/mpeg","mpg","CD-XA MPEG video",{"mpg","dat"},false);
 if(at(d,n,0,"DVDVIDEO-VTS",12)||at(d,n,0,"DVDVIDEO-VMG",12))return found("application/octet-stream","ifo","DVD navigation data",{"ifo","bup"});
 // Data, databases, captures and models.
 if(at(d,n,0,"\xd4\xc3\xb2\xa1",4)||at(d,n,0,"\xa1\xb2\xc3\xd4",4)||at(d,n,0,"\x4d\x3c\xb2\xa1",4)||at(d,n,0,"\xa1\xb2\x3c\x4d",4))return found("application/vnd.tcpdump.pcap","pcap","Packet capture",{"pcap","cap"});
 if(at(d,n,0,"\x0a\x0d\x0d\x0a",4)&&n>=12&&(at(d,n,8,"\x1a\x2b\x3c\x4d",4)||at(d,n,8,"\x4d\x3c\x2b\x1a",4)))return found("application/x-pcapng","pcapng","Packet capture (pcapng)");
 if(at(d,n,0,"PAR1",4))return found("application/vnd.apache.parquet","parquet","Parquet table");
 if(at(d,n,0,"\x89HDF\r\n\x1a\n",8))return found("application/x-hdf5","h5","HDF5 data",{"h5","hdf5","hdf","nc"});
 if(at(d,n,0,"\x93NUMPY",6))return found("application/x-numpy-data","npy","NumPy array");
 if(at(d,n,0,"GGUF",4))return found("application/x-gguf","gguf","GGUF model");
 if(at(d,n,0,"ARROW1",6))return found("application/vnd.apache.arrow.file","arrow","Arrow table",{"arrow","feather"});
 if(at(d,n,4,"Standard Jet DB",15))return found("application/x-msaccess","mdb","Access database");
 if(at(d,n,4,"Standard ACE DB",15))return found("application/x-msaccess","accdb","Access database");
 if(at(d,n,0,"regf",4))return found("application/x-ms-registry-hive","","Registry hive",{"dat","hiv"},false);
 if(at(d,n,0,"REGEDIT4",8)||at(d,n,0,"\xff\xfeW\0i\0n\0d\0o\0w\0s\0 \0R\0e\0g\0i\0s\0t\0r\0y\0",24))return found("text/x-ms-regedit","reg","Registry script");
 if(at(d,n,0,"\x30\x82",2)&&at(d,n,4,"\x30\x82",2))return found("application/pkix-cert","","DER-encoded certificate or key store",{"der","cer","crt","p12","pfx","p7b"},false);
 if(at(d,n,0,"Kaydara FBX Binary  \0",21))return found("application/octet-stream","fbx","FBX scene");
 if(at(d,n,0,"glTF",4))return found("model/gltf-binary","glb","glTF binary model");
 if(at(d,n,0,"BLENDER",7))return found("application/x-blender","blend","Blender scene");
 if(at(d,n,0,"MDMP",4)||at(d,n,0,"PAGEDUMP",8)||at(d,n,0,"PAGEDU64",8))return found("application/x-dmp","dmp","Windows memory dump");
 // Formats written from their public specifications and checked against the libmagic test corpus (docs/content-signatures-libmagic-2026-10-23.md).
 if(n>=4&&d[1]==0xb5&&d[2]==0x2f&&d[3]==0xfd&&d[0]>=0x22&&d[0]<=0x27)return found("application/zstd","zst","Zstandard data (legacy frame)",{"zst","tzst"});
 if(at(d,n,0,"HWP Document File V",19))return found("application/x-hwp","hwp","Hangul Word Processor document (3.x)");
 if(at(d,n,0,"FRM8",4)&&at(d,n,12,"DSD ",4))return found("audio/x-dff","dff","DSDIFF audio");
 if(at(d,n,8,".FIT",4))return found("application/vnd.ant.fit","fit","FIT activity data");
 if(at(d,n,0,"0 HEAD",6))return found("text/x-gedcom","ged","GEDCOM genealogy data",{"ged","gedcom"});
 if(at(d,n,0,"PReg\x01\0\0\0",8))return found("application/octet-stream","pol","Group Policy registry settings");
 if(at(d,n,0,"UF2\n\x57\x51\x5d\x9e",8))return found("application/octet-stream","uf2","UF2 firmware image");
 if(at(d,n,0,"GCDE",4))return found("application/octet-stream","bgcode","Binary G-code");
 if((at(d,n,0,"vdex",4)||at(d,n,0,"wdex",4))&&n>=8&&d[4]>='0'&&d[4]<='9')return found("application/octet-stream","vdex","Android vdex file");
 if(at(d,n,0,"xclbin2\0",8))return found("application/octet-stream","xclbin","AMD/Xilinx accelerator binary");
 if(at(d,n,0,"\0\x09\x0f\xf0\x0f\xf0\x0f\xf0\x0f\xf0\0\0\x01",13))return found("application/octet-stream","bit","Xilinx bitstream");
 if(at(d,n,0,"KXTS\0",5))return found("application/octet-stream","kmx","Keyman compiled keyboard");
 // Tarantool files: magic line, format version line, then a "Version: " line naming the server version.
 const auto tarantool=[&](const char* magic){const size_t length=std::strlen(magic);if(!at(d,n,0,magic,length)||!at(d,n,length,"0.",2))return false;const char key[]="\nVersion: ";const uint8_t* end=d+std::min<size_t>(n,128);auto hit=std::search(d,end,key,key+10);return hit!=end&&hit+10<end&&hit[10]>='0'&&hit[10]<='9';};
 if(tarantool("XLOG\n"))return found("application/octet-stream","xlog","Tarantool write-ahead log");
 if(tarantool("SNAP\n"))return found("application/octet-stream","snap","Tarantool snapshot");
 if(auto key=openPgpKey(d,n);key.verdict==SniffVerdict::Match)return key;
 if(n>0x43a&&d[0x438]==0x53&&d[0x439]==0xef)return found("application/octet-stream","img","Linux ext2/3/4 filesystem image",{"img","ext4","ext3","ext2"},false);
 return {};
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
 if(at(d,n,0,"%PDF-",5)||at(d,n,0,"\xef\xbb\xbf%PDF-",8))return found("application/pdf","pdf","PDF document",{"pdf","ai"});
 if(svgDocument(d,n))return found("image/svg+xml","svg","SVG image",{"svg","xml"});
 if(at(d,n,0,"%!PS",4))return found("application/postscript","ps","PostScript document",{"ps","eps","ai"});
 if(at(d,n,0,"{\\rtf",5))return found("application/rtf","rtf","Rich Text document");
 if(at(d,n,0,"\xd0\xcf\x11\xe0\xa1\xb1\x1a\xe1",8))return oleCompound(d,n);
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
 if(at(d,n,0,"\x89PNG\r\n\x1a\n",8)){const uint8_t* end=d+std::min<size_t>(n,4096);const char actl[]="acTL",idat[]="IDAT";auto animated=std::search(d,end,actl,actl+4),image=std::search(d,end,idat,idat+4);
  if(animated!=end&&animated<image)return found("image/png","png","Animated PNG image",{"png","apng"});return found("image/png","png","PNG image");}
 if(at(d,n,0,"\xff\xd8\xff",3))return found("image/jpeg","jpg","JPEG image",{"jpg","jpeg","jpe","jfif"});
 if(at(d,n,0,"GIF87a",6)||at(d,n,0,"GIF89a",6))return found("image/gif","gif","GIF image");
 if(at(d,n,0,"II*\0",4)||at(d,n,0,"MM\0*",4))return found("image/tiff","tif","TIFF image",{"tif","tiff","dng","cr2","nef","arw"});
 if(at(d,n,0,"8BPS",4))return found("image/vnd.adobe.photoshop","psd","Photoshop image");
 if(at(d,n,0,"BM",2)&&n>=26&&le32(d+14)>=12&&le32(d+14)<=124)return found("image/bmp","bmp","BMP image",{"bmp","dib"},false);
 if(at(d,n,0,"FWS",3)||at(d,n,0,"CWS",3)||at(d,n,0,"ZWS",3))return found("application/x-shockwave-flash","swf","Flash movie");
 if(at(d,n,0,"SQLite format 3\0",16))return found("application/vnd.sqlite3","sqlite","SQLite database",{"sqlite","db","sqlite3"},false);
 if(at(d,n,0,"d8:announce",11)||at(d,n,0,"d13:announce-list",17))return found("application/x-bittorrent","torrent","BitTorrent file");
 return longTail(d,n);
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
 static const std::vector<std::string> list={"3gp","3g2","7z","aac","accdb","ace","ac3","aif","aiff","amr","ape","apk","appx","appxbundle","arj","arrow","arw","asf","avi","avif","azw","azw3","blend","bmp","bz2","cab","caf","cbr","cbz","chm","class","cpio","cr2","cr3","crx","cur","dds","deb","dex","dff","djvu","dll","dmg","dmp","dng","doc","docx","dsf","dts","epub","exe","exr","f4v","fit","flac","flv","gguf","gif","glb","gz","gzip","h5","hdf5","heic","hwp","hwpx","ico","ifo","img","iso","it","ivf","jar","jp2","jpeg","jpg","jxl","kmz","ktx","ktx2","lha","lit","lnk","lz","lz4","lzh","m2ts","m2v","m4a","m4b","m4v","mdb","mid","midi","mka","mkv","mobi","mov","mp2","mp3","mp4","mpa","mpc","mpe","mpeg","mpg","msg","msi","msix","msixbundle","msp","msu","mts","mxf","nef","npy","odg","odp","ods","odt","ogg","ogv","opus","orf","otf","parquet","pcap","pcapng","pdf","pkg","png","pps","ppt","pptx","prc","psd","qcow2","qt","ra","raf","rar","rm","rmvb","rpm","s3m","sit","sitx","snap","squashfs","swf","tar","tbz2","tgz","tif","tiff","ts","ttc","ttf","tta","txz","udf","uf2","vdi","vhd","vhdx","vmdk","vob","vsd","vsix","wasm","wav","webm","webp","wim","wma","wmv","woff","woff2","wv","xar","xcf","xls","xlsx","xm","xpi","xz","y4m","z","zip","zst"};
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
