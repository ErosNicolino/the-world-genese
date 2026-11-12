"""Import the Vortex-owned converted wolf, its eight clips and its color palette into Unreal 5.8.

Run with the editor closed:
UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=Tools/ImportWolf.py -unattended

As cores de vértice deste modelo chegavam à Unreal como um cinza só. ConvertGodotWolf.py agora
grava Wolf_Palette.png e um UV por vértice apontando para a cor do voxel; aqui a paleta vira
T_WolfPalette (sem filtro, sem mipmaps, sem compressão) e os materiais a leem:
- M_WolfFur_Palette: cor da paleta, rugosidade 0,9 (StandardMaterial3D do corpo no Godot);
- M_WolfEyes_Palette: cor da paleta e brilho ciano #7ff5ff (o "Eyes" do Godot brilha 1,6).
A malha é reimportada para ganhar o UV; o esqueleto e os oito clipes são substituídos no lugar.
"""
import os
import unreal

EYE_GLOW = 6.0  # a exposição da Unreal pede mais que o 1,6 do Godot para o mesmo brilho na tela

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
source = os.path.join(root, "Data", "GodotWolf", "Wolf_Skinned.glb")
palette_source = os.path.join(root, "Data", "GodotWolf", "Wolf_Palette.png")
destination = "/Game/Vortex/Creatures/Wolf"
tools = unreal.AssetToolsHelpers.get_asset_tools()
mesh_path = destination + "/Wolf_Skinned/SkeletalMeshes/Wolf_Skinned"
library = unreal.EditorAssetLibrary

# malha + clipes (substitui no lugar: o C++ e os clipes continuam no mesmo caminho)
task = unreal.AssetImportTask()
task.filename = source
task.destination_path = destination
task.automated = True
task.save = True
task.replace_existing = True
tools.import_asset_tasks([task])

# paleta
texture_task = unreal.AssetImportTask()
texture_task.filename = palette_source
texture_task.destination_path = destination
texture_task.destination_name = "T_WolfPalette"
texture_task.automated = True
texture_task.save = False
texture_task.replace_existing = True
tools.import_asset_tasks([texture_task])
palette = library.load_asset(destination + "/T_WolfPalette")
if not palette:
    raise RuntimeError("Wolf palette import failed")
palette.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
palette.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
palette.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
palette.set_editor_property("srgb", True)
palette.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_PIXELS2D)
palette.set_editor_property("never_stream", True)
library.save_loaded_asset(palette)

mel = unreal.MaterialEditingLibrary


def palette_material(name, glow):
    path = destination + "/" + name
    material = library.load_asset(path) if library.does_asset_exist(path) else \
        tools.create_asset(name, destination, unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(material)
    sample = mel.create_material_expression(material, unreal.MaterialExpressionTextureSample, -500, 0)
    sample.set_editor_property("texture", palette)
    sample.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    mel.connect_material_property(sample, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    roughness = mel.create_material_expression(material, unreal.MaterialExpressionConstant, -300, 200)
    roughness.set_editor_property("r", 0.5 if glow else 0.9)
    mel.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    if glow:
        tint = mel.create_material_expression(material, unreal.MaterialExpressionConstant3Vector, -500, 300)
        # #7ff5ff em linear
        tint.set_editor_property("constant", unreal.LinearColor(0.212, 0.913, 1.0, 1.0))
        strength = mel.create_material_expression(material, unreal.MaterialExpressionMultiply, -250, 300)
        strength.set_editor_property("const_b", EYE_GLOW)
        mel.connect_material_expressions(tint, "", strength, "A")
        mel.connect_material_property(strength, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    material.set_editor_property("used_with_skeletal_mesh", True)
    mel.recompile_material(material)
    library.save_loaded_asset(material)
    return material


wolf_materials = (palette_material("M_WolfFur_Palette", False), palette_material("M_WolfEyes_Palette", True))

mesh = library.load_asset(mesh_path)
if not mesh:
    raise RuntimeError("Wolf skeletal mesh import failed")
materials = mesh.get_editor_property("materials")
if len(materials) != 2:
    raise RuntimeError("Wolf needs separate fur and eye material slots: " + str(len(materials)))
replacements = []
for index, material in enumerate(wolf_materials):
    replacement = unreal.SkeletalMaterial()
    replacement.set_editor_property("material_interface", material)
    replacement.set_editor_property("material_slot_name", materials[index].get_editor_property("material_slot_name"))
    replacements.append(replacement)
mesh.modify()
mesh.set_editor_property("materials", replacements)
library.save_loaded_asset(mesh)
for index, slot in enumerate(mesh.get_editor_property("materials")):
    actual = slot.get_editor_property("material_interface")
    if not actual or actual != wolf_materials[index]:
        raise RuntimeError(f"Wolf material {index} not assigned: {actual}")

assets = library.list_assets(destination, recursive=True)
clips = [path for path in assets if str(library.find_asset_data(path).asset_class_path.asset_name) == "AnimSequence"]
unreal.log(f"VORTEX_WOLF_IMPORT mesh={mesh_path} palette=T_WolfPalette clips={len(clips)}")
if len(clips) != 8:
    raise RuntimeError("Expected eight wolf animations, found " + str(len(clips)))
