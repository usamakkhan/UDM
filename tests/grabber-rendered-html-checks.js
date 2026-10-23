(async()=>{
const checks=[];const check=(ok,name)=>{checks.push({name,passed:!!ok});if(!ok)throw Error(name);};
try{
 document.querySelector('#runtime').innerHTML='<h1>Runtime-created heading</h1><img src="photo.png"><a href="download.zip">Runtime download</a>';
 const picture=document.createElement('picture');picture.innerHTML='<source srcset="unused.webp"><img id="responsive" src="fallback.png" srcset="unused-large.png 2x" sizes="100vw">';document.body.append(picture);
 Object.defineProperty(picture.querySelector('img'),'currentSrc',{value:new URL('selected.png',document.baseURI).href});
 const host=document.createElement('section');host.id='component';document.body.append(host);
 const shadow=host.attachShadow({mode:'open'});shadow.innerHTML='<style>p{color:blue}</style><p>Shadow content</p><input value="shadow-private"><script>window.shadowScriptRan=true</script><img src="shadow.png" onclick="void(0)"><div id="nested"></div>';
 shadow.querySelector('#nested').attachShadow({mode:'open'}).innerHTML='<a href="nested.zip">Nested content</a>';
 const constructed=new CSSStyleSheet();constructed.replaceSync('p { color: rgb(12, 34, 56); } p::after { content: "</style><script>bad()</script>"; }');shadow.adoptedStyleSheets=[constructed];
 const documentSheet=new CSSStyleSheet();documentSheet.replaceSync('#runtime { border-top: 7px solid red; }');document.adoptedStyleSheets=[documentSheet];
 const disabledSheet=new CSSStyleSheet();disabledSheet.replaceSync('p { color: lime; }');disabledSheet.disabled=true;shadow.adoptedStyleSheets=[constructed,disabledSheet];
 Object.defineProperty(shadow.querySelector('img'),'currentSrc',{value:new URL('shadow-selected.png',document.baseURI).href});
 const inert=document.createElement('template');inert.id='inert';inert.innerHTML='<input value="template-private"><textarea>template-secret</textarea><script>void(0)</script>';document.body.append(inert);
 for(const [parent,tag,id] of [[document.body,'video','selected-video'],[shadow,'audio','selected-audio']]){
  const media=document.createElement(tag);media.id=id;media.setAttribute('src','stale-media.bin');media.innerHTML='<source src="unused-media.bin"><track src="captions.vtt">';parent.append(media);
  Object.defineProperty(media,'currentSrc',{value:new URL(id+'.mp4',document.baseURI).href});
 }
 const blobMedia=document.createElement('video');blobMedia.id='blob-media';blobMedia.innerHTML='<source src="fallback.mp4">';document.body.append(blobMedia);
 Object.defineProperty(blobMedia,'currentSrc',{value:'blob:'+location.origin+'/unavailable-stream'});
 const panel=document.createElement('div');panel.id='udm-video-panel-7';panel.attachShadow({mode:'open'}).innerHTML='<button>Download with UDM</button>';document.body.append(panel);document.querySelector('#selected-video').setAttribute('data-udm-epoch','3');document.documentElement.setAttribute('data-udm-content-ready','abc:1');
 const original=document.documentElement.outerHTML,originalShadow=shadow.innerHTML;
 const result=UdmRenderedGrabber.collect(true),html=result.html||'';
 check(!result.htmlUnavailable&&html.includes('Runtime-created heading'),'Keeps rendered content');
 check(html.startsWith('<!doctype html>'),'Produces complete HTML document');
 check(html.includes('<base href="'+document.baseURI+'">'),'Preserves effective base URL');
 check(!html.includes('fixture-private-input')&&!html.includes('fixture-private-textarea'),'Excludes form values');
 check(!html.includes('fixture-private-frame'),'Excludes frame srcdoc');
 const parsed=new DOMParser().parseFromString(html,'text/html');
 check(!parsed.querySelector('#blob-media').hasAttribute('src')&&parsed.querySelector('#blob-media source').getAttribute('src')==='fallback.mp4','Unsupported blob media keeps its ordinary fallback');
 check(parsed.querySelector('#responsive').getAttribute('src')===new URL('selected.png',document.baseURI).href,'Preserves selected responsive image URL');
 check(!parsed.querySelector('#responsive').hasAttribute('srcset')&&!parsed.querySelector('#responsive').hasAttribute('sizes'),'Prevents offline image reselection');
 check(!parsed.querySelector('picture source'),'Removes competing picture sources');
 check(!parsed.querySelector('[id^="udm-"]')&&!html.includes('Download with UDM')&&!html.includes('data-udm-'),'Excludes UDM panels and capture markers from the saved page');
 check(result.references.some(item=>item.url===new URL('selected.png',document.baseURI).href),'Discovers the same selected image as saved HTML');
 check(parsed.querySelectorAll('script').length===0,'Does not replay page scripts');
 check(!parsed.querySelector('[onclick]'),'Removes inline event handlers');
 check(!parsed.querySelector('a[href^="javascript:"]')&&!html.includes('user:password'),'Removes active and credentialed link targets');
 check(!parsed.querySelector('input[checked]')&&!parsed.querySelector('option[selected]'),'Clears form selection state');
 const savedShadow=parsed.querySelector('#component > template[shadowrootmode="open"]').content;
 for(const [scope,id] of [[parsed,'selected-video'],[savedShadow,'selected-audio']]){
  const media=scope.querySelector('#'+id);
  check(media.getAttribute('src')===new URL(id+'.mp4',document.baseURI).href,'Freezes selected '+id+' source');
  check(!media.querySelector('source')&&!!media.querySelector('track'),'Removes competing sources but keeps '+id+' captions');
  check(result.references.some(item=>item.url===media.getAttribute('src')),'Discovery agrees with saved '+id);
 }
 check(savedShadow.querySelector('p').textContent==='Shadow content','Preserves open shadow content');
 check(savedShadow.querySelector('style').textContent==='p{color:blue}','Preserves shadow styles');
 check(savedShadow.querySelector('#nested > template').content.querySelector('a').textContent==='Nested content','Preserves nested shadow content');
 check(savedShadow.querySelector('img').getAttribute('src')===new URL('shadow-selected.png',document.baseURI).href,'Freezes shadow responsive images');
 check(!savedShadow.querySelector('script')&&!savedShadow.querySelector('[onclick]')&&!html.includes('shadow-private'),'Sanitizes shadow content');
 check(!html.includes('template-private')&&!html.includes('template-secret')&&!parsed.querySelector('#inert').content.querySelector('script'),'Sanitizes inert template contents');
 check(shadow.innerHTML===originalShadow,'Leaves live shadow content unchanged');
 check(document.documentElement.outerHTML===original,'Leaves live document unchanged');
 check(!Object.hasOwn(UdmRenderedGrabber.collect(),'html'),'Existing reference-only capture stays compatible');
 const huge=document.createElement('p');huge.textContent='x'.repeat(600000);document.body.append(huge);
 const bounded=UdmRenderedGrabber.collect(true);check(bounded.htmlUnavailable&&!Object.hasOwn(bounded,'html'),'Oversized HTML is omitted rather than truncated');huge.remove();
 const unicode=document.createElement('p');unicode.textContent='界'.repeat(180000);document.body.append(unicode);
 const bytes=UdmRenderedGrabber.collect(true);check(bytes.htmlUnavailable&&!Object.hasOwn(bytes,'html'),'Limit counts UTF-8 bytes');unicode.remove();
 const giant=document.createElement('p');giant.textContent='x'.repeat(600000);shadow.append(giant);
 const shadowBound=UdmRenderedGrabber.collect(true);check(shadowBound.htmlUnavailable&&!Object.hasOwn(shadowBound,'html'),'Shadow content participates in size limits');giant.remove();
 const replay=document.createElement('iframe');
 await new Promise((resolve,reject)=>{const timeout=setTimeout(()=>reject(Error('Saved-page load timed out')),5000);replay.onload=()=>{clearTimeout(timeout);resolve();};replay.srcdoc=html;document.body.append(replay);});
 const restored=replay.contentDocument.querySelector('#component').shadowRoot;
 check(restored?.querySelector('p').textContent==='Shadow content','Saved HTML reconstructs shadow root on load');
 check(restored.querySelector('#nested').shadowRoot?.querySelector('a').textContent==='Nested content','Saved HTML reconstructs nested shadow root');
 check(replay.contentWindow.getComputedStyle(restored.querySelector('p')).color==='rgb(12, 34, 56)','Adopted shadow stylesheet preserves cascade order');
 check(replay.contentWindow.getComputedStyle(replay.contentDocument.querySelector('#runtime')).borderTopWidth===getComputedStyle(document.querySelector('#runtime')).borderTopWidth,'Adopted document stylesheet matches live style');
 check(!replay.contentDocument.querySelector('script')&&!restored.querySelector('script'),'CSS text cannot terminate style element');
 check(replay.contentWindow.getComputedStyle(restored.querySelector('p'),'::after').content.includes('</style>'),'CSS escaped content remains faithful');
 check(shadow.adoptedStyleSheets[0]===constructed&&document.adoptedStyleSheets[0]===documentSheet,'Live adopted stylesheet lists unchanged');
 check(!replay.contentWindow.shadowScriptRan,'Saved shadow scripts do not execute');replay.remove();
 const largeSheet=new CSSStyleSheet();largeSheet.replaceSync('p::before { content: "'+'x'.repeat(600000)+'"; }');shadow.adoptedStyleSheets=[largeSheet];
 const limited=UdmRenderedGrabber.collect(true);check(limited.htmlUnavailable&&!Object.hasOwn(limited,'html'),'Adopted styles participate in capture bounds');shadow.adoptedStyleSheets=[constructed];
}catch(error){checks.push({name:error.message,passed:false});}
const output=document.createElement('pre');output.id='results';output.textContent=JSON.stringify({passed:checks.every(x=>x.passed),checks},null,2);document.body.replaceChildren(output);
})();
