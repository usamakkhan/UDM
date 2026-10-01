using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Security.Cryptography;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace Udm {
    public sealed class Fixture : IDisposable {
        readonly TcpListener listener=new TcpListener(IPAddress.Loopback,0);
        readonly CancellationTokenSource stop=new CancellationTokenSource();
        public readonly byte[] Data=new byte[4*1024*1024+123];
        public readonly ConcurrentBag<string> Requests=new ConcurrentBag<string>();
        public bool Changed;
        int drops;
        public string Base;
        public Fixture(){new Random(7281).NextBytes(Data);listener.Start();Base="http://127.0.0.1:"+((IPEndPoint)listener.LocalEndpoint).Port;Task.Run((Func<Task>)Accept);}
        async Task Accept(){while(!stop.IsCancellationRequested){try{var c=await listener.AcceptTcpClientAsync();Task.Run(()=>Serve(c));}catch(ObjectDisposedException){return;}catch(SocketException){return;}}}
        async Task Serve(TcpClient client){using(client)try{
            using(var stream=client.GetStream())using(var reader=new StreamReader(stream,Encoding.ASCII,false,4096,true)){
                string first=await reader.ReadLineAsync();if(first==null)return;string path=first.Split(' ')[1];var headers=new Dictionary<string,string>(StringComparer.OrdinalIgnoreCase);string line;while(!string.IsNullOrEmpty(line=await reader.ReadLineAsync())){int colon=line.IndexOf(':');if(colon>0)headers[line.Substring(0,colon)]=line.Substring(colon+1).Trim();}
                string range=headers.ContainsKey("Range")?headers["Range"]:"";Requests.Add(path+" "+range);
                if(path=="/redirect"){await Header(stream,"302 Found","Location: /range\r\nContent-Length: 0\r\n");return;}
                if(path=="/missing"){await Header(stream,"404 Not Found","Content-Length: 0\r\n");return;}
                if(path=="/forbidden"){await Header(stream,"403 Forbidden","Content-Length: 0\r\n");return;}
                if(path=="/empty"){await Header(stream,"416 Range Not Satisfiable","Content-Range: bytes */0\r\nContent-Length: 0\r\n");return;}
                if(path=="/auth"&&(!headers.ContainsKey("Authorization")||headers["Authorization"]!="Basic dXNlcjpwYXNz")){await Header(stream,"401 Unauthorized","Content-Length: 0\r\n");return;}
                if(path=="/page"){byte[] html=Encoding.UTF8.GetBytes("<a href='/asset.zip'>ZIP</a><a href='/asset.zip'>duplicate</a><a href='https://outside.invalid/no.zip'>external</a><a href='/second'>next</a>");await Header(stream,"200 OK","Content-Type: text/html\r\nContent-Length: "+html.Length+"\r\n");await stream.WriteAsync(html,0,html.Length);return;}
                if(path=="/second"){byte[] html=Encoding.UTF8.GetBytes("<img src='/photo.png'><a href='/page'>loop</a>");await Header(stream,"200 OK","Content-Type: text/html\r\nContent-Length: "+html.Length+"\r\n");await stream.WriteAsync(html,0,html.Length);return;}
                int start=0,end=Data.Length-1;bool partial=range.Length>0&&path!="/norange"&&path!="/unknown";
                if(partial){var bits=range.Substring(6).Split('-');start=int.Parse(bits[0]);end=bits[1].Length>0?int.Parse(bits[1]):end;}
                if(path=="/change"&&range!="bytes=0-0"&&!Changed){Changed=true;partial=false;start=0;end=Data.Length-1;}
                byte[] body=Data;if(path=="/change"&&Changed){body=(byte[])Data.Clone();body[0]^=0x5a;}
                if(path=="/bad"&&range!="bytes=0-0"&&partial)start=Math.Max(0,start-1)+1;
                string extra=path=="/novalidator"?"":"ETag: \""+(path=="/change"&&Changed?"v2":"v1")+"\"\r\n";
                if(partial)extra+="Content-Range: bytes "+start+"-"+end+"/"+body.Length+"\r\n";
                if(path!="/unknown")extra+="Content-Length: "+(end-start+1)+"\r\n";
                await Header(stream,partial?"206 Partial Content":"200 OK",extra);
                int length=end-start+1;if(path=="/drop"&&range!="bytes=0-0"&&Interlocked.Increment(ref drops)==1)length/=2;
                for(int pos=start;pos<start+length;pos+=16384){int n=Math.Min(16384,start+length-pos);await stream.WriteAsync(body,pos,n);if(path=="/slow")await Task.Delay(12);}
            }
        }catch(IOException){}catch(SocketException){}catch(ObjectDisposedException){}}
        static Task Header(Stream stream,string status,string headers){byte[] b=Encoding.ASCII.GetBytes("HTTP/1.1 "+status+"\r\n"+headers+"Connection: close\r\n\r\n");return stream.WriteAsync(b,0,b.Length);}
        public void Dispose(){stop.Cancel();listener.Stop();}
    }
    public static class Tests {
        static int count;
        static string root;
        static void Assert(bool condition,string description){if(!condition)throw new Exception("FAIL: "+description);count++;Console.WriteLine("PASS "+description);}
        static string Hash(byte[] bytes){using(var h=SHA256.Create())return BitConverter.ToString(h.ComputeHash(bytes)).Replace("-","").ToLowerInvariant();}
        public static int Main(){root=Path.Combine(Path.GetTempPath(),"udm-tests-"+Guid.NewGuid().ToString("N"));try{Run().GetAwaiter().GetResult();Console.WriteLine("ALL "+count+" CHECKS PASSED. Test artifacts: "+root);return 0;}catch(Exception ex){Console.Error.WriteLine(ex);Console.Error.WriteLine("Test artifacts: "+root);return 1;}}
        static async Task Finish(Task task){if(await Task.WhenAny(task,Task.Delay(45000))!=task)throw new Exception("Transfer test timed out.");await task;}
        static async Task Run(){
            ServicePointManager.DefaultConnectionLimit=64;
            await MediaTests.Run(root,Assert);WorkflowTests.Run(root,Assert);NetworkMonitorTests.Run(Assert);await SabrTests.Run(root,Assert);
            Assert(Names.Safe("../../CON.exe")=="_CON.exe","Windows traversal and reserved filenames sanitized");
            Assert(Names.Safe("bad:name.zip")=="bad_name.zip","alternate-stream filenames sanitized");
            Assert(Names.Expand("https://example.test/file[001-003].zip").Last().EndsWith("003.zip"),"zero-padded batch expansion");
            bool rejected=false;try{Names.Url("file:///C:/secret.txt");}catch(ArgumentException){rejected=true;}Assert(rejected,"local-file URLs rejected");
            var q=new QueueRule{Scheduled=true,StartMinute=22*60,StopMinute=2*60,Days=1<<(int)DayOfWeek.Friday};
            Assert(q.InWindow(new DateTime(2026,9,19,1,0,0))&&!q.InWindow(new DateTime(2026,9,19,3,0,0)),"overnight scheduler honors the starting weekday");
            string protectedText=Secrets.Protect("session-secret");Assert(!protectedText.Contains("session-secret")&&Secrets.Reveal(protectedText)=="session-secret","credentials round-trip through Windows DPAPI");
            using(var fixture=new Fixture()){
                string downloads=Path.Combine(root,"downloads");
                using(var m=new Manager(Path.Combine(root,"main"),false)){
                    m.State.Settings.Retries=1;
                    var range=m.Add(fixture.Base+"/range",downloads,"range.bin","Main queue",false,null,Hash(fixture.Data),null);await Finish(m.Start(range));
                    Assert(range.Status=="Complete"&&File.ReadAllBytes(range.Target).SequenceEqual(fixture.Data),"parallel byte ranges assemble into an exact file");
                    Assert(range.RangeSupported&&range.Segments.Count>1&&fixture.Requests.Count(r=>r.StartsWith("/range "))>2,"parallel transfer uses multiple validated requests");
                    Assert(range.Workers.Count<=range.Connections&&range.Workers.Sum(w=>w.Downloaded)==fixture.Data.Length&&range.Workers.All(w=>w.State=="Finished"),"progress rows measure real workers and account for transferred bytes");
                    foreach(string route in new[]{"norange","unknown","redirect","novalidator","drop"}){
                        var j=m.Add(fixture.Base+"/"+route,downloads,route+".bin","Main queue",false,null,"",null);await Finish(m.Start(j));Assert(j.Status=="Complete"&&File.ReadAllBytes(j.Target).SequenceEqual(fixture.Data),route+" transfer preserves exact content");
                        if(route=="novalidator")Assert(!j.RangeSupported,"missing validators disable unsafe ranged resume");
                    }
                    var empty=m.Add(fixture.Base+"/empty",downloads,"empty.bin","Main queue",false,null,"",null);await Finish(m.Start(empty));Assert(empty.Status=="Complete"&&new FileInfo(empty.Target).Length==0,"empty resources supported");
                    var changed=m.Add(fixture.Base+"/change",downloads,"changed.bin","Main queue",false,null,"",null);await Finish(m.Start(changed));byte[] expected=(byte[])fixture.Data.Clone();expected[0]^=0x5a;Assert(changed.Status=="Complete"&&File.ReadAllBytes(changed.Target).SequenceEqual(expected),"changing ETag restarts the transfer without mixing versions");
                    var bad=m.Add(fixture.Base+"/bad",downloads,"bad.bin","Main queue",false,null,"",null);await Finish(m.Start(bad));Assert(bad.Status=="Failed"&&!File.Exists(bad.Target),"invalid Content-Range never publishes a corrupted file");
                    var mismatch=m.Add(fixture.Base+"/range",downloads,"wronghash.bin","Main queue",false,null,new string('0',64),null);await Finish(m.Start(mismatch));Assert(mismatch.Status=="Failed"&&!File.Exists(mismatch.Target),"SHA-256 mismatch prevents publication");
                    var missing=m.Add(fixture.Base+"/missing",downloads,"missing.bin","Main queue",false,null,"",null);await Finish(m.Start(missing));Assert(missing.Status=="Failed"&&!File.Exists(missing.Target),"HTTP failures remain actionable failed records");
                    var forbidden=m.Add(fixture.Base+"/forbidden",downloads,"forbidden.bin","Main queue",true,null,"",null);bool classified=false;
                    try{await new Transfer(m,forbidden).Run(CancellationToken.None);}catch(HttpStatusFailure ex){classified=ex.StatusCode==403;}
                    Assert(classified&&!File.Exists(forbidden.Target),"rejected playback URLs preserve the HTTP status for bounded media refresh");
                    var exists=m.Add(fixture.Base+"/range",downloads,"existing.bin","Main queue",false,null,"",null);File.WriteAllText(exists.Target,"KEEP");await Finish(m.Start(exists));Assert(exists.Status=="Failed"&&File.ReadAllText(exists.Target)=="KEEP","existing destination is never overwritten");
                    var auth=m.Add(fixture.Base+"/auth",downloads,"auth.bin","Main queue",false,new Dictionary<string,string>{{"Authorization","Basic dXNlcjpwYXNz"}},"",null);await Finish(m.Start(auth));Assert(auth.Status=="Complete","HTTP authentication is applied to all requests");m.Save();Assert(!File.ReadAllText(Path.Combine(m.DataRoot,"state.json")).Contains("dXNlcjpwYXNz"),"credentials are not persisted in plaintext");
                    var progress=new Progress<string>();var links=await Grabber.Explore(fixture.Base+"/page",1,5,"zip png",progress,CancellationToken.None);Assert(links.Count==2&&links.All(l=>l.Url.StartsWith(fixture.Base)),"grabber deduplicates links, follows depth, and stays on origin");
                    var record=m.Add(fixture.Base+"/range",downloads,"remove.bin","Main queue",true,null,"",null);File.WriteAllText(record.Target,"KEEP");m.Remove(record);Assert(File.ReadAllText(record.Target)=="KEEP","removing records preserves completed user files");
                }
                string pauseRoot=Path.Combine(root,"resume"),id;
                using(var m=new Manager(pauseRoot,false)){
                    var j=m.Add(fixture.Base+"/slow",downloads,"resume.bin","Main queue",false,null,"",null);id=j.Id;Task running=m.Start(j);for(int i=0;i<200&&j.Received<200000;i++)await Task.Delay(10);m.Pause(j);await Finish(running);Assert(j.Status=="Paused"&&j.Received>0&&j.Received<fixture.Data.Length,"pause cancels connections while retaining partial data");m.Save();
                }
                using(var m=new Manager(pauseRoot,false)){
                    var j=m.State.Downloads.Single(d=>d.Id==id);long before=j.Received;Assert(before>0&&j.Status=="Paused","partial state survives application restart");await Finish(m.Start(j));Assert(j.Status=="Complete"&&File.ReadAllBytes(j.Target).SequenceEqual(fixture.Data),"resume after restart assembles exact content");Assert(fixture.Requests.Any(r=>r.StartsWith("/slow bytes=")&&!r.EndsWith("0-0")&&!r.Contains("bytes=0-")&&!r.Contains("bytes=1048576-")&&!r.Contains("bytes=2097152-")&&!r.Contains("bytes=3145728-")&&!r.Contains("bytes=4194304-")),"resume requests an already-partial piece from its saved offset");
                }
                using(var m=new Manager(Path.Combine(root,"schedule"),false)){
                    m.State.Queues[0].Enabled=false;var j=m.Add(fixture.Base+"/range",downloads,"scheduled.bin","Main queue",false,null,"",null);m.Tick();Assert(j.Status=="Queued","disabled queue does not start work");m.State.Queues[0].Enabled=true;j.NotBefore=DateTime.UtcNow.AddHours(1);m.Tick();Assert(j.Status=="Queued","future start time is respected");j.NotBefore=null;m.Tick();await Finish(m.Start(j));Assert(j.Status=="Complete","queue starts eligible work");
                }
                using(var m=new Manager(Path.Combine(root,"pipe"),false))using(var server=new PipeServer(m)){
                    m.AddCategory("Courses");bool duplicateCategory=false;try{m.AddCategory("courses");}catch(ArgumentException){duplicateCategory=true;}using(var savedCategories=new Manager(m.DataRoot,false))Assert(savedCategories.Categories.Contains("Courses")&&duplicateCategory,"custom categories survive restart and reject case-insensitive duplicates");
                    var reply=await Wire.Send(new AddMessage{action="add",url=fixture.Base+"/range",filename="pipe.bin"},5000);Assert(reply.ok&&m.State.Downloads.Any(d=>d.Id==reply.id),"native messaging pipe acknowledges durable download records");var invalid=await Wire.Send(new AddMessage{action="add",url="file:///secret.txt"},5000);Assert(!invalid.ok,"native messaging rejects unsupported URLs");
                    var missingMedia=await Wire.Send(new AddMessage{action="media",url="https://www.youtube.com/watch?v=Q3TI27IN7X0",height=1080},5000);Assert(!missingMedia.ok,"page-only media handoffs require browser capture instead of a resolver");
                    var media=new AddMessage{action="media",url="https://www.youtube.com/watch?v=Q3TI27IN7X0",filename="A title feat. Artist",height=1080,videoUrl="https://rr1.googlevideo.com/videoplayback?itag=137",audioUrl="https://rr1.googlevideo.com/videoplayback?itag=140"};
                    var first=await Wire.Send(media,5000);var duplicate=await Wire.Send(media,5000);
                    Assert(first.ok&&duplicate.ok&&first.id==duplicate.id,"repeated browser media handoffs reuse the queued download");
                    media.height=720;var otherQuality=await Wire.Send(media,5000);
                    Assert(otherQuality.ok&&otherQuality.id!=first.id,"different requested video qualities create separate downloads");
                    var direct=await Wire.Send(new AddMessage{action="media",url=media.url,filename="Direct capture",height=1080,exactQuality=true,formatId="137",videoUrl="https://rr1.googlevideo.com/videoplayback?itag=137",audioUrl="https://rr1.googlevideo.com/videoplayback?itag=140"},5000);
                    var directJob=m.State.Downloads.FirstOrDefault(d=>d.Id==direct.id);
                    Assert(direct.ok&&direct.id!=first.id&&directJob.Video!=null&&directJob.Audio!=null&&directJob.ExactMediaQuality&&directJob.MediaFormatId=="137","native browser handoff stores matched direct streams and exact quality without resolving");
                    var progressive=await Wire.Send(new AddMessage{action="media",url=media.url,filename="Progressive capture",height=360,exactQuality=true,formatId="18",videoUrl="https://rr1.googlevideo.com/videoplayback?itag=18",audioUrl=""},5000);
                    var progressiveJob=m.State.Downloads.FirstOrDefault(d=>d.Id==progressive.id);
                    Assert(progressive.ok&&progressiveJob.Video!=null&&progressiveJob.Audio==null,"native handoff preserves progressive video audio without inventing a second stream");
                    var page=await Wire.Send(new AddMessage{action="add",url=media.url,filename="page.html"},5000);
                    Assert(page.ok&&page.id!=first.id&&page.id!=otherQuality.id,"plain URL downloads do not reuse queued media jobs");
                    Download offered=null;server.DownloadReceived+=j=>offered=j;
                    var offer=await Wire.Send(new AddMessage{action="add",url=fixture.Base+"/slow",filename="confirm.bin"},5000);
                    Assert(offer.ok&&offered!=null&&offered.Id==offer.id&&offered.Status=="Awaiting confirmation","browser handoff is saved without downloading until file info is accepted");
                    var repeat=await Wire.Send(new AddMessage{action="add",url=fixture.Base+"/slow",filename="confirm.bin"},5000);
                    Assert(repeat.id==offer.id,"repeated browser clicks reuse an open file-info request");
                    string custom=Path.Combine(downloads,"chosen.mp4");m.ConfigureDestination(offered,custom,"Video","Description","Main queue",true);
                    var remembered=m.Add(fixture.Base+"/range",null,"next.mp4","Main queue",true,null,"",null);
                    Assert(offered.Target==custom&&offered.Description=="Description"&&remembered.Folder==downloads,"file-info destination and remembered category folder are applied");
                    m.SetLimit(offered,123,true);m.SetLimit(offered,456,false);Assert(offered.EffectiveLimitKbps==456&&offered.LimitKbps==123,"temporary speed limit does not overwrite the remembered limit");
                    m.Resume(offered);Assert(offered.EffectiveLimitKbps==123&&offered.Status=="Queued","resume restores the remembered speed limit");m.Pause(offered);
                    File.WriteAllText(custom,"KEEP");bool collisionRejected=false;try{m.ConfigureDestination(offered,custom,"Video","","Main queue",false);}catch(IOException){collisionRejected=true;}
                    Assert(collisionRejected&&File.ReadAllText(custom)=="KEEP","file info refuses an existing destination without overwriting it");
                }
            }
        }
    }
}
