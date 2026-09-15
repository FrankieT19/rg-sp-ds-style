"""Keep the packaged plain-text instructions in sync with README.md."""
from pathlib import Path
import re
import textwrap

ROOT = Path(__file__).resolve().parents[1]
REPO = 'https://github.com/FrankieT19/rg-sp-ds-style'

def render_readme(markdown):
    def link(match):
        title, target = match.groups()
        if not target.startswith(('https://', 'http://')):
            target = REPO + '/blob/main/' + target
        return f'{title} ({target})'
    text = re.sub(r'!\[[^\]]*\]\([^)]*\)\s*', '', markdown)
    text = re.sub(r'\[([^\]]+)\]\(([^)]+)\)', link, text)
    text = text.replace('**', '').replace('`', '')
    text = re.sub(r'(?m)^\*([^\n]+)\*$', r'\1', text)
    text = text.replace('→', '>').replace('—', ' - ').replace('’', "'")
    lines = []
    for line in text.splitlines():
        if line.startswith('# '):
            continue
        if line.startswith('## '):
            lines += [line[3:].upper(), '']
        elif not line:
            lines.append('')
        else:
            indent = '   ' if re.match(r'^\d+\. ', line) else '  ' if line.startswith('- ') else ''
            lines.extend(textwrap.wrap(line, width=78, subsequent_indent=indent, break_long_words=False, break_on_hyphens=False))
    body = re.sub(r'\n{3,}', '\n\n', '\n'.join(lines)).strip()
    return ('DS Style for RG SP - v1.0\n'
            '========================\n\n'
            'Thanks for downloading DS Style for RG SP!\n\n' + body +
            '\n\nRepository: ' + REPO + '\n\nEnjoy DS Style!\n')

def sync_readme():
    text = render_readme((ROOT/'README.md').read_text(encoding='utf-8'))
    for path in (ROOT/'README.txt', ROOT/'device/Roms/APPS/DSStyle/README.txt'):
        path.write_text(text, encoding='utf-8', newline='\n')
    return text

if __name__ == '__main__':
    sync_readme()
    print('Both README.txt copies match README.md.')
