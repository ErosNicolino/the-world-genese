"""Material de interface da vitrine do herói (janela do herói, mochila): mostra a imagem da câmera do herói
(parâmetro "Doll") sempre opaca. A câmera grava só a cor, com a transparência zerada, e uma imagem direta no
Slate ficava invisível. A opacidade vem da cor do widget (ColorAndOpacity). Rodar com o editor fechado:
  UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<este arquivo> -unattended -nullrhi
"""
import unreal

PATH = "/Game/Vortex/UI/KitAventura"
NAME = "M_UiDoll"
lib = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
full = f"{PATH}/{NAME}"
if unreal.EditorAssetLibrary.does_asset_exist(full):
    unreal.EditorAssetLibrary.delete_asset(full)
mat = tools.create_asset(NAME, PATH, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_UI)
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)

tex = lib.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -400, 0)
tex.set_editor_property("parameter_name", "Doll")
default = unreal.load_asset("/Engine/EngineResources/Black.Black")
if default:
    tex.set_editor_property("texture", default)
vertex = lib.create_material_expression(mat, unreal.MaterialExpressionVertexColor, -400, 300)
mul = lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, -150, 0)
lib.connect_material_expressions(tex, "RGB", mul, "A")
lib.connect_material_expressions(vertex, "RGB", mul, "B")
lib.connect_material_property(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
lib.connect_material_property(vertex, "A", unreal.MaterialProperty.MP_OPACITY)
lib.recompile_material(mat)
unreal.EditorAssetLibrary.save_asset(full)
unreal.log_warning("VORTEX_UI_DOLL_MATERIAL ok")
