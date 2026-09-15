import argparse
import os
from pathlib import Path
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('--zig', required=True)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
exe = root/'preview/defaults-test.exe'
exe.parent.mkdir(exist_ok=True)
env = dict(os.environ, ZIG_GLOBAL_CACHE_DIR=str(root/'.build-cache/global'), ZIG_LOCAL_CACHE_DIR=str(root/'.build-cache/local'))
subprocess.run([args.zig, 'cc', '-target', 'x86_64-windows-gnu', '-O2', '-UNDEBUG', str(root/'tests/defaults_test.c'), '-o', str(exe), '-lgdi32', '-luser32'], check=True, env=env)
with tempfile.TemporaryDirectory(prefix='dsstyle-defaults-') as temp:
    subprocess.run([str(exe), temp], check=True)
device = root/'device/Roms/APPS/DSStyle'
assert not (device/'DISABLED').exists()
assert not (device/'state').exists() or not any((device/'state').iterdir())
assert not list((root/'device').rglob('dmenu.bin')), 'Never bundle an active autoboot override'
print('Fresh package contains no state or autoboot override.')
