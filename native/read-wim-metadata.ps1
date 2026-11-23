param([Parameter(Mandatory=$true)][string]$ImagePath)
$ErrorActionPreference='Stop'
$path=(Get-Item -LiteralPath $ImagePath).FullName
Add-Type -TypeDefinition @'
using System;
using System.ComponentModel;
using System.Runtime.InteropServices;
public static class UdmWimMetadata {
 [DllImport("wimgapi.dll",CharSet=CharSet.Unicode,SetLastError=true)]
 static extern IntPtr WIMCreateFile(string path,uint access,uint disposition,uint flags,uint compression,out uint result);
 [DllImport("wimgapi.dll",SetLastError=true)]
 static extern bool WIMGetImageInformation(IntPtr handle,out IntPtr info,out uint size);
 [DllImport("wimgapi.dll")] static extern bool WIMCloseHandle(IntPtr handle);
 [DllImport("kernel32.dll")] static extern IntPtr LocalFree(IntPtr memory);
 public static string Read(string path) {
  uint result;IntPtr handle=WIMCreateFile(path,0x80000000,3,0,0,out result);
  if(handle==IntPtr.Zero||handle==new IntPtr(-1))throw new Win32Exception(Marshal.GetLastWin32Error());
  IntPtr memory=IntPtr.Zero;
  try {uint size;if(!WIMGetImageInformation(handle,out memory,out size))throw new Win32Exception(Marshal.GetLastWin32Error());
   if(size==0||size>1048576||size%2!=0)throw new InvalidOperationException("Unexpected WIM metadata size");
   return Marshal.PtrToStringUni(memory,(int)size/2).Trim('\0','\uFEFF');
  } finally {if(memory!=IntPtr.Zero)LocalFree(memory);WIMCloseHandle(handle);}
 }
}
'@
[xml]$document=[UdmWimMetadata]::Read($path)
$images=@($document.WIM.IMAGE | ForEach-Object {
 [ordered]@{index=[int]$_.INDEX;name=[string]$_.NAME;edition=[string]$_.WINDOWS.EDITIONID;totalExpandedBytes=[long]$_.TOTALBYTES;build=[string]$_.WINDOWS.VERSION.BUILD;architecture=[string]$_.WINDOWS.ARCH}
})
[ordered]@{readOnly=$true;imagePath=$path;images=$images} | ConvertTo-Json -Depth 5
