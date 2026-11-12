"""Conserta materiais que compilam no editor mas falham no empacotamento (o jogo usaria o material padrão):
- M_WeaponVertexColor: o nó de código lia C.a, mas em alguns passes (Lumen, seleção) a cor do vértice chega
  com 3 canais. Agora o alfa entra numa entrada própria (A). O material é refeito no lugar (as armas
  continuam apontando para ele).
Também lista quem usa os outros materiais com erro no empacotamento. Rodar com o editor fechado:
  UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<este arquivo> -unattended -nullrhi
"""
import unreal

library = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary

m = library.load_asset("/Game/Vortex/Weapons/M_WeaponVertexColor")
MEL.delete_all_material_expressions(m)
m.set_editor_property("two_sided", True)
vc = MEL.create_material_expression(m, unreal.MaterialExpressionVertexColor, -800, 0)
node = MEL.create_material_expression(m, unreal.MaterialExpressionCustom, -400, 200)
node.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
node.set_editor_property("code", "return C * saturate(1.0 - A) * 9.0;")
inputs = []
for name in ("C", "A"):
    item = unreal.CustomInput()
    item.set_editor_property("input_name", name)
    inputs.append(item)
node.set_editor_property("inputs", inputs)
rgb = MEL.create_material_expression(m, unreal.MaterialExpressionComponentMask, -600, 150)
rgb.set_editor_property("r", True)
rgb.set_editor_property("g", True)
rgb.set_editor_property("b", True)
rgb.set_editor_property("a", False)
MEL.connect_material_expressions(vc, "", rgb, "")
MEL.connect_material_expressions(rgb, "", node, "C")
MEL.connect_material_expressions(vc, "A", node, "A")
MEL.connect_material_property(rgb, "", unreal.MaterialProperty.MP_BASE_COLOR)
MEL.connect_material_property(node, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
rough = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -400, 400)
rough.set_editor_property("r", .55)
MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
MEL.recompile_material(m)
library.save_loaded_asset(m)
unreal.log_warning("VORTEX_FIXMAT arma ok")

registry = unreal.AssetRegistryHelpers.get_asset_registry()
for path in ("/Game/Vortex/Creatures/Wolf/M_WolfEyes_Rebuilt", "/Game/Vortex/Creatures/Wolf/M_WolfFur_Rebuilt", "/Game/Basic_VFX/Materials/M_poison"):
    opts = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True)
    refs = registry.get_referencers(path, opts) or []
    unreal.log_warning(f"VORTEX_FIXMAT usa {path}: {[str(r) for r in refs]}")
