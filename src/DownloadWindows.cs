using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace Udm {
    // Original controls following the observed file-info -> progress -> completion workflow.
    public static class DownloadUi {
        public static Button Button(Control parent,string text,int x,int y,int width,Action action) {
            var b=new Button{Text=text,Location=new Point(x,y),Size=new Size(width,28)};
            b.Click+=delegate{try{action();}catch(Exception ex){MessageBox.Show(parent,ex.Message,"UDM",MessageBoxButtons.OK,MessageBoxIcon.Warning);}};
            parent.Controls.Add(b);return b;
        }
        public static Label Label(Control parent,string text,int x,int y,int width) {
            var l=new Label{Text=text,Location=new Point(x,y),Size=new Size(width,22),AutoEllipsis=true};parent.Controls.Add(l);return l;
        }
        public static TextBox Text(Control parent,string value,int x,int y,int width,bool readOnly) {
            var t=new TextBox{Text=value,Location=new Point(x,y),Width=width,ReadOnly=readOnly};parent.Controls.Add(t);return t;
        }
        public static void OpenFile(Download job){if(!File.Exists(job.Target))throw new IOException("The downloaded file was moved or deleted.");Process.Start(new ProcessStartInfo(job.Target){UseShellExecute=true});}
        public static void OpenFolder(Download job){if(File.Exists(job.Target))Process.Start("explorer.exe","/select,"+MediaTransfer.Quote(job.Target));else Process.Start(new ProcessStartInfo(job.Folder){UseShellExecute=true});}
        [StructLayout(LayoutKind.Sequential,CharSet=CharSet.Unicode)]struct OpenAsInfo{[MarshalAs(UnmanagedType.LPWStr)]public string File;[MarshalAs(UnmanagedType.LPWStr)]public string Class;public uint Flags;}
        [DllImport("shell32.dll",CharSet=CharSet.Unicode)]static extern int SHOpenWithDialog(IntPtr owner,ref OpenAsInfo info);
        public static void OpenWith(Form owner,Download job){if(!File.Exists(job.Target))throw new IOException("The downloaded file was moved or deleted.");var info=new OpenAsInfo{File=job.Target,Flags=4};int hr=SHOpenWithDialog(owner.Handle,ref info);if(hr<0&&hr!=unchecked((int)0x800704c7))Marshal.ThrowExceptionForHR(hr);}
    }

    public sealed class DownloadInfoDialog:Form {
        readonly Manager manager;readonly Download job;bool decided;
        public DownloadInfoDialog(Manager owner,Download download) {
            manager=owner;job=download;ClassicLayout.Form(this,"Download File Info",380,128);MinimizeBox=false;
            ClassicLayout.Label(this,"URL",5,5,54).TextAlign=ContentAlignment.TopRight;var url=ClassicLayout.Text(this,job.SourceUrl??job.Url,64,3,244,true);url.AccessibleName="Download URL";
            ClassicLayout.Label(this,"Category",5,24,54).TextAlign=ContentAlignment.TopRight;var category=ClassicLayout.Place(this,new ComboBox{DropDownStyle=ComboBoxStyle.DropDownList},64,22,116,14);
            Action fillCategories=()=>{category.Items.Clear();category.Items.AddRange(manager.Categories.Select(c=>(object)ClassicLayout.CategoryLabel(c)).ToArray());};fillCategories();category.SelectedItem=ClassicLayout.CategoryLabel(job.Category);
            Func<string> selectedCategory=()=>manager.Categories.First(c=>ClassicLayout.CategoryLabel(c)==category.Text);
            ClassicLayout.Button(this,"+",183,21,16,()=>{using(var d=new CategoryNameDialog(manager))if(d.ShowDialog(this)==DialogResult.OK){fillCategories();category.SelectedItem=ClassicLayout.CategoryLabel(d.CategoryName);}});
            ClassicLayout.Label(this,"Save As",5,41,54).TextAlign=ContentAlignment.TopRight;var destination=ClassicLayout.Text(this,job.Target,64,39,223);destination.AccessibleName="Save as";
            ClassicLayout.Button(this,"...",292,39,16,()=>{using(var d=new SaveFileDialog{FileName=Path.GetFileName(destination.Text),InitialDirectory=Path.GetDirectoryName(destination.Text),Filter="All files|*.*",OverwritePrompt=true})if(d.ShowDialog(this)==DialogResult.OK)destination.Text=d.FileName;});
            var folderBox=ClassicLayout.Place(this,new GroupBox(),64,55,244,33);var remember=ClassicLayout.Check(this,"Remember this path for category",69,54,228,false);var folder=ClassicLayout.Text(folderBox,Path.GetDirectoryName(destination.Text),5,14,232,true);
            destination.TextChanged+=delegate{try{folder.Text=Path.GetDirectoryName(destination.Text);}catch(ArgumentException){folder.Text="";}};
            category.SelectedIndexChanged+=delegate{remember.Text="Remember this path for \""+category.Text+"\" category";string saved;if(manager.State.Settings.CategoryPaths!=null&&manager.State.Settings.CategoryPaths.TryGetValue(selectedCategory(),out saved))destination.Text=Path.Combine(saved,Path.GetFileName(destination.Text));};
            ClassicLayout.Label(this,"Description",5,93,54).TextAlign=ContentAlignment.TopRight;var description=ClassicLayout.Text(this,job.Description??"",64,91,244);description.AccessibleName="Description";
            ClassicLayout.Place(this,new PictureBox{Image=Icons.Get(job.Category.ToLowerInvariant(),28),SizeMode=PictureBoxSizeMode.Zoom},333,28,20,20);
            ClassicLayout.Label(this,job.MediaHeight>0?job.MediaHeight+"p MP4":Names.Bytes(job.Size),311,57,64,21).TextAlign=ContentAlignment.TopCenter;
            Action<bool,string> save=(begin,queue)=>{manager.ConfigureDestination(job,destination.Text,selectedCategory(),description.Text,queue,remember.Checked);if(begin)manager.Resume(job);else manager.Pause(job);decided=true;Close();};
            var later=ClassicLayout.Button(this,"Download Later",64,110,74,()=>save(false,job.Queue));
            if(manager.State.Queues.Count>1){var queues=new ContextMenuStrip();foreach(var q in manager.State.Queues){string name=q.Name;queues.Items.Add(name,null,delegate{try{save(false,name);}catch(Exception ex){MessageBox.Show(this,ex.Message,"UDM");}});}later.ContextMenuStrip=queues;}
            AcceptButton=ClassicLayout.Button(this,"Start Download",149,110,74,()=>save(true,job.Queue));CancelButton=ClassicLayout.Button(this,"Cancel",234,110,74,()=>{manager.Remove(job);decided=true;Close();});
            FormClosing+=delegate{if(!decided&&manager.State.Downloads.Contains(job))manager.Pause(job);};AppTheme.Apply(this);
        }
    }

    public sealed class SegmentMap:Control {
        readonly Manager manager;readonly Download job;
        public SegmentMap(Manager owner,Download download){manager=owner;job=download;DoubleBuffered=true;AccessibleName="Download progress by file ranges";}
        protected override void OnPaint(PaintEventArgs e){base.OnPaint(e);lock(manager.Sync){var streams=job.Video==null?new[]{job}:new[]{job.Video,job.Audio}.Where(x=>x!=null).ToArray();int rowHeight=Math.Max(14,Height/Math.Max(1,streams.Length));
            for(int row=0;row<streams.Length;row++){var stream=streams[row];var rect=new Rectangle(1,row*rowHeight+2,Math.Max(1,Width-3),Math.Max(6,rowHeight-5));e.Graphics.FillRectangle(SystemBrushes.ControlLight,rect);if(stream.Size>0)foreach(var s in stream.Segments){int x=rect.X+(int)(rect.Width*(double)s.Start/stream.Size);int w=(int)Math.Ceiling(rect.Width*(double)s.Done/stream.Size);if(w>0)e.Graphics.FillRectangle(Brushes.SeaGreen,x,rect.Y,Math.Min(w,rect.Right-x),rect.Height);if(s.Start>0)e.Graphics.DrawLine(Pens.White,x,rect.Y,x,rect.Bottom);}e.Graphics.DrawRectangle(SystemPens.ControlDark,rect);}
        }}
    }

    public sealed class DownloadProgressDialog:Form {
        readonly Manager manager;readonly Download job;readonly Timer timer;readonly ProgressBar progress;readonly Button pause,detailsButton;readonly Panel detailPanel;readonly ListView connections;readonly SegmentMap map;
        readonly Label status,size,received,rate,remaining,resume,limiterRate,streamInfo;bool expanded=true,closingForCompletion;
        public DownloadProgressDialog(Manager owner,Download download) {
            manager=owner;job=download;ClassicLayout.Form(this,job.FileName+" — UDM",353,250);MinimizeBox=true;
            var tabs=ClassicLayout.Place(this,new TabControl(),7,5,338,101);var statusTab=new TabPage("Download status");var limitTab=new TabPage("Speed Limiter");var optionsTab=new TabPage("Options on completion");tabs.TabPages.AddRange(new[]{statusTab,limitTab,optionsTab});
            var address=ClassicLayout.Label(statusTab,job.SourceUrl??job.Url,7,4,329);address.AccessibleName="Download address";
            ClassicLayout.Label(statusTab,"Status",7,15,40);status=ClassicLayout.Label(statusTab,"",53,15,276);
            string[] names={"File size","Downloaded","Transfer rate","Time left"};var values=new List<Label>();for(int i=0;i<names.Length;i++){ClassicLayout.Label(statusTab,names[i],7,28+10*i,63);values.Add(ClassicLayout.Label(statusTab,"",75,28+10*i,254));}size=values[0];received=values[1];rate=values[2];remaining=values[3];ClassicLayout.Label(statusTab,"Resume capability",7,68,108);resume=ClassicLayout.Label(statusTab,"",127,68,199);
            ClassicLayout.Label(limitTab,"Transfer rate",7,7,63);limiterRate=ClassicLayout.Label(limitTab,"",75,7,202);
            var enabled=ClassicLayout.Check(limitTab,"Use Speed Limiter",7,20,154,job.EffectiveLimitKbps>0);
            ClassicLayout.Label(limitTab,"Maximum download speed:",7,32,170);var maximum=ClassicLayout.Place(limitTab,new NumericUpDown{Minimum=1,Maximum=1000000,Value=Math.Max(1,job.EffectiveLimitKbps>0?job.EffectiveLimitKbps:1024),Enabled=enabled.Checked},7,43,65,14);ClassicLayout.Label(limitTab,"KBytes/sec",78,45,65);
            var remember=ClassicLayout.Place(limitTab,new CheckBox{Text="Remember Speed Limiter settings for this file\r\non download stop/resume",Checked=job.SessionLimitKbps==null},7,59,226,20);
            Action updateLimit=()=>{maximum.Enabled=enabled.Checked;manager.SetLimit(job,enabled.Checked?(long)maximum.Value:0,remember.Checked);};enabled.CheckedChanged+=delegate{Guard(updateLimit);};maximum.ValueChanged+=delegate{Guard(updateLimit);};remember.CheckedChanged+=delegate{Guard(updateLimit);};
            ClassicLayout.Label(optionsTab,"Save To: "+job.Target,7,3,316);var showComplete=ClassicLayout.Check(optionsTab,"Show download complete dialog",7,15,240,!job.SuppressCompletionDialog&&!manager.State.Settings.SuppressCompletionDialog);showComplete.CheckedChanged+=delegate{lock(manager.Sync){job.SuppressCompletionDialog=!showComplete.Checked;manager.Save();}};
            ClassicLayout.Line(optionsTab,7,29,316);ClassicLayout.Label(optionsTab,"Open, Open with, and Open folder are available after completion.",7,36,310,25);
            progress=ClassicLayout.Place(this,new ProgressBar{Maximum=1000,AccessibleName="Download progress"},7,111,338,10);
            detailsButton=ClassicLayout.Button(this,"<< Hide details",15,126,93,()=>{expanded=!expanded;detailPanel.Visible=expanded;ClientSize=new Size(ClassicLayout.X(353),ClassicLayout.Y(expanded?250:146));detailsButton.Text=expanded?"<< Hide details":"Show details >>";});
            pause=ClassicLayout.Button(this,"Pause",203,126,56,()=>{if(manager.IsActive(job)||job.Status=="Queued")manager.Pause(job);else manager.Resume(job);});ClassicLayout.Button(this,"Cancel",277,126,50,Close);
            detailPanel=ClassicLayout.Place(this,new Panel(),7,143,338,104);streamInfo=ClassicLayout.Label(detailPanel,"Start positions and download progress by connections",0,0,338);streamInfo.TextAlign=ContentAlignment.TopCenter;
            map=ClassicLayout.Place(detailPanel,new SegmentMap(manager,job),0,10,338,10);connections=ClassicLayout.Place(detailPanel,new ListView{View=View.Details,FullRowSelect=true,GridLines=true,HideSelection=false},0,25,338,79);connections.Columns.Add("N.",27);connections.Columns.Add("Downloaded",100);connections.Columns.Add("Info",ClassicLayout.X(338)-133);
            timer=new Timer{Interval=250};timer.Tick+=delegate{RefreshProgress();};timer.Start();RefreshProgress();FormClosing+=delegate{timer.Stop();if(!closingForCompletion&&(manager.IsActive(job)||job.Status=="Queued"))manager.Pause(job);};AppTheme.Apply(this);
        }
        void Guard(Action action){try{action();}catch(Exception ex){MessageBox.Show(this,ex.Message,"UDM",MessageBoxButtons.OK,MessageBoxIcon.Warning);}}
        public void CloseForCompletion(){closingForCompletion=true;Close();}
        void RefreshProgress(){if(IsDisposed)return;lock(manager.Sync){
            Text=job.Percent.ToString("0")+"% "+job.FileName+" • UDM";
            status.Text=job.Status=="Merging"?"Mixing audio and video streams into one file":job.Status=="Downloading"?"Receiving data…":job.Status=="Failed"?job.Error:job.Status;
            status.ForeColor=job.Status=="Failed"?Color.IndianRed:AppTheme.Dark?Color.LightGreen:Color.DarkGreen;size.Text=Names.Bytes(job.Size);received.Text=Names.Bytes(job.Received)+" ("+job.Percent.ToString("0.00")+" %)";
            rate.Text=job.Speed>0?Names.Bytes(job.Speed)+"/sec":"—";limiterRate.Text=rate.Text;
            remaining.Text=job.Speed>0&&job.Size>=job.Received?TimeSpan.FromSeconds(Math.Min(31536000,(job.Size-job.Received)/job.Speed)).ToString(@"hh\:mm\:ss"):"—";
            resume.Text=job.Size<0?(manager.IsActive(job)?"Checking…":"Unknown"):job.RangeSupported?"Yes":"No — resuming restarts unsupported streams";
            bool uncertain=(job.Size<0&&manager.IsActive(job))||job.Status=="Merging";progress.Style=uncertain?ProgressBarStyle.Marquee:ProgressBarStyle.Continuous;if(!uncertain)progress.Value=(int)Math.Max(0,Math.Min(1000,job.Percent*10));
            pause.Text=manager.IsActive(job)||job.Status=="Queued"?"Pause":"Resume";pause.Enabled=job.Status!="Complete"&&job.Status!="Pausing";
            var streams=job.Video==null?new[]{job}:new[]{job.Video,job.Audio}.Where(x=>x!=null).ToArray();int active=streams.Sum(x=>(x.Workers??new List<ConnectionProgress>()).Count(w=>w.State=="Receiving data"||w.State=="Connecting"));
            streamInfo.Text="Start positions and download progress by connections ("+active+" active)";
            connections.BeginUpdate();connections.Items.Clear();int number=0;foreach(var stream in streams)foreach(var w in stream.Workers??new List<ConnectionProgress>()){var row=new ListViewItem((++number).ToString());row.SubItems.Add(Names.Bytes(w.Downloaded));row.SubItems.Add((stream==job?"":stream==job.Video?"Video: ":"Audio: ")+w.State+(w.End>=w.Start?" • bytes "+w.Start+"–"+w.End:""));connections.Items.Add(row);}connections.EndUpdate();map.Invalidate();
        }}
        protected override void Dispose(bool disposing){if(disposing&&timer!=null)timer.Dispose();base.Dispose(disposing);}
    }

    public sealed class DownloadCompleteDialog:Form {
        public DownloadCompleteDialog(Manager manager,Download job) {
            ClassicLayout.Form(this,"Download complete",303,132);MinimizeBox=false;
            ClassicLayout.Place(this,new PictureBox{Image=Icons.Get("complete",28),SizeMode=PictureBoxSizeMode.Zoom},7,7,21,20);
            ClassicLayout.Label(this,"Downloaded "+(job.TransferredBytes>0?job.TransferredBytes:job.Size).ToString("N0")+" bytes.",34,7,262,32);
            ClassicLayout.Label(this,"Address",7,38,289);ClassicLayout.Text(this,job.SourceUrl??job.Url,7,48,289,true);ClassicLayout.Label(this,"The file saved as",7,66,289);ClassicLayout.Text(this,job.Target,7,76,289,true);
            ClassicLayout.Button(this,"Open",7,96,61,()=>DownloadUi.OpenFile(job));ClassicLayout.Button(this,"Open with...",76,96,61,()=>DownloadUi.OpenWith(this,job));ClassicLayout.Button(this,"Open folder",145,96,61,()=>DownloadUi.OpenFolder(job));CancelButton=ClassicLayout.Button(this,"Close",235,96,61,Close);
            var hide=ClassicLayout.Check(this,"Don't show this dialog again",16,117,238,false);hide.CheckedChanged+=delegate{lock(manager.Sync){manager.State.Settings.SuppressCompletionDialog=hide.Checked;manager.Save();}};AppTheme.Apply(this);
        }
    }
}
