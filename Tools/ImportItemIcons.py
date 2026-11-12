"""Ícones dos itens e das técnicas do Godot (somente leitura) em /Game/Vortex/UI/Icons/T_<ícone>.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=Tools/ImportItemIcons.py -unattended

A lista vem de Data/Rpg/icons.txt (Tools/BuildRpgData.py). "atlas/C04" vira T_atlas_C04. Também
traz os ícones das abas da janela do herói. Pixel art: sem filtro, sem mipmaps.
Os ícones das classes e habilidades (atlas_*) vêm do recorte novo (Tools/MakeSkillIcons.py, Data/UI/SkillIcons):
são pintura, não pixel art, então entram sempre de novo, com suavização e mipmaps (a barra os desenha pequenos).
Os ícones novos (skill_*, class_*, e os golpes weapon_* / special_* das armas, em Data/UI/SkillIcons/Novos) entram do
mesmo jeito.
"""
import os
import json
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
source = os.path.normpath(os.path.join(root, "..", "..", "the-world-geneses", "ui", "art", "icons"))
tools = unreal.AssetToolsHelpers.get_asset_tools()
library = unreal.EditorAssetLibrary
FOLDER = "/Game/Vortex/UI/Icons"
EXTRA = ["helm_leather", "ui_backpack", "staff_crystal", "shield_wood", "potion_green", "scroll", "ui_coin", "chest_tunic",
         "sword_iron", "potion_red", "ore_iron", "ring_jade"]

names = [line.strip() for line in open(os.path.join(root, "Data", "Rpg", "icons.txt"), encoding="utf-8") if line.strip()]
weapon_types = json.load(open(os.path.join(root, "Data", "Rpg", "weapons.json"), encoding="utf-8"))["types"]
names.extend(icon.replace("/", "_") for weapon in weapon_types
             for icon in (weapon.get("icon", ""), weapon.get("secondary", {}).get("icon", "")) if icon)
names = sorted(set(names) | set(EXTRA))
recut = os.path.join(root, "Data", "UI", "SkillIcons")
names = sorted(set(names) | {f[:-4] for f in os.listdir(recut) if f.startswith("atlas_") and f.endswith(".png")})
# os ícones novos das habilidades e classes (folhas de 8 do ChatGPT, Tools/SliceIconSheet.py): também pintura
fresh = os.path.join(recut, "Novos")
fresh_names = {f[:-4] for f in os.listdir(fresh) if f.startswith(("skill_", "class_", "weapon_", "special_", "armor_", "item_")) and f.endswith(".png")} if os.path.isdir(fresh) else set()
names = sorted(set(names) | fresh_names)
done, missing = 0, []
for name in names:
    painted = name in fresh_names or (name.startswith("atlas_") and os.path.exists(os.path.join(recut, name + ".png")))
    if library.does_asset_exist(FOLDER + "/T_" + name) and not painted:
        continue
    path = os.path.join(fresh if name in fresh_names else recut, name + ".png") if painted else os.path.join(source, name + ".png")
    if not os.path.exists(path):
        missing.append(name)
        continue
    task = unreal.AssetImportTask()
    task.filename = path
    task.destination_path = FOLDER
    task.destination_name = "T_" + name
    task.automated = True
    task.replace_existing = True
    task.save = False
    tools.import_asset_tasks([task])
    texture = library.load_asset(FOLDER + "/T_" + name)
    if not texture:
        missing.append(name)
        continue
    texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE if painted else unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property("filter", unreal.TextureFilter.TF_DEFAULT if painted else unreal.TextureFilter.TF_NEAREST)
    texture.set_editor_property("srgb", True)
    texture.set_editor_property("never_stream", True)   # a janela abre e o ícone já está lá
    library.save_loaded_asset(texture)
    done += 1
# a arte das raças (menu) também aparece na janela do herói: sempre inteira na memória
for race in ("humano", "elfo", "diabo", "anao", "barbaro", "morto_vivo"):
    art = library.load_asset("/Game/Vortex/UI/Menu/Races/T_Race_" + race)
    if art:
        art.set_editor_property("never_stream", True)
        library.save_loaded_asset(art)
with open(os.path.join(root, "Saved", "Logs", "ImportItemIcons.txt"), "w", encoding="utf-8") as out:
    out.write("VORTEX_ICONS %d importados, faltando: %s\n" % (done, ", ".join(missing) or "nenhum"))
