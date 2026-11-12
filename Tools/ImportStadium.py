"""Materiais do Estádio do Despertar (texturas do Godot, somente leitura) — rode com o editor fechado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=Tools/ImportStadium.py -unattended

the-world-geneses/Assets/Stadium/Textures/<material>_{albedo,normal,rough}.png (geradas pelo
stadium_textures.py do Godot) -> /Game/Vortex/Stadium/Textures, e:
- M_StadiumStone: PBR com projeção no espaço do mundo (WorldAlignedTexture), como o triplanar do
  Godot: os blocos continuam de uma peça para a outra. Parâmetros: Albedo, Normal, Rough, Tile
  (metros), Metallic, RoughScale, Tint;
- MI_Stadium_<material>: areia, calcário, travertino, mármore, mármore escuro, basalto, madeira,
  bronze, grama e tecido (tamanhos de ladrilho do AwakeningStadium.Parts.cs);
- M_StadiumGlow: fogo dos braseiros e o portal dourado (emissivo, cor por instância).
"""
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
source = os.path.normpath(os.path.join(root, "..", "..", "the-world-geneses", "Assets", "Stadium", "Textures"))
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
FOLDER = "/Game/Vortex/Stadium"
# nome: (ladrilho em metros, metálico, aspereza)
MATERIALS = {"sand": (6, 0, 1), "limestone_blocks": (3.2, 0, 1), "travertine": (4, 0, 1), "marble": (3, 0, .9),
             "marble_dark": (3, 0, .9), "basalt": (2.5, 0, 1), "wood": (2.2, 0, 1), "bronze": (1.2, .85, 1),
             "grass": (7, 0, 1), "cloth": (1.5, 0, 1)}


def texture(name, kind):
    asset = "T_Stadium_%s_%s" % (name, kind)
    task = unreal.AssetImportTask()
    task.filename = os.path.join(source, "%s_%s.png" % (name, kind))
    task.destination_path = FOLDER + "/Textures"
    task.destination_name = asset
    task.automated = True
    task.replace_existing = True
    task.save = False
    tools.import_asset_tasks([task])
    tex = library.load_asset(FOLDER + "/Textures/" + asset)
    if kind == "normal":
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        tex.set_editor_property("srgb", False)
    elif kind == "rough":
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
        tex.set_editor_property("srgb", False)
    library.save_loaded_asset(tex)
    return tex


def fresh_material(name):
    path = FOLDER + "/" + name
    if library.does_asset_exist(path):
        library.delete_asset(path)
    return tools.create_asset(name, FOLDER, unreal.Material, unreal.MaterialFactoryNew())


# ---- mestre de pedra (projeção no mundo)
stone = fresh_material("M_StadiumStone")
aligned = unreal.load_asset("/Engine/Functions/Engine_MaterialFunctions01/Texturing/WorldAlignedTexture.WorldAlignedTexture")
aligned_normal = unreal.load_asset("/Engine/Functions/Engine_MaterialFunctions01/Texturing/WorldAlignedNormal.WorldAlignedNormal")
tile = MEL.create_material_expression(stone, unreal.MaterialExpressionScalarParameter, -1400, 600)
tile.set_editor_property("parameter_name", "Tile")
tile.set_editor_property("default_value", 3.0)
cm = MEL.create_material_expression(stone, unreal.MaterialExpressionMultiply, -1200, 600)
cm.set_editor_property("const_b", 100.0)
MEL.connect_material_expressions(tile, "", cm, "A")
size = MEL.create_material_expression(stone, unreal.MaterialExpressionAppendVector, -1000, 600)
size2 = MEL.create_material_expression(stone, unreal.MaterialExpressionAppendVector, -900, 600)
MEL.connect_material_expressions(cm, "", size, "A")
MEL.connect_material_expressions(cm, "", size, "B")
MEL.connect_material_expressions(size, "", size2, "A")
MEL.connect_material_expressions(cm, "", size2, "B")


def projected(param, default_tex, y, normal=False):
    t = MEL.create_material_expression(stone, unreal.MaterialExpressionTextureObjectParameter, -1100, y)
    t.set_editor_property("parameter_name", param)
    t.set_editor_property("texture", default_tex)
    if normal:
        t.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    call = MEL.create_material_expression(stone, unreal.MaterialExpressionMaterialFunctionCall, -600, y)
    call.set_editor_property("material_function", aligned_normal if normal else aligned)
    MEL.connect_material_expressions(t, "", call, "TextureObject" if not normal else "TextureObject")
    MEL.connect_material_expressions(size2, "", call, "TextureSize")
    return call


defaults = {kind: texture("limestone_blocks", kind) for kind in ("albedo", "normal", "rough")}
albedo = projected("Albedo", defaults["albedo"], -200)
normal = projected("Normal", defaults["normal"], 100, True)
rough = projected("Rough", defaults["rough"], 400)
tint = MEL.create_material_expression(stone, unreal.MaterialExpressionVectorParameter, -600, -400)
tint.set_editor_property("parameter_name", "Tint")
tint.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
color = MEL.create_material_expression(stone, unreal.MaterialExpressionMultiply, -250, -250)
MEL.connect_material_expressions(albedo, "XYZ Texture", color, "A")
MEL.connect_material_expressions(tint, "", color, "B")
MEL.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
MEL.connect_material_property(normal, "", unreal.MaterialProperty.MP_NORMAL)
rough_scale = MEL.create_material_expression(stone, unreal.MaterialExpressionScalarParameter, -600, 700)
rough_scale.set_editor_property("parameter_name", "RoughScale")
rough_scale.set_editor_property("default_value", 1.0)
mask = MEL.create_material_expression(stone, unreal.MaterialExpressionComponentMask, -300, 400)
mask.set_editor_property("r", True)
mask.set_editor_property("g", False)
mask.set_editor_property("b", False)
mask.set_editor_property("a", False)
MEL.connect_material_expressions(rough, "XYZ Texture", mask, "")
roughness = MEL.create_material_expression(stone, unreal.MaterialExpressionMultiply, -150, 450)
MEL.connect_material_expressions(mask, "", roughness, "A")
MEL.connect_material_expressions(rough_scale, "", roughness, "B")
MEL.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
metal = MEL.create_material_expression(stone, unreal.MaterialExpressionScalarParameter, -300, 800)
metal.set_editor_property("parameter_name", "Metallic")
MEL.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)
MEL.recompile_material(stone)
library.save_loaded_asset(stone)

made = []
for name, (tile_m, metallic, rough_k) in MATERIALS.items():
    textures = {kind: texture(name, kind) for kind in ("albedo", "normal", "rough")}
    path = FOLDER + "/MI_Stadium_" + name
    mi = library.load_asset(path) if library.does_asset_exist(path) else \
        tools.create_asset("MI_Stadium_" + name, FOLDER, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mi, stone)
    MEL.set_material_instance_texture_parameter_value(mi, "Albedo", textures["albedo"])
    MEL.set_material_instance_texture_parameter_value(mi, "Normal", textures["normal"])
    MEL.set_material_instance_texture_parameter_value(mi, "Rough", textures["rough"])
    MEL.set_material_instance_scalar_parameter_value(mi, "Tile", tile_m)
    MEL.set_material_instance_scalar_parameter_value(mi, "Metallic", metallic)
    MEL.set_material_instance_scalar_parameter_value(mi, "RoughScale", rough_k)
    library.save_loaded_asset(mi)
    made.append(name)

# ---- brilho (fogo, portal)
glow = fresh_material("M_StadiumGlow")
glow.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
glow.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
glow.set_editor_property("two_sided", True)
gc = MEL.create_material_expression(glow, unreal.MaterialExpressionVectorParameter, -500, 0)
gc.set_editor_property("parameter_name", "Color")
gc.set_editor_property("default_value", unreal.LinearColor(1, .55, .15, 1))
gs = MEL.create_material_expression(glow, unreal.MaterialExpressionScalarParameter, -500, 200)
gs.set_editor_property("parameter_name", "Glow")
gs.set_editor_property("default_value", 4.0)
gm = MEL.create_material_expression(glow, unreal.MaterialExpressionMultiply, -250, 100)
MEL.connect_material_expressions(gc, "", gm, "A")
MEL.connect_material_expressions(gs, "", gm, "B")
MEL.connect_material_property(gm, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(glow)
library.save_loaded_asset(glow)

with open(os.path.join(root, "Saved", "Logs", "ImportStadium.txt"), "w") as out:
    out.write("VORTEX_STADIUM materiais=%s\n" % ",".join(made))
