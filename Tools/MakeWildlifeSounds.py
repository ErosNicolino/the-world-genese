"""Sons dos bichos do mundo (pássaros e peixes) -> Data/Sfx/Wildlife/*.wav, sintetizados aqui (sem licença a cuidar).

  bird_chirp_1..3  pio curto de passarinho (varre a nota para cima/baixo, com trinado)
  bird_crow        grasnado do corvo (Morthúria, Lodonegro)
  bird_gull        grito da gaivota (no mar e na costa)
  bird_flutter     bater de asas do bando levantando voo
  fish_plop        peixe pulando e caindo na água
    python Tools/MakeWildlifeSounds.py      (depois: Tools/ImportWildlifeAssets.py com o editor fechado)
"""
import os
import wave
import numpy as np

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "Data", "Sfx", "Wildlife")
os.makedirs(OUT, exist_ok=True)
SR = 44100
rng = np.random.default_rng(11)
t = lambda d: np.arange(int(SR * d)) / SR


def save(name, s, peak=.8):
    s = s / (np.abs(s).max() + 1e-9) * peak
    f = int(SR * .005)
    s[:f] *= np.linspace(0, 1, f)
    s[-f:] *= np.linspace(1, 0, f)
    with wave.open(os.path.join(OUT, name + ".wav"), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes((s * 32767).astype(np.int16).tobytes())


def tone(freq, env):
    """Seno com a frequência variando (freq e env do mesmo tamanho)."""
    phase = 2 * np.pi * np.cumsum(freq) / SR
    return np.sin(phase) * env


def silence(d):
    return np.zeros(int(SR * d))


def lowpass(s, k):
    return np.convolve(s, np.ones(k) / k, mode="same")


# pios: notas curtas (40-120 ms) varrendo 2,5-6 kHz, umas com trinado
def chirp(f0, f1, dur, trill=0.0, rate=0.0):
    x = t(dur)
    sweep = f0 + (f1 - f0) * (x / dur) ** .7
    if trill:
        sweep = sweep * (1 + trill * np.sin(2 * np.pi * rate * x))
    env = np.sin(np.pi * np.clip(x / dur, 0, 1)) ** 1.5
    return tone(sweep, env) + .15 * tone(sweep * 2, env)


save("bird_chirp_1", np.concatenate([chirp(3200, 5200, .07), silence(.04), chirp(3400, 5600, .06), silence(.05), chirp(5000, 2800, .12)]))
save("bird_chirp_2", np.concatenate([chirp(4200, 4600, .22, .08, 38), silence(.06), chirp(5200, 3600, .08)]))
save("bird_chirp_3", np.concatenate([chirp(2600, 4800, .05) for _ in range(1)] + [silence(.03), chirp(2800, 5000, .05), silence(.03), chirp(3000, 5200, .05), silence(.05), chirp(4400, 2400, .16, .05, 20)]))

# corvo: "cráá" rouco: dente de serra grave com ruído e formantes, duas vezes
def caw(dur):
    x = t(dur)
    f = 520 - 140 * (x / dur)
    phase = np.cumsum(f) / SR
    saw = 2 * (phase % 1) - 1
    rough = saw * (1 + .5 * rng.standard_normal(len(x)))
    env = np.minimum(1, x / .02) * np.exp(-((x / dur) ** 2) * 1.5)
    s = lowpass(rough * env, 6)
    return s + .3 * tone(f * 3, env)


save("bird_crow", np.concatenate([caw(.32), silence(.12), caw(.28)]), .7)

# gaivota: "kiiá" agudo que sobe e cai, com vibrato
def gull(dur):
    x = t(dur)
    u = x / dur
    f = 1400 + 900 * np.sin(np.pi * u) ** 2
    f = f * (1 + .03 * np.sin(2 * np.pi * 22 * x))
    env = np.sin(np.pi * u) ** .8
    return tone(f, env) + .45 * tone(f * 2, env) + .2 * tone(f * 3, env)


save("bird_gull", np.concatenate([gull(.38), silence(.08), gull(.22), silence(.06), gull(.2)]), .6)

# bater de asas: rajadas de ruído filtrado a ~14 batidas por segundo, sumindo
x = t(1.1)
noise = lowpass(rng.standard_normal(len(x)), 5)
beat = np.clip(np.sin(2 * np.pi * 14 * x * (1 - .25 * x)), 0, 1) ** 3
flutter = noise * beat * np.exp(-x * 2.2)
# vários pássaros fora de passo
for k in range(3):
    shift = int(SR * rng.uniform(.01, .08))
    flutter[shift:] += .6 * (noise * beat * np.exp(-x * 2.6))[:len(x) - shift]
save("bird_flutter", flutter, .7)

# peixe: "plic" (bolha que sobe de nota) e respingo curto
x = t(.35)
plop = tone(380 + 900 * np.clip(x / .05, 0, 1), np.exp(-x * 40))
splash = lowpass(rng.standard_normal(len(x)), 3) * np.exp(-np.maximum(0, x - .04) * 18) * (x > .04)
save("fish_plop", plop + .5 * splash, .6)

print("VORTEX_WILDLIFE_SOUNDS ok:", sorted(os.listdir(OUT)))
