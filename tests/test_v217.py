from pathlib import Path
import subprocess,json,re
p=Path(__file__).resolve().parents[1]
out=p/'preview/v217';out.mkdir(exist_ok=True)
r=subprocess.run([str(p/'preview/v217-test.exe'),str(p/'device/Roms/APPS/DSStyle'),str(out)],capture_output=True,text=True)
assert r.returncode==0,r.stdout+r.stderr
assert all((out/name).is_file() for name in ['about.bmp','snake.bmp','snake-over.bmp'])
checks=r.stdout.strip().splitlines()
assert 'pixel-identical' in checks[0]
times=dict((v,int(ms))for v,ms in re.findall(r'home benchmark (v\d+) frames=1500 ms=(\d+)',r.stdout))
report={'passed':len(checks),'checks':checks,'assertions_enabled':True,'hardware_tested':False,'home_cpu_ms_1500_frames':times}
(p/'docs/v217-test-results.json').write_text(json.dumps(report,indent=2)+'\n')
print(r.stdout)
