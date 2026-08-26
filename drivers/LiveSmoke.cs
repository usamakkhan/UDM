using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Threading;

namespace Udm {
    // Opt-in test executable. It never installs a service or changes boot/trust settings.
    public static class DriverSmoke {
        static int checks;
        static void Check(bool passed,string description){if(!passed)throw new IOException(description);Console.WriteLine("PASS "+description);checks++;}
        static void Tcp(IPAddress address){
            var listener=new TcpListener(address,0);listener.Start();
            try{using(var client=new TcpClient(address.AddressFamily)){
                client.Connect((IPEndPoint)listener.LocalEndpoint);using(var server=listener.AcceptTcpClient()){
                    client.SendTimeout=server.ReceiveTimeout=5000;client.ReceiveTimeout=server.SendTimeout=5000;
                    byte[] sent=Enumerable.Range(0,65536).Select(i=>(byte)i).ToArray(),received=new byte[sent.Length];
                    client.GetStream().Write(sent,0,sent.Length);int offset=0;
                    while(offset<received.Length){int n=server.GetStream().Read(received,offset,received.Length-offset);if(n==0)throw new EndOfStreamException();offset+=n;}
                    if(!sent.SequenceEqual(received))throw new IOException("TCP fixture was altered.");
                }
            }}finally{listener.Stop();}
        }
        static void Udp(IPAddress address){using(var server=new UdpClient(new IPEndPoint(address,0)))using(var client=new UdpClient(address.AddressFamily)){
            server.Client.ReceiveTimeout=5000;var bytes=new byte[512];new Random(41).NextBytes(bytes);
            client.Send(bytes,bytes.Length,(IPEndPoint)server.Client.LocalEndPoint);IPEndPoint sender=null;
            if(!server.Receive(ref sender).SequenceEqual(bytes))throw new IOException("UDP fixture was altered.");
        }}
        public static int Main(string[] args){try{
            if(args.Length==1&&args[0]=="--traffic-only"){Tcp(IPAddress.Loopback);Udp(IPAddress.Loopback);return 0;}
            if(args.Length!=1||args[0]!="--run"){Console.WriteLine("Use --run after separately installing/loading the test driver. No kernel tests ran.");return 2;}
            var ready=DriverReadiness.Read();if(!ready.Administrator)throw new IOException("Run this test from an elevated terminal.");
            using(var monitor=new WfpMonitor()){
                monitor.Watch(new int[0]);var baseline=monitor.Read();
                Check(baseline.Watched==0&&baseline.Flows.Count==0,"empty watch list disables visible collection");
                using(var child=Process.Start(new ProcessStartInfo(Process.GetCurrentProcess().MainModule.FileName,"--traffic-only"){UseShellExecute=false,CreateNoWindow=true})){
                    if(!child.WaitForExit(15000)){child.Kill();throw new IOException("Unwatched traffic fixture timed out.");}
                    Check(child.ExitCode==0,"unwatched traffic passes unchanged");
                }
                Check(monitor.Read().Connections==baseline.Connections,"unwatched connections do not increase collected-flow count");
                int pid=Process.GetCurrentProcess().Id;monitor.Watch(new[]{pid});
                foreach(var address in new[]{IPAddress.Loopback,IPAddress.IPv6Loopback}){
                    var before=monitor.Read();Tcp(address);var after=monitor.Read();
                    Check(after.Connections>before.Connections&&after.Sent>before.Sent&&after.Received>before.Received,"scoped "+address.AddressFamily+" TCP attribution and counters");
                    before=monitor.Read();Udp(address);after=monitor.Read();
                    Check(after.Connections>before.Connections&&after.Sent>before.Sent&&after.Received>before.Received,"scoped "+address.AddressFamily+" UDP attribution and counters");
                }
                var snapshot=monitor.Read();Check(snapshot.Watched==1&&snapshot.Flows.Count>0&&snapshot.Flows.All(f=>f.ProcessId==(ulong)pid),"snapshot contains only the selected process");
                monitor.Watch(new int[0]);Check(monitor.Read().Flows.Count==0,"clearing the watch list removes visible flows");
            }
            using(var reopened=new WfpMonitor())Check(reopened.Read().Watched==0,"closing the client clears scope and releases the exclusive device");
            Console.WriteLine("ALL "+checks+" LIVE DRIVER CHECKS PASSED. This is a smoke test, not Driver Verifier or release certification.");return 0;
        }catch(Exception ex){Console.Error.WriteLine("FAIL "+ex.Message);return 1;}}
    }
}
