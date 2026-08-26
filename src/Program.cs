using System;
using System.IO;
using System.Linq;
using System.Net;
using System.Threading;
using System.Windows.Forms;

namespace Udm {
    public static class Program {
        [System.Runtime.InteropServices.DllImport("user32.dll")]static extern bool SetProcessDPIAware();
        [STAThread] public static int Main(string[] args) {
            SetProcessDPIAware();
            ServicePointManager.SecurityProtocol=SecurityProtocolType.Tls12;
            ServicePointManager.DefaultConnectionLimit=64;
            ServicePointManager.Expect100Continue=false;
            Application.EnableVisualStyles();Application.SetCompatibleTextRenderingDefault(false);
            Application.ThreadException+=delegate(object sender,ThreadExceptionEventArgs e){MessageBox.Show(e.Exception.Message,"UDM",MessageBoxButtons.OK,MessageBoxIcon.Error);};
            bool owns;
            using(var mutex=new Mutex(true,"Local\\"+Wire.PipeName,out owns)) {
                try {
                    string url=Option(args,"--add")??Option(args,"/d");
                    if(args.Length==1&&args[0].StartsWith("udm://",StringComparison.OrdinalIgnoreCase)) {var uri=new Uri(args[0]);var values=System.Web.HttpUtility.ParseQueryString(uri.Query);url=values["url"];}
                    if(!owns){Wire.Send(new AddMessage{action=url==null?"show":"add",url=url},5000).GetAwaiter().GetResult();return 0;}
                    string root=Option(args,"--data-dir")??Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"UDM");
                    using(var manager=new Manager(root,true))using(var pipe=new PipeServer(manager)) {
                        if(url!=null)manager.Add(url,Option(args,"--folder")??Option(args,"/p"),Option(args,"--name")??Option(args,"/f"),"Main queue",args.Contains("--paused")||args.Contains("/n"),null,"",null);
                        Application.Run(new MainWindow(manager,pipe,args.Contains("--background")));
                    }
                    return 0;
                } catch(Exception ex){MessageBox.Show(ex.Message,"UDM could not start",MessageBoxButtons.OK,MessageBoxIcon.Error);return 1;}
                finally{if(owns)mutex.ReleaseMutex();}
            }
        }
        static string Option(string[] args,string key){int i=Array.IndexOf(args,key);return i>=0&&i+1<args.Length?args[i+1]:null;}
    }
}
