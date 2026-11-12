"""Corta a narração gravada de uma vez (Data/Narracao/final/narracao_inteira.wav, uma voz só do começo ao fim) nas
20 falas dos quadros: Data/Narracao/final/pN_qM.wav.

python Tools/SplitNarration.py

A narração foi gerada com "..." entre os quadros, então entre uma fala e outra há uma pausa longa. O script acha
as pausas (trechos quietos de 0,4 s ou mais) e escolhe, em ordem, as 19 que ficam mais perto de onde cada quadro
deveria começar (pela quantidade de letras), preferindo as mais longas. Cada pedaço ganha 0,15 s de respiro
antes e um fim suave.
"""
import json
import pathlib
import wave

import numpy as np

ROOT = pathlib.Path(__file__).resolve().parents[1]
FOLDER = ROOT / "Data" / "Narracao" / "final"


def main():
    comic = json.loads((ROOT / "Data" / "UI" / "Abertura" / "abertura.json").read_text(encoding="utf-8"))
    panels = [(pi + 1, qi + 1, q["text"]) for pi, page in enumerate(comic["pages"]) for qi, q in enumerate(page["panels"])]
    with wave.open(str(FOLDER / "narracao_inteira.wav")) as w:
        rate = w.getframerate()
        audio = np.frombuffer(w.readframes(w.getnframes()), np.int16).astype(np.float32)
    total = len(audio) / rate

    # pausas
    step = int(rate * .02)
    energy = np.array([np.abs(audio[i:i + step]).mean() for i in range(0, len(audio) - step, step)])
    quiet = energy < max(80.0, np.percentile(energy, 30) * .6)
    pauses, start = [], None
    for i, q in enumerate(quiet):
        if q and start is None:
            start = i
        if not q and start is not None:
            if (i - start) * .02 >= .4:
                pauses.append(((start + i) * .01, (i - start) * .02))   # (meio da pausa, tamanho)
            start = None

    # onde cada quadro deveria começar, pela proporção de letras
    sizes = [len(t) + 12 for _, _, t in panels]
    expected = np.cumsum(sizes)[:-1] / sum(sizes) * total

    # escolhe 19 pausas em ordem (programação dinâmica): perto do esperado e longa
    n, m = len(expected), len(pauses)
    cost = np.full((n + 1, m + 1), np.inf)
    cost[0, :] = 0
    take = np.zeros((n + 1, m + 1), bool)
    for i in range(1, n + 1):
        for j in range(1, m + 1):
            skip = cost[i, j - 1]
            at, length = pauses[j - 1]
            use = cost[i - 1, j - 1] + abs(at - expected[i - 1]) - 1.5 * length
            if use < skip:
                cost[i, j], take[i, j] = use, True
            else:
                cost[i, j] = skip
    cuts, i, j = [], n, m
    while i > 0:
        if take[i, j]:
            cuts.append(pauses[j - 1][0])
            i -= 1
        j -= 1
    cuts = [0.0] + sorted(cuts) + [total]

    report = []
    for k, (page, panel, text) in enumerate(panels):
        a, b = int(max(0, cuts[k] - .15) * rate), int(cuts[k + 1] * rate)
        piece = audio[a:b].copy()
        # tira o silêncio do fim (deixa 0,25 s) e suaviza a ponta
        loud = np.where(np.abs(piece) > 300)[0]
        if len(loud):
            piece = piece[:min(len(piece), loud[-1] + int(rate * .25))]
        fade = min(len(piece), int(rate * .06))
        piece[-fade:] *= np.linspace(1, 0, fade)
        name = f"p{page}_q{panel}"
        with wave.open(str(FOLDER / f"{name}.wav"), "wb") as out:
            out.setnchannels(1)
            out.setsampwidth(2)
            out.setframerate(rate)
            out.writeframes(piece.astype(np.int16).tobytes())
        report.append({"name": name, "start": round(cuts[k], 2), "duration": round(len(piece) / rate, 2), "text": text})
        print("%s %6.2fs +%5.2fs  %s" % (name, cuts[k], len(piece) / rate, text[:60]))
    (FOLDER / "cortes.json").write_text(json.dumps(report, ensure_ascii=False, indent=1), encoding="utf-8")


if __name__ == "__main__":
    main()
