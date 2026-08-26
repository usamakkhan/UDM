using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Net.Http.Headers;
using System.Security.Cryptography;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace Udm {
    public sealed class ChangedResource : IOException { public ChangedResource(string text) : base(text) {} }
    public sealed class HttpStatusFailure : IOException {
        public readonly int StatusCode;
        public HttpStatusFailure(int code) : base("The server rejected the download URL (HTTP " + code + ").") { StatusCode=code; }
    }
    public sealed class RateGate {
        readonly object sync = new object();
        DateTime next = DateTime.UtcNow;
        public async Task Wait(int bytes, long kbps, CancellationToken ct) {
            if (kbps <= 0) { lock (sync) next = DateTime.UtcNow; return; }
            double delay;
            lock (sync) { DateTime now = DateTime.UtcNow; if (next < now) next = now; next = next.AddSeconds((double)bytes / (kbps * 1024.0)); delay = (next - now).TotalMilliseconds; }
            if (delay > 0) await Task.Delay((int)Math.Min(int.MaxValue, delay), ct).ConfigureAwait(false);
        }
    }
    public sealed class Transfer {
        readonly Download job;
        readonly Manager owner;
        readonly RateGate localRate;
        readonly Download limitOwner;
        readonly string parts;
        public Transfer(Manager manager, Download download,Download sharedLimitOwner=null,RateGate sharedRate=null) { owner = manager; job = download; limitOwner=sharedLimitOwner??download;localRate=sharedRate??new RateGate();parts = Path.Combine(owner.DataRoot, "parts", job.Id); }
        string Part(Segment s) { return Path.Combine(parts, s.Index.ToString("D4") + ".part"); }
        Dictionary<string,string> Headers() {
            return string.IsNullOrEmpty(job.ProtectedHeaders) ? new Dictionary<string,string>() : Json.Read<Dictionary<string,string>>(Secrets.Reveal(job.ProtectedHeaders));
        }
        HttpClient Client() {
            var handler = new HttpClientHandler { AllowAutoRedirect = false, AutomaticDecompression = DecompressionMethods.None, UseCookies = false };
            if (!string.IsNullOrWhiteSpace(owner.State.Settings.Proxy)) {
                var proxy = new WebProxy(owner.State.Settings.Proxy);
                if (!string.IsNullOrEmpty(owner.State.Settings.ProxyUser)) proxy.Credentials = new NetworkCredential(owner.State.Settings.ProxyUser, Secrets.Reveal(owner.State.Settings.ProxySecret));
                handler.Proxy = proxy;
            }
            return new HttpClient(handler) { Timeout = Timeout.InfiniteTimeSpan };
        }
        async Task<HttpResponseMessage> Send(HttpClient client, long? start, long? end, bool conditional, CancellationToken ct) {
            Uri uri = Names.Url(job.Url);
            var headers = Headers();
            bool allowSensitive = true;
            for (int redirect = 0; redirect < 11; redirect++) {
                if (uri.Scheme != "http" && uri.Scheme != "https") throw new IOException("HTTP redirect used an unsupported protocol.");
                using (var req = new HttpRequestMessage(HttpMethod.Get, uri)) {
                    req.Headers.TryAddWithoutValidation("User-Agent", "UDM/0.6.1");
                    req.Headers.TryAddWithoutValidation("Accept-Encoding", "identity");
                    if (start.HasValue) req.Headers.Range = new RangeHeaderValue(start, end);
                    if (conditional) {
                        if (!string.IsNullOrEmpty(job.ETag) && !job.ETag.StartsWith("W/")) req.Headers.TryAddWithoutValidation("If-Range", job.ETag);
                        else if (!string.IsNullOrEmpty(job.Modified)) req.Headers.TryAddWithoutValidation("If-Range", job.Modified);
                    }
                    foreach (var h in headers) {
                        if (!allowSensitive && (h.Key.Equals("Cookie", StringComparison.OrdinalIgnoreCase) || h.Key.Equals("Authorization", StringComparison.OrdinalIgnoreCase) || h.Key.Equals("Referer", StringComparison.OrdinalIgnoreCase))) continue;
                        req.Headers.Remove(h.Key); req.Headers.TryAddWithoutValidation(h.Key, h.Value);
                    }
                    HttpResponseMessage response;
                    using (var timeout = CancellationTokenSource.CreateLinkedTokenSource(ct)) {
                        timeout.CancelAfter(TimeSpan.FromSeconds(30));
                        response = await client.SendAsync(req, HttpCompletionOption.ResponseHeadersRead, timeout.Token).ConfigureAwait(false);
                    }
                    int code = (int)response.StatusCode;
                    if (new[] {301,302,303,307,308}.Contains(code)) {
                        Uri target = response.Headers.Location;
                        response.Dispose();
                        if (target == null) throw new IOException("Redirect response has no Location header.");
                        target = target.IsAbsoluteUri ? target : new Uri(uri, target);
                        if (uri.Scheme == "https" && target.Scheme != "https") throw new IOException("Refusing an HTTPS downgrade redirect.");
                        if (uri.GetLeftPart(UriPartial.Authority) != target.GetLeftPart(UriPartial.Authority)) allowSensitive = false;
                        uri = target; continue;
                    }
                    return response;
                }
            }
            throw new IOException("Too many redirects.");
        }
        static string Etag(HttpResponseMessage r) { return r.Headers.ETag != null && !r.Headers.ETag.IsWeak ? r.Headers.ETag.ToString() : null; }
        static string Modified(HttpResponseMessage r) { return r.Content.Headers.LastModified.HasValue ? r.Content.Headers.LastModified.Value.ToString("R") : null; }
        static void EnsureSuccess(HttpResponseMessage response) {
            int code=(int)response.StatusCode;
            if(code==401 || code==403)throw new HttpStatusFailure(code);
            response.EnsureSuccessStatusCode();
        }
        async Task Probe(HttpClient client, CancellationToken ct) {
            using (var r = await Send(client, 0, 0, false, ct).ConfigureAwait(false)) {
                bool empty = (int)r.StatusCode == 416 && r.Content.Headers.ContentRange != null && r.Content.Headers.ContentRange.Length == 0;
                if (!empty) EnsureSuccess(r);
                var range = r.Content.Headers.ContentRange;
                bool support = r.StatusCode == HttpStatusCode.PartialContent && range != null && range.Unit == "bytes" && range.From == 0 && range.To == 0 && range.Length.HasValue && range.Length > 0;
                if (r.StatusCode == HttpStatusCode.PartialContent && !support) throw new IOException("Server returned an invalid probe range.");
                long size = empty ? 0 : support ? range.Length.Value : r.Content.Headers.ContentLength ?? -1;
                string etag = Etag(r), modified = Modified(r);
                bool validator = !string.IsNullOrEmpty(etag) || !string.IsNullOrEmpty(modified);
                bool same = job.Size == size && (etag != null ? job.ETag == etag : modified != null && job.Modified == modified);
                lock (owner.Sync) {
                    if (!same || !support || !validator) {
                        ClearParts(); job.Segments.Clear(); job.Received = 0;
                    }
                    job.Size = size; job.ETag = etag; job.Modified = modified;
                    job.RangeSupported = support && validator;
                    if (job.Segments.Count == 0 && size != 0) {
                        if (job.RangeSupported) {
                            // Four pieces per worker reduces request overhead while allowing work redistribution.
                            long chunk = Math.Max(1024L * 1024, (size + Math.Max(1, job.Connections) * 4 - 1) / (Math.Max(1, job.Connections) * 4));
                            int index = 0;
                            for (long begin = 0; begin < size; begin += chunk) job.Segments.Add(new Segment { Index = index++, Start = begin, End = Math.Min(size - 1, begin + chunk - 1) });
                        } else job.Segments.Add(new Segment { Index = 0, Start = 0, End = size - 1 });
                    }
                    foreach (var s in job.Segments) {
                        long bytes = File.Exists(Part(s)) ? new FileInfo(Part(s)).Length : 0;
                        if (s.End >= s.Start && bytes > s.End - s.Start + 1) { File.Delete(Part(s)); bytes = 0; }
                        s.Done = bytes;
                    }
                    job.Received = job.Segments.Sum(s => s.Done);
                }
                owner.Save();
            }
        }
        void ClearParts() {
            if (Directory.Exists(parts)) foreach (string p in Directory.GetFiles(parts, "*.part", SearchOption.TopDirectoryOnly)) File.Delete(p);
        }
        async Task ReadToFile(Stream input, FileStream output, long maximum, Segment s, CancellationToken ct,ConnectionProgress activity=null) {
            byte[] buffer = new byte[64 * 1024];
            long copied = 0;
            for (;;) {
                int n;
                using (var timeout = CancellationTokenSource.CreateLinkedTokenSource(ct)) {
                    timeout.CancelAfter(TimeSpan.FromSeconds(45));
                    n = await input.ReadAsync(buffer, 0, buffer.Length, timeout.Token).ConfigureAwait(false);
                }
                if (n == 0) break;
                if (maximum >= 0 && copied + n > maximum) throw new IOException("Server sent more bytes than the declared range.");
                await owner.GlobalRate.Wait(n, owner.State.Settings.LimitKbps, ct).ConfigureAwait(false);
                await localRate.Wait(n, limitOwner.EffectiveLimitKbps, ct).ConfigureAwait(false);
                await owner.WaitForQuota(n, ct).ConfigureAwait(false);
                await output.WriteAsync(buffer, 0, n, ct).ConfigureAwait(false);
                copied += n;
                lock (owner.Sync) { s.Done += n; job.Received += n; job.TransferredBytes += n;if(activity!=null){activity.Downloaded+=n;activity.Position=s.Start+s.Done;activity.State="Receiving data";} }
            }
            if (maximum >= 0 && copied != maximum) throw new IOException("Connection ended before the expected bytes arrived.");
            output.Flush(true);
        }
        async Task SegmentDownload(HttpClient client, Segment s, CancellationToken ct,ConnectionProgress activity) {
            int retries = owner.RetriesFor(job.Queue);
            for (int attempt = 0;; attempt++) {
                ct.ThrowIfCancellationRequested();
                try {
                    long have = File.Exists(Part(s)) ? new FileInfo(Part(s)).Length : 0;
                    if (!job.RangeSupported) have = 0;
                    lock (owner.Sync) { job.Received += have - s.Done; s.Done = have;activity.Start=s.Start;activity.End=s.End;activity.Position=s.Start+have;activity.State="Connecting"; }
                    long need = s.End >= s.Start ? s.End - s.Start + 1 : -1;
                    if (need >= 0 && have == need) return;
                    using (var r = await Send(client, job.RangeSupported ? (long?)(s.Start + have) : null, job.RangeSupported ? (long?)s.End : null, job.RangeSupported, ct).ConfigureAwait(false)) {
                        if (job.RangeSupported) {
                            if (r.StatusCode == HttpStatusCode.OK || (int)r.StatusCode == 416) throw new ChangedResource("The server changed the file or stopped honoring byte ranges.");
                            EnsureSuccess(r);
                            var cr = r.Content.Headers.ContentRange;
                            if (r.StatusCode != HttpStatusCode.PartialContent || cr == null || cr.Unit != "bytes" || cr.From != s.Start + have || cr.To != s.End || cr.Length != job.Size) throw new ChangedResource("The server returned a mismatched byte range.");
                            if (job.ETag != null && Etag(r) != null && Etag(r) != job.ETag) throw new ChangedResource("The remote file changed during transfer.");
                        } else {
                            EnsureSuccess(r);
                            if (r.StatusCode == HttpStatusCode.PartialContent) throw new IOException("Unexpected partial response to a full download.");
                            long current = r.Content.Headers.ContentLength ?? -1;
                            lock (owner.Sync) { job.Size = current; s.End = current - 1; need = current; }
                        }
                        if (r.Content.Headers.ContentEncoding.Any(e => !e.Equals("identity", StringComparison.OrdinalIgnoreCase))) throw new IOException("Server ignored identity encoding; raw compressed transfer is not supported.");
                        using (var stream = await r.Content.ReadAsStreamAsync().ConfigureAwait(false))
                        using (var output = new FileStream(Part(s), have > 0 ? FileMode.Append : FileMode.Create, FileAccess.Write, FileShare.Read, 65536, true))
                            await ReadToFile(stream, output, need < 0 ? -1 : need - have, s, ct,activity).ConfigureAwait(false);
                    }
                    return;
                } catch (ChangedResource) { throw; }
                catch (HttpStatusFailure) { throw; }
                catch (Exception ex) {
                    if (ct.IsCancellationRequested) throw new OperationCanceledException(ct);
                    if (attempt >= retries || ex is UnauthorizedAccessException) throw;
                }
                lock(owner.Sync)activity.State="Retrying";
                await Task.Delay(Math.Min(10000, 500 * (1 << Math.Min(attempt, 4))), ct).ConfigureAwait(false);
            }
        }
        public async Task Run(CancellationToken ct) {
            var elapsed = Stopwatch.StartNew();
            double priorTransfer = job.TransferSeconds;
            try {
            Directory.CreateDirectory(parts); Directory.CreateDirectory(job.Folder);
            if (Names.Url(job.Url).Scheme == "ftp") await Ftp(ct).ConfigureAwait(false);
            else using (var client = Client()) {
                for (int generation = 0;; generation++) {
                    await Probe(client, ct).ConfigureAwait(false);
                    var pending = new Queue<Segment>(job.Segments.Where(s => s.End < s.Start || s.Done < s.End - s.Start + 1));
                    lock(owner.Sync)job.Workers=Enumerable.Range(1,job.RangeSupported?Math.Max(1,Math.Min(16,job.Connections)):1).Select(n=>new ConnectionProgress{Number=n}).ToList();
                    using (var group = CancellationTokenSource.CreateLinkedTokenSource(ct)) {
                        var workers = job.Workers.Select(async activity => {
                            try {
                                while (true) {
                                    Segment s;
                                    lock (pending) { if (pending.Count == 0) return; s = pending.Dequeue(); }
                                    await SegmentDownload(client, s, group.Token,activity).ConfigureAwait(false);
                                }
                            } catch { group.Cancel(); throw; }
                            finally {lock(owner.Sync)activity.State=group.IsCancellationRequested?"Stopped":"Finished";}
                        }).ToArray();
                        try { await Task.WhenAll(workers).ConfigureAwait(false); break; }
                        catch {
                            ct.ThrowIfCancellationRequested();
                            var errors = workers.Where(t => t.Exception != null).SelectMany(t => t.Exception.Flatten().InnerExceptions).ToList();
                            var changed = errors.OfType<ChangedResource>().FirstOrDefault();
                            if (changed == null || generation >= 1) throw errors.FirstOrDefault(e => !(e is OperationCanceledException)) ?? new IOException("Transfer failed.");
                            lock (owner.Sync) { ClearParts(); job.Segments.Clear(); job.Received = 0; job.ETag = null; job.Modified = null; }
                        }
                    }
                }
            }
            ct.ThrowIfCancellationRequested();
            lock (owner.Sync) job.TransferSeconds = priorTransfer + elapsed.Elapsed.TotalSeconds;
            lock (owner.Sync) job.Status = "Verifying";
            // Assemble on the target volume so final publication is an atomic rename and never overwrites a file.
            string staging = Path.Combine(job.Folder, ".udm-" + job.Id + ".assembling");
            using (var output = new FileStream(staging, FileMode.Create, FileAccess.Write, FileShare.None, 65536, true)) {
                foreach (var s in job.Segments.OrderBy(s => s.Index)) {
                    using (var input = new FileStream(Part(s), FileMode.Open, FileAccess.Read, FileShare.Read, 65536, true)) await input.CopyToAsync(output, 65536, ct).ConfigureAwait(false);
                }
                output.Flush(true);
            }
            string digest;
            using (var sha = SHA256.Create()) using (var file = File.OpenRead(staging)) digest = BitConverter.ToString(sha.ComputeHash(file)).Replace("-", "").ToLowerInvariant();
            if (!string.IsNullOrWhiteSpace(job.ExpectedSha256) && !digest.Equals(job.ExpectedSha256.Trim(), StringComparison.OrdinalIgnoreCase)) {
                File.Delete(staging); throw new IOException("SHA-256 mismatch. The file was not published.");
            }
            ct.ThrowIfCancellationRequested();
            if (File.Exists(job.Target)) throw new IOException("Destination exists. Choose another file name in Properties to keep both files.");
            File.Move(staging, job.Target);
            MarkInternetZone(job.Target);
            lock (owner.Sync) { job.Sha256 = digest; job.Size = new FileInfo(job.Target).Length; job.Received = job.Size; job.Status = "Complete"; job.Finished = DateTime.UtcNow; job.Error = ""; }
            owner.Save(); ClearParts();
            } finally {
                lock (owner.Sync) { if (job.Status != "Complete" && job.Status != "Verifying") job.TransferSeconds = priorTransfer + elapsed.Elapsed.TotalSeconds; job.ElapsedSeconds += elapsed.Elapsed.TotalSeconds; }
            }
        }
        [DllImport("kernel32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
        static extern SafeFileHandle CreateFile(string name,uint access,uint share,IntPtr security,uint creation,uint flags,IntPtr template);
        internal static void MarkInternetZone(string file) {
            // Framework path validation rejects ADS paths; use the documented Win32 file API.
            try { using(var handle=CreateFile(file+":Zone.Identifier",0x40000000,1,IntPtr.Zero,2,0,IntPtr.Zero)) {
                if(handle.IsInvalid)return;using(var stream=new FileStream(handle,FileAccess.Write)) {byte[] bytes=Encoding.ASCII.GetBytes("[ZoneTransfer]\r\nZoneId=3\r\n");stream.Write(bytes,0,bytes.Length);}
            } } catch(IOException) {} catch(UnauthorizedAccessException) {}
        }
        async Task Ftp(CancellationToken ct) {
            // FTP servers vary in REST support; a fresh sequential transfer avoids unsafe resume claims.
            lock (owner.Sync) { ClearParts(); job.Received = 0; job.Size = -1; job.RangeSupported = false; job.Segments = new List<Segment> { new Segment { Index=0, Start=0, End=-2 } }; }
            var request = (FtpWebRequest)WebRequest.Create(job.Url);
            request.Method = WebRequestMethods.Ftp.DownloadFile; request.UseBinary = true; request.UsePassive = true; request.KeepAlive = false; request.Timeout = 30000; request.ReadWriteTimeout = 45000;
            var headers = Headers();
            if (headers.ContainsKey("Authorization") && headers["Authorization"].StartsWith("Basic ")) {
                var credentials = Encoding.UTF8.GetString(Convert.FromBase64String(headers["Authorization"].Substring(6))).Split(new[] {':'}, 2);
                request.Credentials = new NetworkCredential(credentials[0], credentials.Length > 1 ? credentials[1] : "");
            } else request.Credentials = new NetworkCredential("anonymous", "udm@example.invalid");
            using (ct.Register(request.Abort))
            using (var response = (FtpWebResponse)await request.GetResponseAsync().ConfigureAwait(false))
            using (var input = response.GetResponseStream())
            using (var output = new FileStream(Part(job.Segments[0]), FileMode.Create, FileAccess.Write, FileShare.Read, 65536, true)) await ReadToFile(input, output, -1, job.Segments[0], ct).ConfigureAwait(false);
        }
    }
}
