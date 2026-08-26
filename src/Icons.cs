using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.IO;

namespace Udm {
    // Original vector geometry, rendered at the requested resolution; no external assets.
    public static class Icons {
        static readonly Dictionary<string,Bitmap> cache=new Dictionary<string,Bitmap>();
        public static readonly string[] Names={"app","add","resume","pause","stop","stopall","startqueue","stopqueue","remove","schedule","grabber","settings","folder","all","queue","complete","failed","archives","documents","programs","music","video","images","other","browser","link","import","export","info"};
        public static Image Get(string name,int size){string key=name+size;lock(cache){Bitmap image;if(!cache.TryGetValue(key,out image)){image=Draw(name,size);cache.Add(key,image);}return image;}}
        public static Color Accent(string name){switch(name){case "add":return Color.FromArgb(125,180,172);case "resume":case "stop":case "stopall":case "remove":return Color.FromArgb(180,183,186);case "complete":case "stopqueue":return Color.FromArgb(187,137,185);case "settings":return Color.FromArgb(166,148,210);case "schedule":return Color.FromArgb(184,148,200);case "startqueue":return Color.FromArgb(134,177,199);case "failed":return Color.FromArgb(218,80,88);case "folder":case "archives":return Color.FromArgb(202,180,103);case "video":case "music":return Color.FromArgb(131,166,202);default:return Color.FromArgb(132,166,196);}}
        public static Bitmap Draw(string name,int size){
            var bitmap=new Bitmap(size,size,PixelFormat.Format32bppArgb);
            using(var g=Graphics.FromImage(bitmap)){
                g.SmoothingMode=SmoothingMode.AntiAlias;g.PixelOffsetMode=PixelOffsetMode.HighQuality;g.ScaleTransform(size/24f,size/24f);g.Clear(Color.Transparent);
                Color color=Accent(name);using(var pen=new Pen(color,1.75f){StartCap=LineCap.Round,EndCap=LineCap.Round,LineJoin=LineJoin.Round})using(var fill=new SolidBrush(color))using(var tint=new SolidBrush(Color.FromArgb(22,color))){
                    switch(name){
                    case "app":
                        using(var bg=new LinearGradientBrush(new RectangleF(0,0,24,24),Color.FromArgb(58,132,250),Color.FromArgb(37,72,197),45))using(var shape=Rounded(new RectangleF(0,0,24,24),5))g.FillPath(bg,shape);
                        using(var white=new Pen(Color.White,2.0f){StartCap=LineCap.Round,EndCap=LineCap.Round,LineJoin=LineJoin.Round}){g.DrawLine(white,12,4.8f,12,15.7f);g.DrawLines(white,new[]{new PointF(7.9f,11.8f),new PointF(12,15.9f),new PointF(16.1f,11.8f)});g.DrawLines(white,new[]{new PointF(6,16),new PointF(6,19),new PointF(18,19),new PointF(18,16)});}break;
                    case "add":g.DrawPolygon(pen,new[]{new PointF(9,3),new PointF(15,3),new PointF(15,9),new PointF(21,9),new PointF(21,15),new PointF(15,15),new PointF(15,21),new PointF(9,21),new PointF(9,15),new PointF(3,15),new PointF(3,9),new PointF(9,9)});break;
                    case "resume":g.DrawEllipse(pen,2,2,20,20);g.DrawLine(pen,12,5,12,18);g.DrawLines(pen,new[]{new PointF(7,12),new PointF(12,18),new PointF(17,12)});break;
                    case "stop":g.DrawEllipse(pen,2,2,20,20);g.DrawRectangle(pen,8,8,8,8);break;
                    case "stopall":g.DrawLines(pen,new[]{new PointF(5,20),new PointF(3,10),new PointF(5,9),new PointF(8,13),new PointF(8,4),new PointF(10,3),new PointF(11,11),new PointF(11,2),new PointF(13,2),new PointF(14,11),new PointF(14,4),new PointF(16,4),new PointF(17,13),new PointF(17,7),new PointF(19,7),new PointF(20,17),new PointF(17,22),new PointF(8,22),new PointF(5,20)});break;
                    case "startqueue":case "stopqueue":g.DrawRectangle(pen,3,3,13,15);g.DrawLines(pen,new[]{new PointF(7,21),new PointF(7,7),new PointF(20,7),new PointF(20,12)});if(name=="startqueue"){g.DrawLine(pen,17,12,17,22);g.DrawLines(pen,new[]{new PointF(13,18),new PointF(17,22),new PointF(21,18)});}else g.DrawRectangle(pen,13,14,8,8);break;
                    case "pause":g.FillEllipse(tint,1,1,22,22);g.FillRectangle(fill,7,6,3,12);g.FillRectangle(fill,14,6,3,12);break;
                    case "remove":g.DrawLines(pen,new[]{new PointF(6,7),new PointF(7,21),new PointF(17,21),new PointF(18,7)});g.DrawLine(pen,4,6,20,6);g.DrawLines(pen,new[]{new PointF(9,6),new PointF(9,3),new PointF(15,3),new PointF(15,6)});g.DrawLine(pen,10,10,10,17);g.DrawLine(pen,14,10,14,17);break;
                    case "schedule":g.FillEllipse(tint,2,2,20,20);g.DrawEllipse(pen,3,3,18,18);g.DrawLines(pen,new[]{new PointF(12,6),new PointF(12,12),new PointF(16,14)});break;
                    case "grabber":case "browser":g.FillEllipse(tint,2,2,20,20);g.DrawEllipse(pen,3,3,18,18);g.DrawEllipse(pen,8,3,8,18);g.DrawLine(pen,3,12,21,12);g.DrawArc(pen,3,5,18,6,0,180);break;
                    case "settings":for(int i=0;i<8;i++){double a=i*Math.PI/4;g.DrawLine(pen,12+(float)Math.Cos(a)*8,12+(float)Math.Sin(a)*8,12+(float)Math.Cos(a)*10,12+(float)Math.Sin(a)*10);}g.FillEllipse(tint,5,5,14,14);g.DrawEllipse(pen,5,5,14,14);g.DrawEllipse(pen,9,9,6,6);break;
                    case "folder":g.FillPolygon(tint,new[]{new PointF(2,6),new PointF(10,6),new PointF(12,9),new PointF(22,9),new PointF(20,20),new PointF(2,20)});g.DrawLines(pen,new[]{new PointF(2,19),new PointF(2,5),new PointF(9,5),new PointF(12,8),new PointF(21,8),new PointF(21,19),new PointF(2,19)});g.DrawLine(pen,2,10,21,10);break;
                    case "all":for(int i=0;i<4;i++){float x=3+(i%2)*10,y=3+(i/2)*10;g.FillRectangle(tint,x,y,7,7);g.DrawRectangle(pen,x,y,7,7);}break;
                    case "queue":for(int i=0;i<3;i++){float y=5+i*7;g.FillEllipse(fill,2,y-1,3,3);g.DrawLine(pen,9,y,21,y);}break;
                    case "complete":g.FillEllipse(tint,2,2,20,20);g.DrawEllipse(pen,3,3,18,18);g.DrawLines(pen,new[]{new PointF(7,12),new PointF(10.5f,15.5f),new PointF(17,8.5f)});break;
                    case "failed":g.FillEllipse(tint,2,2,20,20);g.DrawEllipse(pen,3,3,18,18);g.DrawLine(pen,12,6.5f,12,13);g.FillEllipse(fill,11,16,2,2);break;
                    case "archives":Document(g,pen,tint);for(int i=0;i<4;i++)g.FillRectangle(fill,10+(i%2)*2,4+i*3,2,3);g.DrawRectangle(pen,10,17,4,3);break;
                    case "documents":Document(g,pen,tint);g.DrawLine(pen,8,11,16,11);g.DrawLine(pen,8,15,16,15);g.DrawLine(pen,8,18,13,18);break;
                    case "programs":g.FillRectangle(tint,3,4,18,16);g.DrawRectangle(pen,3,4,18,16);g.DrawLine(pen,3,8,21,8);g.DrawLines(pen,new[]{new PointF(8,12),new PointF(6,14),new PointF(8,16)});g.DrawLines(pen,new[]{new PointF(16,12),new PointF(18,14),new PointF(16,16)});break;
                    case "music":g.DrawLines(pen,new[]{new PointF(9,17),new PointF(9,5),new PointF(20,3),new PointF(20,15)});g.DrawLine(pen,9,9,20,7);g.FillEllipse(fill,3,15,6,5);g.FillEllipse(fill,14,13,6,5);break;
                    case "video":g.FillRectangle(tint,3,5,18,15);g.DrawRectangle(pen,3,5,18,15);g.DrawLine(pen,3,9,21,9);for(int i=0;i<3;i++)g.DrawLine(pen,6+i*5,5,8+i*5,9);g.FillPolygon(fill,new[]{new PointF(10,11),new PointF(16,14.5f),new PointF(10,18)});break;
                    case "images":g.FillRectangle(tint,3,4,18,16);g.DrawRectangle(pen,3,4,18,16);g.DrawEllipse(pen,14,7,3,3);g.DrawLines(pen,new[]{new PointF(4,18),new PointF(9,11),new PointF(13,16),new PointF(16,13),new PointF(20,18)});break;
                    case "link":g.DrawArc(pen,2,8,13,8,80,280);g.DrawArc(pen,9,8,13,8,260,280);g.DrawLine(pen,8,12,16,12);break;
                    case "import":case "export":Document(g,pen,tint);float sign=name=="import"?1:-1;g.DrawLine(pen,12,8,12,17);g.DrawLines(pen,new[]{new PointF(8,12.5f),new PointF(12,12.5f+4*sign),new PointF(16,12.5f)});break;
                    case "info":g.DrawEllipse(pen,3,3,18,18);g.FillEllipse(fill,11,6,2,2);g.DrawLine(pen,12,11,12,17);break;
                    default:Document(g,pen,tint);break;
                    }
                }
            }
            return bitmap;
        }
        static void Document(Graphics g,Pen pen,Brush fill){var points=new[]{new PointF(5,2),new PointF(14,2),new PointF(19,7),new PointF(19,22),new PointF(5,22),new PointF(5,2)};g.FillPolygon(fill,points);g.DrawLines(pen,points);g.DrawLines(pen,new[]{new PointF(14,2),new PointF(14,7),new PointF(19,7)});}
        static GraphicsPath Rounded(RectangleF r,float radius){var p=new GraphicsPath();float d=radius*2;p.AddArc(r.X,r.Y,d,d,180,90);p.AddArc(r.Right-d,r.Y,d,d,270,90);p.AddArc(r.Right-d,r.Bottom-d,d,d,0,90);p.AddArc(r.X,r.Bottom-d,d,d,90,90);p.CloseFigure();return p;}
        public static void Export(string directory){
            Directory.CreateDirectory(directory);
            foreach(string name in Names)using(var image=Draw(name,64))image.Save(Path.Combine(directory,name+".png"),ImageFormat.Png);
            int[] sizes={16,24,32,48,64,128,256};var images=new List<byte[]>();foreach(int size in sizes)using(var image=Draw("app",size))using(var ms=new MemoryStream()){image.Save(ms,ImageFormat.Png);images.Add(ms.ToArray());}
            using(var output=new BinaryWriter(File.Create(Path.Combine(directory,"udm.ico")))){output.Write((ushort)0);output.Write((ushort)1);output.Write((ushort)sizes.Length);int offset=6+16*sizes.Length;for(int i=0;i<sizes.Length;i++){output.Write((byte)(sizes[i]==256?0:sizes[i]));output.Write((byte)(sizes[i]==256?0:sizes[i]));output.Write((byte)0);output.Write((byte)0);output.Write((ushort)1);output.Write((ushort)32);output.Write(images[i].Length);output.Write(offset);offset+=images[i].Length;}foreach(byte[] bytes in images)output.Write(bytes);}
        }
    }
}
