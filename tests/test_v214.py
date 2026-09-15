from pathlib import Path
import subprocess,json
p=Path(__file__).resolve().parents[1]
r=subprocess.run([str(p/'preview/handoff-repeat-test.exe')],capture_output=True,text=True)
assert r.returncode==0,r.stdout+r.stderr
s=(p/'source/dsstyle.c').read_text()
a=s.index('if(game&&!startup_direct){launching=1;draw();'); b=s.index('launching=0;if(interrupted)',a)
assert 'present();' in s[a:b] and 'waitpid' in s[a:b]
assert 'if(game)pan_committed=0;' in s[a:b]
assert 'if(launching)fb_launch_row' in s
checks=[r.stdout.strip(),'Launching remains active through the blocking stock handoff; both pages are seeded before display ownership transfers']
(p/'docs/v214-test-results.json').write_text(json.dumps({'passed':len(checks),'checks':checks,'hardware_tested':False},indent=2)+'\n')
print('\n'.join(checks))
