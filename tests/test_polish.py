"""Pixel geometry and real stock-player WAV preparation tests; does not claim device validation."""
from pathlib import Path
import tempfile,shutil,subprocess,os,json,wave
import numpy as np
from PIL import Image
P=Path(__file__).resolve().parents[1];D=P/'device/Roms/APPS/DSStyle';checks=[]
with tempfile.TemporaryDirectory() as tmp:
 root=Path(tmp);base=root/'DSStyle';shutil.copytree(D/'assets',base/'assets');(base/'state').mkdir()
 def render(shape=0,border=0,rounded=0,screen='horizontal',events=''):
  (base/'state/style.txt').write_text(f'{border} {shape} 1 {rounded} 0 0 1\n')
  out=root/'screen.bmp';r=subprocess.run([str(P/'preview/dsstyle-preview.exe'),'--base',str(base),'--demo','--screen',screen,'--events',events,'--render',str(out)],capture_output=True,text=True);assert r.returncode==0,r.stderr
  return Image.open(out).convert('RGB').resize((240,160),Image.Resampling.NEAREST)
 # Folder overrides are case-insensitive, support full names, and retain aspect.
 artdir=base/'Folder Art';artdir.mkdir()
 Image.new('RGB',(160,80),(0,255,0)).save(artdir/'Game Boy Advance.png')
 # Demo fallback knows the .gba extension even without a system folder.
 roms=root/'GBA';roms.mkdir()
 for name in ('Aster Trail','Garden Quest','Pocket Rally'):(roms/(name+'.gba')).write_bytes(b'x')
 def art_render(screen='horizontal',fit=1,side=0,border=0,rounding=0,gba=0):
  (base/'state/style.txt').write_text(f'{border} 2 0 {rounding} {side} 0 1\n')
  (base/'state/interface.txt').write_text(f'0 0 {gba} {fit}\n')
  out=root/'screen.bmp';r=subprocess.run([str(P/'preview/dsstyle-preview.exe'),'--base',str(base),'--roms',roms.as_posix(),'--screen',screen,'--events','d' if screen=='vertical' else 'r','--render',str(out)],capture_output=True,text=True);assert r.returncode==0,r.stderr
  return Image.open(out).convert('RGB')
 im=art_render().resize((240,160),Image.Resampling.NEAREST)
 assert im.getpixel((60,37))==(0,255,0) and im.getpixel((179,96))==(0,255,0)
 checks.append('Long-name Folder Art override keeps full 2:1 image within horizontal slot')
 im=art_render(fit=0,border=2).resize((240,160),Image.Resampling.NEAREST)
 assert im.getpixel((40,27))==(0,255,0) and im.getpixel((199,106))==(0,255,0)
 assert im.getpixel((39,50))==(0,0,0) and im.getpixel((200,50))==(0,0,0)
 checks.append('Horizontal overlap retains full height and adaptive border')
 im=art_render(fit=0,border=2,rounding=1).resize((240,160),Image.Resampling.NEAREST);a=np.asarray(im);mask=np.all(a==[0,255,0],axis=2)
 # Main image perimeter, excluding the intentionally overlapping side images.
 for y,x in np.argwhere(mask):
  if not(39<x<200 and 0<y<159):continue
  for dy,dx in ((-1,0),(1,0),(0,-1),(0,1)):
   if not mask[y+dy,x+dx]:assert tuple(a[y+dy,x+dx])==(0,0,0),(x,y,dx,dy)
 checks.append('Rounded image perimeter remains enclosed')
 Image.new('RGB',(80,80),(0,255,0)).save(artdir/'Game Boy Advance.png')
 for side,start,end in [(0,33,64),(1,7,38),(2,59,90)]:
  im=art_render('vertical',side=side).resize((240,160),Image.Resampling.NEAREST)
  assert im.getpixel((21,62))==(0,255,0) and im.getpixel((76,117))==(0,255,0)
  assert im.getpixel((start,25))==(0,255,0) and im.getpixel((end,25))==(0,255,0)
 checks.append('Vertical square main is centered; side artwork aligns actual edges to usable area')
 # Native sampling must preserve detail that the optional GBA grid discards.
 high=Image.new('RGB',(360,240));pixels=high.load()
 for y in range(240):
  for x in range(360):pixels[x,y]=(255,0,0) if x%3==0 else (0,255,0)
 high.save(artdir/'Game Boy Advance.png')
 native=art_render();gba=art_render(gba=1)
 assert native.getpixel((180,81))!=native.getpixel((181,81))
 assert gba.getpixel((180,81))==gba.getpixel((181,81))
 checks.append('Native image detail and optional GBA pixel scaling are distinct')
 # Exercise the actual stock-player WAV preparation code, at every hardware
 # volume level. Every sample, frame count and edge fade is checked independently.
 for level in (0,1,5,10):
  events='0123456';capture=root/'audio';env=os.environ.copy();env.update(AUDIO_CAPTURE=str(capture),AUDIO_EVENTS=events,AUDIO_LEVEL=str(level))
  r=subprocess.run([str(P/'preview/audio-offline-test.exe'),str(base)],env=env,capture_output=True,text=True);assert r.returncode==0,r.stderr
  for i,event in enumerate(events):
   name=['accept','back','launch','menu','move','startup','tab'][int(event)]
   with wave.open(str(base/'assets/sounds'/f'{name}.wav')) as w:sample=np.frombuffer(w.readframes(w.getnframes()),'<i2').reshape(-1,2).astype(np.int64)
   with wave.open(str(capture)+f'-{i}.wav') as w:
    assert (w.getnchannels(),w.getsampwidth(),w.getframerate())==(2,2,48000)
    actual=np.frombuffer(w.readframes(w.getnframes()),'<i2').reshape(-1,2)
   n=len(sample);gain=np.minimum(np.minimum(np.arange(n),np.arange(n)[::-1]),96)
   expected=np.trunc(sample*gain[:,None]*level/960).astype('<i2')
   assert np.array_equal(actual,expected),(name,level)
 checks.append('Stock-player WAVs preserve full duration and exact samples with edge fades at mute, low, mid and full volume')
 for name in ('RESET','POWER'):
  button=Image.open(D/'assets'/f'{name}.png');assert button.size==(14,14)
 checks.append('User-edited power icons keep their 14x14 dimensions')

(P/'docs/polish-test-results.json').write_text(json.dumps({'passed':len(checks),'checks':checks,'hardware_tested':False},indent=2)+'\n')
print('\n'.join(checks))
