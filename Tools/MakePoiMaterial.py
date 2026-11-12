"""Material dos pontos de interesse e das teias: a cor de cada bloco (cor do vértice) como cor de base, fosca.
O M_FarTerrain (do terreno distante) puxava tudo para a cor da terra: as ruínas de pedra cinza saíam marrons.
Rodar com o editor fechado:
  UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<este arquivo> -unattended -nullrhi
"""
import unreal

tools = unreal.AssetToolsHelpers.get_asset_tools()
library = unreal.EditorAssetLibrary
lib = unreal.MaterialEditingLibrary
full = "/Game/Vortex/World/M_PoiBlocks"
if library.does_asset_exist(full):
    library.delete_asset(full)
mat = tools.create_asset("M_PoiBlocks", "/Game/Vortex/World", unreal.Material, unreal.MaterialFactoryNew())
vertex = lib.create_material_expression(mat, unreal.MaterialExpressionVertexColor, -500, 0)
# a saída sem nome é a cor (o VertexColor não tem "RGB": com esse nome a ligação falhava e tudo saía escuro)
ok = lib.connect_material_property(vertex, "", unreal.MaterialProperty.MP_BASE_COLOR)
unreal.log_warning(f"VORTEX_POI_MATERIAL cor ligada: {ok}")
rough = lib.create_material_expression(mat, unreal.MaterialExpressionConstant, -500, 200)
rough.set_editor_property("r", .92)
lib.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
lib.recompile_material(mat)
library.save_asset(full)
unreal.log_warning("VORTEX_POI_MATERIAL ok")
