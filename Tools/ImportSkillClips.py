"""Os gestos das habilidades de classe do Godot (gritar, rezar, arremessar, pisar, invocar, guardar, apontar,
conjurar) no esqueleto do aventureiro — rode com o editor fechado.

Antes: python Tools/ConvertGodotAdventurer.py <Adventurer.glb do Godot> Data/GodotCharacter/default_look.json
       Data/GodotWeapons/Adventurer_SkillClips.glb <Skill_<gesto> e Skill_<gesto>_<família>, ver Docs>
UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportSkillClips.py -unattended

Os 176 clipes vão para /Game/Vortex/Weapons/SkillClips/Adventurer_SkillClips/SkeletalMeshes/Adventurer_SkillClipsSkill_*.
Skill_<gesto> é o gesto de mãos livres (espada, martelo, arco); Skill_<gesto>_<família> é o mesmo gesto com o braço
da arma na guarda dela (cajado, orbe, besta...). UVortexClassSkills::CastAnimation escolhe pelo tipo da arma.
"""
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
skeleton = library.load_asset("/Game/Vortex/Characters/Adventurer/Adventurer_Skinned/SkeletalMeshes/Adventurer_Skinned_Skeleton")

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

FOLDER = "/Game/Vortex/Weapons/SkillClips"
task = unreal.AssetImportTask()
task.filename = os.path.join(root, "Data", "GodotWeapons", "Adventurer_SkillClips.glb")
task.destination_path = FOLDER
task.automated = True
task.save = True
task.replace_existing = True
task.options = override
tools.import_asset_tasks([task])
clips = FOLDER + "/Adventurer_SkillClips/SkeletalMeshes/"

# o corpo que veio junto com os clipes não é usado (só o esqueleto)
for leftover in ("Adventurer_SkillClips", "Adventurer_SkillClips_PhysicsAsset"):
    if library.does_asset_exist(clips + leftover):
        library.delete_asset(clips + leftover)

found = [a for a in library.list_assets(clips, recursive=False) if "Skill_" in a]
lengths = []
for path in found[:8]:
    anim = library.load_asset(path)
    if anim:
        lengths.append("%s=%.2fs" % (path.split("Adventurer_SkillClips")[-1].split(".")[0], anim.get_editor_property("sequence_length")))
line = "VORTEX_SKILL_CLIPS %d clipes | %s" % (len(found), " ".join(lengths))
with open(os.path.join(root, "Saved", "Logs", "ImportSkillClips.txt"), "w", encoding="utf-8") as out:
    out.write(line + "\n")
unreal.log_warning(line)
