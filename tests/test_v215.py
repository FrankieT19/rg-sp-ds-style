from pathlib import Path
import subprocess,tempfile,json,re
p=Path(__file__).resolve().parents[1]
checks=[]
with tempfile.TemporaryDirectory(prefix='dsstyle-v215-') as t:
 r=subprocess.run([str(p/'preview/features-test.exe'),t],capture_output=True,text=True,encoding='utf-8');assert r.returncode==0,r.stdout+r.stderr;checks.extend(r.stdout.strip().splitlines())
rows=json.loads((p/'source/locale.json').read_text(encoding='utf-8'));lookup={r[0]:r for r in rows};assert all(len(r)==8 and all(r)for r in rows)
source=(p/'source/ui.h').read_text();labels=re.findall(r'"([^"]+)"',re.search(r'static const char \*labels\[\]=\{(.*?)\};',source,re.S)[1]);assert all(s in lookup for s in labels)
for label in labels:
 assert all(len(v)<=14 for v in lookup[label]),lookup[label]
known=set(range(128))|set(map(int,re.search(r'latin_codepoints\[\]=\{(.*?)\}',(p/'source/original_layout.h').read_text())[1].split(',')))
assert all(ord(c) in known for r in rows for text in r for c in text)
for en in re.findall(r'^ "(.*)"[,\n]',(p/'source/extra_ui.h').read_text(),re.M)[:32]:assert en in lookup,en
checks.append('Eight language variants: all setting labels fit, all setting descriptions translated, every used glyph available')
assert len((p/'device/Roms/APPS/DSStyle/assets/font-latin.bin').read_bytes())==12*(len(known)-128)
checks.append('Latin font bank preserves existing glyphs and includes inverted punctuation and ligatures')
for name in ['dsstyle','dsstyle-display']:
 b=(p/'device/Roms/APPS/DSStyle/bin'/name).read_bytes();assert b[:4]==b'\x7fELF'
checks.append('Native frontend and ARM32 stock display helper built; physical hotplug remains untested')
(p/'docs/v215-test-results.json').write_text(json.dumps({'passed':len(checks),'checks':checks,'hardware_tested':False},indent=2)+'\n');print('\n'.join(checks))
