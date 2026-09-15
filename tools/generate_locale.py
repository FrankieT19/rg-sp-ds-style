"""Regenerate the checked-in locale header without changing its UTF-8 text."""
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
rows = json.loads((root/'source/locale.json').read_text(encoding='utf-8'))
assert all(len(row) == 8 and all(isinstance(s, str) for s in row) for row in rows)
names = ['English (UK)', 'Français', 'Deutsch', 'Español', 'Português', 'Italiano', 'Nederlands', 'English (US)']
quote = lambda s: json.dumps(s, ensure_ascii=False)
head = 'static int language;\nstatic const char*language_names[]={' + ','.join(map(quote, names)) + '};\nstatic const char*locale_rows[][8]={\n'
tail = '\n};\nstatic const char*tr(const char*s){if(language>0&&language<8)for(unsigned i=0;i<sizeof locale_rows/sizeof*locale_rows;i++)if(!strcmp(s,locale_rows[i][0]))return locale_rows[i][language];return s;}\n'
(root/'source/locale.h').write_text(head + ',\n'.join('{' + ','.join(map(quote, row)) + '}' for row in rows) + tail, encoding='utf-8', newline='\n')
