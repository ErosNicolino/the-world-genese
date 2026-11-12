"""A conta nova (07/11): o golpe da arma multiplica e o item tem nível. Só lê o balance.json e imprime.

golpe = dano do golpe x (1 + 1 + Ataque do item + nível do herói - 1) x escala do tipo
Ataque do item = (attackBase + attackPerLevel x nível do item) x fator da arma (balance.json, itemLevel)

python Tools/Balance/WeaponLevels.py
"""
import json
import os

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
B = json.load(open(os.path.join(ROOT, "Data", "Rpg", "balance.json"), encoding="utf-8"))
IL, SC = B["itemLevel"], B["weaponDamageScale"]
RANKS = ["common", "uncommon", "rare", "epic", "legendary", "unique", "divine"]
TYPES = {"espada_de_cinzas": [(1, .62), (1, .62), (2, .94)], "adagas_gemeas": [(1, .417 / 1.12), (1, .417 / 1.12), (2, .75 / 1.12)],
         "martelo_de_bram": [(2, .8), (2, .8), (4, 1.1)], "lanca_do_patrulheiro": [(1, .625), (1, .625), (3, 1.0)],
         "arco_de_betula": [(1.5, 1.1 / 1.1)], "cajado_de_ilsa": [(1, .667), (1, .667), (3, 1.125)]}


def attack(rank, level, factor=1.0):
    return round((IL["attackBase"][rank] + IL["attackPerLevel"][rank] * level) * factor)


def hit(base, rank, item_level, hero_level, scale):
    return base * (1 + 1 + attack(rank, item_level) + hero_level - 1) * scale


def dps(kind, rank, level):
    swings = TYPES[kind]
    return sum(hit(b, rank, level, level, SC[kind]) for b, _ in swings) / sum(d for _, d in swings)


def monster_hits(level):
    return B["monsterHitBase"] + B["monsterHitPerLevel"] * (level - 1)


if __name__ == "__main__":
    print("== golpe leve da espada (dano 1) com o item do mesmo nível do herói")
    print("nível " + "".join("%10s" % r for r in RANKS) + "   'golpe' dos bichos")
    for L in (1, 10, 20, 30, 50, 78, 100):
        print("%5d " % L + "".join("%10.0f" % hit(1, r, L, L, 1.0) for r in range(7)) + "   %8.1f" % monster_hits(L))
    print("\n== dano por segundo de cada tipo (raro, item = nível do herói), perto da espada")
    for L in (10, 50, 100):
        sword = dps("espada_de_cinzas", 2, L)
        print("  nível %3d: " % L + "  ".join("%s %.0f%%" % (k.split("_")[0], dps(k, 2, L) / sword * 100) for k in TYPES))
    print("\n== 10 níveis de item a mais (lendária, herói no 50): o golpe sobe")
    for il in (30, 40, 50, 60):
        print("  item nível %d: %.0f" % (il, hit(1, 4, il, 50, 1.0)))
