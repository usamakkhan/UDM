using System;
using System.Runtime.InteropServices;
using System.Security.Principal;

namespace Udm {
    public sealed class DriverReadiness {
        public bool Administrator;
        public bool? CodeIntegrityEnabled,TestSignedDriversAllowed,MemoryIntegrityEnabled;
        public string QueryError,DriverStatus;
        [StructLayout(LayoutKind.Sequential)] struct Integrity {public uint Length,Options;}
        [DllImport("ntdll.dll")]static extern int NtQuerySystemInformation(int informationClass,ref Integrity information,int size,out int returned);
        public static DriverReadiness Read(){
            var result=new DriverReadiness();using(var identity=WindowsIdentity.GetCurrent())result.Administrator=new WindowsPrincipal(identity).IsInRole(WindowsBuiltInRole.Administrator);
            try{var data=new Integrity{Length=8};int returned;int status=NtQuerySystemInformation(103,ref data,8,out returned);if(status!=0||returned!=8)result.QueryError="Code Integrity query failed (0x"+status.ToString("X8")+").";else{result.CodeIntegrityEnabled=(data.Options&1)!=0;result.TestSignedDriversAllowed=(data.Options&2)!=0;result.MemoryIntegrityEnabled=(data.Options&0x400)!=0;}}
            catch(Exception ex){if(!(ex is EntryPointNotFoundException)&&!(ex is DllNotFoundException))throw;result.QueryError=ex.Message;}
            try{using(var driver=new WfpMonitor()){driver.Read();result.DriverStatus="Available";}}catch(Exception ex){result.DriverStatus=ex.Message;}
            return result;
        }
        static string State(bool? value){return value.HasValue?(value.Value?"On":"Off"):"Unknown";}
        public override string ToString(){return "Administrator: "+(Administrator?"Yes":"No")+"\nKernel Code Integrity: "+State(CodeIntegrityEnabled)+"\nTest-signed drivers permitted: "+State(TestSignedDriversAllowed)+"\nMemory Integrity: "+State(MemoryIntegrityEnabled)+"\n\n"+DriverStatus+(QueryError==null?"":"\n"+QueryError)+"\n\nThis check reads Windows status only. A development certificate is not Microsoft production signing.";}
    }
}
