"""Folha de contato de um sistema Niagara: python sheet.py <NS_Nome> [t1,t2,...] [cam]"""
import sys, os, glob
from PIL import Image
from mono import call
ROOT = "C:/Users/Eros Nicolino/Documents/4 - Projetos/Jogos e 3D/Vortex-UE5-migracao/Vortex/Saved/Screenshots/Monolith/"
CAMS = {'q': {'location': [-650, 250, 260], 'rotation': [-22, -21, 0], 'fov': 60}, 'f': {'location': [-700, 0, 60], 'rotation': [0, 0, 0], 'fov': 60},
        't': {'location': [0, 0, 900], 'rotation': [-90, 0, 0], 'fov': 60}}


def sheet(name, times=(0.05, 0.15, 0.3, 0.5, 0.8), cam='q', size=320):
    out = ROOT + 'seq_' + name
    for f in glob.glob(out + '/*.png'): os.remove(f)
    r = call('editor_query', 'capture_sequence_frames', {'asset_path': '/Game/Vortex/Fx/Poderes/' + name, 'asset_type': 'niagara', 'timestamps': list(times),
                                                         'camera': CAMS[cam], 'resolution': [size, size], 'output_dir': out, 'filename_prefix': 'f'})
    files = [f['file'] for f in r.get('frames', []) if f.get('success')]
    if not files: print(r); return None
    im = Image.new('RGB', (size * len(files), size))
    for i, f in enumerate(files): im.paste(Image.open(f).convert('RGB'), (i * size, 0))
    dst = ROOT + name + '_' + cam + '.png'
    im.save(dst); print(dst); return dst


if __name__ == '__main__':
    t = [float(x) for x in sys.argv[2].split(',')] if len(sys.argv) > 2 else (0.05, 0.15, 0.3, 0.5, 0.8)
    sheet(sys.argv[1], t, sys.argv[3] if len(sys.argv) > 3 else 'q')
