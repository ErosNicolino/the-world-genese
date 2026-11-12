#!/bin/bash
# Gera as 84 peças de armadura no Blender sem janela (Tools/Blender/MakeBlockOutfit.py + armor_designs.py), uma por vez.
# bash Tools/Armor/MakeAllArmor.sh [conjunto ...]   (sem nada: todos os do Data/Rpg/armors.json)
cd "$(dirname "$0")/../.."
B="/c/Program Files/Blender Foundation/Blender 5.2/blender.exe"
SETS="$*"
[ -z "$SETS" ] && SETS=$(python -c "import json; print(' '.join(json.load(open('Data/Rpg/armors.json', encoding='utf-8'))['sets']))")
for k in $SETS; do
  for p in head chest legs boots; do
    "$B" -b --factory-startup -P Tools/Blender/MakeBlockOutfit.py -- jogo "A_${k}_$p" 2>&1 | grep -E "peças|Error|Traceback" | sed "s/^/[$k $p] /"
  done
done
echo "ARMADURAS_FEITAS"
