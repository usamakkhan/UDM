param(
 [Parameter(Mandatory=$true)][string]$SourceFolder,
 [Parameter(Mandatory=$true)][string]$OutputPath
)
$ErrorActionPreference='Stop'
$source=(Get-Item -LiteralPath $SourceFolder).FullName
$target=[IO.Path]::GetFullPath($OutputPath)
if(Test-Path -LiteralPath $target){throw 'Use a new ISO path.'}
if((Get-Item -LiteralPath $source).Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Source must not redirect.'}
foreach($item in Get-ChildItem -LiteralPath $source -Recurse -Force){if($item.Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Source tree must not redirect.'}}
Add-Type -TypeDefinition @'
using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;
public static class UdmIsoStream {
 public static void Copy(object value,string path) {
  IStream stream=(IStream)value;byte[] buffer=new byte[65536];IntPtr read=Marshal.AllocHGlobal(4);
  try {using(FileStream output=new FileStream(path,FileMode.CreateNew,FileAccess.Write)) {
   for(;;){stream.Read(buffer,buffer.Length,read);int count=Marshal.ReadInt32(read);if(count==0)break;output.Write(buffer,0,count);}
   output.Flush(true);
  }} finally {Marshal.FreeHGlobal(read);}
 }
}
'@
$image=$null;$result=$null;$stream=$null
try {
 $image=New-Object -ComObject IMAPI2FS.MsftFileSystemImage
 $image.WorkingDirectory=[IO.Path]::GetDirectoryName($target)
 $image.StageFiles=$false
 $image.FileSystemsToCreate=3
 $image.VolumeName='UDM_SETUP'
 $image.Root.AddTree($source,$false)
 $result=$image.CreateResultImage();$stream=$result.ImageStream
 [UdmIsoStream]::Copy($stream,$target)
} finally {
 foreach($object in @($stream,$result,$image)){if($null -ne $object){[Runtime.InteropServices.Marshal]::FinalReleaseComObject($object) | Out-Null}}
}
[ordered]@{path=$target;bytes=(Get-Item -LiteralPath $target).Length;sha256=(Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash} | ConvertTo-Json
