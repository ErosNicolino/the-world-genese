"""Andar, correr, parado e pulo do Manny (FreeAnimationsPack, do projeto VortexReferencia) redirecionados para cada herói
novo — editor fechado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/RetargetHeroAnims.py -unattended -nullrhi

Antes: as peças do Manny copiadas para Content/FreeAnimationsPack/Demo/Characters/Mannequins (o esqueleto, o
SKM_Manny_Simple, o IK_Mannequin e as animações MM_*). Elas só servem aqui; depois podem sair do projeto.

- IK_Hero (/Game/Vortex/Characters/Heroes/Retarget): o IK Rig do esqueleto do aventureiro, com as correntes Spine,
  Head, LeftArm, RightArm, LeftLeg e RightLeg e a raiz no Hips;
- RTG_Manny_Hero: Manny -> herói, as correntes ligadas pelo nome e a pose de descanso do herói alinhada à do Manny
  (o herói está de braços caídos; o Manny, em A);
- para cada raça (o corpo dela é a malha alvo, então cada uma anda com as próprias proporções):
  /Game/Vortex/Characters/Heroes/Anims/<raça>/A_<raça>_<Idle|Walk|Run|Jump_Start|Jump_Loop|Jump_Land>.
UVortexAnimInstance usa esses clipes no lugar dos do aventureiro quando o herói é o modelo da raça.
Relatório em Saved/Logs/RetargetHeroAnims.txt.
"""
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
lines = []
MANNY = "/Game/FreeAnimationsPack/Demo/Characters/Mannequins"
FOLDER = "/Game/Vortex/Characters/Heroes/Retarget"
RACES = ["humano", "anao", "elfo", "barbaro", "diabo", "morto_vivo"]
CLIPS = {"MM_Idle": "Idle", "MM_Walk_Fwd": "Walk", "MM_Run_Fwd": "Run", "MM_Jump": "Jump_Start", "MM_Fall_Loop": "Jump_Loop", "MM_Land": "Jump_Land"}
# trote e nado (FreeAnimationLibrary, com o seu próprio esqueleto do Manny)
LIBRARY = "/Game/FreeAnimationLibrary"
LIBRARY_CLIPS = {"anim_Jog_Loop_Fwd": ("Jog", "/Animations/Jog/"), "anim_Swim_Surface_Fwd": ("Swim", "/Animations/Swim/"),
                 "anim_SwimIdle": ("Swim_Idle", "/Animations/Swim/")}
# a velocidade (cm/s) que o jogo usa como base de cada clipe (UVortexAnimInstance: taxa = velocidade / base); a taxa
# do clipe (RateScale) vira base / velocidade da animação, então o pé acompanha o chão em qualquer velocidade
BASE_SPEED = {"Walk": 240.0, "Jog": 460.0, "Run": 620.0, "Swim": 260.0}
Side = unreal.RetargetSourceOrTarget


def log(text):
    lines.append(text)
    unreal.log_warning("VORTEX_RETARGET " + text)


def fresh(name, cls, factory):
    """O asset existente (refeito por cima) ou um novo (apagar e criar de novo no mesmo processo falhava)."""
    path = FOLDER + "/" + name
    if library.does_asset_exist(path):
        return library.load_asset(path)
    return tools.create_asset(name, FOLDER, cls, factory)


manny_mesh = library.load_asset(MANNY + "/Meshes/SKM_Manny_Simple")
manny_rig = library.load_asset(MANNY + "/Rigs/IK_Mannequin")
hero_mesh = library.load_asset("/Game/Vortex/Characters/Heroes/humano/Hero_humano")

# ---- IK Rig dos heróis
rig = fresh("IK_Hero", unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
rc = unreal.IKRigController.get_controller(rig)
rc.set_skeletal_mesh(hero_mesh)
rc.set_retarget_root("Hips")
for old_chain in list(rc.get_retarget_chains()):
    rc.remove_retarget_chain(old_chain.get_editor_property("chain_name"))
for chain, start, end in (("Spine", "Torso", "Torso"), ("Head", "Head", "Head"),
                          ("LeftArm", "UpperArm_L", "Hand_L"), ("RightArm", "UpperArm_R", "Hand_R"),
                          ("LeftLeg", "Thigh_L", "Foot_L"), ("RightLeg", "Thigh_R", "Foot_R")):
    rc.add_retarget_chain(chain, start, end, "")
library.save_loaded_asset(rig)
log("IK_Hero: " + ", ".join(str(c.get_editor_property("chain_name")) for c in rc.get_retarget_chains()))
log("IK_Mannequin: " + ", ".join(str(c.get_editor_property("chain_name")) for c in unreal.IKRigController.get_controller(manny_rig).get_retarget_chains()))

# ---- redirecionador
rtg = fresh("RTG_Manny_Hero", unreal.IKRetargeter, unreal.IKRetargetFactory())
tc = unreal.IKRetargeterController.get_controller(rtg)
tc.set_ik_rig(Side.SOURCE, manny_rig)
tc.set_ik_rig(Side.TARGET, rig)
tc.set_preview_mesh(Side.SOURCE, manny_mesh)
tc.set_preview_mesh(Side.TARGET, hero_mesh)
try:
    tc.remove_all_ops()
    tc.add_default_ops()
except Exception as e:
    log("add_default_ops: %s" % e)
try:
    tc.assign_ik_rig_to_all_ops(Side.TARGET, rig)
    tc.assign_ik_rig_to_all_ops(Side.SOURCE, manny_rig)
except Exception as e:
    log("assign_ik_rig_to_all_ops: %s" % e)
tc.auto_map_chains(unreal.AutoMapChainType.FUZZY, True)
# o herói de braços caídos vira a pose em A do Manny só para comparar (a animação continua igual)
tc.auto_align_all_bones(Side.TARGET)
try:
    for i in range(tc.get_num_retarget_ops()):
        log("op %d: %s ligada=%s" % (i, tc.get_op_name(i), tc.get_retarget_op_enabled(i)))
except Exception as e:
    log("ops: %s" % e)
for chain in ("Spine", "Head", "LeftArm", "RightArm", "LeftLeg", "RightLeg"):
    try:
        log("corrente %s <- %s" % (chain, tc.get_source_chain(chain)))
    except Exception as e:
        log("corrente %s: %s" % (chain, e))
library.save_loaded_asset(rtg)

# ---- uma leva por raça (a malha da raça é o alvo)
sources = [unreal.EditorAssetLibrary.find_asset_data(MANNY + "/Animations/Manny/" + name) for name in CLIPS]
library_sources = [unreal.EditorAssetLibrary.find_asset_data(LIBRARY + folder + name) for name, (_, folder) in LIBRARY_CLIPS.items()]
library_mesh = library.load_asset(LIBRARY + "/Demo/Characters/Mannequins/Meshes/SKM_Manny_Simple")
NAMES = dict(CLIPS)
NAMES.update({name: clip for name, (clip, _) in LIBRARY_CLIPS.items()})


def clip_speed(anim):
    """cm/s que a raiz anda no clipe (as animações do Manny avançam a raiz)."""
    length = anim.get_editor_property("sequence_length")
    if length <= 0:
        return 0.0
    try:
        a = unreal.AnimationLibrary.get_bone_pose_for_time(anim, "Root", 0.0, False)
        b = unreal.AnimationLibrary.get_bone_pose_for_time(anim, "Root", length, False)
        d = b.translation - a.translation
        return (d.x ** 2 + d.y ** 2) ** .5 / length
    except Exception as e:
        log("velocidade: %s" % e)
        return 0.0


for race in RACES:
    target = library.load_asset("/Game/Vortex/Characters/Heroes/%s/Hero_%s" % (race, race))
    if not target:
        log("FALHOU sem malha " + race)
        continue
    out_folder = "/Game/Vortex/Characters/Heroes/Anims/" + race
    made = list(unreal.IKRetargetBatchOperation.duplicate_and_retarget(sources, manny_mesh, target, rtg, "", "", "RT_%s_" % race, "",
                                                                       out_folder, False, False, True))
    made += list(unreal.IKRetargetBatchOperation.duplicate_and_retarget(library_sources, library_mesh, target, rtg, "", "", "RT_%s_" % race, "",
                                                                        out_folder, False, False, True))
    count = 0
    for data in made:
        asset = data.get_asset()
        if not isinstance(asset, unreal.AnimSequence):
            continue
        source_name = str(data.asset_name)[len("RT_%s_" % race):]
        clip = NAMES.get(source_name)
        if not clip:
            continue
        speed = clip_speed(asset)
        if clip in BASE_SPEED and speed > 10:
            asset.set_editor_property("rate_scale", BASE_SPEED[clip] / speed)
        log("  %s %s: %.0f cm/s, taxa %.2f" % (race, clip, speed, asset.get_editor_property("rate_scale")))
        final = "%s/A_%s_%s" % (out_folder, race, clip)
        if library.does_asset_exist(final):
            library.delete_asset(final)
        library.rename_asset(asset.get_path_name().split(".")[0], final)
        moved = library.load_asset(final)
        if moved:
            library.save_loaded_asset(moved)
            count += 1
    log("%s: %d clipes em %s" % (race, count, out_folder))

with open(os.path.join(root, "Saved", "Logs", "RetargetHeroAnims.txt"), "w", encoding="utf-8") as out:
    out.write("\n".join(lines) + "\n")
