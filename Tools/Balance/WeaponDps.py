"""Conta do dano das armas e dos poderes (07/11, o usuário: "veja se os danos e atributos de cada arma estão relevantes
para cada tipo, balanceados"). Só lê os dados e imprime; não muda nada.

A fórmula do jogo (UVortexHeroSheet::OutgoingDamage e UVortexCombatComponent::ApplyHit):
    golpe = (dano do golpe + 1 + Força acima da base + Ataque + nível - 1) x escala do tipo
O dano do golpe é o do DA_<tipo> (1 a 4); os poderes multiplicam só esse dano do golpe ("scale" x o 1º golpe), e o resto
entra somado. A conta usa um herói sem pontos (Força na base), sem raça, e o crítico médio da arma.

python Tools/Balance/WeaponDps.py
"""
import json
import os

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
RANKS = ["common", "uncommon", "rare", "epic", "legendary", "unique", "divine"]
LEVEL = {"common": 1, "uncommon": 5, "rare": 12, "epic": 20, "legendary": 30, "unique": 40, "divine": 50}   # o nível em que cada raridade aparece

# os golpes de cada tipo (DA_<tipo>, lidos no editor em 07/11): (dano, duração em s); escala do tipo (VortexWeaponKit::PrimaryDamageScale)
TYPES = {
    "espada": ([(1, .62), (1, .62), (2, .94)], 1.0, "melee", "após o bônus"),
    "adagas": ([(1, .417), (1, .417), (2, .75)], .61, "melee", "após o bônus"),
    "martelo": ([(2, .8), (2, .8), (4, 1.1)], .62, "melee", "após o bônus"),
    "lança": ([(1, .625), (1, .625), (3, 1.0)], .66, "melee", "após o bônus"),
    "arco": ([(1.5, 1.1)], 1.5, "ranged", "só no dano do golpe"),      # flecha carregada (x1,5) a cada ~1,1 s
    "cajado": ([(1, .667), (1, .667), (3, 1.125)], 1.4, "magic", "só no dano do golpe"),
}
# o que cada tipo ganha de cada raridade (o melhor do arsenal.json; o que falta segue a espada)
ATTACK = {"espada": [2, 3, 4, 6, 9, 11, 14], "adagas": [1, 3, 4, 6, 8, 10, 13], "martelo": [2, 4, 6, 9, 10, 12, 15],
          "lança": [1, 3, 4, 6, 9, 11, 14], "arco": [1, 2, 3, 5, 8, 10, 13], "cajado": [1, 2, 6, 8, 11, 14, 17]}   # cajado: o "skill"
CRIT = {"espada": [(0, 0), (2, 0), (5, 10), (8, 20), (12, 35), (10, 30), (14, 45)], "adagas": [(3, 0), (11, 0), (6, 0), (10, 25), (12, 30), (12, 30), (14, 35)],
        "martelo": [(0, 0), (0, 0), (0, 15), (0, 25), (0, 40), (0, 50), (0, 55)], "lança": [(0, 0)] * 7,
        "arco": [(0, 0), (3, 0), (6, 0), (9, 0), (13, 0), (12, 0), (15, 0)], "cajado": [(0, 0)] * 7}
SPEED = {"adagas": [8, 8, 10, 12, 12, 12, 12], "arco": [0, 4, 6, 8, 10, 10, 12]}


def hit(base, rank, kind, level):
    bonus = 1 + ATTACK[kind][rank] + (level - 1)
    return base, bonus


def normal_dps(kind, rarity):
    rank, level = RANKS.index(rarity), LEVEL[rarity]
    swings, scale, mode, _ = TYPES[kind]
    crit, crit_dmg = CRIT[kind][rank]
    exp = 1 + (.02 + crit / 100) * (.8 + crit_dmg / 100)
    speed = 1 + SPEED.get(kind, [0] * 7)[rank] / 100
    bonus = 1 + ATTACK[kind][rank] + (level - 1)
    total = time = 0.0
    for base, dur in swings:
        if mode == "melee":
            total += (base + bonus) * scale
        else:
            total += base * scale + bonus
        time += dur / speed
    return total / time * exp, bonus


def power_hits(table, rarity_filter=None):
    """O poder em golpes normais de espada (um alvo): quanto ele vale perto do golpe comum do mesmo nível."""
    out = []
    data = json.load(open(os.path.join(ROOT, "Data", "Rpg", table + ".json"), encoding="utf-8"))[table]
    for item, w in data.items():
        p = w["power"]
        kind = p.get("kind")
        if not kind or kind == "heal":
            continue
        v = p.get("values", {})
        rank, level = RANKS.index(w["rarity"]), LEVEL[w["rarity"]]
        weapon = "arco" if table == "bows" else "espada"
        bonus = 1 + ATTACK[weapon][rank] + level - 1
        magic_bonus = 1 + level - 1                     # sem o "skill": a espada e o arco não têm
        s = v.get("scale", 1.0)
        n = {"flurry": v.get("count", 6), "whirl": v.get("hits", 4), "burst3": v.get("count", 3), "orbit": v.get("life", 12) / v.get("every", 1.5),
             "firewall": 6, "arrowrain": max(1, v.get("arrows", 20) // 4)}.get(kind, 1)
        magic = kind in ("tidewave", "runeseal", "overload", "orbit", "firewall", "fireball")
        one = s * (4 if kind == "fireball" else 1) + (magic_bonus if magic else bonus)
        normal = 1 + bonus
        out.append((w["name"], w["rarity"], kind, round(n * one / normal, 1), p.get("cooldown", 0)))
    return out


if __name__ == "__main__":
    print("== dano por segundo do golpe normal (um alvo), no nível em que a raridade aparece")
    print("%-10s" % "raridade" + "".join("%9s" % k for k in TYPES))
    for r in RANKS:
        row = [normal_dps(k, r)[0] for k in TYPES]
        sword = row[0]
        print("%-10s" % r + "".join("%9s" % ("%.0f (%.0f%%)" % (d, d / sword * 100))[:9] for d in row))
    print("\n== poderes: quantos golpes normais de espada valem num alvo (e a recarga)")
    for table in ("swords", "bows"):
        for name, rarity, kind, hits, cd in power_hits(table):
            print("  %-28s %-10s %-10s %5.1f golpes  %4.1f s" % (name, rarity, kind, hits, cd))
