"""Material do relevo distante (AVortexFarTerrain): a cor de cada vértice (o chão da região, a neve,
o mar), fosco, sem textura. Rodar com o editor fechado:
  UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<este arquivo> -unattended -nullrhi
"""
import unreal

tools = unreal.AssetToolsHelpers.get_asset_tools()
library = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
PATH = "/Game/Vortex/Materials"
NAME = "M_FarTerrain"

material = library.load_asset(f"{PATH}/{NAME}") if library.does_asset_exist(f"{PATH}/{NAME}") else \
    tools.create_asset(NAME, PATH, unreal.Material, unreal.MaterialFactoryNew())
mel.delete_all_material_expressions(material)
vc = mel.create_material_expression(material, unreal.MaterialExpressionVertexColor, -500, 0)
mel.connect_material_property(vc, "", unreal.MaterialProperty.MP_BASE_COLOR)
rough = mel.create_material_expression(material, unreal.MaterialExpressionConstant, -500, 200)
rough.set_editor_property("r", .95)
mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
spec = mel.create_material_expression(material, unreal.MaterialExpressionConstant, -500, 280)
spec.set_editor_property("r", .15)
mel.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)
mel.recompile_material(material)
library.save_loaded_asset(material)
unreal.log("VORTEX_FAR_TERRAIN_MATERIAL ok")
