"""As animações do Mixamo de cada herói (Tools/Blender/MixamoToHero.py) na Unreal — editor fechado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportHeroAnims.py -unattended -nullrhi

Data/Heroes/<raça>_Anims/Hero_<raça>_Anims.glb -> /Game/Vortex/Characters/Heroes/Anims/<raça>/MX_<raça>_<clipe>, no
esqueleto do aventureiro (o mesmo dos heróis). UVortexAnimInstance::Clip usa o clipe da raça quando existe.
A taxa (RateScale) dos clipes que andam é a velocidade base do jogo / a velocidade do clipe (clips.json), dentro de
[0,8; 1,6]: o pé fica perto do chão sem o passo virar correria. Relatório em Saved/Logs/ImportHeroAnims.txt.
"""
import glob
import json
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
SKELETON = "/Game/Vortex/Characters/Adventurer/Adventurer_Skinned/SkeletalMeshes/Adventurer_Skinned_Skeleton"
# a velocidade (cm/s) que o jogo usa como base de cada clipe (UVortexAnimInstance: taxa = velocidade / base)
BASE_SPEED = {"Walk": 150.0, "Jog": 300.0, "Run": 460.0, "Sprint": 620.0, "Swim": 260.0}
skeleton = library.load_asset(SKELETON)
lines = []

for path in sorted(glob.glob(os.path.join(root, "Data", "Heroes", "*_Anims", "Hero_*_Anims.glb"))):
    race = os.path.basename(path)[len("Hero_"):-len("_Anims.glb")]
    # 31/10: quem tem o herói do Mixamo como veio usa os clipes no esqueleto próprio (Tools/ImportMixamoHeroes.py)
    if os.path.isdir(os.path.join(root, "Data", "Heroes", race + "_Mixamo")):
        continue
    clips = json.load(open(os.path.join(os.path.dirname(path), "clips.json"), encoding="utf-8"))
    folder = "/Game/Vortex/Characters/Heroes/Anims/" + race
    staging = folder + "/_import"
    if library.does_directory_exist(staging):
        library.delete_directory(staging)
    pipeline = unreal.InterchangeGenericAssetsPipeline()
    pipeline.set_editor_property("scene_name_sub_folder", False)
    pipeline.set_editor_property("asset_type_sub_folders", False)
    common = pipeline.get_editor_property("common_skeletal_meshes_and_animations_properties")
    common.set_editor_property("skeleton", skeleton)
    common.set_editor_property("try_auto_select_skeleton", False)
    common.set_editor_property("import_only_animations", True)
    pipeline.get_editor_property("mesh_pipeline").set_editor_property("create_physics_asset", False)
    pipeline.get_editor_property("animation_pipeline").set_editor_property("import_animations", True)
    pipeline.get_editor_property("material_pipeline").set_editor_property("import_materials", False)
    override = unreal.InterchangePipelineStackOverride()
    override.add_pipeline(pipeline)
    task = unreal.AssetImportTask()
    task.filename = path
    task.destination_path = staging
    task.automated = True
    task.save = False
    task.replace_existing = True
    task.options = override
    tools.import_asset_tasks([task])
    count = 0
    for p in library.list_assets(staging, recursive=True):
        asset = library.load_asset(p)
        if not isinstance(asset, unreal.AnimSequence):
            continue
        # o nome que o glTF dá: o da faixa, às vezes com o do arquivo na frente
        name = next((c for c in sorted(clips, key=len, reverse=True) if asset.get_name().endswith(c)), None)
        if not name:
            lines.append("%s: sem clipe para %s" % (race, asset.get_name()))
            continue
        info = clips[name]
        rate = 1.0
        if name in BASE_SPEED and info["speed"] > 10:
            rate = min(1.6, max(.8, BASE_SPEED[name] / info["speed"]))
        asset.set_editor_property("rate_scale", rate)
        asset.set_editor_property("enable_root_motion", False)
        final = "%s/MX_%s_%s" % (folder, race, name)
        if library.does_asset_exist(final):
            library.delete_asset(final)
        library.rename_asset(p.split(".")[0], final)
        moved = library.load_asset(final)
        if moved:
            library.save_loaded_asset(moved)
            count += 1
        lines.append("  %s %-18s %5.1f s  %5.0f cm/s  taxa %.2f" % (race, name, info["seconds"], info["speed"], rate))
    library.delete_directory(staging)
    lines.append("%s: %d de %d clipes em %s" % (race, count, len(clips), folder))

os.makedirs(os.path.join(root, "Saved", "Logs"), exist_ok=True)
with open(os.path.join(root, "Saved", "Logs", "ImportHeroAnims.txt"), "w", encoding="utf-8") as out:
    out.write("\n".join(lines) + "\n")
unreal.log_warning("VORTEX_HERO_ANIMS " + " | ".join(l for l in lines if not l.startswith("  ")))
