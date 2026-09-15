from pathlib import Path
from PIL import Image,ImageOps
import subprocess,tempfile,shutil,json,re,hashlib
P=Path(__file__).resolve().parents[1];D=P/'device/Roms/APPS/DSStyle';checks=[]
def visible_hash(f):
 im=Image.open(f).convert('RGBA');im.putdata([(r,g,b,a) if a>127 and(r or g or b) else (0,0,0,0) for r,g,b,a in im.getdata()]);return hashlib.sha256(im.tobytes()).hexdigest()

r=subprocess.run([str(P/'preview/pt-cache-test.exe'),str(D)],capture_output=True,text=True);assert r.returncode==0,r.stderr
m=re.search(r'v29_comparison frames=(\d+) old_cached_ms=(\d+) new_cached_ms=(\d+)',r.stdout);assert m
benchmark=dict(zip(['frames','v29_ms','v210_ms'],map(int,m.groups())))
checks.append('Per-pixel and row caches match the full reference with blur both on and off, including clamped top/left shadows and LCD/dark transitions')
assert (D/'assets/icons/icon_apps.png').exists()
checks.append('User Apps cartridge remains available')
for name,digest in json.loads((P/'docs/v210-preserved-icons.json').read_text()).items():assert visible_hash(D/'assets/icons'/name)==digest,name
checks.append('System icons preserve their visible pixels except the requested DS and user NES edits')
for f in (D/'assets/icons').glob('*.png'):
  im=Image.open(f).convert('RGBA');assert all(a==0 for r,g,b,a in im.getdata() if not(r or g or b)),f
checks.append('All PNG icons use actual alpha transparency for their former black-key backgrounds')
im=Image.open(D/'assets/icons/icon_cart.png');box=im.getbbox();assert box[0]>=1 and box[1]>=1 and box[2]<=15 and box[3]<=13
checks.append('Generic cartridge uses a wide GBA shell with transparent padding')
benchmark['blur_comparison']=r.stdout.split('blur_comparison')[1:]
with tempfile.TemporaryDirectory() as tmp:
 root=Path(tmp);base=root/'DSStyle';shutil.copytree(D/'assets',base/'assets');state=base/'state';state.mkdir()
 def run(events,render=False):
  r=subprocess.run([str(P/'preview/dsstyle-preview.exe'),'--base',str(base),'--demo','--screen','settings','--events',events]+(['--render',str(root/'screen.bmp')] if render else ['--self-test']),capture_output=True,text=True);assert r.returncode==0,r.stderr
 for index,file,token,want in [(0,'preferences.txt',1,'2'),(13,'experience.txt',3,'1'),(15,'experience.txt',4,'1'),(16,'interface.txt',0,'1'),(17,'interface.txt',4,'1'),(18,'experience.txt',0,'0'),(19,'experience.txt',1,'0')]:
  for f in state.iterdir():
   if f.is_file():f.unlink()
  run('a'+'d'*index+'a');assert (state/file).read_text().split()[token]==want,(index,(state/file).read_text())
 checks.append('Reordered browsing, dark/filter, clock/home and sound entries change and persist their intended setting')
 run('dda',True);Image.open(root/'screen.bmp').save(P/'preview/help-v210.png')
 run('a',True);Image.open(root/'screen.bmp').save(P/'preview/interface-v210.png')
 checks.append('Help and reordered Interface render with the original DS Style text-screen layout')
(P/'docs/v210-test-results.json').write_text(json.dumps({'passed':len(checks),'checks':checks,'benchmark':benchmark,'benchmark_platform':'Windows host, small motion; not device FPS','hardware_tested':False},indent=2)+'\n');print('\n'.join(checks));print(benchmark)
