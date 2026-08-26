using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net;
using System.Runtime.InteropServices;
using System.Threading;
using System.Windows.Forms;
using Microsoft.Win32.SafeHandles;

namespace Udm {
    public sealed class NetworkFlow {
        public ulong Id,ProcessId,Received,Sent;public string Local,Remote,State,Transport;public DateTime? Opened,LastSeen;
    }
    public sealed class NetworkSnapshot {
        public ulong Generation,Dropped,Connections,Received,Sent;public uint Watched;public List<NetworkFlow> Flows=new List<NetworkFlow>();
    }
    public sealed class WfpMonitor:IDisposable {
        const uint WatchIoctl=0x8000a000,SnapshotIoctl=0x80006004;
        public const int HeaderSize=64,RecordSize=96,MaxFlows=128,MaxPids=32;
        readonly SafeFileHandle handle;
        [DllImport("kernel32.dll",CharSet=CharSet.Unicode,SetLastError=true)]static extern SafeFileHandle CreateFile(string name,uint access,uint share,IntPtr security,uint creation,uint flags,IntPtr template);
        [DllImport("kernel32.dll",SetLastError=true)]static extern bool DeviceIoControl(SafeFileHandle device,uint code,byte[] input,int inputSize,byte[] output,int outputSize,out int returned,IntPtr overlapped);
        public WfpMonitor(){handle=CreateFile(@"\\.\UdmWfp",0xc0000000,0,IntPtr.Zero,3,0,IntPtr.Zero);if(handle.IsInvalid){int code=Marshal.GetLastWin32Error();handle.Dispose();throw new Win32Exception(code,code==2?"UDM's WFP driver is not installed.":code==5?"WFP monitoring requires an elevated UDM monitor.":"Cannot open UDM's WFP monitor.");}}
        public static byte[] WatchRequest(IEnumerable<int> ids){var pids=ids.Distinct().ToArray();if(pids.Length>MaxPids||pids.Any(p=>p<=4))throw new ArgumentException("Choose up to 32 ordinary process IDs.");var bytes=new byte[144];Write(bytes,0,1);Write(bytes,4,144);Write(bytes,8,(uint)pids.Length);for(int i=0;i<pids.Length;i++)Write(bytes,16+i*4,(uint)pids[i]);return bytes;}
        static void Write(byte[] bytes,int offset,uint value){Buffer.BlockCopy(BitConverter.GetBytes(value),0,bytes,offset,4);}
        public void Watch(IEnumerable<int> ids){byte[] bytes=WatchRequest(ids);int count;if(!DeviceIoControl(handle,WatchIoctl,bytes,bytes.Length,null,0,out count,IntPtr.Zero))throw new Win32Exception(Marshal.GetLastWin32Error());}
        public NetworkSnapshot Read(){var bytes=new byte[HeaderSize+MaxFlows*RecordSize];int count;if(!DeviceIoControl(handle,SnapshotIoctl,null,0,bytes,bytes.Length,out count,IntPtr.Zero))throw new Win32Exception(Marshal.GetLastWin32Error());return Parse(bytes,count);}
        static string Address(byte[] bytes,int offset,int family,int port){var address=new byte[family==2?4:16];Buffer.BlockCopy(bytes,offset,address,0,address.Length);return (family==2?new IPAddress(address).ToString():"["+new IPAddress(address)+"]")+":"+port;}
        static DateTime? Timestamp(ulong value){if(value==0)return null;try{return DateTime.FromFileTimeUtc(checked((long)value));}catch(Exception ex){if(ex is ArgumentOutOfRangeException||ex is OverflowException)throw new IOException("Invalid driver timestamp.",ex);throw;}}
        public static NetworkSnapshot Parse(byte[] b,int length){
            if(b==null||length<HeaderSize||length>b.Length||length>HeaderSize+MaxFlows*RecordSize)throw new IOException("Invalid driver snapshot size.");
            uint version=BitConverter.ToUInt32(b,0),header=BitConverter.ToUInt32(b,4),size=BitConverter.ToUInt32(b,8),count=BitConverter.ToUInt32(b,12),watched=BitConverter.ToUInt32(b,56);
            if(version!=1||header!=HeaderSize||size!=RecordSize||count>MaxFlows||length!=HeaderSize+count*RecordSize||watched>MaxPids||BitConverter.ToUInt32(b,60)!=0)throw new IOException("Unsupported or malformed driver snapshot.");
            var result=new NetworkSnapshot{Generation=BitConverter.ToUInt64(b,16),Dropped=BitConverter.ToUInt64(b,24),Connections=BitConverter.ToUInt64(b,32),Received=BitConverter.ToUInt64(b,40),Sent=BitConverter.ToUInt64(b,48),Watched=watched};var seen=new HashSet<ulong>();
            for(int i=0;i<count;i++){int o=HeaderSize+i*RecordSize,family=(int)BitConverter.ToUInt32(b,o+48),protocol=(int)BitConverter.ToUInt32(b,o+52);uint flags=BitConverter.ToUInt32(b,o+60);ulong id=BitConverter.ToUInt64(b,o),pid=BitConverter.ToUInt64(b,o+8);if((family!=2&&family!=23)||(protocol!=6&&protocol!=17)||(flags!=1&&flags!=2)||id==0||pid<=4||pid>int.MaxValue||!seen.Add(id))throw new IOException("Invalid driver flow record.");result.Flows.Add(new NetworkFlow{Id=id,ProcessId=pid,Received=BitConverter.ToUInt64(b,o+32),Sent=BitConverter.ToUInt64(b,o+40),Local=Address(b,o+64,family,BitConverter.ToUInt16(b,o+56)),Remote=Address(b,o+80,family,BitConverter.ToUInt16(b,o+58)),Opened=Timestamp(BitConverter.ToUInt64(b,o+16)),LastSeen=Timestamp(BitConverter.ToUInt64(b,o+24)),State=flags==1?"Active":"Closed",Transport=protocol==6?"TCP":"UDP"});}return result;
        }
        public void Dispose(){handle.Dispose();}
    }
    public static class WindowsConnections {
        [DllImport("iphlpapi.dll",SetLastError=true)]static extern uint GetExtendedTcpTable(IntPtr table,ref int size,bool order,int family,int tableClass,uint reserved);
        [DllImport("iphlpapi.dll",SetLastError=true)]static extern uint GetExtendedUdpTable(IntPtr table,ref int size,bool order,int family,int tableClass,uint reserved);
        public static List<NetworkFlow> ReadUdp(IEnumerable<int> ids){
            var selected=new HashSet<ulong>(ids.Select(i=>(ulong)i));var rows=new List<NetworkFlow>();
            foreach(int family in new[]{2,23}){
                int size=0;uint error=GetExtendedUdpTable(IntPtr.Zero,ref size,false,family,1,0);
                if(error!=122&&error!=0)throw new Win32Exception((int)error);
                for(int attempt=0;attempt<3;attempt++){
                    if(size<4||size>2*1024*1024)throw new IOException("UDP endpoint table size is invalid.");
                    int allocated=size;IntPtr memory=Marshal.AllocHGlobal(allocated);
                    try{
                        error=GetExtendedUdpTable(memory,ref size,false,family,1,0);
                        if(error==122){if(attempt==2)throw new IOException("UDP endpoints are changing too quickly; retrying next refresh.");continue;}
                        if(error!=0)throw new Win32Exception((int)error);
                        if(size<4||size>allocated)throw new IOException("Invalid UDP endpoint table size.");
                        var bytes=new byte[size];Marshal.Copy(memory,bytes,0,size);int count=BitConverter.ToInt32(bytes,0),stride=family==2?12:28;
                        if(count<0||count>20000||4+(long)count*stride>size)throw new IOException("Invalid UDP endpoint table.");
                        for(int i=0;i<count;i++){
                            int o=4+i*stride;uint pid=BitConverter.ToUInt32(bytes,o+(family==2?8:24));if(!selected.Contains(pid))continue;
                            var address=new byte[family==2?4:16];Buffer.BlockCopy(bytes,o,address,0,address.Length);
                            var ip=family==2?new IPAddress(address):new IPAddress(address,BitConverter.ToUInt32(bytes,o+16));int port=o+(family==2?4:20);
                            rows.Add(new NetworkFlow{ProcessId=pid,Transport="UDP",Local=new IPEndPoint(ip,bytes[port]*256+bytes[port+1]).ToString(),Remote="—",State="Bound"});
                        }break;
                    }finally{Marshal.FreeHGlobal(memory);}
                }
            }return rows;
        }
        public static List<NetworkFlow> ReadAll(IEnumerable<int> ids){var selected=ids.ToArray();var rows=Read(selected);rows.AddRange(ReadUdp(selected));return rows;}
        public static List<NetworkFlow> Read(IEnumerable<int> ids){var selected=new HashSet<ulong>(ids.Select(i=>(ulong)i));var rows=new List<NetworkFlow>();foreach(int family in new[]{2,23}){
            int size=0;uint error=GetExtendedTcpTable(IntPtr.Zero,ref size,false,family,5,0);if(error!=122&&error!=0)throw new Win32Exception((int)error);if(size<4||size>2*1024*1024)throw new IOException("Connection table size is invalid.");
            for(int attempt=0;attempt<3;attempt++){
                int allocated=size;IntPtr memory=Marshal.AllocHGlobal(allocated);
                try{
                    error=GetExtendedTcpTable(memory,ref size,false,family,5,0);
                    if(error==122){if(size<4||size>2*1024*1024)throw new IOException("Connection table grew beyond its limit.");if(attempt==2)throw new IOException("Connections are changing too quickly; retrying next refresh.");continue;}
                    if(error!=0)throw new Win32Exception((int)error);
                    if(size<4||size>allocated)throw new IOException("Invalid connection table size.");
                    var bytes=new byte[size];Marshal.Copy(memory,bytes,0,size);int count=BitConverter.ToInt32(bytes,0),stride=family==2?24:56;
                    if(count<0||count>20000||4+(long)count*stride>size)throw new IOException("Invalid connection table.");
                    for(int i=0;i<count;i++){
                        int o=4+i*stride;uint pid=BitConverter.ToUInt32(bytes,o+(family==2?20:52));if(!selected.Contains(pid))continue;
                        int localOffset=family==2?4:0,remoteOffset=family==2?12:24,localPort=family==2?8:20,remotePort=family==2?16:44;
                        var local=new byte[family==2?4:16];var remote=new byte[local.Length];Buffer.BlockCopy(bytes,o+localOffset,local,0,local.Length);Buffer.BlockCopy(bytes,o+remoteOffset,remote,0,remote.Length);
                        IPAddress localAddress=family==2?new IPAddress(local):new IPAddress(local,BitConverter.ToUInt32(bytes,o+16));
                        IPAddress remoteAddress=family==2?new IPAddress(remote):new IPAddress(remote,BitConverter.ToUInt32(bytes,o+40));
                        uint state=BitConverter.ToUInt32(bytes,o+(family==2?0:48));
                        rows.Add(new NetworkFlow{ProcessId=pid,Transport="TCP",Local=new IPEndPoint(localAddress,bytes[o+localPort]*256+bytes[o+localPort+1]).ToString(),Remote=new IPEndPoint(remoteAddress,bytes[o+remotePort]*256+bytes[o+remotePort+1]).ToString(),State=state==5?"Established":state==2?"Listening":"TCP state "+state});
                    }break;
                }finally{Marshal.FreeHGlobal(memory);}
            }
        }return rows;}
        public static Dictionary<int,string> RelevantProcesses(){int session=Process.GetCurrentProcess().SessionId;var result=new Dictionary<int,string>();foreach(var p in Process.GetProcesses())using(p)try{if(p.SessionId==session&&new[]{"UDM","chrome","msedge","firefox"}.Contains(p.ProcessName,StringComparer.OrdinalIgnoreCase))result[p.Id]=p.ProcessName;}catch(InvalidOperationException){}catch(Win32Exception){}return result;}
    }
    public sealed class NetworkMonitorDialog:Form {
        readonly System.Windows.Forms.Timer timer=new System.Windows.Forms.Timer{Interval=1000};
        WfpMonitor driver;readonly ListView table;readonly Label status,driverStatus;readonly Button start,stop;
        readonly Dictionary<int,DateTime> watched=new Dictionary<int,DateTime>();
        public NetworkMonitorDialog(){
            ClassicLayout.Form(this,"UDM network integration",550,320);MinimizeBox=false;
            ClassicLayout.Label(this,"Browser and UDM connections",7,8,530);
            status=ClassicLayout.Label(this,"Reading connections...",7,27,536,25);
            driverStatus=ClassicLayout.Label(this,"Checking the optional driver...",7,54,536,25);
            table=ClassicLayout.Place(this,new ListView{View=View.Details,FullRowSelect=true,HideSelection=false},7,84,536,180);
            foreach(var item in new[]{Tuple.Create("Process",80),Tuple.Create("Local endpoint",125),Tuple.Create("Remote endpoint",125),Tuple.Create("Protocol",43),Tuple.Create("Status",55),Tuple.Create("Received",50),Tuple.Create("Sent",50)})table.Columns.Add(item.Item1,ClassicLayout.X(item.Item2));
            start=ClassicLayout.Button(this,"Start WFP monitor",7,272,108,StartMonitor);
            stop=ClassicLayout.Button(this,"Stop WFP monitor",122,272,108,StopMonitor);
            ClassicLayout.Button(this,"Driver status",237,272,94,()=>{var ready=CheckDriver();MessageBox.Show(this,ready.ToString(),"UDM driver status",MessageBoxButtons.OK,MessageBoxIcon.Information);});
            ClassicLayout.Button(this,"Close",488,296,55,Close);
            ClassicLayout.Label(this,"UDP lists local endpoints; Windows does not supply their remote peers here. WFP counters cover new watched flows, not completed file bytes.",7,296,473,22);
            timer.Tick+=delegate{RefreshData();};Shown+=delegate{CheckDriver();RefreshData();timer.Start();};
            FormClosed+=delegate{timer.Dispose();if(driver!=null)driver.Dispose();};AppTheme.Apply(this);
        }
        DriverReadiness CheckDriver(){
            var ready=DriverReadiness.Read();if(driver!=null)ready.DriverStatus="Available; this window owns monitoring.";
            driverStatus.Text=ready.DriverStatus;start.Enabled=driver==null&&ready.DriverStatus=="Available";stop.Enabled=driver!=null;return ready;
        }
        void StartMonitor(){
            if(driver!=null)return;
            try{
                driver=new WfpMonitor();var processes=WindowsConnections.RelevantProcesses();
                var active=new[]{Process.GetCurrentProcess().Id}.Concat(WindowsConnections.ReadAll(processes.Keys).Select(x=>(int)x.ProcessId)).Concat(processes.Keys).Distinct().Take(WfpMonitor.MaxPids);
                foreach(int id in active)try{using(var p=Process.GetProcessById(id))watched[id]=p.StartTime;}catch(ArgumentException){}catch(InvalidOperationException){}
                driver.Watch(watched.Keys);driverStatus.Text="WFP monitor connected. Up to 32 existing processes are selected.";
            }catch{StopMonitor();throw;}
            start.Enabled=false;stop.Enabled=true;RefreshData();
        }
        void StopMonitor(){if(driver!=null){driver.Dispose();driver=null;}watched.Clear();CheckDriver();RefreshData();}
        void RefreshData(){
            try{
                var processes=WindowsConnections.RelevantProcesses();List<NetworkFlow> rows;
                if(driver==null){rows=WindowsConnections.ReadAll(processes.Keys);status.Text="Windows connections · "+rows.Count(x=>x.Transport=="TCP")+" TCP · "+rows.Count(x=>x.Transport=="UDP")+" UDP. Byte counts require the optional driver.";}
                else{
                    foreach(int id in watched.Keys.ToArray())try{using(var p=Process.GetProcessById(id))if(p.StartTime!=watched[id])watched.Remove(id);}catch(ArgumentException){watched.Remove(id);}catch(InvalidOperationException){watched.Remove(id);}
                    driver.Watch(watched.Keys);var snapshot=driver.Read();rows=snapshot.Flows;
                    status.Text="WFP monitoring "+snapshot.Watched+" processes · "+snapshot.Dropped+" dropped or incomplete observations.";
                }
                string selected=table.SelectedItems.Count==0?null:table.SelectedItems[0].Name;
                table.BeginUpdate();
                try{
                    table.Items.Clear();
                    foreach(var row in rows.OrderBy(r=>r.ProcessId).ThenBy(r=>r.Transport).ThenBy(r=>r.Local)){
                        string name;processes.TryGetValue((int)row.ProcessId,out name);
                        var item=new ListViewItem((name??"Process")+" ("+row.ProcessId+")"){Name=row.ProcessId+"|"+row.Transport+"|"+row.Local+"|"+row.Remote};
                        item.SubItems.Add(row.Local);item.SubItems.Add(row.Remote);item.SubItems.Add(row.Transport);item.SubItems.Add(row.State);
                        item.SubItems.Add(driver==null?"—":Names.Bytes(row.Received));item.SubItems.Add(driver==null?"—":Names.Bytes(row.Sent));table.Items.Add(item);item.Selected=item.Name==selected;
                    }
                }finally{table.EndUpdate();}
            }catch(Exception ex){status.Text=ex.Message;}
        }
    }
    public static class MonitorProgram {
        public static int Main(string[] args){try{
            if(args.Length==1&&args[0]=="--diagnose"){Console.WriteLine(new System.Web.Script.Serialization.JavaScriptSerializer().Serialize(DriverReadiness.Read()));return 0;}
            if(args.Length==0||args[0]=="--status"){try{using(var driver=new WfpMonitor()){driver.Read();Console.WriteLine("UDM WFP monitor is available (protocol 1).");}}catch(Win32Exception ex){Console.WriteLine(ex.Message);return ex.NativeErrorCode==2?2:1;}return 0;}
            if(args.Length<2||args[0]!="--watch")throw new ArgumentException("Usage: Udm.Monitor.exe --status | --watch PID[,PID...] [seconds]");int seconds=args.Length>2?int.Parse(args[2]):10;if(seconds<1||seconds>3600)throw new ArgumentException("Use 1–3600 seconds.");var ids=args[1].Split(',').Select(int.Parse).Distinct().ToArray();WfpMonitor.WatchRequest(ids);var identities=new Dictionary<int,DateTime>();foreach(int id in ids)using(var p=Process.GetProcessById(id))identities[id]=p.StartTime;
            using(var driver=new WfpMonitor()){for(int i=0;i<seconds;i++){foreach(int id in identities.Keys.ToArray())try{using(var p=Process.GetProcessById(id))if(p.StartTime!=identities[id])identities.Remove(id);}catch(ArgumentException){identities.Remove(id);}driver.Watch(identities.Keys);var snapshot=driver.Read();Console.WriteLine(new System.Web.Script.Serialization.JavaScriptSerializer().Serialize(snapshot));Thread.Sleep(1000);}}return 0;
        }catch(Exception ex){Console.Error.WriteLine(ex.Message);return 1;}}
    }
}
