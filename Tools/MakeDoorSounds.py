"""Os sons das portas da vila (07/11, o usuário: "faça animação e som da porta se abrindo"), sintetizados aqui (sem
licença a cuidar) -> Data/Audio/door_open.wav e door_close.wav (o VortexAudio toca pelo nome).

  door_open   o trinco (clique de ferro) e o rangido da dobradiça (atrito que gruda e solta: pulsos que excitam as
              ressonâncias da madeira, a frequência subindo e descendo enquanto a folha gira)
  door_close  um rangido curto e a batida da madeira no batente (o grave com a madeira vibrando) e o trinco
    python Tools/MakeDoorSounds.py      (depois: Tools/ImportExtraSfx.py com o editor fechado)
"""
import os
import wave

import numpy as np

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "Data", "Audio")
SR = 44100
rng = np.random.default_rng(7)


def wood_ir(dur=.035, modes=((520, 18), (1180, 30), (2350, 55), (3900, 90))):
    """A resposta da madeira a um toque: algumas ressonâncias que somem rápido."""
    t = np.arange(int(SR * dur)) / SR
    ir = sum(np.sin(2 * np.pi * f * t) * np.exp(-d * t * 10) * (1.0 / (k + 1)) for k, (f, d) in enumerate(modes))
    return ir / np.max(np.abs(ir))


def creak(dur, f0, f1, wobble=.35, level=1.0):
    """O rangido: pulsos de atrito (gruda e solta) numa frequência que anda de f0 a f1 com um tremor."""
    n = int(SR * dur)
    t = np.arange(n) / SR
    freq = f0 + (f1 - f0) * (.5 - .5 * np.cos(np.pi * t / dur)) + wobble * f0 * np.sin(2 * np.pi * 3.1 * t) * np.sin(np.pi * t / dur)
    phase = np.cumsum(freq / SR)
    pulses = np.zeros(n)
    idx = np.nonzero(np.diff(np.floor(phase)) > 0)[0]
    pulses[idx] = rng.uniform(.6, 1.0, len(idx))
    env = np.sin(np.pi * t / dur) ** .6
    out = np.convolve(pulses * env, wood_ir(), mode="full")[:n]
    # o chiado do atrito por cima
    out += .08 * rng.standard_normal(n) * env * np.sin(2 * np.pi * freq * 6 * t) ** 2
    return level * out / np.max(np.abs(out))


def click(dur=.05, level=.8):
    """O trinco: um clique metálico curto."""
    t = np.arange(int(SR * dur)) / SR
    s = (np.sin(2 * np.pi * 3200 * t) + .6 * np.sin(2 * np.pi * 5100 * t)) * np.exp(-t * 120)
    s += .4 * rng.standard_normal(len(t)) * np.exp(-t * 300)
    return level * s / np.max(np.abs(s))


def thud(dur=.45, level=1.0):
    """A folha batendo no batente: o grave da batida, a madeira vibrando e um pouco de poeira (ruído)."""
    t = np.arange(int(SR * dur)) / SR
    s = np.sin(2 * np.pi * 85 * t) * np.exp(-t * 14) + .5 * np.sin(2 * np.pi * 160 * t) * np.exp(-t * 22)
    s += .35 * np.convolve(rng.standard_normal(len(t)) * np.exp(-t * 60), wood_ir(.05), mode="full")[:len(t)] / 20
    return level * s / np.max(np.abs(s))


def place(buf, sound, at):
    i = int(SR * at)
    end = min(len(buf), i + len(sound))
    buf[i:end] += sound[:end - i]


def save(name, buf):
    buf = buf / np.max(np.abs(buf)) * .89          # -1 dBFS
    fade = int(SR * .02)
    buf[-fade:] *= np.linspace(1, 0, fade)
    data = (buf * 32767).astype(np.int16)
    os.makedirs(OUT, exist_ok=True)
    with wave.open(os.path.join(OUT, name + ".wav"), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(data.tobytes())
    print("porta:", name, "%.2f s" % (len(buf) / SR))


if __name__ == "__main__":
    open_ = np.zeros(int(SR * 1.15))
    place(open_, click(), 0.0)
    place(open_, creak(.95, 55, 120, level=.75), .1)
    save("door_open", open_)

    close = np.zeros(int(SR * .9))
    place(close, creak(.4, 110, 60, level=.55), 0.0)
    place(close, thud(), .36)
    place(close, click(level=.5), .4)
    save("door_close", close)
    # a conferência: os dois arquivos existem e não estouram
    for n in ("door_open", "door_close"):
        with wave.open(os.path.join(OUT, n + ".wav")) as w:
            frames = np.frombuffer(w.readframes(w.getnframes()), dtype=np.int16)
            assert frames.size > SR * .5 and np.max(np.abs(frames)) < 32767, n
