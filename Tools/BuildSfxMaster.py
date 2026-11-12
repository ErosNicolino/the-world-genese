"""Mixagem final dos sons do jogo -> Data/SfxMaster (depois: Tools/ImportSfxMaster.py com o editor fechado).

    python Tools/BuildSfxMaster.py          (precisa de numpy, scipy e soundfile)

O que faz, em cada som:
- Escolhe a melhor fonte que existe (nesta ordem): os sons novos da fauna, as armas (Data/WeaponSfx/out), o portal e,
  por último, o som antigo do Godot (the-world-geneses/Assets/Sfx, só leitura).
- Traz as gravações reais do Godot (Assets/Sfx/recorded/<nome>_v1..v4: Freesound, SONNISS e Ovani, com licença de uso
  comercial; créditos em Data/Sfx/CREDITOS_SONS.txt). O jogo sorteia uma das quatro a cada vez (VortexAudio).
- Corta o silêncio do começo (11 golpes começavam com mais de 50 ms de nada) e o do fim, com rampas curtas.
- Iguala o volume pela categoria (a diferença chegava a 20 dB dentro da mesma categoria): mede a média do trecho que soa
  e leva ao alvo da categoria; as 4 variações de um som recebem o mesmo ganho (a diferença entre elas fica).
  Pico no máximo -1 dB (o excesso é arredondado, sem estalar).
- Ambientes (amb_*): gravações reais da biblioteca SONNISS GDC 2024 (uso comercial sem crédito) no lugar dos
  sintetizados, 40 s em estéreo com a volta sem emenda (o fim se funde no começo; 9 dos antigos estalavam na volta).
- Fauna: pios e corvo de gravações CC0 do OpenGameArt (Data/Sfx/WildlifeCC0, com as licenças), asas de bando a partir
  do "Large Wings Flap" (CC0) e o peixe de uma gravação de água do SONNISS. A gaivota continua sintetizada (não achei
  gravação de gaivota com licença limpa).
- Prévia para ouvir antes e depois: Saved/SfxPreview/index.html.
Relatório em Saved/Logs/BuildSfxMaster.txt.
"""
import glob
import html
import json
import os
import re
from math import gcd

import numpy as np
import soundfile as sf
from scipy.signal import butter, resample_poly, sosfilt

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GODOT = os.path.normpath(os.path.join(ROOT, "..", "..", "the-world-geneses"))
GODOT_SFX = os.path.join(GODOT, "Assets", "Sfx")
RECORDED = os.path.join(GODOT_SFX, "recorded")
SONNISS = os.path.join(GODOT, "_Externos", "Sonniss GDC 2024")
CC0 = os.path.join(ROOT, "Data", "Sfx", "WildlifeCC0")
OUT = os.path.join(ROOT, "Data", "SfxMaster")
PREVIEW = os.path.join(ROOT, "Saved", "SfxPreview")
SR = 44100
rng = np.random.default_rng(7)
report = []


# ---- leitura e escrita ----------------------------------------------------------------------------------------------
def load(path, mono=True, start=0.0, length=None):
    info = sf.info(path)
    frames = -1 if length is None else int(length * info.samplerate)
    x, rate = sf.read(path, dtype="float64", always_2d=True, start=int(start * info.samplerate), frames=frames)
    if x.shape[1] > 2:
        x = x[:, :2]   # quadrifônico: os dois da frente
    if mono:
        x = x.mean(axis=1, keepdims=True)
    elif x.shape[1] == 1:
        x = np.repeat(x, 2, axis=1)
    if rate != SR:
        g = gcd(int(rate), SR)
        x = resample_poly(x, SR // g, int(rate) // g, axis=0)
    return x


def save(path, x):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    sf.write(path, np.clip(x, -1.0, 1.0), SR, subtype="PCM_16")


def preview(name, before, after):
    """Os dois lados em OGG pequeno para a página de prévia."""
    os.makedirs(os.path.join(PREVIEW, "a"), exist_ok=True)
    os.makedirs(os.path.join(PREVIEW, "d"), exist_ok=True)
    def ogg(path, x):
        # em pedaços: a libsndfile fecha o programa sem aviso gravando OGG longo de uma vez
        x = np.clip(x, -1, 1)
        with sf.SoundFile(path, "w", SR, x.shape[1], format="OGG", subtype="VORBIS") as f:
            for i in range(0, len(x), 4096):
                f.write(x[i:i + 4096])
    if before is not None:
        ogg(os.path.join(PREVIEW, "a", name + ".ogg"), before[: SR * 20])
    ogg(os.path.join(PREVIEW, "d", name + ".ogg"), after[: SR * 20])


# ---- medidas e acabamento -------------------------------------------------------------------------------------------
def active_rms_db(x):
    """Média do trecho que soa (janelas de 20 ms acima de -50 dB do pico)."""
    m = x.mean(axis=1)
    win = int(SR * .02)
    n = max(1, len(m) // win)
    frames = m[: n * win].reshape(n, win) if len(m) >= win else m.reshape(1, -1)
    rms = np.sqrt((frames ** 2).mean(axis=1)) + 1e-12
    peak = np.abs(m).max() + 1e-12
    loud = rms[rms > peak * 10 ** (-50 / 20)]
    if len(loud) == 0:
        loud = rms
    return 20 * np.log10(np.sqrt((loud ** 2).mean()))


def soft_limit(x, ceiling=10 ** (-1 / 20), knee=.6):
    a = np.abs(x)
    over = a > knee
    room = ceiling - knee
    y = x.copy()
    y[over] = np.sign(x[over]) * (knee + room * np.tanh((a[over] - knee) / room))
    return y


def trim(x, lead_keep=.004, fade_in=.002, fade_out=.015):
    m = np.abs(x).max(axis=1)
    peak = m.max() + 1e-12
    loud = np.nonzero(m > peak * .03)[0]
    if len(loud) == 0:
        return x
    start = max(0, loud[0] - int(lead_keep * SR))
    quiet = np.nonzero(m > peak * .003)[0]
    end = min(len(m), quiet[-1] + int(.03 * SR))
    y = x[start:end].copy()
    fi, fo = int(fade_in * SR), min(int(fade_out * SR), len(y) // 3)
    if fi > 0 and len(y) > fi:
        y[:fi] *= np.linspace(0, 1, fi)[:, None]
    if fo > 0:
        y[-fo:] *= np.linspace(1, 0, fo)[:, None]
    return y


def gain_to(x, target_db, measured=None):
    return x * 10 ** ((target_db - (measured if measured is not None else active_rms_db(x))) / 20)


def band(x, low=None, high=None, order=4):
    if low and high:
        sos = butter(order, [low, high], btype="band", fs=SR, output="sos")
    elif low:
        sos = butter(order, low, btype="high", fs=SR, output="sos")
    else:
        sos = butter(order, high, btype="low", fs=SR, output="sos")
    return sosfilt(sos, x, axis=0)


# ---- categorias e alvos (dB da média do trecho que soa) -------------------------------------------------------------
RULES = [
    (r"^step_", -27), (r"^amb_", -24), (r"^boss_", -15),
    (r"(swing|whoosh|roll|jump|dash|throw|draw|sweep|whirl|_shot$|release|thrust|page|flutter)", -21),
    (r"(hit|impact|slam|bite|break|stomp|punch|crash|land$|block|execute|seismic|rupture|collapse|debris)", -17),
    (r"(cast|^class_|magic|heal|leech|portal|skill|staff|wand|orb|rune|book|lightning|thunder|meteor|nova|beam|vines|tornado|firewall|acid|potion|holy)", -19),
    (r"(wolf|bat_|bird_|golem|slime|creature|hurt|death|rabbit|deer|horse|bandit|mushroom|growl|howl|screech|pounce)", -19),
    (r"(coin|pickup|quest|level_up|buy|sell|map_open|region_enter|dialogue|gate_open|town_bell|yggdrasil_enter)", -20),
    (r"(water|splash|swim|drink|fish|poison)", -20),
]


def target_for(name):
    for pattern, db in RULES:
        if re.search(pattern, name):
            return db
    return -19


# ---- 1) os efeitos de um arquivo só ---------------------------------------------------------------------------------
def single_sources():
    """nome -> arquivo de origem (a melhor fonte que existe)."""
    sources = {}
    for folder in (GODOT_SFX, os.path.join(ROOT, "Data", "Sfx", "Portal"), os.path.join(ROOT, "Data", "WeaponSfx", "out"),
                   os.path.join(ROOT, "Data", "Sfx", "Wildlife")):
        for path in glob.glob(os.path.join(folder, "*.wav")):
            name = os.path.basename(path)[:-4]
            if name.startswith("amb_") or name.startswith("ui_"):
                continue   # ambientes refeitos abaixo; a interface cartoon já está mixada (Data/UI)
            sources[name] = path   # as pastas de depois têm prioridade
    return sources


def build_singles(fauna):
    sources = single_sources()
    done = 0
    for name, path in sorted(sources.items()):
        x = fauna.get(name)
        before = load(path)
        if x is None:
            x = before
        loop = name == "portal_hum"
        if not loop:
            x = trim(x)
        x = soft_limit(gain_to(x, target_for(name)))
        save(os.path.join(OUT, "Sfx", name + ".wav"), x)
        preview(name, before, x)
        done += 1
    report.append(f"efeitos de um arquivo: {done}")
    return set(sources)


# ---- 2) as gravações com variações ----------------------------------------------------------------------------------
def build_recorded():
    groups = {}
    for path in sorted(glob.glob(os.path.join(RECORDED, "*_v[0-9].wav"))):
        name = os.path.basename(path)[:-4]
        base = name[: name.rfind("_v")]
        if base.startswith("ui_"):
            continue
        groups.setdefault(base, []).append((name, path))
    count = 0
    for base, items in sorted(groups.items()):
        takes = [(n, trim(load(p))) for n, p in items]
        # mesmo ganho para as quatro (a diferença entre as tomadas é parte do som)
        level = 20 * np.log10(np.mean([10 ** (active_rms_db(x) / 20) for _, x in takes]))
        for n, x in takes:
            y = soft_limit(gain_to(x, target_for(base), level))
            save(os.path.join(OUT, "Recorded", n + ".wav"), y)
            if n.endswith("_v1"):
                old = os.path.join(GODOT_SFX, base + ".wav")
                preview(base + "_gravado", load(old) if os.path.exists(old) else None, y)
            count += 1
    report.append(f"gravações com variações: {len(groups)} sons, {count} arquivos")
    return groups


# ---- 3) ambientes em laço -------------------------------------------------------------------------------------------
def sonniss(folder, name_part):
    hits = glob.glob(os.path.join(SONNISS, folder, "*" + name_part + "*.wav"))
    if not hits:
        raise FileNotFoundError(folder + " / " + name_part)
    return hits[0]


def seamless(x, length, cross=3.0):
    """length segundos que voltam ao começo sem emenda (o fim se funde no começo, com ganho de potência constante)."""
    n, c = int(length * SR), int(cross * SR)
    x = x[: n + c]
    if len(x) < n + c:
        n = len(x) - c
    out = x[:n].copy()
    t = np.linspace(0, 1, c)[:, None]
    out[:c] = x[:c] * np.sin(t * np.pi / 2) + x[n:n + c] * np.cos(t * np.pi / 2)
    return out


def layer(*parts):
    n = min(len(p) for p, _ in parts)
    return sum(p[:n] * 10 ** (g / 20) for p, g in parts)


AMBIENCES = {
    # nome: (camadas [(pasta, trecho do arquivo, início s, ganho dB, filtro)], duração)
    "amb_day": ([("Justsoundeffects - Forest Ambiences", "Spring Afternoon", 8, 0, None)], 40),
    "amb_night": ([("Systematic Sound - General Ambience Series - Nightscapes 01", "Nighttime-Woodlands", 30, 0, None),
                   ("Justsoundeffects - Forest Ambiences", "Tawny Owls", 4, -9, None)], 40),
    "amb_wind": ([("Justsoundeffects - Forests of Norway", "WINDVege", 20, 0, None)], 40),
    "amb_ocean": ([("Nick McMahan - Cascadia Shore Waves", "Ocean Medium to Large Rocks", 20, 0, None)], 40),
    "amb_storm": ([("Bolt - Backyard Rain & Thunder - Suburban Rain Recordings", "Thunderstorm", 15, 0, None)], 40),
    "amb_swamp": ([("Systematic Sound - General Ambience Series - Rural Countryside 01", "Field Wheat", 20, -2, None),
                   ("Justsoundeffects - Water in Motion", "Soft Movement", 0, -8, ("low", 2500))], 34),
    "amb_snow": ([("Sonik Sound Library - Spatial Countryside", "Soft Wind Howling", 10, 0, ("low", 6000))], 40),
    "amb_desert": ([("Systematic Sound - General Ambience Series - Rural Countryside 01", "Meadow Open Plane", 30, 0, None)], 40),
    "amb_haunted": ([("Systematic Sound - General Ambience Series - Nightscapes 01", "Nighttime-Woodlands", 120, -2, None),
                     ("InMotionAudio - Sinister Textures 2", "Eerie02", 5, -10, None)], 40),
    "amb_fire": ([("UberDuo - The Wood Stove Audio Prop Set", "Fire Crackling", 1, 0, None)], 11),
    "amb_lava": ([("UberDuo - The Wood Stove Audio Prop Set", "Fire Crackling", 1, -3, ("low", 3000)),
                  ("InMotionAudio - Cave Design", "CaveDesign01", 2, -4, ("low", 400))], 11),
    "amb_volcano": ([("InMotionAudio - Cave Design", "CaveDesign01", 1, 0, ("low", 900)),
                     ("Systematic Sound - General Ambience Series - Rural Countryside 01", "Meadow Open Plane", 90, -6, ("low", 500))], 17),
    "amb_town": ([("Jake Fielding - Sounds of Spain", "Town Square 02", 5, 0, None)], 40),
    "amb_fountain": ([("Pole Position - Winter Forest Stream", "Stream - LIGHT", 2, 0, None)], 26),
    "amb_water": ([("Stefano Cremona - Rivers, Streams, Creeks", "Calm_River_02", 10, 0, None)], 40),
    "amb_elven": ([("InMotionAudio - The Forest", "Forest04", 5, 0, None),
                   ("Justsoundeffects - Forests of Norway", "Woodpeckers", 2, -12, None)], 28),
    "amb_yggdrasil": ([("Pole Position - Wind In Trees", "Aspen", 20, -2, None),
                       ("InMotionAudio - The Forest", "Forest04", 30, -4, None)], 40),
}


def build_ambiences():
    for name, (layers, length) in AMBIENCES.items():
        parts = []
        for folder, part, start, db, filt in layers:
            x = load(sonniss(folder, part), mono=False, start=start, length=length + 4)
            if filt:
                x = band(x, high=filt[1]) if filt[0] == "low" else band(x, low=filt[1])
            parts.append((x, db))
        x = seamless(layer(*parts), length)
        x = soft_limit(gain_to(x, target_for(name)))
        save(os.path.join(OUT, "Ambience", name + ".wav"), x)
        old = os.path.join(GODOT_SFX, name + ".wav")
        preview(name, load(old, mono=False)[: len(x)] if os.path.exists(old) else None, x[: SR * 15])
    report.append(f"ambientes: {len(AMBIENCES)} (gravações SONNISS em laço sem emenda)")


# ---- 4) fauna de gravações ------------------------------------------------------------------------------------------
def pick_events(x, count, min_len=.12, max_len=.7, low=2000, high=9000):
    """Os trechos mais fortes e isolados na faixa dos pios (sem o fundo grave)."""
    b = band(x, low=low, high=high)
    env = np.abs(b.mean(axis=1))
    win = int(SR * .01)
    env = np.convolve(env, np.ones(win) / win, mode="same")
    floor = np.median(env) * 4
    events, used = [], np.zeros(len(env), bool)
    order = np.argsort(env)[::-1]
    for i in order:
        if len(events) >= count or env[i] < floor:
            break
        if used[max(0, i - SR // 2): i + SR // 2].any():
            continue
        s = i
        while s > 0 and env[s] > floor and i - s < max_len * SR / 2:
            s -= 1
        e = i
        while e < len(env) - 1 and env[e] > floor and e - i < max_len * SR / 2:
            e += 1
        if (e - s) / SR < min_len:
            s, e = max(0, i - int(min_len * SR / 2)), min(len(env), i + int(min_len * SR / 2))
        used[s:e] = True
        seg = b[max(0, s - int(.01 * SR)): e + int(.05 * SR)]
        events.append(seg)
    return events


def build_fauna():
    fauna = {}
    # pios: os trechos mais limpos das duas gravações CC0, juntando 2-3 pios em cada som
    chirps = pick_events(load(os.path.join(CC0, "birdchirping071414.wav")), 6) + \
        pick_events(load(os.path.join(CC0, "birds-isaiah658_0.ogg")), 6)
    chirps = [trim(c) for c in chirps if len(c) > SR * .05]
    for k in range(3):
        take = [chirps[(k * 3 + j) % len(chirps)] for j in range(2 + k % 2)]
        gap = np.zeros((int(SR * .07), 1))
        fauna[f"bird_chirp_{k + 1}"] = np.concatenate(sum(([t, gap] for t in take), [])[:-1])
    # corvo: a gravação CC0 do Cairo, o grasnado mais forte duas vezes
    crow = load(os.path.join(CC0, "crow_caw.wav"))
    caws = pick_events(crow, 2, min_len=.25, max_len=.8, low=400, high=6000)
    fauna["bird_crow"] = np.concatenate([trim(c) for c in caws[:2]] if len(caws) >= 2 else [trim(crow)])
    # asas: o bater de asas grande (CC0) em três camadas fora de passo e um pouco mais agudas (bando de passarinhos)
    wings = trim(load(os.path.join(CC0, "wings", "wings_flap_large.flac")))
    flock = np.zeros((len(wings) + int(SR * .3), 1))
    for shift, rate, db in ((0, 1.0, 0), (.06, 1.18, -3), (.13, 1.32, -5), (.2, 1.1, -7)):
        g = gcd(int(SR * rate), SR)
        w = resample_poly(wings, SR // g, int(SR * rate) // g, axis=0)
        i = int(shift * SR)
        n = min(len(w), len(flock) - i)
        flock[i:i + n] += w[:n] * 10 ** (db / 20)
    fauna["bird_flutter"] = flock
    # peixe: o impacto na água do SONNISS, curto e um pouco mais agudo (peixe pequeno)
    splash = trim(load(sonniss("InMotionAudio - Submerge", "Impact20")))[: int(SR * .6)]
    g = gcd(int(SR * 1.25), SR)
    fauna["fish_plop"] = resample_poly(splash, SR // g, int(SR * 1.25) // g, axis=0)
    report.append("fauna: pios (CC0), corvo (CC0), asas (CC0), peixe (SONNISS); gaivota sintetizada")
    return fauna


# ---- prévia ---------------------------------------------------------------------------------------------------------
def write_preview():
    names = sorted(os.path.basename(p)[:-4] for p in glob.glob(os.path.join(PREVIEW, "d", "*.ogg")))
    rows = []
    for n in names:
        before = f'<audio controls preload="none" src="a/{n}.ogg"></audio>' if os.path.exists(os.path.join(PREVIEW, "a", n + ".ogg")) else "<em>novo</em>"
        rows.append(f"<tr><td>{html.escape(n)}</td><td>{before}</td><td><audio controls preload='none' src='d/{n}.ogg'></audio></td></tr>")
    page = ("<!doctype html><meta charset='utf-8'><title>Sons: antes e depois</title>"
            "<style>body{font-family:sans-serif;background:#1d2433;color:#eee;padding:16px}td{padding:4px 10px}"
            "tr:nth-child(odd){background:#262f42}audio{height:30px}</style>"
            "<h1>Sons do Vortex: antes e depois</h1><p>Esquerda: o som que o jogo tocava. Direita: o novo "
            "(nome_gravado = a primeira das 4 variações gravadas).</p><table><tr><th>som</th><th>antes</th><th>depois</th></tr>"
            + "".join(rows) + "</table>")
    with open(os.path.join(PREVIEW, "index.html"), "w", encoding="utf-8") as f:
        f.write(page)
    report.append(f"prévia: {len(names)} sons em Saved/SfxPreview/index.html")


if __name__ == "__main__":
    fauna = build_fauna()
    build_singles(fauna)
    build_recorded()
    build_ambiences()
    write_preview()
    # os créditos vão soltos ao lado do jogo empacotado (Config/DefaultGame.ini: Content/Credits)
    os.makedirs(os.path.join(ROOT, "Content", "Credits"), exist_ok=True)
    with open(os.path.join(ROOT, "Data", "Sfx", "CREDITOS_SONS.txt"), encoding="utf-8") as src, \
            open(os.path.join(ROOT, "Content", "Credits", "CREDITOS_SONS.txt"), "w", encoding="utf-8") as dst:
        dst.write(src.read())
    os.makedirs(os.path.join(ROOT, "Saved", "Logs"), exist_ok=True)
    with open(os.path.join(ROOT, "Saved", "Logs", "BuildSfxMaster.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(report) + "\n")
    print("VORTEX_SFX_MASTER " + " | ".join(report))
