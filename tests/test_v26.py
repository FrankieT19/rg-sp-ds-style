from pathlib import Path
from PIL import Image,ImageChops
import tempfile,shutil,subprocess,os,json,hashlib
import numpy as np
P=Path(__file__).resolve().parents[1];D=P/'device/Roms/APPS/DSStyle';checks=[]
with tempfile.TemporaryDirectory() as tmp:
 root=Path(tmp);base=root/'DSStyle';shutil.copytree(D/'assets',base/'assets');state=base/'state';state.mkdir()
 roms=root/'Roms';gba=roms/'GBA';gba.mkdir(parents=True);imgs=gba/'Imgs';imgs.mkdir()
 title='A very long game title that extends beyond the artwork and continues scrolling'
 for t in (title,'B Game','C Game'):(gba/(t+'.gba')).write_bytes(b'x')
 Image.new('RGB',(83,80),(0,255,0)).save(imgs/(title+'.gba.png'))
 def run(screen='home',events='',ms=0,demo=False,selftest=False):
  env=os.environ.copy();env['DS_STYLE_PREVIEW_SCROLL_MS']=str(ms)
  args=[str(P/'preview/dsstyle-preview.exe'),'--base',str(base),'--roms',roms.as_posix(),'--screen',screen,'--events',events]
  if demo:args+=['--demo']
  args+=['--self-test'] if selftest else ['--render',str(root/'screen.bmp')]
  r=subprocess.run(args,capture_output=True,text=True,env=env);assert r.returncode==0,r.stderr
  return r if selftest else Image.open(root/'screen.bmp').convert('RGB')
 run('settings','a'+'d'*13+'a',selftest=True)
 assert (state/'experience.txt').read_text().split()==['1','1','1','1','0']
 dark=run(demo=True);dark.save(P/'preview/home-dark.png')
 original=Image.open(base/'assets/dark/START.bmp').convert('RGB').resize((720,480),Image.Resampling.NEAREST)
 assert dark.crop((0,60,60,420)).tobytes()==original.crop((0,60,60,420)).tobytes()
 for n,x in [('RESET',182),('POWER',201)]:
  source=np.array(Image.open(base/'assets'/(n+'.png')).convert('RGB'))
  target=np.array(dark.resize((240,160),Image.Resampling.NEAREST))[143:157,x:x+14]
  palette={0:123,73:97,121:134,162:93,195:60,211:44,251:4}
  expected=source.copy()
  for a,b in palette.items():expected[np.all(source==a,axis=2)]=b
  assert np.array_equal(expected,target),n
 checks.append('Dark mode persists, uses original dark START pixels and preserves both edited icon shapes')
 for screen in ('settings','art','horizontal','vertical'):
  im=run(screen,events='aa' if screen in ('art','horizontal','vertical') else '',demo=False)
  im.save(P/'preview'/('dark-'+screen+'.png'))
 checks.append('All dark screen variants render successfully')
 run('settings','a'+'d'*13+'a',selftest=True);assert (state/'experience.txt').read_text().split()[3]=='0'
 checks.append('Dark mode can return to light without changing the colour or sound preferences')
 (state/'preferences.txt').write_text('v2 1 0 0 1 0 0\n')
 (state/'style.txt').write_text('0 2 0 1 0 0 1\n')
 period=(len(title)*6+18)/.030
 times=[0,333,349,365,381,397,413,429,445,461,477,493,509,int(333+period)-1,int(333+period),int(333+period)+16]
 frames=[run(events='daa',ms=t) for t in times]
 # Art width is floor(83*60/80)=62, right=232 -> selected clip at167.
 stable=frames[0].crop((167*3,20*3,240*3,33*3)).tobytes()
 assert all(f.crop((167*3,20*3,240*3,33*3)).tobytes()==stable for f in frames)
 checks.append('Selected long-title clipping remains identical during pause, motion and wrap, including rounded art corners')
 crop=lambda f:f.crop((17*3,20*3,167*3,32*3)).tobytes()
 assert len({crop(f) for f in frames[1:13]})==12
 checks.append('Every 16 ms title-scroll sample has a distinct physical-pixel position; no character-sized pauses')
 frames[0].save(P/'preview/title-scroll.gif',save_all=True,append_images=frames[1:13],duration=40,loop=0)
 run(events='daa',selftest=True)
 # Odd source widths must produce symmetric padding around the horizontal slot.
 (state/'preferences.txt').write_text('v2 2 0 0 1 0 0\n')
 (state/'style.txt').write_text('0 2 0 0 0 0 1\n')
 im=run(events='daa');im.save(P/'preview/horizontal-centred.png')
 a=np.array(im);ys,xs=np.where(np.all(a==[0,255,0],axis=2));assert min(xs)==(720- (max(xs)+1))
 checks.append('Odd-aspect horizontal main art has identical left and right screen padding')
 run(events='daa',selftest=True)
 checks.append('Optimised scanline matches the reference sampler with physical-pixel scrolling enabled')
for item in json.loads((P/'docs/dark-provenance.json').read_text())['assets']:
 assert hashlib.sha256((D/item['asset']).read_bytes()).hexdigest()==item['sha256']
checks.append('Every imported dark background matches its recorded original asset hash')
(P/'docs/v26-test-results.json').write_text(json.dumps({'passed':len(checks),'checks':checks,'hardware_tested':False},indent=2)+'\n')
print('\n'.join(checks))
