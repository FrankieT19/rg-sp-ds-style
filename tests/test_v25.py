from pathlib import Path
from PIL import Image,ImageChops
import tempfile,shutil,subprocess,os,json,wave
import numpy as np
P=Path(__file__).resolve().parents[1];D=P/'device/Roms/APPS/DSStyle';checks=[]
with tempfile.TemporaryDirectory() as tmp:
 root=Path(tmp);base=root/'DSStyle';shutil.copytree(D/'assets',base/'assets');state=base/'state';state.mkdir()
 def run(screen='home',events='',render=False,extra=None):
  env=os.environ.copy();env.update(extra or {});args=[str(P/'preview/dsstyle-preview.exe'),'--base',str(base),'--demo','--screen',screen,'--events',events]
  args+=['--render',str(root/'screen.bmp')] if render else ['--self-test']
  r=subprocess.run(args,env=env,capture_output=True,text=True);assert r.returncode==0,r.stderr
  return Image.open(root/'screen.bmp').convert('RGB') if render else r
 # Sound controls now live in Interface; old three-field preferences still load.
 run('settings','a'+'d'*18+'a');assert (state/'experience.txt').read_text().split()==['0','1','1','0','0']
 assert 'sound:' not in run(events='d').stderr
 run('settings','a'+'d'*19+'a');assert (state/'experience.txt').read_text().split()==['0','0','1','0','0']
 run('settings','a'+'d'*4+'a');assert (state/'experience.txt').read_text().split()==['0','0','0','0','0']
 checks.append('UI sound, startup sound and system-icon preferences persist independently')
 (state/'experience.txt').write_text('1 1 1\n');assert 'sound: move' in run(events='d').stderr
 checks.append('Re-enabling UI audio restores valid navigation sound events')
 for screen in ('home','settings','horizontal','apps'):
  off=run(screen,render=True,extra={'DS_STYLE_PREVIEW_WIFI':'0'});on=run(screen,render=True,extra={'DS_STYLE_PREVIEW_WIFI':'1'})
  changed=ImageChops.difference(off,on).getbbox()
  if screen=='home':on.save(P/'preview/home-wifi.png')
  if screen in ('home','settings'):assert changed and 153*3<=changed[0]<166*3
  else:assert changed is None
 checks.append('Wi-Fi appears only when connected, on home/settings; never on browsers or Apps')
 frames=[]
 for ms in range(0,201,16):frames.append(run(events='d',render=True,extra={'DS_STYLE_PREVIEW_ANIM_MS':str(ms)}))
 assert len({f.tobytes() for f in frames})==len(frames)
 frames.append(run(events='d',render=True,extra={'DS_STYLE_PREVIEW_ANIM_MS':'240'}))
 frames[0].save(P/'preview/home-glide.gif',save_all=True,append_images=frames[1:],duration=[20]*(len(frames)-1)+[600],loop=0)
 checks.append('200 ms glide produces distinct intermediate frames at every 16 ms sample')
 popup=run('horizontal',render=True,extra={'DS_STYLE_PREVIEW_LAUNCHING':'1'});popup.save(P/'preview/launching.png')
 assert popup.getpixel((67*3,66*3))==(73,73,73) and popup.getpixel((172*3,93*3))==(73,73,73)
 checks.append('Launching popup uses the compact 106x28 logical-pixel notice geometry')
 # The real stream mixer writes a silent warmup, complete voices and a silent
 # drain. It is tested at mute/low/mid/full volume, including short move/back.
 for level in range(21):
  for event in ('4','1','0','3','6','5','2'):
   capture=root/'stream.pcm';env=os.environ.copy();env.update(AUDIO_CAPTURE=str(capture),AUDIO_EVENTS=event,AUDIO_LEVEL=str(level))
   r=subprocess.run([str(P/'preview/audio-stream-test.exe'),str(base)],env=env,capture_output=True,text=True);assert r.returncode==0,r.stderr
   out=np.frombuffer(capture.read_bytes(),'<i2').reshape(-1,2)
   name=['accept','back','launch','menu','move','startup','tab'][int(event)]
   with wave.open(str(base/'assets/sounds'/f'{name}.wav')) as w:sample=np.frombuffer(w.readframes(w.getnframes()),'<i2').reshape(-1,2).astype(np.int64)
   n=len(sample);edge=np.minimum(np.minimum(np.arange(n),np.arange(n)[::-1]),96)
   faded=np.trunc(sample*edge[:,None]/96).astype(np.int64);balanced=np.trunc(faded*[200,350,200,200,200,105,200][int(event)]/200).astype(np.int64)
   q=[0,84,211,581,1460,3050,5181,6522,8211,10337,16384];lo=level//2;g=q[lo]/16384
   if level%2:g=(g*q[lo+1]/16384)**.5 if lo else q[1]/32768
   nav=int(375*g**.65+.5) if level else 0;old=[0,12,30,55,90,135,185,245,315,400,500];boot=(old[lo]+old[lo+1])//2 if level%2 else old[lo]
   expected=np.trunc(balanced*(boot if event=='5' else nav)/1000).astype('<i2')
   assert np.array_equal(out[6144:6144+n],expected),(name,level)
   assert not np.any(out[:6144]) and not np.any(out[6144+n:]) and len(out)-6144-n>=63*256
 checks.append('All seven sounds retain every sample and their full tail at all 21 volume levels, including startup')
 # The exact FIFO header is independently decoded as stereo PCM WAV.
 import re,struct
 h=(P/'source/stock_stream.h').read_text();text=h.split('unsigned char h[44]={',1)[1].split('}',1)[0]
 values=[ord(x[1]) if x.startswith("'") else int(x,0) for x in text.split(',')];hdr=bytes(values)
 assert hdr[:4]==b'RIFF' and hdr[8:16]==b'WAVEfmt ' and struct.unpack_from('<HHIIHH',hdr,20)==(1,2,48000,192000,4,16)
 checks.append('Continuous stock-player FIFO header describes the actual PCM stream')
(P/'docs/v25-test-results.json').write_text(json.dumps({'passed':len(checks),'checks':checks,'hardware_tested':False},indent=2)+'\n')
print('\n'.join(checks))
