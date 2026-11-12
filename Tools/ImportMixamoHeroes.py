"""Os heróis do Mixamo como vieram (Tools/Blender/MixamoToHero.py): a malha com o esqueleto completo do Mixamo e os
clipes nesse esqueleto — editor fechado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportMixamoHeroes.py -unattended -nullrhi

Data/Heroes/<raça>_Mixamo/Hero_<raça>.glb ->
- /Game/Vortex/Characters/Heroes/<raça>/Hero_<raça> (malha) e Hero_<raça>_Skeleton (o esqueleto próprio, não o do
  aventureiro), com o MI_Hero_<raça> que já estava na pasta (a textura do herói);
- /Game/Vortex/Characters/Heroes/Anims/<raça>/MX_<raça>_<clipe> (UVortexAnimInstance::Clip).
A taxa (RateScale) dos clipes que andam é a velocidade base do jogo / a do clipe (clips.json), sem limite: o
UVortexAnimInstance tira dela a velocidade do clipe e toca na taxa que casa o passo com o chão.
Antes de rodar, Hero_<raça>, Hero_<raça>_FP e Anims/<raça> antigos (no esqueleto do aventureiro) saem de Content.
VORTEX_RACES=humano limita a uma raça. Relatório em Saved/Logs/ImportMixamoHeroes.txt.
"""
import glob
import json
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
DEST = "/Game/Vortex/Characters/Heroes"
# a velocidade (cm/s) que o jogo usa como base de cada clipe (UVortexAnimInstance: taxa = velocidade / base)
# (o nado fica de fora: o clipe é no lugar e a medida pelo pé não vale)
BASE_SPEED = {"Walk": 150.0, "Jog": 300.0, "Run": 460.0, "Sprint": 620.0, "RunArmed": 460.0}
only = [x for x in os.environ.get("VORTEX_RACES", "").split(",") if x]
lines = []
kind = lambda p: str(library.find_asset_data(p).asset_class_path.asset_name)

for path in sorted(glob.glob(os.path.join(root, "Data", "Heroes", "*_Mixamo", "Hero_*.glb"))):
    race = os.path.basename(path)[len("Hero_"):-4]
    if only and race not in only:
        continue
    clips = json.load(open(os.path.join(os.path.dirname(path), "clips.json"), encoding="utf-8"))
    folder = DEST + "/" + race
    anims = DEST + "/Anims/" + race
    staging = folder + "/_mixamo"
    if library.does_directory_exist(staging):
        library.delete_directory(staging)
    # reimportação: os clipes antes da malha e a malha antes do esqueleto (quem é usado não se apaga, e os clipes novos
    # ficavam presos a um esqueleto temporário)
    # (se um clipe ficou sem esqueleto, a Unreal não consegue nem apagar: aí a pasta sai de Content à mão)
    try:
        if library.does_directory_exist(anims):
            library.delete_directory(anims)
        for old in (folder + "/Hero_%s" % race, folder + "/Hero_%s_Skeleton" % race):
            if library.does_asset_exist(old):
                library.delete_asset(old)
    except Exception as error:
        lines.append("%s: não apagou os antigos (%s)" % (race, str(error)[:120]))
    pipeline = unreal.InterchangeGenericAssetsPipeline()
    pipeline.set_editor_property("scene_name_sub_folder", False)
    pipeline.set_editor_property("asset_type_sub_folders", False)
    common = pipeline.get_editor_property("common_skeletal_meshes_and_animations_properties")
    common.set_editor_property("try_auto_select_skeleton", False)
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
    assets = library.list_assets(staging, recursive=True)
    mesh_path = next((p for p in assets if kind(p) == "SkeletalMesh"), None)
    skel_path = next((p for p in assets if kind(p) == "Skeleton"), None)
    if not mesh_path or not skel_path:
        lines.append("FALHOU %s: %s" % (race, assets))
        continue
    # o esqueleto e a malha com os nomes que o jogo procura
    for old, new in ((skel_path, folder + "/Hero_%s_Skeleton" % race), (mesh_path, folder + "/Hero_%s" % race)):
        if library.does_asset_exist(new):
            library.delete_asset(new)
        library.rename_asset(old.split(".")[0], new)
    mesh = library.load_asset(folder + "/Hero_%s" % race)
    mi = library.load_asset(folder + "/MI_Hero_" + race)
    # herói de pixel (Data/Heroes/Mixamo/<raça>/pixel.json, o humano de blocos): a cor vem de <raça>_cor.png, com o pixel
    # nítido (sem mipmap e sem filtro), no lugar da textura do modelo antigo
    if os.path.exists(os.path.join(root, "Data", "Heroes", "Mixamo", race, "pixel.json")) and mi:
        t = unreal.AssetImportTask()
        t.filename = os.path.join(root, "Data", "Heroes", "Mixamo", race + "_cor.png")
        t.destination_path = folder
        t.destination_name = "T_%s_Pixel" % race
        t.automated = True
        t.save = True
        t.replace_existing = True
        tools.import_asset_tasks([t])
        tex = library.load_asset(folder + "/T_%s_Pixel" % race)
        tex.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
        tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        tex.set_editor_property("never_stream", True)
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        library.save_loaded_asset(tex)
        mel.set_material_instance_texture_parameter_value(mi, "Base", tex)
        library.save_loaded_asset(mi)
        lines.append("%s: textura de pixel %s" % (race, tex.get_name()))
    if mi:
        slots = []
        for slot in mesh.get_editor_property("materials"):
            s = unreal.SkeletalMaterial()
            s.set_editor_property("material_interface", mi)
            s.set_editor_property("material_slot_name", slot.get_editor_property("material_slot_name"))
            slots.append(s)
        mesh.modify()
        mesh.set_editor_property("materials", slots)
    library.save_loaded_asset(mesh)
    library.save_loaded_asset(library.load_asset(folder + "/Hero_%s_Skeleton" % race))
    count = 0
    for p in assets:
        if kind(p) != "AnimSequence":
            continue
        asset = library.load_asset(p)
        # o nome que o glTF dá: o da faixa, às vezes com o do arquivo na frente
        name = next((c for c in sorted(clips, key=len, reverse=True) if asset.get_name().endswith(c)), None)
        if not name:
            lines.append("%s: sem clipe para %s (apagado)" % (race, asset.get_name()))
            library.delete_asset(p.split(".")[0])
            continue
        info = clips[name]
        rate = 1.0
        if name in BASE_SPEED and info["speed"] > 10:
            # sem limite: o jogo usa o RateScale para saber a velocidade do clipe e calcula a taxa pelo tamanho do herói
            rate = BASE_SPEED[name] / info["speed"]
        asset.set_editor_property("rate_scale", rate)
        asset.set_editor_property("enable_root_motion", False)
        final = "%s/MX_%s_%s" % (anims, race, name)
        if library.does_asset_exist(final):
            library.delete_asset(final)
        library.rename_asset(p.split(".")[0], final)
        moved = library.load_asset(final)
        if moved:
            library.save_loaded_asset(moved)
            count += 1
        lines.append("  %s %-18s %5.1f s  %5.0f cm/s  taxa %.2f" % (race, name, info["seconds"], info["speed"], rate))
    library.delete_directory(staging)
    skeleton = mesh.get_editor_property("skeleton")
    bones = [str(skeleton.get_reference_pose().get_bone_names()[i]) for i in range(8)] if hasattr(skeleton, "get_reference_pose") else []
    lines.append("%s: malha %s, esqueleto %s, %d de %d clipes em %s; ossos %s" % (
        race, mesh.get_path_name(), skeleton.get_path_name(), count, len(clips), anims, bones))

os.makedirs(os.path.join(root, "Saved", "Logs"), exist_ok=True)
with open(os.path.join(root, "Saved", "Logs", "ImportMixamoHeroes.txt"), "w", encoding="utf-8") as out:
    out.write("\n".join(lines) + "\n")
unreal.log_warning("VORTEX_MIXAMO_HEROES " + " | ".join(l for l in lines if not l.startswith("  ")))
