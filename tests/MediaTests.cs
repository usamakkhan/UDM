using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Web.Script.Serialization;
namespace Udm {
    public static class MediaTests {
        public static async Task Run(string root,Action<bool,string> check) {
            string title="AADAT INSTRUMENTAL/BHANWARAY feat. Goher Mumtaz | NESCAFÉ Basement Season 5 | 2019";
            check(Names.MediaTitle(title)=="AADAT INSTRUMENTAL_BHANWARAY feat. Goher Mumtaz _ NESCAFÉ Basement Season 5 _ 2019.mp4","video titles preserve slashes as text and periods inside artist names");
            check(Names.MediaTitle("clip.MP4")=="clip.mp4" && Names.MediaTitle(null)=="YouTube video.mp4","media filenames use one MP4 extension and a safe empty-title fallback");
            var exact=new Download{MediaHeight=1080,MediaPixelHeight=804,ExactMediaQuality=true,MediaFormatId="137"};
            var roundtrip=Json.Read<Download>(Json.Write(exact));check(roundtrip.ExactMediaQuality&&roundtrip.MediaFormatId=="137"&&roundtrip.MediaHeight==1080&&roundtrip.MediaPixelHeight==804,"nominal quality and actual cropped dimensions survive persistence");
            string dir=Path.Combine(root,"media-checks");Directory.CreateDirectory(dir);
            string video=Path.Combine(dir,"input-video.mp4"),audio=Path.Combine(dir,"input-audio.m4a");
            await MediaTransfer.Execute(MediaTransfer.RequireTool("ffmpeg.exe"),new[]{"-hide_banner","-loglevel","error","-nostdin","-f","lavfi","-i","testsrc2=size=320x180:rate=25","-t","2","-an","-c:v","libx264",video},20,CancellationToken.None);
            await MediaTransfer.Execute(MediaTransfer.RequireTool("ffmpeg.exe"),new[]{"-hide_banner","-loglevel","error","-nostdin","-f","lavfi","-i","sine=frequency=440:sample_rate=48000","-t","2","-vn","-c:a","aac",audio},20,CancellationToken.None);
            using(var server=new MediaFixture(new Dictionary<string,byte[]>{{"/video",File.ReadAllBytes(video)},{"/audio",File.ReadAllBytes(audio)},{"/invalid",Encoding.UTF8.GetBytes("not a valid audio file")}}))
            using(var m=new Manager(Path.Combine(dir,"state"),false)) {
                m.State.Settings.Retries=0;
                Func<string,Download> create=route=>{
                    var j=m.Add("https://www.youtube.com/watch?v=Q3TI27IN7X0",dir,"merged.mp4","Main queue",true,null,"",null);j.SourceUrl=j.Url;j.MediaHeight=180;j.MediaPixelHeight=180;j.ExactMediaQuality=true;
                    j.Video=new Download{Url=server.Base+"/video",FileName="video.mp4",Folder=dir,Connections=2};
                    j.Audio=new Download{Url=server.Base+route,FileName="audio.mp4",Folder=dir,Connections=2};return j;
                };
                var good=create("/audio");await m.Start(good);
                check(good.Status=="Complete"&&File.Exists(good.Target),"paired streams download through UDM and publish a merged MP4");
                string json=await MediaTransfer.Execute(MediaTransfer.RequireTool("ffprobe.exe"),new[]{"-v","error","-show_entries","stream=codec_type,width,height","-of","json",good.Target},10,CancellationToken.None);
                check(json.Contains("\"video\"")&&json.Contains("\"audio\"")&&json.Contains("320"),"merged output contains the expected video and audio tracks");
                check(good.TransferredBytes==new FileInfo(video).Length+new FileInfo(audio).Length&&good.TransferSeconds>0&&good.MergeSeconds>0&&good.ElapsedSeconds>=good.TransferSeconds+good.MergeSeconds,"measurements count both media streams and separate download from merge time");
                check(!File.Exists(good.Video.Target)&&!File.Exists(good.Audio.Target),"successful merge cleans only its own intermediate streams");
                check(good.ResolveSeconds==0,"browser-captured streams complete with no URL resolver stage");
                var wrong=create("/audio");wrong.MediaPixelHeight=1080;await m.Start(wrong);check(wrong.Status=="Failed"&&!File.Exists(wrong.Target)&&File.Exists(wrong.Video.Target),"mismatched selected dimensions preserve inputs and never publish a lower-quality file");
                var missing=create("/audio");missing.Video=null;missing.Audio=null;await m.Start(missing);check(missing.Status=="Failed"&&missing.Error.Contains("browser-captured")&&!File.Exists(missing.Target),"missing browser links fail clearly without launching a resolver");
                var bad=create("/invalid");await m.Start(bad);check(bad.Status=="Failed"&&!File.Exists(bad.Target)&&File.Exists(bad.Video.Target),"failed merge preserves stream inputs without publishing a broken MP4");
                var denied=create("/audio");denied.Video.Url=server.Base+"/forbidden";denied.Audio=null;await m.Start(denied);check(denied.Status=="Failed"&&denied.Error.Contains("HTTP 403")&&denied.Received==0&&denied.ResolveSeconds==0&&!File.Exists(denied.Target),"rejected captured media preserves HTTP status without resolving or publishing output");
                var collision=create("/audio");File.WriteAllText(collision.Target,"KEEP");await m.Start(collision);check(collision.Status=="Failed"&&File.ReadAllText(collision.Target)=="KEEP","media publication never overwrites an existing user file");
                m.Save();using(var restored=new Manager(m.DataRoot,false))check(restored.State.Downloads.First(j=>j.Id==good.Id).TransferredBytes==good.TransferredBytes,"media measurements and nested streams survive state reload");
            }
            bool rejected=false;try{MediaTransfer.ValidateStream("https://googlevideo.com.attacker.invalid/videoplayback");}catch(ArgumentException){rejected=true;}check(rejected,"browser stream URLs reject lookalike hosts");
        }
    }
    public sealed class MediaFixture:IDisposable {
        readonly TcpListener listener=new TcpListener(IPAddress.Loopback,0);
        readonly Dictionary<string,byte[]> files;
        public string Base;
        bool stopped;
        public MediaFixture(Dictionary<string,byte[]> data){files=data;listener.Start();Base="http://127.0.0.1:"+((IPEndPoint)listener.LocalEndpoint).Port;Task.Run((Func<Task>)Loop);}
        async Task Loop(){while(!stopped){try{var client=await listener.AcceptTcpClientAsync();Task ignored=Serve(client);}catch(ObjectDisposedException){return;}catch(SocketException){return;}}}
        async Task Serve(TcpClient client){try{using(client)using(var stream=client.GetStream())using(var reader=new StreamReader(stream,Encoding.ASCII,false,4096,true)){
            string first=await reader.ReadLineAsync();if(first==null)return;string route=first.Split(' ')[1];string line,range=null;
            while(!string.IsNullOrEmpty(line=await reader.ReadLineAsync()))if(line.StartsWith("Range:",StringComparison.OrdinalIgnoreCase))range=line.Substring(6).Trim();
            if(route=="/forbidden"){byte[] denied=Encoding.ASCII.GetBytes("HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");await stream.WriteAsync(denied,0,denied.Length);return;}byte[] bytes=files[route];
            int start=0,end=bytes.Length-1;if(range!=null){var parts=range.Substring(6).Split('-');start=int.Parse(parts[0]);end=int.Parse(parts[1]);}
            string headers="HTTP/1.1 "+(range==null?"200 OK":"206 Partial Content")+"\r\nETag: \"fixture\"\r\nContent-Length: "+(end-start+1)+"\r\nConnection: close\r\n";
            if(range!=null)headers+="Content-Range: bytes "+start+"-"+end+"/"+bytes.Length+"\r\n";
            byte[] h=Encoding.ASCII.GetBytes(headers+"\r\n");await stream.WriteAsync(h,0,h.Length);await stream.WriteAsync(bytes,start,end-start+1);
        }}catch(IOException){}catch(SocketException){}}
        public void Dispose(){stopped=true;listener.Stop();}
    }
}
