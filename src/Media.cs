using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Web.Script.Serialization;

namespace Udm {
    // Media URLs come from UDM browser capture. All bytes go through Transfer; no external URL resolver.
    public sealed class MediaTransfer {
        readonly Manager owner;
        readonly Download job;
        public MediaTransfer(Manager manager, Download download) { owner=manager; job=download; }
        public static string RequireTool(string name) {
            string path=Path.Combine(AppDomain.CurrentDomain.BaseDirectory,"tools",name);
            if (!File.Exists(path)) throw new IOException("Media helper missing: "+name+". Run setup-media.ps1 in the UDM folder.");
            return path;
        }
        public static void ValidateSource(string value) {
            var u=Names.Url(value);
            if(u.Scheme!="https" || !(u.Host=="youtube.com" || u.Host.EndsWith(".youtube.com",StringComparison.OrdinalIgnoreCase) || u.Host=="youtu.be")) throw new ArgumentException("Media page downloads currently support HTTPS YouTube links.");
        }
        public static void ValidateStream(string value) {
            var u=Names.Url(value);
            if(u.Scheme!="https" || !u.Host.EndsWith(".googlevideo.com",StringComparison.OrdinalIgnoreCase) || u.AbsolutePath!="/videoplayback") throw new ArgumentException("Expected a YouTube playback stream.");
        }
        static Download Child(Manager owner, Download job, string url, string kind, int connections) {
            return new Download { Url=url, FileName=kind+".mp4", Folder=Path.Combine(owner.DataRoot,"media",job.Id), Category="Video", Queue=job.Queue, Connections=connections, ProtectedHeaders=job.ProtectedHeaders, LimitKbps=job.LimitKbps>0 ? Math.Max(1,job.LimitKbps/2) : 0 };
        }
        public static void SetStreams(Manager owner, Download job, string video, string audio, string description) {
            ValidateStream(video); if(audio!=null) ValidateStream(audio);
            // Keep completed child files and validated parts on retries; replace only the expiring URL.
            if(job.Video==null)job.Video=Child(owner,job,video,"video",Math.Max(1,job.Connections-(audio==null?0:2)));
            else job.Video.Url=video;
            if(audio!=null) { if(job.Audio==null)job.Audio=Child(owner,job,audio,"audio",Math.Min(2,Math.Max(1,job.Connections/2))); else job.Audio.Url=audio; }
            job.FormatDescription=description;
        }
        public static string Quote(string value) {
            // Windows CommandLineToArgvW quoting, including trailing backslashes. Never invoke a shell.
            var s=new StringBuilder("\""); int slashes=0;
            foreach(char c in value) { if(c=='\\'){slashes++;continue;} if(c=='\"'){s.Append('\\',slashes*2+1);s.Append(c);} else {s.Append('\\',slashes);s.Append(c);} slashes=0; }
            s.Append('\\',slashes*2);s.Append('"');return s.ToString();
        }
        public static async Task<string> Execute(string exe, IEnumerable<string> arguments, int seconds, CancellationToken ct) {
            using(var p=new Process()) using(var timeout=CancellationTokenSource.CreateLinkedTokenSource(ct)) {
                timeout.CancelAfter(TimeSpan.FromSeconds(seconds));
                p.StartInfo=new ProcessStartInfo(exe,string.Join(" ",arguments.Select(Quote))) { UseShellExecute=false,CreateNoWindow=true,RedirectStandardOutput=true,RedirectStandardError=true,StandardOutputEncoding=Encoding.UTF8,StandardErrorEncoding=Encoding.UTF8,WorkingDirectory=Path.GetDirectoryName(exe) };
                p.Start();
                var output=p.StandardOutput.ReadToEndAsync(); var error=p.StandardError.ReadToEndAsync();
                using(timeout.Token.Register(delegate {try{if(!p.HasExited)p.Kill();}catch(InvalidOperationException){} })) {
                    await Task.Run(()=>p.WaitForExit()).ConfigureAwait(false);
                    string result=await output.ConfigureAwait(false), diagnostic=await error.ConfigureAwait(false);
                    ct.ThrowIfCancellationRequested();
                    if(timeout.IsCancellationRequested)throw new IOException(Path.GetFileName(exe)+" timed out. Retry the download.");
                    if(p.ExitCode!=0)throw new IOException(Path.GetFileName(exe)+": "+Sanitize(diagnostic));
                    return result;
                }
            }
        }
        static string Sanitize(string message) {
            message=System.Text.RegularExpressions.Regex.Replace(message??"","https?://[^\\s]+","[URL]");
            return message.Length>1500?message.Substring(message.Length-1500):message;
        }
        void UpdateProgress() {
            lock(owner.Sync) {
                var children=new[]{job.Video,job.Audio}.Where(x=>x!=null).ToArray();
                job.Received=children.Sum(x=>x.Received);job.Size=children.All(x=>x.Size>=0)?children.Sum(x=>x.Size):-1;
                job.TransferredBytes=children.Sum(x=>x.TransferredBytes);
                job.RangeSupported=children.All(x=>x.RangeSupported);
            }
        }
        public async Task Run(CancellationToken ct) {
            try { await RunOnce(ct).ConfigureAwait(false); }
            catch(HttpStatusFailure ex) { throw new IOException("HTTP "+ex.StatusCode+": the media server rejected this captured link. No completed video was saved.",ex); }
        }
        async Task RunOnce(CancellationToken ct) {
            var total=Stopwatch.StartNew();
            try {
                ValidateSource(job.SourceUrl);string ffmpeg=RequireTool("ffmpeg.exe");
                if(job.Video==null || (string.IsNullOrEmpty(job.ProtectedSabr)&&((job.Video.Status!="Complete" && Expiring(job.Video.Url)) || (job.Audio!=null && job.Audio.Status!="Complete" && Expiring(job.Audio.Url)))))throw new IOException("Fresh browser-captured links are required. Open UDM on this video and choose the quality again.");
                string mediaRoot=Path.Combine(owner.DataRoot,"media",job.Id);
                Directory.CreateDirectory(mediaRoot);Directory.CreateDirectory(job.Folder);
                foreach(var child in new[]{job.Video,job.Audio}.Where(x=>x!=null)) {
                    // Persisted child paths are derived, never trusted for writing.
                    Guid id;if(!Guid.TryParseExact(child.Id,"N",out id))throw new IOException("Invalid media stream identifier.");
                    child.Folder=mediaRoot;child.FileName=child==job.Video?"video.mp4":"audio.mp4";
                    if(child.Status=="Complete" && !File.Exists(child.Target)) {child.Status="Paused";child.Segments.Clear();child.Received=0;}
                }
                lock(owner.Sync)job.Status="Downloading";
                var network=Stopwatch.StartNew();
                var sharedRate=new RateGate();
                if(!string.IsNullOrEmpty(job.ProtectedSabr)){
                    try{if(job.Video.Status!="Complete"||job.Audio==null||job.Audio.Status!="Complete")await new SabrTransfer(owner,job,Json.Read<SabrOffer>(Secrets.Reveal(job.ProtectedSabr))).Run(ct).ConfigureAwait(false);}
                    finally{UpdateProgress();lock(owner.Sync)job.TransferSeconds+=network.Elapsed.TotalSeconds;}
                }else using(var group=CancellationTokenSource.CreateLinkedTokenSource(ct)) {
                    var streams=new[]{job.Video,job.Audio}.Where(x=>x!=null).Select(async child=>{
                        try {if(child.Status!="Complete")await new Transfer(owner,child,job,sharedRate).Run(group.Token).ConfigureAwait(false);}
                        catch {group.Cancel();throw;}
                    }).ToArray();
                    var all=Task.WhenAll(streams);
                    try {
                        while(!all.IsCompleted) {UpdateProgress();await Task.WhenAny(all,Task.Delay(200)).ConfigureAwait(false);}
                        await all.ConfigureAwait(false);
                    } catch {
                        ct.ThrowIfCancellationRequested();
                        var error=streams.Where(t=>t.Exception!=null).SelectMany(t=>t.Exception.Flatten().InnerExceptions).FirstOrDefault(e=>!(e is OperationCanceledException));
                        throw error??new IOException("Media stream transfer failed.");
                    } finally {UpdateProgress();lock(owner.Sync)job.TransferSeconds+=network.Elapsed.TotalSeconds;}
                }
                ct.ThrowIfCancellationRequested();lock(owner.Sync)job.Status="Merging";owner.Save();
                string staging=Path.Combine(job.Folder,".udm-"+job.Id+".muxing.mp4");
                var merge=Stopwatch.StartNew();
                var args=new List<string>{"-hide_banner","-loglevel","error","-nostdin","-y","-protocol_whitelist","file,pipe","-i",job.Video.Target};
                if(job.Audio!=null)args.AddRange(new[]{"-protocol_whitelist","file,pipe","-i",job.Audio.Target,"-map","0:v:0","-map","1:a:0"});
                else args.AddRange(new[]{"-map","0:v:0","-map","0:a:0"});
                args.AddRange(new[]{"-c","copy","-movflags","+faststart",staging});
                try {
                    await Execute(ffmpeg,args,300,ct).ConfigureAwait(false);
                    if(!File.Exists(staging) || new FileInfo(staging).Length==0)throw new IOException("Media merge produced no output.");
                    if(job.ExactMediaQuality&&job.MediaPixelHeight>0){string actual=await Execute(RequireTool("ffprobe.exe"),new[]{"-v","error","-select_streams","v:0","-show_entries","stream=height","-of","default=noprint_wrappers=1:nokey=1",staging},20,ct).ConfigureAwait(false);int height;if(!int.TryParse(actual.Trim(),out height)||height!=job.MediaPixelHeight)throw new IOException("Captured stream dimensions do not match the selected quality. The output was not published.");}
                    string digest;using(var sha=SHA256.Create())using(var file=File.OpenRead(staging))digest=BitConverter.ToString(sha.ComputeHash(file)).Replace("-","").ToLowerInvariant();
                    ct.ThrowIfCancellationRequested();File.Move(staging,job.Target);Transfer.MarkInternetZone(job.Target);
                    lock(owner.Sync) {job.MergeSeconds+=merge.Elapsed.TotalSeconds;job.Sha256=digest;job.Size=new FileInfo(job.Target).Length;job.Received=job.Size;job.Status="Complete";job.Finished=DateTime.UtcNow;job.Error="";}
                    // Keep sources until the completed record is durably saved; a failed save can be recovered.
                    owner.Save();
                    foreach(var child in new[]{job.Video,job.Audio}.Where(x=>x!=null))try{File.Delete(child.Target);}catch(IOException){}
                } finally {if(File.Exists(staging))try{File.Delete(staging);}catch(IOException){}}
            } finally {lock(owner.Sync)job.ElapsedSeconds+=total.Elapsed.TotalSeconds;}
        }
        static bool Expiring(string url) {
            var q=System.Web.HttpUtility.ParseQueryString(new Uri(url).Query);long epoch;
            return long.TryParse(q["expire"],out epoch) && new DateTime(1970,1,1,0,0,0,DateTimeKind.Utc).AddSeconds(epoch)<DateTime.UtcNow.AddMinutes(2);
        }
    }
}
