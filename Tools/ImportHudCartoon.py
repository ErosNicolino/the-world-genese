"""HUD cartoon: as peças de Data/UI/HudCartoon (Tools/MakeHudCartoon.py) e o material do minimapa que gira. Rode com o
editor fechado:
  UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportHudCartoon.py -unattended -nullrhi

- /Game/Vortex/UI/HudCartoon/T_<peça>: interface, sem mipmaps, bordas presas.
- /Game/Vortex/UI/HudCartoon/M_HudMinimapCartoon: o recorte redondo do mapa local (1 ponto por bloco de 0,75 m, feito pelo
  jogo em volta do herói). View = (centro u, centro v, meia largura u, meia altura v); Angle = giro em radianos (o mapa
  gira com a câmera, o norte anda na borda). A borda escurece de leve; fora do mapa, azul-fundo. A cor sai já para a tela
  (o Canvas não converte: sem a gama, o minimapa ficava bem mais escuro que o mapa grande).
"""
import glob
import os
import unreal

ROOT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
FOLDER = "/Game/Vortex/UI/HudCartoon"
tools = unreal.AssetToolsHelpers.get_asset_tools()
library = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
lines = []

count = 0
for path in sorted(glob.glob(os.path.join(ROOT, "Data", "UI", "HudCartoon", "*.png"))):
    name = os.path.basename(path)[:-4]
    if name == "preview":
        continue
    task = unreal.AssetImportTask()
    task.filename = path
    task.destination_path = FOLDER
    task.destination_name = "T_" + name
    task.automated = True
    task.replace_existing = True
    task.save = False
    tools.import_asset_tasks([task])
    texture = library.load_asset(f"{FOLDER}/T_{name}")
    if not texture:
        raise RuntimeError("falhou: " + name)
    texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property("srgb", True)
    texture.set_editor_property("address_x", unreal.TextureAddress.TA_CLAMP)
    texture.set_editor_property("address_y", unreal.TextureAddress.TA_CLAMP)
    library.save_loaded_asset(texture)
    count += 1
lines.append(f"{count} peças")


def custom(material, name, code, inputs, output, y):
    node = MEL.create_material_expression(material, unreal.MaterialExpressionCustom, -400, y)
    node.set_editor_property("output_type", output)
    node.set_editor_property("description", name)
    node.set_editor_property("code", code)
    items = []
    for input_name, _ in inputs:
        item = unreal.CustomInput()
        item.set_editor_property("input_name", input_name)
        items.append(item)
    node.set_editor_property("inputs", items)
    for input_name, (source, pin) in inputs:
        if not MEL.connect_material_expressions(source, pin, node, input_name):
            raise RuntimeError("ligação falhou: %s.%s" % (name, input_name))
    return node


path = FOLDER + "/M_HudMinimapCartoon"
if library.does_asset_exist(path):
    library.delete_asset(path)
m = tools.create_asset("M_HudMinimapCartoon", FOLDER, unreal.Material, unreal.MaterialFactoryNew())
m.set_editor_property("material_domain", unreal.MaterialDomain.MD_UI)
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
uv = MEL.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -1200, 0)
view = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -1200, 100)
view.set_editor_property("parameter_name", "View")
view.set_editor_property("default_value", unreal.LinearColor(0.5, 0.5, 0.1, 0.1))
angle = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -1200, 250)
angle.set_editor_property("parameter_name", "Angle")
angle.set_editor_property("default_value", 0.0)
where = custom(m, "Onde", """
float2 p = (UV - 0.5) * 2.0;
float c = cos(A), s = sin(A);
float2 q = float2(c * p.x - s * p.y, s * p.x + c * p.y);
return V.xy + q * V.zw;
""", [("UV", (uv, "")), ("V", (view, "RGBA")), ("A", (angle, ""))], unreal.CustomMaterialOutputType.CMOT_FLOAT2, 0)
tex = MEL.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -800, 150)
tex.set_editor_property("parameter_name", "Map")
tex.set_editor_property("texture", library.load_asset(FOLDER + "/T_H_Disc"))
MEL.connect_material_expressions(where, "", tex, "UVs")
color = custom(m, "Mapa", """
float r = length(UV - 0.5) * 2.0;
bool inside = all(M >= 0.0) && all(M <= 1.0);
float3 c = inside ? T.rgb : float3(0.03, 0.06, 0.16);
c *= lerp(1.0, 0.85, smoothstep(0.8, 1.0, r));
// o Canvas da HUD não converte a cor do material para a tela: sem isto o minimapa saía bem mais escuro que o mapa grande
return pow(saturate(c), 1.0 / 2.2);
""", [("UV", (uv, "")), ("M", (where, "")), ("T", (tex, "RGBA"))], unreal.CustomMaterialOutputType.CMOT_FLOAT3, 300)
MEL.connect_material_property(color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mask = custom(m, "Circulo", "return 1.0 - smoothstep(0.985, 1.0, length(UV - 0.5) * 2.0);", [("UV", (uv, ""))],
              unreal.CustomMaterialOutputType.CMOT_FLOAT1, 450)
MEL.connect_material_property(mask, "", unreal.MaterialProperty.MP_OPACITY)
MEL.recompile_material(m)
library.save_loaded_asset(m)
lines.append("M_HudMinimapCartoon")
with open(os.path.join(ROOT, "Saved", "Logs", "ImportHudCartoon.txt"), "w", encoding="utf-8") as out:
    out.write("\n".join(lines) + "\n")
unreal.log_warning("VORTEX_HUD_CARTOON_IMPORT " + " | ".join(lines))
