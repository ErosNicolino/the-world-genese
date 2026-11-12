"""Metal das roupas menos espelhado (11/11): no M_ArmorBlock a rugosidade do metal (o B do lerp da rugosidade, canal azul da
máscara) era a constante 0,36 e brilhava como espelho até na sombra; foi para 0,65 e, à noite, para 0,9, com o metal de
0,12 para 0,05. Sem refazer o material (o ImportBlockArmor.py apaga tudo e perderia o molhado e o fosco que vieram depois);
o ImportBlockArmor.py já sai com estes valores.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<raiz>/Tools/ArmorMetalRough.py -unattended -nullrhi
"""
import unreal

library = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
# 11/11, de noite (o usuário: "ainda está com reflexo mesmo na sombra"): o reflexo é o do céu, que a sombra do sol não
# tampa; o metal fica bem fosco (0,9) e com menos metal (0,05 no lugar de 0,12)
ROUGH = ((0.36, 0.9), (0.65, 0.9))
METAL = ((0.12, 0.05), (0.35, 0.05))   # no material estava 0,35 (o ImportBlockArmor.py dizia 0,12)
mat = library.load_asset("/Game/Vortex/Characters/Heroes/humano/Armaduras/M_ArmorBlock")


found = {}


def walk(prop, pairs):
    """Troca as constantes da cadeia de uma entrada do material (o Python não lista os nós: anda a partir da entrada)."""
    seen, todo, changed = set(), [mel.get_material_property_input_node(mat, prop)], 0
    found[str(prop)] = []
    while todo:
        n = todo.pop()
        if not n or n.get_name() in seen:
            continue
        seen.add(n.get_name())
        if isinstance(n, unreal.MaterialExpressionConstant):
            found[str(prop)].append(round(n.get_editor_property("r"), 3))
            for old, new in pairs:
                if abs(n.get_editor_property("r") - old) < 1e-3:
                    n.set_editor_property("r", new)
                    changed += 1
        todo += [i for i in mel.get_inputs_for_material_expression(mat, n) if i]
    return changed


rough, metal = walk(unreal.MaterialProperty.MP_ROUGHNESS, ROUGH), walk(unreal.MaterialProperty.MP_METALLIC, METAL)
unreal.log_warning("VORTEX_ARMOR_ROUGH constantes vistas: %s" % found)
if rough != 1 or metal != 1:
    raise RuntimeError("VORTEX_ARMOR_ROUGH esperava 1 constante em cada (rugosidade %d, metal %d)" % (rough, metal))
mel.recompile_material(mat)
library.save_loaded_asset(mat)
unreal.log_warning("VORTEX_ARMOR_ROUGH ok: rugosidade do metal 0,9 e metal 0,05")
