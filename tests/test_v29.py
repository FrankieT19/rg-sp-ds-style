from pathlib import Path
from PIL import Image
import subprocess,tempfile,shutil,json,re,hashlib
P=Path(__file__).resolve().parents[1];D=P/'device/Roms/APPS/DSStyle';checks=[]
r=subprocess.run([str(P/'preview/pt-cache-test.exe'),str(D)],capture_output=True,text=True);assert r.returncode==0,r.stderr
checks.append('Cached Pixel Transparency exactly matches the full renderer with blur on and off across 144 moving light/dark/LCD frames and screen-edge shadows')
m=re.search(r'frames=(\d+) reference_ms=(\d+) cached_ms=(\d+)',r.stdout);assert m,r.stdout
benchmark=dict(zip(['frames','reference_ms','cached_ms'],map(int,m.groups())))
with tempfile.TemporaryDirectory() as tmp:
 root=Path(tmp);base=root/'DSStyle';shutil.copytree(D/'assets',base/'assets');(base/'state').mkdir()
 out=root/'brightness.bmp';r=subprocess.run([str(P/'preview/dsstyle-preview.exe'),'--base',str(base),'--demo','--screen','brightness','--render',str(out)],capture_output=True,text=True);assert r.returncode==0,r.stderr
 im=Image.open(out).convert('RGB').resize((240,160),Image.Resampling.NEAREST)
 black={(x,y) for y in range(73,86) for x in range(57,70) if im.getpixel((x,y))==(0,0,0)}
 expected={(x,y) for y in range(77,82) for x in range(61,66) if x in (61,65) or y in (77,81)}|{(63,73),(63,74),(63,84),(63,85),(57,79),(58,79),(68,79),(69,79)}
 assert black==expected,(black-expected,expected-black);im.resize((720,480),Image.Resampling.NEAREST).save(P/'preview/brightness-cardinal-rays.png')
 checks.append('Brightness icon contains exactly the square outline and four straight rays, with no diagonal pixels')
# The enlarged GB occupies 12 of the 14 sprite rows, with transparent margins.
im=Image.open(D/'assets/icons/icon_GB.png');box=im.getbbox();assert box[3]-box[1]==12 and box[2]-box[0]>=9,box
checks.append('Game Boy is enlarged to 12 rows with at least nine columns of hardware')
for name in ['FC','PS','SFC','N64','MD','DREAMCAST','SATURN','SMS','PCE','MSX','apps','NDS']:
 im=Image.open(D/'assets/icons'/('icon_'+name+'.png')).convert('RGBA');b=im.getbbox();assert b[1]>=1 and b[3]<=13,(name,b)
checks.append('Controllers, open DS and cartridge retain a clear gap between adjacent list rows')
(P/'docs/v29-test-results.json').write_text(json.dumps({'passed':len(checks),'checks':checks,'benchmark':benchmark,'benchmark_platform':'Windows host; small moving regions, not RG SP FPS','hardware_tested':False},indent=2)+'\n')
print('\n'.join(checks));print(benchmark)
