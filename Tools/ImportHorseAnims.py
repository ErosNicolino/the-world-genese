"""Os clipes novos do cavalo (16/11: Brake, a freada) nas cinco pelagens: editor fechado, depois do
Tools/Blender/MakeHorseAnims.py.
UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportHorseAnims.py -unattended -nullrhi

Data/Modelos/Cavalo/Horse_Anims.glb -> /Game/Vortex/Creatures/Horse_<pelagem>/Horse_<pelagem>_Skinned/SkeletalMeshes/
Horse_<pelagem>_Skinned<Clipe>, no esqueleto de cada pelagem (o caminho que o UVortexMonsterAnimInstance procura). Só as
animações: a malha e os clipes do Tools/ImportHorse.py ficam como estão. Relatório em Saved/Logs/ImportHorseAnims.txt.
"""
import os
import sys

import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
SRC = os.path.join(root, "Data", "Modelos", "Cavalo", "Horse_Anims.glb")
sys.path.insert(0, os.path.join(root, "Tools"))
import glb_motion  # noqa: E402
# 16/11: clipe em que nenhum osso se mexe não entra
STILL = glb_motion.still(SRC)
CLIPS = [c for c in ["Brake", "Jump"] if c not in STILL]
lines = ["RECUSADOS (parados, exportar de novo): " + ", ".join(STILL)] if STILL else []
for coat in ("Baio", "Negro", "Tordilho", "Palomino", "Malhado"):
    name = "Horse_%s_Skinned" % coat
    folder = "/Game/Vortex/Creatures/Horse_%s/%s/SkeletalMeshes" % (coat, name)
    staging = "/Game/Vortex/Creatures/Horse_%s/_clipes" % coat
    skeleton = library.load_asset("%s/%s_Skeleton" % (folder, name))
    if not skeleton:
        lines.append("%s: SEM ESQUELETO" % coat)
        continue
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
    task.filename = SRC
    task.destination_path = staging
    task.automated = True
    task.save = False
    task.replace_existing = True
    task.options = override
    tools.import_asset_tasks([task])
    done = []
    for p in library.list_assets(staging, recursive=True):
        if str(library.find_asset_data(p).asset_class_path.asset_name) != "AnimSequence":
            continue
        asset = library.load_asset(p)
        # com um clipe só no arquivo, a animação nasce com o nome do arquivo (Horse_Anims), não com o do clipe
        clip = next((c for c in CLIPS if asset.get_name().endswith(c)), CLIPS[0] if len(CLIPS) == 1 else None)
        if not clip:
            continue
        asset.set_editor_property("enable_root_motion", False)
        final = "%s/%s%s" % (folder, name, clip)
        if library.does_asset_exist(final):
            library.delete_asset(final)
        library.rename_asset(p.split(".")[0], final)
        moved = library.load_asset(final)
        if moved:
            library.save_loaded_asset(moved)
            done.append("%s (%.2f s, %s)" % (clip, moved.get_play_length(), moved.get_editor_property("skeleton").get_name()))
    library.delete_directory(staging)
    lines.append("%s: %s" % (coat, ", ".join(done) or "NADA"))
open(os.path.join(root, "Saved", "Logs", "ImportHorseAnims.txt"), "w", encoding="utf-8").write("\n".join(lines) + "\n")
unreal.log_warning("VORTEX_HORSE_ANIMS " + " | ".join(lines))
