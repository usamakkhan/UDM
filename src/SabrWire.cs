using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace Udm {
    // Small original wire codec. Unknown protobuf fields survive request updates.
    internal sealed class ProtoField { public int Id, Wire; public ulong Number; public byte[] Bytes; }
    internal sealed class Proto {
        public readonly List<ProtoField> Fields=new List<ProtoField>();
        static ulong Varint(byte[] bytes,ref int offset){ulong value=0;for(int i=0;i<10;i++){if(offset>=bytes.Length)throw new IOException("Truncated streaming metadata.");byte b=bytes[offset++];if(i==9&&b>1)throw new IOException("Streaming integer overflow.");value|=(ulong)(b&127)<<(7*i);if((b&128)==0)return value;}throw new IOException("Invalid streaming integer.");}
        public static Proto Parse(byte[] bytes){if(bytes==null)return new Proto();if(bytes.Length>262144)throw new IOException("Streaming metadata is too large.");var result=new Proto();int offset=0;
            while(offset<bytes.Length){ulong tag=Varint(bytes,ref offset);if(tag>uint.MaxValue||(tag>>3)==0)throw new IOException("Invalid streaming field.");var field=new ProtoField{Id=(int)(tag>>3),Wire=(int)(tag&7)};
                if(field.Wire==0)field.Number=Varint(bytes,ref offset);
                else {int length;if(field.Wire==2){ulong n=Varint(bytes,ref offset);if(n>262144)throw new IOException("Streaming field is too large.");length=(int)n;}else if(field.Wire==1)length=8;else if(field.Wire==5)length=4;else throw new IOException("Unsupported streaming wire type.");if(length>bytes.Length-offset)throw new IOException("Truncated streaming field.");field.Bytes=new byte[length];Buffer.BlockCopy(bytes,offset,field.Bytes,0,length);offset+=length;}
                result.Fields.Add(field);if(result.Fields.Count>8192)throw new IOException("Too many streaming fields.");
            }return result;
        }
        static void WriteVarint(Stream stream,ulong value){while(value>127){stream.WriteByte((byte)((value&127)|128));value>>=7;}stream.WriteByte((byte)value);}
        public byte[] Encode(){using(var s=new MemoryStream()){foreach(var f in Fields){WriteVarint(s,(ulong)((f.Id<<3)|f.Wire));if(f.Wire==0)WriteVarint(s,f.Number);else{if(f.Wire==2)WriteVarint(s,(ulong)f.Bytes.Length);s.Write(f.Bytes,0,f.Bytes.Length);}}return s.ToArray();}}
        public ulong Number(int id,ulong fallback=0){var f=Fields.LastOrDefault(x=>x.Id==id&&x.Wire==0);return f==null?fallback:f.Number;}
        public byte[] Bytes(int id){var f=Fields.LastOrDefault(x=>x.Id==id&&x.Wire==2);return f==null?null:f.Bytes;}
        public string Text(int id){return Encoding.UTF8.GetString(Bytes(id)??new byte[0]);}
        public bool Has(int id){return Fields.Any(x=>x.Id==id);}
        public Proto Remove(params int[] ids){Fields.RemoveAll(x=>ids.Contains(x.Id));return this;}
        public Proto Add(int id,byte[] bytes){Fields.Add(new ProtoField{Id=id,Wire=2,Bytes=bytes});return this;}
        public Proto Set(int id,byte[] bytes){Remove(id);return Add(id,bytes);}
        public Proto Set(int id,string value){return Set(id,Encoding.UTF8.GetBytes(value));}
        public Proto Set(int id,ulong value){Remove(id);Fields.Add(new ProtoField{Id=id,Wire=0,Number=value});return this;}
        public Proto Float(int id,float value){Remove(id);Fields.Add(new ProtoField{Id=id,Wire=5,Bytes=BitConverter.GetBytes(value)});return this;}
        public IEnumerable<ulong> Numbers(int id){foreach(var f in Fields.Where(x=>x.Id==id)){if(f.Wire==0)yield return f.Number;else if(f.Wire==2){int i=0;while(i<f.Bytes.Length)yield return Varint(f.Bytes,ref i);}}}
    }
    internal sealed class UmpPart {public uint Type;public byte[] Data;}
    internal static class Ump {
        static async Task<int> Byte(Stream stream,CancellationToken ct){var b=new byte[1];return await stream.ReadAsync(b,0,1,ct).ConfigureAwait(false)==0?-1:b[0];}
        internal static uint Decode(byte[] bytes){int first=bytes[0],count=first<128?1:first<192?2:first<224?3:first<240?4:5;if(bytes.Length!=count)throw new IOException("Invalid UMP integer.");if(count==5)return BitConverter.ToUInt32(bytes,1);int bits=8-count;uint result=(uint)(first&((1<<bits)-1)),multiplier=(uint)(1<<bits);for(int i=1;i<count;i++){result+=(uint)bytes[i]*multiplier;multiplier*=256;}return result;}
        static async Task<uint?> Integer(Stream stream,CancellationToken ct){int first=await Byte(stream,ct).ConfigureAwait(false);if(first<0)return null;int count=first<128?1:first<192?2:first<224?3:first<240?4:5;var bytes=new byte[count];bytes[0]=(byte)first;for(int i=1;i<count;i++){int b=await Byte(stream,ct).ConfigureAwait(false);if(b<0)throw new EndOfStreamException("Truncated UMP integer.");bytes[i]=(byte)b;}return Decode(bytes);}
        public static async Task<UmpPart> Read(Stream stream,CancellationToken ct){var type=await Integer(stream,ct).ConfigureAwait(false);if(!type.HasValue)return null;var length=await Integer(stream,ct).ConfigureAwait(false);if(!length.HasValue||length>32*1024*1024)throw new IOException("Invalid UMP part size.");var data=await Wire.ReadExact(stream,(int)length.Value,ct).ConfigureAwait(false);return new UmpPart{Type=type.Value,Data=data};}
    }
}
