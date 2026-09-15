"""Render documentation screenshots with synthetic, non-playable fixtures.
Optional dependency: Pillow (python -m pip install Pillow).
"""
from pathlib import Path
import shutil
import subprocess
import tempfile
from PIL import Image, ImageDraw

root = Path(__file__).resolve().parents[1]
output = root/'docs/screenshots'
output.mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(prefix='dsstyle-screens-') as temp:
    temp = Path(temp)
    base = temp/'DSStyle'
    shutil.copytree(root/'device/Roms/APPS/DSStyle/assets', base/'assets')
    (base/'state').mkdir()
    games = temp/'Roms/GBA'
    (games/'Imgs').mkdir(parents=True)
    names = ['Aster Trail', 'Garden Quest', 'Pocket Rally', 'Skybound', 'Tiny Workshop']
    for i, name in enumerate(names):
        (games/(name+'.gba')).write_bytes(b'Non-playable screenshot fixture')
        im = Image.new('RGB', (240, 160), ['#a7c5dd','#b9d6b1','#e2bcb1','#b5ceda','#cdb9d7'][i])
        draw = ImageDraw.Draw(im)
        draw.rectangle((14,14,225,145), outline='#f8faf1', width=4)
        draw.rectangle((90,40,150,100), fill='#f0dfad')
        draw.rectangle((106,56,134,84), fill=im.getpixel((0,0)))
        draw.text((28,120), name, fill='#33404b')
        im.save(games/'Imgs'/(name+'.png'))
    (base/'state/recent.txt').write_text((games/'Garden Quest.gba').as_posix()+'\n', encoding='utf-8')
    for screen in ('home','horizontal','art','settings'):
        bmp = temp/(screen+'.bmp')
        command = [str(root/'preview/dsstyle-preview.exe'), '--base', str(base), '--roms', str(games.parent), '--screen', screen, '--render', str(bmp)]
        if screen in ('horizontal','art'):
            command += ['--events', 'a']
        subprocess.run(command, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        with Image.open(bmp) as im:
            assert im.size == (720,480)
            im.save(output/(screen+'.png'))
print('Rendered actual frontend with synthetic games and artwork.')
