"""História, missões e diálogos do Vortex: Data/Story/story.json.

python Tools/BuildStoryData.py

Junta (o Godot só é lido):
- as missões do Godot (data/quests.json) e as do Vortex (Data/Story/vortex_story.json), com o
  capítulo da jornada; as do Vortex podem trocar uma do Godot com o mesmo id;
- os diálogos do Godot (data/dialogues/*.json) e os do Vortex;
- as falas curtas (barks), o nome e o título de cada NPC (data/npcs.json e Data/Population/npc_extra.json);
- os capítulos e as regiões (lore) para a aba Missões.
Missões do Godot dos NPCs de fora do vale (Amal, Lyra) ficam na região deles, travadas até o
continente chegar ao Vortex.
"""
import json
import pathlib

ROOT = pathlib.Path(__file__).resolve().parents[1]
GODOT = ROOT.parent.parent / "the-world-geneses" / "data"
OUT = ROOT / "Data" / "Story"

# capítulo e linha principal das missões do Godot
GODOT_CHAPTER = {
    "rhea_recruta": (1, True), "rhea_patrulha": (1, True), "rhea_sombra": (1, True), "rhea_caldeira": (1, True),
}
GODOT_REGION = {"amal_rochas": "Solcânion", "amal_seryth": "Solcânion", "lyra_trilhas": "Verdalume", "lyra_thorun": "Verdalume"}
# a primeira missão da Rhea passa a vir depois de chegar ao Santuário (linha principal do Vortex)
GODOT_REQUIRES = {"rhea_recruta": ["v_despertar"]}
# 02/11: a Juh do Godot era para ser a Julia (o usuário: "só bugou e duplicou com outro nome"). As missões dela passam
# para a Julia, os textos chamam a Julia, e a conversa e o posto da Juh saem (a Julia tem os dela)
MERGED_NPCS = {"juh": ("julia", "Juh", "Julia")}


def merge_npc(value):
    """Troca a Juh pela Julia em tudo de uma missão ou diálogo (quem dá, os alvos e os textos)."""
    text = json.dumps(value, ensure_ascii=False)
    for old, (new, old_name, new_name) in MERGED_NPCS.items():
        text = text.replace('"%s"' % old, '"%s"' % new).replace(old_name, new_name)
    return json.loads(text)


def main():
    vortex = json.loads((OUT / "vortex_story.json").read_text(encoding="utf-8"))
    godot_quests = json.loads((GODOT / "quests.json").read_text(encoding="utf-8"))["quests"]
    quests = {}
    for q in godot_quests:
        chapter, main_line = GODOT_CHAPTER.get(q["id"], (1, False))
        entry = merge_npc(dict(q))
        entry.setdefault("chapter", chapter if q["id"] not in GODOT_REGION else (3 if GODOT_REGION[q["id"]] == "Verdalume" else 4))
        entry["main"] = main_line
        if q["id"] in GODOT_REGION:
            entry["region"] = GODOT_REGION[q["id"]]
        if q["id"] in GODOT_REQUIRES:
            entry["requires"] = sorted(set(entry.get("requires", []) + GODOT_REQUIRES[q["id"]]))
        entry["source"] = "godot"
        quests[q["id"]] = entry
    for q in vortex["quests"]:
        entry = dict(q)
        entry["source"] = "vortex"
        quests[q["id"]] = entry
    # ordem: linha principal primeiro, depois por capítulo, mantendo a ordem dos arquivos
    order = list(quests.values())
    order.sort(key=lambda q: (q.get("chapter", 1), 0 if q.get("main") else 1))

    dialogues = {}
    for path in sorted((GODOT / "dialogues").glob("*.json")):
        if path.stem in MERGED_NPCS:
            continue
        dialogues[path.stem] = merge_npc(json.loads(path.read_text(encoding="utf-8")))
    dialogues.update(vortex.get("dialogues", {}))

    npcs = {}
    sources = [json.loads((GODOT / "npcs.json").read_text(encoding="utf-8"))["npcs"]]
    extra = ROOT / "Data" / "Population" / "npc_extra.json"
    if extra.exists():
        sources.append(json.loads(extra.read_text(encoding="utf-8"))["npcs"])
    for group in sources:
        for npc in group:
            if npc["id"] in MERGED_NPCS:
                continue
            npc = merge_npc(npc)
            npcs[npc["id"]] = {"name": npc.get("name", npc["id"]), "title": npc.get("title", ""),
                               "barks": npc.get("barks", []), "dialogue": npc.get("dialogue", npc["id"]),
                               "position": npc.get("position", [0, 0])}

    # conferências: quem dá cada missão existe, os requisitos existem
    for q in order:
        if q["giver"] not in npcs:
            print("AVISO: missão", q["id"], "dada por NPC desconhecido", q["giver"])
        for r in q.get("requires", []):
            if r not in quests:
                print("AVISO: missão", q["id"], "requer", r, "que não existe")

    data = {
        "_doc": "Gerado por Tools/BuildStoryData.py (Godot + Data/Story/vortex_story.json). Não editar à mão.",
        "chapters": vortex["chapters"], "regions": vortex["regions"],
        "quests": order, "dialogues": dialogues, "npcs": npcs,
    }
    (OUT / "story.json").write_text(json.dumps(data, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print("STORY", len(order), "missões", len(dialogues), "diálogos", len(npcs), "NPCs", len(vortex["chapters"]), "capítulos")


if __name__ == "__main__":
    main()
