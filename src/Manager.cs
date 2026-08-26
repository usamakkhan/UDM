using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace Udm {
    public sealed partial class Manager : IDisposable {
        public readonly object Sync = new object();
        public readonly string DataRoot;
        public readonly AppState State;
        public readonly RateGate GlobalRate = new RateGate();
        readonly Dictionary<string,CancellationTokenSource> active = new Dictionary<string,CancellationTokenSource>();
        readonly Dictionary<string,Task> tasks = new Dictionary<string,Task>();
        readonly Dictionary<string,long> priorBytes = new Dictionary<string,long>();
        readonly HashSet<string> schedulePaused = new HashSet<string>();
        readonly Timer timer;
        DateTime lastTick = DateTime.UtcNow;
        int ticks;
        bool stopping;
        public string StorageError = "";
        public event Action<Download> Completed;
        public event Action<Download> Started;
        public Manager(string root, bool startTimer) {
            DataRoot = Path.GetFullPath(root); Directory.CreateDirectory(DataRoot);
            string path = Path.Combine(DataRoot, "state.json");
            if (File.Exists(path)) {
                try { State = Json.Read<AppState>(File.ReadAllText(path)); }
                catch (Exception ex) { throw new IOException("Cannot read UDM state. The file was preserved at " + path + ". Restore state.json.bak if needed.", ex); }
            } else State = new AppState();
            if (State.Schema != 1 || State.Settings == null || State.Downloads == null || State.Queues == null) throw new IOException("Unsupported or invalid UDM state schema.");
            if (State.Queues.Count == 0) State.Queues.Add(new QueueRule());
            if(State.Projects==null)State.Projects=new List<GrabProject>();
            if(State.Settings.CategoryRules==null)State.Settings.CategoryRules=new List<CategoryRule>();
            if(State.Settings.SiteLogins==null)State.Settings.SiteLogins=new List<SiteLogin>();
            if(State.Settings.CaptureExtensions==null)State.Settings.CaptureExtensions="zip 7z rar iso exe msi pdf mp4 mkv mp3 flac";
            if(State.Settings.CaptureExcludedHosts==null)State.Settings.CaptureExcludedHosts="";
            foreach (var j in State.Downloads) {
                if (new[] { "Downloading", "Verifying", "Pausing", "Resolving", "Merging", "Awaiting confirmation" }.Contains(j.Status)) j.Status = "Paused";
                // Do not trust persisted identifiers as paths.
                Guid id; if (!Guid.TryParseExact(j.Id, "N", out id)) throw new IOException("Invalid download identifier in state.");
                j.FileName = Names.Safe(j.FileName);
            }
            if (startTimer) timer = new Timer(delegate { try { Tick(); } catch (Exception ex) { StorageError = ex.Message; } }, null, 500, 1000);
        }
        public void Save() {
            lock (Sync) {
                string path = Path.Combine(DataRoot, "state.json"), temp = path + ".tmp";
                byte[] bytes = Encoding.UTF8.GetBytes(Json.Write(State));
                using (var file = new FileStream(temp, FileMode.Create, FileAccess.Write, FileShare.None)) { file.Write(bytes, 0, bytes.Length); file.Flush(true); }
                if (File.Exists(path)) File.Replace(temp, path, path + ".bak", true); else File.Move(temp, path);
                StorageError = "";
            }
        }
        public Download Add(string url, string folder, string name, string queue, bool paused, Dictionary<string,string> headers, string expected, DateTime? notBefore) {
            Uri uri = Names.Url(url);
            if (!string.IsNullOrEmpty(expected) && !System.Text.RegularExpressions.Regex.IsMatch(expected, "^[a-fA-F0-9]{64}$")) throw new ArgumentException("SHA-256 must contain 64 hexadecimal characters.");
            var allowed = new[] { "Authorization", "Cookie", "Referer", "User-Agent" };
            if (headers != null) foreach (var h in headers) if (!allowed.Contains(h.Key, StringComparer.OrdinalIgnoreCase) || h.Value.Contains("\r") || h.Value.Contains("\n")) throw new ArgumentException("Invalid request header.");
            string file = Names.Safe(string.IsNullOrWhiteSpace(name) ? Uri.UnescapeDataString(uri.AbsolutePath.Split('/').Last()) : name);
            string category = CategoryFor(uri,file);
            headers=HeadersFor(uri,headers);
            if (string.IsNullOrWhiteSpace(folder)) {
                string remembered;
                if(State.Settings.CategoryPaths!=null && State.Settings.CategoryPaths.TryGetValue(category,out remembered))folder=remembered;
                else {folder = State.Settings.DownloadFolder; if (State.Settings.CategoryFolders) folder = Path.Combine(folder, category);}
            }
            folder = Path.GetFullPath(folder);
            lock (Sync) {
                if (stopping) throw new InvalidOperationException("UDM is closing.");
                if (!State.Queues.Any(q => q.Name == queue)) queue = State.Queues[0].Name;
                string original = file;
                for (int i = 1; File.Exists(Path.Combine(folder, file)) || State.Downloads.Any(d => d.Target.Equals(Path.Combine(folder, file), StringComparison.OrdinalIgnoreCase)); i++) file = Path.GetFileNameWithoutExtension(original) + " (" + i + ")" + Path.GetExtension(original);
                var job = new Download { Url=uri.AbsoluteUri, FileName=file, Folder=folder, Category=category, Queue=queue, Status=paused ? "Paused" : "Queued", Connections=State.Settings.Connections, ExpectedSha256=expected ?? "", NotBefore=notBefore, ProtectedHeaders=headers == null || headers.Count == 0 ? "" : Secrets.Protect(Json.Write(headers)) };
                State.Downloads.Add(job);
                try { Save(); } catch { State.Downloads.Remove(job); throw; }
                return job;
            }
        }
        public bool IsActive(Download job) { lock (Sync) return active.ContainsKey(job.Id); }
        public string[] Categories {get{lock(Sync)return new[]{"Archives","Documents","Music","Programs","Video","Images","Other"}.Concat(State.Settings.CustomCategories??new List<string>()).Distinct(StringComparer.OrdinalIgnoreCase).ToArray();}}
        public void AddCategory(string value){string name=(value??"").Trim();if(name.Length==0||name.Length>60||Names.Safe(name)!=name)throw new ArgumentException("Choose a valid category name up to 60 characters.");lock(Sync){if(Categories.Contains(name,StringComparer.OrdinalIgnoreCase))throw new ArgumentException("That category already exists.");if(State.Settings.CustomCategories==null)State.Settings.CustomCategories=new List<string>();State.Settings.CustomCategories.Add(name);try{Save();}catch{State.Settings.CustomCategories.Remove(name);throw;}}}
        public Download AddMedia(string source, string name, int height, string videoUrl, string audioUrl, Dictionary<string,string> headers, bool exactQuality=false, string formatId=null, int pixelHeight=0) {
            MediaTransfer.ValidateSource(source);
            if (height < 144 || height > 4320) throw new ArgumentException("Choose a video height from 144 to 4320.");
            if(!string.IsNullOrEmpty(formatId)&&!System.Text.RegularExpressions.Regex.IsMatch(formatId,@"^\d{1,6}$"))throw new ArgumentException("Invalid video format identifier.");
            if(pixelHeight<0||pixelHeight>4320)throw new ArgumentException("Invalid video dimensions.");
            if(string.IsNullOrEmpty(videoUrl))throw new ArgumentException("UDM needs a captured playback link. Open the video in the browser and use its UDM download panel.");
            if (!string.IsNullOrEmpty(videoUrl)) MediaTransfer.ValidateStream(videoUrl);
            if (!string.IsNullOrEmpty(audioUrl)) MediaTransfer.ValidateStream(audioUrl);
            MediaTransfer.RequireTool("ffmpeg.exe");
            lock (Sync) {
                var job = Add(source, null, Names.MediaTitle(name), "Main queue", true, headers, "", null);
                try {
                    job.SourceUrl = source; job.MediaHeight = height;job.MediaPixelHeight=pixelHeight;job.ExactMediaQuality=exactQuality;job.MediaFormatId=formatId;
                    if (!string.IsNullOrEmpty(videoUrl)) MediaTransfer.SetStreams(this, job, videoUrl, string.IsNullOrEmpty(audioUrl)?null:audioUrl, "Browser capture · " + height + "p");
                    job.Status = "Queued"; Save(); return job;
                } catch { State.Downloads.Remove(job); Save(); throw; }
            }
        }
        public void Resume(Download job) { lock (Sync) { if (active.ContainsKey(job.Id) || job.Status == "Complete") return; job.SessionLimitKbps=null;job.Status = "Queued"; job.Error = ""; schedulePaused.Remove(job.Id); Save(); } }
        public void SetLimit(Download job,long kbps,bool remember) {
            if(kbps<0 || kbps>1000000)throw new ArgumentOutOfRangeException("kbps");
            lock(Sync){if(remember){job.LimitKbps=kbps;job.SessionLimitKbps=null;Save();}else job.SessionLimitKbps=kbps;}
        }
        public void ConfigureDestination(Download job,string destination,string category,string description,string queue,bool remember) {
            if(!Path.IsPathRooted(destination))throw new ArgumentException("Choose a complete destination path.");
            destination=Path.GetFullPath(destination);string name=Names.Safe(Path.GetFileName(destination));string folder=Path.GetDirectoryName(destination);
            if(Path.GetFileName(destination)!=name)throw new ArgumentException("Choose a valid Windows file name.");
            lock(Sync) {
                if(IsActive(job)||job.Status=="Complete")throw new InvalidOperationException("Stop the download before changing its destination.");
                if(File.Exists(destination)||State.Downloads.Any(d=>d.Id!=job.Id&&d.Target.Equals(destination,StringComparison.OrdinalIgnoreCase)))throw new IOException("That destination already exists. Choose another name to keep both files.");
                if(!State.Queues.Any(q=>q.Name==queue))throw new ArgumentException("Choose an existing queue.");
                job.FileName=name;job.Folder=folder;job.Category=category;job.Description=description;job.Queue=queue;
                if(remember){if(State.Settings.CategoryPaths==null)State.Settings.CategoryPaths=new Dictionary<string,string>();State.Settings.CategoryPaths[category]=folder;}
                Save();
            }
        }
        public void Pause(Download job) { lock (Sync) { schedulePaused.Remove(job.Id); if (active.ContainsKey(job.Id)) { job.Status = "Pausing"; active[job.Id].Cancel(); } else if (job.Status != "Complete") job.Status = "Paused"; Save(); } }
        public void Remove(Download job) {
            lock (Sync) { if (active.ContainsKey(job.Id)) throw new InvalidOperationException("Pause the download and wait until it stops before removing it."); State.Downloads.Remove(job); Save(); }
            // Completed user files are deliberately retained.
            string p = Path.Combine(DataRoot, "parts", job.Id);
            if (Directory.Exists(p)) { foreach (string f in Directory.GetFiles(p, "*.part")) File.Delete(f); if (!Directory.EnumerateFileSystemEntries(p).Any()) Directory.Delete(p); }
        }
        public void Tick() {
            lock (Sync) {
                if (stopping) return;
                DateTime now = DateTime.UtcNow; double seconds = Math.Max(0.1, (now-lastTick).TotalSeconds); lastTick = now;
                foreach (var job in State.Downloads) {
                    long previous; priorBytes.TryGetValue(job.Id, out previous); job.Speed = active.ContainsKey(job.Id) && job.Status == "Downloading" ? Math.Max(0, (job.Received-previous)/seconds) : 0; priorBytes[job.Id] = job.Received;
                    if (job.Speed > job.PeakSpeed) job.PeakSpeed = job.Speed;
                }
                foreach (var q in State.Queues) {
                    if(q.RunOnce&&q.OnceStarted&&!State.Downloads.Any(d=>d.Queue==q.Name&&(active.ContainsKey(d.Id)||d.Status=="Queued"))){q.Enabled=false;q.ManualRun=false;}
                    if(q.ManualRun&&!State.Downloads.Any(d=>d.Queue==q.Name&&(active.ContainsKey(d.Id)||d.Status=="Queued")))q.ManualRun=false;
                    bool open = q.InWindow(DateTime.Now);
                    if (!open) foreach (var j in State.Downloads.Where(d => d.Queue == q.Name && active.ContainsKey(d.Id) && d.Status != "Pausing")) { schedulePaused.Add(j.Id); j.Status = "Pausing"; active[j.Id].Cancel(); }
                    if (!open) continue;
                    int running = State.Downloads.Count(d => d.Queue == q.Name && active.ContainsKey(d.Id));
                    foreach (var j in State.Downloads.Where(d => d.Queue == q.Name && d.Status == "Queued" && (!d.NotBefore.HasValue || d.NotBefore <= now)).ToList()) {
                        if (running >= q.Parallel || active.Count >= State.Settings.Parallel) break;
                        if(q.RunOnce)q.OnceStarted=true;Start(j); running++;
                    }
                }
                if (++ticks % 3 == 0) Save();
            }
        }
        public Task Start(Download job) {
            lock (Sync) {
                if (active.ContainsKey(job.Id)) return tasks[job.Id];
                if (stopping || job.Status == "Complete") return Task.FromResult(0);
                var cts = new CancellationTokenSource(); active.Add(job.Id, cts);job.LastAttempt=DateTime.UtcNow;job.Status = "Downloading"; job.Error = ""; priorBytes[job.Id] = job.Received;
                Task task = Task.Run(async () => {
                    bool completed = false;
                    try { var started=Started;if(started!=null)started(job);if (!string.IsNullOrEmpty(job.SourceUrl)) await new MediaTransfer(this, job).Run(cts.Token).ConfigureAwait(false); else await new Transfer(this, job).Run(cts.Token).ConfigureAwait(false); completed = true; }
                    catch (OperationCanceledException) { lock (Sync) { job.Status = schedulePaused.Remove(job.Id) && !stopping ? "Queued" : "Paused"; job.Error = ""; } }
                    catch (Exception ex) { lock (Sync) { if (cts.IsCancellationRequested) { job.Status = schedulePaused.Remove(job.Id) && !stopping ? "Queued" : "Paused"; job.Error = ""; } else { job.Status = "Failed"; job.Error = ex.Message; } } }
                    finally { lock (Sync) { active.Remove(job.Id); tasks.Remove(job.Id); job.Speed = 0; cts.Dispose(); try { Save(); } catch (Exception ex) { StorageError = ex.Message; } } }
                    if (completed) {
                        try { Scan(job); } catch (Exception ex) { lock (Sync) { job.Error = "Downloaded; scanner could not start: " + ex.Message; Save(); } }
                        var handler = Completed; if (handler != null) handler(job);
                    }
                });
                tasks[job.Id] = task; return task;
            }
        }
        void Scan(Download job) {
            string scanner = State.Settings.ScanProgram;
            if (string.IsNullOrWhiteSpace(scanner)) return;
            if (!File.Exists(scanner) || !Path.IsPathRooted(scanner)) throw new IOException("Choose an existing scanner executable with an absolute path.");
            Process.Start(new ProcessStartInfo(scanner, State.Settings.ScanArguments.Replace("{file}", job.Target)) { UseShellExecute=false, CreateNoWindow=true });
        }
        public async Task WaitForQuota(int bytes, CancellationToken ct) {
            while (true) {
                lock (Sync) {
                    if (DateTime.UtcNow - State.QuotaStart >= TimeSpan.FromHours(1)) { State.QuotaStart=DateTime.UtcNow; State.QuotaBytes=0; }
                    long max = State.Settings.QuotaMb * 1024L * 1024;
                    if (max <= 0 || State.QuotaBytes + bytes <= max) { State.QuotaBytes += bytes; return; }
                }
                await Task.Delay(1000, ct).ConfigureAwait(false);
            }
        }
        public async Task Stop() {
            Task[] waiting;
            lock (Sync) { stopping = true; if (timer != null) timer.Change(Timeout.Infinite, Timeout.Infinite); foreach (var c in active.Values) c.Cancel(); waiting=tasks.Values.ToArray(); }
            await Task.WhenAll(waiting).ConfigureAwait(false); Save();
        }
        public void Dispose() { Stop().GetAwaiter().GetResult(); if (timer != null) timer.Dispose(); }
    }
}
