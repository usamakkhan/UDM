/* Observe the browser's own entry list without canceling, replacing or resubmitting a form. */
(()=>{
 'use strict';
 const api=globalThis.browser||globalThis.chrome;
 if(!api?.runtime?.id||globalThis.__udmFormCapture)return;globalThis.__udmFormCapture=true;
 const submissions=new WeakMap();
 // Firefox can hide Symbol.iterator on a FormData iterator crossing an Xray
 // wrapper. Native forEach retains order and duplicates without unwrapping it.
 function entries(data){
  const values=[];FormData.prototype.forEach.call(data,(value,key)=>{
   if(values.length>=16384)throw Error('Too many submitted form entries.');
   values.push([key,value]);
  });return values;
 }
 document.addEventListener('submit',event=>{
  if(!event.isTrusted||!(event.target instanceof HTMLFormElement))return;
  const form=event.target,button=event.submitter;
  submissions.set(form,{
   action:button?.hasAttribute('formaction')?button.formAction:form.action,
   method:button?.hasAttribute('formmethod')?button.formMethod:form.method,
   enctype:button?.hasAttribute('formenctype')?button.formEnctype:form.enctype,
   time:performance.now()
  });
 },true);
 document.addEventListener('formdata',event=>{
  if(!event.isTrusted||!(event.target instanceof HTMLFormElement))return;
  const form=event.target,submission=submissions.get(form);
  if(!submission||performance.now()-submission.time>1000)return;
  submissions.delete(form);
  if(submission.method.toLowerCase()!=='post'||submission.enctype.toLowerCase()!=='multipart/form-data')return;
  const charset=(form.acceptCharset||document.characterSet||'').trim().toLowerCase();
  if(!/^(utf-8|utf8)$/.test(charset))return;
  let action;try{action=new URL(submission.action,location.href);if(!/^https?:$/.test(action.protocol)||action.username||action.password)return;action.hash='';}catch{return;}
  const data=event.formData;let priorPromise;
  try{priorPromise=UdmMultipart.captureEntries(entries(data));}catch{return;}
  // A microtask can run between event listeners. Wait for dispatch to finish,
  // retaining the initial ordering so an indistinguishable reorder is rejected.
  setTimeout(async()=>{
   try{
    const finalPromise=UdmMultipart.captureEntries(entries(data));
    const [prior,snapshot]=await Promise.all([priorPromise,finalPromise]);if(!prior||!snapshot)return;
    void api.runtime.sendMessage({action:'capture-multipart-form',url:action.href,fields:snapshot.fields,priorFields:UdmMultipart.sameEncoding(prior,snapshot)?undefined:prior.fields}).catch(()=>{});
   }catch{}
  });
 },true);
})();
