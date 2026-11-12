"""As 5 armas do Meshy com as normais do próprio arquivo (suaves) e o UV em precisão total — no editor completo
(no modo de comando, -run=pythonscript, as funções de malha da Unreal não fazem nada).

UnrealEditor.exe Vortex.uproject -ExecutePythonScript=<caminho completo>/Tools/FixWeaponNormals.py -unattended

A Unreal recalculava uma normal por triângulo, sem suavizar: nas milhares de faces pequenas a arma ficava facetada,
escura e manchada, com cara de pedra. Relatório em Saved/Logs/FixWeaponNormals.txt; fecha o editor no fim.
"""
import unreal

library = unreal.EditorAssetLibrary
meshes = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
NAMES = ["W_espada_de_cinzas", "W_martelo_de_bram", "W_adagas_gemeas", "W_adagas_gemeas_L", "W_cajado_de_ilsa",
         "W_arco_de_betula", "W_arco_ametista", "W_flecha"]
lines = []
for name in NAMES:
    path = next((p for p in library.list_assets("/Game/Vortex/Weapons/Models", recursive=True)
                 if p.split("/")[-1].split(".")[0] == name and isinstance(library.load_asset(p), unreal.StaticMesh)), None)
    if not path:
        lines.append("sem " + name)
        continue
    mesh = library.load_asset(path)
    build = meshes.get_lod_build_settings(mesh, 0)
    before = build.get_editor_property("recompute_normals")
    build.set_editor_property("recompute_normals", False)
    build.set_editor_property("recompute_tangents", True)
    build.set_editor_property("use_full_precision_u_vs", True)
    build.set_editor_property("use_high_precision_tangent_basis", True)
    meshes.set_lod_build_settings(mesh, 0, build)
    after = meshes.get_lod_build_settings(mesh, 0).get_editor_property("recompute_normals")
    library.save_loaded_asset(mesh, False)
    lines.append("%s recalcular normais %s -> %s" % (name, before, after))
with open(unreal.Paths.project_saved_dir() + "Logs/FixWeaponNormals.txt", "w", encoding="utf-8") as out:
    out.write("\n".join(lines) + "\n")
unreal.SystemLibrary.quit_editor()
