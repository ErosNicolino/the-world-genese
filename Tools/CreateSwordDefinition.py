"""Create the first native weapon data asset from Godot's authored sword combo profile."""

import json
import os
import unreal

project = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
profile_path = os.path.join(project, "Data", "GodotWeapons", "combo_profile.json")
profile = json.load(open(profile_path, encoding="utf-8"))
path = "/Game/Vortex/Weapons/Sword/DA_DefaultSword"
asset = unreal.EditorAssetLibrary.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
if not asset:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.VortexWeaponDefinition)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "DA_DefaultSword", "/Game/Vortex/Weapons/Sword", unreal.VortexWeaponDefinition, factory
    )
if not asset:
    raise RuntimeError("Could not create the sword data asset")

mesh_path = "/Game/Vortex/Weapons/Sword/Sword_Embedded/StaticMeshes/Sword_Embedded"
mesh = unreal.EditorAssetLibrary.load_asset(mesh_path)
if not mesh:
    raise RuntimeError("Sword visual was not imported")
asset.set_editor_property("weapon_id", "espada_de_cinzas")
asset.set_editor_property("visual_mesh", mesh)
asset.set_editor_property("primary_ability", unreal.VortexSwordAbility.static_class())
asset.set_editor_property("secondary_ability", unreal.VortexGuardAbility.static_class())
guard = unreal.EditorAssetLibrary.load_asset(
    "/Game/Vortex/Weapons/Sword/Adventurer_SwordAttacks/SkeletalMeshes/Adventurer_SwordAttacksSkill_Guard"
)
if not guard:
    raise RuntimeError("Missing guard animation")
asset.set_editor_property("guard_animation", guard)
asset.set_editor_property("chain_grace_seconds", profile["buffer"]["chain_window_after_attack_s"])
asset.set_editor_property("buffer_open_fraction", profile["buffer"]["open_fraction_of_attack"])

swings = []
for attack in profile["attacks"]:
    clip = attack["action"]
    animation_path = (
        "/Game/Vortex/Weapons/Sword/Adventurer_SwordAttacks/SkeletalMeshes/"
        "Adventurer_SwordAttacks" + clip
    )
    animation = unreal.EditorAssetLibrary.load_asset(animation_path)
    if not animation:
        raise RuntimeError("Missing imported clip: " + animation_path)
    swing = unreal.VortexWeaponSwing()
    swing.set_editor_property("animation", animation)
    swing.set_editor_property("duration", attack["duration_s"])
    swing.set_editor_property("active_start", attack["active_s"][0])
    swing.set_editor_property("active_end", attack["active_s"][1])
    swing.set_editor_property("damage", attack["damage"])
    # Godot MeleeReach: 3 m * 1.3 + .4 m; MeleeMinDot: .25 - .15.
    swing.set_editor_property("reach_cm", 430.0)
    swing.set_editor_property("min_forward_dot", 0.1)
    swings.append(swing)
asset.set_editor_property("swings", swings)
unreal.EditorAssetLibrary.save_loaded_asset(asset)
unreal.log("VORTEX_SWORD_DEFINITION {} swings={} mesh={}".format(path, len(swings), mesh_path))
