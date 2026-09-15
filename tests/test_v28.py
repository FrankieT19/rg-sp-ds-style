from pathlib import Path
from PIL import Image
import tempfile,shutil,subprocess,json,hashlib,time
P=Path(__file__).resolve().parents[1];D=P/'device/Roms/APPS/DSStyle';checks=[]
def visible_hash(f):
 im=Image.open(f).convert('RGBA');im.putdata([(r,g,b,a) if a>127 and(r or g or b) else (0,0,0,0) for r,g,b,a in im.getdata()]);return hashlib.sha256(im.tobytes()).hexdigest()

with tempfile.TemporaryDirectory() as tmp:
 root=Path(tmp);base=root/'DSStyle';shutil.copytree(D/'assets',base/'assets');state=base/'state';state.mkdir();pictures=[]
 def run(name='check',selftest=False):
  out=root/(name+'.bmp');start=time.perf_counter();r=subprocess.run([str(P/'preview/dsstyle-preview.exe'),'--base',str(base),'--demo','--screen','home']+(['--self-test'] if selftest else ['--render',str(out)]),capture_output=True,text=True);assert r.returncode==0,r.stderr
  return (time.perf_counter()-start) if selftest else Image.open(out).convert('RGB')
 for lcd,pt in [(0,0),(1,0),(0,1),(1,1)]:
  (state/'experience.txt').write_text(f'1 1 1 0 {pt}\n');(state/'preferences.txt').write_text(f'v2 1 0 {lcd} 1 0 0\n');im=run();pictures.append(im);im.save(P/'preview'/f'filter-lcd{lcd}-transparency{pt}.png');run(selftest=True)
 assert len({im.tobytes() for im in pictures})==4
 checks.append('LCD and Pixel Transparency independently change output and stack; reference/scanline sampling agrees')
 (state/'experience.txt').write_text('1 1 1 0 0\n');(state/'preferences.txt').write_text('v2 1 0 0 1 0 0\n');assert run().tobytes()==pictures[0].tobytes()
 checks.append('Disabling filters restores the original unfiltered pixels exactly')
 (state/'experience.txt').write_text('1 1 1 0\n');assert run().tobytes()==pictures[0].tobytes()
 checks.append('Existing four-field experience settings default the new filter to off')
 (state/'experience.txt').write_text('1 1 1 1 1\n');run(selftest=True)
 checks.append('Pixel Transparency also renders correctly with dark mode')
# All built-in system compositions must be high-res and white, including aliases.
count=0
for shape,size in [('wide',(480,320))]:
 for f in (D/'assets/systems'/shape).glob('*.png'):
  im=Image.open(f).convert('RGB');assert im.size==size,(f,im.size)
  assert all(im.getpixel(x)==(255,255,255) for x in [(0,0),(im.width-1,0),(0,im.height-1),(im.width-1,im.height-1)]),f;count+=1
checks.append(f'All {count} built-in system images and aliases use high-resolution white compositions')
for name,digest in json.loads((P/'docs/v29-preserved-icons.json').read_text()).items():
 assert visible_hash(D/'assets/icons'/name)==digest,name
checks.append('PSP and the retained original handheld icons retain identical visible pixels after transparency normalization')
for name in ['GB','NDS','PS','PSP','SFC','N64','MD','DREAMCAST','SATURN','apps']:
 im=Image.open(D/'assets/icons'/('icon_'+name+'.png')).convert('RGBA');bbox=im.getbbox();assert bbox and bbox[0]>=1 and bbox[1]>=1 and bbox[2]<=15 and bbox[3]<=13,(name,bbox)
checks.append('Every revised sprite has transparent padding to prevent adjacent rows touching')
(P/'docs/v28-test-results.json').write_text(json.dumps({'passed':len(checks),'checks':checks,'hardware_tested':False},indent=2)+'\n');print('\n'.join(checks))
