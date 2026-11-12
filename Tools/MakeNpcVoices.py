"""As "vozes" dos moradores (08/11, o usuário pediu som quando eles falam): um murmúrio curto de vogal, como em Zelda e
Animal Crossing, sintetizado aqui (sem licença a cuidar) -> Data/Audio/npc_voz_<tom>_<1..4>.wav.

Cada um é um trem de pulsos da garganta no tom da voz (grave, medio, agudo, crianca), com uma subida ou descida curta
de entonação, passado por duas ressonâncias da boca (os formantes de a, e, o, i). O VortexAudio sorteia um tiquinho de
tom a mais a cada toque. Usados no diálogo (a cada 6 letras) e no cumprimento (AVortexNpc::Say).

    python Tools/MakeNpcVoices.py      (depois: Tools/ImportExtraSfx.py com o editor fechado)
"""
import os
import wave

import numpy as np

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "Data", "Audio")
SR = 44100
TONES = {"grave": 105, "medio": 145, "agudo": 225, "crianca": 310}
VOWELS = [("a", 800, 1250), ("e", 450, 1900), ("o", 500, 900), ("i", 320, 2300)]
GLIDES = [1.12, .9, 1.05, .95]          # a entonação de cada variação (sobe, desce)


def resonator(x, f, bw):
    """Filtro de duas casas (uma ressonância da boca)."""
    r = np.exp(-np.pi * bw / SR)
    a1, a2 = -2 * r * np.cos(2 * np.pi * f / SR), r * r
    y = np.zeros_like(x)
    for n in range(len(x)):
        y[n] = x[n] - a1 * (y[n - 1] if n else 0) - a2 * (y[n - 2] if n > 1 else 0)
    return y


def voice(f0, vowel, glide, dur):
    n = int(SR * dur)
    t = np.arange(n) / SR
    freq = f0 * (1 + (glide - 1) * t / dur)
    phase = np.cumsum(freq / SR)
    pulses = (np.diff(np.floor(phase), prepend=0) > 0).astype(np.float64)
    # a forma do pulso da garganta (uma rampa que cai): mais rica que um clique
    src = np.convolve(pulses, np.exp(-np.arange(int(SR * .004)) / (SR * .0012)))[:n]
    src += np.random.default_rng(int(f0 + glide * 100)).normal(0, .03, n)          # um pouco de ar
    _, f1, f2 = vowel
    scale = 1.18 if f0 > 250 else 1.0                                              # boca menor, formantes mais altos
    y = resonator(src, f1 * scale, 90) + .6 * resonator(src, f2 * scale, 130)
    env = np.minimum(1, t / .012) * np.minimum(1, (dur - t) / .035) ** 1.5
    y *= env
    return y / (np.max(np.abs(y)) + 1e-9) * .7


def save(path, y):
    data = (np.clip(y, -1, 1) * 32767).astype(np.int16)
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(data.tobytes())


if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    for tone, f0 in TONES.items():
        for k in range(4):
            dur = .085 + .02 * (k % 2) + (.015 if tone == "grave" else 0)
            y = voice(f0, VOWELS[k], GLIDES[k], dur)
            save(os.path.join(OUT, "npc_voz_%s_%d.wav" % (tone, k + 1)), y)
    print("vozes:", len(TONES) * 4)
