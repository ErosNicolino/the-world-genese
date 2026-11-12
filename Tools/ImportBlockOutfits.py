"""As roupas do humano de blocos na Unreal (01/11) — editor fechado, depois do ImportMixamoHeroes.py.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportBlockOutfits.py -unattended -nullrhi

Data/Heroes/humano_Mixamo/Roupas/Roupa_<id>.glb e T_Roupa_<id>.png (Tools/Blender/MakeBlockOutfit.py: as roupas da ficha
"modelando ainda/Ficha de Trajes RPG em Pixel Art.png", no esqueleto do herói) ->
/Game/Vortex/Characters/Heroes/humano/Roupas/Roupa_<id> (malha com os nomes de osso do herói: o AVortexCharacter veste
por cima do corpo com a pose dele, SetLeaderPoseComponent), T_Roupa_<id> (pixel nítido) e MI_Roupa_<id> (o material do
herói, M_HeroFace sem rosto não serve: o pai do MI_Hero_humano, com a textura da roupa no "Base").
Relatório em Saved/Logs/ImportBlockOutfits.txt.
"""
import glob
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
SOURCE = os.path.join(root, "Data", "Heroes", "humano_Mixamo", "Roupas")
DEST = "/Game/Vortex/Characters/Heroes/humano/Roupas"
lines = []

hero_mi = library.load_asset("/Game/Vortex/Characters/Heroes/humano/MI_Hero_humano")
parent = hero_mi.get_editor_property("parent") if hero_mi else None
lines.append("material pai: %s" % (parent.get_path_name() if parent else None))

for glb in sorted(glob.glob(os.path.join(SOURCE, "Roupa_*.glb"))):
    name = os.path.splitext(os.path.basename(glb))[0]
    oid = name[len("Roupa_"):]
    # a textura
    t = unreal.AssetImportTask()
    t.filename = os.path.join(SOURCE, "T_Roupa_%s.png" % oid)
    t.destination_path = DEST
    t.destination_name = "T_Roupa_" + oid
    t.automated = True
    t.save = False
    t.replace_existing = True
    tools.import_asset_tasks([t])
    tex = library.load_asset(DEST + "/T_Roupa_" + oid)
    tex.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
    tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    tex.set_editor_property("never_stream", True)
    tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    library.save_loaded_asset(tex)
    # o material: o mesmo do herói, com a textura da roupa
    mi_path = DEST + "/MI_Roupa_" + oid
    mi = library.load_asset(mi_path) if library.does_asset_exist(mi_path) else tools.create_asset(
        "MI_Roupa_" + oid, DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    # 10/11: com a máscara e o relevo (as roupas geradas desde 10/11), o material das armaduras: o metal brilha, o relevo
    # por pixel e o contorno de luz. Sem eles (as roupas antigas dos NPCs), o material do herói
    extra = {k: os.path.join(SOURCE, "T_Roupa_%s_%s.png" % (oid, k)) for k in ("mask", "normal")}
    armor_parent = library.load_asset("/Game/Vortex/Characters/Heroes/humano/Armaduras/M_ArmorBlock")
    rich = armor_parent and all(os.path.exists(f) for f in extra.values())
    if rich or parent:
        mi.set_editor_property("parent", armor_parent if rich else parent)
    mel.set_material_instance_texture_parameter_value(mi, "Base", tex)
    if rich:
        for k, f in extra.items():
            t2 = unreal.AssetImportTask()
            t2.filename = f
            t2.destination_path = DEST
            t2.destination_name = "T_Roupa_%s_%s" % (oid, k)
            t2.automated = True
            t2.replace_existing = True
            t2.save = False
            tools.import_asset_tasks([t2])
            tx2 = library.load_asset(DEST + "/T_Roupa_%s_%s" % (oid, k))
            tx2.set_editor_property("srgb", False)
            tx2.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
            tx2.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
            tx2.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP if k == "normal" else unreal.TextureCompressionSettings.TC_MASKS)
            library.save_loaded_asset(tx2)
            mel.set_material_instance_texture_parameter_value(mi, "Mask" if k == "mask" else "Normal", tx2)
    library.save_loaded_asset(mi)
    # a malha
    pipeline = unreal.InterchangeGenericAssetsPipeline()
    pipeline.set_editor_property("scene_name_sub_folder", False)
    pipeline.set_editor_property("asset_type_sub_folders", False)
    common = pipeline.get_editor_property("common_skeletal_meshes_and_animations_properties")
    common.set_editor_property("try_auto_select_skeleton", False)
    pipeline.get_editor_property("mesh_pipeline").set_editor_property("create_physics_asset", False)
    pipeline.get_editor_property("animation_pipeline").set_editor_property("import_animations", False)
    pipeline.get_editor_property("material_pipeline").set_editor_property("import_materials", False)
    override = unreal.InterchangePipelineStackOverride()
    override.add_pipeline(pipeline)
    task = unreal.AssetImportTask()
    task.filename = glb
    task.destination_path = DEST
    task.automated = True
    task.save = False
    task.replace_existing = True
    task.options = override
    tools.import_asset_tasks([task])
    mesh = library.load_asset(DEST + "/" + name) if library.does_asset_exist(DEST + "/" + name) else None
    if not mesh or not isinstance(mesh, unreal.SkeletalMesh):
        lines.append("FALHOU %s" % name)
        continue
    slots = [unreal.SkeletalMaterial(material_interface=mi, material_slot_name=s.get_editor_property("material_slot_name"),
                                     uv_channel_data=s.get_editor_property("uv_channel_data"))
             for s in mesh.get_editor_property("materials")]
    mesh.set_editor_property("materials", slots)
    library.save_loaded_asset(mesh)
    skeleton = mesh.get_editor_property("skeleton")
    if skeleton:
        library.save_loaded_asset(skeleton)
    # sobras da importação (texturas do glb)
    for a in library.list_assets(DEST, recursive=False):
        n = a.split("/")[-1].split(".")[0]
        if n.startswith(("Image", "Material_", "T_Roupa_" + oid + "_")) and n not in ("T_Roupa_%s_mask" % oid, "T_Roupa_%s_normal" % oid):
            library.delete_asset(a)
    lines.append("%s: %s (%d materiais)" % (name, mesh.get_path_name(), len(slots)))

with open(os.path.join(root, "Saved", "Logs", "ImportBlockOutfits.txt"), "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
unreal.log_warning("VORTEX_BLOCK_OUTFITS " + " | ".join(lines))
