"""A adaga inicial (Tools/Blender/BuildStarterDagger.py) como visual da espada padrão do herói.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportStarterDagger.py -unattended
(também roda no editor aberto, pelo Python)

- Data/VortexWeapons/StarterDagger/W_adaga_inicial.glb -> /Game/Vortex/Weapons/StarterDagger, com o material e as
  três texturas do modelo (cor, metal/rugosidade e normal; no máximo 1024 px, é uma arma na mão);
- sem Nanite, como as outras armas;
- DA_DefaultSword (Espada de Cinzas) passa a usar a adaga no Socket_Weapon_R. Golpes, tempos e dano não mudam.
Relatório em Saved/Logs/ImportStarterDagger.txt.
"""
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
DEST = "/Game/Vortex/Weapons/StarterDagger"
lines = []

task = unreal.AssetImportTask()
task.filename = os.path.join(root, "Data", "VortexWeapons", "StarterDagger", "W_adaga_inicial.glb")
task.destination_path = DEST
task.automated = True
task.save = True
task.replace_existing = True
tools.import_asset_tasks([task])

mesh = None
for p in library.list_assets(DEST, recursive=True):
    asset = library.load_asset(p)
    if isinstance(asset, unreal.StaticMesh) and mesh is None:
        mesh = asset
    elif isinstance(asset, unreal.Texture2D):
        asset.set_editor_property("max_texture_size", 1024)
        library.save_loaded_asset(asset)
        lines.append("textura " + p)
if not mesh:
    raise RuntimeError("A adaga não foi importada: " + task.filename)

# como nas outras armas (ImportVortexWeapons.py): pouco metálico e um pouco mais claro. Sem Lumen o metal
# inteiro do glTF só refletia o chão e a lâmina ficava marrom-escura.
mel = unreal.MaterialEditingLibrary
for slot in mesh.get_editor_property("static_materials"):
    mi = slot.get_editor_property("material_interface")
    if isinstance(mi, unreal.MaterialInstanceConstant):
        mel.set_material_instance_scalar_parameter_value(mi, "MetallicFactor", .3)
        mel.set_material_instance_vector_parameter_value(mi, "BaseColorFactor", unreal.LinearColor(1.2, 1.2, 1.2, 1))
        library.save_loaded_asset(mi)
        lines.append("material " + mi.get_name() + " metálico .3, cor x1,2")

settings = mesh.get_editor_property("nanite_settings")
settings.set_editor_property("enabled", False)
mesh.set_editor_property("nanite_settings", settings)
library.save_loaded_asset(mesh)
lines.append("malha " + mesh.get_path_name())

sword = library.load_asset("/Game/Vortex/Weapons/Sword/DA_DefaultSword")
sword.set_editor_property("visual_mesh", mesh)
sword.set_editor_property("hand_bone", "Socket_Weapon_R")
library.save_loaded_asset(sword)
lines.append("espada padrão = %s no Socket_Weapon_R" % mesh.get_name())

with open(os.path.join(root, "Saved", "Logs", "ImportStarterDagger.txt"), "w", encoding="utf-8") as out:
    out.write("\n".join(lines) + "\n")
unreal.log("VORTEX_STARTER_DAGGER " + " | ".join(lines))
