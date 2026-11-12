"""Os monstros novos do Meshy (com as animações dos antigos: Tools/Blender/MeshyBiped.py) no lugar dos do Godot —
editor fechado.

UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho completo>/Tools/ImportMeshyMonsters.py -unattended -nullrhi

Cada um vira /Game/Vortex/Creatures/<Espécie>/<Espécie>_Skinned (malha e os clipes <Espécie>_Skinned<Clipe>, os
caminhos que AVortexMonster procura), com MI_<Espécie>_Meshy filha de M_MeshyCreature: a textura do modelo e um brilho
pela própria cor (os olhos e as pedras verdes do Thorun). Antes de rodar, a pasta antiga sai de Content.
VORTEX_SPECIES=Golem limita a uma espécie. Relatório em Saved/Logs/ImportMeshyMonsters.txt.
"""
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
SHARED = "/Game/Vortex/Creatures/Shared"
# espécie: (glb, brilho verde)
MONSTERS = {
    "Golem": (os.path.join("Data", "Modelos", "Golem_Musgoso", "Golem_Skinned.glb"), 0.0),
    "RootTitan": (os.path.join("Data", "Modelos", "Thorun_Tita", "RootTitan_Skinned.glb"), 6.0),
    # 02/11: o goblin novo do Meshy (corpo de gente, com a corrida do site e um porrete de blocos)
    "Goblin": (os.path.join("Data", "Modelos", "Goblin_Meshy", "Goblin_Skinned.glb"), 0.0),
    # 10/11: o Cogumelo Corrompido (Docs/Modelos_Meshy), do tamanho do antigo, com os clipes do Mixamo e os especiais
    "Mushroom": (os.path.join("Data", "Modelos", "Cogumelo_Corrompido", "Mushroom_Skinned.glb"), 0.0),
}
only = [x for x in os.environ.get("VORTEX_SPECIES", "").split(",") if x]
lines = []


def node(m, kind, x, y, **props):
    e = mel.create_material_expression(m, kind, x, y)
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e


def link(a, b, pin_b="", pin_a=""):
    mel.connect_material_expressions(a, pin_a, b, pin_b)


def base_material(texture):
    path = SHARED + "/M_MeshyCreature"
    m = library.load_asset(path) if library.does_asset_exist(path) else tools.create_asset("M_MeshyCreature", SHARED, unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(m)
    base = node(m, unreal.MaterialExpressionTextureSampleParameter2D, -900, 0, parameter_name="Base", texture=texture)
    mel.connect_material_property(base, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(node(m, unreal.MaterialExpressionConstant, -400, 160, r=.85), "", unreal.MaterialProperty.MP_ROUGHNESS)
    # brilho onde o verde passa muito do vermelho e do azul (as pedras e os olhos do Thorun)
    r = node(m, unreal.MaterialExpressionComponentMask, -700, 260, r=True, g=False, b=False, a=False)
    g = node(m, unreal.MaterialExpressionComponentMask, -700, 340, r=False, g=True, b=False, a=False)
    bl = node(m, unreal.MaterialExpressionComponentMask, -700, 420, r=False, g=False, b=True, a=False)
    for c in (r, g, bl):
        link(base, c)
    mx = node(m, unreal.MaterialExpressionMax, -560, 380)
    link(r, mx, "A")
    link(bl, mx, "B")
    diff = node(m, unreal.MaterialExpressionSubtract, -430, 330)
    link(g, diff, "A")
    link(mx, diff, "B")
    shifted = node(m, unreal.MaterialExpressionAdd, -320, 330, const_b=-.25)
    link(diff, shifted, "A")
    gain = node(m, unreal.MaterialExpressionMultiply, -220, 330, const_b=5.0)
    link(shifted, gain, "A")
    mask = node(m, unreal.MaterialExpressionSaturate, -120, 330)
    link(gain, mask)
    strength = node(m, unreal.MaterialExpressionScalarParameter, -220, 460, parameter_name="Glow", default_value=0.0)
    amount = node(m, unreal.MaterialExpressionMultiply, -20, 380)
    link(mask, amount, "A")
    link(strength, amount, "B")
    emit = node(m, unreal.MaterialExpressionMultiply, 100, 200)
    link(base, emit, "A", "RGB")
    link(amount, emit, "B")
    mel.connect_material_property(emit, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    m.set_editor_property("used_with_skeletal_mesh", True)
    mel.recompile_material(m)
    library.save_loaded_asset(m)
    return m


def mushroom_material(folder, color):
    """M_CogumeloCorrompido: a cor da textura (fosca); onde a máscara é vermelha (os olhos) acende com Olhos (o
    AVortexMonster pulsa, pisca com Piscar e acende mais antes do golpe); onde é verde (as manchas roxas do chapéu)
    pulsa devagar com Manchas."""
    mask_file = os.path.join(root, "Data", "Modelos", "Cogumelo_Corrompido", "T_Cogumelo_Mascara.png")
    if not os.path.exists(mask_file):
        return None
    t = unreal.AssetImportTask()
    t.filename = mask_file
    t.destination_path = folder
    t.destination_name = "T_Cogumelo_Mascara"
    t.automated = True
    t.replace_existing = True
    t.save = True
    tools.import_asset_tasks([t])
    mask_tex = library.load_asset(folder + "/T_Cogumelo_Mascara")
    mask_tex.set_editor_property("srgb", False)
    mask_tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
    library.save_loaded_asset(mask_tex)
    path = folder + "/M_CogumeloCorrompido"
    m = library.load_asset(path) if library.does_asset_exist(path) else tools.create_asset("M_CogumeloCorrompido", folder, unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(m)
    base = node(m, unreal.MaterialExpressionTextureSampleParameter2D, -1000, 0, parameter_name="Base", texture=color)
    mk = node(m, unreal.MaterialExpressionTextureSampleParameter2D, -1000, 300, parameter_name="Mascara", texture=mask_tex,
              sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    eyes_p = node(m, unreal.MaterialExpressionScalarParameter, -800, 500, parameter_name="Olhos", default_value=1.5)
    blink = node(m, unreal.MaterialExpressionScalarParameter, -800, 580, parameter_name="Piscar", default_value=0.0)
    spots_p = node(m, unreal.MaterialExpressionScalarParameter, -800, 660, parameter_name="Manchas", default_value=1.2)
    # piscar: os olhos escurecem (a pálpebra é a pele em volta, mais escura)
    eye_blink = node(m, unreal.MaterialExpressionMultiply, -600, 420)
    link(mk, eye_blink, "A", "R")
    link(blink, eye_blink, "B")
    dark = node(m, unreal.MaterialExpressionMultiply, -600, 120, const_b=.18)
    link(base, dark, "A", "RGB")
    col = node(m, unreal.MaterialExpressionLinearInterpolate, -420, 40)
    link(base, col, "A", "RGB")
    link(dark, col, "B")
    link(eye_blink, col, "Alpha")
    mel.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(node(m, unreal.MaterialExpressionConstant, -400, 200, r=.85), "", unreal.MaterialProperty.MP_ROUGHNESS)
    # o brilho: olhos x Olhos x (1 - Piscar) + manchas x Manchas x pulso lento
    one_minus = node(m, unreal.MaterialExpressionOneMinus, -600, 520)
    link(blink, one_minus)
    e1 = node(m, unreal.MaterialExpressionMultiply, -450, 480)
    link(mk, e1, "A", "R")
    link(eyes_p, e1, "B")
    e2 = node(m, unreal.MaterialExpressionMultiply, -320, 500)
    link(e1, e2, "A")
    link(one_minus, e2, "B")
    tm = node(m, unreal.MaterialExpressionTime, -800, 760)
    sp = node(m, unreal.MaterialExpressionMultiply, -680, 760, const_b=1.3)
    link(tm, sp, "A")
    sn = node(m, unreal.MaterialExpressionSine, -560, 760)
    link(sp, sn)
    pulse = node(m, unreal.MaterialExpressionAdd, -440, 760, const_b=1.4)   # 0,4 a 2,4
    link(sn, pulse, "A")
    s1 = node(m, unreal.MaterialExpressionMultiply, -450, 640)
    link(mk, s1, "A", "G")
    link(spots_p, s1, "B")
    s2 = node(m, unreal.MaterialExpressionMultiply, -320, 680)
    link(s1, s2, "A")
    link(pulse, s2, "B")
    glow = node(m, unreal.MaterialExpressionAdd, -180, 560)
    link(e2, glow, "A")
    link(s2, glow, "B")
    lit = node(m, unreal.MaterialExpressionMultiply, -40, 400)
    link(base, lit, "A", "RGB")
    link(glow, lit, "B")
    mel.connect_material_property(lit, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    m.set_editor_property("used_with_skeletal_mesh", True)
    mel.recompile_material(m)
    library.save_loaded_asset(m)
    lines.append("Mushroom: material M_CogumeloCorrompido (olhos e manchas pela máscara)")
    return m


material = None
for species, (glb, glow) in MONSTERS.items():
    if only and species not in only:
        continue
    folder = "/Game/Vortex/Creatures/" + species
    task = unreal.AssetImportTask()
    task.filename = os.path.join(root, glb)
    task.destination_path = folder
    task.automated = True
    task.save = True
    task.replace_existing = True
    tools.import_asset_tasks([task])
    assets = library.list_assets(folder, recursive=True)
    kind = lambda p: str(library.find_asset_data(p).asset_class_path.asset_name)
    meshes = [p for p in assets if kind(p) == "SkeletalMesh"]
    color = next((library.load_asset(p) for p in assets if kind(p) == "Texture2D" and library.load_asset(p).get_editor_property("srgb")), None)
    if color:
        color.set_editor_property("max_texture_size", 2048)
        library.save_loaded_asset(color)
    if material is None and color:
        material = base_material(color)
    mi = None
    if material:
        mi_path = folder + "/MI_%s_Meshy" % species
        mi = library.load_asset(mi_path) if library.does_asset_exist(mi_path) else \
            tools.create_asset("MI_%s_Meshy" % species, folder, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(mi, material)
        if color:
            mel.set_material_instance_texture_parameter_value(mi, "Base", color)
        mel.set_material_instance_scalar_parameter_value(mi, "Glow", glow)
        library.save_loaded_asset(mi)
    # 10/11: o Cogumelo Corrompido tem o rosto vivo (Tools/Blender/AddMushroomJaw.py): olhos e manchas pela máscara
    if species == "Mushroom" and color:
        mi = mushroom_material(folder, color) or mi
    for p in meshes:
        mesh = library.load_asset(p)
        slots = []
        for slot in mesh.get_editor_property("materials"):
            s = unreal.SkeletalMaterial()
            s.set_editor_property("material_interface", mi or slot.get_editor_property("material_interface"))
            s.set_editor_property("material_slot_name", slot.get_editor_property("material_slot_name"))
            slots.append(s)
        mesh.modify()
        mesh.set_editor_property("materials", slots)
        library.save_loaded_asset(mesh)
    clips = sorted(p.split("/")[-1].split(".")[0] for p in assets if kind(p) == "AnimSequence")
    lines.append("%s: malha %s, textura %s, %d clipes %s" % (species, meshes[0].split("/")[-1] if meshes else "NENHUMA", color.get_name() if color else "-", len(clips), clips))

with open(os.path.join(root, "Saved", "Logs", "ImportMeshyMonsters.txt"), "w", encoding="utf-8") as out:
    out.write("\n".join(lines) + "\n")
