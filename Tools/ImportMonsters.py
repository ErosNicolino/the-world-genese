"""Importa as criaturas convertidas (Tools/ConvertGodotMonsters.py) para a Unreal 5.8.

Rode com o editor fechado:
UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=Tools/ImportMonsters.py -unattended

Para cada espécie de Data/GodotMonsters/monsters.json:
- /Game/Vortex/Creatures/<Espécie>/<Espécie>_Skinned (malha, esqueleto e os clipes <Espécie>_Skinned<Clipe>);
- T_<Espécie>Palette: a paleta das cores dos voxels (sem filtro, sem mipmaps, sem compressão);
- MI_<Espécie>_Body e MI_<Espécie>_Glow: filhas de M_MonsterBody (fosco) e M_MonsterGlow (brilho na
  cor do emissiveFactor do Godot: olhos, cristais, runas).
Um relatório fica em Saved/Logs/ImportMonsters.txt.
"""
import json
import os
import unreal

GLOW_STRENGTH = 6.0   # como o olho do lobo: a exposição da Unreal pede mais que o Godot

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
data = os.path.join(root, "Data", "GodotMonsters")
catalog = json.load(open(os.path.join(data, "monsters.json"), encoding="utf-8"))
tools = unreal.AssetToolsHelpers.get_asset_tools()
library = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
shared = "/Game/Vortex/Creatures/Shared"


def parent_material(name, glow):
    path = shared + "/" + name
    material = library.load_asset(path) if library.does_asset_exist(path) else \
        tools.create_asset(name, shared, unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(material)
    sample = mel.create_material_expression(material, unreal.MaterialExpressionTextureSampleParameter2D, -600, 0)
    sample.set_editor_property("parameter_name", "Palette")
    sample.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    mel.connect_material_property(sample, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    roughness = mel.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -300, 200)
    roughness.set_editor_property("parameter_name", "Roughness")
    roughness.set_editor_property("default_value", 0.5 if glow else 0.9)
    mel.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    if glow:
        tint = mel.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -600, 320)
        tint.set_editor_property("parameter_name", "Glow")
        tint.set_editor_property("default_value", unreal.LinearColor(0.2, 1.0, 0.7, 1.0))
        lit = mel.create_material_expression(material, unreal.MaterialExpressionMultiply, -350, 320)
        mel.connect_material_expressions(sample, "RGB", lit, "A")
        mel.connect_material_expressions(tint, "", lit, "B")
        strength = mel.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -350, 480)
        strength.set_editor_property("parameter_name", "Strength")
        strength.set_editor_property("default_value", GLOW_STRENGTH)
        glow_out = mel.create_material_expression(material, unreal.MaterialExpressionMultiply, -150, 360)
        mel.connect_material_expressions(lit, "", glow_out, "A")
        mel.connect_material_expressions(strength, "", glow_out, "B")
        mel.connect_material_property(glow_out, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    material.set_editor_property("used_with_skeletal_mesh", True)
    mel.recompile_material(material)
    library.save_loaded_asset(material)
    return material


def instance(name, folder, parent, palette, glow=None):
    path = folder + "/" + name
    mi = library.load_asset(path) if library.does_asset_exist(path) else \
        tools.create_asset(name, folder, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, parent)
    mel.set_material_instance_texture_parameter_value(mi, "Palette", palette)
    if glow is not None:
        # o emissiveFactor do Godot é linear; um pouco de branco para não sumir em cores escuras
        mel.set_material_instance_vector_parameter_value(mi, "Glow", unreal.LinearColor(glow[0], glow[1], glow[2], 1.0))
    library.save_loaded_asset(mi)
    return mi


body_parent = parent_material("M_MonsterBody", False)
glow_parent = parent_material("M_MonsterGlow", True)
lines = []
only = [x for x in os.environ.get("VORTEX_SPECIES", "").split(",") if x]   # só algumas: VORTEX_SPECIES=Golem,Bat
# quem tem import próprio não volta para o modelo do Godot: o Azhûr (Tools/ImportAzhur.py) e o cavalo novo do Meshy
# (Tools/ImportHorse.py, as cinco pelagens)
OWN_IMPORT = {"RuneColossus", "Horse_Baio", "Horse_Malhado", "Horse_Negro", "Horse_Palomino", "Horse_Tordilho",
              "Golem", "RootTitan"}   # golem e Thorun do Meshy: Tools/ImportMeshyMonsters.py
for species, info in catalog.items():
    if (only and species not in only) or species in OWN_IMPORT:
        continue
    folder = "/Game/Vortex/Creatures/" + species
    name = species + "_Skinned"
    task = unreal.AssetImportTask()
    task.filename = os.path.join(data, species, name + ".glb")
    task.destination_path = folder
    task.automated = True
    task.save = True
    task.replace_existing = True
    tools.import_asset_tasks([task])

    texture_task = unreal.AssetImportTask()
    texture_task.filename = os.path.join(data, species, species + "_Palette.png")
    texture_task.destination_path = folder
    texture_task.destination_name = "T_" + species + "Palette"
    texture_task.automated = True
    texture_task.save = False
    texture_task.replace_existing = True
    tools.import_asset_tasks([texture_task])
    palette = library.load_asset(folder + "/T_" + species + "Palette")
    if not palette:
        raise RuntimeError("paleta de " + species)
    palette.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
    palette.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    palette.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    palette.set_editor_property("srgb", True)
    palette.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_PIXELS2D)
    palette.set_editor_property("never_stream", True)
    library.save_loaded_asset(palette)

    wanted = [instance("MI_" + species + "_Body", folder, body_parent, palette)]
    if info.get("materials", 1) > 1:
        wanted.append(instance("MI_" + species + "_Glow", folder, glow_parent, palette, info.get("glow", [0, 0, 0])))

    mesh = None
    for path in library.list_assets(folder, recursive=True):
        data_asset = library.find_asset_data(path)
        if str(data_asset.asset_class_path.asset_name) == "SkeletalMesh":
            mesh = library.load_asset(path)
    if not mesh:
        raise RuntimeError("malha de " + species)
    slots = mesh.get_editor_property("materials")
    replacements = []
    for index, slot in enumerate(slots):
        replacement = unreal.SkeletalMaterial()
        replacement.set_editor_property("material_interface", wanted[min(index, len(wanted) - 1)])
        replacement.set_editor_property("material_slot_name", slot.get_editor_property("material_slot_name"))
        replacements.append(replacement)
    mesh.modify()
    mesh.set_editor_property("materials", replacements)
    library.save_loaded_asset(mesh)

    clips = [path for path in library.list_assets(folder, recursive=True)
             if str(library.find_asset_data(path).asset_class_path.asset_name) == "AnimSequence"]
    lines.append("%s malha=%s slots=%d clipes=%d esperados=%d" % (species, mesh.get_path_name(), len(slots), len(clips), len(info["clips"])))
    unreal.log("VORTEX_MONSTER_IMPORT " + lines[-1])

with open(os.path.join(root, "Saved", "Logs", "ImportMonsters.txt"), "w", encoding="utf-8") as out:
    out.write("\n".join(lines) + "\n")
