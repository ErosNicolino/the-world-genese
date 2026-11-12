"""O som do poder de cada espada (07/11, o usuário: "os sons de cada golpe"), sintetizado aqui (sem licença a cuidar)
-> Data/Audio/sw_<chave>.wav (o jogo toca no começo do poder: VortexSwords::OnSpecialPose). 07/11: e os dos arcos
(bw_<chave>, de Data/Rpg/bows.json: a corda estala e as flechas assobiam).

Três camadas, pelo Data/Rpg/swords.json:
  o golpe (o tipo do poder): o vento da investida, os cortes da rajada, a batida do golpe no chão, o giro do
      redemoinho, o zumbido da runa, a carga da sobrecarga, o sino do golpe sagrado...
  o elemento: fogo crepitando, gelo cristalino, o estalo do raio, folhas, bolhas de veneno, água, o coro da luz,
      a sombra grave;
  a raridade: das raras para cima, a cauda mais longa e um brilho agudo no fim.
    python Tools/MakeSwordSounds.py      (depois: Tools/ImportExtraSfx.py com o editor fechado)
"""
import json
import os
import wave
import zlib

import numpy as np

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "Data", "Audio")
SR = 44100
RANK = {"common": 0, "uncommon": 1, "rare": 2, "epic": 3, "legendary": 4, "unique": 5, "divine": 6}


def T(d):
    return np.arange(int(SR * d)) / SR


def env(n, attack=.01, decay=4.0):
    t = np.arange(n) / SR
    a = np.clip(t / max(attack, 1e-4), 0, 1)
    return a * np.exp(-t * decay)


def lowpass(x, cut):
    """Filtro de um polo (simples e estável)."""
    a = np.exp(-2 * np.pi * cut / SR)
    y = np.zeros_like(x)
    acc = 0.0
    for i, v in enumerate(x):
        acc = (1 - a) * v + a * acc
        y[i] = acc
    return y


def band(x, lo, hi):
    return lowpass(x, hi) - lowpass(x, lo)


def whoosh(d, f0, f1, rng, level=1.0):
    """O vento da lâmina: ruído num filtro que varre de f0 a f1, subindo e descendo."""
    n = int(SR * d)
    noise = rng.standard_normal(n)
    out = np.zeros(n)
    steps = 24
    for k in range(steps):
        a, b = k * n // steps, (k + 1) * n // steps
        f = f0 + (f1 - f0) * (k / steps)
        out[a:b] = band(noise[a:b], f * .6, f * 1.6)
    return level * out * np.sin(np.pi * np.arange(n) / n) ** 1.5


def thud(d, f, rng, level=1.0):
    t = T(d)
    s = np.sin(2 * np.pi * f * t * (1 - .3 * t / d)) * np.exp(-t * 9) + .4 * lowpass(rng.standard_normal(len(t)), 300) * np.exp(-t * 20)
    return level * s


def chime(d, freqs, decay=3.0, level=1.0):
    t = T(d)
    return level * sum(np.sin(2 * np.pi * f * t) * np.exp(-t * decay * (1 + i * .3)) / (i + 1) for i, f in enumerate(freqs))


def place(buf, s, at):
    i = int(SR * at)
    e = min(len(buf), i + len(s))
    if e > i:
        buf[i:e] += s[:e - i]


def kind_layer(kind, rng):
    buf = np.zeros(int(SR * 1.6))
    if kind == "dash":
        place(buf, whoosh(.35, 400, 2400, rng, 1.2), 0)
        place(buf, thud(.3, 120, rng, .6), .3)
    elif kind == "flurry":
        for i in range(6):
            place(buf, whoosh(.14, 900, 2600, rng, .8), i * .17)
    elif kind == "sweep":
        place(buf, whoosh(.6, 250, 900, rng, 1.3), 0)
        place(buf, thud(.4, 90, rng, .5), .5)
    elif kind == "groundslam":
        place(buf, whoosh(.3, 300, 1200, rng, .8), 0)
        place(buf, thud(.8, 60, rng, 1.4), .28)
    elif kind == "reap":
        place(buf, whoosh(.45, 600, 2000, rng, 1.1), 0)
        place(buf, whoosh(.3, 1500, 500, rng, .6), .35)
    elif kind == "shockwave":
        place(buf, thud(.7, 55, rng, 1.2), 0)
        place(buf, lowpass(rng.standard_normal(int(SR * .9)), 900) * env(int(SR * .9), .02, 3.5) * 1.4, .05)
    elif kind == "whirl":
        for i in range(5):
            place(buf, whoosh(.24, 500 + 150 * (i % 2), 1800, rng, .9), i * .22)
    elif kind == "runeseal":
        t = T(1.2)
        place(buf, (np.sin(2 * np.pi * 220 * t) + .5 * np.sin(2 * np.pi * 331 * t)) * np.sin(np.pi * t / 1.2) * .5, 0)
        place(buf, thud(.5, 80, rng, 1.0), .9)
    elif kind == "overload":
        t = T(.9)
        f = 120 + 900 * (t / .9) ** 2
        place(buf, np.sign(np.sin(2 * np.pi * np.cumsum(f) / SR)) * .25 * (t / .9), 0)
        place(buf, thud(.5, 100, rng, 1.0) + lowpass(rng.standard_normal(int(SR * .5)), 4000) * env(int(SR * .5), .005, 9) * .6, .85)
    elif kind == "orbit":
        for i in range(5):
            place(buf, chime(.6, (880 * 1.5 ** (i % 3), 1320 * 1.2 ** (i % 2)), 5, .5), i * .2)
    elif kind == "firewall":
        place(buf, lowpass(rng.standard_normal(int(SR * 1.4)), 1200) * env(int(SR * 1.4), .25, 1.6) * 1.6, 0)
    elif kind == "holyslam":
        place(buf, chime(1.4, (523, 659, 784, 1046), 1.6, .8), 0)
        place(buf, thud(.6, 70, rng, 1.0), .15)
    elif kind == "heal":
        place(buf, chime(1.5, (440, 554, 659, 880), 1.2, .9), 0)
    # ---- 07/11: os arcos (a corda estala e as flechas assobiam)
    elif kind == "volley":
        place(buf, twang(rng, 1.0), 0)
        for i in range(5):
            place(buf, whoosh(.3, 1400 + 200 * i, 3200, rng, .5), .03 + i * .025)
    elif kind == "pierce":
        place(buf, twang(rng, 1.3), 0)
        place(buf, whoosh(.55, 2600, 700, rng, 1.2), .04)
        place(buf, thud(.3, 140, rng, .5), .5)
    elif kind == "explosive":
        place(buf, twang(rng, 1.0), 0)
        place(buf, whoosh(.35, 1800, 900, rng, .7), .04)
        place(buf, thud(.7, 50, rng, 1.3), .42)
        place(buf, lowpass(rng.standard_normal(int(SR * .7)), 1400) * env(int(SR * .7), .01, 4) * 1.3, .42)
    elif kind == "arrowrain":
        place(buf, twang(rng, 1.0), 0)
        for i in range(10):
            place(buf, whoosh(.2, 3000, 1200, rng, .35), .35 + rng.uniform(0, .6))
            place(buf, thud(.12, 300, rng, .25), .6 + rng.uniform(0, .6))
    elif kind == "burst3":
        for i in range(3):
            place(buf, twang(rng, .8), i * .15)
            place(buf, whoosh(.18, 1600, 3000, rng, .4), i * .15 + .02)
    elif kind == "fireball":
        place(buf, twang(rng, .8), 0)
        place(buf, lowpass(rng.standard_normal(int(SR * 1.2)), 900) * env(int(SR * 1.2), .15, 2.2) * 1.4, .05)
    return buf


def twang(rng, level=1.0):
    """A corda do arco soltando: um estalo grave que vibra e some rápido."""
    t = T(.35)
    f = 110 * (1 + .25 * np.exp(-t * 30))
    s = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-t * 14) + .5 * lowpass(rng.standard_normal(len(t)), 2500) * np.exp(-t * 60)
    return level * s


def element_layer(element, rng, n):
    buf = np.zeros(n)
    t = np.arange(n) / SR
    if element == "fire":
        crack = (rng.random(n) < .004) * rng.uniform(-1, 1, n)
        buf += lowpass(crack, 5000) * 3 + lowpass(rng.standard_normal(n), 700) * .5 * np.exp(-t * 1.5)
    elif element == "ice":
        for f in (2637, 3520, 4186, 5274):
            place(buf, np.sin(2 * np.pi * f * T(.8)) * np.exp(-T(.8) * 6) * .25, rng.uniform(0, .3))
        place(buf, band(rng.standard_normal(int(SR * .12)), 2000, 8000) * 1.2 * env(int(SR * .12), .001, 30), .05)
    elif element == "lightning":
        for k in range(4):
            z = rng.standard_normal(int(SR * .08)) * env(int(SR * .08), .001, 25)
            place(buf, np.sign(z) * np.abs(z) ** .5 * .5, k * .07)
        place(buf, lowpass(rng.standard_normal(int(SR * .9)), 250) * env(int(SR * .9), .01, 3) * 1.5, .3)
    elif element in ("nature", "poison"):
        for k in range(10 if element == "nature" else 14):
            at = rng.uniform(0, .9)
            if element == "nature":
                place(buf, band(rng.standard_normal(int(SR * .06)), 1500, 6000) * env(int(SR * .06), .005, 30) * .6, at)
            else:
                f = rng.uniform(300, 900)
                place(buf, np.sin(2 * np.pi * f * T(.07) * (1 + T(.07) * 6)) * env(int(SR * .07), .005, 30) * .35, at)
    elif element == "water":
        place(buf, lowpass(rng.standard_normal(int(SR * .7)), 1800) * env(int(SR * .7), .01, 5) * 1.2, 0)
        for k in range(6):
            place(buf, np.sin(2 * np.pi * rng.uniform(600, 1400) * T(.05)) * env(int(SR * .05), .002, 40) * .3, rng.uniform(.1, .6))
    elif element == "holy":
        vib = 1 + .004 * np.sin(2 * np.pi * 5 * t)
        buf += sum(np.sin(2 * np.pi * f * t * vib) for f in (523, 659, 784)) * .18 * np.sin(np.pi * t / t[-1]) ** .8
    elif element == "shadow":
        buf += (np.sin(2 * np.pi * 55 * t) + .5 * np.sin(2 * np.pi * 82 * t)) * .4 * np.sin(np.pi * t / t[-1])
        rev = lowpass(rng.standard_normal(int(SR * .6)), 900) * np.linspace(0, 1, int(SR * .6)) ** 2
        place(buf, rev * .8, .1)
    return buf


def rarity_layer(rank, n, rng):
    buf = np.zeros(n)
    if rank >= 2:
        sparkle = chime(.9, (2093 * (1 + .02 * rank), 3136), 4.0, .08 * rank)
        place(buf, sparkle, .35 + .05 * rank)
    return buf


def save(name, buf, tail):
    # a cauda (um eco curto): as mais raras soam mais longas
    if tail > 0:   # (+0,3 s: cabe o último eco)
        echo = np.zeros(len(buf) + int(SR * (tail + .3)))
        echo[:len(buf)] += buf
        for k, g in ((.09, .35), (.17, .22), (.29, .14)):
            i = int(SR * k)
            echo[i:i + len(buf)] += buf * g
        buf = echo
    buf = buf / (np.max(np.abs(buf)) + 1e-9) * .89
    fade = int(SR * .03)
    buf[-fade:] *= np.linspace(1, 0, fade)
    data = (buf * 32767).astype(np.int16)
    with wave.open(os.path.join(OUT, name + ".wav"), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(data.tobytes())
    return len(buf) / SR


if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    # 07/11: as espadas (sw_<chave>) e os arcos (bw_<chave>)
    tables = [("sw_", json.load(open(os.path.join(ROOT, "Data", "Rpg", "swords.json"), encoding="utf-8"))["swords"]),
              ("bw_", json.load(open(os.path.join(ROOT, "Data", "Rpg", "bows.json"), encoding="utf-8"))["bows"])]
    made = 0
    for prefix, item, sw in [(pre, i, e) for pre, table in tables for i, e in table.items()]:
        kind = sw["power"].get("kind")
        if not kind:
            continue
        rng = np.random.default_rng(zlib.crc32(sw["key"].encode()))   # a mesma semente sempre (o hash do Python muda)
        rank = RANK[sw["rarity"]]
        base = kind_layer(kind, rng)
        mix = base + element_layer(sw["element"], rng, len(base)) * (.8 if sw["element"] else 0) + rarity_layer(rank, len(base), rng)
        secs = save(prefix + sw["key"], mix, .1 * rank)
        made += 1
        print("som " + prefix + "%-12s %-10s %-9s %.2f s" % (sw["key"], kind, sw["element"] or "-", secs))
    # a conferência: cada som existe, tem duração e não estoura
    for prefix, item, sw in [(pre, i, e) for pre, table in tables for i, e in table.items()]:
        if sw["power"].get("kind"):
            with wave.open(os.path.join(OUT, "%s%s.wav" % (prefix, sw["key"]))) as w:
                frames = np.frombuffer(w.readframes(w.getnframes()), dtype=np.int16)
                assert frames.size > SR * .5 and np.abs(frames).max() < 32767
    print("sons:", made)
