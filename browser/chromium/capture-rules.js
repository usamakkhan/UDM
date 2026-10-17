/* UDM site capture rules. Compiles "[FLAGS][#EXT]!PATTERN" lines and decides
   whether a request is ignored, captured, or left to the normal rules. Patterns
   are matched by a memoized matcher, so a rule's cost stays bounded by the
   address length instead of growing with the number of wildcards. */
(function(root){
 'use strict';
 const LINE_LIMIT=2048,RULE_LIMIT=2000,URL_LIMIT=4096,FLAGS='GPTSDVACRHX';
 const classes={F:'/?#&;',T:'/?#&;.'};
 // Tokens: lit (one character), star (any text), wstar (word characters, the
 // scheme wildcard), sub (optional host label), query (optional trailing
 // query), open/close (capture bounds) and cstar (capture wildcard).
 function tokenize(pattern){
  const tokens=[],captures=[];const schemeEnd=pattern.indexOf('://');
  for(let at=0;at<pattern.length;){
   const c=pattern[at];
   const capture=c==='<'?/^<([FT]):([^>]*)>/.exec(pattern.slice(at)):null;
   if(capture){
    const index=captures.length;captures.push(capture[1]);tokens.push({t:'open',index});
    for(const inner of capture[2])tokens.push(inner==='*'?{t:'cstar',stop:classes[capture[1]]}:{t:'lit',c:inner});
    tokens.push({t:'close',index});at+=capture[0].length;continue;
   }
   if(c==='*')tokens.push(at<schemeEnd?{t:'wstar'}:{t:'star'});
   else if(c==='~'&&at>schemeEnd+2&&pattern.slice(schemeEnd+3,at).search(/[:/]/)<0)tokens.push({t:'sub'});
   else if(c==='?'&&at===pattern.length-1)tokens.push({t:'query'});
   else tokens.push({t:'lit',c});
   at++;
  }
  return {tokens,captures};
 }
 function match(tokens,text){
  const n=text.length,m=tokens.length,starts=[],ends=[],failed=new Uint8Array((m+1)*(n+1)),starHit=new Uint8Array((m+1)*(n+1));
  const word=ch=>/\w/.test(ch);
  function go(i,j){
   const key=i*(n+1)+j;if(failed[key])return false;
   let ok=false;
   if(i===m)ok=j===n;
   else{const tok=tokens[i];
    switch(tok.t){
     case 'lit':ok=j<n&&text[j]===tok.c&&go(i+1,j+1);break;
     case 'star':{
      // A wildcard at j succeeds exactly when one at j+1 does or the rest
      // matches at j, so every position is settled once per token.
      const next=tokens[i+1],base=i*(n+1);let k=j;
      for(;k<=n;k++){if(failed[base+k]||starHit[base+k])break;if(!(next&&next.t==='lit'&&(k===n||text[k]!==next.c))&&go(i+1,k))break;}
      if(k<=n&&!failed[base+k]){ok=true;for(let p=j;p<=k;p++)starHit[base+p]=1;}
      else for(let p=j;p<k;p++)failed[base+p]=1;
      break;}
     case 'cstar':{for(let k=j;k<=n;k++){if(go(i+1,k)){ok=true;break;}if(k===n||tok.stop.includes(text[k]))break;}break;}
     case 'wstar':{for(let k=j;k<=n;k++){if(go(i+1,k)){ok=true;break;}if(k===n||!word(text[k]))break;}break;}
     case 'sub':{if(go(i+1,j))ok=true;else{let k=j;while(k<n&&text[k]!=='.'&&text[k]!=='/')k++;if(k>j&&k<n&&text[k]==='.'&&go(i+1,k+1))ok=true;}break;}
     case 'query':ok=j===n||(text[j]==='?'&&i===m-1);break;
     case 'open':if(go(i+1,j)){starts[tok.index]=j;ok=true;}break;
     case 'close':if(go(i+1,j)){ends[tok.index]=j;ok=true;}break;
    }
   }
   if(!ok)failed[key]=1;return ok;
  }
  if(!go(0,0))return null;
  return starts.map((start,index)=>text.slice(start,ends[index]));
 }
 function parseLine(line){
  const m=/^(?:([A-Z]+)?(?:#([A-Za-z0-9]{1,16}))?!)?(.+)$/.exec(line);
  if(!m||line.length>LINE_LIMIT)return null;
  const flags=m[1]||'';if([...flags].some(f=>!FLAGS.includes(f)))return null;
  const pattern=m[3];if(!pattern.includes('://')||/\s/.test(pattern))return null;
  if(pattern.includes('!')&&pattern.indexOf('!')<pattern.indexOf('://'))return null; // malformed prefix, not a pattern
  if(/<(?![FT]:)[^>]*>/.test(pattern)||pattern.replace(/<[FT]:[^>]*>/g,'').includes('<'))return null;
  const {tokens,captures}=tokenize(pattern);
  return {line,flags,ext:(m[2]||'').toLowerCase(),pattern,tokens,captures};
 }
 function compile(lines){
  const rules=[],errors=[];
  for(const raw of Array.isArray(lines)?lines.slice(0,RULE_LIMIT):[]){
   const line=String(raw||'').trim();if(!line||line.startsWith('//'))continue;
   const rule=parseLine(line);if(!rule){errors.push(line);continue;}
   rules.push(rule);
  }
  return {rules,errors};
 }
 const extensionOf=name=>{const m=/\.([A-Za-z0-9]{1,16})$/.exec(name||'');return m?m[1].toLowerCase():'';};
 const pathExtension=href=>{try{return extensionOf(new URL(href).pathname.split('/').pop());}catch{return '';}};
 // request: {url, method, background, secure, hidden}. allowed(ext) reports
 // whether an extension is in the user's automatic-capture list. Returns null
 // when no rule matches, otherwise {action:'ignore'|'capture'|'stream'|'continue', extension, rule}.
 function evaluate(compiled,request,allowed){
  const rules=compiled?.rules||[];const href=String(request?.url||'');if(!rules.length||!href||href.length>URL_LIMIT)return null;
  const method=String(request?.method||'GET').toUpperCase(),background=!!request?.background,secure=request?.secure!==undefined?!!request.secure:href.startsWith('https:');
  let hidden=!!request?.hidden;
  for(const rule of rules){
   const captured=match(rule.tokens,href);if(!captured)continue;
   const f=rule.flags;let extension='';const result=action=>({action,extension,rule:rule.line});
   let methodFiltered=false,methodOk=false;
   if(f.includes('G')){methodFiltered=true;methodOk||=method==='GET';}
   if(f.includes('P')){methodFiltered=true;methodOk||=method==='POST';}
   if(methodFiltered&&!methodOk)return result('continue');
   const stream=f.includes('V')||f.includes('A')||f.includes('C');
   if(f.includes('H'))hidden=true;
   if((stream?hidden:background?!f.includes('S'):f.includes('X'))||(!secure&&f.includes('D')))return result('ignore');
   let fileName='',explicit=false;
   rule.captures.forEach((tag,i)=>{const value=captured[i]||'';if(tag==='F'){explicit=true;fileName=value;}if(tag==='T'){explicit=true;extension=extension||value.toLowerCase();}});
   extension=extension||extensionOf(fileName)||(f.includes('T')&&(explicit=true,pathExtension(href)))||rule.ext;
   if(stream)return result('stream');
   if(extension?typeof allowed==='function'&&allowed(extension):!explicit)return result('capture');
   return result('continue');
  }
  return null;
 }
 const api={compile,parseLine,match,evaluate,limits:{line:LINE_LIMIT,rules:RULE_LIMIT,url:URL_LIMIT}};
 root.UdmCaptureRules=api;if(typeof module==='object'&&module.exports)module.exports=api;
})(globalThis);
