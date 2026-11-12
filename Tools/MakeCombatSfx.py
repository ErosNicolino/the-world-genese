"""As vozes dos monstros que não tinham som próprio e os sons das armas (03/11): gravações CC0 em camadas.

python Tools/MakeCombatSfx.py

Fontes: Data/Sfx/CombatCC0 (OpenGameArt, CC0; a lista está em LICENCAS.txt).
Sai em Data/Audio/<nome>_v1..v3.wav (48 kHz, mono): o VortexAudio sorteia uma variação a cada vez.
Na Unreal: Tools/ImportExtraSfx.py.

Quem toca:
- <bicho>_hit / _death: VortexAudio::CreatureVoice (o bicho apanhando e morrendo);
- <bicho>_attack: AVortexMonster::PlayMoveSound (o grito quando começa o golpe);
- <bicho>_strike: VortexCombatComponent (o golpe do bicho acertando o herói; antes todos tocavam a mordida do lobo);
- wfx_<arma>_<tipo>: VortexAudio::WeaponSound (martelo, adagas, cajado e arco; a espada já tem as gravações do Godot).
"""
import glob
import os

import numpy as np
import soundfile as sf
from scipy import signal

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "Data", "Sfx", "CombatCC0")
OUT = os.path.join(ROOT, "Data", "Audio")
SR = 48000
_cache = {}


def load(name):
    """Um arquivo da pasta CombatCC0 pelo nome (mono, 48 kHz), sem o silêncio do começo."""
    if name not in _cache:
        path = next(p for p in glob.glob(os.path.join(SRC, "**", "*"), recursive=True) if os.path.basename(p) == name)
        data, rate = sf.read(path, always_2d=True)
        mono = data.mean(axis=1).astype(np.float64)
        if rate != SR:
            mono = signal.resample_poly(mono, SR, rate)
        loud = np.flatnonzero(np.abs(mono) > np.abs(mono).max() * .04)
        _cache[name] = mono[max(0, loud[0] - 48):] if len(loud) else mono
    return _cache[name].copy()


def pitch(x, factor):
    """Tom e duração juntos: factor > 1 deixa agudo e curto (bicho pequeno), < 1 grave e lento (bicho grande)."""
    return signal.resample(x, max(16, int(len(x) / factor)))


def filt(x, kind, freq, order=4):
    return signal.sosfilt(signal.butter(order, freq, btype=kind, fs=SR, output="sos"), x)


def fade(x, fade_in=.003, fade_out=.08):
    x = x.copy()
    a, b = int(fade_in * SR), int(fade_out * SR)
    if a:
        x[:a] *= np.linspace(0, 1, a)
    if b and b < len(x):
        x[-b:] *= np.linspace(1, 0, b) ** 2
    return x


def cut(x, seconds):
    return fade(x[:int(seconds * SR)], .002, min(.08, seconds * .3))


def thump(seconds=.25, f0=140.0, f1=55.0, decay=18.0):
    """O peso de uma pancada: um seno grave que cai rápido."""
    t = np.arange(int(seconds * SR)) / SR
    freq = f1 + (f0 - f1) * np.exp(-t * 30)
    return np.sin(2 * np.pi * np.cumsum(freq) / SR) * np.exp(-t * decay)


def air(rng, seconds, lo, hi, peak_at=.45):
    """Um whoosh: ruído filtrado que cresce até o meio e some."""
    n = int(seconds * SR)
    u = np.linspace(0, 1, n)
    env = np.where(u < peak_at, (u / peak_at) ** 2, ((1 - u) / (1 - peak_at)) ** 1.6)
    return filt(rng.standard_normal(n), "bandpass", [lo, hi], 2) * env


def mix(length, layers):
    out = np.zeros(int(length * SR))
    for x, start, gain in layers:
        a = int(start * SR)
        b = min(len(out), a + len(x))
        if a < len(out):
            out[a:b] += x[:b - a] * gain
    return out


def save(name, x, peak=.85, drive=1.3):
    x = filt(x, "highpass", 40, 2)
    x = np.tanh(x / (np.abs(x).max() + 1e-9) * drive)
    x = fade(x / (np.abs(x).max() + 1e-9) * peak, .001, .03)
    sf.write(os.path.join(OUT, name + ".wav"), x.astype(np.float32), SR, subtype="PCM_16")


def nrm(x):
    return x / (np.abs(x).max() + 1e-9)


# ---- vozes -----------------------------------------------------------------------------------------
# cada função recebe a variação v (0, 1, 2) e o gerador; devolve o som

def goblin_hit(v, rng):
    # o ogro com a voz fina (o goblin é pequeno e chato)
    src = ["ogre1.wav", "ogre3.wav", "ogre5.wav"][v]
    return cut(pitch(load(src), 1.42 + .06 * v), .45)


def goblin_death(v, rng):
    src = ["ogre2.wav", "ogre4.wav", "troll_02.ogg"][v]
    x = pitch(load(src), [1.32, 1.36, 1.15][v])
    return mix(.9, [(fade(x, .003, .3), 0, 1), (cut(pitch(load("cloth-heavy.wav"), .8), .3), .3, .35)])


def goblin_attack(v, rng):
    src = ["grunt_04.ogg", "grunt_05.ogg", "troll_03.ogg"][v]
    return cut(pitch(load(src), [1.25, 1.2, 1.3][v]), .45)


def goblin_strike(v, rng):
    # a clava de pau: madeira batendo e o peso
    wood = pitch(load("wood-small.wav"), [.62, .7, .56][v])
    return mix(.4, [(cut(wood, .3), 0, 1), (thump(.25, 150, 60), 0, .8), (cut(pitch(load("cloth-heavy.wav"), 1.1), .15), .005, .4)])


def bones(rng, count, spread, k=1.0):
    """Ossos chacoalhando: estalinhos de madeira e contas em sequência."""
    layers = []
    for i in range(count):
        src = "wood-small.wav" if i % 2 == 0 else "beads.wav"
        layers.append((cut(pitch(load(src), (1.2 + rng.random() * .6) * k), .12), i * spread * (.7 + rng.random() * .6), .9 - i * .08))
    return mix(count * spread + .3, layers)


def skeleton_hit(v, rng):
    rattle = filt(bones(rng, 3, .045, .7), "lowpass", 6000)
    return mix(.5, [(rattle, 0, 1), (thump(.18, 170, 70, 20), 0, .5), (cut(pitch(load("shade14.wav"), 1.3 + .1 * v), .25), .02, .45)])


def skeleton_death(v, rng):
    # desmonta: um chiado e os ossos caindo no chão
    hiss = cut(pitch(load(["shade15.wav", "shade14.wav", "shade5.wav"][v]), 1.3), .45)
    return mix(1.3, [(hiss, 0, .55), (bones(rng, 7, .09, .85), .15, 1), (thump(.2, 120, 50), .25, .4), (thump(.2, 110, 45), .55, .3)])


def skeleton_attack(v, rng):
    return cut(pitch(load(["shade14.wav", "shade15.wav", "shade1.wav"][v]), [1.4, 1.35, 1.25][v]), .4)


def skeleton_strike(v, rng):
    # a espada velha enferrujada acertando
    blade = cut(pitch(load(["metal-small2.wav", "metal-small1.wav", "metal-small3.wav"][v]), .85), .3)
    return mix(.4, [(blade, 0, .8), (thump(.2, 160, 70), 0, .7), (cut(load("swing2.wav"), .1), 0, .3)])


def undead_hit(v, rng):
    return cut(pitch(load(["shade2.wav", "shade3.wav", "shade4.wav"][v]), .85), .5)


def undead_death(v, rng):
    moan = pitch(load(["shade8.wav", "shade9.wav", "shade12.wav"][v]), .82)
    return mix(1.8, [(fade(moan, .005, .5), 0, 1), (cut(pitch(load("cloth-heavy.wav"), .6), .35), .9, .5), (thump(.3, 100, 40, 10), .95, .5)])


def undead_attack(v, rng):
    return cut(pitch(load(["shade6.wav", "shade7.wav", "shade10.wav"][v]), .9), .6)


def undead_strike(v, rng):
    # a garra: um rasgo de pano e o baque
    return mix(.4, [(cut(pitch(load("cloth.wav"), 1.3 + .1 * v), .2), 0, .8), (thump(.25, 130, 55), 0, .9)])


def spider_hit(v, rng):
    return cut(pitch(load(["bug_01.ogg", "bug_03.ogg", "bug_04.ogg"][v]), .9), .4)


def spider_death(v, rng):
    squeal = filt(pitch(load(["bug_02.ogg", "bug_01.ogg", "bug_04.ogg"][v]), .62), "lowpass", 5000)
    legs = mix(.6, [(cut(pitch(load("wood-small.wav"), 1.4 + rng.random() * .5), .06), i * .07, .5) for i in range(6)])
    return mix(1.1, [(fade(squeal, .003, .25), 0, 1), (legs, .25, .6), (thump(.25, 120, 50, 12), .3, .5)])


def spider_attack(v, rng):
    return filt(cut(pitch(load(["eat_03.ogg", "bug_03.ogg", "bug_01.ogg"][v]), [.8, .9, .85][v]), .35), "lowpass", 6500)


def spider_strike(v, rng):
    return mix(.4, [(cut(load(["bite-small.wav", "bite-small2.wav", "bite-small3.wav"][v]), .3), 0, 1), (thump(.15, 200, 90, 25), 0, .4)])


def frog_hit(v, rng):
    return cut(pitch(load(["burp_01.ogg", "burp_02.ogg", "burble_02.ogg"][v]), [1.25, 1.3, 1.4][v]), .4)


def frog_death(v, rng):
    croak = pitch(load(["burble_01.ogg", "burble_02.ogg", "burp_02.ogg"][v]), .8)
    return mix(1.2, [(fade(croak, .003, .35), 0, 1), (cut(pitch(load("slime6.wav"), .7), .35), .45, .6)])


def frog_attack(v, rng):
    # o coaxar grave antes da língua
    return cut(pitch(load(["burp_01.ogg", "burp_02.ogg", "misc_04.ogg"][v]), [.72, .78, .85][v]), .5)


def frog_strike(v, rng):
    return mix(.35, [(cut(load(["slime1.wav", "slime3.wav", "slime5.wav"][v]), .3), 0, 1), (thump(.18, 140, 60), 0, .6)])


def slime_strike(v, rng):
    return mix(.35, [(cut(pitch(load(["slime4.wav", "slime6.wav", "slime8.wav"][v]), .85), .3), 0, 1), (thump(.2, 120, 50), 0, .7)])


def golem_strike(v, rng):
    rock = pitch(load("wood-small.wav"), [.35, .4, .32][v])
    return mix(.6, [(cut(rock, .45), 0, .9), (thump(.45, 120, 38, 9), 0, 1)])


def mushroom_strike(v, rng):
    return mix(.35, [(cut(pitch(load("cloth-heavy.wav"), .9 + .1 * v), .25), 0, 1), (thump(.2, 110, 50), 0, .6)])


def bat_strike(v, rng):
    return cut(pitch(load(["bite-small.wav", "bite-small2.wav", "bite-small3.wav"][v]), 1.35), .25)


def bandit_strike(v, rng):
    blade = cut(pitch(load(["metal-small2.wav", "metal-small3.wav", "metal-small1.wav"][v]), 1.0), .28)
    return mix(.4, [(blade, 0, .7), (thump(.2, 170, 70), 0, .8)])


# ---- armas -----------------------------------------------------------------------------------------

def hammer_swing(v, rng, i):
    # o martelo pesado: o whoosh grave e lento (cada golpe da combinação um pouco diferente)
    src = ["swing.wav", "swing2.wav", "swing3.wav"][i]
    x = pitch(load(src), .55 + .04 * v)
    return mix(.6, [(fade(filt(x, "lowpass", 2500), .01, .12), .03, 1), (air(rng, .45 + .05 * i, 120, 900, .55), 0, .7)])


def hammer_hit(v, rng):
    wood = pitch(load("wood-small.wav"), .45 + .05 * v)
    ring = filt(pitch(load("metal-ringing.wav"), .6), "lowpass", 3000)
    return mix(.8, [(thump(.5, 130, 42, 8), 0, 1), (cut(wood, .4), 0, .8), (cut(ring, .6), .005, .3)])


def dagger_swing(v, rng, i):
    # a lâmina leve: curtinha, aguda e rápida
    x = pitch(load(["swing3.wav", "swing.wav", "swing2.wav"][i]), 1.35 + .07 * v)
    shing = filt(cut(load(["sword-unsheathe2.wav", "sword-unsheathe4.wav", "sword-unsheathe5.wav"][i]), .14), "highpass", 3000)
    return mix(.3, [(fade(x, .002, .06), 0, 1), (shing, .02, .2)])


def dagger_hit(v, rng):
    blade = cut(pitch(load(["metal-small2.wav", "metal-small1.wav", "metal-small3.wav"][v]), 1.25), .2)
    return mix(.3, [(blade, 0, .55), (cut(pitch(load("cloth.wav"), 1.5), .15), 0, .7), (thump(.15, 220, 90, 25), 0, .6)])


def staff_cast(v, rng):
    x = pitch(load("magic1.wav"), [1.0, 1.12, .9][v])
    return fade(x, .003, .3)


def staff_shot(v, rng):
    # a bolinha de magia saindo: um brilho curto e o ar
    sparkle = cut(pitch(load("magic1.wav"), 1.6 + .15 * v), .35)
    return mix(.45, [(sparkle, 0, .9), (air(rng, .35, 1200, 6000, .2), 0, .5)])


def staff_hit(v, rng):
    pop = cut(pitch(load(["bubble.wav", "bubble2.wav", "bubble3.wav"][v]), .7), .3)
    shine = cut(pitch(load("magic1.wav"), 1.9), .3)
    return mix(.5, [(pop, 0, .8), (shine, .01, .5), (thump(.2, 180, 70, 16), 0, .6)])


def bow_hit(v, rng):
    # a flecha cravando: madeira e o baque seco
    wood = pitch(load("wood-small.wav"), [1.0, .9, 1.1][v])
    return mix(.35, [(cut(wood, .2), 0, .9), (thump(.18, 190, 80, 22), 0, .7), (cut(pitch(load("cloth.wav"), 1.4), .12), .003, .4)])


def bow_draw(v, rng):
    # a corda esticando: madeira rangendo devagar
    creak = pitch(load("wood-small.wav"), [.28, .31, .26][v])
    return fade(filt(creak, "bandpass", [300, 5000]), .1, .15)


VOICES = {
    "goblin": (goblin_hit, goblin_death, goblin_attack, goblin_strike),
    "skeleton": (skeleton_hit, skeleton_death, skeleton_attack, skeleton_strike),
    "undead": (undead_hit, undead_death, undead_attack, undead_strike),
    "spider": (spider_hit, spider_death, spider_attack, spider_strike),
    "frog": (frog_hit, frog_death, frog_attack, frog_strike),
}
STRIKES = {"slime": slime_strike, "golem": golem_strike, "mushroom": mushroom_strike, "bat": bat_strike, "bandit": bandit_strike}


def main():
    os.makedirs(OUT, exist_ok=True)
    rng = np.random.default_rng(3011)
    made = []

    def put(name, fn, *args, **kw):
        for v in range(3):
            save(f"{name}_v{v + 1}", fn(v, rng, *args), **kw)
        made.append(name)

    for voice, (hit, death, attack, strike) in VOICES.items():
        put(f"{voice}_hit", hit)
        put(f"{voice}_death", death)
        put(f"{voice}_attack", attack, peak=.75)
        put(f"{voice}_strike", strike)
    for voice, strike in STRIKES.items():
        put(f"{voice}_strike", strike)
    for i in range(3):
        put(f"wfx_hammer_swing_{i + 1}", hammer_swing, i, drive=1.1)
        put(f"wfx_dagger_swing_{i + 1}", dagger_swing, i, drive=1.1)
    put("wfx_hammer_hit", hammer_hit, drive=1.6)
    put("wfx_dagger_hit", dagger_hit)
    put("wfx_staff_cast", staff_cast, peak=.7, drive=1.0)
    put("wfx_staff_shot", staff_shot, peak=.7, drive=1.0)
    put("wfx_staff_hit", staff_hit)
    put("wfx_bow_hit", bow_hit)
    put("wfx_bow_draw", bow_draw, peak=.75, drive=1.8)
    print(f"{len(made)} sons x 3 variações em {OUT}:", " ".join(made))


if __name__ == "__main__":
    main()
