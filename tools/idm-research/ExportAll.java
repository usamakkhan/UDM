// Analysis pseudocode is not original source. No target code is executed.
// @category UDM.ReferenceAnalysis
import ghidra.app.util.headless.HeadlessScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import ghidra.program.util.DefinedStringIterator;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;
import java.io.*;
import java.util.*;
import com.google.gson.*;
public class ExportAll extends HeadlessScript {
  private String clean(Object s) { return String.valueOf(s).replace("\t"," ").replace("\r","\\r").replace("\n","\\n"); }
  public void run() throws Exception {
    if(getScriptArgs().length>1 && !getScriptArgs()[1].equalsIgnoreCase(currentProgram.getExecutableSHA256()))throw new IllegalArgumentException("Existing project hash differs from expected specimen");
    String evidenceName=currentProgram.getName().contains("__")?currentProgram.getName():currentProgram.getExecutableSHA256().substring(0,12)+"__"+currentProgram.getName();
    Path dir=Paths.get(getScriptArgs()[0],evidenceName);Files.createDirectories(dir);
    if(Files.exists(dir.resolve("summary.json"))) {
      JsonObject previous=JsonParser.parseString(Files.readString(dir.resolve("summary.json"),StandardCharsets.UTF_8)).getAsJsonObject();
      if(!currentProgram.getExecutableSHA256().equalsIgnoreCase(previous.get("sha256").getAsString()))throw new IllegalArgumentException("Existing export hash differs");
      println("EXISTING COMPLETE EXPORT RETAINED "+evidenceName);return;
    }
    Gson gson=new GsonBuilder().create();
    try(BufferedWriter w=Files.newBufferedWriter(dir.resolve("strings.tsv"),StandardCharsets.UTF_8)) {
      w.write("address\tvalue\treferences\n");
      for(Data d:DefinedStringIterator.forProgram(currentProgram)) {
        List<String> refs=new ArrayList<>();for(Reference r:currentProgram.getReferenceManager().getReferencesTo(d.getAddress())) {Function f=getFunctionContaining(r.getFromAddress());refs.add(r.getFromAddress()+":"+(f==null?"data":f.getEntryPoint()));}
        w.write(d.getAddress()+"\t"+clean(d.getValue())+"\t"+String.join(",",refs)+"\n");
      }
    }
    int total=0,eligible=0,success=0,failed=0,thunks=0,external=0;
    DecompInterface dec=new DecompInterface();dec.openProgram(currentProgram);
    try(BufferedWriter idx=Files.newBufferedWriter(dir.resolve("functions.tsv"),StandardCharsets.UTF_8);BufferedWriter status=Files.newBufferedWriter(dir.resolve("decompilation.jsonl"),StandardCharsets.UTF_8);BufferedWriter code=Files.newBufferedWriter(dir.resolve("all-functions.c"),StandardCharsets.UTF_8)) {
      idx.write("entry\tname\tbytes\texternal\tthunk\tcallers\tcallees\n");
      for(Function f:currentProgram.getFunctionManager().getFunctions(true)) {
        monitor.checkCancelled();total++;
        String callers=String.join(",",f.getCallingFunctions(monitor).stream().map(x->x.getEntryPoint().toString()).sorted().toArray(String[]::new));
        String callees=String.join(",",f.getCalledFunctions(monitor).stream().map(x->x.getEntryPoint()+":"+clean(x.getName())).sorted().toArray(String[]::new));
        idx.write(f.getEntryPoint()+"\t"+clean(f.getName())+"\t"+f.getBody().getNumAddresses()+"\t"+f.isExternal()+"\t"+f.isThunk()+"\t"+callers+"\t"+callees+"\n");
        if(f.isExternal()){external++;continue;}if(f.isThunk()){thunks++;continue;}eligible++;
        DecompileResults result=dec.decompileFunction(f,15,monitor);boolean ok=result.decompileCompleted()&&result.getDecompiledFunction()!=null;
        Map<String,Object> row=new LinkedHashMap<>();row.put("entry",f.getEntryPoint().toString());row.put("name",f.getName());row.put("completed",ok);row.put("error",result.getErrorMessage());status.write(gson.toJson(row)+"\n");
        if(ok){success++;code.write("\n/* FUNCTION "+f.getEntryPoint()+"; inferred pseudocode */\n"+result.getDecompiledFunction().getC());}else failed++;
        if(eligible%100==0){idx.flush();status.flush();code.flush();println("COVERAGE "+currentProgram.getName()+" attempted="+eligible+" successful="+success);}
      }
    }finally{dec.dispose();}
    Map<String,Object> summary=new LinkedHashMap<>();summary.put("program",currentProgram.getName());summary.put("sha256",currentProgram.getExecutableSHA256());summary.put("imageBase",currentProgram.getImageBase().toString());summary.put("language",currentProgram.getLanguageID().toString());summary.put("analysisTimedOut",analysisTimeoutOccurred());summary.put("functions",total);summary.put("eligible",eligible);summary.put("successful",success);summary.put("failed",failed);summary.put("external",external);summary.put("thunks",thunks);summary.put("allRecognizedNonThunkFunctionsAttempted",true);
    Files.writeString(dir.resolve("summary.pending.json"),new GsonBuilder().setPrettyPrinting().create().toJson(summary),StandardCharsets.UTF_8);
    Files.move(dir.resolve("summary.pending.json"),dir.resolve("summary.json"),StandardCopyOption.REPLACE_EXISTING,StandardCopyOption.ATOMIC_MOVE);
    println("ALL FUNCTION EXPORT COMPLETE "+gson.toJson(summary));
  }
}
