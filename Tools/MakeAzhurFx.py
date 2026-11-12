"""Os efeitos Niagara do Azhûr, o golem vulcânico (02/11) — editor fechado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/MakeAzhurFx.py -unattended -nullrhi

Cada sistema é uma cópia de um efeito dos pacotes gratuitos do projeto (Vefects Free Fire, Free Spells, ArrowTrail),
com o nome que o chefe usa, em /Game/Vortex/Bosses/Azhur/FX. O UVortexVolcanicBoss liga cada um na hora da animação,
com escala, duração e cor (User.Color, quando o sistema tem) vindas do Data/Creatures/azhur.json. Os originais
continuam intactos. Sem as bases do Free Spells com runas douradas (02/11: em cada golpe elas cobriam a tela). Relatório em Saved/Logs/MakeAzhurFx.txt.
"""
import os
import unreal

library = unreal.EditorAssetLibrary
root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
DEST = "/Game/Vortex/Bosses/Azhur/FX"
SOURCES = {
    "NS_MagmaCore_Charge": "/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Aura_Fire",
    "NS_MagmaCore_Explosion": "/Game/Vefects/Free_Fire/Shared/Particles/NS_Fire_Big",
    "NS_Volcanic_Fissure": "/Game/Vefects/Free_Fire/Shared/Particles/NS_Fire_Floor_02_Simple",
    "NS_Lava_Pillar": "/Game/Vefects/Free_Fire/Shared/Particles/NS_Fire_Big_Simple",
    "NS_Molten_Meteor": "/Game/ArrowTrail/FX/NS_ArrowTrail_Fire_02",
    "NS_Core_Beam": "/Game/Vefects/Free_Fire/Shared/Particles/NS_Fire_Medium",
    "NS_Stone_Impact": "/Game/Vefects/Free_Fire/Shared/Particles/NS_Fire_Floor_02_Smoke",
    "NS_Rock_Debris": "/Game/Vefects/Free_Fire/Shared/Particles/NS_Fire_Floor_01_Smoke",
    "NS_Volcanic_Death": "/Game/Vefects/Free_Fire/Shared/Particles/NS_Fire_Big_Smoke",
}
lines = []
for name, src in SOURCES.items():
    dest = DEST + "/" + name
    if library.does_asset_exist(dest):
        library.delete_asset(dest)
    if not library.does_asset_exist(src):
        lines.append("FALTA " + src)
        continue
    made = library.duplicate_asset(src, dest)
    if made:
        library.save_loaded_asset(made)
    lines.append("%s <- %s %s" % (name, src, "ok" if made else "FALHOU"))
out = os.path.join(root, "Saved", "Logs", "MakeAzhurFx.txt")
open(out, "w", encoding="utf-8").write("\n".join(lines) + "\n")
unreal.log_warning("VORTEX_AZHUR_FX " + " | ".join(lines))
