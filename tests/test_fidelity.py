"""Independent pixel checks against imported source artwork/glyphs and lcd1x maths."""
import hashlib, json, math, subprocess, tempfile
from pathlib import Path
from PIL import Image, ImageChops, ImageDraw

PROJECT=Path(__file__).resolve().parents[1]
ASSETS=PROJECT/'device/Roms/APPS/DSStyle/assets'
EXE=PROJECT/'preview/dsstyle-preview.exe'
font=(ASSETS/'font.bin').read_bytes()
assert hashlib.sha256(font).hexdigest()=='f3a11a729c120f176659f6c5eb52f9b2c7b0948d6f176e3751a5d2af69a09240'

def text(image,x,y,value,colour=0):
    colour=((colour>>16)&255,(colour>>8)&255,colour&255)
    for c in value:
        glyph=font[ord(c)*12:ord(c)*12+12]
        for yy,bits in enumerate(glyph):
            for xx in range(8):
                if bits&(128>>xx):image.putpixel((x+xx,y+yy),colour)
        x+=6

def background(name,title='',counter=None):
    image=Image.open(ASSETS/(name+'.bmp')).convert('RGB')
    image.paste(Image.open(ASSETS/'themes/pale_blue/pale_blue.bmp').convert('RGB'),(0,0))
    text(image,3,3,'DS Style',0xffffff)
    text(image,(240-len(title)*6)//2,3,title,0xffffff)
    text(image,189 if counter is None else 235-len(counter)*6,3,'12:34:56' if counter is None else counter,0xffffff)
    if counter is None:
        d=ImageDraw.Draw(image);d.rectangle((171,6,182,12),outline='white');d.rectangle((183,8,183,10),fill='white');d.rectangle((173,8,178,10),fill='white')
    return image

checks=[]
with tempfile.TemporaryDirectory(prefix='dsstyle-fidelity-') as folder:
    root=Path(folder);base=root/'DSStyle';base.mkdir()
    # Private assets/state isolates all verification from the card/user preferences.
    import shutil
    shutil.copytree(ASSETS,base/'assets')
    def render(screen,events='',lcd=False):
        out=root/(screen+events+str(lcd)+'.bmp')
        command=[str(EXE),'--base',str(base),'--demo','--screen',screen,'--events',events,'--render',str(out)]
        if lcd:command+=['--lcd']
        subprocess.run(command,check=True,capture_output=True)
        return Image.open(out).convert('RGB')
    def compare(name,actual,expected):
        expected=expected.resize((720,480),Image.Resampling.NEAREST)
        diff=ImageChops.difference(actual,expected)
        assert diff.getbbox() is None,(name,diff.getbbox())
        checks.append(name)
    # Independent original SET.bmp placement: labels stay on the left, only
    # the selected value is filled, original page title/clock coordinates.
    expected=background('SET','Settings')
    labels=['Interface','System','Controls']
    values=['>','>','>']
    for i,(label,value) in enumerate(zip(labels,values)):
        y=24+i*14;text(expected,23,y,label)
        if i==0:expected.paste((82,115,140),(112,y,224,y+13))
        text(expected,119,y,value,0xffffff if i==0 else 0)
    compare('settings matches original backdrop and text/value positions',render('settings'),expected)
    (base/'state/interface.txt').write_text('0 0 1 1\n')
    expected=background('SD_HORIZONTAL','Games','2/5')
    art=Image.open(ASSETS/'systems/wide/GBA.png').convert('RGB')
    # Match nearest sampling at integer source coordinates, as the cartridge
    # scaler does, rather than Pillow's centre-based nearest convention.
    def art_at(x,y,w,h):
        for yy in range(h):
            for xx in range(w):
                if 0<=x+xx<240:expected.putpixel((x+xx,y+yy),art.getpixel((xx*art.width//w,yy*art.height//h)))
    art_at(-5,47,60,40);art_at(185,47,60,40);art_at(60,27,120,80)
    text(expected,39+(162-12*6)//2,115+(39-12)//2,'Garden Quest')
    compare('horizontal thumbnail/title positions and original empty lower area',render('horizontal'),expected)
    # lcd1x reference is deliberately expressed as the published sine equation,
    # independently of the renderer's cosine lookup, for every physical pixel.
    plain=render('home');lcd=render('home',lcd=True)
    for y in range(480):
        for x in range(720):
            gx=(4+math.sin(2*math.pi*((x+.5)/3-.25)))/5
            gy=(16+math.sin(2*math.pi*((y+.5)/3-.25)))/17
            expected_pixel=tuple(int(c*gx*gy+.5) for c in plain.getpixel((x,y)))
            assert max(abs(a-b) for a,b in zip(expected_pixel,lcd.getpixel((x,y))))<=1
    checks.append('LCD grid matches default lcd1x intensity within one 8-bit level')
    render('settings','a'+'d'*14+'a')
    prefs=(base/'state/preferences.txt').read_text().split()
    assert prefs[0]=='v2' and prefs[3]=='1'
    checks.append('LCD preference persists independently of RetroArch')
    (base/'state/preferences.txt').write_text('1 1\n')
    render('settings','a'+'d'*12+'a')
    assert (base/'state/preferences.txt').read_text().split()[2]=='13'
    checks.append('legacy pink colour maps to correct 16-colour palette before next change')
checks.append('actual named DS Style sans font hash matches source preset')
(PROJECT/'docs/fidelity-test-results.json').write_text(json.dumps({'passed':len(checks),'checks':checks,'device_visual_validation':False},indent=2)+'\n')
print('\n'.join(checks))
