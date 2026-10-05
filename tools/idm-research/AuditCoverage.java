// @category UDM.ReferenceAnalysis
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.address.*;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;
import java.io.*;
import java.util.*;
import com.google.gson.*;

public class AuditCoverage extends GhidraScript {
  public void run() throws Exception {
    String evidenceName=currentProgram.getName().contains("__")?currentProgram.getName():currentProgram.getExecutableSHA256().substring(0,12)+"__"+currentProgram.getName();
    Path dir=Paths.get(getScriptArgs()[0],evidenceName);
    if(!Files.exists(dir.resolve("summary.json"))) {println("NO EXPORT "+currentProgram.getName());return;}
    if(Files.exists(dir.resolve("audit.json"))) {println("ALREADY AUDITED "+currentProgram.getName());return;}
    Gson gson=new GsonBuilder().setPrettyPrinting().create();
    AddressSet functions=new AddressSet();
    for(Function f:currentProgram.getFunctionManager().getFunctions(true))functions.add(f.getBody());
    AddressSet instructions=new AddressSet();long instructionCount=0;
    try(BufferedWriter out=Files.newBufferedWriter(dir.resolve("instructions.asm"),StandardCharsets.UTF_8)) {
      for(Instruction i:currentProgram.getListing().getInstructions(true)) {
        monitor.checkCancelled();instructions.add(i.getMinAddress(),i.getMaxAddress());instructionCount++;
        out.write(i.getAddress()+"\t"+HexFormat.of().formatHex(i.getBytes())+"\t"+i.toString()+"\n");
      }
    }
    List<Map<String,Object>> blocks=new ArrayList<>();
    for(MemoryBlock b:currentProgram.getMemory().getBlocks()) {
      if(!b.isExecute())continue;
      AddressSet span=new AddressSet(b.getStart(),b.getEnd());Map<String,Object> row=new LinkedHashMap<>();
      row.put("name",b.getName());row.put("start",b.getStart().toString());row.put("bytes",b.getSize());
      row.put("instructionBytes",span.intersect(instructions).getNumAddresses());row.put("functionBodyBytes",span.intersect(functions).getNumAddresses());
      row.put("bytesOutsideRecognizedFunctions",span.subtract(functions).getNumAddresses());blocks.add(row);
    }
    List<Map<String,Object>> retries=new ArrayList<>();DecompInterface dec=new DecompInterface();dec.openProgram(currentProgram);
    try(BufferedWriter code=Files.newBufferedWriter(dir.resolve("retry-functions.c"),StandardCharsets.UTF_8);BufferedWriter asm=Files.newBufferedWriter(dir.resolve("failed-functions.asm"),StandardCharsets.UTF_8)) {
      for(String line:Files.readAllLines(dir.resolve("decompilation.jsonl"),StandardCharsets.UTF_8)) {
        JsonObject original=JsonParser.parseString(line).getAsJsonObject();if(original.get("completed").getAsBoolean())continue;
        String entry=original.get("entry").getAsString();Function f=getFunctionAt(toAddr(entry));Map<String,Object> row=new LinkedHashMap<>();row.put("entry",entry);row.put("initialError",original.get("error").getAsString());
        DecompileResults res=dec.decompileFunction(f,120,monitor);boolean ok=res.decompileCompleted()&&res.getDecompiledFunction()!=null;
        row.put("completed",ok);row.put("error",res.getErrorMessage());row.put("retryTimeoutSeconds",120);
        if(ok)code.write("\n/* FUNCTION "+entry+"; retry inferred pseudocode */\n"+res.getDecompiledFunction().getC());
        else {asm.write("\n; FUNCTION "+entry+" "+f.getName()+"\n");for(Instruction i:currentProgram.getListing().getInstructions(f.getBody(),true))asm.write(i.getAddress()+"\t"+HexFormat.of().formatHex(i.getBytes())+"\t"+i.toString()+"\n");}
        retries.add(row);println("RETRY "+currentProgram.getName()+" "+entry+" completed="+ok);
      }
    }finally{dec.dispose();}
    Map<String,Object> audit=new LinkedHashMap<>();audit.put("program",currentProgram.getName());audit.put("sha256",currentProgram.getExecutableSHA256());audit.put("instructions",instructionCount);audit.put("executableBlocks",blocks);audit.put("retries",retries);audit.put("limitation","Executable bytes outside recognized functions include padding, embedded data, and possibly undiscovered code; they are not automatically missing functions.");
    Files.writeString(dir.resolve("audit.pending.json"),gson.toJson(audit),StandardCharsets.UTF_8);
    Files.move(dir.resolve("audit.pending.json"),dir.resolve("audit.json"),StandardCopyOption.REPLACE_EXISTING,StandardCopyOption.ATOMIC_MOVE);println("AUDIT COMPLETE "+currentProgram.getName());
  }
}
