"""O baú em blocos (Tools/Blender/MakeChest.py) na Unreal — editor fechado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportChest.py -unattended -nullrhi

Data/Props/Chest/Chest_<Body|Lid>[_Rare].glb -> /Game/Vortex/Props/Chest/Chest_<...>/StaticMeshes/Chest_<...> e
T_Chest -> MI_Chest (a madeira, fosca) e MI_Chest_Brilho (as moedas, acesas). O AVortexWorldFeatures monta o baú
(o corpo e a tampa na dobradiça) e abre a tampa. Relatório em Saved/Logs/ImportChest.txt.
"""
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
DEST = "/Game/Vortex/Props/Chest"
src = os.path.join(root, "Data", "Props", "Chest")
lines = []


def bring(path, name):
    t = unreal.AssetImportTask()
    t.filename = path
    t.destination_path = DEST
    t.destination_name = name
    t.automated = True
    t.save = True
    t.replace_existing = True
    tools.import_asset_tasks([t])


def instance(name, parent):
    path = DEST + "/" + name
    mi = library.load_asset(path) if library.does_asset_exist(path) else \
        tools.create_asset(name, DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, parent)
    return mi


bring(os.path.join(src, "T_Chest.png"), "T_Chest")
tex = library.load_asset(DEST + "/T_Chest")
tex.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
tex.set_editor_property("never_stream", True)
library.save_loaded_asset(tex)
mi = instance("MI_Chest", library.load_asset("/Game/Vortex/Weapons/Materials/M_WeaponTextured"))
mel.set_material_instance_texture_parameter_value(mi, "Base", tex)
mel.set_material_instance_texture_parameter_value(mi, "Normal", unreal.load_asset("/Engine/EngineMaterials/FlatNormal"))
mel.set_material_instance_texture_parameter_value(mi, "RoughnessMap", unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture"))
mel.set_material_instance_scalar_parameter_value(mi, "Metallic", 0.0)
mel.set_material_instance_scalar_parameter_value(mi, "Bright", 1.4)
mel.set_material_instance_scalar_parameter_value(mi, "Glow", 0.0)
library.save_loaded_asset(mi)
glow = instance("MI_Chest_Brilho", mi)
mel.set_material_instance_scalar_parameter_value(glow, "Glow", 2.5)
library.save_loaded_asset(glow)

for name in ("Chest_Body", "Chest_Lid", "Chest_Body_Rare", "Chest_Lid_Rare"):
    bring(os.path.join(src, name + ".glb"), name)
    # o glb cai numa subpasta (DEST/<nome>/StaticMeshes)
    mesh = next((library.load_asset(p) for p in library.list_assets(DEST, recursive=True)
                 if p.split("/")[-1].split(".")[0] == name and isinstance(library.load_asset(p), unreal.StaticMesh)), None)
    if not mesh:
        lines.append("FALHOU " + name)
        continue
    settings = mesh.get_editor_property("nanite_settings")
    settings.set_editor_property("enabled", False)
    mesh.set_editor_property("nanite_settings", settings)
    if "Body" in name:
        # o corpo bloqueia o herói pela própria malha (é uma caixa)
        body = mesh.get_editor_property("body_setup")
        if body:
            body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    for i, s in enumerate(mesh.get_editor_property("static_materials")):
        mesh.set_material(i, glow if "Brilho" in str(s.get_editor_property("material_slot_name")) else mi)
    library.save_loaded_asset(mesh)
    box = mesh.get_bounding_box()
    lines.append("%s %s .. %s" % (mesh.get_path_name(), box.min, box.max))
open(os.path.join(root, "Saved", "Logs", "ImportChest.txt"), "w", encoding="utf-8").write("\n".join(lines) + "\n")
