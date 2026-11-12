"""Catálogo da criação de herói: Data/GodotCharacter/look_catalog.json (+ palette.png e palette_map.json).

Uso: python Tools/BuildLookCatalog.py   (depois de ConvertGodotWardrobe.py --all-local e ConvertGodotRacePieces.py)

Lê do Godot (somente leitura): Art/Adventurer/wardrobe.json, palette_map.json, RacePieces_palette.png
e os números das raças de Systems/Player/RaceCatalog.cs (copiados abaixo). Só entram as peças que
já foram convertidas para o Unreal (Data/GodotCharacter/Wardrobe/manifest.json).

O que é do Vortex (não existia no Godot): cor de destaque e frase de cada raça na seleção,
paletas maiores por cor, sílabas para o gerador de nomes e as categorias na ordem do criador novo.
"""
import json
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GODOT = ROOT.parents[1] / "the-world-geneses" / "Art" / "Adventurer"
OUT = ROOT / "Data" / "GodotCharacter"

ATTRIBUTES = ["Força", "Destreza", "Vitalidade", "Inteligência", "Mana", "Espírito", "Recuperação", "Sorte"]
# id, nome, frase (Godot), atributos iniciais, escala, traços, pele, prefixos dos traços, cor de destaque,
# epíteto (Vortex), sílabas para nomes
RACES = [
    ("humano", "Humano", "Equilibrado e versátil.", [6, 5, 8, 4, 6, 4, 4, 4], 1.0,
     ["Usa qualquer arma e magia, sem fraquezas"], ["#F1C6A1", "#DFAF87", "#B77E5B", "#8E573D", "#5C382B"],
     ["race_none"], "#E9B949", "Filhos do Vale",
     (["Ar", "Bel", "Cai", "Dar", "El", "Fen", "Gar", "Hal", "Ian", "Lu", "Ma", "Rai", "Tor", "Val"],
      ["an", "en", "ric", "ton", "do", "ra", "lia", "mir", "son", "vel", "ina", "ro"])),
    ("elfo", "Elfo", "Ligado à magia e às flechas do vale.", [3, 6, 9, 7, 8, 5, 2, 3], 1.0,
     ["+20% dano com arco e magia", "-20% dano corpo a corpo", "Mana volta 20% mais rápido"],
     ["#F6DCC4", "#EFD2BA", "#D9B48E", "#9AA6C8", "#6E7AA8"], ["elf_"], "#6FD39A", "Guardiões da Floresta",
     (["Ae", "Ca", "El", "Fae", "Gil", "Ith", "Lae", "Mel", "Nae", "Syl", "Thal", "Yl"],
      ["dris", "lien", "riel", "thas", "wen", "lia", "nor", "rion", "sael", "vyn", "ndra"])),
    ("diabo", "Demônio", "Nascido das brasas da cratera.", [7, 5, 7, 6, 7, 4, 2, 3], 1.0,
     ["+10% dano corpo a corpo", "+20% dano mágico", "Recebe +10% de dano"],
     ["#B8342C", "#8E2430", "#C25A3A", "#6A3A8A", "#3A2A3A"], ["demon_"], "#FF5A3C", "Herdeiros da Cratera",
     (["Az", "Bal", "Ka", "Mal", "Mor", "Naz", "Ra", "Shai", "Vex", "Xar", "Zar"],
      ["eth", "gor", "ira", "kul", "oth", "rax", "thar", "vas", "zul", "ys", "mon"])),
    ("anao", "Anão", "Baixo, forte e teimoso como pedra.", [8, 4, 10, 2, 3, 3, 6, 5], 1 / 1.5,
     ["1,5x menor", "+20% dano corpo a corpo", "Recebe -10% de dano", "-40% dano com arco", "Não usa magia"],
     ["#F1C6A1", "#E2AE8D", "#C98E6A", "#AA7558", "#8A5A40"], ["race_none", "dwarf_"], "#D08A3C", "Clãs da Pedra",
     (["Bal", "Bor", "Dur", "Grim", "Kaz", "Mor", "Thor", "Brun", "Dain", "Hel"],
      ["din", "dek", "grim", "in", "li", "mund", "rak", "rin", "uk", "dra"])),
    ("barbaro", "Bárbaro", "Gigante das terras geladas.", [10, 4, 12, 1, 2, 2, 6, 4], 1.5,
     ["1,5x maior", "+35% dano corpo a corpo", "Recebe -20% de dano", "-40% dano mágico"],
     ["#E2AE8D", "#C98E6A", "#AA7558", "#8A5A40", "#5C382B"], ["race_none", "barb_"], "#7FC8F0", "Gigantes do Gelo",
     (["Bjor", "Gun", "Hrof", "Kja", "Rag", "Sig", "Tor", "Ulf", "Vald", "Yr"],
      ["ar", "dis", "grim", "hild", "mund", "nar", "rik", "stein", "ulf", "vor"])),
    ("morto_vivo", "Morto-vivo", "Esqueleto erguido por cristais antigos.", [4, 5, 4, 5, 9, 7, 7, 2], 1.0,
     ["Vida e mana voltam 2x mais rápido"], ["#E6E0CC", "#D8D2BE", "#BFC4B2", "#A8A89A", "#8C9A8A"], ["undead_"],
     "#4FE3D6", "Erguidos pelos Cristais",
     (["Ash", "Cor", "Gho", "Lich", "Mor", "Nyx", "Os", "Sep", "Vel", "Wra"],
      ["an", "ra", "this", "ul", "veth", "rix", "mora", "kas", "thul", "eon"])),
]

COLOR_SLOTS = [
    ("skin_color", "Pele", "#F1C6A1", None),
    ("hair_color", "Cabelo", "#784026", ["#F3D9A0", "#E4B877", "#C9853F", "#B8783D", "#784026", "#4A2A1C", "#392A24", "#1E1A1C", "#D6D3C7", "#B24A3A", "#4C6FA8", "#7D4BA0"]),
    ("shirt_color", "Camisa", "#507E38", ["#507E38", "#2D6755", "#28638A", "#324E92", "#403C8E", "#8A342C", "#562C3C", "#B9A47A", "#E2D8BE", "#28262C", "#C47A2C", "#7A7F86"]),
    ("sleeve_color", "Mangas", "#507E38", ["#507E38", "#2D6755", "#28638A", "#324E92", "#403C8E", "#8A342C", "#562C3C", "#B9A47A", "#E2D8BE", "#28262C", "#C47A2C", "#7A7F86"]),
    ("pants_color", "Calça", "#2C3142", ["#2C3142", "#3D4939", "#795A42", "#354B62", "#171E25", "#784C2E", "#BCA67C", "#A82028", "#5A4A6A", "#2C2E3A", "#6A6E72", "#C4688A"]),
    ("boots_color", "Botas", "#5C3422", ["#5C3422", "#784C2E", "#9A6E43", "#3C2B29", "#28262C", "#CEC2A0", "#A6A9AC", "#2E3A4A", "#6B2A24", "#4A5A3A"]),
    ("glove_color", "Luvas", "#583220", ["#583220", "#784C2E", "#9A6E43", "#3C2B29", "#28262C", "#CEC2A0", "#A6A9AC", "#562C3C", "#2E3A4A", "#4A5A3A"]),
    ("belt_color", "Cinto", "#5E3A26", ["#5E3A26", "#7A4A2C", "#9A6E43", "#3C2B29", "#252B32", "#B08A3A", "#6B2A24", "#2E3A4A"]),
    ("strap_color", "Alça e bainha", "#7A4A2C", ["#7A4A2C", "#5E3A26", "#9A6E43", "#3C2B29", "#252B32", "#B08A3A", "#6B2A24", "#2E3A4A"]),
]
# Regras do Vortex para cada raça ficar com cara própria (no Godot as peças comuns valiam para todas):
# own = categorias em que só aparecem as peças da raça; hide = categorias que a raça não tem;
# faceless = o rosto-base some (nenhuma raça usa hoje: as peças de rosto são placas sobre a cabeça); traitLabel = nome do traço.
RULES = {
    "humano": {"own": [], "hide": [], "faceless": False, "traitLabel": "Traço da raça"},
    "elfo": {"own": ["hair", "eyes"], "hide": [], "faceless": False, "traitLabel": "Orelhas"},
    "diabo": {"own": ["hair", "eyes"], "hide": [], "faceless": False, "traitLabel": "Chifres e cauda"},
    "anao": {"own": ["hair", "eyes"], "hide": [], "faceless": False, "traitLabel": "Barba"},
    "barbaro": {"own": ["hair", "eyes"], "hide": [], "faceless": False, "traitLabel": "Marcas de guerra"},
    # o crânio é uma placa na frente da cabeça (como no Godot): a cabeça fica, na cor do osso
    "morto_vivo": {"own": [], "hide": ["hair", "eyes", "mouth"], "faceless": False, "traitLabel": "Crânio"},
}
# Proporção de cada raça (Vortex): height/width = tamanho do corpo inteiro; legs/legThick =
# comprimento e grossura das pernas; torso = largura do tronco; arms = grossura dos braços;
# head = tamanho da cabeça; meters = altura mostrada na criação.
SHAPES = {
    "humano": {"height": 1.0, "width": 1.0, "legs": 1.0, "legThick": 1.0, "torso": 1.0, "arms": 1.0, "head": 1.0, "meters": 1.40},
    "elfo": {"height": 1.04, "width": .92, "legs": 1.08, "legThick": .9, "torso": .94, "arms": .9, "head": .96, "meters": 1.52},
    "diabo": {"height": 1.03, "width": 1.0, "legs": 1.0, "legThick": 1.0, "torso": 1.07, "arms": 1.05, "head": 1.0, "meters": 1.45},
    # baixinho e parrudo: pernas curtas e grossas, tronco largo, cabeça grande
    "anao": {"height": .84, "width": 1.02, "legs": .72, "legThick": 1.25, "torso": 1.25, "arms": 1.2, "head": 1.12, "meters": 0.98},
    # gigante musculoso: largo, braços grossos, cabeça proporcionalmente menor
    "barbaro": {"height": 1.42, "width": 1.5, "legs": 1.02, "legThick": 1.12, "torso": 1.12, "arms": 1.15, "head": .88, "meters": 2.05},
    "morto_vivo": {"height": 1.0, "width": .9, "legs": 1.02, "legThick": .78, "torso": .9, "arms": .78, "head": 1.0, "meters": 1.42},
}
CATEGORIES = [("race", "Traço da raça"), ("hair", "Cabelo"), ("eyes", "Olhos"), ("mouth", "Boca"),
              ("head", "Chapéu e cabeça"), ("face_gear", "Rosto"), ("back", "Pescoço e costas"),
              ("shirt", "Camisa"), ("pants", "Calça"), ("shoes", "Sapatos"), ("gloves", "Luvas")]
# No Godot os cosméticos eram um "acessório" só. No Vortex eles se combinam em três lugares
# (um chapéu, uns óculos e uma capa ao mesmo tempo), cada um com a própria peça no corpo.
COSMETIC_SLOTS = ("head", "face_gear", "back")
HEAD_WORDS = ("tiara", "chapéu", "coroa", "chifres pequenos", "anéis nos chifres", "diadema", "flor", "pena", "laço", "bandana",
              "faixa", "auréola")
FACE_WORDS = ("óculos", "tapa-olho", "máscara", "monóculo", "brincos")


def cosmetic_slot(name):
    low = name.lower()
    if any(w in low for w in HEAD_WORDS):
        return "head"
    if any(w in low for w in FACE_WORDS):
        return "face_gear"
    return "back"


def main():
    wardrobe = json.loads((GODOT / "wardrobe.json").read_text(encoding="utf-8"))
    imported = json.loads((OUT / "Wardrobe" / "manifest.json").read_text(encoding="utf-8"))
    pieces = {}
    for piece_id, piece in wardrobe["pieces"].items():
        if piece_id in imported or piece_id in ("accessory_none", "race_none"):
            pieces[piece_id] = {"name": piece["name"], "category": piece["category"], "defaults": piece.get("defaults", {}),
                                "races": piece.get("races", [])}
    pieces.pop("accessory_none", None)
    pieces.setdefault("race_none", {"name": "Nenhum", "category": "race", "defaults": {}, "races": []})
    for slot in COSMETIC_SLOTS:
        pieces[slot + "_none"] = {"name": "Nenhum", "category": slot, "defaults": {}, "races": []}
    for piece_id, piece in pieces.items():
        if piece["category"] == "accessory":
            piece["category"] = cosmetic_slot(piece["name"])
    catalog = {}
    for who, categories in wardrobe["catalog"].items():
        catalog[who] = {}
        accessories = [p for p in categories.get("accessory", []) if p in pieces]
        for category, _ in CATEGORIES:
            if category in COSMETIC_SLOTS:
                # "nenhum" primeiro: o herói começa sem e escolhe o que quiser
                options = [category + "_none"] + [p for p in accessories if pieces[p]["category"] == category]
            else:
                options = [p for p in categories.get(category, []) if p in pieces]
            if category == "race" and not options:
                options = ["race_none"]
            catalog[who][category] = options
    races = []
    for race_id, name, tagline, start, scale, traits, skin, features, accent, epithet, syllables in RACES:
        races.append({"id": race_id, "name": name, "tagline": tagline, "epithet": epithet, "start": start, "scale": scale,
                      "traits": traits, "skin": skin, "features": features, "accent": accent,
                      "nameStart": syllables[0], "nameEnd": syllables[1], "shape": SHAPES[race_id], **RULES[race_id]})
    slots = []
    for key, label, default, palette in COLOR_SLOTS:
        slots.append({"key": key, "label": label, "default": default, "palette": palette or []})
    data = {"_doc": "Gerado por Tools/BuildLookCatalog.py a partir do Godot (somente leitura). Não editar à mão.",
            "attributes": ATTRIBUTES, "races": races, "colorSlots": slots,
            "categories": [{"id": c, "label": l} for c, l in CATEGORIES],
            "fixed": wardrobe["fixed"], "pieces": pieces, "catalog": catalog}
    (OUT / "look_catalog.json").write_text(json.dumps(data, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    shutil.copyfile(GODOT / "palette_map.json", OUT / "palette_map.json")
    # a paleta das peças de raça contém a do herói inteira e mais os tons novos: é a única do jogo
    shutil.copyfile(GODOT / "RacePieces_palette.png", OUT / "palette.png")
    counts = {c: len(catalog["Male"][c]) for c, _ in CATEGORIES}
    print("LOOK_CATALOG", len(pieces), "peças", counts)


if __name__ == "__main__":
    main()
