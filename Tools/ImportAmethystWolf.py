"""Importa o lobo ametista (Data/Modelos/Lobo/Lobo_Ametista.fbx, feito por BuildAmethystWolf.py)
com as dez animações, as quatro texturas PBR do Meshy e o material M_LoboAmetista.

Com o editor fechado:
UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportAmethystWolf.py -unattended
"""
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
src = os.path.join(root, "Data", "Modelos", "Lobo")
dest = "/Game/Vortex/Creatures/Wolf/Ametista"
tools = unreal.AssetToolsHelpers.get_asset_tools()
library = unreal.EditorAssetLibrary
# 10/11: Bite2, Rear e IdleSniff (Tools/Blender/RetargetWolf.py, os especiais); IdleSniff antes de Idle (o nome termina igual)
CLIPS = ["IdleSniff", "Idle", "Walk", "Run", "StrafeL", "StrafeR", "Howl", "Attack", "Pounce", "Hit", "Death", "Bite2", "Rear"]

if library.does_directory_exist(dest):
    library.delete_directory(dest)

task = unreal.AssetImportTask()
task.filename = os.path.join(src, "Lobo_Ametista.fbx")
task.destination_path = dest
task.automated = True
task.save = True
task.replace_existing = True
tools.import_asset_tasks([task])


def texture(file, name, normal=False, linear=False):
    t = unreal.AssetImportTask()
    t.filename = os.path.join(src, file)
    t.destination_path = dest
    t.destination_name = name
    t.automated = True
    t.replace_existing = True
    tools.import_asset_tasks([t])
    tex = library.load_asset(dest + "/" + name)
    if not tex:
        raise RuntimeError("textura nao importou: " + file)
    if normal:
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        tex.set_editor_property("srgb", False)
    elif linear:
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
        tex.set_editor_property("srgb", False)
    # 4096 do Meshy: 2048 na tela basta para um bicho de 2,3 m
    tex.set_editor_property("max_texture_size", 2048)
    library.save_loaded_asset(tex)
    return tex


color = texture("Lobo_texture_0.png", "T_LoboAmetista_Cor")
normal = texture("Lobo_texture_0_normal.png", "T_LoboAmetista_Normal", normal=True)
rough = texture("Lobo_texture_0_roughness.png", "T_LoboAmetista_Rugosidade", linear=True)
metal = texture("Lobo_texture_0_metallic.png", "T_LoboAmetista_Metal", linear=True)

mel = unreal.MaterialEditingLibrary
material = tools.create_asset("M_LoboAmetista", dest, unreal.Material, unreal.MaterialFactoryNew())


def sample(tex, kind, y):
    node = mel.create_material_expression(material, unreal.MaterialExpressionTextureSample, -500, y)
    node.set_editor_property("texture", tex)
    node.set_editor_property("sampler_type", kind)
    return node


mel.connect_material_property(sample(color, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, -300), "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(sample(normal, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, 0), "RGB", unreal.MaterialProperty.MP_NORMAL)
mel.connect_material_property(sample(rough, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS, 300), "R", unreal.MaterialProperty.MP_ROUGHNESS)
mel.connect_material_property(sample(metal, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS, 600), "R", unreal.MaterialProperty.MP_METALLIC)
material.set_editor_property("used_with_skeletal_mesh", True)
mel.recompile_material(material)
library.save_loaded_asset(material)

meshes, clips, extra = [], {}, []
for path in library.list_assets(dest, recursive=True, include_folder=False):
    data = library.find_asset_data(path)
    cls = str(data.asset_class_path.asset_name)
    name = str(data.asset_name)
    if cls == "SkeletalMesh":
        meshes.append(path)
    elif cls == "AnimSequence":
        clips[name] = path
    elif cls in ("Material", "MaterialInstanceConstant", "Texture2D") and not name.startswith(("M_LoboAmetista", "T_LoboAmetista")):
        extra.append(path)
unreal.log("VORTEX_AMETHYST_ASSETS meshes=%s clips=%s extra=%s" % (meshes, sorted(clips), extra))
if len(meshes) != 1:
    raise RuntimeError("esperava uma malha esqueletal: " + str(meshes))

# nomes curtos e fixos para o C++: SK_LoboAmetista e A_LoboAmetista_<Clipe>
mesh_path = dest + "/SK_LoboAmetista"
if meshes[0].split(".")[0] != mesh_path:
    library.rename_asset(meshes[0].split(".")[0], mesh_path)
found = []
for name, path in clips.items():
    clip = next((c for c in CLIPS if name.endswith(c)), None)
    if clip is None:
        library.delete_asset(path.split(".")[0])
        continue
    target = dest + "/A_LoboAmetista_" + clip
    if path.split(".")[0] != target:
        library.rename_asset(path.split(".")[0], target)
    found.append(clip)
missing = [c for c in CLIPS if c not in found]
for path in extra:
    library.delete_asset(path.split(".")[0])

mesh = library.load_asset(mesh_path)
slots = mesh.get_editor_property("materials")
replaced = []
for slot in slots:
    s = unreal.SkeletalMaterial()
    s.set_editor_property("material_interface", material)
    s.set_editor_property("material_slot_name", slot.get_editor_property("material_slot_name"))
    replaced.append(s)
mesh.modify()
mesh.set_editor_property("materials", replaced)
library.save_loaded_asset(mesh)
box = mesh.get_bounds().box_extent
unreal.log("VORTEX_AMETHYST_WOLF mesh=%s slots=%d extent=(%.0f, %.0f, %.0f) clips=%d missing=%s" % (
    mesh_path, len(replaced), box.x, box.y, box.z, len(found), missing))
for clip in found:
    seq = library.load_asset(dest + "/A_LoboAmetista_" + clip)
    unreal.log("VORTEX_AMETHYST_CLIP %s %.2fs" % (clip, seq.get_play_length()))
library.save_directory(dest)
if missing:
    raise RuntimeError("faltam clipes: " + str(missing))
