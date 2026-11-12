"""Os clipes novos das famílias de arma (Tools/Blender/BuildWeaponClips.py) nas definições — editor fechado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportWeaponClips.py -unattended

- Data/VortexWeapons/Adventurer_VortexClips.glb -> /Game/Vortex/Weapons/VxClips (no esqueleto do aventureiro);
- cada DA_<tipo> da família passa a usar:
    postura = Vx_<Família>_Hold (o anim instance mistura com parado/andar/correr),
    golpes  = Vx_<Família>_Attack_0N (com o tempo do clipe e a janela de acerto de clips.json),
    secundário = Vx_<Família>_Special (e a carga: Vx_Wand_Charge, Vx_Bow_Draw), defesa da espada e escudo = Vx_SwordShield_Guard;
- números da revisão que moram no DA: alcance da alabarda (4,5 m) e da besta (40 m), dano da besta (1,3× o do arco).
Manoplas e martelo continuam com os clipes do Godot (você gostou deles). Relatório em Saved/Logs/ImportWeaponClips.txt.
"""
import json
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
skeleton = library.load_asset("/Game/Vortex/Characters/Adventurer/Adventurer_Skinned/SkeletalMeshes/Adventurer_Skinned_Skeleton")
data = json.load(open(os.path.join(root, "Data", "VortexWeapons", "clips.json"), encoding="utf-8"))
HITS = data["hits"]
FAMILY_OF = {weapon: family for family, weapon in data["families"].items()}
lines = []

# ---- importar
pipeline = unreal.InterchangeGenericAssetsPipeline()
pipeline.set_editor_property("scene_name_sub_folder", True)
pipeline.set_editor_property("asset_type_sub_folders", True)
common = pipeline.get_editor_property("common_skeletal_meshes_and_animations_properties")
common.set_editor_property("skeleton", skeleton)
common.set_editor_property("try_auto_select_skeleton", False)
pipeline.get_editor_property("mesh_pipeline").set_editor_property("create_physics_asset", False)
pipeline.get_editor_property("material_pipeline").set_editor_property("import_materials", False)
pipeline.get_editor_property("material_pipeline").get_editor_property("texture_pipeline").set_editor_property("import_textures", False)
override = unreal.InterchangePipelineStackOverride()
override.add_pipeline(pipeline)
CLIPS = "/Game/Vortex/Weapons/VxClips"
task = unreal.AssetImportTask()
task.filename = os.path.join(root, "Data", "VortexWeapons", "Adventurer_VortexClips.glb")
task.destination_path = CLIPS
task.automated = True
task.save = True
task.replace_existing = True
task.options = override
tools.import_asset_tasks([task])

found = {}
for path in library.list_assets(CLIPS, recursive=True):
    asset_name = path.split("/")[-1].split(".")[0]
    data_ = library.find_asset_data(path)
    cls = str(data_.asset_class_path.asset_name)
    if cls == "AnimSequence" and "Vx_" in asset_name:
        found[asset_name[asset_name.index("Vx_"):]] = path
    elif cls in ("SkeletalMesh", "PhysicsAsset", "Skeleton"):
        library.delete_asset(path)   # o corpo que veio junto não é usado
lines.append("clipes %d" % len(found))


def clip(name):
    path = found.get(name)
    return library.load_asset(path) if path else None


def first(*names):
    for n in names:
        a = clip(n)
        if a:
            return a, n
    return None, None


bow = library.load_asset("/Game/Vortex/Weapons/Types/DA_arco_de_betula")
bow_damage = bow.get_editor_property("swings")[0].get_editor_property("damage") if bow else 10.0
REACH = {"alabarda": 450.0, "besta": 4000.0, "arco_de_betula": 2500.0, "correntes": 500.0}
# elemento dos golpes mágicos (a cor, o rastro e o desenho do projétil)
ELEMENT = {"instrumento": "music", "orbe": "holy", "livro_da_aurora": "fire", "cajado_de_ilsa": "vine",
           "pistola_runica": "rune", "varinha": "arcane"}
# segurar o botão direito: o clipe que fica tocando (o instrumento toca o próprio secundário)
HOLD_SPECIAL = {"Lute": "Vx_Lute_Special"}

for weapon, family in FAMILY_OF.items():
    path = "/Game/Vortex/Weapons/Types/DA_" + weapon
    da = library.load_asset(path)
    if not da:
        lines.append("%s: sem DA" % weapon)
        continue
    hold = clip("Vx_%s_Hold" % family)
    if hold:
        for prop in ("stance_idle", "stance_walk", "stance_run"):
            da.set_editor_property(prop, hold)
    swings = list(da.get_editor_property("swings"))
    used = []
    for i, sw in enumerate(swings):
        n = i + 1
        anim, name = first("Vx_%s_Attack_%02d" % (family, n), "Vx_%s_Attack_%02d_Full" % (family, n),
                           "Vx_%s_Attack_01" % family)
        if family == "Bow":
            anim, name = first("Vx_Bow_Release")
        if not anim:
            continue
        length = anim.get_editor_property("sequence_length")
        hit = HITS.get(name, length * .45)
        sw.set_editor_property("animation", anim)
        sw.set_editor_property("play_rate", 1.0)
        sw.set_editor_property("duration", max(.2, length))
        melee = str(sw.get_editor_property("mode")) in ("melee", "slam")
        sw.set_editor_property("active_start", max(.02, hit - (.05 if melee else 0)))
        sw.set_editor_property("active_end", hit + (.1 if melee else .06))
        if weapon in REACH and str(sw.get_editor_property("mode")) != "slam":
            sw.set_editor_property("reach_cm", REACH[weapon])
        if weapon == "besta":
            sw.set_editor_property("damage", bow_damage * 1.3)
        if weapon in ELEMENT:
            sw.set_editor_property("element", ELEMENT[weapon])
        used.append(name)
    da.set_editor_property("swings", swings)
    special, sname = first("Vx_%s_Special" % family, "Vx_%s_Special_Full" % family)
    if special:
        da.set_editor_property("secondary_clip", special)
    charge, _ = first(HOLD_SPECIAL.get(family, "-"), "Vx_%s_Charge" % family, "Vx_%s_Draw" % family)
    if charge:
        da.set_editor_property("secondary_hold_clip", charge)
    guard, _ = first("Vx_%s_Guard" % family)
    if guard:
        da.set_editor_property("guard_animation", guard)
    library.save_loaded_asset(da)
    lines.append("%s (%s): postura=%s golpes=%s especial=%s" % (weapon, family, bool(hold), ",".join(used), sname))

with open(os.path.join(root, "Saved", "Logs", "ImportWeaponClips.txt"), "w", encoding="utf-8") as out:
    out.write("VORTEX_WEAPON_CLIPS\n" + "\n".join(lines) + "\n")
