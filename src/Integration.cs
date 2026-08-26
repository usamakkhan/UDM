using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.IO.Pipes;
using System.Linq;
using System.Net.Http;
using System.Runtime.Serialization;
using System.Security.AccessControl;
using System.Security.Principal;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Threading.Tasks;
using System.Web.Script.Serialization;

namespace Udm {
    [DataContract] public class AddMessage {
        [DataMember] public string action;
        [DataMember] public string url;
        [DataMember] public string filename;
        [DataMember] public string referrer;
        [DataMember] public string cookies;
        [DataMember] public string userAgent;
        [DataMember] public int height;
        [DataMember] public int pixelHeight;
        [DataMember] public bool exactQuality;
        [DataMember] public string formatId;
        [DataMember] public string videoUrl;
        [DataMember] public string audioUrl;
        [DataMember] public SabrOffer sabr;
    }
    [DataContract] public class Reply {
        [DataMember] public bool ok;
        [DataMember] public string id;
        [DataMember] public string error;
        [DataMember(EmitDefaultValue=false)] public string[] extensions;
        [DataMember(EmitDefaultValue=false)] public string[] excluded;
    }
    public static class Wire {
        public static string PipeName { get { return "udm-" + WindowsIdentity.GetCurrent().User.Value.Replace('-', '_'); } }
        public static async Task<byte[]> ReadExact(Stream stream, int count, CancellationToken ct) {
            byte[] bytes = new byte[count]; int offset=0;
            while (offset < count) { int n = await stream.ReadAsync(bytes, offset, count-offset, ct).ConfigureAwait(false); if (n == 0) throw new EndOfStreamException(); offset += n; }
            return bytes;
        }
        public static async Task<string> Read(Stream stream, CancellationToken ct) {
            byte[] size = await ReadExact(stream, 4, ct).ConfigureAwait(false);
            int length = BitConverter.ToInt32(size, 0);
            if (length < 1 || length > 256 * 1024) throw new IOException("Message exceeds the 256 KB limit.");
            return Encoding.UTF8.GetString(await ReadExact(stream, length, ct).ConfigureAwait(false));
        }
        public static async Task Write(Stream stream, string message, CancellationToken ct) {
            byte[] bytes = Encoding.UTF8.GetBytes(message), size = BitConverter.GetBytes(bytes.Length);
            await stream.WriteAsync(size,0,4,ct).ConfigureAwait(false); await stream.WriteAsync(bytes,0,bytes.Length,ct).ConfigureAwait(false); await stream.FlushAsync(ct).ConfigureAwait(false);
        }
        public static async Task<Reply> Send(AddMessage message, int timeout) {
            using (var pipe = new NamedPipeClientStream(".", PipeName, PipeDirection.InOut, PipeOptions.Asynchronous))
            using (var cts = new CancellationTokenSource(timeout)) {
                await Task.Run(() => pipe.Connect(timeout)).ConfigureAwait(false);
                await Write(pipe,Json.Write(message),cts.Token).ConfigureAwait(false);
                return Json.Read<Reply>(await Read(pipe,cts.Token).ConfigureAwait(false));
            }
        }
    }
    public sealed class PipeServer : IDisposable {
        readonly Manager manager;
        readonly CancellationTokenSource stop = new CancellationTokenSource();
        NamedPipeServerStream current;
        readonly object sync = new object();
        public event Action ShowRequested;
        public event Action<Download> DownloadReceived;
        public PipeServer(Manager m) { manager=m; Task.Run((Func<Task>)Listen); }
        async Task Listen() {
            while (!stop.IsCancellationRequested) {
                var security = new PipeSecurity();
                security.SetAccessRuleProtection(true, false);
                security.AddAccessRule(new PipeAccessRule(WindowsIdentity.GetCurrent().User, PipeAccessRights.FullControl, AccessControlType.Allow));
                using (var pipe = new NamedPipeServerStream(Wire.PipeName, PipeDirection.InOut, 1, PipeTransmissionMode.Byte, PipeOptions.Asynchronous,4096,4096,security)) {
                    lock(sync) { if (stop.IsCancellationRequested) return; current=pipe; }
                    try {
                        await Task.Factory.FromAsync(pipe.BeginWaitForConnection, pipe.EndWaitForConnection, null).ConfigureAwait(false);
                        using (var timeout = CancellationTokenSource.CreateLinkedTokenSource(stop.Token)) {
                            timeout.CancelAfter(10000);
                            Reply reply;
                            try {
                                var message = Json.Read<AddMessage>(await Wire.Read(pipe, timeout.Token).ConfigureAwait(false));
                                if (message.action == "ping") reply=new Reply {ok=true};
                                else if(message.action=="preferences"){lock(manager.Sync)reply=new Reply{ok=true,extensions=Manager.Words(manager.State.Settings.CaptureExtensions),excluded=Manager.Words(manager.State.Settings.CaptureExcludedHosts)};}
                                else if (message.action == "show") { var h=ShowRequested; if(h!=null) h(); reply=new Reply {ok=true}; }
                                else if (message.action == "add" || message.action == "media" || message.action == "sabr") {
                                    var headers=new Dictionary<string,string>();
                                    if (!string.IsNullOrEmpty(message.referrer)) headers["Referer"]=message.referrer;
                                    if (!string.IsNullOrEmpty(message.cookies)) headers["Cookie"]=message.cookies;
                                    if (!string.IsNullOrEmpty(message.userAgent)) headers["User-Agent"]=message.userAgent;
                                    Download job;bool created=false;
                                    lock(manager.Sync) {
                                        job=manager.State.Downloads.FirstOrDefault(d=>d.Url==message.url &&
                                            (message.action!="add" ? !string.IsNullOrEmpty(d.SourceUrl) && d.MediaHeight==message.height && d.ExactMediaQuality==message.exactQuality && (d.MediaFormatId??"")== (message.formatId??"") : string.IsNullOrEmpty(d.SourceUrl)) &&
                                            (d.Status=="Queued" || d.Status=="Awaiting confirmation" || manager.IsActive(d)));
                                        if(job==null) {
                                            job=message.action=="sabr"?manager.AddSabr(message.url,message.filename,message.height,message.pixelHeight,message.sabr,headers):message.action == "media" ? manager.AddMedia(message.url,message.filename,message.height,message.videoUrl,message.audioUrl,headers,message.exactQuality,message.formatId,message.pixelHeight) : manager.Add(message.url,null,message.filename,"Main queue",false,headers,"",null);
                                            created=true;
                                            if(DownloadReceived!=null&&!manager.State.Settings.SkipBrowserFileInfo){job.Status="Awaiting confirmation";manager.Save();}
                                        }
                                    }
                                    reply=new Reply {ok=true,id=job.Id};
                                    if(created){var received=DownloadReceived;if(received!=null)received(job);}
                                } else throw new ArgumentException("Unknown UDM command.");
                            } catch(Exception ex) { reply=new Reply {ok=false,error=ex.Message}; }
                            await Wire.Write(pipe,Json.Write(reply),timeout.Token).ConfigureAwait(false);
                        }
                    } catch(Exception) { if(stop.IsCancellationRequested) return; }
                    finally { lock(sync) current=null; }
                }
            }
        }
        public void Dispose() { stop.Cancel(); lock(sync) if(current!=null) current.Dispose(); }
    }
    public static class NativeHost {
        public static int Main() {
            try { Run().GetAwaiter().GetResult(); return 0; }
            catch (Exception) { return 1; }
        }
        static async Task Run() {
            using(var input=Console.OpenStandardInput()) using(var output=Console.OpenStandardOutput()) {
                string text=await Wire.Read(input,CancellationToken.None).ConfigureAwait(false);
                Reply reply;
                try {
                    // Parse the same plain object protocol on Chromium and Firefox.
                    var message=Json.Read<AddMessage>(text);
                    reply=null;
                    try { reply=await Wire.Send(message,1000).ConfigureAwait(false); }
                    catch(TimeoutException) {}
                    if(reply==null) {
                        string exe=Path.Combine(AppDomain.CurrentDomain.BaseDirectory,"UDM.exe");
                        Process.Start(new ProcessStartInfo(exe,"--background") {UseShellExecute=false,CreateNoWindow=true});
                        reply=null;
                        for(int i=0;i<10;i++) { await Task.Delay(300).ConfigureAwait(false); try { reply=await Wire.Send(message,1000).ConfigureAwait(false); break; } catch(TimeoutException) {} }
                        if(reply==null) throw new IOException("UDM did not become ready.");
                    }
                } catch(Exception ex) {reply=new Reply{ok=false,error=ex.Message};}
                await Wire.Write(output,Json.Write(reply),CancellationToken.None).ConfigureAwait(false);
            }
        }
    }
    public sealed class GrabResult { public string Url; public string Type; }
    public static class Grabber {
        public static async Task<List<GrabResult>> Explore(string start, int depth, int maxPages, string extensions, IProgress<string> progress, CancellationToken ct) {
            Uri root=Names.Url(start); if(root.Scheme=="ftp") throw new ArgumentException("Site exploration requires HTTP or HTTPS.");
            var found=new Dictionary<string,GrabResult>(); var seen=new HashSet<string>(); var pending=new Queue<Tuple<Uri,int>>(); pending.Enqueue(Tuple.Create(root,0));
            var filters=new HashSet<string>(Regex.Split(extensions.ToLowerInvariant(),@"[ ,;]+"),StringComparer.OrdinalIgnoreCase);
            using(var handler=new HttpClientHandler {AllowAutoRedirect=false}) using(var client=new HttpClient(handler) {Timeout=TimeSpan.FromSeconds(20)}) {
                client.DefaultRequestHeaders.UserAgent.ParseAdd("UDM/0.6.1");
                while(pending.Count>0 && seen.Count<maxPages) {
                    ct.ThrowIfCancellationRequested(); var item=pending.Dequeue(); if(!seen.Add(item.Item1.AbsoluteUri)) continue;
                    progress.Report("Exploring page " + seen.Count + " / " + maxPages + "  •  " + item.Item1.Host);
                    string html;
                    try {
                        using(var r=await client.GetAsync(item.Item1,HttpCompletionOption.ResponseHeadersRead,ct).ConfigureAwait(false)) {
                            if((int)r.StatusCode>=300 && (int)r.StatusCode<400) {
                                if(r.Headers.Location!=null) {Uri next=new Uri(item.Item1,r.Headers.Location); if(next.GetLeftPart(UriPartial.Authority)==root.GetLeftPart(UriPartial.Authority)) pending.Enqueue(Tuple.Create(next,item.Item2));}
                                continue;
                            }
                            r.EnsureSuccessStatusCode();
                            if(r.Content.Headers.ContentType!=null && !r.Content.Headers.ContentType.MediaType.Contains("html")) { if(Matches(item.Item1,filters)) found[item.Item1.AbsoluteUri]=new GrabResult{Url=item.Item1.AbsoluteUri,Type=Names.Category(item.Item1.LocalPath)}; continue; }
                            using(var stream=await r.Content.ReadAsStreamAsync().ConfigureAwait(false)) using(var ms=new MemoryStream()) {
                                byte[] b=new byte[16384]; int n; while((n=await stream.ReadAsync(b,0,b.Length,ct).ConfigureAwait(false))>0) {if(ms.Length+n>2*1024*1024) throw new IOException("HTML page exceeds 2 MB."); ms.Write(b,0,n);} html=Encoding.UTF8.GetString(ms.ToArray());
                            }
                        }
                    } catch(OperationCanceledException) {ct.ThrowIfCancellationRequested(); continue;} catch(HttpRequestException) {continue;} catch(IOException) {continue;}
                    foreach(Match m in Regex.Matches(html,"(?:href|src)\\s*=\\s*[\"']([^\"']+)[\"']",RegexOptions.IgnoreCase)) {
                        Uri link; if(!Uri.TryCreate(item.Item1,System.Net.WebUtility.HtmlDecode(m.Groups[1].Value),out link) || (link.Scheme!="http" && link.Scheme!="https")) continue;
                        var builder=new UriBuilder(link) {Fragment=""}; link=builder.Uri;
                        if(link.GetLeftPart(UriPartial.Authority)!=root.GetLeftPart(UriPartial.Authority)) continue;
                        string ext=Path.GetExtension(link.AbsolutePath).ToLowerInvariant();
                        bool page=ext=="" || new[]{".html",".htm",".php",".asp",".aspx"}.Contains(ext);
                        if(page && item.Item2<depth && !seen.Contains(link.AbsoluteUri)) pending.Enqueue(Tuple.Create(link,item.Item2+1));
                        if(!page && Matches(link,filters)) found[link.AbsoluteUri]=new GrabResult{Url=link.AbsoluteUri,Type=Names.Category(link.LocalPath)};
                        if(found.Count>=2000) return found.Values.ToList();
                    }
                    await Task.Delay(200,ct).ConfigureAwait(false);
                }
            }
            return found.Values.ToList();
        }
        static bool Matches(Uri uri,HashSet<string> filters) {return filters.Contains("*") || filters.Contains(Path.GetExtension(uri.AbsolutePath).TrimStart('.'));}
    }
}
