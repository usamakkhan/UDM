"""Read-only storage context for finishing measurements; never changes disk settings."""
import argparse
import ctypes
import json
import struct
import subprocess
from ctypes import wintypes
from pathlib import Path

PS = r'''
$mapping=@(Get-CimInstance Win32_DiskDrive | ForEach-Object {
 $disk=$_
 foreach ($part in @(Get-CimAssociatedInstance -InputObject $disk -Association Win32_DiskDriveToDiskPartition)) {
  $letters=@(Get-CimAssociatedInstance -InputObject $part -Association Win32_LogicalDiskToPartition | Select-Object -ExpandProperty DeviceID -Unique)
  [pscustomobject]@{DiskIndex=$disk.Index;Model=$disk.Model;PartitionType=$part.Type;DriveLetters=$letters}
 }
})
@{Mappings=$mapping;Physical=@(Get-PhysicalDisk | Select-Object FriendlyName,MediaType,BusType,HealthStatus);Volumes=@(Get-Volume -DriveLetter C,D | Select-Object DriveLetter,FileSystem,AllocationUnitSize,Size)} | ConvertTo-Json -Depth 6
'''


def query_seek(path):
    kernel=ctypes.WinDLL('kernel32',use_last_error=True)
    kernel.CreateFileW.argtypes=[wintypes.LPCWSTR,wintypes.DWORD,wintypes.DWORD,ctypes.c_void_p,wintypes.DWORD,wintypes.DWORD,wintypes.HANDLE]
    kernel.CreateFileW.restype=wintypes.HANDLE
    kernel.DeviceIoControl.argtypes=[wintypes.HANDLE,wintypes.DWORD,ctypes.c_void_p,wintypes.DWORD,ctypes.c_void_p,wintypes.DWORD,ctypes.POINTER(wintypes.DWORD),ctypes.c_void_p]
    kernel.CloseHandle.argtypes=[wintypes.HANDLE]
    handle=kernel.CreateFileW(path,0,3,None,3,0,None)
    if handle==ctypes.c_void_p(-1).value:return {'openError':ctypes.get_last_error()}
    try:
        query=ctypes.create_string_buffer(struct.pack('<II',7,0)+bytes(4))
        output=ctypes.create_string_buffer(64);size=wintypes.DWORD()
        ok=kernel.DeviceIoControl(handle,0x2d1400,query,12,output,64,ctypes.byref(size),None)
        result={'succeeded':bool(ok),'returnedBytes':size.value}
        if not ok:result['error']=ctypes.get_last_error();return result
        if size.value<12:return {**result,'descriptorValid':False}
        version,length=struct.unpack_from('<II',output.raw)
        valid=version>=12 and 12<=length<=size.value<=64 and output.raw[8]in (0,1)
        result.update({'descriptorVersion':version,'descriptorSize':length,'descriptorValid':valid})
        if valid:result['incursSeekPenalty']=bool(output.raw[8])
        return result
    finally:kernel.CloseHandle(handle)


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--output',type=Path);args=parser.parse_args()
    p=subprocess.run(['powershell.exe','-NoProfile','-Command',PS],capture_output=True,text=True,check=True)
    context=json.loads(p.stdout.lstrip('\ufeff'))
    context['seekQueries']={name:query_seek('\\\\.\\'+name)for name in ['C:','D:','PhysicalDrive0','PhysicalDrive1']}
    context.update({'readOnly':True,'dataPayloadBytesWritten':0,'cachePolicyChanged':False,'elevated':bool(ctypes.windll.shell32.IsUserAnAdmin()),'kernelCauseVerified':False,'limits':'Device topology and reported seek capability do not attribute a particular flush delay. Failed queries remain unknown. Compare both applications on the same target volume.', 'descriptorReference':'https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ns-winioctl-device_seek_penalty_descriptor'})
    text=json.dumps(context,indent=2)
    if args.output:args.output.write_text(text,encoding='utf-8')
    print(text)


if __name__=='__main__':main()
