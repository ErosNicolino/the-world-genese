"""O prompt único dos ícones das armas novas, armaduras e itens (08/11), para o ChatGPT.

python Tools/BuildItemIconPrompt.py
Sai Docs/04_Interface/PROMPT_ICONES_ITENS.md e as folhas I01... em Data/UI/SkillIcons/Folhas/folhas.json (qual ícone vai em
cada casa, para o Tools/SliceIconSheet.py).
"""
import json
import pathlib

ROOT = pathlib.Path(__file__).resolve().parents[1]
J = lambda name: json.loads((ROOT / "Data" / "Rpg" / name).read_text(encoding="utf-8"))
RARITY = {"common": "common (gray)", "uncommon": "uncommon (green)", "rare": "rare (blue)", "epic": "epic (purple)",
          "legendary": "legendary (orange)", "unique": "unique (crimson)", "divine": "divine (white-gold)"}
ELEMENT = {"": "", "fire": ", fire", "ice": ", ice", "lightning": ", lightning", "nature": ", nature", "poison": ", poison",
           "water": ", water", "holy": ", holy light", "shadow": ", shadow"}
SLOT = {"head": "head piece", "chest": "chestplate", "legs": "leg armor", "boots": "pair of boots"}
ITEMS = {
    "azhur_core": "glowing runic stone core with an orange diamond heart",
    "colossus_shard": "broken gray stone shard with a glowing orange crack",
    "caldera_amulet": "amulet with an orange diamond gem on a gold chain",
    "seryth_sting_charm": "charm made of a pale blue crystal scorpion stinger",
    "thorun_heartwood_charm": "charm of green glowing heartwood set in dark wood",
    "ring_jade": "gold ring with a pointed jade stone",
    "amulet_moon": "dark leather necklace with a small blue moon pendant",
    "potion_red": "round glass flask of red healing potion with a cork",
    "potion_blue": "round glass flask of blue mana potion with a cork",
    "potion_green": "round glass flask of green herbal potion with a cork",
    "bread": "golden loaf of bread",
    "gel_jade": "drop of translucent teal jelly",
    "wolf_pelt": "gray wolf pelt with the head and paws spread out",
    "healing_herb": "sprig of green healing leaves",
    "mushroom_cap": "red mushroom with white spots",
    "moss_core": "round mossy stone with a glowing green center",
    "bat_wing": "purple amethyst-toned bat wing",
    "shield_wood": "round oak shield with an iron rim",
    "iron_scrap": "pile of rusty iron scraps and bolts",
    "bark_fiber": "coiled ring of rough bark rope",
    "obsidian_fragment": "sharp black obsidian shard with a red ember glow",
    "antidoto": "small green antidote vial with a white cross label shape, no letters",
    "racao_de_viagem": "brown leather travel satchel stuffed with food",
    "mapa_de_viajante": "folded parchment map with a red dotted route",
    "carta_de_terras_distantes": "rolled letter map of distant lands with a red wax seal",
    "mapa_de_nicolly": "old rolled treasure map with torn golden edges",
    "meat": "roasted leg of game meat with a bone",
    "rabbit_pelt": "brown rabbit pelt with long ears",
    "deer_antler": "pale branching deer antler",
    "bandit_bandana": "red bandana with white dots, folded in a triangle",
    "frutas_silvestres": "bunch of dark purple wild berries with a leaf",
}


def main():
    rpg, swords, bows, armors = J("rpg.json"), J("swords.json")["swords"], J("bows.json")["bows"], J("armors.json")
    entries = []
    for kind, table in (("sword", swords), ("bow", bows)):
        for e in table.values():
            entries.append((e["icon"], e["name"], f"{kind} named \"{e['name']}\", {RARITY[e['rarity']]}{ELEMENT.get(e['element'], '')}"))
    for piece in armors["pieces"].values():
        rarity = armors["sets"][piece["set"]]["rarity"]
        entries.append((piece["icon"], piece["name"], f"{SLOT[piece['slot']]} named \"{piece['name']}\", {RARITY[rarity]}{ELEMENT.get(piece['element'], '')}"))
    for key, text in ITEMS.items():
        item = rpg["items"][key]
        entries.append(("item_" + key, item["name"], f"{text}, {RARITY.get(item['rarity'], RARITY['common'])}"))
    ids = [e[0] for e in entries]
    assert len(ids) == len(set(ids)), [i for i in ids if ids.count(i) > 1]
    sheets = [entries[i:i + 8] for i in range(0, len(entries), 8)]

    path = ROOT / "Data" / "UI" / "SkillIcons" / "Folhas" / "folhas.json"
    data = json.loads(path.read_text(encoding="utf-8"))
    data = {k: v for k, v in data.items() if not (k.startswith("I") and k[1:].isdigit())}
    for n, sheet in enumerate(sheets, 1):
        data[f"I{n:02d}"] = [{"id": i, "name": name} for i, name, _ in sheet]
    path.write_text(json.dumps(data, ensure_ascii=False, indent=1), encoding="utf-8")

    lines = [f"""I'm making the item icons for a cozy fantasy voxel RPG called "The World Gêneses" (Minecraft-like blocky world). Below are {len(sheets)} sheets of 8 icons each. Generate ONE sheet per image, starting with sheet I01. After each image, stop and wait. When I say "next", generate the following sheet. Keep the exact same style in all of them.

Rules for every sheet:
- Layout: landscape 1536x1024 canvas, 4 columns x 2 rows, 8 square tiles of the same size, evenly spaced, separated by a flat dark navy (#101826) gap, in the order listed (left to right, top row first).
- No numbers, labels, letters or text anywhere, and no frame or border (the game draws its own).
- Each tile: full-bleed square with a soft radial background in the item's rarity color (the color in parentheses).
- Style: chunky voxel / blocky 3D look with subtle pixel texture, one bold centered object with a simple silhouette that still reads at 48 px, strong contrast, soft lighting. The object is shown alone: no character, no hands.
- Rarity shows in the material: common is plain, and higher rarities get richer materials, gold, gems and a small magic glow. An element, when listed, sets the glow and details.
- The names are in Portuguese: use them as inspiration for the look, and never write them on the image.
"""]
    for n, sheet in enumerate(sheets, 1):
        lines.append(f"Sheet I{n:02d}:")
        lines.extend(f"{i}. {text}" for i, (_, _, text) in enumerate(sheet, 1))
        lines.append("")
    prompt = "\n".join(lines).strip()
    doc = f"""# Prompt único dos ícones dos itens (08/11)

São {len(entries)} ícones em {len(sheets)} folhas de 8: 35 espadas, 35 arcos, {len(armors['pieces'])} peças de armadura e {len(ITEMS)} itens (materiais, poções, amuletos).
Uma imagem só não comporta todos de forma legível, por isso o prompt pede uma folha por vez.

## Como usar

1. Abra uma conversa nova no ChatGPT e cole o prompt abaixo inteiro. Ele gera a folha I01.
2. Para cada folha seguinte, escreva só `next`.
3. Salve cada imagem com o nome da folha (`I01.png`, `I02.png`...) em `Data/UI/SkillIcons/Folhas/`.
4. Se o estilo começar a mudar, abra outra conversa, cole o prompt e peça a folha em que parou (por exemplo: `start at sheet I12`).
5. Me avise quando tiver as folhas: eu corto (`python Tools/SliceIconSheet.py I01 I02 ...`), ligo no jogo e, a partir delas, faço os modelos 3D.

## O prompt

```
{prompt}
```
"""
    (ROOT / "Docs" / "04_Interface" / "PROMPT_ICONES_ITENS.md").write_text(doc, encoding="utf-8")
    print(len(entries), "ícones", len(sheets), "folhas", len(prompt), "caracteres")


main()
