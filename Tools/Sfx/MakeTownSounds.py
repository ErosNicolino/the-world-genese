"""Os sons da cidade e do clima (11/11, o usuário: "bichos na cidade e som de cada lugar").

python Tools/Sfx/MakeTownSounds.py   ->   Data/Sfx/Town/*.wav (mono, 44,1 kHz, 16 bits, volume igualado)
Depois: Tools/ImportTownSounds.py (commandlet) leva para /Game/Vortex/Audio/Sfx (os loops com "Looping").

De onde vem cada um (todos de uso livre, sem crédito obrigatório):
- OpenGameArt, CC0 (`modelando ainda/sons_baixados/opengameart`): gato (Kerzoven: cat_softmew, cat_mewpurr,
  cat_mewfood, cat_purrsleepy_loop), cachorro (dog_barking_mono, licença dupla OGA-BY/CC0: usamos a CC0), ovelha
  (sheep_baa), martelo na bigorna (forger_0, "Hammer on Anvil") e a chuva em loop (amb_rain_loop_1);
- SONNISS GDC 2024 (uso comercial sem crédito; a pasta do projeto Godot `_Externos`): porco, galo, cavalo, as pancadas de
  ferro, o fogo do fogão a lenha, a conversa da taverna (walla de interior) e a tempestade;
- sintetizados aqui: o cacarejo das galinhas (não achei um CC0).
"""
import glob
import os

import numpy as np
import soundfile as sf
from scipy.signal import resample_poly

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OGA = os.path.join(ROOT, "modelando ainda", "sons_baixados", "opengameart")
SONNISS = os.path.join(os.path.dirname(os.path.dirname(ROOT)), "the-world-geneses", "_Externos", "Sonniss GDC 2024")
OUT = os.path.join(ROOT, "Data", "Sfx", "Town")
RATE = 44100
rng = np.random.default_rng(1111)


def load(path, start=0.0, length=None):
    x, rate = sf.read(path, dtype="float32", always_2d=True)
    x = x.mean(axis=1)
    if rate != RATE:
        g = np.gcd(rate, RATE)
        x = resample_poly(x, RATE // g, rate // g).astype(np.float32)
    a = int(start * RATE)
    b = len(x) if length is None else min(len(x), a + int(length * RATE))
    return x[a:b]


def sonniss(folder, part):
    hits = sorted(glob.glob(os.path.join(SONNISS, folder + "*", "*%s*.wav" % part)))   # o nome da pasta pode ter apóstrofo curvo
    assert hits, (folder, part)
    return hits[0]


def level(x, rms_db, peak_db=-1.0):
    """Volume igualado pelo RMS das partes com som (sem o silêncio), com o pico limitado."""
    win = 2048
    frames = x[: len(x) // win * win].reshape(-1, win) if len(x) >= win else x[None, :]
    energy = np.sqrt((frames ** 2).mean(axis=1) + 1e-12)
    loud = energy[energy > energy.max() * .1]
    rms = float(np.sqrt((loud ** 2).mean())) if len(loud) else 1e-6
    y = x * (10 ** (rms_db / 20) / rms)
    peak = np.abs(y).max() + 1e-9
    limit = 10 ** (peak_db / 20)
    return y * min(1.0, limit / peak)


def fade(x, a=.01, b=.05):
    y = x.copy()
    na, nb = int(a * RATE), int(b * RATE)
    if na: y[:na] *= np.linspace(0, 1, na)
    if nb: y[-nb:] *= np.linspace(1, 0, nb)
    return y


def loop(x, cross=1.5):
    """Emenda o fim no começo (o loop toca sem estalo)."""
    n = int(cross * RATE)
    head, tail = x[:n], x[-n:]
    t = np.linspace(0, 1, n)
    mixed = tail * np.cos(t * np.pi / 2) + head * np.sin(t * np.pi / 2)
    return np.concatenate([mixed, x[n:-n]])


def pieces(x, count, min_len=.15, max_len=1.6, gap=.12):
    """Os trechos com som (latidos, pancadas, grunhidos), do mais forte para o mais fraco."""
    win = 512
    env = np.sqrt(np.convolve(x ** 2, np.ones(win) / win, mode="same"))
    on = env > env.max() * .12
    found, i = [], 0
    while i < len(on):
        if on[i]:
            j = i
            quiet = 0
            while j < len(on) and quiet < gap * RATE:
                quiet = quiet + 1 if not on[j] else 0
                j += 1
            a, b = max(0, i - int(.02 * RATE)), j - quiet + int(.05 * RATE)
            if min_len * RATE <= b - a <= max_len * RATE:
                found.append((float(env[a:b].max()), a, b))
            i = j
        else:
            i += 1
    found.sort(reverse=True)
    assert found, "sem trechos"
    return [fade(x[a:b]) for _, a, b in found[:count]]


def pitch(x, factor):
    up, down = int(round(1000 * factor)), 1000
    return resample_poly(x, down, up).astype(np.float32)


def hen(seed):
    """Um "cócó" de galinha: 2 a 4 sílabas curtas, tom de 400-700 Hz com harmônicos e um pouco de ruído."""
    r = np.random.default_rng(seed)
    out = []
    for k in range(r.integers(2, 5)):
        dur = r.uniform(.07, .13) if k else r.uniform(.12, .2)
        t = np.arange(int(dur * RATE)) / RATE
        f0 = r.uniform(420, 620) * (1.25 if k == 0 else 1) * np.linspace(1.08, .92, len(t))
        phase = 2 * np.pi * np.cumsum(f0) / RATE
        voice = sum(np.sin(phase * h) / h ** 1.3 for h in range(1, 7))
        noise = np.convolve(r.normal(0, 1, len(t)), np.ones(6) / 6, mode="same") * .35
        env = np.sin(np.pi * np.minimum(1, t / dur)) ** .6 * np.exp(-t / (dur * .9))
        out += [(voice + noise) * env, np.zeros(int(r.uniform(.05, .11) * RATE))]
    return fade(np.concatenate(out).astype(np.float32), .002, .02)


def write(name, x):
    sf.write(os.path.join(OUT, name + ".wav"), np.clip(x, -1, 1), RATE, subtype="PCM_16")
    print("SOM %-18s %5.1f s" % (name, len(x) / RATE))


os.makedirs(OUT, exist_ok=True)
# ---- bichos (efeitos curtos, -20 dB RMS: o jogo baixa com a distância)
for n, f in enumerate(("cat_softmew", "cat_mewpurr", "cat_mewfood"), 1):
    write("anim_cat_%d" % n, level(fade(load(os.path.join(OGA, f + ".wav"))), -22))
write("anim_cat_purr", level(loop(load(os.path.join(OGA, "cat_purrsleepy_loop.wav")), .5), -30))
for n, x in enumerate(pieces(load(os.path.join(OGA, "dog_barking_mono.wav")), 3), 1):
    write("anim_dog_%d" % n, level(x, -20))
baa = level(fade(load(os.path.join(OGA, "sheep_baa.flac"))), -22)
for n, k in enumerate((1.0, .92, 1.08), 1):
    write("anim_sheep_%d" % n, pitch(baa, k))
for n, x in enumerate(pieces(load(sonniss("Mechanical Wave - Farm Animals", "Pig")), 3, max_len=1.2), 1):
    write("anim_pig_%d" % n, level(x, -22))
rooster = pieces(load(sonniss("Mechanical Wave - Farm Animals", "Rooster")), 1, min_len=.8, max_len=3.5, gap=.25)[0]
write("anim_rooster", level(rooster, -20))
for n in range(1, 5):
    write("anim_hen_%d" % n, level(hen(n * 17), -24))
write("anim_horse", level(pieces(load(sonniss("UberDuo - Horsin", "Snort")), 1, max_len=2.0)[0], -22))
# ---- lugares da cidade
hits = pieces(load(os.path.join(OGA, "forger_0.ogg")), 2, max_len=3.0, gap=.08)
iron = load(sonniss("Pole Position - The Metal Hit Sweeteners Library", "Iron"))
start = int(np.argmax(np.abs(iron) > np.abs(iron).max() * .2))   # uma pancada só, com o soar do ferro
hits.append(fade(iron[max(0, start - 200): start + int(1.6 * RATE)], .002, .5))
for n, x in enumerate(hits, 1):
    write("town_anvil_%d" % n, level(x, -18))
write("town_fire", level(loop(load(sonniss("UberDuo - The Wood Stove Audio Prop Set", "Crackling"), 5, 24)), -28))
write("town_tavern", level(loop(load(sonniss("Jake Fielding - Neutral Interior Walla", "WALLA"), 3, 32), 2.5), -26))
# ---- clima (loops longos, mais baixos: tocam em volta da câmera)
write("amb_rain", level(loop(load(os.path.join(OGA, "amb_rain_loop_1.wav"), 0, 40), 2.0), -26))
write("amb_rain_storm", level(loop(load(sonniss("Bolt - Backyard Rain & Thunder - Suburban Rain Recordings", "Thunderstorm"), 20, 50), 3.0), -24))
write("amb_wind_snow", level(loop(load(sonniss("Sonik Sound Library - Spatial Countryside", "Howling"), 5, 40), 3.0), -28))
print("SONS_CIDADE prontos em", OUT)
