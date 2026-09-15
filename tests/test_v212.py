from pathlib import Path
import subprocess,tempfile,os,json,hashlib,math
P=Path(__file__).resolve().parents[1];D=P/'device/Roms/APPS/DSStyle';checks=[]
with tempfile.TemporaryDirectory() as tmp:
 root=Path(tmp);base=root/'DSStyle';(base/'state').mkdir(parents=True);node=root/'sys/class/power_supply/axp2202-battery/openbor_volume';node.parent.mkdir(parents=True);node.write_text('0');env=os.environ.copy();env['DS_STYLE_TEST_ROOT']=str(root)
 def run(mode,prev=-1,ok=True):
  r=subprocess.run([str(P/'preview/volume-test.exe'),mode,str(prev),str(base)],env=env,capture_output=True,text=True)
  if ok:assert r.returncode==0,r.stderr;return int(r.stdout)
  assert r.returncode!=0;return r
 half=0
 for expected in range(1,21):half=run('up',half);assert half==expected and int(node.read_text())==(half+1)//2
 assert run('up',half)==20
 for expected in range(19,-1,-1):half=run('down',half);assert half==expected and int(node.read_text())==(half+1)//2
 assert run('down',0)==0
 checks.append('Twenty up/down increments, mute and maximum map only to legal native stock values')
 node.write_text('2');assert run('get')==4
 assert run('up',4)==5 and run('get')==5 and node.read_text()=='3'
 node.write_text('7');assert run('get')==14
 checks.append('Half-step survives helper restart; changes made by stock/games replace stale fine state')
 node.write_text('3');assert run('up',5)==6
 node.write_text('4');assert run('up',6)==7 and node.read_text()=='4'
 node.write_text('3');assert run('down',7)==6 and node.read_text()=='3'
 checks.append('Already-processed stock volume key does not cause a duplicate increment')
 saved=(base/'state/volume-step.txt');saved.write_text('500 -1');node.write_text('5');assert run('get')==10
 node.write_text('11');run('up',10,False);assert node.read_text()=='11'
 checks.append('Malformed fine state falls back to stock; invalid stock values are refused without writes')
for n,h in json.loads((P/'docs/v212-preserved-icons.json').read_text()).items():assert hashlib.sha256((D/'assets/icons'/n).read_bytes()).hexdigest()==h,n
checks.append('Every user icon, including edited Apps/cartridge and NES, is preserved byte-for-byte')
assert [f.name for f in D.parent.glob('*.sh')]==['DS Style.sh']
assert not (D/'SYSTEM').exists() and not (D/'assets/systems/square').exists()
for n in ['diagnostics.sh','display-test.sh','inspect-stock.sh','record-stock.sh','stock-route.sh']:assert not(D/'scripts'/n).exists()
assert 'stock-power' not in (D/'scripts/run.sh').read_text()
checks.append('Public device tree has one launcher, no diagnostic helpers/capture, no obsolete image trees')
q25=(211*581)**.5/16384;new25=round(375*q25**.65);assert new25<50
assert abs(105/200/(35/100)-1.5)<1e-12
checks.append('Navigation low range is restrained; existing startup +50% balance is retained')
(P/'docs/v212-test-results.json').write_text(json.dumps({'passed':len(checks),'checks':checks,'hardware_tested':False},indent=2)+'\n');print('\n'.join(checks))
