"""New navigation, preference and artwork acceptance checks on the real renderer."""
from pathlib import Path
from PIL import Image
import tempfile,shutil,subprocess,os,json
P=Path(__file__).resolve().parents[1];D=P/'device/Roms/APPS/DSStyle';checks=[]
with tempfile.TemporaryDirectory() as tmp:
 root=Path(tmp);base=root/'DSStyle';shutil.copytree(D/'assets',base/'assets');state=base/'state';state.mkdir();roms=root/'Roms';gba=roms/'GBA';gba.mkdir(parents=True)
 for n in ('A Game','B Game','C Game'):(gba/(n+'.gba')).write_bytes(b'x')
 def run(events='',screen='home',render=False,elapsed=0):
  args=[str(P/'preview/dsstyle-preview.exe'),'--base',str(base),'--roms',roms.as_posix(),'--screen',screen,'--events',events]
  args+=['--render',str(root/'screen.bmp')] if render else ['--self-test']
  env=os.environ.copy();env['DS_STYLE_PREVIEW_SCROLL_MS']=str(elapsed)
  r=subprocess.run(args,capture_output=True,text=True,env=env);assert r.returncode==0,r.stderr
  return Image.open(root/'screen.bmp').convert('RGB').resize((240,160),Image.Resampling.NEAREST) if render else r
 def interface(home=0): (state/'interface.txt').write_text(f'0 0 0 1 {home}\n')
 (state/'favourites.txt').write_text((gba/'B Game.gba').as_posix()+'\n'+(gba/'C Game.gba').as_posix()+'\n')
 (state/'recent.txt').write_text((gba/'A Game.gba').as_posix()+'\n')
 for hb,n in ((0,3),(1,2),(2,1)):
  interface(hb);r=run('dra');assert f'page=1 entries={n}' in r.stdout,(hb,r.stdout)
  assert 'page=0' in run('drab').stdout
 checks.append('Home Apps/Favs/Recents button opens the chosen destination and B returns home')
 interface(1);assert 'page=2' in run('madadb').stdout
 checks.append('System > Apps remains accessible and returns to settings')
 run('ma'+'d'*17+'a');assert (state/'interface.txt').read_text().split()[-1]=='2'
 checks.append('Home button setting cycles and persists independently of quick-launch source')
 run('datb');assert (state/'launch-modes/GBA.txt').read_text().strip()=='gameroom'
 run('data');assert (state/'launch-modes/GBA.txt').read_text().strip()=='retroarch'
 before=(state/'launch-modes/GBA.txt').read_bytes();r=run('daat');assert (state/'launch-modes/GBA.txt').read_bytes()==before and 'page=1 entries=3' in r.stdout
 checks.append('START selects per-system launch mode only on the Systems overview')
 run('se');assert (state/'home.txt').read_text().splitlines()==['1',(gba/'C Game.gba').as_posix()]
 assert 'quick=1' in run().stdout
 (state/'favourites.txt').write_text((gba/'C Game.gba').as_posix()+'\n'+(gba/'B Game.gba').as_posix()+'\n')
 assert 'quick=0' in run().stdout
 run('s');assert (state/'home.txt').read_text().splitlines()[0]=='0' and 'quick=0' in run().stdout
 checks.append('Favourite quick-launch persists by exact path across reorder; recents restarts at the newest game')
 (state/'style.txt').write_text('0 2 0 0 0 0 2\n');assert 'effective=1' in run('da',screen='home').stdout
 (state/'preferences.txt').write_text('v2 2 0 0 1 0 0\n');assert 'effective=2' in run('daa').stdout
 checks.append('List folders: List + Art applies only to folder-only screens')
 # Stock-style ROM artwork and missing-art list behaviour.
 (state/'style.txt').write_text('0 2 0 0 0 0 1\n');(state/'preferences.txt').write_text('v2 1 0 0 1 0 0\n')
 imgs=gba/'Imgs';imgs.mkdir();Image.new('RGB',(80,80),(0,255,0)).save(imgs/'A Game.gba.png')
 im=run('daa',render=True);assert im.getpixel((231,50))==(0,255,0) and im.getpixel((232,50))!=(0,255,0)
 assert im.getpixel((172,50))==(0,255,0) and im.getpixel((171,50))!=(0,255,0)
 missing=run('daad',render=True);assert (0,255,0) not in list(missing.get_flattened_data())
 checks.append('List + Art keeps square art at the right padding and omits game placeholders')
 font=(base/'assets/font.bin').read_bytes();x=17+len('B Game ')*6;y=20+14
 # Locate the literal <3 glyphs in the selected favourite row.
 for col,c in enumerate('<3'):
  for yy,bits in enumerate(font[ord(c)*12:ord(c)*12+12]):
   for xx in range(8):
    if bits&(128>>xx):assert missing.getpixel((x+col*6+xx,y+yy))==(255,255,255),(x,y,c)
 checks.append('Favourites append the literal <3 in list titles')
 long='A very long game title that scrolls all the way beyond the artwork region'
 (gba/'A Game.gba').rename(gba/(long+'.gba'));(imgs/'A Game.gba.png').rename(imgs/(long+'.gba.png'))
 still=run('daa',render=True);scroll=run('daa',render=True,elapsed=1400)
 assert still.crop((17,20,169,32)).tobytes()!=scroll.crop((17,20,169,32)).tobytes()
 assert still.crop((172,27,232,87)).tobytes()==scroll.crop((172,27,232,87)).tobytes()
 checks.append('Selected long titles scroll in the left region while artwork remains unchanged')
 interface(1);im=run(render=True);im.resize((720,480),Image.Resampling.NEAREST).save(P/'preview/home-favs.png')
 im=run('dat',render=True);im.resize((720,480),Image.Resampling.NEAREST).save(P/'preview/launch-mode.png')
(P/'docs/v24-test-results.json').write_text(json.dumps({'passed':len(checks),'checks':checks,'hardware_tested':False},indent=2)+'\n')
print('\n'.join(checks))
