"""Lista os materiais das armas importadas (diagnóstico): Saved/Logs/WeaponMaterials.txt."""
import os
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
library = unreal.EditorAssetLibrary
lines = []
for name in ("W_espada_escudo", "W_espada_escudo_L", "W_martelo_de_bram"):
    mesh = library.load_asset("/Game/Vortex/Weapons/Models/" + name)
    if not mesh:
        lines.append(name + " NAO EXISTE")
        continue
    for i, m in enumerate(mesh.get_editor_property("static_materials")):
        mi = m.get_editor_property("material_interface")
        info = mi.get_path_name() if mi else "None"
        if isinstance(mi, unreal.MaterialInstance):
            parent = mi.get_editor_property("parent")
            info += " parent=" + (parent.get_path_name() if parent else "None")
            for tp in mi.get_editor_property("texture_parameter_values"):
                tex = tp.get_editor_property("parameter_value")
                info += " tex=" + (tex.get_name() if tex else "None")
        lines.append("%s[%d] %s %s" % (name, i, m.get_editor_property("material_slot_name"), info))
da = library.load_asset("/Game/Vortex/Weapons/Types/DA_espada_escudo")
lines.append("left_mesh=" + str(da.get_editor_property("left_mesh")))
assets = library.list_assets("/Game/Vortex/Weapons/Models", recursive=True)
lines.append("assets na pasta: %d (%s)" % (len(assets), ", ".join(a.split("/")[-1] for a in assets[:30])))
with open(os.path.join(root, "Saved", "Logs", "WeaponMaterials.txt"), "w", encoding="utf-8") as out:
    out.write("\n".join(lines) + "\n")
