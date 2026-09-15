"""Host integration tests. Hardware acceptance is a separate documented checklist.
Run: python tests/test_v1.py --shell /path/to/sh [--preview /path/to/exe]
"""
import argparse, hashlib, json, os, shutil, struct, subprocess, sys, tempfile, unittest, time
from pathlib import Path

arg = argparse.ArgumentParser()
arg.add_argument('--shell', required=True)
arg.add_argument('--preview')
arg.add_argument('--report')
opts = arg.parse_args()
PROJECT = Path(__file__).resolve().parents[1]
DEVICE = PROJECT / 'device/Roms/APPS/DSStyle'

class Integration(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='dsstyle-v1-')
        self.root = Path(self.temp.name)
        self.base = self.root/'mnt/sdcard/Roms/APPS/DSStyle'
        shutil.copytree(DEVICE, self.base, ignore=shutil.ignore_patterns('state'))
        shutil.copy2(PROJECT/'tools/developer-apps/display-test.sh',self.base/'scripts/display-test.sh')
        self.sd1 = self.root/'mnt/mmc'
        self.sd1.mkdir(parents=True)
        self.vendor = self.root/'mnt/vendor/deep/retro'
        (self.vendor/'cores').mkdir(parents=True)
        (self.vendor/'cores/mgba_libretro.so').write_bytes(b'fixture, not a core')
        self.cfg = self.root/'.config/retroarch/retroarch_GBA.cfg'
        self.cfg.parent.mkdir(parents=True)
        self.cfg.write_text('savefile_directory = "/stock/save path"\nsavestate_directory = "/stock/states"\n')
        self.rom = self.root/"mnt/mmc/Roms/GBA/Space ' quote ; dollar $.gba"
        self.rom.parent.mkdir(parents=True)
        self.rom.write_bytes(b'not a real ROM')
        self.capture = self.root/'argv.bin'
        exe=self.vendor/'retroarch'
        exe.write_text('#!/bin/sh\nprintf "%s\\000" "$@" > "$DS_CAPTURE"\nexit "${DS_EXIT:-0}"\n')
        exe.chmod(0o755)
        self.env = os.environ.copy()
        self.env.update(DS_STYLE_TEST_ROOT=self.root.as_posix(),DS_CAPTURE=self.capture.as_posix(), MSYS2_ARG_CONV_EXCL='*')
        # Do not mix Git-for-Windows and devkitPro's different MSYS mount tables.
        if os.name == 'nt':
            shell_dir=Path(opts.shell).resolve().parent
            self.env['PATH']=str(shell_dir.parent/'usr/bin')+os.pathsep+str(shell_dir)+os.pathsep+os.environ.get('SYSTEMROOT','C:/Windows')+'/System32'
    def tearDown(self): self.temp.cleanup()
    def runscript(self,n,*args):
        return subprocess.run([opts.shell,(self.base/'scripts'/n).as_posix(),*args],env=self.env,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
    def test_unknown_system_is_not_guessed(self):
        other=self.rom.parent.parent/'UNKNOWN/game.gba';other.parent.mkdir();other.write_bytes(b'fixture')
        self.assertEqual(self.runscript('launch.sh','game',other.as_posix()).returncode,20)
    def test_app_exit_status_propagates(self):
        self.env['DS_EXIT']='7'
        helper=self.base/'bin/dsstyle-stock-ra'
        helper.write_text('#!/bin/sh\nprintf "%s\\000" "$@" >> "$DS_CAPTURE"\nexit "$DS_EXIT"\n');helper.chmod(0o755)
        self.assertEqual(self.runscript('launch.sh','game',self.rom.as_posix()).returncode,7)
        self.assertEqual(self.capture.read_bytes().split(b'\0')[:-1],[b'game',b'GBA',self.rom.as_posix().encode()])
    def test_installed_app_with_special_filename(self):
        app=self.base.parent/"app ' ; $.sh"
        app.write_text('#!/bin/sh\nprintf app > "$DS_CAPTURE"\n')
        self.assertEqual(self.runscript('launch.sh','app',app.as_posix()).returncode,0)
        self.assertEqual(self.capture.read_bytes(),b'app')
    def test_enable_disable_without_prior_override(self):
        self.assertEqual(self.runscript('boot-manager.sh','enable').returncode,0)
        target=self.sd1/'dmenu.bin';self.assertIn(b'DS_STYLE_V1_BOOT_SHIM',target.read_bytes())
        self.assertEqual(self.runscript('boot-manager.sh','enable').returncode,0)
        self.assertEqual(self.runscript('boot-manager.sh','disable').returncode,0)
        self.assertFalse(target.exists())
        self.assertTrue(list(self.sd1.glob('.dsstyle-v1-restored-*')))
    def test_previous_override_restored_byte_for_byte(self):
        target=self.sd1/'dmenu.bin';original=b'original launcher\x00\xff\n';target.write_bytes(original)
        self.assertEqual(self.runscript('boot-manager.sh','enable').returncode,0)
        self.assertEqual(self.runscript('boot-manager.sh','disable').returncode,0)
        self.assertEqual(target.read_bytes(),original)
    def test_modified_override_not_overwritten_on_disable(self):
        self.assertEqual(self.runscript('boot-manager.sh','enable').returncode,0)
        target=self.sd1/'dmenu.bin';target.write_bytes(b'another launcher')
        self.assertNotEqual(self.runscript('boot-manager.sh','disable').returncode,0)
        self.assertEqual(target.read_bytes(),b'another launcher')
    def test_mu_theme_is_not_modified(self):
        marker=self.root/'mnt/vendor/muos1.ini';marker.write_text('original theme')
        self.assertNotEqual(self.runscript('boot-manager.sh','enable').returncode,0)
        self.assertEqual(marker.read_text(),'original theme')
        self.assertFalse((self.sd1/'dmenu.bin').exists())
    def test_display_trial_has_independent_timeout(self):
        # Simulate a renderer stalled in startup, with no game/app child.
        exe=self.base/'bin/dsstyle'
        exe.write_text('#!/bin/sh\nwhile :; do :; done\n')
        exe.chmod(0o755)
        self.env['DS_STYLE_TEST_TIMEOUT']='1'
        started=time.monotonic()
        r=self.runscript('display-test.sh')
        self.assertEqual(r.returncode,0,r.stderr)
        self.assertLess(time.monotonic()-started,8)
        log=(self.base/'state/display-test.log').read_text()
        self.assertIn('Display test process exit code: 124',log)
        self.assertFalse(self.capture.exists())
        self.assertFalse((self.sd1/'dmenu.bin').exists())
    def test_display_trial_does_not_enter_game_mode(self):
        exe=self.base/'bin/dsstyle'
        exe.write_text('#!/bin/sh\nprintf "%s\\000" "$@" > "$DS_CAPTURE"\n')
        exe.chmod(0o755)
        self.assertEqual(self.runscript('display-test.sh').returncode,0)
        got=self.capture.read_bytes().split(b'\0')[:-1]
        self.assertEqual(got[0],b'--base')
        self.assertEqual(got[-1],b'--display-test')
        self.assertEqual(len(got),3)
    def test_manual_exit_returns_to_stock_scheduler(self):
        marker=self.root/'mnt/vendor/muos1.ini';marker.write_text('selected MU theme')
        exe=self.base/'bin/dsstyle'
        exe.write_text('#!/bin/sh\nexit 42\n');exe.chmod(0o755)
        self.assertEqual(self.runscript('run.sh').returncode,0)
        self.assertEqual(marker.read_text(),'selected MU theme')
        self.assertIn('Return to existing stock scheduler',(self.base/'state/launch.log').read_text())
        # A fresh boot clears /tmp; same-session bypass is tested separately.
        (self.root/'tmp/dsstyle-stock-session').unlink()
        self.assertEqual(self.runscript('run.sh','--boot').returncode,42)
    @unittest.skipUnless(opts.preview,'Windows preview not supplied')
    def test_skips_empty_sd1_and_bypasses_single_card_chooser(self):
        sd2=self.base.parent.parent
        game=sd2/'GBA/example.gba';game.parent.mkdir();game.write_bytes(b'fixture')
        self.rom.unlink()  # SD1 keeps an empty GBA folder, like a stock template.
        command=[str(Path(opts.preview).resolve()),'--base',self.base.as_posix(),
                 '--roms',(self.sd1/'Roms').as_posix(),'--roms2',sd2.as_posix()]
        r=subprocess.run(command+['--events','daaa','--self-test'],capture_output=True,text=True)
        self.assertEqual(r.returncode,0,r.stderr)
        self.assertNotIn('path='+self.rom.parent.as_posix(),r.stderr)
        self.assertIn('path='+game.parent.as_posix(),r.stderr)
        self.assertIn('page=1 entries=1 selected=0',r.stdout)
    def test_shell_syntax(self):
        for p in self.base.parent.rglob('*.sh'):
            r=subprocess.run([opts.shell,'-n',str(p)],capture_output=True)
            self.assertEqual(r.returncode,0,(p,r.stderr))
    def test_device_binary_is_static_aarch64(self):
        data=(DEVICE/'bin/dsstyle').read_bytes()
        self.assertEqual(data[:6],b'\x7fELF\x02\x01')
        self.assertEqual(struct.unpack_from('<H',data,18)[0],183)
        off=struct.unpack_from('<Q',data,32)[0]
        size,num=struct.unpack_from('<HH',data,54)
        self.assertNotIn(3,[struct.unpack_from('<I',data,off+i*size)[0] for i in range(num)])
    @unittest.skipUnless(opts.preview,'Windows preview not supplied')
    def test_shared_renderer_and_navigation(self):
        exe=str(Path(opts.preview).resolve())
        for screen in ['home','list','art','horizontal','vertical','settings','apps']:
            dest=self.root/(screen+'.bmp')
            r=subprocess.run([exe,'--base',str(self.base),'--roms',self.rom.parent.as_posix(),'--screen',screen,'--render',str(dest)],capture_output=True,text=True)
            self.assertEqual(r.returncode,0,r.stderr)
            data=dest.read_bytes();self.assertEqual(data[:2],b'BM')
            self.assertEqual(struct.unpack_from('<ii',data,18),(720,480))
            self.assertGreater(len(set(data[54:])),10)
        r=subprocess.run([exe,'--base',str(self.base),'--roms',self.rom.parent.as_posix(),'--screen','list','--events','y','--self-test'],capture_output=True,text=True)
        self.assertEqual(r.returncode,0,r.stderr)
        self.assertIn(self.rom.as_posix(),(self.base/'state/favourites.txt').read_text())
        r=subprocess.run([exe,'--base',str(self.base),'--roms',self.rom.parent.as_posix(),'--events','daa','--self-test'],capture_output=True,text=True)
        self.assertIn('page=1 entries=1 selected=0',r.stdout)


    def live_table(self, core='gpsp_libretro.so'):
        table=self.root/'mnt/mod/ctrl/configs/CORES.txt';table.parent.mkdir(parents=True)
        table.write_text('-GBA,'+core+'\n')
        (self.vendor/'cores'/core).write_bytes(b'fixture')
        wrapper=table.parent.parent/'RA_launch.sh'
        wrapper.write_text('#!/bin/sh\nprintf "%s\\000" "$@" > "$DS_CAPTURE"\nexit "${DS_EXIT:-0}"\n');wrapper.chmod(0o755)
        return table
    def preview_run(self,events='',screen='home',env=None):
        return subprocess.run([str(Path(opts.preview).resolve()),'--base',self.base.as_posix(),'--roms',(self.sd1/'Roms').as_posix(),'--roms2',self.base.parent.parent.as_posix(),'--screen',screen,'--events',events,'--self-test'],capture_output=True,text=True,env=env)
    @unittest.skipUnless(opts.preview,'Windows preview not supplied')
    def test_no_navigation_sound_for_noop(self):
        for events in ['b','u','l','r']:
            r=self.preview_run(events);self.assertNotIn('sound:',r.stderr)
        r=self.preview_run('u','list');self.assertNotIn('sound:',r.stderr)
        r=self.preview_run('daa');self.assertEqual(r.stderr.count('sound: accept'),2)
        r=self.preview_run('daaa');self.assertEqual(r.stderr.count('sound: launch'),1);self.assertEqual(r.stderr.count('sound: accept'),2)
    @unittest.skipUnless(opts.preview,'Windows preview not supplied')
    def test_tab_restores_folder_position(self):
        (self.base/'state').mkdir(exist_ok=True)
        (self.base/'state/preferences.txt').write_text('v2 1 0 0 1 0 3\n')
        for i in range(20):(self.rom.parent/f'{i:02}.gba').write_bytes(b'x')
        r=self.preview_run('daa'+('d'*12)+'e'+'q')
        self.assertIn('page=1 entries=21 selected=12',r.stdout)
        self.assertIn('top=3',r.stdout)
    @unittest.skipUnless(opts.preview,'Windows preview not supplied')
    def test_scrolling_up_moves_selection_immediately(self):
        (self.base/'state').mkdir(exist_ok=True)
        (self.base/'state/preferences.txt').write_text('v2 1 0 0 1 0 3\n')
        for i in range(20):(self.rom.parent/f'{i:02}.gba').write_bytes(b'x')
        r=self.preview_run('daa'+('d'*12)+'u')
        self.assertIn('selected=11',r.stdout);self.assertIn('top=3',r.stdout)
    @unittest.skipUnless(opts.preview,'Windows preview not supplied')
    def test_sd1_first_when_both_cards_have_roms(self):
        game=self.base.parent.parent/'GBA/sd2.gba';game.parent.mkdir();game.write_bytes(b'x')
        r=self.preview_run('daaa');self.assertIn('path='+self.rom.parent.as_posix(),r.stderr)
        self.assertNotIn('path='+game.parent.as_posix(),r.stderr)
    @unittest.skipUnless(opts.preview,'Windows preview not supplied')
    def test_y_shares_stock_favourites_crc_and_backup(self):
        import zlib
        # Bit0 chooses SD2, bit1 selects stock's RA game list.
        data=('Version=1\n'+self.rom.name+':GBA:nul:2:1:0\n').encode()
        original=data+struct.pack('<I',zlib.crc32(data,0xffffffff)^0xffffffff)
        stock=self.root/'stock.favorite';stock.write_bytes(original)
        env=os.environ.copy();env['DS_STYLE_STOCK_FAV_TEST']=str(stock)
        r=self.preview_run('2y',env=env);self.assertEqual(r.returncode,0,r.stderr)
        changed=stock.read_bytes();self.assertEqual(changed[:-4],b'Version=1\n')
        self.assertEqual(struct.unpack('<I',changed[-4:])[0],zlib.crc32(changed[:-4],0xffffffff)^0xffffffff)
        self.assertEqual((self.base/'state/stock-favourites-before.bin').read_bytes(),original)
        r=self.preview_run('daay',env=env);self.assertEqual(r.returncode,0,r.stderr)
        self.assertEqual(stock.read_bytes(),original)
    @unittest.skipUnless(opts.preview,'Windows preview not supplied')
    def test_invalid_stock_crc_is_never_written(self):
        stock=self.root/'stock.favorite';original=b'Version=1\ninvalid trailer';stock.write_bytes(original)
        env=os.environ.copy();env['DS_STYLE_STOCK_FAV_TEST']=str(stock)
        self.preview_run('daay',env=env)
        self.assertEqual(stock.read_bytes(),original)
    def test_copied_sound_rates_and_duration(self):
        import wave
        root=DEVICE/'assets/sounds';metadata=json.loads((root/'conversion.json').read_text())
        for item in metadata:
            with wave.open(str(root/(item['sound']+'.wav'))) as w:
                self.assertEqual(w.getframerate(),48000);self.assertEqual(w.getsampwidth(),2)
                self.assertLess(abs(w.getnframes()/48000-item['source_seconds']),1/48000)


    @unittest.skipUnless(opts.preview,'Windows preview not supplied')
    def test_legacy_gba_pack_is_ignored(self):
        from PIL import Image
        # GBA stores red in low bits; conventional BMP interpretation is wrong.
        pack=self.base/'SYSTEM/IMGS/CUSTOM';pack.mkdir(parents=True)
        raw=b'BM'+bytes(52)+struct.pack('<H',31)*(120*80)
        (pack/(self.rom.stem+'.bmp')).write_bytes(raw)
        image=self.root/'pack.bmp'
        r=subprocess.run([str(Path(opts.preview).resolve()),'--base',str(self.base),'--roms',self.rom.parent.as_posix(),'--screen','horizontal','--render',str(image)],capture_output=True,text=True)
        self.assertEqual(r.returncode,0,r.stderr)
        im=Image.open(image);self.assertNotEqual(im.getpixel((300,150)),(255,0,0))
    @unittest.skipUnless(opts.preview,'Windows preview not supplied')
    def test_stock_full_filename_art_preserves_edges(self):
        from PIL import Image
        art=self.rom.parent/'Imgs';art.mkdir();im=Image.new('RGB',(240,80),'red')
        for x in range(60,180):
            for y in range(80):im.putpixel((x,y),(0,255,0))
        im.save(art/(self.rom.name+'.png'))
        image=self.root/'crop.bmp'
        r=subprocess.run([str(Path(opts.preview).resolve()),'--base',str(self.base),'--roms',self.rom.parent.as_posix(),'--screen','horizontal','--render',str(image)],capture_output=True,text=True)
        self.assertEqual(r.returncode,0,r.stderr)
        im=Image.open(image)
        self.assertEqual(im.getpixel((183,150)),(255,0,0));self.assertEqual(im.getpixel((534,240)),(255,0,0));self.assertEqual(im.getpixel((360,180)),(0,255,0))

    @unittest.skipUnless(opts.preview,'Windows preview not supplied')
    def test_home_dpad_cycles_only_quick_launch_and_shoulders_any_box(self):
        state=self.base/'state';state.mkdir(exist_ok=True)
        (state/'recent.txt').write_text('\n'.join([self.rom.as_posix()]*3)+'\n')
        for events,home,quick in [('r',0,1),('l',0,2),('rl',0,0),('dr',2,0),('de',1,1),('ddq',3,2)]:
            r=self.preview_run(events)
            self.assertIn(f'home={home} quick={quick}',r.stdout,(events,r.stdout,r.stderr))
        self.assertIn('sound: move',self.preview_run('r').stderr)
        self.assertIn('sound: tab',self.preview_run('de').stderr)
    @unittest.skipUnless(opts.preview,'Windows preview not supplied')
    def test_start_and_x_are_noops_on_home(self):
        for events in ('t','x','tx'):
            r=self.preview_run(events);self.assertIn('page=0',r.stdout);self.assertNotIn('sound:',r.stderr)

    def test_folder_only_list_is_temporary_and_optional(self):
        r=self.preview_run('',screen='horizontal');self.assertIn('view=2',r.stdout);self.assertIn('effective=0',r.stdout)
        r=self.preview_run('a',screen='horizontal');self.assertIn('view=2',r.stdout);self.assertIn('effective=2',r.stdout)
        state=self.base/'state';state.mkdir(exist_ok=True);(state/'style.txt').write_text('0 0 1 0 0 0 0\n')
        r=self.preview_run('',screen='horizontal');self.assertIn('effective=2',r.stdout)

    @unittest.skipUnless(opts.preview,'Windows preview not supplied')
    def test_collection_returns_to_caller(self):
        for events in ['tb','xb','yb','1b','2b']:
            r=self.preview_run(events);self.assertIn('page=0',r.stdout,(events,r.stdout))
        for events in ['daa1bb']:
            r=self.preview_run(events);self.assertIn('page=1 entries=1',r.stdout,(events,r.stdout))
        r=self.preview_run('m1b');self.assertIn('page=2',r.stdout)
    @unittest.skipUnless(opts.preview,'Windows preview not supplied')
    def test_favourite_notice_consumes_accept_and_back(self):
        for key in ['a','b']:
            r=self.preview_run('daay'+key);self.assertIn('page=1 entries=1',r.stdout);self.assertNotIn('sound: launch',r.stderr)
    @unittest.skipUnless(opts.preview,'Windows preview not supplied')
    def test_last_page_stays_full(self):
        (self.base/'state').mkdir(exist_ok=True)
        (self.base/'state/preferences.txt').write_text('v2 1 0 0 1 0 3\n')
        for i in range(22):(self.rom.parent/f'{i:02}.gba').write_bytes(b'x')
        for events,selected,top in [('daar',10,10),('daarr',20,13),('daarrr',22,13),('daarrl',10,3)]:
            r=self.preview_run(events);self.assertIn(f'selected={selected}',r.stdout);self.assertIn(f'top={top}',r.stdout)
    @unittest.skipUnless(opts.preview,'Windows preview not supplied')
    def test_x_opens_search_without_changing_view(self):
        r=self.preview_run('daax');self.assertIn('view=2',r.stdout);self.assertIn('page=1 entries=1',r.stdout);self.assertIn('sound: menu',r.stderr)
    @unittest.skipUnless(opts.preview,'Windows preview not supplied')
    def test_defaults_and_settings_collection_return(self):
        r=self.preview_run('mdda'+'d'*4+'a');self.assertEqual(r.returncode,0,r.stderr)
        state=self.base/'state'
        self.assertEqual((state/'preferences.txt').read_text().split(),['v2','2','0','0','1','0','3'])
        self.assertEqual((state/'style.txt').read_text().split(),['3','2','0','2','0','0','1'])
        self.assertEqual((state/'interface.txt').read_text().split(),['0','1','0','1','0'])
        r=self.preview_run('daam1bb');self.assertIn('page=1 entries=1',r.stdout)
    def test_sleep_calls_stock_script_and_propagates_result(self):
        script=self.root/'mnt/vendor/ctrl/pwr_new.sh';script.parent.mkdir(parents=True,exist_ok=True)
        script.write_text('#!/bin/sh\nprintf "%s\\000" "$@" > "$DS_CAPTURE"\nexit 7\n')
        r=self.runscript('sleep-stock.sh','auto');self.assertEqual(r.returncode,7,r.stderr)
        self.assertEqual(self.capture.read_bytes(),b'auto\0')
        self.assertEqual((self.base/'state/stock-power/vendor-pwr_new.sh').read_bytes(),script.read_bytes())

    def test_game_room_choice_uses_reader_and_never_retries(self):
        state=self.base/'state/launch-modes';state.mkdir(parents=True)
        (state/'GBA.txt').write_text('gameroom\n')
        helper=self.base/'bin/dsstyle-stock-room'
        helper.write_text('#!/bin/sh\nprintf "%s\\000" "$@" > "$DS_CAPTURE"\nexit 7\n');helper.chmod(0o755)
        r=self.runscript('launch.sh','game',self.rom.as_posix());self.assertEqual(r.returncode,7,r.stderr+(self.base/'state/last-launch.txt').read_text())
        self.assertEqual(self.capture.read_bytes().split(b'\0')[:-1],[b'game',b'GBA',self.rom.as_posix().encode()])
    def test_plan_does_not_persist_hardware(self):
        marker=self.base/'state/hardware-write';helper=self.base/'bin/dsstyle-stock-state'
        helper.write_text('#!/bin/sh\nprintf touched > "$1/state/hardware-write"\n');helper.chmod(0o755)
        reader=self.base/'bin/dsstyle-stock-ra';reader.write_text('#!/bin/sh\nexit 0\n');reader.chmod(0o755)
        r=self.runscript('launch.sh','plan',self.rom.as_posix());self.assertEqual(r.returncode,0,r.stderr);self.assertFalse(marker.exists())
    def test_stock_cpu_policy_and_state_transitions(self):
        ctrl=self.root/'mnt/vendor/ctrl';ctrl.mkdir(parents=True,exist_ok=True)
        cpu=ctrl/'cpu_setting.sh';cpu.write_text('#!/bin/sh\nprintf "cpu:%s\\n" "$1" >> "$DS_CAPTURE"\n');cpu.chmod(0o755)
        helper=self.base/'bin/dsstyle-stock-state';helper.write_text('#!/bin/sh\nprintf "state:%s\\n" "$2" >> "$DS_CAPTURE"\n');helper.chmod(0o755)
        for mode in ('boot','leave','enter'):self.assertEqual(self.runscript('stock-session.sh',mode).returncode,0)
        self.assertEqual(self.capture.read_text().splitlines(),['state:restore','cpu:full','state:save','cpu:deflt','cpu:full'])

    def test_stock_os_return_bypasses_autoboot_until_reboot(self):
        stock=self.root/'mnt/vendor/bin/dmenu.bin';stock.parent.mkdir(parents=True,exist_ok=True)
        stock.write_text('#!/bin/sh\nprintf stock >> "$DS_CAPTURE"\n');stock.chmod(0o755)
        frontend=self.base/'bin/dsstyle';frontend.write_text('#!/bin/sh\nprintf dsstyle >> "$DS_CAPTURE"\nexit 42\n');frontend.chmod(0o755)
        r=self.runscript('run.sh','--boot');self.assertEqual(r.returncode,42,r.stderr)
        self.assertTrue((self.root/'tmp/dsstyle-stock-session').exists())
        r=self.runscript('run.sh','--boot');self.assertEqual(r.returncode,0,r.stderr)
        self.assertEqual(self.capture.read_text(),'dsstylestock')
        (self.root/'tmp/dsstyle-stock-session').unlink()
        r=self.runscript('run.sh','--boot');self.assertEqual(r.returncode,42,r.stderr)
        self.assertEqual(self.capture.read_text(),'dsstylestockdsstyle')

suite=unittest.defaultTestLoader.loadTestsFromTestCase(Integration)
res=unittest.TextTestRunner(verbosity=2).run(suite)
if opts.report:
    Path(opts.report).write_text(json.dumps({'tests':res.testsRun,'failures':len(res.failures),'errors':len(res.errors),'skipped':len(res.skipped),'hardware_tested':False},indent=2))
sys.exit(not res.wasSuccessful())
