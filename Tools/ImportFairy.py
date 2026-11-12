"""As fadas (10/11) na Unreal, editor fechado:
UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportFairy.py -unattended -nullrhi

Data/Creatures/Fada (Tools/Blender/MakeFairy.py -- <v> jogo): SK_Fada_0..5.glb (um esqueleto só: Fada_Esqueleto), as
paletas T_Fada_<v>_<cor>.png (pixel nítido) e T_FadaAsa.png (as asas, lisa) -> /Game/Vortex/Creatures/Fada.
Materiais: M_FadaCorpo (a paleta, parâmetro Base) e M_FadaAsa (translúcida, dos dois lados, um pouco acesa).
O AVortexFairies (VortexWorld) escolhe a malha e a paleta de cada fada. Relatório em Saved/Logs/ImportFairy.txt.
"""
import glob
import os

import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
SOURCE = os.path.join(root, "Data", "Creatures", "Fada")
DEST = "/Game/Vortex/Creatures/Fada"
lines = []


def bring_texture(file, crisp):
    name = os.path.splitext(os.path.basename(file))[0]
    t = unreal.AssetImportTask()
    t.filename = file
    t.destination_path = DEST
    t.destination_name = name
    t.automated = True
    t.save = False
    t.replace_existing = True
    tools.import_asset_tasks([t])
    tex = library.load_asset(DEST + "/" + name)
    tex.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST if crisp else unreal.TextureFilter.TF_DEFAULT)
    if crisp:
        tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    tex.set_editor_property("never_stream", True)
    library.save_loaded_asset(tex)
    return tex


def new_material(name):
    path = DEST + "/" + name
    m = library.load_asset(path) if library.does_asset_exist(path) else tools.create_asset(name, DEST, unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(m)
    return m


def node(m, cls, x, y, **props):
    n = mel.create_material_expression(m, cls, x, y)
    for k, v in props.items():
        n.set_editor_property(k, v)
    return n


palettes = [bring_texture(f, True) for f in sorted(glob.glob(os.path.join(SOURCE, "T_Fada_*_*.png")))]
face_tex = bring_texture(os.path.join(SOURCE, "T_FadaRosto.png"), True)
wing_tex = bring_texture(os.path.join(SOURCE, "T_FadaAsa.png"), False)
lines.append("paletas: %d" % len(palettes))

# o corpo: a cor da paleta, fosca
body = new_material("M_FadaCorpo")
base = node(body, unreal.MaterialExpressionTextureSampleParameter2D, -500, 0, parameter_name="Base", texture=palettes[0])
mel.connect_material_property(base, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
# um brilho próprio leve (criatura mágica): de lado e de costas, contra o sol, ela ficava preta
self_glow = node(body, unreal.MaterialExpressionScalarParameter, -500, 300, parameter_name="BrilhoProprio", default_value=.7)
glow_mul = node(body, unreal.MaterialExpressionMultiply, -300, 300)
mel.connect_material_expressions(base, "RGB", glow_mul, "A")
mel.connect_material_expressions(self_glow, "", glow_mul, "B")
mel.connect_material_property(glow_mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
rough = node(body, unreal.MaterialExpressionConstant, -300, 200, r=.8)
mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
body.set_editor_property("used_with_skeletal_mesh", True)
mel.recompile_material(body)
library.save_loaded_asset(body)

# as asas: translúcidas, dos dois lados, a cor um pouco acesa (brilham contra a mata) e o canal alfa da textura
wing = new_material("M_FadaAsa")
wing.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
wing.set_editor_property("two_sided", True)
wing.set_editor_property("used_with_skeletal_mesh", True)
wt = node(wing, unreal.MaterialExpressionTextureSampleParameter2D, -600, 0, parameter_name="Asa", texture=wing_tex)
mel.connect_material_property(wt, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(wt, "A", unreal.MaterialProperty.MP_OPACITY)
glow = node(wing, unreal.MaterialExpressionScalarParameter, -600, 250, parameter_name="Brilho", default_value=.8)
lit = node(wing, unreal.MaterialExpressionMultiply, -350, 200)
mel.connect_material_expressions(wt, "RGB", lit, "A")
mel.connect_material_expressions(glow, "", lit, "B")
mel.connect_material_property(lit, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.recompile_material(wing)
library.save_loaded_asset(wing)

# o rosto (10/11): 4 quadros lado a lado; Quadro (0 normal, 1 piscando, 2 feliz, 3 assustada) escolhe qual aparece
face = new_material("M_FadaRosto")
uv = node(face, unreal.MaterialExpressionTextureCoordinate, -900, 0, u_tiling=.25, v_tiling=1.0)
frame = node(face, unreal.MaterialExpressionScalarParameter, -900, 200, parameter_name="Quadro", default_value=0.0)
quarter = node(face, unreal.MaterialExpressionMultiply, -700, 200, const_b=.25)
mel.connect_material_expressions(frame, "", quarter, "A")
zero = node(face, unreal.MaterialExpressionConstant, -700, 320, r=0.0)
shift = node(face, unreal.MaterialExpressionAppendVector, -550, 250)
mel.connect_material_expressions(quarter, "", shift, "A")
mel.connect_material_expressions(zero, "", shift, "B")
moved = node(face, unreal.MaterialExpressionAdd, -400, 100)
mel.connect_material_expressions(uv, "", moved, "A")
mel.connect_material_expressions(shift, "", moved, "B")
ft = node(face, unreal.MaterialExpressionTextureSampleParameter2D, -250, 0, parameter_name="Rosto", texture=face_tex)
mel.connect_material_expressions(moved, "", ft, "UVs")
mel.connect_material_property(ft, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
fpar = node(face, unreal.MaterialExpressionScalarParameter, -250, 300, parameter_name="BrilhoProprio", default_value=.7)
fglow = node(face, unreal.MaterialExpressionMultiply, -50, 200)
mel.connect_material_expressions(ft, "RGB", fglow, "A")
mel.connect_material_expressions(fpar, "", fglow, "B")
mel.connect_material_property(fglow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
froughness = node(face, unreal.MaterialExpressionConstant, -50, 320, r=.8)
mel.connect_material_property(froughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
face.set_editor_property("used_with_skeletal_mesh", True)
mel.recompile_material(face)
library.save_loaded_asset(face)

# as malhas: a primeira cria o esqueleto, as outras usam o mesmo (ao reimportar, o que já existe). Apagar antes, com o
# editor fechado, Content/Vortex/Creatures/Fada/SK_Fada_*.uasset: o delete_asset daqui não solta a malha velha
skeleton = library.load_asset(DEST + "/Fada_Esqueleto") if library.does_asset_exist(DEST + "/Fada_Esqueleto") else None
for glb in sorted(glob.glob(os.path.join(SOURCE, "SK_Fada_*.glb"))):
    name = os.path.splitext(os.path.basename(glb))[0]
    staging = DEST + "/_import_" + name
    if library.does_directory_exist(staging):
        library.delete_directory(staging)
    pipeline = unreal.InterchangeGenericAssetsPipeline()
    pipeline.set_editor_property("scene_name_sub_folder", False)
    pipeline.set_editor_property("asset_type_sub_folders", False)
    common = pipeline.get_editor_property("common_skeletal_meshes_and_animations_properties")
    common.set_editor_property("try_auto_select_skeleton", False)
    if skeleton:
        common.set_editor_property("skeleton", skeleton)
    pipeline.get_editor_property("mesh_pipeline").set_editor_property("create_physics_asset", False)
    pipeline.get_editor_property("animation_pipeline").set_editor_property("import_animations", False)
    pipeline.get_editor_property("material_pipeline").set_editor_property("import_materials", False)
    override = unreal.InterchangePipelineStackOverride()
    override.add_pipeline(pipeline)
    task = unreal.AssetImportTask()
    task.filename = glb
    task.destination_path = staging
    task.automated = True
    task.save = False
    task.replace_existing = True
    task.options = override
    tools.import_asset_tasks([task])
    kind = lambda p: str(library.find_asset_data(p).asset_class_path.asset_name)
    found = library.list_assets(staging, recursive=True)
    mesh_path = next((p for p in found if kind(p) == "SkeletalMesh"), None)
    skel_path = next((p for p in found if kind(p) == "Skeleton"), None)
    if not mesh_path:
        lines.append("FALHOU %s: %s" % (name, found))
        continue
    target = DEST + "/" + name
    if library.does_asset_exist(target):
        library.delete_asset(target)
    library.rename_asset(mesh_path.split(".")[0], target)
    if skel_path and not skeleton:
        library.rename_asset(skel_path.split(".")[0], DEST + "/Fada_Esqueleto")
        skeleton = library.load_asset(DEST + "/Fada_Esqueleto")
    mesh = library.load_asset(target)
    slots = []
    for i, s in enumerate(mesh.get_editor_property("materials")):
        slot = str(s.get_editor_property("material_slot_name")).lower()
        slots.append(unreal.SkeletalMaterial(material_interface=wing if "asa" in slot else face if "rosto" in slot else body, material_slot_name=s.get_editor_property("material_slot_name"),
                                             uv_channel_data=s.get_editor_property("uv_channel_data")))
    mesh.set_editor_property("materials", slots)
    library.save_loaded_asset(mesh)
    if library.does_directory_exist(staging):
        library.delete_directory(staging)
    lines.append("%s: %d materiais, esqueleto %s" % (name, len(slots), mesh.get_editor_property("skeleton").get_name()))
if skeleton:
    library.save_loaded_asset(skeleton)
with open(os.path.join(root, "Saved", "Logs", "ImportFairy.txt"), "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
unreal.log_warning("VORTEX_FAIRY " + " | ".join(lines))
