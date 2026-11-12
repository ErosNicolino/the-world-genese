"""Importa os modelos de planta/pedra do Godot (Art/World/<espécie>.glb) usados na região exportada.

Lê Data/GodotTerrain/props.bin para saber as espécies, importa cada .glb para
/Game/Vortex/Props/<espécie> (malha estática com Nanite ligado) e cria os materiais do terreno.
O Godot e seus arquivos não são alterados (só leitura). Rodar:
  UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<este arquivo> [-- only=tree_oak]
"""

import os
import struct
import sys
import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
GODOT_ART = os.path.normpath(os.path.join(PROJECT, "..", "..", "the-world-geneses", "Art", "World"))
DEST = "/Game/Vortex/Props"


def kinds_from_export():
    with open(os.path.join(PROJECT, "Data", "GodotTerrain", "props.bin"), "rb") as f:
        data = f.read()
    count = struct.unpack_from("<i", data, 8)[0]
    o, names = 12, []
    for _ in range(count):
        n = data[o]
        names.append(data[o + 1:o + 1 + n].decode("utf-8"))
        o += 1 + n
    return names


only = [a.split("=", 1)[1] for a in sys.argv if a.startswith("only=")]
names = only[0].split(",") if only else kinds_from_export()
tools = unreal.AssetToolsHelpers.get_asset_tools()
imported, missing = [], []
for name in names:
    src = os.path.join(GODOT_ART, name + ".glb")
    if not os.path.exists(src):
        missing.append(name)
        continue
    if unreal.EditorAssetLibrary.does_asset_exist(f"{DEST}/{name}/{name}"):
        continue
    task = unreal.AssetImportTask()
    task.filename = src
    task.destination_path = f"{DEST}/{name}"
    task.automated = True
    task.save = True
    task.replace_existing = True
    tools.import_asset_tasks([task])
    imported.append(name)

# Nanite em todas as malhas importadas (a Unreal escolhe o detalhe pela distância sozinha)
for path in unreal.EditorAssetLibrary.list_assets(DEST, recursive=True):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if isinstance(asset, unreal.StaticMesh):
        settings = asset.get_editor_property("nanite_settings")
        if not settings.enabled:
            settings.enabled = True
            asset.set_editor_property("nanite_settings", settings)
            unreal.EditorAssetLibrary.save_loaded_asset(asset)
unreal.log(f"VORTEX_PROPS_IMPORTED importados={len(imported)} faltando={missing}")


# ---- materiais ----------------------------------------------------------------------------
MEL = unreal.MaterialEditingLibrary
MAT_DIR = "/Game/Vortex/Materials"


def make_material(name, build):
    path = f"{MAT_DIR}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.EditorAssetLibrary.load_asset(path)
    material = tools.create_asset(name, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    build(material)
    MEL.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material


def voxel_prop(material):
    # cor do vértice (com a sombra dos cantos) x atlas da espécie, como o VoxelProp.gdshader
    color = MEL.create_material_expression(material, unreal.MaterialExpressionVertexColor, -600, 0)
    atlas = MEL.create_material_expression(material, unreal.MaterialExpressionTextureSampleParameter2D, -600, 200)
    atlas.set_editor_property("parameter_name", "Atlas")
    atlas.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    mul = MEL.create_material_expression(material, unreal.MaterialExpressionMultiply, -300, 100)
    MEL.connect_material_expressions(color, "", mul, "A")
    MEL.connect_material_expressions(atlas, "RGB", mul, "B")
    MEL.connect_material_property(mul, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = MEL.create_material_expression(material, unreal.MaterialExpressionConstant, -300, 300)
    rough.set_editor_property("r", 0.9)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)


def terrain(material):
    color = MEL.create_material_expression(material, unreal.MaterialExpressionVertexColor, -400, 0)
    MEL.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = MEL.create_material_expression(material, unreal.MaterialExpressionConstant, -400, 200)
    rough.set_editor_property("r", 0.95)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)


def water(material):
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    material.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE)
    color = MEL.create_material_expression(material, unreal.MaterialExpressionConstant3Vector, -400, 0)
    color.set_editor_property("constant", unreal.LinearColor(0.05, 0.22, 0.32, 1.0))
    MEL.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    opacity = MEL.create_material_expression(material, unreal.MaterialExpressionConstant, -400, 200)
    opacity.set_editor_property("r", 0.62)
    MEL.connect_material_property(opacity, "", unreal.MaterialProperty.MP_OPACITY)
    rough = MEL.create_material_expression(material, unreal.MaterialExpressionConstant, -400, 300)
    rough.set_editor_property("r", 0.08)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)


master = make_material("M_VoxelProp", voxel_prop)
make_material("M_TerrainVertexColor", terrain)
make_material("M_Water", water)

# ---- atlas de cada espécie e a instância do material na malha -------------------------------
for name in names:
    mesh_path = next((p for p in unreal.EditorAssetLibrary.list_assets(f"{DEST}/{name}", recursive=True)
                      if unreal.EditorAssetLibrary.find_asset_data(p).asset_class_path.asset_name == "StaticMesh"), None)
    if not mesh_path:
        continue
    mesh = unreal.EditorAssetLibrary.load_asset(mesh_path)
    mi_path = f"{DEST}/{name}/MI_{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(mi_path):
        continue
    texture = None
    atlas_src = os.path.join(GODOT_ART, name + "_atlas.png")
    if os.path.exists(atlas_src):
        task = unreal.AssetImportTask()
        task.filename = atlas_src
        task.destination_path = f"{DEST}/{name}"
        task.automated = True
        task.save = True
        tools.import_asset_tasks([task])
        texture = unreal.EditorAssetLibrary.load_asset(f"{DEST}/{name}/{name}_atlas")
        if texture:
            texture.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
            texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
            unreal.EditorAssetLibrary.save_loaded_asset(texture)
    mi = tools.create_asset(f"MI_{name}", f"{DEST}/{name}", unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mi, master)
    if texture:
        MEL.set_material_instance_texture_parameter_value(mi, "Atlas", texture)
    unreal.EditorAssetLibrary.save_loaded_asset(mi)
    for i in range(mesh.get_num_sections(0)):
        mesh.set_material(i, mi)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
unreal.log("VORTEX_PROPS_MATERIALS_DONE")
