"""O especial da espada inicial (Lâmina do Recruta, Corte à Distância; DA_DefaultSword) no nível dos outros especiais
comuns (10/11, o usuário: "baixar o especial"). Antes: 16 vezes o golpe (o padrão do C++), ~60 de dano no nível 1, que
matava qualquer bicho do começo de uma vez. Agora 3 vezes o golpe (os comuns vão de 2,6 a 3,4) e 9 s de recarga.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/FixStarterSwordSpecial.py -unattended -nullrhi
"""
import unreal

SCALE, COOLDOWN = 3.0, 9.0
library = unreal.EditorAssetLibrary
sword = library.load_asset("/Game/Vortex/Weapons/Sword/DA_DefaultSword")
before = (sword.get_editor_property("special_damage"), sword.get_editor_property("special_cooldown"))
sword.set_editor_property("special_damage", SCALE)
sword.set_editor_property("special_cooldown", COOLDOWN)
library.save_loaded_asset(sword)
unreal.log_warning("VORTEX_STARTER_SPECIAL antes dano=%.1f recarga=%.1f -> dano=%.1f recarga=%.1f" % (before + (SCALE, COOLDOWN)))
