"""Read-only installed firmware plans; no firmware is redistributed or executed."""
from pathlib import Path
import argparse,os,subprocess,tempfile,struct,json
ap=argparse.ArgumentParser();ap.add_argument('--binary',required=True);args=ap.parse_args()
P=Path(__file__).resolve().parents[1];checks=[]
binary=Path(args.binary).read_bytes()
def word(p):return struct.unpack_from('<I',binary,p)[0]
def addr(a):
 for i in range(struct.unpack_from('<H',binary,44)[0]):
  p=word(28)+i*struct.unpack_from('<H',binary,42)[0]
  if word(p)==1 and word(p+8)<=a<word(p+8)+word(p+16):return word(p+4)+a-word(p+8)
 raise AssertionError(hex(a))
def string(a):
 if not a:return ''
 p=addr(a);return binary[p:binary.index(0,p)].decode()
with tempfile.TemporaryDirectory() as tmp:
 root=Path(tmp);menu=root/'mnt/vendor/bin/dmenu.bin';menu.parent.mkdir(parents=True);menu.write_bytes(binary)
 config=root/'mnt/data/misc/.setcore';config.parent.mkdir(parents=True)
 wrapper=root/'mnt/mod/ctrl/RA_launch.sh';wrapper.parent.mkdir(parents=True);wrapper.write_text('# fixture\n')
 env=os.environ.copy();env['DS_STYLE_TEST_ROOT']=root.as_posix()
 def plan(sys='SFC',card='sdcard',name="Game ' ; $.sfc"):
  return subprocess.run([str(P/'preview/stock-ra-test.exe'),'plan',sys,f'{root.as_posix()}/mnt/{card}/Roms/{sys}/{name}'],env=env,capture_output=True,text=True)
 systems=[]
 for i in range(54):
  at=0x36260+i*52;system=string(word(at+12));core=string(word(at+24));r=plan(system)
  assert r.returncode==0,(system,core,r.stderr,r.stdout)
  if core:assert 'Core: '+core+'\n' in r.stdout,(system,r.stdout)
  systems.append(system)
 checks.append('All 54 installed RetroArch-menu systems resolve their installed default and stock dispatcher')
 expected=['snes9x_libretro.so','snes9x2002_libretro.so','snes9x2005_libretro.so','snes9x2005_plus_libretro.so','snes9x2010_libretro.so']
 for choice,core in enumerate(expected):
  config.write_text(f"Version=1\nSFC:Game ' ; $.sfc:1:1:{choice}\n")
  r=plan();assert r.returncode==0 and 'Core: '+core+'\n' in r.stdout,r
 checks.append('Every SNES per-game choice is reread between launches; installed defaults and all four alternatives match stock')
 for text in ["Version=1\nSFC:Game ' ; $.sfc:0:1:3\n","Version=1\nSFC:Game ' ; $.sfc:1:0:3\n","Version=1\nSFC:game ' ; $.sfc:1:1:3\n","Version=2\nSFC:Game ' ; $.sfc:1:1:3\n"]:
  config.write_text(text);assert 'Core: snes9x_libretro.so\n' in plan().stdout
 checks.append('Stock card, RetroArch flag, exact filename and version matching are respected')
 config.write_text("Version=1\nother directory:Game ' ; $.sfc:1:1:3\n")
 assert 'Core: snes9x2005_plus_libretro.so\n' in plan().stdout
 checks.append('Directory field is ignored exactly as in stock basename lookup')
 config.write_text("Version=1\nSFC:Game ' ; $.sfc:1:1:999\n");assert plan().returncode==20
 config.unlink();assert plan('UNKNOWN').returncode==20
 changed=bytearray(binary);changed[0x21cc8]^=1;menu.write_bytes(changed);assert plan().returncode==20;menu.write_bytes(binary)
 checks.append('Invalid saved choice, unknown system and unknown firmware ABI fail without guessing or retrying')
 changed=bytearray(binary);slot=0x36260+47*52+24;struct.pack_into('<I',changed,slot,word(0x36260+23*52+24));menu.write_bytes(changed)
 assert 'Core: mgba_libretro.so\n' in plan().stdout;menu.write_bytes(binary)
 checks.append('Changing the installed table changes the result without any DS Style core map')
 table=root/'mnt/mod/ctrl/configs/CORES.txt';table.parent.mkdir(parents=True);table.write_text('-SFC,wrong_libretro.so\n')
 assert 'Core: snes9x_libretro.so\n' in plan().stdout
 checks.append('The unrelated random-launch CORES.txt table cannot override the menu core')
 r=plan();assert "Argument 2: "+root.as_posix()+"/mnt/sdcard/Roms/SFC/Game ' ; $.sfc\n" in r.stdout
 checks.append('Spaces, quotes, dollar and semicolon remain one ROM argument')
 wrapper.unlink();r=plan();assert r.returncode==0 and 'stock embedded command template' in r.stdout and 'Argument 1: -c\n' in r.stdout and 'Argument 2: /.config/retroarch/retroarch.cfg\n' in r.stdout
 debug=root/'mnt/data/debug.ini';debug.touch();assert 'Argument 1: -v\n' in plan().stdout
 checks.append('Unmodified-stock fallback reads its own command template and debug flag')
(P/'docs/stock-ra-test-results.json').write_text(json.dumps({'passed':len(checks),'checks':checks,'systems':systems,'hardware_tested':False},indent=2)+'\n')
print('\n'.join(checks))
