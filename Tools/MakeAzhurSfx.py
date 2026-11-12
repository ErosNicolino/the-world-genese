"""Os sons do Azhûr, o golem vulcânico (02/11): gravações de verdade em camadas, mais graves (é um colosso de 7 m).

python Tools/MakeAzhurSfx.py

Fontes (as licenças estão nas pastas):
- Data/Sfx/AzhurSonniss: Sonniss GDC 2024 (impacto de pedra, impacto cinematográfico, explosões, desmoronamento,
  rosnado de criatura, whooshes, fogo de fogão a lenha, caverna);
- Data/Sfx/AzhurCC0: OpenGameArt CC0 (rugido grave de monstro, bolhas, fogo crepitando).
Cada som sai em 3 variações (Data/Audio/azhur_<nome>_v1..v3.wav, 48 kHz, mono): o VortexAudio sorteia uma.
Na Unreal: Tools/ImportExtraSfx.py. Quem toca: UVortexVolcanicBoss (nos instantes do Data/Creatures/azhur.json).
"""
import glob
import os

import numpy as np
import soundfile as sf
from scipy import signal

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SON = os.path.join(ROOT, "Data", "Sfx", "AzhurSonniss")
CC0 = os.path.join(ROOT, "Data", "Sfx", "AzhurCC0")
OUT = os.path.join(ROOT, "Data", "Audio")
SR = 48000
_cache = {}


def load(pattern):
    """Um arquivo (mono, 48 kHz, float)."""
    if pattern in _cache:
        return _cache[pattern].copy()
    path = next(iter(glob.glob(os.path.join(SON, pattern)) + glob.glob(os.path.join(CC0, pattern))))
    data, rate = sf.read(path, always_2d=True)
    mono = data.mean(axis=1)
    if rate != SR:
        mono = signal.resample_poly(mono, SR, rate)
    _cache[pattern] = mono.astype(np.float64)
    return _cache[pattern].copy()


def pitch(x, factor):
    """Muda o tom e a duração juntos (como baixar a rotação de uma fita): factor < 1 deixa grave e lento."""
    n = int(len(x) / factor)
    return signal.resample(x, max(16, n))


def filt(x, kind, freq, order=4):
    sos = signal.butter(order, freq, btype=kind, fs=SR, output="sos")
    return signal.sosfilt(sos, x)


def span(x, start, seconds):
    a = int(start * SR)
    return x[a:a + int(seconds * SR)]


def fade(x, fade_in=.005, fade_out=.2):
    x = x.copy()
    a, b = int(fade_in * SR), int(fade_out * SR)
    if a:
        x[:a] *= np.linspace(0, 1, a)
    if b and b < len(x):
        x[-b:] *= np.linspace(1, 0, b) ** 2
    return x


def sub(seconds, f0=55.0, f1=28.0, decay=4.0):
    """O peso: um seno grave que desce e some (o "corpo" de um impacto gigante)."""
    t = np.arange(int(seconds * SR)) / SR
    freq = f1 + (f0 - f1) * np.exp(-t * 3.0)
    phase = 2 * np.pi * np.cumsum(freq) / SR
    return np.sin(phase) * np.exp(-t * decay)


def rumble(seconds, rng, cutoff=90.0, swell=.3):
    """Tremor do chão: ruído grave, entra devagar e sai devagar."""
    n = int(seconds * SR)
    x = filt(rng.standard_normal(n), "lowpass", cutoff, 6)
    env = np.minimum(1, np.arange(n) / (swell * SR + 1)) * np.linspace(1, 0, n) ** 1.5
    return x / (np.abs(x).max() + 1e-9) * env


def riser(seconds, rng, lo=60.0, hi=900.0):
    """A carga do núcleo: ruído filtrado que sobe e um tom que sobe junto, crescendo até o fim."""
    n = int(seconds * SR)
    t = np.arange(n) / SR
    u = t / seconds
    noise = rng.standard_normal(n)
    out = np.zeros(n)
    block = 2048
    for i in range(0, n, block):
        f = lo + (hi - lo) * (u[min(i, n - 1)] ** 2)
        seg = noise[i:i + block]
        out[i:i + block] = filt(seg, "bandpass", [max(20, f * .6), min(SR / 2 - 100, f * 1.6)], 2)
    freq = 38 + 160 * u ** 2.2
    tone = np.sin(2 * np.pi * np.cumsum(freq) / SR) * .5 + np.sin(2 * np.pi * np.cumsum(freq * 1.5) / SR) * .2
    return (out / (np.abs(out).max() + 1e-9) * .7 + tone) * u ** 1.3


def mix(length, layers):
    """layers: [(som, começo em s, ganho)]."""
    out = np.zeros(int(length * SR))
    for x, start, gain in layers:
        a = int(start * SR)
        if a >= len(out):
            continue
        b = min(len(out), a + len(x))
        out[a:b] += x[:b - a] * gain
    return out


def master(x, peak=.89):
    x = filt(x, "highpass", 22, 2)
    x = np.tanh(x / (np.abs(x).max() + 1e-9) * 1.6)   # cola as camadas e dá corpo
    return x / (np.abs(x).max() + 1e-9) * peak


def save(name, x):
    os.makedirs(OUT, exist_ok=True)
    sf.write(os.path.join(OUT, name + ".wav"), master(x).astype(np.float32), SR, subtype="PCM_16")


# ---- os sons ---------------------------------------------------------------------------------------

def roar(rng, k):
    """O rugido (despertar, troca de fase): o rugido grave mais baixo ainda, o rosnado de criatura por baixo e o chão."""
    r = pitch(load("monster_roar.wav"), .72 * k)
    growl = pitch(load("CREAMisc_Creature*"), .55 * k)
    horn = pitch(load("DSGNBram_Cinematic Horn*"), .8)
    return mix(5.0, [(fade(r, .05, .8), .15, 1.0), (fade(filt(growl, "lowpass", 1500), .1, .5), .35, .55),
                     (fade(horn, .3, 1.5), 0, .25), (rumble(4.5, rng, 70, .8), 0, .6)])


def step(rng, k):
    """O passo: a pedra batendo grave e o tranco do chão (curto)."""
    hit = pitch(span(load("Bluezone_BC0297_stone_impact_015.wav"), 0, .9), .5 * k)
    return mix(1.2, [(fade(filt(hit, "lowpass", 2500), .002, .5), 0, .9), (sub(1.0, 70, 32, 6), 0, .9)])


def whoosh(rng, k):
    """O braço de pedra cortando o ar (soco e varredura): whoosh pesado bem mais grave."""
    w = pitch(load("WHSH_Sci-Fi Heavy Whoosh*"), .85 * k)
    air = pitch(load("WHSH_Airy-Whoosh*"), .8 * k)
    return mix(2.2, [(fade(filt(filt(w, "lowpass", 5000), "highpass", 90), .05, .6), 0, 1.0), (fade(filt(air, "highpass", 200), .05, .6), .05, .6)])


def slam(rng, k):
    """O punho no chão: impacto de pedra + estrondo cinematográfico + o peso grave + pedras caindo."""
    stone = pitch(load("Bluezone_BC0297_stone_impact_041.wav"), .6 * k)
    boom = pitch(load("Bluezone_BC0294_modern_cinematic_impact_boom_003.wav"), .85 * k)
    debris = pitch(load("WOODCrsh_*"), .55 * k)
    return mix(3.0, [(fade(stone, .002, .8), 0, 1.0), (fade(boom, .002, 1.2), 0, .8), (sub(2.0, 60, 26, 2.5), 0, 1.1),
                     (fade(filt(debris, "lowpass", 4000), .01, .8), .08, .5)])


def charge(rng, k):
    """O núcleo carregando (Coração Incandescente, Raio, Despertar): o tom subindo, fogo crepitando e o chão."""
    fire = span(load("FIREMisc_Fire Crackling*"), 2.0 + rng.uniform(0, 4), 3.0)
    return mix(3.2, [(riser(3.0, rng), 0, .9), (fade(filt(fire, "highpass", 300), .8, .3), 0, .5),
                     (rumble(3.0, rng, 80, 1.5), 0, .7)])


def explode(rng, k):
    """A explosão do núcleo: a explosão "nuclear" desenhada, o estrondo e o peso."""
    nuke = pitch(span(load("EXPLDsgn_Nuclear*"), 0, 6.0), .8 * k)
    boom = pitch(load("DSGNBoom_Cinematic Hit*"), .9 * k)
    return mix(5.5, [(fade(nuke, .002, 2.0), 0, 1.0), (fade(boom, .002, 2.0), 0, .7), (sub(3.0, 50, 22, 1.4), 0, 1.0)])


def fissure(rng, k):
    """A fenda de lava abrindo: pedra rachando, lava borbulhando e o chiado do fogo."""
    crack = pitch(load("Bluezone_BC0297_stone_impact_steel_bar*"), .45 * k)
    bubbles = pitch(load("bubbles-loop1-amp.wav"), .55 * k)
    hiss = filt(load("fire-1.wav"), "highpass", 1500)
    return mix(2.6, [(fade(crack, .002, .4), 0, .7), (fade(bubbles, .2, .8), .1, .8), (fade(hiss, .1, .8), .05, .35),
                     (rumble(2.4, rng, 120, .2), 0, .5)])


def meteor(rng, k):
    """A rocha em brasa arremessada (sai da mão chiando)."""
    w = pitch(load("WHSH_Airy-Whoosh*"), .7 * k)
    fire = span(load("fire-1.wav"), 0, 2.0)
    return mix(2.4, [(fade(w, .1, .8), 0, 1.0), (fade(fire, .1, .6), .1, .6)])


def meteor_land(rng, k):
    """O meteoro caindo: explosão realista, pedras e o peso."""
    expl = pitch(load("EXPLReal_Medium*"), .7 * k)
    debris = pitch(load("WOODCrsh_*"), .6 * k)
    return mix(3.2, [(fade(expl, .002, 1.0), 0, 1.0), (fade(debris, .01, .8), .06, .5), (sub(2.0, 64, 30, 2.6), 0, .9)])


def beam(rng, k):
    """O raio do núcleo (2,6 s): o rugido do fogo, o zumbido grave pulsando e o chiado na ponta."""
    n = 2.8
    t = np.arange(int(n * SR)) / SR
    hum = (np.sin(2 * np.pi * 46 * k * t) + .5 * np.sin(2 * np.pi * 92 * k * t + 1) + .25 * np.sin(2 * np.pi * 139 * t)) * (.8 + .2 * np.sin(2 * np.pi * 7 * t))
    fire = span(load("FIREMisc_Fire Crackling*"), 4 + rng.uniform(0, 5), n)
    roar_ = pitch(span(load("EXPLDsgn_Nuclear*"), 1.5, 3.5), 1.0)
    return mix(n, [(fade(hum, .15, .5), 0, .5), (fade(filt(fire, "highpass", 200), .1, .5), 0, .7),
                   (fade(filt(roar_, "lowpass", 900), .2, .6), 0, .6)])


def eruption(rng, k):
    """Uma coluna de lava estourando: bolha grande, o jato de fogo e o chão."""
    pop = pitch(load("bubbles-single2.wav"), .35 * k)
    jet = pitch(load("WHSH_Watery-Whoosh*"), .55 * k)
    boom = pitch(load("Bluezone_BC0294_modern_cinematic_impact_percussion*"), .7 * k)
    return mix(2.2, [(fade(pop, .002, .3), 0, .8), (fade(jet, .02, .8), .05, .9), (fade(boom, .002, .8), 0, .6), (sub(1.2, 70, 35, 4), 0, .6)])


def shed(rng, k):
    """A armadura derretida: a casca de pedra estalando e voando em pedaços."""
    crash = pitch(load("WOODCrsh_*"), .5 * k)
    stone = pitch(load("Bluezone_BC0297_stone_impact_hammer*"), .55 * k)
    expl = pitch(load("EXPLReal_Medium*"), .9 * k)
    return mix(3.0, [(fade(crash, .002, .8), 0, 1.0), (fade(stone, .002, .4), .02, .7), (fade(expl, .002, 1.0), 0, .5)])


def stun(rng, k):
    """Atordoado: um gemido grave cansado e a pedra rangendo."""
    r = pitch(span(load("monster_roar.wav"), .5, 3.0), .5 * k)
    grind = pitch(load("Bluezone_BC0297_stone_impact_015.wav"), .3)
    return mix(3.5, [(fade(filt(r, "lowpass", 600), .3, 1.2), 0, .8), (fade(grind, .01, .8), .2, .5), (rumble(3.0, rng, 60, .5), 0, .4)])


def death(rng, k):
    """A morte: o último rugido apagando, o corpo de pedra desabando e o tremor longo."""
    r = pitch(load("monster_roar.wav"), .6 * k)
    crash = pitch(load("WOODCrsh_*"), .4 * k)
    stone = pitch(load("Bluezone_BC0297_stone_impact_041.wav"), .45)
    return mix(7.0, [(fade(filt(r, "lowpass", 1200), .1, 2.0), 0, .8), (fade(crash, .01, 1.0), 1.4, .9),
                     (fade(stone, .01, 1.0), 2.4, 1.0), (sub(3.5, 55, 22, 1.0), 2.4, 1.0), (rumble(6.0, rng, 70, .5), .5, .6)])


def hurt(rng, k):
    """Levou um golpe: lasca de pedra curta."""
    s = pitch(load("Bluezone_BC0297_stone_impact_hammer*"), .7 * k)
    return mix(.9, [(fade(s, .001, .4), 0, 1.0), (sub(.5, 90, 50, 9), 0, .4)])


SOUNDS = {"roar": roar, "step": step, "whoosh": whoosh, "slam": slam, "charge": charge, "explode": explode,
          "fissure": fissure, "meteor": meteor, "meteor_land": meteor_land, "beam": beam, "eruption": eruption,
          "shed": shed, "stun": stun, "death": death, "hurt": hurt}

if __name__ == "__main__":
    for name, make in SOUNDS.items():
        for v in range(1, 4):
            rng = np.random.default_rng(hash((name, v)) & 0xFFFF)
            k = [1.0, .94, 1.06][v - 1]   # cada variação um pouco mais grave ou mais aguda
            save("azhur_%s_v%d" % (name, v), make(rng, k))
        print("AZHUR_SFX", name)
