"""Tira o Nanite das malhas do jogo (as malhas passam a ser desenhadas inteiras, do jeito normal).

Por quê: o Nanite foi feito para modelos de milhões de triângulos; as plantas, pedras e peças do The World
Gêneses são de cubos, com poucos triângulos, e o custo fixo do Nanite (montar, cortar, sombras) comia ~25%
do quadro: no passeio a 1080p, 86 FPS com Nanite e 114 sem. Rodar com o editor fechado:
  UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<este arquivo> -unattended -nullrhi
Com "-- listar" só conta, sem mudar nada.
"""
import sys
import unreal

only_list = "listar" in " ".join(sys.argv)
registry = unreal.AssetRegistryHelpers.get_asset_registry()
flt = unreal.ARFilter(class_paths=[unreal.TopLevelAssetPath("/Script/Engine", "StaticMesh")], package_paths=["/Game"], recursive_paths=True)
assets = registry.get_assets(flt)
changed = 0
nanite = 0
biggest = []
for data in assets:
    mesh = data.get_asset()
    if not mesh:
        continue
    settings = mesh.get_editor_property("nanite_settings")
    if not settings.enabled:
        continue
    nanite += 1
    tris = unreal.EditorStaticMeshLibrary.get_number_verts(mesh, 0)
    biggest.append((tris, str(data.package_name)))
    if only_list:
        continue
    settings.enabled = False
    mesh.set_editor_property("nanite_settings", settings)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh, False)
    changed += 1
biggest.sort(reverse=True)
unreal.log_warning(f"VORTEX_NANITE malhas={len(assets)} com_nanite={nanite} desligadas={changed}")
for tris, name in biggest[:12]:
    unreal.log_warning(f"VORTEX_NANITE   {name}: {tris} vértices")
