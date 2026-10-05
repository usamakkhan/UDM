param([string]$Path='D:\UDM\IDM FIles\idmantypeinfo.tlb',[string]$Output='D:\UDM\benchmarks\idm-complete-20260927\typelib.json')
$ErrorActionPreference='Stop'
Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;
public class TypeLibraryReader {
 [DllImport("oleaut32.dll", CharSet=CharSet.Unicode, PreserveSig=false)]
 static extern void LoadTypeLibEx(string file, int regkind, out ITypeLib lib);
 public static object Read(string path) {
  ITypeLib lib; LoadTypeLibEx(path,2,out lib); // REGKIND_NONE: parse metadata without registration.
  var types=new List<object>();
  try {
   for(int i=0;i<lib.GetTypeInfoCount();i++) {
    ITypeInfo ti;lib.GetTypeInfo(i,out ti);IntPtr pa;ti.GetTypeAttr(out pa);
    try {
     var a=Marshal.PtrToStructure<TYPEATTR>(pa);string name,doc,help;int context;ti.GetDocumentation(-1,out name,out doc,out context,out help);
     var funcs=new List<object>();var impl=new List<object>();
     for(int j=0;j<a.cFuncs;j++) {
      IntPtr pf;ti.GetFuncDesc(j,out pf);
      try {
       var f=Marshal.PtrToStructure<FUNCDESC>(pf);string[] names=new string[f.cParams+1];int count;ti.GetNames(f.memid,names,names.Length,out count);
       var pars=new List<object>();int stride=Marshal.SizeOf<ELEMDESC>();
       for(int k=0;k<f.cParams;k++){var e=Marshal.PtrToStructure<ELEMDESC>(IntPtr.Add(f.lprgelemdescParam,k*stride));pars.Add(new {name=k+1<count?names[k+1]:null,variantType=e.tdesc.vt,flags=e.desc.paramdesc.wParamFlags.ToString()});}
       funcs.Add(new {name=count>0?names[0]:null,memberId=f.memid,invoke=f.invkind.ToString(),callConvention=f.callconv.ToString(),vtableOffset=f.oVft,returnVariantType=f.elemdescFunc.tdesc.vt,parameters=pars});
      }finally{ti.ReleaseFuncDesc(pf);}
     }
     for(int j=0;j<a.cImplTypes;j++){int href;ti.GetRefTypeOfImplType(j,out href);ITypeInfo other;ti.GetRefTypeInfo(href,out other);try{string n,d,h;int c;other.GetDocumentation(-1,out n,out d,out c,out h);IMPLTYPEFLAGS flags;ti.GetImplTypeFlags(j,out flags);impl.Add(new {name=n,flags=flags.ToString()});}finally{Marshal.ReleaseComObject(other);}}
     types.Add(new {name,guid=a.guid,kind=a.typekind.ToString(),major=a.wMajorVerNum,minor=a.wMinorVerNum,flags=a.wTypeFlags.ToString(),functions=funcs,implements=impl});
    }finally{ti.ReleaseTypeAttr(pa);Marshal.ReleaseComObject(ti);}
   }
   return new {path,registration="REGKIND_NONE",types};
  }finally{Marshal.ReleaseComObject(lib);}
 }
}
'@
[TypeLibraryReader]::Read($Path) | ConvertTo-Json -Depth 14 | Set-Content -LiteralPath $Output -Encoding utf8
Get-Content -LiteralPath $Output -Raw
