"""Os heróis novos (Tools/Blender/RigHero.py) na Unreal, no esqueleto do aventureiro — editor fechado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportHeroes.py -unattended -nullrhi

- Data/Heroes/<raça>/Hero_<raça>.glb -> /Game/Vortex/Characters/Heroes/Hero_<raça> (malha com o esqueleto
  Adventurer_Skinned_Skeleton, então todas as animações do aventureiro servem);
- a textura do modelo -> MI_Hero_<raça> (filha do M_HeroTextured: a cor da textura, fosco);
- o esqueleto: a translação de cada osso vem da malha (modo "Skeleton"), menos a raiz e o quadril (da animação),
  para cada herói animar com as próprias proporções (o aventureiro e os bandidos continuam iguais).
Relatório em Saved/Logs/ImportHeroes.txt.
"""
import glob
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
DEST = "/Game/Vortex/Characters/Heroes"
SKELETON = "/Game/Vortex/Characters/Adventurer/Adventurer_Skinned/SkeletalMeshes/Adventurer_Skinned_Skeleton"
lines = []
skeleton = library.load_asset(SKELETON)

# material: a cor da textura do modelo, fosco (o modelo voxel não tem mapa de normais)
base_path = DEST + "/M_HeroTextured"
base = library.load_asset(base_path) if library.does_asset_exist(base_path) else \
    tools.create_asset("M_HeroTextured", DEST, unreal.Material, unreal.MaterialFactoryNew())
mel.delete_all_material_expressions(base)
tex = mel.create_material_expression(base, unreal.MaterialExpressionTextureSampleParameter2D, -500, 0)
tex.set_editor_property("parameter_name", "Base")
rough = mel.create_material_expression(base, unreal.MaterialExpressionScalarParameter, -300, 200)
rough.set_editor_property("parameter_name", "Roughness")
rough.set_editor_property("default_value", .85)
mel.connect_material_property(tex, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
base.set_editor_property("used_with_skeletal_mesh", True)
mel.recompile_material(base)
library.save_loaded_asset(base)

for path in sorted(glob.glob(os.path.join(root, "Data", "Heroes", "*", "Hero_*.glb"))):
    race = os.path.basename(path)[len("Hero_"):-4]
    if race.endswith("_Anims"):      # as animações do Mixamo (Tools/ImportHeroAnims.py)
        continue
    # 31/10: quem tem o herói do Mixamo como veio (esqueleto próprio) é o Tools/ImportMixamoHeroes.py
    if "_Mixamo" in path or os.path.isdir(os.path.join(root, "Data", "Heroes", race + "_Mixamo")):
        continue
    folder = DEST + "/" + race
    pipeline = unreal.InterchangeGenericAssetsPipeline()
    pipeline.set_editor_property("scene_name_sub_folder", False)
    pipeline.set_editor_property("asset_type_sub_folders", False)
    common = pipeline.get_editor_property("common_skeletal_meshes_and_animations_properties")
    common.set_editor_property("skeleton", skeleton)
    common.set_editor_property("try_auto_select_skeleton", False)
    pipeline.get_editor_property("mesh_pipeline").set_editor_property("create_physics_asset", False)
    pipeline.get_editor_property("animation_pipeline").set_editor_property("import_animations", False)
    override = unreal.InterchangePipelineStackOverride()
    override.add_pipeline(pipeline)
    task = unreal.AssetImportTask()
    task.filename = path
    task.destination_path = folder
    task.automated = True
    task.save = True
    task.replace_existing = True
    task.options = override
    tools.import_asset_tasks([task])
    mesh = None
    texture = None
    for p in library.list_assets(folder, recursive=True):
        asset = library.load_asset(p)
        if isinstance(asset, unreal.SkeletalMesh):
            mesh = asset
        elif isinstance(asset, unreal.Texture2D):
            # a cor (não o mapa de normais, o metálico ou a rugosidade)
            low = asset.get_name().lower()
            # a da cor é a única em sRGB (o importador marca os mapas de dados como lineares)
            if not any(k in low for k in ("normal", "metal", "rough", "occlusion")) and asset.get_editor_property("srgb"):
                texture = asset
    if not mesh:
        lines.append("FALHOU " + race)
        continue
    mi_path = folder + "/MI_Hero_" + race
    mi = library.load_asset(mi_path) if library.does_asset_exist(mi_path) else \
        tools.create_asset("MI_Hero_" + race, folder, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, base)
    if texture:
        mel.set_material_instance_texture_parameter_value(mi, "Base", texture)
    library.save_loaded_asset(mi)
    # o herói usa o MI no jogo (AVortexCharacter::ApplyHeroModel põe por cima de cada espaço); o Python da Unreal não
    # grava a troca de material da malha com esqueleto
    used = mi
    same = mesh.get_editor_property("skeleton") == skeleton
    lines.append("%s %s esqueleto do aventureiro=%s textura=%s material=%s" % (race, mesh.get_path_name(), same, texture.get_name() if texture else "-",
                                                                               used.get_name() if used else "-"))

with open(os.path.join(root, "Saved", "Logs", "ImportHeroes.txt"), "w", encoding="utf-8") as out:
    out.write("\n".join(lines) + "\n")
unreal.log_warning("VORTEX_HEROES " + " | ".join(lines))
