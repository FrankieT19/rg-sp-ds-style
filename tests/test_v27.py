from pathlib import Path
import tempfile,shutil,subprocess,os,json,re
P=Path(__file__).resolve().parents[1];D=P/'device/Roms/APPS/DSStyle';checks=[];bench=[]
with tempfile.TemporaryDirectory() as tmp:
 base=Path(tmp)/'DSStyle';shutil.copytree(D/'assets',base/'assets');state=base/'state';state.mkdir()
 def run(events='',screen='home',benchmark=False):
  env=os.environ.copy()
  if benchmark:env['DS_STYLE_BENCHMARK']='1'
  r=subprocess.run([str(P/'preview/dsstyle-preview.exe'),'--base',str(base),'--demo','--screen',screen,'--events',events,'--self-test'],env=env,capture_output=True,text=True)
  assert r.returncode==0,r.stderr
  return r
 for screen in ('home','settings','horizontal','vertical','list','art'):
  for dark in (0,1):
   (state/'experience.txt').write_text(f'1 1 1 {dark}\n')
   run(screen=screen)
 checks.append('LCD cached scanlines exactly match reference output across six views in light and dark modes')
 (state/'experience.txt').write_text('1 1 1 0\n')
 for screen in ('home','settings','horizontal'):
  r=run(screen=screen,benchmark=True)
  rows=re.findall(r'lcd_benchmark path=(\w+) frames=100 ms=(\d+) checksum=(\d+)',r.stdout)
  assert len(rows)==2 and rows[0][2]==rows[1][2],r.stdout
  bench.append({'screen':screen,'reference_ms':int(rows[0][1]),'cached_ms':int(rows[1][1]),'checksum':rows[0][2]})
 checks.append('LCD renderer benchmark processes identical complete frames with matching checksums')
 for prefix,mode in [('ddra','reboot'),('ddrra','shutdown')]:
  r=run(prefix+'a');lines=r.stderr.splitlines();idx=lines.index('power: starting '+mode+' handoff')
  assert lines[idx-1]=='sound: accept',lines
  assert not any(x=='sound: accept' for x in lines[idx+1:]),lines
  r=run(prefix+'b');assert 'power: starting' not in r.stderr and 'sound: back' in r.stderr
 checks.append('Reboot/shutdown acceptance plays once before handoff; cancellation only plays Back')
 (state/'experience.txt').write_text('0 1 1 0\n');r=run('ddrraa');assert 'power: starting shutdown handoff' in r.stderr and 'sound:' not in r.stderr
 checks.append('Shutdown feedback respects the UI-sounds toggle')
(P/'docs/v27-test-results.json').write_text(json.dumps({'passed':len(checks),'checks':checks,'benchmark':bench,'benchmark_platform':'Windows host CPU, 100 frames; not RG SP panel FPS','hardware_tested':False},indent=2)+'\n')
print('\n'.join(checks));print(json.dumps(bench,indent=2))
