"""As 24 armas do Godot (somente leitura) em Data/Rpg/weapons.json.

python Tools/BuildWeaponData.py

Para cada tipo de arma (data/weapon_types.json do Godot):
- nome, ala do Estádio do Despertar, se é magia, texto do golpe primário;
- os 3 golpes do clique esquerdo: clipe, duração, início do acerto, dano (unidades do Godot), atordoamento,
  modo (melee, slam, arrow, bolt, orb, burst), alcance (m), cone, raio (m), velocidade do projétil (m/s),
  velocidade do clipe e elemento. As 5 armas iniciais têm os golpes em Game/Player/WeaponStyles.cs
  (copiados aqui); as outras vêm do json;
- o especial do botão direito (kind, nome, texto e os números);
- a pegada (inclinação na mão, como no WeaponDefinition.cs do Godot), se é de par (cópia na mão
  esquerda), a postura (clipes de parado/andar/correr da família) e o modelo (.glb do item comum).
Também escreve a lista de clipes que o Tools/ConvertGodotAdventurer.py precisa converter.
"""
import json
import pathlib

ROOT = pathlib.Path(__file__).resolve().parents[1]
GODOT = ROOT.parent.parent / "the-world-geneses"
OUT = ROOT / "Data" / "Rpg"


def swing(clip, duration, impact, active, damage, stun, mode, reach, min_dot=0.0, radius=0.0, sound="sword_swing_1",
          speed=24.0, clip_speed=1.0, element=None):
    return {"clip": clip, "duration": duration, "impact": impact, "active_start": active, "damage": damage, "stun": stun,
            "mode": mode, "reach": reach, "min_dot": min_dot, "radius": radius, "sound": sound, "speed": speed,
            "clip_speed": clip_speed, "element": element}


# Game/Player/WeaponStyles.cs (os tipos novos trazem os golpes no json)
STARTER_SWINGS = {
    "lanca_do_patrulheiro": [
        swing("Spear_Attack_01", .5, .22, .16, 1, .3, "melee", 3.9, .75, sound="spear_thrust"),
        swing("Spear_Attack_02", .5, .22, .16, 1, .3, "melee", 3.9, .75, sound="spear_thrust"),
        swing("Spear_Attack_03", .8, .4, .32, 3, .6, "melee", 4.6, .7, sound="spear_thrust")],
    "martelo_de_bram": [
        swing("Hammer_Attack_01", .8, .45, .38, 2, .45, "melee", 3.2, -.1, sound="hammer_swing"),
        swing("Hammer_Attack_02", .8, .45, .38, 2, .45, "melee", 3.2, -.1, sound="hammer_swing"),
        swing("Hammer_Attack_03", 1.1, .62, .55, 4, 1.0, "slam", 0, radius=3.4, sound="hammer_slam")],
    "cajado_de_ilsa": [
        swing("Staff_Attack_01", .55, .3, .26, 1, .2, "bolt", 34, sound="staff_cast", speed=30),
        swing("Staff_Attack_02", .55, .3, .26, 1, .2, "bolt", 34, sound="staff_cast", speed=30),
        swing("Staff_Attack_03", .9, .5, .46, 3, .6, "orb", 30, radius=2.8, sound="staff_blast", speed=22)],
    "arco_de_betula": [
        swing("Bow_Attack_01", .6, .34, .3, 1, .15, "arrow", 16, sound="bow_release", speed=28),
        swing("Bow_Attack_02", .6, .34, .3, 1, .15, "arrow", 16, sound="bow_release", speed=28),
        swing("Bow_Attack_03", .95, .62, .58, 3, .4, "arrow", 20, sound="bow_release", speed=34)],
}
# inclinação da pegada (graus; positivo = ponta para baixo) — WeaponDefinition.cs; None = como o modelo vem
GRIP = {"espada_de_cinzas": 40, "adagas_gemeas": 40, "katana_do_vento": 40, "lanca_do_patrulheiro": -15, "martelo_de_bram": 55,
        "cajado_de_ilsa": -50, "arco_de_betula": None, "manoplas_de_ferro": None, "livro_da_aurora": None,
        "espada_escudo": 40, "espadao": 40, "machado": 55, "machados_duplos": 40, "maca_sagrada": 55, "alabarda": -15,
        "besta": 0, "pistola_runica": 0, "chakrams": 40, "varinha": -50, "orbe": -75, "instrumento": -30,
        "catalisador": -75, "foice": -15, "chicote": 40, "correntes": 40}
PAIRED = {"adagas_gemeas", "manoplas_de_ferro", "machados_duplos", "chakrams"}
# prefixo dos clipes de postura da família (Adventurer.glb do Godot)
STANCE = {"espada_escudo": "SwordShield", "espadao": "Greatsword", "machado": "Axe", "machados_duplos": "DualAxes",
          "lanca_do_patrulheiro": "Spear", "alabarda": "Halberd", "adagas_gemeas": "Dagger", "manoplas_de_ferro": "Fist",
          "maca_sagrada": "Mace", "katana_do_vento": "Katana", "besta": "Crossbow", "pistola_runica": "Pistol",
          "chakrams": "Chakram", "cajado_de_ilsa": "Staff", "varinha": "Wand", "livro_da_aurora": "Book", "orbe": "Orb",
          "instrumento": "Lute", "foice": "Scythe", "chicote": "Whip", "correntes": "Chain", "catalisador": "Flask"}
RARITY = ["common", "uncommon", "rare", "epic", "legendary", "unique"]



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
    types = json.loads((GODOT / "data" / "weapon_types.json").read_text(encoding="utf-8"))["types"]
    items = json.loads((GODOT / "data" / "items.json").read_text(encoding="utf-8"))["items"]
    clips = json.loads(pathlib.Path(ROOT / "Data" / "Rpg" / "adventurer_clips.json").read_text(encoding="utf-8")) \
        if (ROOT / "Data" / "Rpg" / "adventurer_clips.json").exists() else None
    weapons, wanted = [], set()
    for t in types:
        tid = t["id"]
        swings = t.get("swings") or STARTER_SWINGS.get(tid) or []
        # o modelo: a própria arma inicial, ou o item mais comum da família
        if (GODOT / "Assets" / "Equipment" / "Weapons" / (tid + ".glb")).exists():
            item = tid
        else:
            family = sorted((i for i, d in items.items() if d.get("type") == "weapon" and d.get("family") == tid),
                            key=lambda i: RARITY.index(items[i].get("rarity", "common")) if items[i].get("rarity", "common") in RARITY else 9)
            item = next((i for i in family if (GODOT / "Assets" / "Equipment" / "Weapons" / (i + ".glb")).exists()), None)
        stance = STANCE.get(tid)
        weapon = {
            "id": tid, "name": t["name"], "wing": t["wing"], "magic": t["magic"], "primary": t["primary"],
            "baseFamily": t["base_family"], "icon": t["icon"].replace("/", "_"), "item": item,
            "grip": GRIP.get(tid), "paired": tid in PAIRED, "swings": swings, "secondary": t["secondary"],
            "stance": {k: f"{stance}_{k}" for k in ("Idle", "Walk", "Run")} if stance else {},
        }
        weapons.append(weapon)
        for s in swings:
            wanted.add(s["clip"])
        for c in weapon["stance"].values():
            wanted.add(c)
        for key in ("clip", "hold_clip"):
            if t["secondary"].get(key):
                wanted.add(t["secondary"][key])
    OUT.mkdir(parents=True, exist_ok=True)
    data = {"_doc": "Gerado por Tools/BuildWeaponData.py a partir do Godot (somente leitura). Não editar à mão.",
            "types": weapons}
    # tipos tirados do jogo (Espada e Escudo: REVISAO_ARMAS, 11/10/2026)
    data = _retire(data)
    (OUT / "weapons.json").write_text(json.dumps(data, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    # os clipes da espada já vêm do Adventurer_SwordAttacks
    wanted -= {"Attack_01", "Attack_02", "Attack_03"}
    (OUT / "weapon_clips.txt").write_text(",".join(sorted(wanted)) + "\n", encoding="utf-8")
    print("WEAPON_DATA", len(weapons), "tipos", len(wanted), "clipes", "sem modelo:", [w["id"] for w in weapons if not w["item"]])


if __name__ == "__main__":
    main()
