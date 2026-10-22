const checks=[];const check=(ok,name)=>{checks.push({name,passed:!!ok});if(!ok)throw Error(name);};
try{
 document.querySelector('#runtime').innerHTML='<h1>Runtime-created heading</h1><img src="photo.png"><a href="download.zip">Runtime download</a>';
 const picture=document.createElement('picture');picture.innerHTML='<source srcset="unused.webp"><img id="responsive" src="fallback.png" srcset="unused-large.png 2x" sizes="100vw">';document.body.append(picture);
 Object.defineProperty(picture.querySelector('img'),'currentSrc',{value:new URL('selected.png',document.baseURI).href});
 const original=document.documentElement.outerHTML;
 const result=UdmRenderedGrabber.collect(true),html=result.html||'';
 check(!result.htmlUnavailable&&html.includes('Runtime-created heading'),'Keeps rendered content');
 check(html.startsWith('<!doctype html>'),'Produces complete HTML document');
 check(html.includes('<base href="'+document.baseURI+'">'),'Preserves effective base URL');
 check(!html.includes('fixture-private-input')&&!html.includes('fixture-private-textarea'),'Excludes form values');
 check(!html.includes('fixture-private-frame'),'Excludes frame srcdoc');
 const parsed=new DOMParser().parseFromString(html,'text/html');
 check(parsed.querySelector('#responsive').getAttribute('src')===new URL('selected.png',document.baseURI).href,'Preserves selected responsive image URL');
 check(!parsed.querySelector('#responsive').hasAttribute('srcset')&&!parsed.querySelector('#responsive').hasAttribute('sizes'),'Prevents offline image reselection');
 check(!parsed.querySelector('picture source'),'Removes competing picture sources');
 check(result.references.some(item=>item.url===new URL('selected.png',document.baseURI).href),'Discovers the same selected image as saved HTML');
 check(parsed.querySelectorAll('script').length===0,'Does not replay page scripts');
 check(!parsed.querySelector('[onclick]'),'Removes inline event handlers');
 check(!parsed.querySelector('a[href^="javascript:"]')&&!html.includes('user:password'),'Removes active and credentialed link targets');
 check(!parsed.querySelector('input[checked]')&&!parsed.querySelector('option[selected]'),'Clears form selection state');
 check(document.documentElement.outerHTML===original,'Leaves live document unchanged');
 check(!Object.hasOwn(UdmRenderedGrabber.collect(),'html'),'Existing reference-only capture stays compatible');
 const huge=document.createElement('p');huge.textContent='x'.repeat(600000);document.body.append(huge);
 const bounded=UdmRenderedGrabber.collect(true);check(bounded.htmlUnavailable&&!Object.hasOwn(bounded,'html'),'Oversized HTML is omitted rather than truncated');huge.remove();
 const unicode=document.createElement('p');unicode.textContent='界'.repeat(180000);document.body.append(unicode);
 const bytes=UdmRenderedGrabber.collect(true);check(bytes.htmlUnavailable&&!Object.hasOwn(bytes,'html'),'Limit counts UTF-8 bytes');unicode.remove();
}catch(error){checks.push({name:error.message,passed:false});}
const output=document.createElement('pre');output.id='results';output.textContent=JSON.stringify({passed:checks.every(x=>x.passed),checks},null,2);document.body.replaceChildren(output);
