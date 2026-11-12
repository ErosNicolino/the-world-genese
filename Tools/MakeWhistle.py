"""O assobio de chamar o cavalo (02/11), gerado (sem som de terceiros): duas notas, um "fi" curto subindo e um "fiiiu"
mais longo que sobe e desce, com vibrato, um pouco do sopro de ar e um harmônico fraco. Três variações (o jogo sorteia
uma: Sfx/Recorded/<nome>_v1..).

python Tools/MakeWhistle.py   ->   Data/Audio/horse_whistle_v1.wav .. _v3.wav  (Tools/ImportExtraSfx.py leva para a Unreal)
"""
import os
import wave

import numpy as np

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "Data", "Audio")
RATE = 44100


def glide(points, length):
    """A frequência ao longo da nota: pontos (tempo de 0 a 1, Hz), com curvas suaves entre eles."""
    t = np.linspace(0, 1, length)
    xs, ys = zip(*points)
    f = np.interp(t, xs, ys)
    k = max(1, length // 60)
    return np.convolve(f, np.ones(k) / k, mode="same")


def note(points, seconds, rng, vibrato=.012):
    n = int(seconds * RATE)
    f = glide(points, n)
    t = np.arange(n) / RATE
    f = f * (1 + vibrato * np.sin(2 * np.pi * rng.uniform(5.5, 7) * t) * np.clip(t / .12, 0, 1))
    phase = 2 * np.pi * np.cumsum(f) / RATE
    tone = np.sin(phase) + .08 * np.sin(2 * phase)
    # o sopro: ruído que segue a nota (o ar passando pelos lábios)
    noise = rng.normal(0, 1, n)
    breath = np.convolve(noise, np.ones(6) / 6, mode="same") * .06
    env = np.clip(t / .03, 0, 1) * np.clip((seconds - t) / .08, 0, 1)
    env = env * (.85 + .15 * np.sin(np.pi * np.clip(t / seconds, 0, 1)))
    return (tone + breath) * env


def make(seed):
    rng = np.random.default_rng(seed)
    base = rng.uniform(.94, 1.06)
    first = note([(0, 1500 * base), (1, 2350 * base)], rng.uniform(.16, .2), rng)
    gap = np.zeros(int(rng.uniform(.05, .08) * RATE))
    second = note([(0, 2250 * base), (.35, 2900 * base), (.7, 2650 * base), (1, 1850 * base)], rng.uniform(.5, .62), rng)
    sound = np.concatenate([first, gap, second, np.zeros(int(.12 * RATE))])
    # um eco curto (ao ar livre)
    echo = np.zeros_like(sound)
    d = int(.09 * RATE)
    echo[d:] = sound[:-d] * .18
    sound = sound + echo
    sound = sound / np.max(np.abs(sound)) * .85
    return (sound * 32767).astype(np.int16)


os.makedirs(OUT, exist_ok=True)
for i in range(3):
    path = os.path.join(OUT, "horse_whistle_v%d.wav" % (i + 1))
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(make(41 + i).tobytes())
    print("assobio:", path)
