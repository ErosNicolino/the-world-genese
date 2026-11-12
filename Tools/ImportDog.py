"""O cachorro da Julia em blocos (Tools/Blender/MakeDog.py) na Unreal — editor fechado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportDog.py -unattended -nullrhi

Data/Creatures/Pet/Dog_<parte>.glb -> /Game/Vortex/Creatures/Pet/Dog_<parte>/StaticMeshes/Dog_<parte> (Body, Head, Leg, Tail) e T_Dog ->
MI_Dog (filha do M_WeaponTextured: pixel nítido, fosco). O AVortexPet monta as partes nos pivôs dele.
Relatório em Saved/Logs/ImportDog.txt (a caixa de cada parte: a cabeça tem de ir para +X).
"""
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
DEST = "/Game/Vortex/Creatures/Pet"
src = os.path.join(root, "Data", "Creatures", "Pet")
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
    # os glb caem numa subpasta (DEST/<nome>/StaticMeshes): quem chama procura
    return None if path.endswith(".glb") else library.load_asset(DEST + "/" + name)


tex = bring(os.path.join(src, "T_Dog.png"), "T_Dog")
tex.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
tex.set_editor_property("never_stream", True)
library.save_loaded_asset(tex)
path = DEST + "/MI_Dog"
mi = library.load_asset(path) if library.does_asset_exist(path) else \
    tools.create_asset("MI_Dog", DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
mel.set_material_instance_parent(mi, library.load_asset("/Game/Vortex/Weapons/Materials/M_WeaponTextured"))
mel.set_material_instance_texture_parameter_value(mi, "Base", tex)
mel.set_material_instance_texture_parameter_value(mi, "Normal", unreal.load_asset("/Engine/EngineMaterials/FlatNormal"))
mel.set_material_instance_texture_parameter_value(mi, "RoughnessMap", unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture"))
mel.set_material_instance_scalar_parameter_value(mi, "Metallic", 0.0)
mel.set_material_instance_scalar_parameter_value(mi, "Bright", 1.6)   # o branco na sombra saía cinza-escuro
mel.set_material_instance_scalar_parameter_value(mi, "Glow", 0.0)
library.save_loaded_asset(mi)

for part in ("Body", "Head", "Leg", "Tail"):
    # a malha sai com o nome do objeto de dentro do glb (Dog_<parte>), numa pasta própria (DEST/Dog_<parte>/StaticMeshes)
    bring(os.path.join(src, "Dog_%s.glb" % part), "Dog_" + part)
    mesh = next((library.load_asset(p) for p in library.list_assets(DEST, recursive=True)
                 if p.split("/")[-1].split(".")[0] == "Dog_" + part and isinstance(library.load_asset(p), unreal.StaticMesh)), None)
    settings = mesh.get_editor_property("nanite_settings")
    settings.set_editor_property("enabled", False)
    mesh.set_editor_property("nanite_settings", settings)
    for i in range(len(mesh.get_editor_property("static_materials"))):
        mesh.set_material(i, mi)
    library.save_loaded_asset(mesh)
    box = mesh.get_bounding_box()
    lines.append("%s %s .. %s" % (mesh.get_path_name(), box.min, box.max))
open(os.path.join(root, "Saved", "Logs", "ImportDog.txt"), "w", encoding="utf-8").write("\n".join(lines) + "\n")
