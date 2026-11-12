"""Sons do portal (estilo Solo Leveling) -> Data/Sfx/Portal/*.wav, a partir dos sons CC0 da Kenney (Sci-Fi Sounds,
Data/Sfx/PortalKenney, com a licença) misturados com um zumbido e um brilho sintetizados aqui.

  portal_hum    o zumbido do portal aberto (em laço, sem emenda)
  portal_open   o portal rasgando o ar (quando aparece / chega perto)
  portal_enter  entrar no portal (o puxão)
    python Tools/MakePortalAssets.py      (depois: Tools/ImportPortalAssets.py com o editor fechado)
"""
import os
import wave
import numpy as np
import soundfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "Data", "Sfx", "PortalKenney")
OUT = os.path.join(ROOT, "Data", "Sfx", "Portal")
os.makedirs(OUT, exist_ok=True)
SR = 44100


def load(name):
    data, rate = soundfile.read(os.path.join(SRC, name + ".ogg"), dtype="float64")
    if data.ndim > 1:
        data = data.mean(axis=1)
    if rate != SR:
        x = np.linspace(0, len(data) - 1, int(len(data) * SR / rate))
        data = np.interp(x, np.arange(len(data)), data)
    return data / (np.abs(data).max() + 1e-9)


def save(name, s, peak=.8, fade=True):
    s = s / (np.abs(s).max() + 1e-9) * peak
    if fade:
        f = int(SR * .02)
        s[:f] *= np.linspace(0, 1, f)
        s[-f:] *= np.linspace(1, 0, f)
    with wave.open(os.path.join(OUT, name + ".wav"), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes((s * 32767).astype(np.int16).tobytes())


def mix(a, b, at=0.0, gain=1.0):
    i = int(at * SR)
    n = max(len(a), i + len(b))
    out = np.zeros(n)
    out[:len(a)] += a
    out[i:i + len(b)] += b * gain
    return out


t = lambda d: np.arange(int(SR * d)) / SR

# zumbido em laço: o motor baixo da Kenney + um acorde grave que pulsa e um brilho agudo; o fim é cruzado com o
# começo para não estalar quando repete
dur = 4.0
x = t(dur)
base = load("spaceEngineLow_000")
base = np.tile(base, int(np.ceil(len(x) / len(base))) + 1)[:len(x)]
drone = (np.sin(2 * np.pi * 55 * x) + .6 * np.sin(2 * np.pi * 82.5 * x) + .3 * np.sin(2 * np.pi * 110 * x)) * (.75 + .25 * np.sin(2 * np.pi * .5 * x))
shimmer = np.sin(2 * np.pi * (1760 + 40 * np.sin(2 * np.pi * .25 * x)) * x) * .08 * (.5 + .5 * np.sin(2 * np.pi * 1.5 * x))
hum = base * .6 + drone * .35 + shimmer
cross = int(SR * .5)
head = hum[:cross].copy()
hum = hum[cross:]
hum[-cross:] = hum[-cross:] * np.linspace(1, 0, cross) + head * np.linspace(0, 1, cross)
save("portal_hum", hum, .7, fade=False)

# abrir: o campo de força rasgando + o estrondo grave + um brilho subindo
rise = np.sin(2 * np.pi * np.cumsum(np.linspace(300, 2400, int(SR * .9))) / SR) * np.linspace(0, 1, int(SR * .9)) ** 2 * .25
s = mix(load("forceField_001"), load("lowFrequency_explosion_000"), .05, .9)
s = mix(s, rise, .0, 1.0)
save("portal_open", s, .85)

# entrar: o puxão (o motor girando) + o campo de força + um estalo grave no fim
e = load("engineCircular_001")[:int(SR * 1.2)]
s = mix(e * np.linspace(.2, 1, len(e)), load("forceField_003"), .3, 1.0)
s = mix(s, load("lowFrequency_explosion_000"), .95, .8)
save("portal_enter", s, .85)
print("VORTEX_PORTAL_SONS", sorted(os.listdir(OUT)))
