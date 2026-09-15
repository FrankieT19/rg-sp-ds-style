"""Create the single clean installation ZIP; never package personal state."""
from pathlib import Path
import hashlib
import zipfile
from sync_readme import sync_readme

sync_readme()

root = Path(__file__).resolve().parents[1]
device = root/'device'
base = device/'Roms/APPS/DSStyle'
required = ['dsstyle', 'dsstyle-audio', 'dsstyle-brightness', 'dsstyle-display', 'dsstyle-volume', 'dsstyle-stock-ra', 'dsstyle-stock-room', 'dsstyle-stock-state']
for name in required:
    data = (base/'bin'/name).read_bytes()
    assert data[:4] == b'\x7fELF', f'Build missing or invalid: {name}'
assert sorted(p.name for p in (base/'config').rglob('*') if p.is_file()) == ['launch.conf'], 'Do not publish private launch profiles'
assert (base/'username.txt').read_text().strip() == 'DS Style', 'Do not publish a personal username'
files = []
for p in sorted(device.rglob('*')):
    if not p.is_file():
        continue
    rel = p.relative_to(device)
    if 'state' in rel.parts or p.name == 'DISABLED':
        continue
    assert p.suffix.lower() not in ('.log', '.exe', '.zip', '.pyc'), rel
    if p.suffix == '.sh':
        assert b'\r' not in p.read_bytes(), f'Shell script needs LF: {rel}'
    files.append((p, rel.as_posix()))
files.append((root/'README.txt', 'README.txt'))
files.append((root/'LICENSE', 'Roms/APPS/DSStyle/LICENSE.txt'))
dist = root/'dist'
dist.mkdir(exist_ok=True)
out = dist/'DS-Style-RG-SP-v1.0.zip'
with zipfile.ZipFile(out, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
    for p, name in files:
        info = zipfile.ZipInfo(name, (2026, 9, 15, 0, 0, 0))
        info.create_system = 3
        info.external_attr = (0o100755 if '/bin/' in name or name.endswith('.sh') else 0o100644) << 16
        archive.writestr(info, p.read_bytes(), compress_type=zipfile.ZIP_DEFLATED, compresslevel=9)
with zipfile.ZipFile(out) as archive:
    assert archive.testzip() is None
    assert len(archive.namelist()) == len(set(archive.namelist()))
digest = hashlib.sha256(out.read_bytes()).hexdigest()
(dist/'SHA256SUMS.txt').write_text(f'{digest}  {out.name}\n', encoding='ascii')
print(f'{out}\n{len(files)} files; {out.stat().st_size:,} bytes\nSHA256 {digest}')
