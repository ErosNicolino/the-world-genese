"""Dados de RPG do Vortex a partir do Godot (somente leitura): Data/Rpg/rpg.json.

python Tools/BuildRpgData.py
python Tools/BuildRpgData.py --icons   (só troca os ícones das habilidades e classes no rpg.json atual)

- itens (data/items.json): nome, descrição, raridade, tipo, lugar no corpo, ícone, pilha, atributos
  (attack, defense, max_health...) e uso (heal, mana);
- saque por criatura e a mochila inicial;
- progressão (data/progression.json): XP por criatura e os efeitos dos atributos;
- raças (Systems/Player/RaceCatalog.cs): multiplicadores de combate;
- técnicas de cada arma inicial (Systems/Player/StarterWeaponCatalog.cs) e os 24 tipos de arma.
Depois rode Tools/ImportItemIcons.py (com o editor fechado) para trazer os ícones.
"""
import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
GODOT = ROOT.parent.parent / "the-world-geneses"
OUT = ROOT / "Data" / "Rpg"
NEW_ICONS = ROOT / "Data" / "UI" / "SkillIcons" / "Novos"

# Multiplicadores do RaceCatalog.cs do Godot (1 = humano; taken < 1 apanha menos)
RACES = {
    "humano": {"melee": 1.0, "ranged": 1.0, "magic": 1.0, "taken": 1.0, "hpRegen": 1.0, "mpRegen": 1.0, "canMagic": True},
    "elfo": {"melee": .80, "ranged": 1.20, "magic": 1.20, "taken": 1.0, "hpRegen": 1.0, "mpRegen": 1.2, "canMagic": True},
    "diabo": {"melee": 1.10, "ranged": 1.0, "magic": 1.20, "taken": 1.10, "hpRegen": 1.0, "mpRegen": 1.0, "canMagic": True},
    "anao": {"melee": 1.20, "ranged": .60, "magic": 0.0, "taken": .90, "hpRegen": 1.0, "mpRegen": 1.0, "canMagic": False},
    "barbaro": {"melee": 1.35, "ranged": .90, "magic": .60, "taken": .80, "hpRegen": 1.0, "mpRegen": 1.0, "canMagic": True},
    "morto_vivo": {"melee": 1.0, "ranged": 1.0, "magic": 1.0, "taken": 1.0, "hpRegen": 2.0, "mpRegen": 2.0, "canMagic": True},
}


def icon_name(icon):
    return (icon or "").replace("/", "_")


def skills():
    text = (GODOT / "Systems" / "Player" / "StarterWeaponCatalog.cs").read_text(encoding="utf-8")
    out = {}
    for block in re.finditer(r'\["(\w+)"\]\s*=\s*new\[\]\s*\{(.*?)\}\s*,?\s*(?=\["|\};)', text, re.S):
        family = block.group(1)
        out[family] = []
        for m in re.finditer(r'new Skill\("(\w+)", "([^"]*)", "([^"]*)", (\d+), (\d+), ([\d.]+)f?, "([^"]*)", "(\w+)"\)', block.group(2)):
            out[family].append({"id": m.group(1), "name": m.group(2), "description": m.group(3), "level": int(m.group(4)),
                                "mana": int(m.group(5)), "cooldown": float(m.group(6)), "category": m.group(7), "accent": m.group(8),
                                "icon": "ability_" + m.group(1)})
    return out



# tipos tirados do jogo (Espada e Escudo: REVISAO_ARMAS, 11/10/2026; em 28/10/2026 ficaram só a espada, o martelo,
# as adagas, o arco e o cajado)
KEPT = {"espada_de_cinzas", "martelo_de_bram", "adagas_gemeas", "arco_de_betula", "cajado_de_ilsa"}
RETIRED = {"espada_escudo", "espadao", "machado", "machados_duplos", "lanca_do_patrulheiro", "alabarda", "manoplas_de_ferro",
           "maca_sagrada", "katana_do_vento", "besta", "pistola_runica", "chakrams", "varinha", "livro_da_aurora", "orbe",
           "instrumento", "foice", "chicote", "correntes", "catalisador"}


def _retire(x):
    """Tira os tipos aposentados de todas as listas (itens, tipos, armas das classes)."""
    if isinstance(x, list):
        return [_retire(v) for v in x if not (isinstance(v, str) and v in RETIRED) and not (isinstance(v, dict) and (v.get("id") in RETIRED or v.get("family") in RETIRED))]
    if isinstance(x, dict):
        return {k: _retire(v) for k, v in x.items() if not (isinstance(v, dict) and v.get("family") in RETIRED)}
    return x


def main():
    items_doc = json.loads((GODOT / "data" / "items.json").read_text(encoding="utf-8"))
    progression = json.loads((GODOT / "data" / "progression.json").read_text(encoding="utf-8"))
    weapon_types = json.loads((GODOT / "data" / "weapon_types.json").read_text(encoding="utf-8"))
    items = {}
    for item_id, item in items_doc["items"].items():
        items[item_id] = {
            "name": item.get("name", item_id), "description": item.get("description", ""), "rarity": item.get("rarity", "common"),
            "type": item.get("type", "material"), "slot": item.get("slot", ""), "icon": icon_name(item.get("icon", item_id)),
            "stack": int(item.get("stack", 1)), "stats": item.get("stats", {}), "use": item.get("use", {}), "family": item.get("family", ""), "price": int(item.get("price", 0)),
        }
    loot = {species.lower(): [{"item": e[0], "min": e[1], "max": e[2], "chance": e[3]} for e in entries]
            for species, entries in items_doc["loot"].items()}
    data = {
        "_doc": "Gerado por Tools/BuildRpgData.py a partir do Godot (somente leitura). Não editar à mão.",
        "items": items,
        "loot": loot,
        "starting": {"bag": [{"item": e[0], "count": e[1]} for e in items_doc["starting"]["bag"]]},
        "xpSources": progression["xp_sources"],
        "effects": progression["attributes"]["effects"],
        # os 12 oficiais recebem os pontos de nível; os oito antigos ficam na base (mais a raça)
        "officialPerPoint": {k: v for k, v in progression["official_per_point"].items() if not k.startswith("_")},
        "attributeBase": progression["attributes"]["base"],
        "races": RACES,
        "skills": skills(),
        "weaponTypes": [{"id": t["id"], "name": t["name"], "wing": t.get("wing", ""), "primary": t.get("primary", "")}
                        for t in weapon_types["types"]],
        "classes": classes(),
        "classSkills": class_skills(),
        "recipes": recipes(),
    }
    OUT.mkdir(parents=True, exist_ok=True)
    # tipos tirados do jogo (Espada e Escudo: REVISAO_ARMAS, 11/10/2026)
    data = _retire(data)
    apply_new_icons(data)
    icons = write(data)
    print("RPG_DATA", len(items), "itens", len(loot), "saques", sum(len(v) for v in data["skills"].values()), "técnicas", len(icons), "ícones")


def write(data):
    """Grava o rpg.json e a lista de ícones que o Tools/ImportItemIcons.py importa."""
    (OUT / "rpg.json").write_text(json.dumps(data, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    icons = sorted({i["icon"] for i in data["items"].values()} | {s["icon"] for f in data["skills"].values() for s in f}
                   | {c["icon"] for c in data["classes"]["list"] if c["icon"]}
                   | {sk["icon"] for group in data["classSkills"]["classes"].values() for sk in group if sk.get("icon")})
    (OUT / "icons.txt").write_text("\n".join(icons) + "\n", encoding="utf-8")
    return icons


def apply_new_icons(data):
    """Ícones novos das habilidades e das classes (folhas de 8 geradas no ChatGPT, cortadas por Tools/SliceIconSheet.py):
    quando Data/UI/SkillIcons/Novos/skill_<id>.png ou class_<id>.png existe, ele vence o recorte do Godot. Devolve
    quantos trocaram."""
    changed = 0
    for c in data["classes"]["list"]:
        if (NEW_ICONS / f"class_{c['id']}.png").exists() and c["icon"] != "class_" + c["id"]:
            c["icon"] = "class_" + c["id"]
            changed += 1
    for group in data["classSkills"]["classes"].values():
        for sk in group:
            if (NEW_ICONS / f"skill_{sk['id']}.png").exists() and sk.get("icon") != "skill_" + sk["id"]:
                sk["icon"] = "skill_" + sk["id"]
                changed += 1
    # 08/11: os itens (folhas I01...: Docs/04_Interface/PROMPT_ICONES_ITENS.md)
    for key, item in data["items"].items():
        if (NEW_ICONS / f"item_{key}.png").exists() and item["icon"] != "item_" + key:
            item["icon"] = "item_" + key
            changed += 1
    return changed


def classes():
    """Classes do Compêndio v1.2 (data/classes.json): o Despertar no nível 10 tira 6 cartas."""
    doc = json.loads((GODOT / "data" / "classes.json").read_text(encoding="utf-8"))
    out = {"awakeningLevel": doc["awakening_level"], "ascensionLevel": doc["ascension_level"],
           "rarities": {k: v["chance"] for k, v in doc["rarities"].items()}, "list": []}
    for c in doc["classes"]:
        out["list"].append({
            "id": c["id"], "name": c["name"], "rarity": c["rarity"], "role": c.get("role", ""), "attribute": c.get("attribute", ""),
            "description": c.get("description", ""), "weapons": c.get("weapons", []), "buffs": c.get("buffs", {}),
            "icon": icon_name(c.get("icon", "")),
            "passive": c.get("passive", {}), "affinity": c.get("affinity", {}), "drawable": c.get("drawable", True),
            "ascensions": [{"name": a["name"], "description": a.get("description", "")} for a in c.get("ascensions", [])],
        })
    return out


def class_skills():
    """As 4 habilidades de cada classe (data/class_skills.json): todas liberam no Despertar (nível 10).
    Os números são os do Godot (metros, segundos, poder); o Vortex converte para cm ao usar."""
    doc = json.loads((GODOT / "data" / "class_skills.json").read_text(encoding="utf-8"))
    out = {"unlockLevels": doc["unlock_levels"], "rarityPower": doc["rarity_power"], "ascension": doc["ascension"], "classes": {}}
    for owner, skills in doc["classes"].items():
        out["classes"][owner] = []
        for sk in skills:
            entry = dict(sk)
            entry["icon"] = icon_name(sk.get("icon", ""))
            out["classes"][owner].append(entry)
    apply_skill_fixes(out)
    return out


def apply_skill_fixes(class_skills_data):
    """Data/Rpg/class_skill_fixes.json: nomes que vinham quebrados ("Duelo Mortal da Ascensão:") e o ícone de cada
    habilidade escolhido nas 4 folhas (A, B, C, D). Devolve quantas mudaram."""
    fixes = json.loads((OUT / "class_skill_fixes.json").read_text(encoding="utf-8"))
    changed = 0
    for group in class_skills_data["classes"].values():
        for sk in group:
            name = fixes["names"].get(sk["id"])
            icon = fixes["icons"].get(sk["id"])
            if name and sk["name"] != name:
                sk["name"] = name
                changed += 1
            if icon and sk.get("icon") != "atlas_" + icon and not (NEW_ICONS / f"skill_{sk['id']}.png").exists():
                sk["icon"] = "atlas_" + icon
                changed += 1
    return changed


def recipes():
    """Receitas (data/crafting.json): poções na mesa de alquimia e equipamento na bancada."""
    doc = json.loads((GODOT / "data" / "crafting.json").read_text(encoding="utf-8"))
    return [{"id": r["id"], "name": r["name"], "description": r.get("description", ""), "output": r["output"],
             "count": r.get("count", 1), "station": r.get("station", ""), "ingredients": r.get("ingredients", {})}
            for r in doc["recipes"]]


if __name__ == "__main__":
    if sys.argv[1:] == ["--icons"]:
        current = json.loads((OUT / "rpg.json").read_text(encoding="utf-8"))
        print("RPG_ICONS", apply_new_icons(current), "ícones trocados,", len(write(current)), "na lista")
    else:
        main()
