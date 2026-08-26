using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Json;
using System.Security.Cryptography;
using System.Text;
using System.Text.RegularExpressions;

namespace Udm {
    public sealed class ConnectionProgress {
        public int Number;
        public long Downloaded,Start,End,Position;
        public string State="Waiting";
    }
    [DataContract] public class Segment {
        [DataMember] public int Index;
        [DataMember] public long Start;
        [DataMember] public long End;
        [DataMember] public long Done;
    }
    [DataContract] public class Download {
        [DataMember] public string Id = Guid.NewGuid().ToString("N");
        [DataMember] public string Url;
        [DataMember] public string FileName;
        [DataMember] public string Folder;
        [DataMember] public string Category;
        [DataMember] public string Queue = "Main queue";
        [DataMember] public string Status = "Queued";
        [DataMember] public string Error = "";
        [DataMember] public long Size = -1;
        [DataMember] public long Received;
        [DataMember] public string ETag;
        [DataMember] public string Modified;
        [DataMember] public bool RangeSupported;
        [DataMember] public int Connections = 8;
        [DataMember] public long LimitKbps;
        [DataMember] public string ExpectedSha256 = "";
        [DataMember] public string Sha256 = "";
        [DataMember] public string ProtectedHeaders = "";
        [DataMember] public DateTime Added = DateTime.UtcNow;
        [DataMember] public DateTime? Finished;
        [DataMember] public DateTime? LastAttempt;
        [DataMember] public DateTime? NotBefore;
        [DataMember] public List<Segment> Segments = new List<Segment>();
        [DataMember] public string SourceUrl;
        [DataMember] public int MediaHeight;
        [DataMember] public int MediaPixelHeight;
        [DataMember] public bool ExactMediaQuality;
        [DataMember] public string MediaFormatId;
        [DataMember] public Download Video;
        [DataMember] public Download Audio;
        [DataMember] public string FormatDescription;
        [DataMember] public long TransferredBytes;
        [DataMember] public double TransferSeconds;
        [DataMember] public double ResolveSeconds;
        [DataMember] public double MergeSeconds;
        [DataMember] public double ElapsedSeconds;
        [DataMember] public double PeakSpeed;
        [DataMember] public string Description="";
        [DataMember] public bool SuppressCompletionDialog;
        [DataMember] public string ProjectId;
        [DataMember] public string ProtectedSabr;
        public double Speed;
        public long? SessionLimitKbps;
        public List<ConnectionProgress> Workers=new List<ConnectionProgress>();
        public long EffectiveLimitKbps {get{return SessionLimitKbps??LimitKbps;}}
        public string TimingSummary { get { return TransferSeconds > 0 ? string.Format("Transferred {0} in {1:0.00}s | Average {2}/s | Peak sample {3}/s | Merge {4:0.00}s | Total {5:0.00}s", Names.Bytes(TransferredBytes), TransferSeconds, Names.Bytes(TransferredBytes / TransferSeconds), Names.Bytes(PeakSpeed), MergeSeconds, ElapsedSeconds) : ""; } }
        public string Target { get { return Path.Combine(Folder, FileName); } }
        public double Percent { get { return Size > 0 ? Math.Min(100, 100.0 * Received / Size) : Status == "Complete" ? 100 : 0; } }
    }
    [DataContract] public class QueueRule {
        [DataMember] public string Name = "Main queue";
        [DataMember] public bool Enabled = true;
        [DataMember] public int Parallel = 2;
        [DataMember] public bool Scheduled;
        [DataMember] public int StartMinute = 0;
        [DataMember] public int StopMinute = 1440;
        [DataMember] public int Days = 127;
        [DataMember] public bool RunOnce;
        [DataMember] public DateTime? StartOnceUtc;
        [DataMember] public DateTime? StopOnceUtc;
        [DataMember] public bool OnceStarted;
        [DataMember] public int? Retries;
        public bool ManualRun;
        public bool InWindow(DateTime now) {
            if (!Enabled) return false;
            if (ManualRun) return true;
            if (RunOnce) return StartOnceUtc.HasValue && now.ToUniversalTime()>=StartOnceUtc.Value && (!StopOnceUtc.HasValue || now.ToUniversalTime()<StopOnceUtc.Value);
            if (!Scheduled) return true;
            int m = now.Hour * 60 + now.Minute;
            var day = now.DayOfWeek;
            if (StartMinute > StopMinute && m < StopMinute) day = now.AddDays(-1).DayOfWeek;
            if ((Days & (1 << (int)day)) == 0) return false;
            if (StartMinute == StopMinute) return true;
            return StartMinute < StopMinute ? m >= StartMinute && m < StopMinute : m >= StartMinute || m < StopMinute;
        }
    }
    [DataContract] public class Preferences {
        [DataMember] public string DownloadFolder = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile), "Downloads", "UDM");
        [DataMember] public int Parallel = 3;
        [DataMember] public int Connections = 8;
        [DataMember] public int Retries = 3;
        [DataMember] public long LimitKbps;
        [DataMember] public long QuotaMb;
        [DataMember] public bool CategoryFolders = true;
        [DataMember] public bool ClipboardMonitor;
        [DataMember] public bool CloseToTray;
        [DataMember] public bool Sound = true;
        [DataMember] public bool SkipBrowserFileInfo;
        [DataMember] public bool SuppressProgressDialog;
        [DataMember] public bool SuppressCompletionDialog;
        [DataMember] public Dictionary<string,string> CategoryPaths=new Dictionary<string,string>();
        [DataMember] public List<string> CustomCategories=new List<string>();
        [DataMember] public List<CategoryRule> CategoryRules=new List<CategoryRule>();
        [DataMember] public List<SiteLogin> SiteLogins=new List<SiteLogin>();
        [DataMember] public string CaptureExtensions="zip 7z rar iso exe msi pdf mp4 mkv mp3 flac";
        [DataMember] public string CaptureExcludedHosts="";
        [DataMember] public string Proxy = "";
        [DataMember] public string ProxyUser = "";
        [DataMember] public string ProxySecret = "";
        [DataMember] public string ScanProgram = "";
        [DataMember] public string ScanArguments = "\"{file}\"";
    }
    [DataContract] public class AppState {
        [DataMember] public int Schema = 1;
        [DataMember] public Preferences Settings = new Preferences();
        [DataMember] public List<Download> Downloads = new List<Download>();
        [DataMember] public List<QueueRule> Queues = new List<QueueRule> { new QueueRule() };
        [DataMember] public List<GrabProject> Projects=new List<GrabProject>();
        [DataMember] public DateTime QuotaStart = DateTime.UtcNow;
        [DataMember] public long QuotaBytes;
    }
    [DataContract] public sealed class CategoryRule {
        [DataMember] public string Category;
        [DataMember] public string Extensions="";
        [DataMember] public string Hosts="";
    }
    [DataContract] public sealed class SiteLogin {
        [DataMember] public string Origin;
        [DataMember] public string UserName;
        [DataMember] public string ProtectedPassword;
    }
    [DataContract] public sealed class GrabProject {
        [DataMember] public string Id=Guid.NewGuid().ToString("N");
        [DataMember] public string Name="New project";
        [DataMember] public string StartUrl="https://";
        [DataMember] public string Extensions="zip pdf jpg png mp4 mp3";
        [DataMember] public int Depth=1;
        [DataMember] public int MaxPages=20;
        [DataMember] public DateTime? LastExplored;
        [DataMember] public List<GrabLink> Links=new List<GrabLink>();
    }
    [DataContract] public sealed class GrabLink {
        [DataMember] public string Url;
        [DataMember] public bool Selected=true;
        [DataMember] public string DownloadId;
        public override string ToString(){return Url;}
    }
    public static class Json {
        public static string Write<T>(T value) { using (var ms = new MemoryStream()) { new DataContractJsonSerializer(typeof(T)).WriteObject(ms, value); return Encoding.UTF8.GetString(ms.ToArray()); } }
        public static T Read<T>(string value) { using (var ms = new MemoryStream(Encoding.UTF8.GetBytes(value))) return (T)new DataContractJsonSerializer(typeof(T)).ReadObject(ms); }
    }
    public static class Secrets {
        public static string Protect(string value) { return string.IsNullOrEmpty(value) ? "" : Convert.ToBase64String(ProtectedData.Protect(Encoding.UTF8.GetBytes(value), null, DataProtectionScope.CurrentUser)); }
        public static string Reveal(string value) { return string.IsNullOrEmpty(value) ? "" : Encoding.UTF8.GetString(ProtectedData.Unprotect(Convert.FromBase64String(value), null, DataProtectionScope.CurrentUser)); }
    }
    public static class Names {
        public static string MediaTitle(string title) {
            // A page title is text, not a path; periods such as "feat." are not extensions.
            title = string.IsNullOrWhiteSpace(title) ? "YouTube video" : title.Replace('/', '_').Replace('\\', '_').Trim();
            title = Regex.Replace(title, @"\.(mp4|mkv|webm)$", "", RegexOptions.IgnoreCase);
            return Safe(title + ".mp4");
        }
        public static string Safe(string value) {
            value = (value ?? "download.bin").Replace('\\', '/').Split('/').Last();
            foreach (char c in Path.GetInvalidFileNameChars()) value = value.Replace(c, '_');
            value = value.Trim().TrimEnd('.');
            if (string.IsNullOrWhiteSpace(value)) value = "download.bin";
            if (Regex.IsMatch(value, @"^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\.|$)", RegexOptions.IgnoreCase)) value = "_" + value;
            if (value.Length > 160) value = value.Substring(0, 140) + Path.GetExtension(value).Substring(0, Math.Min(19, Path.GetExtension(value).Length));
            return value;
        }
        public static Uri Url(string value) {
            Uri url;
            if (!Uri.TryCreate(value, UriKind.Absolute, out url) || (url.Scheme != "https" && url.Scheme != "http" && url.Scheme != "ftp")) throw new ArgumentException("Use a complete HTTP, HTTPS or FTP URL.");
            if (!string.IsNullOrEmpty(url.UserInfo)) throw new ArgumentException("Put credentials in the authentication fields, not in the URL.");
            return url;
        }
        public static string Category(string file) {
            string ext = Path.GetExtension(file).ToLowerInvariant();
            if (new[] { ".zip", ".7z", ".rar", ".gz", ".tar", ".iso" }.Contains(ext)) return "Archives";
            if (new[] { ".mp4", ".mkv", ".webm", ".mov", ".avi", ".ts" }.Contains(ext)) return "Video";
            if (new[] { ".mp3", ".flac", ".wav", ".ogg", ".m4a", ".aac" }.Contains(ext)) return "Music";
            if (new[] { ".exe", ".msi", ".msix", ".apk" }.Contains(ext)) return "Programs";
            if (new[] { ".jpg", ".jpeg", ".png", ".svg", ".webp", ".gif" }.Contains(ext)) return "Images";
            if (new[] { ".pdf", ".doc", ".docx", ".xlsx", ".txt", ".epub", ".pptx", ".csv" }.Contains(ext)) return "Documents";
            return "Other";
        }
        public static string Bytes(double n) { if (n < 0) return "Unknown"; string[] units = { "B", "KB", "MB", "GB", "TB" }; int i = 0; while (n >= 1024 && i < units.Length - 1) { n /= 1024; i++; } return n.ToString(i == 0 ? "0" : "0.0") + " " + units[i]; }
        public static List<string> Expand(string text) {
            var result = new List<string>();
            foreach (string line in Regex.Split(text ?? "", @"[\r\n]+")) {
                string s = line.Trim(); if (s.Length == 0 || s.StartsWith("#")) continue;
                Match m = Regex.Match(s, @"\[(\d{1,6})-(\d{1,6})\]");
                if (!m.Success) { Url(s); result.Add(s); }
                else {
                    int first = int.Parse(m.Groups[1].Value), last = int.Parse(m.Groups[2].Value);
                    if (last < first || last - first > 999) throw new ArgumentException("A batch range can contain at most 1,000 URLs.");
                    int width = m.Groups[1].Value.StartsWith("0") ? m.Groups[1].Value.Length : 1;
                    for (int i = first; i <= last; i++) { string url = s.Substring(0, m.Index) + i.ToString(new string('0', width)) + s.Substring(m.Index + m.Length); Url(url); result.Add(url); }
                }
                if (result.Count > 1000) throw new ArgumentException("Add at most 1,000 URLs at a time.");
            }
            return result.Distinct().ToList();
        }
    }
}
