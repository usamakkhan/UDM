using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Windows.Forms;

namespace Udm {
    public class Sheet : Form {
        protected readonly TableLayoutPanel Fields;
        protected readonly FlowLayoutPanel Actions;
        protected readonly Panel Body;
        public Sheet(string title, int width, int height) {
            Text=title; Size=new Size(width,height); MinimumSize=new Size(width,Math.Min(height,450)); StartPosition=FormStartPosition.CenterParent;
            Font=new Font("Segoe UI",10); BackColor=Color.White; FormBorderStyle=FormBorderStyle.Sizable; MinimizeBox=false; MaximizeBox=false;
            Actions=new FlowLayoutPanel {Dock=DockStyle.Bottom,Height=60,Padding=new Padding(12),FlowDirection=FlowDirection.RightToLeft,BackColor=Color.FromArgb(245,247,250)};
            Body=new Panel {Dock=DockStyle.Fill,Padding=new Padding(22),AutoScroll=true};
            Fields=new TableLayoutPanel {Dock=DockStyle.Top,AutoSize=true,ColumnCount=2}; Fields.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute,168)); Fields.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,100));
            Body.Controls.Add(Fields); Controls.Add(Body); Controls.Add(Actions);
            Shown+=delegate{AppTheme.Apply(this);};
        }
        protected void Row(string caption,Control control,int height) {int row=Fields.RowCount++; Fields.RowStyles.Add(new RowStyle(SizeType.Absolute,height)); var label=new Label{Text=caption,AutoSize=false,Dock=DockStyle.Fill,TextAlign=ContentAlignment.MiddleLeft}; control.Dock=DockStyle.Fill; control.Margin=new Padding(0,5,0,5); Fields.Controls.Add(label,0,row); Fields.Controls.Add(control,1,row);}
        protected TextBox Input(string label,string value,int height) {var t=new TextBox{Text=value}; if(height>50) {t.Multiline=true;t.ScrollBars=ScrollBars.Vertical;} Row(label,t,height);return t;}
        protected NumericUpDown Number(string label,decimal value,decimal min,decimal max) {var n=new NumericUpDown{Minimum=min,Maximum=max,Value=Math.Max(min,Math.Min(max,value))};Row(label,n,40);return n;}
        protected CheckBox Check(string label,bool value) {var c=new CheckBox{Text=label,Checked=value};Row("",c,38);return c;}
        protected ComboBox Choice(string label,IEnumerable<string> values,string selected) {var c=new ComboBox{DropDownStyle=ComboBoxStyle.DropDownList};c.Items.AddRange(values.Cast<object>().ToArray());c.SelectedItem=selected;if(c.SelectedIndex<0 && c.Items.Count>0)c.SelectedIndex=0;Row(label,c,40);return c;}
        protected Button ActionButton(string text,Action action,bool primary) {var b=new Button{Text=text,AutoSize=true,Height=34,Padding=new Padding(8,0,8,0),FlatStyle=FlatStyle.Flat,BackColor=primary?Color.FromArgb(35,94,223):Color.White,ForeColor=primary?Color.White:Color.FromArgb(38,48,65)};b.FlatAppearance.BorderColor=Color.FromArgb(218,224,234);b.Click+=delegate {try{action();}catch(Exception ex){MessageBox.Show(this,ex.Message,"UDM",MessageBoxButtons.OK,MessageBoxIcon.Warning);}};Actions.Controls.Add(b);return b;}
        protected void Note(string text) {var l=new Label{Text=text,AutoSize=true,MaximumSize=new Size(470,0),ForeColor=Color.FromArgb(94,106,122)};Row("",l,58);}
    }
    public sealed class AddDialog : Sheet {
        public AddDialog(Manager manager,string initial) : base("Add downloads • UDM",760,760) {
            var urls=Input("Download URLs",initial ?? "",140);
            Note("One URL per line. Numbered batches are supported: file[001-020].zip");
            var folder=Input("Save in", "",40);folder.AccessibleDescription="Leave blank to use category folders from Settings.";
            var name=Input("File name (optional)","",40);
            var queue=Choice("Queue",manager.State.Queues.Select(q=>q.Name),"Main queue");
            var username=Input("User name", "",40);var password=Input("Password", "",40);password.UseSystemPasswordChar=true;
            var referer=Input("Referrer (optional)","",40);var cookie=Input("Cookie (optional)","",40);cookie.UseSystemPasswordChar=true;
            var hash=Input("Expected SHA-256","",40);
            var scheduled=Check("Start after this date and time",false);
            var when=new DateTimePicker{Format=DateTimePickerFormat.Custom,CustomFormat="yyyy-MM-dd HH:mm",Value=DateTime.Now.AddHours(1)};Row("Local time",when,40);
            Action<bool> add=paused=>{
                var list=Names.Expand(urls.Text);if(list.Count==0)throw new ArgumentException("Paste at least one URL.");
                if(list.Count>1 && (!string.IsNullOrWhiteSpace(name.Text)||!string.IsNullOrWhiteSpace(hash.Text)))throw new ArgumentException("A file name or checksum can only be set for one URL at a time.");
                var headers=new Dictionary<string,string>();
                if(username.Text.Length>0)headers["Authorization"]="Basic "+Convert.ToBase64String(Encoding.UTF8.GetBytes(username.Text+":"+password.Text));
                if(referer.Text.Length>0)headers["Referer"]=referer.Text;
                if(cookie.Text.Length>0)headers["Cookie"]=cookie.Text;
                // Validate every input before creating the first record.
                if(hash.Text.Length>0 && !System.Text.RegularExpressions.Regex.IsMatch(hash.Text.Trim(),"^[a-fA-F0-9]{64}$"))throw new ArgumentException("SHA-256 must contain 64 hexadecimal characters.");
                if(folder.Text.Length>0)Path.GetFullPath(folder.Text);
                foreach(string url in list)manager.Add(url,folder.Text,name.Text,queue.Text,paused,headers,hash.Text.Trim(),scheduled.Checked?(DateTime?)when.Value.ToUniversalTime():null);
                DialogResult=DialogResult.OK;Close();
            };
            AcceptButton=ActionButton("Download",()=>add(false),true);ActionButton("Add paused",()=>add(true),false);ActionButton("Cancel",Close,false);
            ActionButton("Browse folder",()=>{using(var d=new FolderBrowserDialog()){if(d.ShowDialog(this)==DialogResult.OK)folder.Text=d.SelectedPath;}},false);
        }
    }
    public sealed class TransferOptionsDialog : Sheet {
        public TransferOptionsDialog(Manager manager,Download job) : base("Download properties • UDM",760,650) {
            var url=Input("URL",job.Url,72);var name=Input("File name",job.FileName,40);var folder=Input("Save in",job.Folder,40);
            var queue=Choice("Queue",manager.State.Queues.Select(q=>q.Name),job.Queue);
            var connections=Number("Connections",job.Connections,1,16);var limit=Number("Limit (KB/s; 0 = off)",job.LimitKbps,0,1000000);
            var hash=Input("Expected SHA-256",job.ExpectedSha256,40);var actual=Input("Actual SHA-256",job.Sha256,65);actual.ReadOnly=true;
            var info=Input("Transfer details",string.Format("{0} {1}\r\nResume: {2}\r\n{3}\r\n{4}",job.Status,job.FormatDescription??"",job.RangeSupported?"Validated byte ranges":"Full restart required",job.TimingSummary,job.Error),140);info.ReadOnly=true;
            Note(job.SourceUrl==null?"To refresh an expired download link, pause the download and replace the URL. UDM checks the remote file before reusing partial data.":"For fresh playback links, reopen the video in your browser and choose its quality in UDM's panel.");
            if(job.SourceUrl!=null)url.ReadOnly=true;
            ActionButton("Save",()=>{
                if(manager.IsActive(job))throw new InvalidOperationException("Pause this download before changing its properties.");
                if(job.Status=="Complete")throw new InvalidOperationException("Completed downloads are read-only.");
                string newUrl=Names.Url(url.Text).AbsoluteUri,newFolder=Path.GetFullPath(folder.Text),newName=Names.Safe(name.Text);
                if(hash.Text.Length>0&&!System.Text.RegularExpressions.Regex.IsMatch(hash.Text.Trim(),"^[a-fA-F0-9]{64}$"))throw new ArgumentException("Invalid SHA-256.");
                lock(manager.Sync) {if(manager.State.Downloads.Any(d=>d.Id!=job.Id && d.Target.Equals(Path.Combine(newFolder,newName),StringComparison.OrdinalIgnoreCase)))throw new ArgumentException("Another download uses that destination.");
                    if(new Uri(job.Url).GetLeftPart(UriPartial.Authority)!=new Uri(newUrl).GetLeftPart(UriPartial.Authority))job.ProtectedHeaders="";
                    job.Url=newUrl;job.FileName=newName;job.Folder=newFolder;job.Category=Names.Category(newName);job.Queue=queue.Text;job.Connections=(int)connections.Value;job.LimitKbps=(long)limit.Value;job.ExpectedSha256=hash.Text.Trim();manager.Save();}
                Close();
            },true);ActionButton("Close",Close,false);
        }
    }
    public sealed class SettingsDialog : Form {
        public SettingsDialog(Manager manager) {
            ClassicLayout.Form(this,"UDM Download Manager Configuration",316,301);MinimizeBox=false;Tag="light";
            var s=manager.State.Settings;var tabs=ClassicLayout.Place(this,new TabControl{Multiline=true},5,5,306,270);
            var general=new TabPage("General");var types=new TabPage("File types");var save=new TabPage("Save to");var downloads=new TabPage("Downloads");var connection=new TabPage("Connection");var proxyPage=new TabPage("Proxy");var logins=new TabPage("Sites Logins");var sounds=new TabPage("Sounds");tabs.TabPages.AddRange(new[]{general,types,save,downloads,connection,proxyPage,logins,sounds});
            Header(types,"Browser download capture","browser");Header(logins,"Saved site authentication","link");
            ClassicLayout.Label(types,"Automatically capture these file types when browser capture is enabled:",7,37,280,28);var captureTypes=ClassicLayout.Place(types,new TextBox{Multiline=true,Text=s.CaptureExtensions},7,68,280,53);
            ClassicLayout.Label(types,"Do not capture downloads from these sites:",7,134,280,22);var excludedHosts=ClassicLayout.Place(types,new TextBox{Multiline=true,Text=s.CaptureExcludedHosts},7,160,280,49);ClassicLayout.Label(types,"Separate extensions and host names with spaces. The extension's automatic capture switch must also be on.",7,218,280,27);
            var loginList=ClassicLayout.Place(logins,new ListView{View=View.Details,FullRowSelect=true,MultiSelect=false,HideSelection=false},7,34,280,169);loginList.Columns.Add("Site",ClassicLayout.X(168));loginList.Columns.Add("User name",ClassicLayout.X(103));Action reloadLogins=()=>{loginList.Items.Clear();foreach(var item in s.SiteLogins){var row=new ListViewItem(item.Origin){Tag=item};row.SubItems.Add(item.UserName);loginList.Items.Add(row);}};reloadLogins();
            ClassicLayout.Button(logins,"New",7,214,60,()=>{using(var d=new SiteLoginDialog(manager,null))d.ShowDialog(this);reloadLogins();});ClassicLayout.Button(logins,"Edit",76,214,60,()=>{if(loginList.SelectedItems.Count==0)return;using(var d=new SiteLoginDialog(manager,(SiteLogin)loginList.SelectedItems[0].Tag))d.ShowDialog(this);reloadLogins();});ClassicLayout.Button(logins,"Delete",145,214,60,()=>{if(loginList.SelectedItems.Count==0)return;manager.RemoveSiteLogin((SiteLogin)loginList.SelectedItems[0].Tag);reloadLogins();});
            Header(general,"Browser/System Integration","browser");Header(save,"Categories, file types, folders","folder");Header(downloads,"Default download settings","settings");Header(connection,"Connections and Limits","link");Header(proxyPage,"Proxy server configuration","browser");Header(sounds,"Sounds","music");
            var clipboard=ClassicLayout.Check(general,"Offer URLs placed on the clipboard",13,46,278,s.ClipboardMonitor);var tray=ClassicLayout.Check(general,"Close UDM to the notification area",13,64,278,s.CloseToTray);
            var browsers=ClassicLayout.Place(general,new GroupBox{Text="Browser integration"},7,85,280,122);ClassicLayout.Label(browsers,"Chrome / Edge: UDM Browser Integration\r\nFirefox: UDM Browser Integration",10,16,254,34);ClassicLayout.Label(browsers,"Use the browser extension to capture downloads and show a download panel on supported video pages.",10,54,251,34);
            ClassicLayout.Button(browsers,"Setup instructions",145,97,124,()=>MessageBox.Show(this,"Load browser/chromium as an unpacked extension, then run browser/register-host.ps1 with its displayed extension ID.\n\nAutomatic browser capture is configured in the extension popup. See browser/README.md.","Browser integration"));
            var saveGroup=ClassicLayout.Place(save,new GroupBox{Text="Save To..."},7,29,280,140);ClassicLayout.Label(saveGroup,"Default download directory",7,17,262);var folder=ClassicLayout.Text(saveGroup,s.DownloadFolder,7,33,198);ClassicLayout.Button(saveGroup,"Browse",214,33,57,()=>{using(var d=new FolderBrowserDialog{SelectedPath=folder.Text})if(d.ShowDialog(this)==DialogResult.OK)folder.Text=d.SelectedPath;});
            var categories=ClassicLayout.Check(saveGroup,"Organize files into category folders",7,56,263,s.CategoryFolders);var categoryList=ClassicLayout.Place(saveGroup,new ComboBox{DropDownStyle=ComboBoxStyle.DropDownList},7,81,198,14);categoryList.Items.AddRange(manager.Categories.Select(c=>(object)ClassicLayout.CategoryLabel(c)).ToArray());categoryList.SelectedIndex=0;ClassicLayout.Button(saveGroup,"New",214,80,57,()=>{using(var d=new CategoryNameDialog(manager))if(d.ShowDialog(this)==DialogResult.OK)categoryList.Items.Add(d.CategoryName);});ClassicLayout.Button(saveGroup,"File type rules...",7,107,105,()=>{string selected=manager.Categories[categoryList.SelectedIndex];using(var d=new CategoryRuleDialog(manager,selected))d.ShowDialog(this);});
            var temp=ClassicLayout.Place(save,new GroupBox{Text="Temporary directory"},7,179,280,66);ClassicLayout.Text(temp,manager.DataRoot,7,14,264,true);ClassicLayout.Label(temp,"UDM keeps resumable file parts in this data directory.",7,35,264,23);
            var fileInfo=ClassicLayout.Check(downloads,"Show start download dialog",9,44,280,!s.SkipBrowserFileInfo);var progressDialog=ClassicLayout.Check(downloads,"Show download progress dialog",9,61,280,!s.SuppressProgressDialog);var completionDialog=ClassicLayout.Check(downloads,"Show download complete dialog",9,78,280,!s.SuppressCompletionDialog);ClassicLayout.Line(downloads,7,100,280);
            ClassicLayout.Label(downloads,"Virus checking program",7,112,278);var scan=ClassicLayout.Text(downloads,s.ScanProgram,7,128,280);ClassicLayout.Label(downloads,"Arguments ({file} is the downloaded file)",7,150,280);var arguments=ClassicLayout.Text(downloads,s.ScanArguments,7,165,280);ClassicLayout.Label(downloads,"Downloaded programs are opened only when you choose Open.",7,190,280,24);
            var group=ClassicLayout.Place(connection,new GroupBox{Text="Max. connections number"},7,30,280,83);ClassicLayout.Label(group,"Default max. conn. number",13,15,174);var connections=Number(group,s.Connections,1,16,197,12,65);ClassicLayout.Label(group,"Simultaneous downloads",13,38,174);var parallel=Number(group,s.Parallel,1,16,197,35,65);ClassicLayout.Label(group,"Retries per file piece",13,60,174);var retries=Number(group,s.Retries,0,10,197,57,65);
            var limits=ClassicLayout.Place(connection,new GroupBox{Text="Download limits"},7,130,280,99);ClassicLayout.Label(limits,"Maximum KBytes/sec (0 = unlimited)",10,19,193);var limit=Number(limits,s.LimitKbps,0,1000000,202,16,66);ClassicLayout.Label(limits,"MBytes per hour (0 = unlimited)",10,48,193);var quota=Number(limits,s.QuotaMb,0,10000000,202,45,66);
            ClassicLayout.Label(proxyPage,"HTTP proxy URL (blank uses Windows settings)",7,40,280);var proxy=ClassicLayout.Text(proxyPage,s.Proxy,7,57,280);ClassicLayout.Label(proxyPage,"User name",7,83,90);var user=ClassicLayout.Text(proxyPage,s.ProxyUser,98,81,189);ClassicLayout.Label(proxyPage,"Password",7,107,90);var secret=ClassicLayout.Text(proxyPage,Secrets.Reveal(s.ProxySecret),98,105,189);secret.UseSystemPasswordChar=true;
            ClassicLayout.Label(proxyPage,"Supported here: HTTP/HTTPS proxy configuration. SOCKS and per-protocol proxy rules are not implemented.",7,140,280,40);
            var sound=ClassicLayout.Check(sounds,"Play a sound when a download finishes",13,43,278,s.Sound);ClassicLayout.Button(sounds,"Play",218,69,65,()=>System.Media.SystemSounds.Asterisk.Play());
            AcceptButton=ClassicLayout.Button(this,"OK",190,282,55,()=>{
                Manager.ValidateCaptureRules(captureTypes.Text,excludedHosts.Text);string target=Path.GetFullPath(folder.Text);if(!string.IsNullOrWhiteSpace(proxy.Text)){var u=Names.Url(proxy.Text);if(u.Scheme!="http"&&u.Scheme!="https")throw new ArgumentException("Use an HTTP proxy URL.");}
                if(!string.IsNullOrWhiteSpace(scan.Text)&&(!Path.IsPathRooted(scan.Text)||!File.Exists(scan.Text)))throw new ArgumentException("Select an existing scanner executable.");
                lock(manager.Sync){s.CaptureExtensions=string.Join(" ",Manager.Words(captureTypes.Text));s.CaptureExcludedHosts=string.Join(" ",Manager.Words(excludedHosts.Text));s.DownloadFolder=target;s.Parallel=(int)parallel.Value;s.Connections=(int)connections.Value;s.Retries=(int)retries.Value;s.LimitKbps=(long)limit.Value;s.QuotaMb=(long)quota.Value;s.CategoryFolders=categories.Checked;s.ClipboardMonitor=clipboard.Checked;s.CloseToTray=tray.Checked;s.Sound=sound.Checked;s.SkipBrowserFileInfo=!fileInfo.Checked;s.SuppressProgressDialog=!progressDialog.Checked;s.SuppressCompletionDialog=!completionDialog.Checked;s.Proxy=proxy.Text.Trim();s.ProxyUser=user.Text;s.ProxySecret=Secrets.Protect(secret.Text);s.ScanProgram=scan.Text;s.ScanArguments=arguments.Text;manager.Save();}Close();
            });CancelButton=ClassicLayout.Button(this,"Cancel",251,282,55,Close);
        }
        static NumericUpDown Number(Control p,decimal value,decimal min,decimal max,int x,int y,int width){return ClassicLayout.Place(p,new NumericUpDown{Minimum=min,Maximum=max,Value=Math.Max(min,Math.Min(max,value))},x,y,width,14);}
        static void Header(Control p,string title,string icon){ClassicLayout.Place(p,new PictureBox{Image=Icons.Get(icon,28),SizeMode=PictureBoxSizeMode.Zoom},7,4,20,20);ClassicLayout.Label(p,title,39,7,250).TextAlign=ContentAlignment.TopRight;ClassicLayout.Line(p,35,20,253);}
    }
    public sealed class QueueDialog : Sheet {
        public QueueDialog(Manager manager,QueueRule existing) : base("Queue & scheduler • UDM",670,550) {
            var q=existing ?? new QueueRule{Name="New queue"};
            var name=Input("Queue name",q.Name,40);if(existing!=null)name.ReadOnly=true;
            var enabled=Check("Enable this queue",q.Enabled);var parallel=Number("Active downloads",q.Parallel,1,16);var scheduled=Check("Restrict to a daily time window",q.Scheduled);
            var start=new DateTimePicker{Format=DateTimePickerFormat.Custom,CustomFormat="HH:mm",ShowUpDown=true,Value=DateTime.Today.AddMinutes(q.StartMinute)};Row("Start (local time)",start,40);
            var end=new DateTimePicker{Format=DateTimePickerFormat.Custom,CustomFormat="HH:mm",ShowUpDown=true,Value=DateTime.Today.AddMinutes(q.StopMinute==1440?0:q.StopMinute)};Row("Stop (local time)",end,40);
            var days=new CheckedListBox{CheckOnClick=true,MultiColumn=true};string[] labels={"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};for(int i=0;i<7;i++)days.Items.Add(labels[i],(q.Days&(1<<i))!=0);Row("Days",days,85);
            Note("An overnight window belongs to its starting day. Equal start and stop times mean all day. Closing the window pauses active queue downloads until its next opening.");
            ActionButton("Save queue",()=>{string n=name.Text.Trim();if(n.Length==0)throw new ArgumentException("Name this queue.");int mask=0;foreach(int i in days.CheckedIndices)mask|=1<<i;if(scheduled.Checked&&mask==0)throw new ArgumentException("Select at least one day.");lock(manager.Sync){if(existing==null&&manager.State.Queues.Any(x=>x.Name==n))throw new ArgumentException("That queue already exists.");q.Name=n;q.Enabled=enabled.Checked;q.Parallel=(int)parallel.Value;q.Scheduled=scheduled.Checked;q.StartMinute=start.Value.Hour*60+start.Value.Minute;q.StopMinute=end.Value.Hour*60+end.Value.Minute;q.Days=mask;if(existing==null)manager.State.Queues.Add(q);manager.Save();}Close();},true);ActionButton("Cancel",Close,false);
        }
    }
    public sealed class LegacyGrabDialog : Sheet {
        readonly CancellationTokenSource cancel=new CancellationTokenSource();
        public LegacyGrabDialog(Manager manager) : base("Site grabber • UDM",850,740) {
            var url=Input("Start page","https://",40);var filter=Input("File extensions","zip pdf jpg png mp4 mp3",40);var depth=Number("Link depth",1,0,5);var max=Number("Maximum pages",20,1,100);
            var status=new Label{Text="Explore a site, select files, then add them to your downloads.",ForeColor=Color.FromArgb(85,98,116)};Row("",status,52);
            var results=new CheckedListBox{CheckOnClick=true,HorizontalScrollbar=true,Font=new Font("Segoe UI",9)};Row("Discovered files",results,300);
            Note("Explores links on the same origin with bounded requests. This version reads static HTML; it does not execute JavaScript or mirror pages for offline use.");
            var explore=ActionButton("Explore",()=>{},true);
            explore.Click+=async delegate {
                if(explore.Tag!=null)return;explore.Tag=true;explore.Enabled=false;
                try {results.Items.Clear();var found=await Grabber.Explore(url.Text,(int)depth.Value,(int)max.Value,filter.Text,new Progress<string>(s=>status.Text=s),cancel.Token);foreach(var r in found)results.Items.Add(r.Url,true);status.Text=found.Count+" matching files found.";}
                catch(OperationCanceledException){}catch(Exception ex){status.Text=ex.Message;}finally{if(!IsDisposed){explore.Enabled=true;explore.Tag=null;}}
            };
            ActionButton("Add selected",()=>{foreach(string link in results.CheckedItems)manager.Add(link,null,null,"Main queue",false,null,"",null);status.Text=results.CheckedItems.Count+" files added to UDM.";},false);
            ActionButton("Close",Close,false);FormClosing+=delegate{cancel.Cancel();};
        }
    }
}
