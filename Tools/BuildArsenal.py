"""O arsenal do Vortex por cima dos itens do Godot: Data/Rpg/arsenal.json.

python Tools/BuildArsenal.py

- Toda arma ganha preço (pela raridade) e, quando o nome pede, um elemento (Malho de Geada = gelo,
  Machado do Trovão = raio, Colossal de Brasa = fogo, Cajado da Maré = água, Orbe do Vazio = sombra...).
- Variantes novas: grimórios, orbes, varinhas e cajados de cada elemento (água, raio, gelo, natureza,
  sombra, fogo, luz) e, nas armas de perto e de distância, versões com fogo, gelo, raio, veneno,
  sangramento e roubo de vida. Dano e atributos crescem com a raridade.
- Armaduras em conjuntos de 4 peças (cabeça, peito, pernas, pés) com o visual no herói (peça do
  guarda-roupa + cores) e bônus de 2 e 4 peças. Os 4 conjuntos do Godot ganham visual e bônus; 5 novos.
O jogo lê este arquivo depois do rpg.json (FVortexRpgCatalog): "patch" completa itens que já existem,
"items" acrescenta, "sets" são os bônus de conjunto.
"""
import json
import os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RPG = json.load(open(os.path.join(ROOT, "Data", "Rpg", "rpg.json"), encoding="utf-8"))
ITEMS = RPG["items"]
ORDER = ["common", "uncommon", "rare", "epic", "legendary", "unique"]
PRICE = {"common": 30, "uncommon": 80, "rare": 200, "epic": 520, "legendary": 1300, "unique": 3200}
MULT = {"common": 1.0, "uncommon": 1.5, "rare": 2.2, "epic": 3.2, "legendary": 4.5, "unique": 6.0}

# elemento: nome, sufixo do nome (feminino/masculino não importa: "da/do" já vem no sufixo), o que faz
ELEMENTS = {
    "fire": ("Fogo", "das Chamas", "Queima o alvo: dano de fogo por 3 s."),
    "water": ("Água", "das Marés", "Encharca o alvo: 25% mais lento por 2 s."),
    "lightning": ("Raio", "do Trovão", "O raio salta para até 2 inimigos perto (40% do dano)."),
    "ice": ("Gelo", "da Geada", "Congela aos poucos: 35% mais lento por 2 s."),
    "nature": ("Natureza", "Verdejante", "Vinhas prendem o alvo por meio segundo (1 em cada 4 golpes)."),
    "poison": ("Veneno", "da Peçonha", "Envenena: dano por 4 s."),
    "holy": ("Luz", "Sagrado", "+50% de dano em mortos-vivos e cura 2% do dano causado."),
    "shadow": ("Sombra", "das Sombras", "Rouba 8% do dano como vida."),
}
TRAITS = {   # nome da peculiaridade, o que faz, sufixo do nome da arma
    "bleed": ("Sangramento", "Abre feridas: dano por 3 s.", "de Serrilha"),
    "vampiric": ("Vampirismo", "Rouba 6% do dano causado como vida.", "do Sangue"),
    "heavy": ("Peso", "1 em cada 5 golpes atordoa por meio segundo.", "de Peso"),
    "keen": ("Fio fino", "+8% de chance de crítico.", "de Fio Fino"),
}
KEYWORDS = [  # palavra no nome -> elemento
    ("Geada", "ice"), ("Gelo", "ice"), ("Inverno", "ice"), ("Glacial", "ice"), ("Frio", "ice"),
    ("Trovão", "lightning"), ("Tempestade", "lightning"), ("Raio", "lightning"),
    ("Brasa", "fire"), ("Fênix", "fire"), ("Meteoro", "fire"), ("Chama", "fire"), ("Inferno", "fire"), ("Rubro", "fire"), ("Rubra", "fire"),
    ("Maré", "water"), ("Névoa", "water"),
    ("Vazio", "shadow"), ("Crepúsculo", "shadow"), ("Eclipse", "shadow"), ("Noite", "shadow"), ("Sombra", "shadow"), ("Abismo", "shadow"), ("Almas", "shadow"), ("Lua", "shadow"),
    ("Sol", "holy"), ("Solar", "holy"), ("Aurora", "holy"), ("Consagrada", "holy"), ("Mártires", "holy"), ("Manhã", "holy"),
    ("Víbora", "poison"), ("Serpente", "poison"), ("Venenoso", "poison"), ("Espinhos", "poison"),
    ("Jade", "nature"), ("Folha", "nature"), ("Carvalho", "nature"), ("Salgueiro", "nature"),
]
NOUN = {"espada_de_cinzas": "Espada", "espadao": "Espadão", "machado": "Machado", "machados_duplos": "Machados", "martelo_de_bram": "Martelo",
        "lanca_do_patrulheiro": "Lança", "alabarda": "Alabarda", "adagas_gemeas": "Adagas", "manoplas_de_ferro": "Manoplas", "maca_sagrada": "Maça",
        "katana_do_vento": "Katana", "arco_de_betula": "Arco", "besta": "Besta", "pistola_runica": "Pistola", "chakrams": "Chakrams",
        "cajado_de_ilsa": "Cajado", "varinha": "Varinha", "livro_da_aurora": "Grimório", "orbe": "Orbe", "instrumento": "Alaúde",
        "foice": "Foice", "chicote": "Chicote", "correntes": "Correntes", "catalisador": "Catalisador"}
MAGIC = ["livro_da_aurora", "orbe", "varinha", "cajado_de_ilsa"]


def base_of(family):
    pool = [(k, v) for k, v in ITEMS.items() if v.get("family") == family]
    pool.sort(key=lambda kv: ORDER.index(kv[1]["rarity"]))
    return pool[0] if pool else (None, None)


def scaled(stats, rarity):
    m = MULT[rarity]
    out = {}
    for k, v in stats.items():
        out[k] = round(v * m) if isinstance(v, int) or float(v).is_integer() else round(v * m, 1)
        if k in ("speed", "crit"):
            out[k] = v   # velocidade e crítico não escalam com a raridade
    out["attack"] = max(1, round(stats.get("attack", 1) * m))
    return out


def element_of(name):
    for word, element in KEYWORDS:
        if word in name:
            return element
    return ""


def describe(element, trait):
    lines = []
    if element:
        lines.append("%s: %s" % (ELEMENTS[element][0], ELEMENTS[element][2]))
    if trait:
        lines.append("%s: %s" % (TRAITS[trait][0], TRAITS[trait][1]))
    return " ".join(lines)


patch, items = {}, {}
# ---- armas que já existem: preço e o elemento do nome
for key, v in ITEMS.items():
    if v.get("type") != "weapon":
        continue
    element = element_of(v["name"])
    p = {"price": PRICE.get(v["rarity"], 50)}
    if element:
        p["element"] = element
        p["effect_text"] = describe(element, "")
    patch[key] = p

# ---- variantes novas
for family, noun in NOUN.items():
    base_key, base = base_of(family)
    if not base:
        continue
    if family in MAGIC:
        plan = [(e, "", r) for e in ["water", "lightning", "ice", "nature", "shadow", "fire", "holy"] for r in ("rare", "epic")]
        plan += [("lightning", "", "legendary"), ("water", "", "legendary")]
    elif family in ("instrumento", "catalisador"):
        plan = [("water", "", "rare"), ("lightning", "", "epic"), ("poison", "", "rare"), ("holy", "", "epic")]
    else:
        plan = [("fire", "", "rare"), ("ice", "", "rare"), ("lightning", "", "epic"), ("poison", "bleed", "rare"),
                ("shadow", "vampiric", "epic"), ("", "heavy" if family in ("martelo_de_bram", "maca_sagrada", "machado", "espadao") else "keen", "uncommon")]
    for element, trait, rarity in plan:
        key = "%s_%s%s_%s" % (family, element or "", ("_" + trait) if trait else "", rarity)
        if element:
            name = "%s %s" % (noun, ELEMENTS[element][1])
        else:
            name = "%s %s" % (noun, TRAITS[trait][2])
        if rarity == "legendary":
            name += " Ancestral"
        elif rarity == "epic":
            name += " Superior"
        # a arma do Godot da mesma família e raridade dá a base (um pouco mais forte pelo elemento)
        same = [v for v in ITEMS.values() if v.get("family") == family and v["rarity"] == rarity]
        stats = dict(same[0]["stats"]) if same else scaled(base["stats"], rarity)
        stats["attack"] = round(stats.get("attack", 1) * (1.15 if element else 1.05) + .49)
        if trait == "keen":
            stats["crit"] = stats.get("crit", 0) + 8
        # sombra e vampirismo: roubo de vida (o jogo já soma "lifesteal" nos atributos)
        if element == "shadow":
            stats["lifesteal"] = stats.get("lifesteal", 0) + 8
        if trait == "vampiric":
            stats["lifesteal"] = stats.get("lifesteal", 0) + 6
        if family in MAGIC:
            stats["skill"] = max(stats.get("skill", 0), round(2.5 * MULT[rarity]))
            stats["max_mana"] = max(stats.get("max_mana", 0), round(12 * MULT[rarity]))
        items[key] = {
            "name": name,
            "description": "%s de %s. %s" % (noun, "raridade " + {"uncommon": "incomum", "rare": "rara", "epic": "épica", "legendary": "lendária"}[rarity],
                                            describe(element, trait)),
            "rarity": rarity, "type": "weapon", "slot": "weapon", "icon": base["icon"], "stack": 1,
            "stats": stats, "use": {}, "family": family,
            "element": element, "trait": trait, "price": PRICE[rarity], "effect_text": describe(element, trait),
        }

# ---- armaduras: conjuntos com visual (peça do guarda-roupa por parte) e bônus
SETS = {
    "lobo": {"name": "Couro de Lobo", "rarity": "uncommon", "look": {"head": "accessory_bandana", "shirt": "shirt_hunter", "pants": "pants_hunter", "shoes": "boots_hunter"},
             "colors": {"shirt_color": "6b5a48", "pants_color": "4a3c30", "boots_color": "3a2c22"},
             "bonus2": {"haste": 5}, "bonus4": {"crit": 5, "speed": 6}, "text": "Caçador: 2 peças +5% de pressa; 4 peças +5% de crítico e mais rápido.",
             "existing": {"head": "wolf_head", "chest": "wolf_chest", "legs": "wolf_legs", "boots": "wolf_boots"}},
    "guarda": {"name": "Guarda da Vila", "rarity": "uncommon", "look": {"head": "accessory_hat", "shirt": "shirt_mail", "pants": "pants_leather", "shoes": "boots_laced"},
               "colors": {"shirt_color": "8a9bb0", "pants_color": "34405a", "boots_color": "3a3026"},
               "bonus2": {"defense": 3}, "bonus4": {"resistance": 10, "max_health": 25}, "text": "Sentinela: 2 peças +3 de defesa; 4 peças +10 de resistência e +25 de vida.",
               "existing": {"head": "guard_head", "chest": "guard_chest", "legs": "guard_legs", "boots": "guard_boots"}},
    "casca": {"name": "Casca Viva", "rarity": "rare", "look": {"head": "accessory_circlet", "shirt": "shirt_monk", "pants": "pants_monk", "shoes": "boots_monk"},
              "colors": {"shirt_color": "4f6b34", "pants_color": "5a4128", "boots_color": "3b2a18"},
              "bonus2": {"hp_regen": 4}, "bonus4": {"hp_regen": 8, "max_mana": 25}, "text": "Floresta: 2 peças regeneram vida; 4 peças regeneram mais e +25 de mana.",
              "existing": {"head": "bark_head", "chest": "bark_chest", "legs": "bark_legs", "boots": "bark_boots"}},
    "cratera": {"name": "Cratera", "rarity": "epic", "look": {"head": "accessory_horns", "shirt": "shirt_winter", "pants": "pants_shadow", "shoes": "boots_winter"},
                "colors": {"shirt_color": "5a2a24", "pants_color": "2a2222", "boots_color": "6b2a1a"},
                "bonus2": {"resistance": 10}, "bonus4": {"defense": 6, "max_health": 50}, "text": "Lava: 2 peças +10 de resistência; 4 peças +6 de defesa e +50 de vida.",
                "existing": {"head": "crater_head", "chest": "crater_chest", "legs": "crater_legs", "boots": "crater_boots"}},
    "dunas": {"name": "Andarilho das Dunas", "rarity": "rare", "look": {"head": "accessory_goggles", "shirt": "shirt_nomad", "pants": "pants_nomad", "shoes": "boots_nomad"},
              "colors": {"shirt_color": "c9a76a", "pants_color": "8a6a3e", "boots_color": "5a4026"},
              "bonus2": {"speed": 6}, "bonus4": {"haste": 10, "resistance": 8}, "text": "Deserto: 2 peças mais rápido; 4 peças +10% de pressa e +8 de resistência.",
              "pieces": {"head": ("Óculos do Andarilho", {"defense": 2, "resistance": 3}), "chest": ("Manto das Dunas", {"defense": 3, "resistance": 6, "speed": 3}),
                         "legs": ("Calças de Viagem", {"defense": 2, "speed": 3}), "boots": ("Botas de Areia", {"defense": 2, "speed": 4})}},
    "sombra": {"name": "Sombra Ligeira", "rarity": "rare", "look": {"head": "accessory_mask", "shirt": "shirt_shadow", "pants": "pants_shadow", "shoes": "boots_shadow"},
               "colors": {"shirt_color": "2b2433", "pants_color": "1d1a24", "boots_color": "181520"},
               "bonus2": {"crit": 6}, "bonus4": {"crit_dmg": 25, "haste": 8}, "text": "Assassino: 2 peças +6% de crítico; 4 peças críticos +25% mais fortes e +8% de pressa.",
               "pieces": {"head": ("Máscara da Sombra", {"defense": 1, "crit": 3}), "chest": ("Gibão da Sombra", {"defense": 3, "crit": 3, "haste": 4}),
                          "legs": ("Calças da Sombra", {"defense": 2, "crit": 2}), "boots": ("Botas Silenciosas", {"defense": 1, "speed": 5})}},
    "sabio": {"name": "Sábio Rúnico", "rarity": "epic", "look": {"head": "accessory_circlet", "shirt": "shirt_robe", "pants": "pants_cloth", "shoes": "boots_healer"},
              "colors": {"shirt_color": "2d3f8a", "pants_color": "1f2a5a", "boots_color": "c9a24a"},
              "bonus2": {"max_mana": 30, "skill": 4}, "bonus4": {"skill": 10, "haste": 12}, "text": "Arcano: 2 peças +30 de mana e poder de magia; 4 peças muito mais poder e recargas menores.",
              "pieces": {"head": ("Diadema Rúnico", {"defense": 2, "max_mana": 20, "skill": 3}), "chest": ("Túnica Rúnica", {"defense": 3, "max_mana": 30, "skill": 4, "resistance": 8}),
                         "legs": ("Saiote Rúnico", {"defense": 2, "max_mana": 15, "resistance": 5}), "boots": ("Sandálias Rúnicas", {"defense": 2, "max_mana": 10, "speed": 3})}},
    "azhur": {"name": "Placas de Azhûr", "rarity": "legendary", "look": {"head": "accessory_crown", "shirt": "shirt_royal", "pants": "pants_royal", "shoes": "boots_royal"},
              "colors": {"shirt_color": "8a1f1a", "pants_color": "3a1414", "boots_color": "c99a34"},
              "bonus2": {"defense": 8, "max_health": 40}, "bonus4": {"resistance": 25, "hp_regen": 6, "attack": 6}, "text": "Colosso: 2 peças muita defesa e vida; 4 peças resistência, regeneração e ataque.",
              "pieces": {"head": ("Elmo de Azhûr", {"defense": 6, "max_health": 30, "resistance": 10}), "chest": ("Couraça de Azhûr", {"defense": 9, "max_health": 50, "resistance": 16}),
                         "legs": ("Grevas de Azhûr", {"defense": 6, "max_health": 25, "resistance": 10}), "boots": ("Botas de Azhûr", {"defense": 5, "max_health": 20, "speed": 2})}},
    "yggdrasil": {"name": "Guardião de Yggdrasil", "rarity": "unique", "look": {"head": "accessory_flower", "shirt": "shirt_healer", "pants": "pants_healer", "shoes": "boots_healer"},
                  "colors": {"shirt_color": "e8e2c8", "pants_color": "6a8a4a", "boots_color": "c9b06a"},
                  "bonus2": {"hp_regen": 12, "resistance": 15}, "bonus4": {"max_health": 80, "max_mana": 40, "haste": 15}, "text": "Árvore do mundo: 2 peças regeneração e resistência; 4 peças vida, mana e pressa.",
                  "pieces": {"head": ("Coroa de Folhas Eternas", {"defense": 6, "hp_regen": 6, "resistance": 12}), "chest": ("Manto de Yggdrasil", {"defense": 10, "max_health": 60, "resistance": 20}),
                             "legs": ("Calças de Raiz Antiga", {"defense": 7, "max_health": 40, "resistance": 12}), "boots": ("Botas do Caminho Verde", {"defense": 6, "speed": 5, "hp_regen": 4})}},
}
SLOT_LOOK = {"head": "head", "chest": "shirt", "legs": "pants", "boots": "shoes"}
sets_out = {}
for set_id, s in SETS.items():
    pieces = []
    if "existing" in s:
        for slot, key in s["existing"].items():
            look_key = SLOT_LOOK[slot]
            patch.setdefault(key, {}).update({"set": set_id, "price": PRICE[ITEMS[key]["rarity"]],
                                              "look": {look_key: s["look"][look_key]}, "look_colors": s["colors"]})
            pieces.append(key)
    else:
        for slot, (name, stats) in s["pieces"].items():
            key = "%s_%s" % (set_id, slot)
            look_key = SLOT_LOOK[slot]
            items[key] = {"name": name, "description": "Peça do conjunto %s. %s" % (s["name"], s["text"]), "rarity": s["rarity"], "type": "armor",
                          "slot": slot, "icon": ITEMS.get("%s_%s" % ("bark", slot), {}).get("icon", "helm_iron"), "stack": 1, "stats": stats, "use": {},
                          "family": "", "price": PRICE[s["rarity"]], "set": set_id, "look": {look_key: s["look"][look_key]}, "look_colors": s["colors"]}
            pieces.append(key)
    sets_out[set_id] = {"name": s["name"], "pieces": pieces, "bonus2": s["bonus2"], "bonus4": s["bonus4"], "text": s["text"]}

# peças soltas do Godot também aparecem no corpo
for key, look in {"helm_leather": {"head": "accessory_bandana"}, "helm_iron": {"head": "accessory_hat"}, "chest_tunic": {"shirt": "shirt_tunic"},
                  "chest_mail": {"shirt": "shirt_mail"}, "chest_punk": {"shirt": "shirt_punk"}, "legs_cloth": {"pants": "pants_cloth"},
                  "legs_leather": {"pants": "pants_leather"}, "boots_leather": {"shoes": "boots_laced"}, "boots_iron": {"shoes": "boots_winter"}}.items():
    if key in ITEMS:
        patch.setdefault(key, {}).update({"look": look, "price": PRICE[ITEMS[key]["rarity"]]})
for key, v in ITEMS.items():
    if v.get("type") in ("material", "consumable", "accessory", "food") and key not in patch:
        patch[key] = {"price": max(4, PRICE.get(v["rarity"], 20) // (5 if v.get("type") == "material" else 2))}

# 07/11: as 35 espadas das pranchas do usuário (Tools/Weapons/BuildSwords.py -> Data/Rpg/swords.json): as que já existiam
# ganham o nome, o ícone e o elemento; as outras entram como itens novos. O poder do botão direito vai no texto do item
# (o jogo usa o swords.json para o modelo e o poder). As divinas só caem do Colosso de Runas e do Golem ("loot").
SLOT_NAME = {"head": "Capacete", "chest": "Peitoral", "legs": "Calças", "boots": "Botas"}
STAT_NAME = {"attack": "de ataque", "defense": "de defesa", "resistance": "de resistência", "max_health": "de vida", "max_mana": "de mana",
             "crit": "% de crítico", "crit_dmg": "% de dano crítico", "speed": "% de velocidade de ataque", "haste": "% de recarga",
             "skill": "de poder de magia", "lifesteal": "% de roubo de vida", "hp_regen": "de regeneração"}
RARITY_NAME = {"common": "comum", "uncommon": "incomum", "rare": "rara", "epic": "épica", "legendary": "lendária", "unique": "única", "divine": "divina"}
loot = {}
# 07/11: e os 35 arcos (Tools/Weapons/BuildBows.py -> Data/Rpg/bows.json), do mesmo jeito
for table, family, kind_name in (("swords", "espada_de_cinzas", "Espada"), ("bows", "arco_de_betula", "Arco")):
    table_path = os.path.join(ROOT, "Data", "Rpg", table + ".json")
    if not os.path.exists(table_path):
        continue
    for item, sw in json.load(open(table_path, encoding="utf-8"))[table].items():
        power = "Botão direito — %s: %s" % (sw["power"]["name"], sw["power"]["text"])
        element = sw["element"]
        effect = ("%s: %s" % (ELEMENTS[element][0], ELEMENTS[element][2])) if element else ""
        fields = {"name": sw["name"], "icon": sw["icon"], "price": sw["price"], "element": element, "effect_text": effect}
        if item in ITEMS:
            base = ITEMS[item].get("description", "")
            patch.setdefault(item, {}).update(dict(fields, description=(base + " " if base else "") + power))
        else:
            items[item] = dict(fields, description="%s %s. %s" % (kind_name, RARITY_NAME[sw["rarity"]], power), rarity=sw["rarity"],
                               type="weapon", slot="weapon", stack=1, stats=sw["stats"], use={}, family=family, trait="")
        if sw["rarity"] == "divine":
            for species in ("runecolossus", "golem"):
                loot.setdefault(species, []).append({"item": item, "min": 1, "max": 1, "chance": .02})

# 07/11: as 21 armaduras das pranchas do usuário (Tools/Armor/BuildArmors.py -> Data/Rpg/armors.json): 84 peças e os
# conjuntos; a passiva vai no texto do item e o jogo a lê do armors.json (VortexArmor.cpp)
armors_path = os.path.join(ROOT, "Data", "Rpg", "armors.json")
if os.path.exists(armors_path):
    armors = json.load(open(armors_path, encoding="utf-8"))
    for item, p in armors["pieces"].items():
        aset = armors["sets"][p["set"]]
        text = "%s do conjunto %s. Passiva — %s: %s." % (SLOT_NAME[p["slot"]], aset["name"], p["passive"]["name"], p["passive"]["text"][0].upper() + p["passive"]["text"][1:])
        items[item] = {"name": p["name"], "description": text, "rarity": p["rarity"], "type": "armor", "slot": p["slot"], "icon": p["icon"],
                       "stack": 1, "stats": p["stats"], "use": {}, "family": "", "price": p["price"], "set": p["set"],
                       "element": p["element"], "effect_text": "", "trait": ""}
    for key, aset in armors["sets"].items():
        two = ", ".join("+%s %s" % (v, STAT_NAME.get(k, k)) for k, v in aset["bonus2"].items())
        p4 = aset["passive4"]
        sets_out[key] = {"name": aset["name"], "pieces": [aset["pieces"][s] for s in ("head", "chest", "legs", "boots")], "bonus2": aset["bonus2"],
                         "bonus4": {}, "text": "2 peças: %s. 4 peças — %s: %s." % (two, p4["name"], p4["text"])}
    for species, entries in armors["loot"].items():
        loot.setdefault(species, []).extend(entries)

out = {"_doc": "Gerado por Tools/BuildArsenal.py (por cima do rpg.json). Não editar à mão.",
       "elements": {k: {"name": v[0], "text": v[2]} for k, v in ELEMENTS.items()},
       "traits": {k: {"name": v[0], "text": v[1]} for k, v in TRAITS.items()},
       "patch": patch, "items": items, "sets": sets_out, "loot": loot}
path = os.path.join(ROOT, "Data", "Rpg", "arsenal.json")
open(path, "w", encoding="utf-8").write(json.dumps(out, ensure_ascii=False, indent=1) + "\n")
print("ARSENAL itens_novos=%d remendos=%d conjuntos=%d" % (len(items), len(patch), len(sets_out)))
