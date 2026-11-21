from pathlib import Path
base=Path('<development>');s=(base/'run-combined-write-through-app-9.64-private.py').read_text(encoding='utf-8-sig').replace('catalog-epoch2','catalog-epoch3');(base/'run-physical-text-write-through-app-9.64-private.py').write_text(s,encoding='utf-8',newline='\n')
