using System;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Windows.Forms;
namespace Udm {
    public static class AppTheme {
        public static bool Dark=true;
        static readonly Color background=Color.FromArgb(56,56,56),input=Color.FromArgb(48,48,48),foreground=Color.FromArgb(236,237,239),line=Color.FromArgb(83,85,88);
        [DllImport("dwmapi.dll")]static extern int DwmSetWindowAttribute(IntPtr hwnd,int attribute,ref int value,int size);
        public static void Apply(Control root){
            if(!Dark || (root.Tag as string)=="light")return;
            root.BackColor=root is TextBoxBase||root is ComboBox||root is NumericUpDown||root is ListView?input:background;root.ForeColor=foreground;
            var button=root as Button;if(button!=null){button.FlatStyle=FlatStyle.Flat;button.FlatAppearance.BorderColor=line;button.BackColor=input;}
            var grid=root as DataGridView;if(grid!=null){grid.BackgroundColor=background;grid.GridColor=line;grid.DefaultCellStyle.BackColor=background;grid.DefaultCellStyle.ForeColor=foreground;grid.DefaultCellStyle.SelectionBackColor=Color.FromArgb(47,76,107);grid.DefaultCellStyle.SelectionForeColor=Color.White;grid.RowsDefaultCellStyle.BackColor=background;grid.RowsDefaultCellStyle.ForeColor=foreground;grid.AlternatingRowsDefaultCellStyle.BackColor=background;grid.AlternatingRowsDefaultCellStyle.ForeColor=foreground;grid.ColumnHeadersDefaultCellStyle.BackColor=Color.FromArgb(32,33,35);grid.ColumnHeadersDefaultCellStyle.ForeColor=foreground;}
            var tree=root as TreeView;if(tree!=null){tree.LineColor=line;tree.BackColor=Color.FromArgb(32,32,32);}
            var tabs=root as TabControl;if(tabs!=null){tabs.DrawMode=TabDrawMode.OwnerDrawFixed;tabs.DrawItem+=delegate(object sender,DrawItemEventArgs e){using(var brush=new SolidBrush(background))e.Graphics.FillRectangle(brush,e.Bounds);TextRenderer.DrawText(e.Graphics,tabs.TabPages[e.Index].Text,tabs.Font,e.Bounds,foreground,TextFormatFlags.HorizontalCenter|TextFormatFlags.VerticalCenter);};}
            var menu=root as MenuStrip;if(menu!=null)menu.BackColor=Color.FromArgb(32,32,32);if(menu!=null)foreach(ToolStripMenuItem item in menu.Items)StyleMenu(item);
            foreach(Control child in root.Controls)Apply(child);
            var form=root as Form;if(form!=null){if(form.IsHandleCreated)DarkTitle(form);else form.HandleCreated+=delegate{DarkTitle(form);};}
        }
        static void StyleMenu(ToolStripMenuItem item){item.BackColor=background;item.ForeColor=foreground;foreach(ToolStripItem child in item.DropDownItems){child.BackColor=background;child.ForeColor=foreground;var nested=child as ToolStripMenuItem;if(nested!=null)StyleMenu(nested);}}
        static void DarkTitle(Form form){try{int value=1;DwmSetWindowAttribute(form.Handle,20,ref value,4);}catch(DllNotFoundException){}catch(EntryPointNotFoundException){}}
    }
}
