"""Herói fosco (11/11): as roupas, a pele e o cabelo brilhavam demais ao sol (o usuário: "o meu personagem tá brilhando
muito as roupas"). Nos materiais-mestre do herói (os pais do MI_Hero_humano, das roupas MI_Roupa_* e dos cabelos
M_BlockHair*, e o M_ArmorBlock):
- especular 0,15 (antes o padrão da Unreal, 0,5: o brilho de plástico nas quinas);
- rugosidade 0,95 no lugar de 0,85 (o parâmetro "Roughness", no mestre e nas instâncias que o mudam);
- o contorno de luz das bordas (RimColor do M_ArmorBlock) pela metade.
O metal das armaduras continua brilhando (ele usa outra rugosidade) e o molhado (Tools/AddWetLook.py) continua valendo:
ele mistura a partir do que já está na rugosidade. Rodar de novo não duplica (o nó "VortexMatte" marca).

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<raiz>/Tools/MatteHeroMaterials.py -unattended -nullrhi
"""
import unreal

library = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
SPECULAR, ROUGH, RIM = 0.15, 0.95, unreal.LinearColor(0.11, 0.11, 0.13, 1.0)
masters, instances = {}, []
for p in library.list_assets("/Game/Vortex/Characters/Heroes", recursive=True):
    data = library.find_asset_data(p)
    kind = str(data.asset_class_path.asset_name) if data else ""
    if kind not in ("MaterialInstanceConstant", "Material"):
        continue
    asset = library.load_asset(p)
    if isinstance(asset, unreal.MaterialInstanceConstant):
        instances.append(asset)
    base = asset
    while isinstance(base, unreal.MaterialInstance):
        base = base.get_editor_property("parent")
    if isinstance(base, unreal.Material):
        masters[base.get_path_name()] = base

for path, mat in masters.items():
    current = mel.get_material_property_input_node(mat, unreal.MaterialProperty.MP_SPECULAR)
    if not (current and current.get_editor_property("desc") == "VortexMatte"):
        spec = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 760)
        spec.set_editor_property("r", SPECULAR)
        spec.set_editor_property("desc", "VortexMatte")
        mel.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)
    mel.recompile_material(mat)
    library.save_loaded_asset(mat)
    unreal.log("VORTEX_MATTE mestre %s" % path)

# padrões dos parâmetros: o Python não lista os nós do material, então o valor vai em cada instância (vale igual)
for mi in instances:
    names = [str(n) for n in mel.get_scalar_parameter_names(mi)]
    if "Roughness" in names:
        mel.set_material_instance_scalar_parameter_value(mi, "Roughness", ROUGH)
    if "RimColor" in [str(n) for n in mel.get_vector_parameter_names(mi)]:
        mel.set_material_instance_vector_parameter_value(mi, "RimColor", RIM)
    library.save_loaded_asset(mi)
unreal.log("VORTEX_MATTE ok: %d mestres, %d instâncias" % (len(masters), len(instances)))
