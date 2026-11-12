"""HUD: molduras, barras e ícones do Godot (somente leitura) em /Game/Vortex/UI/Hud e os materiais
do retrato e do minimapa. Rode com o editor fechado:

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=Tools/ImportHud.py -unattended

Origem: the-world-geneses/ui/art/hud (molduras já recortadas pelo make_hud_frames.py do Godot, com
as posições em layout.json, copiadas para Source como constantes) e ui/art/icons (ícones das ações).
- M_HudPortrait: retrato redondo (a cor do modelo capturada numa textura) com fundo azul-noite.
- M_HudMinimap: recorte redondo do mapa do vale (textura feita pelo jogo a partir do terreno).
Os dois são de interface (domínio UI), desenhados pelo AVortexAimHUD no Canvas.
"""
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
godot = os.path.normpath(os.path.join(root, "..", "..", "the-world-geneses"))
tools = unreal.AssetToolsHelpers.get_asset_tools()
library = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
FOLDER = "/Game/Vortex/UI/Hud"

FRAMES = ["player_frame", "player_hp_fill", "player_hp_under", "player_mp_fill", "player_mp_under",
          "player_xp_fill", "player_xp_under", "boss_frame", "boss_hp_fill", "boss_hp_under", "boss_cast_fill",
          "hotbar_frame", "minimap_frame", "potion_slot"]
ICONS = ["ability_sword_ashen_arc", "ability_flying_slash", "potion_red", "potion_blue"]

lines = []
for folder, names in ((os.path.join(godot, "ui", "art", "hud"), FRAMES), (os.path.join(godot, "ui", "art", "icons"), ICONS)):
    for name in names:
        task = unreal.AssetImportTask()
        task.filename = os.path.join(folder, name + ".png")
        task.destination_path = FOLDER
        task.destination_name = "T_" + name
        task.automated = True
        task.replace_existing = True
        task.save = False
        tools.import_asset_tasks([task])
        texture = library.load_asset(FOLDER + "/T_" + name)
        if not texture:
            raise RuntimeError("falhou: " + name)
        texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
        texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        texture.set_editor_property("srgb", True)
        texture.set_editor_property("address_x", unreal.TextureAddress.TA_CLAMP)
        texture.set_editor_property("address_y", unreal.TextureAddress.TA_CLAMP)
        library.save_loaded_asset(texture)
        lines.append("T_" + name)


def ui_material(name):
    path = FOLDER + "/" + name
    if library.does_asset_exist(path):
        library.delete_asset(path)
    material = tools.create_asset(name, FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    material.set_editor_property("material_domain", unreal.MaterialDomain.MD_UI)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    return material


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


placeholder = library.load_asset(FOLDER + "/T_player_xp_under")

# ---- retrato: textura (captura) em círculo; fora do boneco, azul-noite com brilho no centro
m = ui_material("M_HudPortrait")
uv = MEL.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -1000, 0)
tex = MEL.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -1000, 150)
tex.set_editor_property("parameter_name", "Portrait")
tex.set_editor_property("texture", placeholder)
ready = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -1000, 400)
ready.set_editor_property("parameter_name", "Ready")
ready.set_editor_property("default_value", 0.0)
color = custom(m, "Retrato", """
float2 p = UV - 0.5;
float r = length(p) * 2.0;
float3 back = lerp(float3(0.035, 0.07, 0.16), float3(0.004, 0.008, 0.025), saturate(r));
// a captura é a cor do modelo: onde não há nada, sai preto
float cover = Ready * saturate(max(T.r, max(T.g, T.b)) * 300.0);
return lerp(back, T.rgb, cover);
""", [("UV", (uv, "")), ("T", (tex, "RGBA")), ("Ready", (ready, ""))],
    unreal.CustomMaterialOutputType.CMOT_FLOAT3, 0)
MEL.connect_material_property(color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mask = custom(m, "Circulo", "return 1.0 - smoothstep(0.96, 1.0, length(UV - 0.5) * 2.0);",
              [("UV", (uv, ""))], unreal.CustomMaterialOutputType.CMOT_FLOAT1, 200)
MEL.connect_material_property(mask, "", unreal.MaterialProperty.MP_OPACITY)
MEL.recompile_material(m)
library.save_loaded_asset(m)
lines.append("M_HudPortrait")

# ---- minimapa: View = (centro u, centro v, meia largura em u, meia altura em v)
m = ui_material("M_HudMinimap")
uv = MEL.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -1200, 0)
view = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -1200, 100)
view.set_editor_property("parameter_name", "View")
view.set_editor_property("default_value", unreal.LinearColor(0.5, 0.5, 0.05, 0.05))
where = custom(m, "Onde", "return V.xy + (UV - 0.5) * 2.0 * V.zw;", [("UV", (uv, "")), ("V", (view, "RGBA"))],
               unreal.CustomMaterialOutputType.CMOT_FLOAT2, 0)
tex = MEL.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -800, 150)
tex.set_editor_property("parameter_name", "Map")
tex.set_editor_property("texture", placeholder)
MEL.connect_material_expressions(where, "", tex, "UVs")
color = custom(m, "Mapa", """
float2 p = UV - 0.5;
float r = length(p) * 2.0;
bool inside = all(M >= 0.0) && all(M <= 1.0);
float3 c = inside ? T.rgb : float3(0.01, 0.02, 0.05);
// borda: escurece perto do aro dourado; centro levemente mais claro
c *= lerp(1.08, 0.55, smoothstep(0.55, 1.0, r));
return c;
""", [("UV", (uv, "")), ("M", (where, "")), ("T", (tex, "RGBA"))], unreal.CustomMaterialOutputType.CMOT_FLOAT3, 300)
MEL.connect_material_property(color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mask = custom(m, "Circulo", "return 1.0 - smoothstep(0.97, 1.0, length(UV - 0.5) * 2.0);",
              [("UV", (uv, ""))], unreal.CustomMaterialOutputType.CMOT_FLOAT1, 450)
MEL.connect_material_property(mask, "", unreal.MaterialProperty.MP_OPACITY)
MEL.recompile_material(m)
library.save_loaded_asset(m)
lines.append("M_HudMinimap")

with open(os.path.join(root, "Saved", "Logs", "ImportHud.txt"), "w") as out:
    out.write("VORTEX_HUD " + " ".join(lines) + "\n")
