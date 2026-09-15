"""Host tests of the actual stock settings writer and installed-table reader.
Pass --capture-dir with a private read-only capture of dmenu.bin/dmenu_attr.bin.
Firmware is never executed and the user's card is never written by this suite.
"""
from pathlib import Path
import argparse,os,subprocess,tempfile,struct,json,shutil,zlib
ap=argparse.ArgumentParser();ap.add_argument('--binary',required=True);ap.add_argument('--settings',required=True);args=ap.parse_args()
P=Path(__file__).resolve().parents[1];checks=[]
def crc(b):return zlib.crc32(b,0xffffffff)^0xffffffff
with tempfile.TemporaryDirectory() as tmp:
 root=Path(tmp);base=root/'DSStyle';(base/'state').mkdir(parents=True)
 attr=root/'mnt/data/dmenu/dmenu_attr.ini';attr.parent.mkdir(parents=True);original=Path(args.settings).read_bytes();assert len(original)==140 and crc(original[:136])==struct.unpack_from('<I',original,136)[0]
 attr.write_bytes(original);nodes=root/'sys/class/power_supply/axp2202-battery';nodes.mkdir(parents=True)
 env=os.environ.copy();env['DS_STYLE_TEST_ROOT']=root.as_posix()
 def state(mode):return subprocess.run([str(P/'preview/stock-state-test.exe'),base.as_posix(),mode],env=env,capture_output=True,text=True)
 (nodes/'brightness').write_text('0');(nodes/'openbor_volume').write_text('0')
 r=state('restore');assert r.returncode==0,r.stderr
 assert int((nodes/'brightness').read_text())==struct.unpack_from('<I',original,24)[0]
 assert int((nodes/'openbor_volume').read_text())==struct.unpack_from('<I',original,36)[0]
 assert attr.read_bytes()==original;checks.append('Boot restores the recorded stock brightness and volume without rewriting settings')
 for flags in (0,1,16,17):
  policy=bytearray(original);struct.pack_into('<I',policy,132,flags);struct.pack_into('<I',policy,136,crc(policy[:136]));attr.write_bytes(policy)
  assert state('sleep-policy').returncode==0
  assert int((nodes/'workled_sleep').read_text())==flags&1
  assert int((nodes/'os_sleep').read_text())==flags&16
  assert attr.read_bytes()==policy
 attr.write_bytes(original)
 checks.append('Stock sleep LED and standby bits are restored exactly without changing saved settings')
 (nodes/'brightness').write_text('6');(nodes/'openbor_volume').write_text('3');r=state('save');assert r.returncode==0,r.stderr
 after=attr.read_bytes();expected=bytearray(original);struct.pack_into('<I',expected,24,6);struct.pack_into('<I',expected,36,3);struct.pack_into('<I',expected,136,crc(expected[:136]));assert after==expected
 assert (base/'state/dmenu-attr-before.bin').read_bytes()==original;checks.append('Save changes only brightness, volume and CRC; original backup is byte-identical')
 r=state('save');assert r.returncode==0 and attr.read_bytes()==after;checks.append('Unchanged state produces no data changes')
 for invalid in (original[:-4],original+b'x',b'x'+original[1:]):
  attr.write_bytes(invalid);r=state('save');assert r.returncode!=0 and attr.read_bytes()==invalid
 checks.append('Truncated, extended and invalid-CRC files are refused without writes')
 attr.write_bytes(original);(nodes/'brightness').write_text('999');(nodes/'openbor_volume').write_text('-1');assert state('save').returncode==0 and attr.read_bytes()==original;checks.append('Invalid sysfs levels never corrupt persisted settings')
 binary=root/'mnt/vendor/bin/dmenu.bin';binary.parent.mkdir(parents=True);binary.write_bytes(Path(args.binary).read_bytes())
 def plan(system,name='Game with spaces.gba',card='mmc'):
  rom=f'{root.as_posix()}/mnt/{card}/Roms/{system}/{name}'
  return subprocess.run([str(P/'preview/stock-room-test.exe'),'plan',system,rom],env=env,capture_output=True,text=True)
 records=json.loads((P/'docs/stock-game-room-table.json').read_text())
 for item in records:
  r=plan(item['system']);assert r.returncode==0,(item,r.stderr)
  assert item['executable'].removeprefix('./') in r.stdout
 checks.append('All 23 recorded systems resolve from the installed stock binary')
 assert '/mnt/vendor/deep/drastic-modify/launch.sh' in plan('NDS').stdout
 assert 'Arguments: -cdfile ROM' in plan('PS').stdout
 assert 'Arguments: ROM m68k' in plan('FBNEO','goldnaxe.zip').stdout
 checks.append('DraStic wrapper, PCSX argument and stock arcade special argument match the capture')
 mycore=root/'mnt/data/misc/.mycore';mycore.parent.mkdir(parents=True)
 mycore.write_text('GBA:Game with spaces.gba:1:0:1\n')
 assert './vbanext.dge' in plan('GBA',card='sdcard').stdout
 assert './gba_emu.dge' in plan('GBA',card='mmc').stdout
 mycore.write_text('GBA:Game with spaces.gba:1:1:1\n')
 assert './gba_emu.dge' in plan('GBA',card='sdcard').stdout
 checks.append('GBA Game Rooms honours matching stock per-game emulator choice and card/RA separation')
 assert plan('UNKNOWN').returncode==20
 b=bytearray(binary.read_bytes());b[17160]^=1;binary.write_bytes(b);assert plan('NDS').returncode==20
 checks.append('Unknown systems and firmware signatures fail instead of guessing a route')
(P/'docs/stock-handoff-test-results.json').write_text(json.dumps({'passed':len(checks),'checks':checks,'hardware_tested':False},indent=2)+'\n')
print('\n'.join(checks))
