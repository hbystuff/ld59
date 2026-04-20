import os, sys

EXTS = ('.png', '.json', '.txt', '.ttf', '.ogg', '.wav', '.glsl')
ROOT = os.path.dirname(os.path.abspath(__file__))
DATA_DIR = os.path.join(ROOT, 'data')
MANIFEST = os.path.join(DATA_DIR, 'manifest.txt')

def main():
    if not os.path.isdir(DATA_DIR):
        os.makedirs(DATA_DIR)

    entries = []
    for dirpath, _, files in os.walk(DATA_DIR):
        for name in files:
            if name.startswith('_'):
                continue
            full = os.path.join(dirpath, name)
            if os.path.abspath(full) == os.path.abspath(MANIFEST):
                continue
            if not name.lower().endswith(EXTS):
                continue
            rel = os.path.relpath(full, ROOT).replace('\\', '/')
            entries.append(rel)
    entries.sort()

    with open(MANIFEST, 'w', encoding='utf-8', newline='\n') as f:
        for e in entries:
            f.write(e + '\n')

    print('[gen_manifest] wrote %d entries to %s' % (len(entries), os.path.relpath(MANIFEST, ROOT)))

if __name__ == '__main__':
    main()
