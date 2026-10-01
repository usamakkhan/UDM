using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Threading;
using System.Threading.Tasks;

namespace Udm {
    static class SabrTests {
        sealed class Transcript:HttpMessageHandler {
            readonly byte[] response;public byte[] Request;
            public Transcript(byte[] bytes){response=bytes;}
            protected override async Task<HttpResponseMessage> SendAsync(HttpRequestMessage request,CancellationToken ct){Request=await request.Content.ReadAsByteArrayAsync();var result=new HttpResponseMessage(HttpStatusCode.OK){Content=new ByteArrayContent(response)};result.Content.Headers.ContentType=new System.Net.Http.Headers.MediaTypeHeaderValue("application/vnd.yt-ump");return result;}
        }
        static byte[] Integer(uint n){if(n<128)return new[]{(byte)n};if(n<16384)return new[]{(byte)((n&63)|128),(byte)(n>>6)};if(n<2097152)return new[]{(byte)((n&31)|192),(byte)(n>>5),(byte)(n>>13)};if(n<268435456)return new[]{(byte)((n&15)|224),(byte)(n>>4),(byte)(n>>12),(byte)(n>>20)};return new[]{(byte)240}.Concat(BitConverter.GetBytes(n)).ToArray();}
        static byte[] Part(uint type,byte[] bytes){return Integer(type).Concat(Integer((uint)bytes.Length)).Concat(bytes).ToArray();}
        static SabrOffer Offer(){return new SabrOffer{url="https://rr1.googlevideo.com/videoplayback?sabr=1",videoId="Q3TI27IN7X0",capturedAt=(DateTime.UtcNow-new DateTime(1970,1,1,0,0,0,DateTimeKind.Utc)).TotalMilliseconds,durationMs=1000,body=Convert.ToBase64String(new Proto().Set(5,"config").Set(19,new Proto().Set(1,new Proto().Set(16,1UL).Encode()).Set(2,"test-proof").Encode()).Set(1000,"preserved").Encode()),video=new SabrFormat{id="137",lastModified="12345",mime="video/mp4"},audio=new SabrFormat{id="140",lastModified="12346",mime="audio/mp4"}};}
        static byte[] Response(SabrOffer offer,string videoId,bool gap=false,bool shortSegment=false){var bytes=new List<byte>();int id=0;foreach(var format in new[]{offer.video,offer.audio}){byte[] identity=format.Encode();bytes.AddRange(Part(42,new Proto().Set(1,videoId).Set(2,identity).Set(3,1000UL).Set(4,1UL).Set(5,format.mime).Encode()));for(int i=0;i<2;i++){id++;var h=new Proto().Set(1,(ulong)id).Set(2,videoId).Set(13,identity).Set(14,3UL).Set(8,(ulong)(i==0?1:0)).Set(9,(ulong)i).Set(11,(ulong)(gap&&i==1?500:0)).Set(12,(ulong)(i==0?0:1000));bytes.AddRange(Part(20,h.Encode()));bytes.AddRange(Part(21,new[]{(byte)id,(byte)(i+1),(byte)2,(byte)3}.Take(shortSegment?3:4).ToArray()));bytes.AddRange(Part(22,new[]{(byte)id}));}}return bytes.ToArray();}
        public static async Task Run(string root,Action<bool,string> check){
            foreach(uint n in new uint[]{0,127,128,16383,16384,2097151,2097152,268435455,268435456,uint.MaxValue})if(Ump.Decode(Integer(n))!=n)throw new Exception("UMP integer boundary failed");check(true,"UMP framing handles all five integer widths and boundaries");
            var proto=new Proto().Set(1,ulong.MaxValue).Set(19,"opaque").Float(35,1).Set(1000,"unknown");var parsed=Proto.Parse(proto.Encode());parsed.Set(19,"changed");check(parsed.Number(1)==ulong.MaxValue&&Proto.Parse(parsed.Encode()).Text(1000)=="unknown","protobuf request edits preserve unsigned identities and unknown fields");
            bool truncated=false;try{await Ump.Read(new MemoryStream(new byte[]{20,10,1}),CancellationToken.None);}catch(EndOfStreamException){truncated=true;}check(truncated,"truncated UMP payloads fail before media assembly");
            using(var manager=new Manager(Path.Combine(root,"sabr"),false)){
                var offer=Offer();manager.State.Settings.DownloadFolder=Path.Combine(root,"sabr-output");var job=manager.AddSabr("https://www.youtube.com/watch?v="+offer.videoId,"Streaming test",1080,1080,offer,null);
                check(!File.ReadAllText(Path.Combine(manager.DataRoot,"state.json")).Contains("test-proof")&&Secrets.Reveal(job.ProtectedSabr).Contains(offer.body),"browser streaming context is encrypted at rest");
                var transcript=new Transcript(Response(offer,offer.videoId));await new SabrTransfer(manager,job,offer).Run(CancellationToken.None,transcript);check(File.ReadAllBytes(job.Video.Target).SequenceEqual(new byte[]{1,2,3,2,2,3})&&File.ReadAllBytes(job.Audio.Target).SequenceEqual(new byte[]{1,2,3,2,2,3}),"SABR assembles verified initialization and media segments into separate tracks");
                var request=Proto.Parse(transcript.Request);check(request.Fields.Count(f=>f.Id==16)==1&&request.Fields.Count(f=>f.Id==17)==1&&request.Number(4)==0&&request.Text(1000)=="preserved","streaming requests use the chosen audio/video identities and start from the beginning");
                foreach(string failure in new[]{"other-video","gap","short","encrypted"}){var another=manager.AddSabr(job.SourceUrl,failure,1080,1080,offer,null);byte[] response=failure=="encrypted"?Part(12,new byte[]{1}):Response(offer,failure=="other-video"?"aaaaaaaaaaa":offer.videoId,failure=="gap",failure=="short");bool rejected=false;try{await new SabrTransfer(manager,another,offer).Run(CancellationToken.None,new Transcript(response));}catch(IOException){rejected=true;}check(rejected&&!File.Exists(another.Video.Target)&&!File.Exists(another.Target),"SABR rejects "+failure+" before publishing media");}
                var limited=manager.AddSabr(job.SourceUrl,"Quota",1080,1080,offer,null);var big=new List<byte>();big.AddRange(Part(42,new Proto().Set(1,offer.videoId).Set(2,offer.video.Encode()).Set(3,1000UL).Set(5,"video/mp4").Encode()));big.AddRange(Part(20,new Proto().Set(1,1UL).Set(2,offer.videoId).Set(13,offer.video.Encode()).Set(14,2097152UL).Set(8,1UL).Encode()));var payload=new byte[2097153];payload[0]=1;big.AddRange(Part(21,payload));manager.State.Settings.QuotaMb=1;manager.State.QuotaBytes=0;bool cancelled=false;using(var stop=new CancellationTokenSource(2000))try{await new SabrTransfer(manager,limited,offer).Run(stop.Token,new Transcript(big.ToArray()));}catch(OperationCanceledException){cancelled=true;}check(cancelled&&limited.TransferredBytes==1048576&&!File.Exists(limited.Target),"large SABR frames progress to the quota and cancel without publishing partial output");manager.State.Settings.QuotaMb=0;
                offer.capturedAt=1;bool expired=false;try{manager.AddSabr(job.SourceUrl,"Expired",1080,1080,offer,null);}catch(ArgumentException){expired=true;}check(expired,"expired browser sessions cannot create new streaming jobs");
            }
        }
    }
}
