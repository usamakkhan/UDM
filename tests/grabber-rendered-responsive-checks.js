window.addEventListener('load',()=>{
 const img=document.querySelector('img'),before=document.documentElement.outerHTML,result=UdmRenderedGrabber.collect(true),saved=new DOMParser().parseFromString(result.html,'text/html');
 const checks={browserSelectedImage:img.complete&&img.naturalWidth===8&&img.currentSrc.endsWith('/grabber-rendered-responsive.svg'),savedSelectedImage:saved.querySelector('img').getAttribute('src')===img.currentSrc,noCompetingVariants:!saved.querySelector('source')&&!saved.querySelector('img[srcset]'),matchingDiscovery:result.references.some(r=>r.url===img.currentSrc),livePageUnchanged:document.documentElement.outerHTML===before};
 const pre=document.createElement('pre');pre.textContent=JSON.stringify({passed:Object.values(checks).every(Boolean),checks},null,2);document.body.append(pre);
});
