using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;

namespace Udm {
    static class NetworkMonitorTests {
        static void Set(byte[] b,int o,uint v){Buffer.BlockCopy(BitConverter.GetBytes(v),0,b,o,4);}
        static void Set64(byte[] b,int o,ulong v){Buffer.BlockCopy(BitConverter.GetBytes(v),0,b,o,8);}
        static byte[] Frame(int family){var b=new byte[160];Set(b,0,1);Set(b,4,64);Set(b,8,96);Set(b,12,1);Set(b,56,1);Set64(b,64,17);Set64(b,72,1234);Set64(b,80,(ulong)DateTime.UtcNow.ToFileTimeUtc());Set64(b,88,(ulong)DateTime.UtcNow.ToFileTimeUtc());Set64(b,96,4294967300);Set64(b,104,23);Set(b,112,(uint)family);Set(b,116,6);b[120]=0x50;b[122]=0xbb;b[123]=1;Set(b,124,1);if(family==2){b[128]=127;b[131]=1;b[144]=203;b[145]=0;b[146]=113;b[147]=1;}else{b[143]=1;b[144]=0x20;b[145]=1;b[159]=1;}return b;}
        static bool Rejected(byte[] b,int length){try{WfpMonitor.Parse(b,length);return false;}catch(IOException){return true;}}
        public static void Run(Action<bool,string> check){
            var v4=WfpMonitor.Parse(Frame(2),160).Flows.Single();check(v4.Remote=="203.0.113.1:443"&&v4.Local=="127.0.0.1:80"&&v4.Received==4294967300,"driver protocol decodes network addresses, ports and 64-bit counters");
            var v6=WfpMonitor.Parse(Frame(23),160).Flows.Single();check(v6.Local=="[::1]:80"&&v6.Remote.StartsWith("[2001:"),"driver protocol supports IPv6 without exposing memory pointers");
            var malformed=Frame(2);Set(malformed,12,uint.MaxValue);check(Rejected(malformed,160)&&Rejected(Frame(2),159)&&Rejected(Frame(2),161),"oversized and truncated driver snapshots are rejected");
            malformed=Frame(2);Set(malformed,0,2);bool unknown=Rejected(malformed,160);malformed=Frame(2);Set(malformed,116,999);check(unknown&&Rejected(malformed,160),"unsupported driver ABI versions and protocols fail explicitly");
            malformed=Frame(2);Set64(malformed,80,ulong.MaxValue);check(Rejected(malformed,160),"invalid kernel timestamps never reach the UI");
            var request=WfpMonitor.WatchRequest(new[]{1234,1234,5678});bool bounded=false;try{WfpMonitor.WatchRequest(Enumerable.Range(1000,33));}catch(ArgumentException){bounded=true;}check(request.Length==144&&BitConverter.ToUInt32(request,8)==2&&bounded,"driver watch requests deduplicate PIDs and enforce the 32-process limit");
            bool system=false;try{WfpMonitor.WatchRequest(new[]{4});}catch(ArgumentException){system=true;}check(system,"system process IDs cannot enter the monitor's explicit watch list");
            using(var client=new TcpClient()){var listener=new TcpListener(IPAddress.Loopback,0);listener.Start();try{int port=((IPEndPoint)listener.LocalEndpoint).Port;client.Connect(IPAddress.Loopback,port);using(var server=listener.AcceptTcpClient()){int id=Process.GetCurrentProcess().Id;var rows=WindowsConnections.Read(new[]{id});check(rows.Any(r=>r.ProcessId==(ulong)id&&r.Remote=="127.0.0.1:"+port)&&rows.All(r=>r.ProcessId==(ulong)id),"live Windows connection attribution finds the test socket and stays within the selected process");}}finally{listener.Stop();}}
            foreach(var address in new[]{IPAddress.Loopback,IPAddress.IPv6Loopback})using(var udp=new UdpClient(address.AddressFamily)){
                udp.Client.Bind(new IPEndPoint(address,0));string local=udp.Client.LocalEndPoint.ToString();int id=Process.GetCurrentProcess().Id;
                var rows=WindowsConnections.ReadAll(new[]{id});check(rows.Any(r=>r.ProcessId==(ulong)id&&r.Transport=="UDP"&&r.Local==local&&r.Remote=="—")&&rows.All(r=>r.ProcessId==(ulong)id),"live "+address.AddressFamily+" UDP endpoints are attributed without inventing a remote address");
            }
        }
    }
}
