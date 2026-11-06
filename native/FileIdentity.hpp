#pragma once
#include "Core.hpp"
#include <algorithm>

namespace udm {
inline Json stableFileIdentity(HANDLE file){
 FILE_ID_INFO id{};FILE_BASIC_INFO basic{};FILE_STANDARD_INFO standard{};
 if(!GetFileInformationByHandleEx(file,FileIdInfo,&id,sizeof(id))||
    !GetFileInformationByHandleEx(file,FileBasicInfo,&basic,sizeof(basic))||
    !GetFileInformationByHandleEx(file,FileStandardInfo,&standard,sizeof(standard))||
    standard.Directory||standard.DeletePending||standard.EndOfFile.QuadPart<0||
    (basic.FileAttributes&FILE_ATTRIBUTE_REPARSE_POINT)||
    std::none_of(std::begin(id.FileId.Identifier),std::end(id.FileId.Identifier),[](BYTE b){return b!=0;}))return Json();
 return {{"Volume",id.VolumeSerialNumber},{"Id",Bytes(std::begin(id.FileId.Identifier),std::end(id.FileId.Identifier))},
         {"Size",standard.EndOfFile.QuadPart},{"Created",basic.CreationTime.QuadPart},{"Written",basic.LastWriteTime.QuadPart}};
}
}
