#!/bin/bash
# Gera tudo do elfo no Blender sem janela, 3 por vez (09/11): orelhas, peças P_elfo_*, trajes Elfo_* e as saias do humano (agora em tecido).
# bash Tools/Blender/MakeAllElf.sh
cd "$(dirname "$0")/../.."
B="/c/Program Files/Blender Foundation/Blender 5.2/blender.exe"
python -c "
import re
t = open('Data/Looks/pecas.json', encoding='utf-8').read()
ids = re.findall(r'\"((?:P_elfo_|P_orelha_)\w+|P_saia_\w+)\": \{', t)
print('\n'.join(ids + ['Elfo_%s_%s' % (a, b) for a in ('floresta', 'mago', 'sombra') for b in 'mf']))
" | tr -d '\r' | xargs -P 3 -I{} sh -c "\"$B\" -b --factory-startup -P Tools/Blender/MakeBlockOutfit.py -- jogo {} 2>&1 | grep -E 'exportado|Traceback|Error:' | cut -c1-120"
echo TERMINOU
