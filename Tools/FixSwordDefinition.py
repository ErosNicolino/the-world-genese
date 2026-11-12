"""A Espada de Cinzas continua no DA_DefaultSword (golpes da Fase da espada), mas o Estádio precisa dos
dados de exibição dela: nome, ala, textos do golpe e do especial, e "jogável". Sem isto o pedestal da
espada dizia "Esta arma ainda não chegou ao Vortex". Rode com o editor fechado:

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=Tools/FixSwordDefinition.py -unattended
"""
import json
import os
import unreal

library = unreal.EditorAssetLibrary
root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
weapons = json.load(open(os.path.join(root, "Data", "Rpg", "weapons.json"), encoding="utf-8"))["types"]
w = [t for t in weapons if t["id"] == "espada_de_cinzas"][0]
sword = library.load_asset("/Game/Vortex/Weapons/Sword/DA_DefaultSword")
sword.set_editor_property("weapon_id", w["id"])
sword.set_editor_property("display_name", w["name"])
sword.set_editor_property("wing", w["wing"])
sword.set_editor_property("magic", w["magic"])
sword.set_editor_property("primary_text", w["primary"])
sec = w["secondary"]
sword.set_editor_property("secondary_name", sec["name"])
sword.set_editor_property("secondary_text", sec["text"])
sword.set_editor_property("secondary_icon", sec.get("icon", "").replace("/", "_"))
sword.set_editor_property("playable", True)
library.save_loaded_asset(sword)
with open(os.path.join(root, "Saved", "Logs", "FixSwordDefinition.txt"), "w", encoding="utf-8") as out:
    out.write("VORTEX_SWORD_DEFINITION %s %s\n" % (w["name"], w["wing"]))
