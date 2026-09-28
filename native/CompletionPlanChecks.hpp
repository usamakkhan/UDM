#pragma once
#include "CompletionPolicy.hpp"
static void completionPlanChecks(const udm::fs::path& root){
 using namespace udm;
 check(completionSteps(Json{{"CompletionAction","Exit UDM"}})==std::vector<std::string>{"Exit UDM"},"Existing single-action records retain their completion choice");
 check(completionSteps(Json::object()).empty(),"New completion plans default to no actions");
 Manager manager(root/L"completion-plan");auto job=manager.add("https://example.test/combined.bin");
 manager.setCompletionPlan(job,{"Exit UDM","Shut down","Disconnect dial-up / VPN"},true,45,false);
 const auto steps=std::vector<std::string>{"Disconnect dial-up / VPN","Shut down","Exit UDM"};
 check(completionSteps(job->data)==steps&&yes(job->data,"CompletionForce"),"Combined plan preserves separate disconnect, power and exit choices in dispatch order");
 {Manager reopened(manager.root);check(completionSteps(reopened.jobs.at(0)->data)==steps&&yes(reopened.jobs.at(0)->data,"CompletionActionArmed"),"Combined completion options survive process restart");}
 check(manager.takeDownloadCompletion(job).empty(),"Combined actions wait for successful completion");
 auto before=job->data;
 rejects([&]{manager.setCompletionPlan(job,{"Sleep","Restart"});},"Conflicting power choices are rejected");
 rejects([&]{manager.setCompletionPlan(job,{"Exit UDM","Exit UDM"});},"Duplicate completion actions are rejected");
 rejects([&]{manager.setCompletionPlan(job,{"Open command"});},"Combined plans cannot launch arbitrary commands");
 rejects([&]{manager.setCompletionPlan(job,{"Sleep"},true);},"Force termination cannot be applied to sleep");
 rejects([&]{manager.setCompletionPlan(job,{"Exit UDM"},false,0);},"Combined actions cannot skip the countdown");
 check(job->data==before,"Invalid completion plans leave the previously armed plan unchanged");
 auto orphan=std::make_shared<Job>(job->data);rejects([&]{manager.setCompletionPlan(orphan,{"Exit UDM"});},"Detached records cannot arm completion actions");
 auto folder=manager.root;manager.root=root/L"completion-save-blocker";writeBytes(manager.root,Bytes{1});
 rejects([&]{manager.setCompletionPlan(job,{"Restart"},false,60);},"Combined completion settings report a failed save");manager.root=folder;
 check(job->data==before,"Failed completion save restores the entire previous plan");
 job->data["Status"]="Complete";job->data["ScanResult"]={{"Status","Running"}};
 check(manager.takeDownloadCompletion(job).empty()&&yes(job->data,"CompletionActionArmed"),"Virus scanning prevents early combined actions");
 job->data.erase("ScanResult");auto spec=manager.takeDownloadCompletion(job);
 check(spec.at("Actions")==Json(steps)&&yes(spec,"Force")&&num(spec,"DelaySeconds")==45&&!yes(spec,"WaitForOthers",true),"Completion dispatch carries all selected actions, force flag, delay and wait policy");
 check(manager.takeDownloadCompletion(job).empty(),"A combined completion plan is dispatched only once");
 {Manager reopened(folder);check(reopened.takeDownloadCompletion(reopened.jobs.at(0)).empty(),"Restart cannot replay consumed completion actions");}
 job->data["Status"]="Paused";manager.setCompletionAction(job,"Sleep");
 check(completionSteps(job->data)==std::vector<std::string>{"Sleep"}&&!yes(job->data,"CompletionForce"),"Legacy single-action editing replaces a composite plan and resets force");
 manager.setCompletionPlan(job,{});check(!yes(job->data,"CompletionActionArmed")&&completionSteps(job->data).empty(),"Clearing all completion checkboxes disarms the plan");
 check(completionShutdownFlags("Shut down",false)==EWX_POWEROFF&&completionShutdownFlags("Restart",false)==EWX_REBOOT,"Normal power actions preserve Windows application shutdown checks");
 check(completionShutdownFlags("Shut down",true)==(EWX_POWEROFF|EWX_FORCE),"Force termination is added only when explicitly selected");
 rejects([]{completionShutdownFlags("Sleep",true);},"Non-shutdown actions cannot produce force-shutdown flags");
 job->data["CompletionActions"]={"Launch command"};job->data["CompletionActionArmed"]=true;job->data["Status"]="Complete";
 check(manager.takeDownloadCompletion(job).empty()&&!yes(job->data,"CompletionActionArmed"),"Malformed persisted action plans are disarmed without dispatch");
}
