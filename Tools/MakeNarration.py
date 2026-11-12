"""A voz do narrador da HQ de abertura, gravada DE UMA VEZ só no VoiceStudio (a mesma voz do começo ao fim) e
cortada nas 20 falas dos quadros: Data/Narracao/final/pN_qM.wav. Precisa do VoiceStudio aberto (127.0.0.1:3900).

python Tools/MakeNarration.py          (depois: Tools/ImportIntroComic.py com o editor fechado)

1. junta as 20 falas (Data/UI/Abertura/abertura.json) num texto só, uma por parágrafo (com "..." entre elas a voz
   repetia trechos), com os nomes escritos
   do jeito que a voz fala certo (Gênesse, Ázur, Rêia, Ygdrassil; a legenda continua com a grafia de verdade);
2. grava com a voz escolhida (F: homem maduro, tom bem grave, devagar);
3. corta a gravação em todas as pausas, transcreve cada pedaço e junta os pedaços em ordem nos 20 quadros,
   comparando as palavras ouvidas com as de cada fala;
4. se uma fala não bater (nota < 0,85) ou tiver palavras sobrando (a voz às vezes repete um trecho), grava tudo
   de novo (até 6 vezes) e fica com a melhor.
Relatório em Data/Narracao/final/cortes.json.
"""
import ast
import base64
import io
import json
import pathlib
import re
import unicodedata
import urllib.request
import wave

import numpy as np

ROOT = pathlib.Path(__file__).resolve().parents[1]
FOLDER = ROOT / "Data" / "Narracao" / "final"
URL = "http://127.0.0.1:3900/mcp/"
VOICE = {"language": "pt", "instruct": "male, middle-aged, very low pitch", "speed": 0.78, "steps": 32}
SPOKEN = {"Gênese": "Gênesse", "gênese": "gênesse", "Azhûr": "Ázur", "Rhea": "Rêia", "Yggdrasil": "Ygdrassil"}


def mcp(tool, args):
    head = {"Content-Type": "application/json", "Accept": "application/json, text/event-stream", "X-OmniVoice-Client-Id": "claude-code"}

    def post(body, sid=None):
        h = dict(head)
        if sid:
            h["Mcp-Session-Id"] = sid
        with urllib.request.urlopen(urllib.request.Request(URL, json.dumps(body).encode(), h), timeout=900) as r:
            text, sid = r.read().decode("utf-8"), r.headers.get("mcp-session-id") or sid
        data = next((line[5:] for line in text.splitlines() if line.startswith("data:")), text)
        return (json.loads(data) if data.strip() else None), sid

    _, sid = post({"jsonrpc": "2.0", "id": 1, "method": "initialize", "params": {"protocolVersion": "2025-03-26", "capabilities": {}, "clientInfo": {"name": "vortex", "version": "1"}}})
    post({"jsonrpc": "2.0", "method": "notifications/initialized"}, sid)
    res, _ = post({"jsonrpc": "2.0", "id": 2, "method": "tools/call", "params": {"name": tool, "arguments": args}}, sid)
    text = res["result"]["content"][0]["text"]
    try:
        return json.loads(text)
    except json.JSONDecodeError:
        return ast.literal_eval(text)


def words(text):
    text = unicodedata.normalize("NFKD", text.lower())
    text = "".join(c for c in text if not unicodedata.combining(c))
    return re.findall(r"[a-z]+", text)


def match(heard, expected):
    """Quanto das palavras esperadas foi ouvido, e quanto do ouvido era esperado (F1)."""
    if not heard or not expected:
        return 0.0
    pool = list(expected)
    hits = 0
    for w in heard:
        if w in pool:
            pool.remove(w)
            hits += 1
    p, r = hits / len(heard), hits / len(expected)
    return 0.0 if hits == 0 else 2 * p * r / (p + r)


def wav_bytes(samples, rate):
    buf = io.BytesIO()
    with wave.open(buf, "wb") as out:
        out.setnchannels(1)
        out.setsampwidth(2)
        out.setframerate(rate)
        out.writeframes(samples.astype(np.int16).tobytes())
    return buf.getvalue()


def take(lines):
    text = ""
    for i, line in enumerate(lines):
        spoken = line.replace("…", "...")
        for a, b in SPOKEN.items():
            spoken = spoken.replace(a, b)
        text += spoken + ("" if i == len(lines) - 1 else ("\n\n\n" if (i + 1) % 4 == 0 else "\n\n"))
    out = mcp("generate_speech", dict(VOICE, text=text))
    raw = base64.b64decode(out["wav_base64"])
    with wave.open(io.BytesIO(raw)) as w:
        rate = w.getframerate()
        audio = np.frombuffer(w.readframes(w.getnframes()), np.int16).astype(np.float32)
    return raw, audio, rate


def chunks(audio, rate):
    """Pedaços entre as pausas de 0,3 s ou mais (os cortes ficam no meio das pausas)."""
    step = int(rate * .02)
    energy = np.array([np.abs(audio[i:i + step]).mean() for i in range(0, len(audio) - step, step)])
    quiet = energy < max(80.0, np.percentile(energy, 30) * .6)
    cuts, start = [0.0], None
    for i, q in enumerate(quiet):
        if q and start is None:
            start = i
        if not q and start is not None:
            if (i - start) * .02 >= .3 and start > 0:
                cuts.append((start + i) * .01)
            start = None
    cuts.append(len(audio) / rate)
    return [(a, b) for a, b in zip(cuts, cuts[1:]) if b - a > .25]


def align(pieces, heard, expected):
    """Junta os pedaços em ordem nas falas (programação dinâmica: a soma das notas de cada fala)."""
    n, m = len(expected), len(pieces)
    best = np.full((n + 1, m + 1), -1e9)
    best[0, 0] = 0
    back = {}
    for k in range(1, n + 1):
        for j in range(1, m + 1):
            for i in range(max(0, j - 6), j):   # cada fala junta até 6 pedaços
                if best[k - 1, i] < -1e8:
                    continue
                said = [w for piece in heard[i:j] for w in piece]
                score = best[k - 1, i] + match(said, expected[k - 1])
                if score > best[k, j]:
                    best[k, j], back[(k, j)] = score, i
    spans, j = [], m
    for k in range(n, 0, -1):
        i = back[(k, j)]
        spans.append((i, j))
        j = i
    return spans[::-1]


def main():
    comic = json.loads((ROOT / "Data" / "UI" / "Abertura" / "abertura.json").read_text(encoding="utf-8"))
    panels = [(pi + 1, qi + 1, q["text"]) for pi, page in enumerate(comic["pages"]) for qi, q in enumerate(page["panels"])]
    lines = [t for _, _, t in panels]
    expected = []
    for t in lines:
        for a, b in SPOKEN.items():
            t = t.replace(a, b)
        expected.append(words(t))
    FOLDER.mkdir(parents=True, exist_ok=True)
    best = None
    for attempt in range(1, 7):
        raw, audio, rate = take(lines)
        pieces = chunks(audio, rate)
        heard = []
        for a, b in pieces:
            got = mcp("transcribe", {"audio_base64": base64.b64encode(wav_bytes(audio[int(a * rate):int(b * rate)], rate)).decode(), "language": "pt"})
            heard.append(words(got.get("text", "")))
        spans = align(pieces, heard, expected)
        scores = [match([w for piece in heard[i:j] for w in piece], expected[k]) for k, (i, j) in enumerate(spans)]
        # palavras sobrando numa fala = a voz repetiu um trecho
        extra = max(sum(len(heard[x]) for x in range(i, j)) / max(1, len(expected[k])) for k, (i, j) in enumerate(spans))
        quality = min(scores) - max(0.0, extra - 1.2)
        print("TAKE %d: %d pedaços, nota mínima %.2f (média %.2f), palavras sobrando x%.2f" % (attempt, len(pieces), min(scores), sum(scores) / len(scores), extra), flush=True)
        if best is None or quality > best[0]:
            best = (quality, raw, audio, rate, pieces, heard, spans, scores)
        if min(scores) >= .85 and extra <= 1.2:
            break
    _, raw, audio, rate, pieces, heard, spans, scores = best
    (FOLDER / "narracao_inteira.wav").write_bytes(raw)
    report = []
    for k, ((page, panel, text), (i, j)) in enumerate(zip(panels, spans)):
        a, b = pieces[i][0], pieces[j - 1][1]
        piece = audio[int(max(0, a - .05) * rate):int(b * rate)].copy()
        loud = np.where(np.abs(piece) > 300)[0]
        if len(loud):
            piece = piece[max(0, loud[0] - int(rate * .12)):min(len(piece), loud[-1] + int(rate * .3))]
        fade = min(len(piece), int(rate * .06))
        piece[-fade:] *= np.linspace(1, 0, fade)
        name = f"p{page}_q{panel}"
        (FOLDER / f"{name}.wav").write_bytes(wav_bytes(piece, rate))
        said = " ".join(w for piece_words in heard[i:j] for w in piece_words)
        report.append({"name": name, "start": round(a, 2), "duration": round(len(piece) / rate, 2), "score": round(scores[k], 2), "text": text, "heard": said})
        print("%s %6.2fs +%5.2fs nota %.2f  %s" % (name, a, len(piece) / rate, scores[k], said[:70]))
    (FOLDER / "cortes.json").write_text(json.dumps(report, ensure_ascii=False, indent=1), encoding="utf-8")


if __name__ == "__main__":
    main()
