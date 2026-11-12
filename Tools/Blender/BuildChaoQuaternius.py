"""O chão e os enfeites da vila com o kit da Quaternius (11/11, o usuário: "o chão faça um que combine com a vila, as texturas
também, os caminhos certinhos, os postes"): as mesmas peças e os mesmos nomes do kit antigo (Tools/Blender/BuildVilaKit.py),
para o Tools/Town/BuildVila.py não mudar.

- vila_rua_a..d: a placa de 3 x 3 m de pedra irregular (Floor_UnevenBrick); iguais e sem giro, para a textura emendar;
- vila_laje_a/b: a placa das praças, de pedra cortada (Floor_Brick);
- vila_borda_a..c: o meio-fio de pedra na beira da rua que dá para a grama (antes uma faixa de pedras soltas);
- vila_poste: poste de madeira com o lampião do kit; vila_banco, vila_caixotes e vila_cerca com as peças do kit.
A terra (vila_terra_*), a floreira, o poço, as barracas e a carroça continuam os de antes (o kit não tem).

Sem janela (exporta):  blender -b --factory-startup -P Tools/Blender/BuildChaoQuaternius.py -- exportar
No Blender aberto:     exec(open(r"<raiz>/Tools/Blender/BuildChaoQuaternius.py").read())
"""
import importlib
import json
import sys

sys.path.insert(0, r"C:\Users\Eros Nicolino\Documents\4 - Projetos\Jogos e 3D\Vortex-UE5-migracao\Vortex\Tools\Blender")
import quaternius_kit as qk  # noqa: E402

importlib.reload(qk)
EXPORT = "exportar" in sys.argv
built = []


def piece(name, lamps=None):
    b = qk.Build(name)
    built.append((b, lamps))
    return b


# a placa do kit é só a face de cima: por baixo vai um bloco da cor da pedra, para a beira não ficar no ar onde o chão desce
for names, floor in ((("vila_rua_a", "vila_rua_b", "vila_rua_c", "vila_rua_d"), "Floor_UnevenBrick"), (("vila_laje_a", "vila_laje_b"), "Floor_Brick")):
    for name in names:
        b = piece(name)
        b.put(floor, 0, 0, .04, scale=(1.5, 1.5, 1))
        b.cube("Base", "Q_" + floor[6:], -1.5, -1.5, -.15, 1.5, 1.5, .045)
piece("vila_rua_p").put("Floor_UnevenBrick", 0, 0, .05, scale=(.6, .75, 1))      # o caminho da porta da casa até a rua (1,2 x 1,5 m)
for name, curb in (("vila_borda_a", "Prop_ExteriorBorder_Straight1"), ("vila_borda_b", "Prop_ExteriorBorder_Straight2"), ("vila_borda_c", "Prop_ExteriorBorder_Straight1")):
    piece(name).put(curb, 0, 0, 0, rot=180, scale=(1.5, 1, 1))                # de y = 0 (a beira da placa) a 0,7 m para fora

b = piece("vila_poste", lamps=[[0, 2.0, -1.2]])
b.put("Corner_ExteriorWide_Wood", 0, 0)
b.put("Prop_ExteriorBorder_Corner", .35, .35, 0, scale=(1, 1, 2.2))           # a base de pedra
b.put("Lantern_Wall", 0, .17, 1.55, rot=180)
b.cube("Luz", "Lanterna", -.05, 1.19, 1.8, .05, 1.29, 1.94)                  # o vidro que acende à noite (MI_Vila_Lanterna)
b.box(-.2, -.2, 0, .2, .2, 3.0)

b = piece("vila_banco")
b.put("Bench", 0, 0)
b.box(-1.39, -.27, 0, 1.39, .26, .53)

b = piece("vila_caixotes")
b.put("Crate_Wooden", -.3, -.1, 0, rot=8)
b.put("Barrel", .8, -.15)
b.put("FarmCrate_Apple", -.3, -.1, .88, rot=20)
b.box(-.8, -.6, 0, 1.2, .35, .9)

b = piece("vila_cerca")
for x in (-1.88, 0, 1.88):
    b.put("Prop_WoodenFence_Single", x, 0, scale=(.913, 1, 1.3))
b.box(-2.82, -.07, 0, 2.82, .07, 1.0)

old = json.load(open(qk.INFO, encoding="utf-8"))
for n, (b, lamps) in enumerate(built):
    if EXPORT:
        info = {k: v for k, v in old.get(b.name, old["vila_rua_a"]).items() if k != "boxes"}
        if lamps:
            info["lamps"] = lamps
        b.export(info)
    else:
        b.shift(n * 4 - 24)
