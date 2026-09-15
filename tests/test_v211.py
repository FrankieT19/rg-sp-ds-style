from pathlib import Path
from PIL import Image
import hashlib,json,subprocess
P=Path(__file__).resolve().parents[1];D=P/'device/Roms/APPS/DSStyle';checks=[]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
for n,h in json.loads((P/'docs/v212-preserved-icons.json').read_text()).items():assert sha(D/'assets/icons'/n)==h,n
checks.append('All unrequested icons, including the user-edited NES controller, are byte-identical to the card backup')
# Apps and cart may now be individually edited by the user.
for n in ['NDS','cart','apps']:
 im=Image.open(D/'assets/icons'/('icon_'+n+'.png')).convert('RGBA');box=im.getbbox();assert box[1]>=1 and box[3]<=13,(n,box)
 assert not any(im.getpixel((x,0))[3] or im.getpixel((x,13))[3] for x in range(16))
checks.append('DS, cartridge and user Apps retain transparent top and bottom rows')
(P/'docs/v211-test-results.json').write_text(json.dumps({'passed':len(checks),'checks':checks,'hardware_tested':False},indent=2)+'\n');print('\n'.join(checks))
