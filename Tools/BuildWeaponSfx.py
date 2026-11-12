"""Os sons de cada arma revisada (REVISAO_ARMAS): golpe, acerto, tiro e secundários.

python Tools/BuildWeaponSfx.py

Fontes (só leitura):
- Data/WeaponSfx/ref/*.wav: os pacotes do VortexReferencia (Tools/ExportReferenceSounds.py): magia modular,
  Free_Sounds_Pack (golpes, livro, explosões, madeira, metal) e SmallSoundKit (magia de fogo e água);
- the-world-geneses/Assets/Sfx/*.wav: os sons do Godot (arco, lança, martelo, magias por elemento);
- o que não existe pronto é sintetizado aqui: cordas do alaúde (Karplus-Strong), vidro estilhaçando,
  elos de corrente, estalo do chicote e lufadas de ar (golpes) com o peso de cada arma.
Saída: Data/WeaponSfx/out/wfx_<família>_<tipo>[_N].wav e wfx_<efeito>.wav (44,1 kHz, mono, 16 bits),
importados por Tools/ImportWeaponSfx.py para /Game/Vortex/Audio/Sfx (o jogo procura por esses nomes).
"""
import glob
import os
import wave

import numpy as np
from scipy import signal

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REF = os.path.join(ROOT, "Data", "WeaponSfx", "ref")
GODOT = os.path.normpath(os.path.join(ROOT, "..", "..", "the-world-geneses", "Assets", "Sfx"))
OUT = os.path.join(ROOT, "Data", "WeaponSfx", "out")
SR = 44100
rng = np.random.default_rng(7)


# ---- ler, escrever e mexer ------------------------------------------------------------------------
def read(path):
    with wave.open(path) as w:
        n, ch, sw, sr = w.getnframes(), w.getnchannels(), w.getsampwidth(), w.getframerate()
        raw = w.readframes(n)
    dt = {1: np.uint8, 2: np.int16, 4: np.int32}[sw]
    x = np.frombuffer(raw, dt).astype(np.float64)
    if sw == 1:
        x = (x - 128) / 128
    else:
        x /= float(2 ** (8 * sw - 1))
    if ch > 1:
        x = x.reshape(-1, ch).mean(axis=1)
    if sr != SR:
        g = np.gcd(SR, sr)
        x = signal.resample_poly(x, SR // g, sr // g)
    return x


def ref(name, variant=1):
    """Um som de referência pelo nome (ex.: 'SW_MagicCast_FireBall1', 'Whoosh_1-1')."""
    hits = glob.glob(os.path.join(REF, "*__%s_%d.wav" % (name, variant))) or glob.glob(os.path.join(REF, "*__%s.wav" % name))
    if not hits:
        raise FileNotFoundError(name)
    return read(hits[0])


def godot(name):
    return read(os.path.join(GODOT, name + ".wav"))


def trim(x, start=0.0, length=None, fade_in=.003, fade_out=.04):
    a = int(start * SR)
    b = len(x) if length is None else min(len(x), a + int(length * SR))
    y = x[a:b].copy()
    fi, fo = int(fade_in * SR), int(fade_out * SR)
    if fi and len(y) > fi:
        y[:fi] *= np.linspace(0, 1, fi)
    if fo and len(y) > fo:
        y[-fo:] *= np.linspace(1, 0, fo)
    return y


def pitch(x, factor):
    """Mais agudo (>1) e mais curto, ou mais grave (<1) e mais longo (como tocar a fita mais rápido)."""
    if abs(factor - 1) < 1e-3:
        return x
    n = max(8, int(len(x) / factor))
    return signal.resample(x, n)


def gain(x, db):
    return x * 10 ** (db / 20)


def norm(x, peak=-1.5):
    m = np.max(np.abs(x)) or 1
    return x / m * 10 ** (peak / 20)


def mix(*parts):
    """(sinal, atraso em s) — soma alinhada."""
    n = max(int(d * SR) + len(s) for s, d in parts)
    y = np.zeros(n)
    for s, d in parts:
        a = int(d * SR)
        y[a:a + len(s)] += s
    return y


def band(x, lo, hi, order=3):
    lo = max(20, lo)
    hi = min(SR / 2 - 100, hi)
    sos = signal.butter(order, [lo, hi], btype="band", fs=SR, output="sos")
    return signal.sosfilt(sos, x)


def lowpass(x, f):
    return signal.sosfilt(signal.butter(4, f, btype="low", fs=SR, output="sos"), x)


def highpass(x, f):
    return signal.sosfilt(signal.butter(4, f, btype="high", fs=SR, output="sos"), x)


def env(n, attack, release, shape=2.0):
    t = np.arange(n) / SR
    a = np.clip(t / max(attack, 1e-4), 0, 1)
    r = np.clip((t[-1] - t) / max(release, 1e-4), 0, 1) if n else t
    return (a ** .5) * (r ** shape)


# ---- síntese ----------------------------------------------------------------------------------------
def whoosh(dur=.32, lo=300, hi=2400, weight=1.0, sweep=True):
    """Lufada de ar de um golpe: ruído em banda que sobe e desce (peso = mais grave e mais longo)."""
    n = int(dur * SR)
    noise = rng.standard_normal(n)
    y = np.zeros(n)
    steps = 12
    for i in range(steps):
        a, b = i * n // steps, (i + 1) * n // steps
        t = (i + .5) / steps
        c = np.sin(np.pi * t) if sweep else 1
        f0 = lo / weight + (hi / weight - lo / weight) * c
        seg = band(noise, max(60, f0 * .6), f0 * 1.6)[a:b]
        y[a:b] = seg
    e = np.sin(np.pi * np.linspace(0, 1, n)) ** 1.6
    return norm(y * e, -4)


def pluck(freq, dur=1.1, bright=.6, decay=.996):
    """Corda dedilhada (Karplus-Strong), um pouco de corpo de madeira."""
    n = int(dur * SR)
    period = int(SR / freq)
    buf = rng.uniform(-1, 1, period) * bright + rng.uniform(-1, 1, period) * (1 - bright) * .3
    y = np.zeros(n)
    for i in range(n):
        y[i] = buf[i % period]
        buf[i % period] = decay * .5 * (buf[i % period] + buf[(i + 1) % period])
    body = band(y, 180, 3200, 2)
    return norm(y * .6 + body * .6, -3)


def chord(freqs, gap=.025, dur=1.4):
    return norm(mix(*[(pluck(f, dur), i * gap) for i, f in enumerate(freqs)]), -2)


def melody(notes, step=.22, dur=1.2):
    return norm(mix(*[(pluck(f, dur, bright=.7) * (1 if i % 4 == 0 else .7), i * step) for i, f in enumerate(notes) if f]), -3)


def glass(dur=.7):
    """Vidro estilhaçando: estalo, cacos tilintando (parciais agudas que somem) e um chiado."""
    n = int(dur * SR)
    t = np.arange(n) / SR
    y = np.zeros(n)
    for _ in range(40):
        f = rng.uniform(2500, 9000)
        d = rng.uniform(0, .25)
        a = int(d * SR)
        k = np.exp(-(t[:n - a]) * rng.uniform(18, 45))
        y[a:] += np.sin(2 * np.pi * f * t[:n - a]) * k * rng.uniform(.2, 1)
    crack = highpass(rng.standard_normal(n), 2000) * np.exp(-t * 30)
    return norm(y * .5 + crack * .8, -2)


def chain(dur=.45, links=9):
    """Elos de corrente batendo uns nos outros (cliques metálicos inarmônicos)."""
    n = int(dur * SR)
    y = np.zeros(n)
    for i in range(links):
        d = rng.uniform(0, dur * .7)
        a = int(d * SR)
        m = int(.08 * SR)
        tt = np.arange(m) / SR
        clink = sum(np.sin(2 * np.pi * f * tt) for f in rng.uniform(1800, 6500, 3)) * np.exp(-tt * 55)
        clink += highpass(rng.standard_normal(m), 3000) * np.exp(-tt * 120) * .5
        y[a:a + m] += clink[:max(0, min(m, n - a))] * rng.uniform(.4, 1)
    return norm(y, -3)


def crack(dur=.35):
    """Estalo do chicote: um silvo que acelera e o estouro seco da ponta."""
    w = whoosh(.22, 600, 4000, 1.0) * .6
    n = int(.12 * SR)
    t = np.arange(n) / SR
    pop = highpass(rng.standard_normal(n), 1500) * np.exp(-t * 90)
    pop[:40] += np.linspace(1, 0, 40) * 1.2
    y = mix((w, 0), (pop, .2))
    # um pouco de saturação: o estalo fica alto sem um pico só
    return norm(np.tanh(norm(y, 0) * 3.0), -1)


# duração máxima de cada tipo de som (os de referência às vezes têm cauda longa)
LIMIT = {"_swing": .7, "_hit": .7, "_shot": 1.2, "wand_blast": 1.6, "throw_heavy": 1.3, "book_cast": 1.2, "book_page": 1.0}


def whistle(dur=.55, f0=2600, f1=900):
    """Flecha caindo: um assobio que desce (ruído estreito acompanhando a altura) e o baque ao cravar."""
    n = int(dur * SR)
    t = np.arange(n) / SR
    f = f0 + (f1 - f0) * (t / dur) ** 1.5
    phase = 2 * np.pi * np.cumsum(f) / SR
    tone = np.sin(phase) * .35 + band(rng.standard_normal(n), 900, 4200) * .25 * np.sin(phase * .5) ** 2
    e = np.clip(t / .08, 0, 1) * np.clip((dur - t) / .05, 0, 1) * (.4 + .6 * t / dur)
    return tone * e


def save(name, x, peak=-1.5):
    os.makedirs(OUT, exist_ok=True)
    for key, most in LIMIT.items():
        if key in name and len(x) > most * SR:
            x = trim(x, 0, most, 0, .12)
            break
    x = norm(x, peak)
    x = np.concatenate([x, np.zeros(int(.02 * SR))])
    data = (np.clip(x, -1, 1) * 32767).astype(np.int16)
    with wave.open(os.path.join(OUT, name + ".wav"), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(data.tobytes())
    made.append(name)


made = []


# ---- golpes de perto: lufada (com o peso da arma) + um toque de referência --------------------------
def swings(family, weight, lo, hi, dur, extra=None, extra_db=-8):
    for i in range(3):
        w = whoosh(dur * (1 + .08 * i), lo, hi, weight * (1 + .05 * (i - 1)))
        parts = [(w, 0)]
        if extra is not None:
            parts.append((gain(pitch(extra, 1 + .06 * (i - 1)), extra_db), .02))
        save("wfx_%s_swing_%d" % (family, i + 1), mix(*parts), -3)


def main():
    sword = [godot("sword_swing_%d" % i) for i in (1, 2, 3)]
    swings("mace", 1.35, 250, 1600, .36, godot("hammer_swing"), -6)
    swings("axe", 1.4, 240, 1500, .4, godot("hammer_swing"), -9)
    swings("dualaxes", 1.15, 300, 1900, .3, sword[1], -8)
    swings("greatsword", 1.7, 180, 1300, .55, pitch(sword[2], .8), -6)
    swings("halberd", 1.5, 200, 1400, .48, pitch(sword[0], .85), -8)
    swings("spear", .9, 500, 2600, .22, godot("spear_thrust"), -2)
    swings("dagger", .7, 700, 3600, .18, pitch(sword[1], 1.25), -6)
    swings("fist", .9, 400, 2200, .16)
    swings("swordshield", 1.0, 350, 2400, .3, sword[0], -4)
    swings("scythe", 1.3, 260, 2000, .45, ref("SW_MagicWeapon_Swing", 1), -10)
    for i in range(3):
        save("wfx_chain_swing_%d" % (i + 1), mix((whoosh(.3, 400, 2400, 1.1), 0), (gain(chain(.4), -4), .02)), -3)
        save("wfx_whip_swing_%d" % (i + 1), crack(), -2)

    # ---- acertos
    save("wfx_mace_hit", mix((godot("hammer_hit"), 0), (gain(ref("Hit_Generic_5-1"), -6), 0)))
    save("wfx_hammer_hit", godot("hammer_hit"))
    save("wfx_axe_hit", mix((ref("Wood_Chop_1-4"), 0), (gain(godot("sword_hit"), -8), 0)))
    save("wfx_dualaxes_hit", pitch(ref("Wood_Chop_1-4"), 1.15))
    save("wfx_greatsword_hit", mix((pitch(godot("sword_hit"), .8), 0), (gain(ref("Shield_Metal_Impact_1-3"), -10), 0)))
    save("wfx_halberd_hit", mix((pitch(godot("sword_hit"), .9), 0), (gain(ref("Stab_4-1"), -4), 0)))
    save("wfx_spear_hit", ref("Stab_4-1"))
    save("wfx_dagger_hit", pitch(ref("Stab_4-1"), 1.2))
    save("wfx_fist_hit", ref("Punch_2-1"))
    save("wfx_swordshield_hit", godot("sword_hit"))
    save("wfx_chain_hit", mix((gain(chain(.3, 6), -2), 0), (gain(ref("Stab_4-1"), -6), 0)))
    save("wfx_whip_hit", mix((crack(), 0), (gain(ref("Punch_2-1"), -10), .2)))
    save("wfx_scythe_hit", mix((godot("sword_hit"), 0), (gain(ref("Stab_4-1"), -6), 0)))
    save("wfx_chakram_hit", mix((pitch(godot("sword_hit"), 1.2), 0), (gain(chain(.2, 3), -8), 0)))

    # ---- tiros, magias e arremessos (o golpe normal)
    notes = [523.25, 659.25, 783.99, 587.33, 698.46, 880.0]   # dó, mi, sol, ré, fá, lá
    for i in range(3):
        save("wfx_lute_shot_%d" % (i + 1), mix((pluck(notes[i], .9, .7), 0), (gain(pluck(notes[i + 3], .9, .7), -4), .06)), -3)
    save("wfx_orb_shot", mix((ref("SW_MagicCast_Projectile1"), 0), (gain(godot("holy_cast"), -8), 0)))
    save("wfx_book_shot", mix((ref("Book_Page_1-2"), 0), (gain(ref("Mgc_Fire_Cast_01"), -2), .1)))
    save("wfx_wand_shot", mix((ref("SW_MagicCast_Generic1"), 0), (gain(godot("arcane_cast"), -10), 0)))
    save("wfx_staff_shot", mix((godot("nature_cast"), 0), (gain(ref("SW_MagicCast_Arrow"), -4), 0)))
    save("wfx_flask_shot", whoosh(.25, 600, 2200, .9))
    save("wfx_chakram_shot", mix((whoosh(.3, 900, 3800, .8), 0), (gain(chain(.2, 3), -12), .05)))
    save("wfx_pistol_shot", mix((trim(ref("Sci-Fi_Gun_1-1"), 0, .5), 0), (gain(ref("SW_MagicFade_Shoot1"), -6), 0)))
    save("wfx_crossbow_shot", mix((pitch(godot("bow_release"), .85), 0), (gain(trim(ref("Nail_Wood_1-1"), 0, .25), -8), 0)))
    save("wfx_bow_shot", godot("bow_release"))
    save("wfx_bow_draw", godot("bow_draw"))
    save("wfx_bow_release", godot("bow_release"))

    # ---- secundários e efeitos
    song = [523.25, 659.25, 783.99, 659.25, 587.33, 698.46, 880.0, 698.46, 659.25, 783.99, 1046.5, 783.99, 698.46, 587.33, 523.25, None]
    save("wfx_lute_song", mix((melody(song, .2), 0), (gain(chord([261.63, 329.63, 392.0], .03, 2.5), -8), 0), (gain(chord([293.66, 349.23, 440.0], .03, 2.5), -8), 1.6)), -3)
    save("wfx_lute_end", chord([261.63, 329.63, 392.0, 523.25], .04, 1.8))
    save("wfx_orb_summon", mix((ref("SW_MagicCast_Buff1"), 0), (gain(godot("holy_cast"), -6), .1)))
    save("wfx_firewall", mix((ref("SW_MagicSpell_FireTrail"), 0), (gain(ref("Fire_Whoosh_2-15"), -4), 0)))
    save("wfx_tornado", mix((trim(ref("SW_MagicCast_FireBall2"), 0, 1.4), 0), (gain(trim(ref("Fire_Burning_Loop_2"), 0, 2.4, .2, .6), -6), 0)))
    save("wfx_book_cast", mix((ref("Book_Page_1-2"), 0), (gain(ref("Book_Handle_1-5"), -6), 0)))
    save("wfx_wand_charge", trim(ref("SW_MagicChant_Charge1"), 0, .9, .01, .2))
    save("wfx_wand_blast", mix((ref("SW_MagicFade_Shoot2"), 0), (gain(ref("SW_MagicImpact_Barrier"), -6), .05)))
    save("wfx_staff_cast", mix((godot("staff_cast"), 0), (gain(trim(ref("SW_MagicChant_Riser1"), 0, 1.2), -6), 0)))
    save("wfx_vines", mix((ref("SW_MagicCast_StaffStrikeGround"), 0), (gain(godot("nature_cast"), -4), .05)))
    save("wfx_throw", whoosh(.28, 500, 2000, .9))
    save("wfx_throw_heavy", mix((ref("SW_MagicCast_ThrowHeavy"), 0), (gain(whoosh(.45, 250, 1500, 1.3), -4), 0)))
    save("wfx_glass_break", glass())
    save("wfx_acid", mix((godot("poison_land"), 0), (gain(ref("SW_MagicCast_ThrowToxin"), -8), 0)))
    save("wfx_potion", mix((ref("SW_MagicCast_Healing"), 0), (gain(godot("poison_smoke"), -10), 0)))
    save("wfx_holy_slam", mix((ref("SW_MagicCast_GroundStrike"), 0), (gain(godot("holy_cast"), -4), .05)))
    save("wfx_dagger_dash", mix((whoosh(.3, 900, 4200, .7), 0), (gain(godot("skill_escape"), -6), 0)))
    save("wfx_heavy_impact", mix((godot("rock_impact"), 0), (gain(trim(ref("SW_MagicSpell_EarthQuake"), 0, 1.2), -6), 0)))
    save("wfx_spear_thrust", godot("spear_thrust"))
    save("wfx_chain_lash", mix((whoosh(.25, 500, 2600, 1.0), 0), (chain(.35), .05)))
    save("wfx_hammer_slam", mix((godot("golem_slam"), 0), (gain(godot("hammer_hit"), -4), 0)))
    save("wfx_axe_whirl", whoosh(.42, 220, 1500, 1.3))
    save("wfx_scythe_reap", mix((whoosh(.5, 220, 1700, 1.2), 0), (gain(ref("SW_MagicWeapon_Swing", 2), -6), 0)))
    save("wfx_greatsword_sweep", mix((whoosh(.6, 160, 1200, 1.7), 0), (gain(pitch(godot("sword_swing_3"), .75), -4), 0)))
    save("wfx_whip_crack", crack())
    save("wfx_grapple_pull", mix((whoosh(.5, 300, 1800, 1.0), 0), (gain(ref("Wood_Move_2-1"), -8), 0)))
    save("wfx_chakram_throw", mix((whoosh(.3, 900, 3800, .8), 0), (gain(chain(.2, 3), -12), .05)))
    save("wfx_chakram_giant", mix((ref("SW_MagicWeapon_Summon"), 0), (gain(whoosh(.7, 200, 1500, 1.5), -2), .1)))
    save("wfx_rune_shot", ref("SW_MagicFade_Shoot1"))
    save("wfx_rune_burst", mix((ref("SW_MagicImpact_Barrier"), 0), (gain(ref("Explosion_Medium_2-1"), -6), 0)))
    # chuva de flechas: a corda solta, vários assobios descendo e as flechas cravando no chão
    hits = [godot("arrow_hit")]
    rain = [(gain(godot("bow_release"), -2), 0)]
    for i in range(9):
        w = whistle(rng.uniform(.45, .7), rng.uniform(2200, 3200), rng.uniform(700, 1100))
        start = .25 + i * .2 + rng.uniform(0, .08)
        rain.append((gain(w, -6), start))
        rain.append((gain(pitch(hits[0], rng.uniform(.85, 1.15)), -4), start + len(w) / SR - .02))
    save("wfx_arrow_rain", mix(*rain))
    save("wfx_arrow_whistle", mix((whistle(.6), 0), (gain(godot("arrow_hit"), -3), .58)))
    save("wfx_shield_block", mix((ref("Shield_Metal_Impact_1-3"), 0), (gain(godot("guard_armor"), -6), 0)))
    save("wfx_book_page", ref("Book_Page_1-2"))
    print("WEAPON_SFX", len(made))


if __name__ == "__main__":
    main()
