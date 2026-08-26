using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
namespace Udm {
    // Original controls positioned in Windows dialog units from the measured reference layout.
    public static class ClassicLayout {
        public static readonly float DpiScale=ReadDpiScale();
        static float ReadDpiScale(){using(var graphics=Graphics.FromHwnd(IntPtr.Zero))return graphics.DpiX/96f;}
        public static int X(double value){return (int)Math.Round(value*1.4*DpiScale);}
        public static int Y(double value){return (int)Math.Round(value*1.625*DpiScale);}
        public static void Form(Form form,string title,int width,int height){form.Text=title;form.Font=new Font("Tahoma",8.25f);form.AutoScaleMode=AutoScaleMode.None;form.ClientSize=new Size(X(width),Y(height));form.FormBorderStyle=FormBorderStyle.FixedDialog;form.MaximizeBox=false;form.StartPosition=FormStartPosition.CenterScreen;form.Icon=Icon.ExtractAssociatedIcon(Application.ExecutablePath);}
        public static T Place<T>(Control parent,T control,double x,double y,double width,double height) where T:Control{control.SetBounds(X(x),Y(y),X(width),Y(height));parent.Controls.Add(control);return control;}
        public static Label Label(Control parent,string text,double x,double y,double width,double height=10){return Place(parent,new Label{Text=text,AutoEllipsis=true},x,y,width,height);}
        public static TextBox Text(Control parent,string text,double x,double y,double width,bool readOnly=false){return Place(parent,new TextBox{Text=text,ReadOnly=readOnly},x,y,width,14);}
        public static Button Button(Control parent,string text,double x,double y,double width,Action action){var b=Place(parent,new Button{Text=text},x,y,width,15);b.Click+=delegate{try{action();}catch(Exception ex){MessageBox.Show(parent,ex.Message,"UDM",MessageBoxButtons.OK,MessageBoxIcon.Warning);}};return b;}
        public static CheckBox Check(Control parent,string text,double x,double y,double width,bool value){var control=Place(parent,new CheckBox{Text=text,Checked=value},x,y,width,12);control.BringToFront();return control;}
        public static void Line(Control parent,double x,double y,double width){Place(parent,new Label{BorderStyle=BorderStyle.Fixed3D},x,y,width,1);}
        public static string CategoryLabel(string name){return name=="Archives"?"Compressed":name=="Other"?"General":name;}
    }
    public sealed class AddressDialog:Form {
        public AddressDialog(Manager manager,string initial){
            ClassicLayout.Form(this,"Enter new address to download",373,57);MinimizeBox=false;
            ClassicLayout.Label(this,"Address",8,9,35);var address=ClassicLayout.Place(this,new ComboBox{Text=initial??"",DropDownStyle=ComboBoxStyle.DropDown},45,7,264,14);
            address.Items.AddRange(manager.State.Downloads.Select(d=>d.SourceUrl??d.Url).Distinct().Reverse().Take(30).Cast<object>().ToArray());
            var group=ClassicLayout.Place(this,new GroupBox(),7,24,302,29);
            var auth=ClassicLayout.Check(this,"Use authorization",13,23,95,false);
            ClassicLayout.Label(group,"Login",6,12,42);var login=ClassicLayout.Text(group,"",70,10,78);login.Enabled=false;
            ClassicLayout.Label(group,"Password",153,12,64);var password=ClassicLayout.Text(group,"",221,10,75);password.UseSystemPasswordChar=true;password.Enabled=false;
            auth.CheckedChanged+=delegate{login.Enabled=password.Enabled=auth.Checked;};
            AcceptButton=ClassicLayout.Button(this,"OK",316,7,50,()=>{
                var uri=Names.Url(address.Text);if(uri.Host.Equals("youtube.com",StringComparison.OrdinalIgnoreCase)||uri.Host.EndsWith(".youtube.com",StringComparison.OrdinalIgnoreCase)||uri.Host.Equals("youtu.be",StringComparison.OrdinalIgnoreCase))throw new ArgumentException("Open this video in your browser and use its Download with UDM panel to capture the media.");
                var headers=new Dictionary<string,string>();if(auth.Checked)headers["Authorization"]="Basic "+Convert.ToBase64String(Encoding.UTF8.GetBytes(login.Text+":"+password.Text));
                var job=manager.Add(address.Text,null,null,"Main queue",true,headers,"",null);Hide();using(var info=new DownloadInfoDialog(manager,job))info.ShowDialog(Owner);DialogResult=DialogResult.OK;Close();
            });CancelButton=ClassicLayout.Button(this,"Cancel",316,24,50,Close);Shown+=delegate{address.Focus();};AppTheme.Apply(this);
        }
    }
    public sealed class ClassicToolbarButton:Button {
        bool hovered;
        public ClassicToolbarButton(){DoubleBuffered=true;MouseEnter+=delegate{hovered=true;Invalidate();};MouseLeave+=delegate{hovered=false;Invalidate();};}
        protected override void OnPaint(PaintEventArgs e){
            e.Graphics.Clear(hovered&&Enabled?Color.FromArgb(65,75,85):BackColor);
            float scale=e.Graphics.DpiX/96f;int icon=(int)Math.Round(28*scale),top=(int)Math.Round(3*scale);
            if(Image!=null)e.Graphics.DrawImage(Image,new Rectangle((Width-icon)/2,top,icon,icon));
            string label=Text=="Start Queue"?"Start\nQueue":Text=="Stop Queue"?"Stop\nQueue":Text;
            var rect=new Rectangle(0,top+icon+2,Width,Height-top-icon-2);
            TextRenderer.DrawText(e.Graphics,label,Font,rect,Enabled?ForeColor:Color.FromArgb(145,145,145),TextFormatFlags.HorizontalCenter|TextFormatFlags.Top|TextFormatFlags.NoPadding|TextFormatFlags.EndEllipsis);
            if(Focused&&ShowFocusCues)ControlPaint.DrawFocusRectangle(e.Graphics,ClientRectangle);
        }
    }
    public sealed class CategoryNameDialog:Form {
        public string CategoryName;
        public CategoryNameDialog(Manager manager){ClassicLayout.Form(this,"Add category",225,65);MinimizeBox=false;ClassicLayout.Label(this,"Name",7,10,40);var name=ClassicLayout.Text(this,"",50,7,167);AcceptButton=ClassicLayout.Button(this,"OK",108,42,50,()=>{manager.AddCategory(name.Text);CategoryName=name.Text.Trim();DialogResult=DialogResult.OK;Close();});CancelButton=ClassicLayout.Button(this,"Cancel",167,42,50,Close);AppTheme.Apply(this);}
    }
}
