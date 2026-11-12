"""O PDF das classes e das habilidades (03/11): Docs/05_Referencias/CLASSES_E_HABILIDADES.pdf.

python Tools/MakeClassesPdf.py

Lê Data/Rpg/rpg.json (as 66 classes e as 4 habilidades de cada uma) e os ícones (Data/UI/SkillIcons/atlas_<código>.png,
o código vem de Data/Rpg/class_skill_fixes.json). Monta um HTML e imprime em PDF com o Edge (sem janela).
Cada classe tem um número e cada habilidade também (ex.: 12.3), para o usuário dizer qual quer refazer.
"""
import html
import json
import os
import subprocess

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "Docs", "05_Referencias")
EDGE = r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe"

RARITY = {
    "common": ("Comum", "#8b8f98"), "rare": ("Rara", "#3b82f6"), "epic": ("Épica", "#9b5de5"),
    "legendary": ("Lendária", "#f59e0b"), "unique": ("Única", "#dc2626"), "divine": ("Divina", "#d4af37"),
}
KIND = {
    "cleave": "golpe em leque à frente", "dash": "investida", "buff": "fortalece", "zone": "área no chão",
    "nova": "explosão em volta", "chain": "salta de alvo em alvo", "projectile": "projétil", "heal": "cura",
    "shield": "escudo", "strike": "golpe num alvo", "summon": "invocação", "wave": "onda", "beam": "raio",
}
ELEMENT = {
    "steel": "aço", "fire": "fogo", "ice": "gelo", "water": "água", "nature": "natureza", "jade": "jade",
    "holy": "sagrado", "arcane": "arcano", "shadow": "sombra", "lightning": "raio", "poison": "veneno", "blood": "sangue",
    "bone": "osso", "earth": "terra", "wind": "vento", "moon": "lua", "star": "estrela", "spirit": "espírito",
    "time": "tempo", "music": "música", "rune": "runa",
}
ATTRIBUTE = {
    "strength": "Força", "agility": "Agilidade", "intelligence": "Inteligência", "spirit": "Espírito",
    "vitality": "Vitalidade", "dexterity": "Destreza", "faith": "Fé", "luck": "Sorte", "charisma": "Carisma",
}

CSS = """
@page { size: A4; margin: 14mm 12mm; }
* { box-sizing: border-box; }
body { font-family: 'Segoe UI', Arial, sans-serif; color: #1f2430; margin: 0; font-size: 10.5pt; }
.cover { height: 260mm; display: flex; flex-direction: column; justify-content: center; align-items: center;
  background: linear-gradient(160deg, #1c2340 0%, #2c1d4a 55%, #4a2a1a 100%); color: #f4ead2; border-radius: 14px;
  page-break-after: always; text-align: center; padding: 30mm; }
.cover h1 { font-size: 34pt; margin: 0 0 6mm; color: #f0cf7a; letter-spacing: 1px; }
.cover p { font-size: 13pt; line-height: 1.5; margin: 2mm 0; max-width: 150mm; }
.cover .count { display: flex; gap: 6mm; margin-top: 12mm; flex-wrap: wrap; justify-content: center; }
.cover .pill { padding: 3mm 6mm; border-radius: 999px; font-weight: 700; color: #fff; font-size: 11pt; }
.how { background: rgba(255,255,255,.08); border-radius: 10px; padding: 6mm 8mm; margin-top: 12mm; text-align: left; font-size: 11pt; }
.index { page-break-after: always; }
.index h2 { color: #2c1d4a; margin: 0 0 4mm; }
.index ol { columns: 3; column-gap: 8mm; margin: 0; padding-left: 7mm; font-size: 10pt; line-height: 1.55; }
.index li span { font-size: 8pt; font-weight: 700; padding: 0 2mm; border-radius: 4px; color: #fff; margin-left: 1.5mm; }
.class { border: 1.5px solid #d9dce5; border-radius: 12px; margin: 0 0 6mm; overflow: hidden; page-break-inside: avoid; }
.head { display: flex; align-items: center; gap: 4mm; padding: 3.5mm 5mm; color: #fff; }
.head .num { font-size: 20pt; font-weight: 800; opacity: .9; min-width: 12mm; }
.head .name { font-size: 16pt; font-weight: 800; }
.head .tag { margin-left: auto; font-weight: 700; background: rgba(0,0,0,.22); padding: 1mm 4mm; border-radius: 999px; font-size: 9.5pt; }
.meta { padding: 2.5mm 5mm; background: #f5f6fa; font-size: 9.5pt; line-height: 1.5; }
.meta b { color: #2c1d4a; }
.skills { display: grid; grid-template-columns: 1fr 1fr; gap: 0; }
.skill { display: flex; gap: 3mm; padding: 3mm 4mm; border-top: 1px solid #e6e8ef; }
.skill:nth-child(odd) { border-right: 1px solid #e6e8ef; }
.skill img { width: 15mm; height: 15mm; border-radius: 6px; flex: none; background: #222; }
.skill .noimg { width: 15mm; height: 15mm; border-radius: 6px; background: #ccd; flex: none; }
.skill .t { font-weight: 800; font-size: 10.5pt; }
.skill .t i { font-style: normal; color: #8a8fa0; font-weight: 700; margin-right: 1mm; }
.skill .k { font-size: 8.5pt; color: #5b6070; margin: .5mm 0 1mm; }
.skill .d { font-size: 9pt; line-height: 1.35; }
.skill .box { margin-top: 1.5mm; font-size: 8pt; color: #8a8fa0; }
.asc { padding: 2mm 5mm 3mm; font-size: 9pt; background: #fbfaf5; border-top: 1px solid #e6e8ef; }
"""


def esc(text):
    return html.escape(str(text or ""))


def clean(name, text):
    """As descrições do Godot começam repetindo o nome ("Frasco Explosivo: ..."): sai o nome."""
    text = str(text or "")
    if name and text.startswith(name + ":"):
        text = text[len(name) + 1:].strip()
        text = text[:1].upper() + text[1:]
    return text


def thumb(code):
    """O ícone em 128 px (os originais deixavam o PDF com 16 MB), numa pasta temporária."""
    if not code:
        return None
    source = os.path.join(ROOT, "Data", "UI", "SkillIcons", f"atlas_{code}.png")
    if not os.path.exists(source):
        return None
    folder = os.path.join(ROOT, "Saved", "ClassesPdfIcons")
    os.makedirs(folder, exist_ok=True)
    small = os.path.join(folder, f"{code}.jpg")
    if not os.path.exists(small):
        from PIL import Image
        Image.open(source).convert("RGB").resize((128, 128), Image.LANCZOS).save(small, quality=88)
    return small


def weapon_name(weapon_id):
    return weapon_id.replace("_", " ")


def main():
    data = json.load(open(os.path.join(ROOT, "Data", "Rpg", "rpg.json"), encoding="utf-8"))
    fixes = json.load(open(os.path.join(ROOT, "Data", "Rpg", "class_skill_fixes.json"), encoding="utf-8"))
    icons = fixes.get("icons", {})
    classes = data["classes"]["list"]
    skills = data["classSkills"]["classes"]
    order = ["common", "rare", "epic", "legendary", "unique", "divine"]
    classes = sorted(classes, key=lambda c: (order.index(c.get("rarity", "common")) if c.get("rarity") in order else 9, c["name"]))
    counts = {r: sum(1 for c in classes if c.get("rarity") == r) for r in order}

    parts = ["<!doctype html><html lang='pt-BR'><head><meta charset='utf-8'><title>Classes e Habilidades</title><style>", CSS, "</style></head><body>"]
    parts.append("<div class='cover'><h1>Classes e Habilidades</h1>")
    parts.append(f"<p>The World Gêneses: as <b>{len(classes)} classes</b> do Despertar (nível 10) e as "
                 f"<b>{sum(len(skills.get(c['id'], [])) for c in classes)} habilidades</b> (4 por classe, teclas 1 a 4).</p>")
    parts.append("<p>Gerado de <code>Data/Rpg/rpg.json</code> em 03/11/2026.</p><div class='count'>")
    for r in order:
        name, color = RARITY[r]
        plural = {"Comum": "Comuns"}.get(name, name + "s") if counts[r] != 1 else name
        parts.append(f"<div class='pill' style='background:{color}'>{counts[r]} {esc(plural)}</div>")
    parts.append("</div><div class='how'><b>Como usar:</b> cada classe tem um número e cada habilidade também "
                 "(por exemplo, <b>12.3</b> é a terceira habilidade da classe 12). Diga qual quer refazer e como "
                 "deveria ser, uma por vez. Ao lado de cada habilidade há espaço para marcar <b>ok / feia / bugada</b>.</div></div>")

    parts.append("<div class='index'><h2>Índice</h2><ol>")
    for c in classes:
        _, color = RARITY.get(c.get("rarity"), ("", "#888"))
        parts.append(f"<li>{esc(c['name'])}<span style='background:{color}'>{esc(RARITY.get(c.get('rarity'), ('?',))[0])}</span></li>")
    parts.append("</ol></div>")

    for n, c in enumerate(classes, 1):
        rarity, color = RARITY.get(c.get("rarity"), ("?", "#888"))
        parts.append("<div class='class'>")
        parts.append(f"<div class='head' style='background:linear-gradient(90deg,{color},{color}cc)'><div class='num'>{n}</div>"
                     f"<div class='name'>{esc(c['name'])}</div><div class='tag'>{esc(rarity)}</div></div>")
        weapons = ", ".join(weapon_name(w) for w in c.get("weapons", [])) or "qualquer"
        passive = c.get("passive") or {}
        parts.append(f"<div class='meta'><b>Papel:</b> {esc(c.get('role'))} &nbsp;·&nbsp; <b>Atributo:</b> "
                     f"{esc(ATTRIBUTE.get(c.get('attribute'), c.get('attribute')))} &nbsp;·&nbsp; <b>Armas:</b> {esc(weapons)}<br>"
                     f"<b>Passiva — {esc(passive.get('name'))}:</b> {esc(passive.get('text'))}</div>")
        parts.append("<div class='skills'>")
        for i, s in enumerate(skills.get(c["id"], []), 1):
            code = icons.get(s.get("id"))
            path = thumb(code)
            img = f"<img src='file:///{path.replace(os.sep, '/')}'>" if path and os.path.exists(path) else "<div class='noimg'></div>"
            kind = KIND.get(s.get("kind"), s.get("kind"))
            element = ELEMENT.get(s.get("element"), s.get("element"))
            numbers = f"mana {s.get('mana', 0)} · recarga {s.get('cooldown', 0)} s"
            if s.get("power"):
                numbers += f" · poder {s['power']}"
            parts.append(f"<div class='skill'>{img}<div><div class='t'><i>{n}.{i}</i>{esc(s.get('name'))}</div>"
                         f"<div class='k'>{esc(kind)} · {esc(element)} · {esc(numbers)}</div>"
                         f"<div class='d'>{esc(clean(s.get('name'), s.get('description')))}</div>"
                         f"<div class='box'>☐ ok &nbsp; ☐ feia &nbsp; ☐ bugada</div></div></div>")
        parts.append("</div>")
        asc = c.get("ascensions") or []
        if asc:
            parts.append("<div class='asc'><b>Ascensões (nível 50):</b> " +
                         " &nbsp;·&nbsp; ".join(f"<b>{esc(a.get('name'))}</b>: {esc(a.get('description'))}" for a in asc) + "</div>")
        parts.append("</div>")
    parts.append("</body></html>")

    os.makedirs(OUT, exist_ok=True)
    page = os.path.join(OUT, "CLASSES_E_HABILIDADES.html")
    pdf = os.path.join(OUT, "CLASSES_E_HABILIDADES.pdf")
    open(page, "w", encoding="utf-8").write("".join(parts))
    print_pdf(page, pdf)
    print("PDF:", pdf, os.path.getsize(pdf) // 1024, "KB,", len(classes), "classes")


def print_pdf(page, pdf):
    """Imprime um HTML em PDF com o Edge sem janela (sem cabeçalho nem rodapé)."""
    subprocess.run([EDGE, "--headless=new", "--disable-gpu", "--no-pdf-header-footer", "--allow-file-access-from-files",
                    f"--print-to-pdf={pdf}", "file:///" + page.replace(os.sep, "/")], check=True, timeout=180,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


if __name__ == "__main__":
    main()
