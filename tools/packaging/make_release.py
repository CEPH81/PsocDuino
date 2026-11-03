#!/usr/bin/env python3
"""Empaqueta PsocDuino para el Gestor de placas del Arduino IDE.

  make_release.py VERSION [--out DIR] [--url URL_DEL_ARCHIVO] [--index package_psocduino_index.json]

Crea DIR/psocduino-m8c-VERSION.tar.bz2 (carpeta raiz unica, permisos de los
ejecutables conservados) y agrega o reemplaza esa version en el indice JSON.
"""
import argparse, hashlib, io, json, os, re, tarfile, time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
INCLUDE = ['boards.txt', 'platform.txt', 'programmers.txt', 'importar_variante.bat', 'README.md',
           'cores', 'variants', 'libraries', 'docs', 'tools/bin', 'tools/udev']
REPO = 'https://github.com/CEPH81/PsocDuino'

def files():
    for item in INCLUDE:
        p = os.path.join(ROOT, item)
        if os.path.isfile(p):
            yield item
        for d, dirs, fs in os.walk(p):
            dirs.sort()
            for f in sorted(fs):
                yield os.path.relpath(os.path.join(d, f), ROOT)

def build(version, out):
    os.makedirs(out, exist_ok=True)
    name = f'psocduino-m8c-{version}'
    path = os.path.join(out, name + '.tar.bz2')
    with tarfile.open(path, 'w:bz2') as tar:
        seen = set()
        for rel in files():
            parts = rel.replace(os.sep, '/').split('/')[:-1]
            for i in range(len(parts) + 1):
                d = '/'.join([name] + parts[:i])
                if d not in seen:
                    seen.add(d)
                    di = tarfile.TarInfo(d)
                    di.type = tarfile.DIRTYPE
                    di.mode = 0o755
                    di.mtime = int(time.time())
                    tar.addfile(di)
            data = open(os.path.join(ROOT, rel), 'rb').read()
            if rel == 'platform.txt':
                data = re.sub(rb'(?m)^version=.*$', b'version=' + version.encode(), data)
            info = tarfile.TarInfo(f'{name}/{rel}'.replace(os.sep, '/'))
            info.size = len(data)
            info.mtime = int(time.time())
            exe = rel.startswith(('tools/bin/linux/', 'tools/bin/macos/')) and not rel.endswith('.txt')
            info.mode = 0o755 if exe else 0o644
            tar.addfile(info, io.BytesIO(data))
    data = open(path, 'rb').read()
    return path, 'SHA-256:' + hashlib.sha256(data).hexdigest(), str(len(data))

def boards():
    names = re.findall(r'(?m)^[a-z0-9_]+\.name=(.*)$', open(os.path.join(ROOT, 'boards.txt')).read())
    return [{'name': n.strip()} for n in names]

def update_index(index_path, version, url, archive, checksum, size):
    if os.path.exists(index_path):
        idx = json.load(open(index_path))
    else:
        idx = {'packages': [{
            'name': 'psocduino', 'maintainer': 'CEPH81', 'websiteURL': REPO, 'email': '',
            'help': {'online': REPO}, 'platforms': [], 'tools': []}]}
    pkg = idx['packages'][0]
    pkg['platforms'] = [p for p in pkg['platforms'] if p['version'] != version]
    pkg['platforms'].append({
        'name': 'PsocDuino (Cypress PSoC 1)', 'architecture': 'm8c', 'version': version,
        'category': 'Contributed', 'help': {'online': REPO},
        'url': url, 'archiveFileName': os.path.basename(archive),
        'checksum': checksum, 'size': size, 'boards': boards(), 'toolsDependencies': []})
    pkg['platforms'].sort(key=lambda p: [int(x) if x.isdigit() else x for x in re.split(r'[.-]', p['version'])])
    with open(index_path, 'w') as f:
        json.dump(idx, f, indent=2, ensure_ascii=False)
        f.write('\n')

if __name__ == '__main__':
    a = argparse.ArgumentParser()
    a.add_argument('version')
    a.add_argument('--out', default='dist')
    a.add_argument('--url')
    a.add_argument('--index', default=os.path.join(ROOT, 'package_psocduino_index.json'))
    o = a.parse_args()
    v = o.version.lstrip('v')
    archive, checksum, size = build(v, o.out)
    url = o.url or f'{REPO}/releases/download/v{v}/{os.path.basename(archive)}'
    update_index(o.index, v, url, archive, checksum, size)
    print(f'{archive}\n{checksum}  {size} bytes\nindice: {o.index}')
