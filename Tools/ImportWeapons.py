"""As 25 armas do Godot na Unreal — rode com o editor fechado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=Tools/ImportWeapons.py -unattended

Antes: python Tools/BuildWeaponData.py, python Tools/ConvertGodotWeapons.py e
python Tools/ConvertGodotAdventurer.py <Adventurer.glb> Data/GodotCharacter/default_look.json
Data/GodotWeapons/Adventurer_WeaponClips.glb <clipes de Data/Rpg/weapon_clips.txt>.

- materiais: M_WeaponVertexColor (a cor de cada peça vem no vértice; alfa < 1 = brilho) e
  M_ProjectileGlow (núcleo das flechas, dardos e esferas, na cor do elemento);
- os 152 clipes das famílias de arma no esqueleto do aventureiro (/Game/Vortex/Weapons/Clips);
- os modelos (/Game/Vortex/Weapons/Models/W_<tipo>, e W_<tipo>_L das armas de par);
- uma definição por tipo (/Game/Vortex/Weapons/Types/DA_<tipo>, a espada continua no DA_DefaultSword):
  golpes (clipe, tempos, dano, alcance, modo, raio, velocidade), postura, especial e o modelo.
Escala: o dano do Godot vezes o do primeiro golpe da espada (o mesmo dano relativo); metros x 100.
"""
import json
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
skeleton = library.load_asset("/Game/Vortex/Characters/Adventurer/Adventurer_Skinned/SkeletalMeshes/Adventurer_Skinned_Skeleton")
weapons = json.load(open(os.path.join(root, "Data", "Rpg", "weapons.json"), encoding="utf-8"))["types"]
log = []


def material(name, build):
    path = "/Game/Vortex/Weapons/" + name
    if library.does_asset_exist(path):
        library.delete_asset(path)
    m = tools.create_asset(name, "/Game/Vortex/Weapons", unreal.Material, unreal.MaterialFactoryNew())
    build(m)
    MEL.recompile_material(m)
    library.save_loaded_asset(m)
    return m


def vertex_color(m):
    m.set_editor_property("two_sided", True)
    vc = MEL.create_material_expression(m, unreal.MaterialExpressionVertexColor, -800, 0)
    node = MEL.create_material_expression(m, unreal.MaterialExpressionCustom, -400, 200)
    node.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    node.set_editor_property("code", "return C.rgb * saturate(1.0 - C.a) * 9.0;")
    item = unreal.CustomInput()
    item.set_editor_property("input_name", "C")
    node.set_editor_property("inputs", [item])
    MEL.connect_material_expressions(vc, "", node, "C")
    MEL.connect_material_property(vc, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(node, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    rough = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -400, 400)
    rough.set_editor_property("r", .55)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)


def glow(m):
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    color = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -800, 0)
    color.set_editor_property("parameter_name", "Color")
    color.set_editor_property("default_value", unreal.LinearColor(.4, 1, .4, 1))
    strength = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -800, 200)
    strength.set_editor_property("parameter_name", "Glow")
    strength.set_editor_property("default_value", 6.0)
    fresnel = MEL.create_material_expression(m, unreal.MaterialExpressionFresnel, -800, 350)
    node = MEL.create_material_expression(m, unreal.MaterialExpressionCustom, -400, 100)
    node.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    # miolo quase branco, borda na cor do elemento
    node.set_editor_property("code", "float core = saturate(1.0 - F * 1.4); return lerp(C.rgb, float3(1,1,1), core * 0.7) * G * (0.6 + 1.4 * F + core);")
    items = []
    for n in ("C", "G", "F"):
        i = unreal.CustomInput()
        i.set_editor_property("input_name", n)
        items.append(i)
    node.set_editor_property("inputs", items)
    MEL.connect_material_expressions(color, "", node, "C")
    MEL.connect_material_expressions(strength, "", node, "G")
    MEL.connect_material_expressions(fresnel, "", node, "F")
    MEL.connect_material_property(node, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)


weapon_material = material("M_WeaponVertexColor", vertex_color)
glow_material = material("M_ProjectileGlow", glow)
# o projétil procura o brilho na pasta do combate
if library.does_asset_exist("/Game/Vortex/Combat/M_ProjectileGlow"):
    library.delete_asset("/Game/Vortex/Combat/M_ProjectileGlow")
library.duplicate_asset("/Game/Vortex/Weapons/M_ProjectileGlow", "/Game/Vortex/Combat/M_ProjectileGlow")

# ---- clipes
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
CLIPS = "/Game/Vortex/Weapons/Clips"
task = unreal.AssetImportTask()
task.filename = os.path.join(root, "Data", "GodotWeapons", "Adventurer_WeaponClips.glb")
task.destination_path = CLIPS
task.automated = True
task.save = True
task.replace_existing = True
task.options = override
tools.import_asset_tasks([task])
clip_folder = CLIPS + "/Adventurer_WeaponClips/SkeletalMeshes/"


def clip(name):
    if not name:
        return None
    if name in ("Attack_01", "Attack_02", "Attack_03"):
        return library.load_asset("/Game/Vortex/Weapons/Sword/Adventurer_SwordAttacks/SkeletalMeshes/Adventurer_SwordAttacks" + name)
    return library.load_asset(clip_folder + "Adventurer_WeaponClips" + name)


# o corpo que veio junto com os clipes não é usado (só o esqueleto)
for leftover in ("Adventurer_WeaponClips", "Adventurer_WeaponClips_PhysicsAsset"):
    if library.does_asset_exist(clip_folder + leftover):
        library.delete_asset(clip_folder + leftover)

# ---- modelos
MODELS = "/Game/Vortex/Weapons/Models"
mesh_pipeline = unreal.InterchangeGenericAssetsPipeline()
mesh_pipeline.get_editor_property("material_pipeline").set_editor_property("import_materials", False)
mesh_override = unreal.InterchangePipelineStackOverride()
mesh_override.add_pipeline(mesh_pipeline)
model_dir = os.path.join(root, "Data", "GodotWeapons", "Models")
tasks = []
for f in sorted(os.listdir(model_dir)):
    t = unreal.AssetImportTask()
    t.filename = os.path.join(model_dir, f)
    t.destination_path = MODELS
    t.automated = True
    t.save = True
    t.replace_existing = True
    t.options = mesh_override
    tasks.append(t)
tools.import_asset_tasks(tasks)


def model(name):
    for path in library.list_assets(MODELS, recursive=True):
        if path.split("/")[-1].split(".")[0] == name:
            data = library.find_asset_data(path)
            if str(data.asset_class_path.asset_name) == "StaticMesh":
                mesh = library.load_asset(path)
                mesh.set_material(0, weapon_material)
                library.save_loaded_asset(mesh)
                return mesh
    return None


# ---- definições
sword = library.load_asset("/Game/Vortex/Weapons/Sword/DA_DefaultSword")
K = sword.get_editor_property("swings")[0].get_editor_property("damage") / 1.0
log.append("escala de dano %.2f" % K)
guard_anim = sword.get_editor_property("guard_animation")
SOUNDS = {"sword_swing_1": "sword_swing_1", "sword_swing_2": "sword_swing_2", "sword_swing_3": "sword_swing_3"}
for w in weapons:
    if w["id"] == "espada_de_cinzas":
        continue
    name = "DA_" + w["id"]
    path = "/Game/Vortex/Weapons/Types/" + name
    asset = library.load_asset(path) if library.does_asset_exist(path) else None
    if not asset:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.VortexWeaponDefinition)
        asset = tools.create_asset(name, "/Game/Vortex/Weapons/Types", unreal.VortexWeaponDefinition, factory)
    swings = []
    missing = []
    for i, s in enumerate(w["swings"]):
        anim = clip(s["clip"])
        if not anim:
            missing.append(s["clip"])
        sw = unreal.VortexWeaponSwing()
        sw.set_editor_property("animation", anim)
        rate = max(.1, s.get("clip_speed") or 1.0)
        sw.set_editor_property("duration", s["duration"])
        sw.set_editor_property("active_start", s["active_start"])
        sw.set_editor_property("active_end", max(s["active_start"] + .06, s["impact"] + .08))
        sw.set_editor_property("damage", s["damage"] * K)
        sw.set_editor_property("reach_cm", max(150.0, s["reach"] * 100.0))
        sw.set_editor_property("min_forward_dot", s["min_dot"])
        sw.set_editor_property("mode", s["mode"])
        sw.set_editor_property("radius_cm", s["radius"] * 100.0)
        sw.set_editor_property("projectile_speed", s["speed"] * 100.0)
        sw.set_editor_property("play_rate", rate)
        sw.set_editor_property("stun", s["stun"])
        sw.set_editor_property("element", s["element"] or ("jade" if w["magic"] else ("nature" if s["mode"] == "arrow" else "")))
        sw.set_editor_property("sound", SOUNDS.get(s["sound"], "sword_swing_%d" % (i % 3 + 1)))
        swings.append(sw)
    asset.set_editor_property("weapon_id", w["id"])
    asset.set_editor_property("display_name", w["name"])
    asset.set_editor_property("wing", w["wing"])
    asset.set_editor_property("magic", w["magic"])
    asset.set_editor_property("primary_text", w["primary"])
    asset.set_editor_property("visual_mesh", model("W_" + w["id"]))
    asset.set_editor_property("left_mesh", model("W_" + w["id"] + "_L") if w["paired"] else None)
    asset.set_editor_property("hand_bone", "Socket_Weapon_R")
    asset.set_editor_property("primary_ability", unreal.VortexSwordAbility.static_class())
    asset.set_editor_property("secondary_ability", unreal.VortexGuardAbility.static_class())
    asset.set_editor_property("guard_animation", guard_anim)
    asset.set_editor_property("swings", swings)
    stance = w.get("stance", {})
    asset.set_editor_property("stance_idle", clip(stance.get("Idle")))
    asset.set_editor_property("stance_walk", clip(stance.get("Walk")))
    asset.set_editor_property("stance_run", clip(stance.get("Run")))
    sec = w["secondary"]
    asset.set_editor_property("secondary_kind", sec["kind"])
    asset.set_editor_property("secondary_shape", sec.get("shape", ""))
    asset.set_editor_property("secondary_name", sec["name"])
    asset.set_editor_property("secondary_text", sec["text"])
    asset.set_editor_property("secondary_icon", sec.get("icon", "").replace("/", "_"))
    asset.set_editor_property("secondary_clip", clip(sec.get("clip")))
    asset.set_editor_property("secondary_hold_clip", clip(sec.get("hold_clip")))
    values = {}
    for key, value in sec.items():
        if not isinstance(value, (int, float)) or isinstance(value, bool):
            continue
        if key in ("reach", "radius", "range"):
            value = value * 100.0
        elif key in ("min", "max", "base", "shield"):
            value = value * K
        values[key] = float(value)
    asset.set_editor_property("secondary_values", values)
    strongest = max((s["damage"] for s in w["swings"]), default=1)
    asset.set_editor_property("special_swing_index", -1)
    asset.set_editor_property("special_release", w["swings"][-1]["active_start"] if w["swings"] else .3)
    asset.set_editor_property("special_damage", strongest * 2 * K)
    asset.set_editor_property("special_cooldown", float(sec.get("cooldown", 4)))
    asset.set_editor_property("playable", bool(swings) and not missing and asset.get_editor_property("visual_mesh") is not None)
    library.save_loaded_asset(asset)
    log.append("%s golpes=%d especial=%s faltando=%s" % (w["id"], len(swings), sec["kind"], ",".join(missing) or "-"))

with open(os.path.join(root, "Saved", "Logs", "ImportWeapons.txt"), "w", encoding="utf-8") as out:
    out.write("VORTEX_WEAPONS\n" + "\n".join(log) + "\n")
