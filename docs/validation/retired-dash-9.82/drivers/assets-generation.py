assets=area/'assets';assets.mkdir();expected=[]
def frames(path):
 text=subprocess.check_output([str(tools/'ffmpeg.exe'),'-hide_banner','-loglevel','error','-xerror','-nostdin','-i',str(path),'-map','0:v:0','-f','framemd5','-'],text=True);return [line.rsplit(',',1)[1].strip() for line in text.splitlines() if line and not line.startswith('#')]
for n,color in enumerate(['blue','red']):
 folder=assets/('p'+str(n));folder.mkdir();movie=folder/'original.mp4';subprocess.run([str(tools/'ffmpeg.exe'),'-hide_banner','-loglevel','error','-nostdin','-y','-f','lavfi','-i',('testsrc2=s=640x360:r=10:d=1'+(',hue=h=90' if n else '')),'-c:v','libx264','-preset','ultrafast','-profile:v','baseline','-level','3.0','-bf','0','-g','10','-pix_fmt','yuv420p','-video_track_timescale','1000','-movflags','empty_moov+frag_keyframe+default_base_moof',str(movie)],check=True)
 blob=movie.read_bytes();boxes=[];at=0
 while at<len(blob):
  length,kind=struct.unpack('>I4s',blob[at:at+8]);assert length>=8 and at+length<=len(blob);boxes.append((kind,blob[at:at+length]));at+=length
 assert [k for k,_ in boxes].count(b'moof')==1
 (folder/'init.mp4').write_bytes(b''.join(v for k,v in boxes if k in [b'ftyp',b'moov']));(folder/'segment.m4s').write_bytes(b''.join(v for k,v in boxes if k in [b'moof',b'mdat']));oracle=frames(movie);assert len(oracle)==10;expected.append(oracle)
