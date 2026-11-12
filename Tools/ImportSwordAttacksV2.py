"""Os golpes refeitos da espada (Tools/Blender/BuildSwordAttacks.py) no DA_DefaultSword.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportSwordAttacksV2.py -unattended
(também roda no editor aberto, pelo Python)

Importa Data/GodotWeapons/Adventurer_SwordAttacksV2.glb no esqueleto do aventureiro, como o ImportSword.py
faz com o original, e troca os clipes dos três golpes e da defesa. Tempos, janelas de dano e alcance não
mudam. Os clipes do Godot continuam em /Game/Vortex/Weapons/Sword/Adventurer_SwordAttacks (para voltar
atrás, é só apontar os golpes para eles de novo).
"""
import os
import unreal

project = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
destination = "/Game/Vortex/Weapons/Sword"
folder = destination + "/Adventurer_SwordAttacksV2/SkeletalMeshes/Adventurer_SwordAttacksV2"
skeleton = library.load_asset("/Game/Vortex/Characters/Adventurer/Adventurer_Skinned/SkeletalMeshes/Adventurer_Skinned_Skeleton")
if not skeleton:
    raise RuntimeError("Import the original adventurer skeleton first")

pipeline = unreal.InterchangeGenericAssetsPipeline()
pipeline.set_editor_property("scene_name_sub_folder", True)
pipeline.set_editor_property("asset_type_sub_folders", True)
common = pipeline.get_editor_property("common_skeletal_meshes_and_animations_properties")
common.set_editor_property("skeleton", skeleton)
common.set_editor_property("try_auto_select_skeleton", False)
pipeline.get_editor_property("mesh_pipeline").set_editor_property("create_physics_asset", False)
pipeline.get_editor_property("material_pipeline").set_editor_property("import_materials", False)
pipeline.get_editor_property("material_pipeline").get_editor_property("texture_pipeline").set_editor_property("import_textures", False)
override = unreal.InterchangePipelineStackOverride()
override.add_pipeline(pipeline)

task = unreal.AssetImportTask()
task.filename = os.path.join(project, "Data", "GodotWeapons", "Adventurer_SwordAttacksV2.glb")
task.destination_path = destination
task.automated = True
task.save = True
task.replace_existing = True
task.options = override
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

clips = {}
for name in ("Attack_01", "Attack_02", "Attack_03", "Skill_Guard"):
    clip = library.load_asset(folder + name)
    if not clip:
        raise RuntimeError("Missing imported clip: " + folder + name)
    clips[name] = clip

sword = library.load_asset("/Game/Vortex/Weapons/Sword/DA_DefaultSword")
swings = sword.get_editor_property("swings")
for swing, name in zip(swings, ("Attack_01", "Attack_02", "Attack_03")):
    swing.set_editor_property("animation", clips[name])
sword.set_editor_property("swings", swings)
sword.set_editor_property("guard_animation", clips["Skill_Guard"])
library.save_loaded_asset(sword)
unreal.log("VORTEX_SWORD_ATTACKS_V2 " + " ".join("%s=%.2fs" % (n, c.get_editor_property("sequence_length")) for n, c in clips.items()))
