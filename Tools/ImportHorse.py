"""O cavalo novo (Meshy, com as animações da Quaternius: Tools/Blender/QuadrupedTransfer.py horse_meshy) nas cinco
pelagens do jogo — editor fechado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportHorse.py -unattended -nullrhi

- o mesmo Data/Modelos/Cavalo/Horse_Skinned.glb vira /Game/Vortex/Creatures/Horse_<pelagem>/Horse_<pelagem>_Skinned
  (malha e os clipes Horse_<pelagem>_Skinned<Clipe>, os caminhos que AVortexMonster procura);
- M_HorseCoat: a textura do cavalo, desbotada (Desat) e tingida (Tint) — MI_Horse_<pelagem> em cada uma;
- antes de rodar, a pasta antiga de cada pelagem sai de Content (o import refaz tudo).
Relatório em Saved/Logs/ImportHorse.txt.
"""
import os
import shutil
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
SRC = os.path.join(root, "Data", "Modelos", "Cavalo", "Horse_Skinned.glb")
COPIES = os.path.join(root, "Data", "Modelos", "Cavalo", "pelagens")
SHARED = "/Game/Vortex/Creatures/Shared"
# (desbotar 0..1, tinta): baio é a cor do modelo
COATS = {"Baio": (0.0, (1.0, 1.0, 1.0)), "Negro": (.85, (.2, .18, .18)), "Tordilho": (1.0, (.9, .9, .93)),
         "Palomino": (.55, (1.3, 1.0, .55)), "Malhado": (.35, (1.05, .9, .78))}
lines = []


def node(m, kind, x, y, **props):
    e = mel.create_material_expression(m, kind, x, y)
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e


def link(a, b, pin_b="", pin_a=""):
    mel.connect_material_expressions(a, pin_a, b, pin_b)


os.makedirs(COPIES, exist_ok=True)
material = None
for coat, (desat, tint) in COATS.items():
    name = "Horse_%s_Skinned" % coat
    copy = os.path.join(COPIES, name + ".glb")
    shutil.copyfile(SRC, copy)
    folder = "/Game/Vortex/Creatures/Horse_" + coat
    task = unreal.AssetImportTask()
    task.filename = copy
    task.destination_path = folder
    task.automated = True
    task.save = True
    task.replace_existing = True
    tools.import_asset_tasks([task])
    meshes = [p for p in library.list_assets(folder, recursive=True) if str(library.find_asset_data(p).asset_class_path.asset_name) == "SkeletalMesh"]
    textures = [p for p in library.list_assets(folder, recursive=True) if str(library.find_asset_data(p).asset_class_path.asset_name) == "Texture2D"]
    color = next((library.load_asset(p) for p in textures if library.load_asset(p).get_editor_property("srgb")), None)
    if material is None and color:
        path = SHARED + "/M_HorseCoat"
        material = library.load_asset(path) if library.does_asset_exist(path) else tools.create_asset("M_HorseCoat", SHARED, unreal.Material, unreal.MaterialFactoryNew())
        mel.delete_all_material_expressions(material)
        base = node(material, unreal.MaterialExpressionTextureSampleParameter2D, -900, 0, parameter_name="Base", texture=color)
        gray = node(material, unreal.MaterialExpressionDesaturation, -650, 0)
        link(base, gray, "", "RGB")
        amount = node(material, unreal.MaterialExpressionScalarParameter, -900, 200, parameter_name="Desat", default_value=0.0)
        link(amount, gray, "Fraction")
        tinted = node(material, unreal.MaterialExpressionMultiply, -450, 40)
        link(gray, tinted, "A")
        link(node(material, unreal.MaterialExpressionVectorParameter, -650, 220, parameter_name="Tint", default_value=unreal.LinearColor(1, 1, 1, 1)), tinted, "B")
        mel.connect_material_property(tinted, "", unreal.MaterialProperty.MP_BASE_COLOR)
        mel.connect_material_property(node(material, unreal.MaterialExpressionConstant, -450, 220, r=.8), "", unreal.MaterialProperty.MP_ROUGHNESS)
        material.set_editor_property("used_with_skeletal_mesh", True)
        mel.recompile_material(material)
        library.save_loaded_asset(material)
    mi = None
    if material:
        mi_path = folder + "/MI_Horse_%s_Coat" % coat
        mi = library.load_asset(mi_path) if library.does_asset_exist(mi_path) else \
            tools.create_asset("MI_Horse_%s_Coat" % coat, folder, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(mi, material)
        if color:
            mel.set_material_instance_texture_parameter_value(mi, "Base", color)
        mel.set_material_instance_scalar_parameter_value(mi, "Desat", desat)
        mel.set_material_instance_vector_parameter_value(mi, "Tint", unreal.LinearColor(tint[0], tint[1], tint[2], 1))
        library.save_loaded_asset(mi)
    for p in meshes:
        mesh = library.load_asset(p)
        slots = []
        for slot in mesh.get_editor_property("materials"):
            s = unreal.SkeletalMaterial()
            s.set_editor_property("material_interface", mi or slot.get_editor_property("material_interface"))
            s.set_editor_property("material_slot_name", slot.get_editor_property("material_slot_name"))
            slots.append(s)
        mesh.modify()
        mesh.set_editor_property("materials", slots)
        library.save_loaded_asset(mesh)
    clips = sorted(p.split("/")[-1].split(".")[0] for p in library.list_assets(folder, recursive=True)
                   if str(library.find_asset_data(p).asset_class_path.asset_name) == "AnimSequence")
    lines.append("%s: malha %s, textura %s, clipes %s" % (coat, meshes[0].split("/")[-1] if meshes else "NENHUMA", color.get_name() if color else "-", clips))

with open(os.path.join(root, "Saved", "Logs", "ImportHorse.txt"), "w", encoding="utf-8") as out:
    out.write("\n".join(lines) + "\n")
