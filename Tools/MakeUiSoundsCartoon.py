"""Sons de desenho animado das janelas -> Data/UI/SonsCartoon/*.wav. Base sintetizada aqui; por cima, sons gravados
da Kenney (CC0, uso comercial livre, Data/UI/SonsKenney com as licenças) onde combinam — ver o fim do arquivo.

  ui_c_hover   tique baixinho quando o mouse passa num botão
  ui_c_pop     o clique comum (bolha que estoura subindo)
  ui_c_select  escolher um item (blip curto)
  ui_c_tab     trocar de aba (assobio curto + vento)
  ui_c_open    abrir a janela (apito subindo + pop)
  ui_c_close   fechar a janela (apito descendo)
  ui_c_point   gastar um ponto de atributo (boing de mola + brilhinho)
  ui_c_coin    moeda (duas notas, a segunda longa)
  ui_c_buy     comprar (moeda + arpejo de sininho)
  ui_c_equip   vestir/empunhar (baque + brilho)
  ui_c_use     usar poção (três bolhinhas: glub glub)
  ui_c_error   não pode (bonk bonk)
    python Tools/MakeUiSoundsCartoon.py      (depois: Tools/ImportUiKitCartoon.py com o editor fechado)
"""
import os
import wave
import numpy as np

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "Data", "UI", "SonsCartoon")
os.makedirs(OUT, exist_ok=True)
SR = 44100
rng = np.random.default_rng(3)


def t(dur):
    return np.arange(int(SR * dur)) / SR


def sweep(f0, f1, dur, curve=1.0):
    """Fase de um tom que vai de f0 a f1 (curve > 1: muda mais no fim)."""
    x = t(dur) / dur
    f = f0 + (f1 - f0) * x ** curve
    return 2 * np.pi * np.cumsum(f) / SR


def env(n, attack=.004, decay=8.0):
    x = np.arange(n) / SR
    a = np.clip(x / attack, 0, 1)
    return a * np.exp(-decay * x)


def soft_square(phase, k=2.5):
    return np.tanh(k * np.sin(phase))


def bell(freq, dur, decay=6.0):
    x = t(dur)
    s = np.sin(2 * np.pi * freq * x) + .35 * np.sin(2 * np.pi * freq * 2.01 * x) + .12 * np.sin(2 * np.pi * freq * 3.02 * x)
    return s * env(len(x), .002, decay)


def noise(dur, lp=.2):
    n = rng.standard_normal(int(SR * dur))
    out = np.zeros_like(n)
    acc = 0.0
    for i, v in enumerate(n):  # passa-baixa simples
        acc += lp * (v - acc)
        out[i] = acc
    return out / (np.abs(out).max() + 1e-9)


def mixat(base, part, at):
    i = int(at * SR)
    end = i + len(part)
    if end > len(base):
        base = np.concatenate([base, np.zeros(end - len(base))])
    base[i:end] += part
    return base


def save(name, s, peak=.8):
    s = s / (np.abs(s).max() + 1e-9) * peak
    fade = min(len(s) // 4, int(SR * .006))
    s[-fade:] *= np.linspace(1, 0, fade)
    data = (s * 32767).astype(np.int16)
    with wave.open(os.path.join(OUT, name + ".wav"), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(data.tobytes())


# tique do mouse
d = .035
save("ui_c_hover", np.sin(sweep(1700, 2300, d)) * env(int(SR * d), .002, 90), .35)

# clique: bolha que sobe
d = .09
s = np.sin(sweep(320, 1150, d, .7)) * env(int(SR * d), .003, 38)
s[:200] += noise(200 / SR, .6) * .3 * np.linspace(1, 0, 200)
save("ui_c_pop", s, .75)

# escolher item
d = .06
save("ui_c_select", np.sin(sweep(950, 1400, d)) * env(int(SR * d), .002, 45) + .3 * np.sin(sweep(1900, 2800, d)) * env(int(SR * d), .002, 60), .6)

# aba: assobio curto + vento
d = .16
s = np.sin(sweep(520, 980, d, .8)) * env(int(SR * d), .01, 16) * .8
s += noise(d, .08) * np.sin(np.linspace(0, np.pi, int(SR * d))) * .35
save("ui_c_tab", s, .6)

# abrir: apito subindo com vibrato + pop
d = .22
x = t(d)
ph = sweep(380, 1350, d, 1.4) + .9 * np.sin(2 * np.pi * 22 * x)
s = np.sin(ph) * np.sin(np.linspace(0, np.pi * .9, len(x))) * .8
pop = np.sin(sweep(500, 1500, .07, .7)) * env(int(SR * .07), .002, 40)
save("ui_c_open", mixat(s, pop, .19), .7)

# fechar: apito descendo
d = .2
x = t(d)
s = np.sin(sweep(1250, 330, d, .7) + .7 * np.sin(2 * np.pi * 20 * x)) * np.sin(np.linspace(0, np.pi, len(x)))
save("ui_c_close", s, .6)

# ponto: boing de mola + brilhinho subindo
d = .42
x = t(d)
f = 210 * (1 + .45 * np.exp(-7 * x) * np.sin(2 * np.pi * 16 * x)) * (1 + .5 * x)
s = soft_square(2 * np.pi * np.cumsum(f) / SR, 1.6) * env(len(x), .004, 6.5)
for i, fr in enumerate((1319, 1760, 2093)):
    s = mixat(s, bell(fr, .25, 14) * .35, .05 + i * .05)
save("ui_c_point", s, .75)

# moeda: si e mi (clássico)
s = soft_square(2 * np.pi * 988 * t(.07), 1.2) * .6
s = np.concatenate([s, soft_square(2 * np.pi * 1319 * t(.32), 1.2) * env(int(SR * .32), .002, 9) * .6])
save("ui_c_coin", s, .55)

# comprar: moeda + arpejo de sininho
s = soft_square(2 * np.pi * 988 * t(.06), 1.2) * .5
s = np.concatenate([s, soft_square(2 * np.pi * 1319 * t(.12), 1.2) * env(int(SR * .12), .002, 14) * .5])
for i, fr in enumerate((1047, 1319, 1568, 2093)):
    s = mixat(s, bell(fr, .45, 7) * .55, .14 + i * .06)
save("ui_c_buy", s, .7)

# vestir: baque + brilho
d = .14
s = np.sin(sweep(190, 70, d)) * env(int(SR * d), .002, 22)
s += noise(d, .25) * env(int(SR * d), .001, 45) * .5
for i in range(6):
    s = mixat(s, bell(2400 + rng.uniform(-300, 900), .22, 18) * .22, .06 + i * .035)
save("ui_c_equip", s, .75)

# poção: glub glub glub
s = np.zeros(1)
for i, (a, b) in enumerate(((260, 520), (320, 640), (380, 820))):
    blub = np.sin(sweep(a, b, .07, .6)) * env(int(SR * .07), .003, 35)
    s = mixat(s, blub, i * .09)
save("ui_c_use", s, .7)

# não pode: bonk bonk
s = soft_square(2 * np.pi * 180 * t(.09), 2.0) * env(int(SR * .09), .003, 14)
s = np.concatenate([s, np.zeros(int(SR * .03)), soft_square(2 * np.pi * 140 * t(.13), 2.0) * env(int(SR * .13), .003, 12)])
save("ui_c_error", s, .6)

# ---- sons gravados da Kenney (CC0, uso comercial livre; Data/UI/SonsKenney com as licenças) ----------------
# "procure sons online que combinem e dê para usar comercialmente": onde há um som da Kenney que serve, ele
# substitui o sintetizado; a mola do ponto e as bolhas da poção continuam os daqui (mais desenho animado),
# e a compra e o vestir misturam os dois.
KENNEY = os.path.join(ROOT, "Data", "UI", "SonsKenney")
try:
    import soundfile
except ImportError:
    soundfile = None


def kenney(name):
    data, rate = soundfile.read(os.path.join(KENNEY, name + ".ogg"), dtype="float64")
    if data.ndim > 1:
        data = data.mean(axis=1)
    if rate != SR:  # reamostra simples (linear)
        x = np.linspace(0, len(data) - 1, int(len(data) * SR / rate))
        data = np.interp(x, np.arange(len(data)), data)
    return data


if soundfile and os.path.isdir(KENNEY):
    save("ui_c_hover", kenney("ui_rollover2"), .3)
    save("ui_c_pop", kenney("interface_pluck_001"), .7)
    save("ui_c_select", kenney("interface_select_002"), .6)
    save("ui_c_tab", kenney("interface_switch_003"), .6)
    save("ui_c_open", kenney("interface_maximize_006"), .65)
    save("ui_c_close", kenney("interface_minimize_006"), .6)
    save("ui_c_error", kenney("interface_error_004"), .6)
    save("ui_c_coin", kenney("rpg_handleCoins"), .7)
    # compra: moedas + o "confirmado" com o sininho daqui por cima
    s = mixat(kenney("rpg_handleCoins2") * .9, kenney("interface_confirmation_001") * .7, .08)
    for i, fr in enumerate((1568, 2093)):
        s = mixat(s, bell(fr, .35, 8) * .25, .2 + i * .07)
    save("ui_c_buy", s, .7)
    # vestir: couro/cinto + um brilho de vidro
    s = mixat(kenney("rpg_handleSmallLeather"), kenney("interface_glass_002") * .5, .05)
    save("ui_c_equip", s, .75)
    # ponto: a mola daqui + o "confirmado" da Kenney
    d = .42
    x = t(d)
    f = 210 * (1 + .45 * np.exp(-7 * x) * np.sin(2 * np.pi * 16 * x)) * (1 + .5 * x)
    s = soft_square(2 * np.pi * np.cumsum(f) / SR, 1.6) * env(len(x), .004, 6.5) * .7
    save("ui_c_point", mixat(s, kenney("interface_confirmation_002") * .8, .04), .75)
else:
    print("sem soundfile (pip install soundfile) ou sem Data/UI/SonsKenney: ficaram só os sintetizados")

print("VORTEX_SONS_CARTOON", len([f for f in os.listdir(OUT) if f.endswith(".wav")]), "sons em", OUT)
