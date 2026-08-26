using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace Udm {
    static class WorkflowTests {
        static bool Rejects(Action action){try{action();return false;}catch(ArgumentException){return true;}catch(InvalidOperationException){return true;}}
        public static void Run(string root,Action<bool,string> check){
            string data=Path.Combine(root,"workflows"),files=Path.Combine(root,"workflow-files"),projectId;
            using(var m=new Manager(data,false)){
                m.State.Settings.DownloadFolder=files;m.State.Settings.CategoryFolders=false;
                var q=m.CreateQueue("Nightly");check(Rejects(()=>m.CreateQueue("nightly")),"queue names are unique regardless of case");
                var first=m.Add("https://example.test/1.zip",files,"1.zip",q.Name,true,null,"",null);var unrelated=m.Add("https://example.test/other.zip",files,"other.zip","Main queue",true,null,"",null);var second=m.Add("https://example.test/2.zip",files,"2.zip",q.Name,true,null,"",null);
                m.MoveWithinQueue(second,-1);check(m.State.Downloads[0]==second&&m.State.Downloads[1]==unrelated&&m.State.Downloads[2]==first,"queue ordering preserves positions of unrelated queues");
                var edit=Json.Read<QueueRule>(Json.Write(q));edit.Enabled=true;edit.RunOnce=true;edit.StartOnceUtc=DateTime.UtcNow.AddHours(1);edit.StopOnceUtc=DateTime.UtcNow.AddHours(2);edit.Retries=7;m.UpdateQueue(q,edit);
                check(!q.InWindow(DateTime.UtcNow)&&q.InWindow(DateTime.UtcNow.AddMinutes(90))&&!q.InWindow(DateTime.UtcNow.AddHours(3))&&m.RetriesFor(q.Name)==7,"dated schedule honors its start, stop and per-queue retry setting");
                first.Status="Awaiting confirmation";m.RunQueue(q,true);check(q.InWindow(DateTime.Now)&&second.Status=="Queued"&&first.Status=="Awaiting confirmation","start queue overrides its schedule without accepting browser confirmation");m.RunQueue(q,false);check(!q.InWindow(DateTime.Now)&&second.Status=="Paused","stop queue pauses waiting files and clears the manual override");
                m.DeleteQueue(q);check(first.Queue=="Main queue"&&second.Queue=="Main queue"&&second.Status=="Paused"&&Rejects(()=>m.DeleteQueue(m.State.Queues[0])),"deleting a custom queue retains its files and protects the main queue");
                var protectedHeaders=new Dictionary<string,string>{{"Cookie","secret"},{"Authorization","Basic private"},{"Referer","https://example.test/private"}};
                m.EditProperties(second,"https://different.test/file.zip",Path.Combine(files,"renamed.zip"),"Edited",protectedHeaders);check(second.FileName=="renamed.zip"&&second.Description=="Edited"&&m.ReadHeaders(second).Count==0,"changing a download's origin does not forward its old credentials");
                m.SaveSiteLogin("https://example.test/path","user","private-password");var withLogin=m.Add("https://example.test/secure.zip",files,null,"Main queue",true,null,"",null);var other=m.Add("https://sub.example.test/secure.zip",files,null,"Main queue",true,null,"",null);
                check(m.ReadHeaders(withLogin).ContainsKey("Authorization")&&!m.ReadHeaders(other).ContainsKey("Authorization")&&!File.ReadAllText(Path.Combine(data,"state.json")).Contains("private-password"),"saved authentication is encrypted and scoped to an exact HTTPS origin");
                m.AddCategory("Research");m.SaveCategoryRule("Research","pdf","example.test");var classified=m.Add("https://files.example.test/paper.pdf",files,null,"Main queue",true,null,"",null);var ordinary=m.Add("https://different.test/paper.pdf",files,null,"Main queue",true,null,"",null);check(classified.Category=="Research"&&ordinary.Category=="Documents","category rules combine actual extension and host matching");
                var project=new GrabProject{Name="Documentation",StartUrl="https://example.test/docs",Links=new List<GrabLink>{new GrabLink{Url="https://example.test/book.pdf"},new GrabLink{Url="https://example.test/skip.zip",Selected=false}}};projectId=project.Id;
                check(m.AddProjectFiles(project,"Main queue",true)==1&&m.AddProjectFiles(project,"Main queue",true)==0&&m.State.Downloads.Count(d=>d.ProjectId==project.Id)==1,"saved grabber selections add once and remain associated with their project");
                project.Links.Add(new GrabLink{Url="https://different.test/file.zip"});check(Rejects(()=>m.SaveProject(project)),"grabber projects reject cross-origin file injection");project.Links.RemoveAt(2);m.Save();
                check(BatchPattern.Expand("https://example.test/img*.jpg","1","3",3,false).SequenceEqual(new[]{"https://example.test/img001.jpg","https://example.test/img002.jpg","https://example.test/img003.jpg"})&&BatchPattern.Expand("https://example.test/*.zip","a","c",1,true).Count==3,"batch patterns produce padded numeric and alphabetic file sequences");
                check(Rejects(()=>BatchPattern.Expand("https://example.test/*.zip","0","1000",1,false))&&Rejects(()=>Manager.ValidateCaptureRules(".zip", "example.test")),"oversized batches and malformed browser capture rules are rejected");
            }
            using(var m=new Manager(data,false))check(m.State.Projects.Single().Id==projectId&&m.State.Settings.SiteLogins.Count==1&&m.State.Settings.CategoryRules.Count==1,"projects, category rules and site logins survive restarting UDM");
        }
    }
}
