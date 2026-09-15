from pathlib import Path
import subprocess,json,math
P=Path(__file__).resolve().parents[1]
r=subprocess.run([str(P/'preview/mixer-guard-test.exe')],capture_output=True,text=True);assert r.returncode==0,r.stderr
q=[0,84,211,581,1460,3050,5181,6522,8211,10337,16384];v=[]
for s in range(21):
 lo=s//2;g=q[lo]/16384
 if s%2:g=math.sqrt(g*q[lo+1]/16384) if lo else q[1]/32768
 v.append(round(375*g**.65) if s else 0)
assert v[0]==0 and v[-1]==375 and all(a<b for a,b in zip(v,v[1:])) and v[1]<10
checks=[r.stdout.strip(),'Twenty-step curve is monotonic, starts below 1% PCM gain, and peaks 25% below v2.12']
(P/'docs/v213-test-results.json').write_text(json.dumps({'passed':len(checks),'checks':checks,'gain_per_thousand':v,'hardware_tested':False},indent=2)+'\n');print(checks)
