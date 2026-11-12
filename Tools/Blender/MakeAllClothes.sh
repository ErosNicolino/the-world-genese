#!/bin/bash
# Gera todas as roupas e armaduras no Blender sem janela, 3 por vez (09/11): peças P_*, trajes Elfo_*, armaduras A_* e os trajes de herói e NPC.
# bash Tools/Blender/MakeAllClothes.sh   (o relatório sai na tela; no fim, TERMINOU)
cd "$(dirname "$0")/../.."
B="/c/Program Files/Blender Foundation/Blender 5.2/blender.exe"
python -c "
import json, re
ids = list(json.load(open('Data/Looks/pecas.json', encoding='utf-8'))['pieces'])
ids += ['Elfo_%s_%s' % (a, b) for a in ('floresta', 'mago', 'sombra') for b in 'mf']
for k in json.load(open('Data/Rpg/armors.json', encoding='utf-8'))['sets']:
    ids += ['A_%s_%s' % (k, p) for p in ('head', 'chest', 'legs', 'boots')]
src = open('Tools/Blender/MakeBlockOutfit.py', encoding='utf-8').read()
ids += re.findall(r'\"(\w+)\": \w+[,}]', src[src.index('OUTFITS = {'):src.index('if PIECE:', src.index('OUTFITS = {'))])
print('\n'.join(ids))
" | tr -d '\r' | xargs -P 3 -I{} sh -c "\"$B\" -b --factory-startup -P Tools/Blender/MakeBlockOutfit.py -- jogo {} 2>&1 | grep -E 'exportado|Traceback|Error:' | cut -c1-110"
python Tools/Blender/piece_designs.py | tail -1
echo TERMINOU
