"""A HQ de abertura (Docs/03_Mundo/ABERTURA_QUADRINHOS.md): 2 folhas de 12 quadros e a música "Acorde".

python Tools/MakeIntroComic.py      (depois: Tools/ImportIntroComic.py com o editor fechado)
python Tools/MakeIntroComic.py --legendas   (só a letra e o tempo das palavras; não precisa importar nada)

- as folhas geradas (Docs/Midia/Abertura/Abertura_Folha_1.jpg e Abertura_Folha_2.png, 4 colunas x 3 linhas cada) vão para
  Data/UI/Abertura/folha_N.png; a que veio pequena é ampliada (Lanczos + um pouco de nitidez) para ~1800 px;
- acha os quadros pelas sarjetas cor de papel e grava os retângulos (0 a 1);
- a música (Docs/Midia/Abertura/Acorde.mp3) vira Data/UI/Abertura/MUS_Abertura.wav (a Unreal importa WAV);
- se houver a transcrição da música com o tempo das palavras (Data/Narracao/hq/acorde_palavras.json, do
  faster-whisper), acha em que segundo começa o trecho da letra de cada quadro;
- grava Data/UI/Abertura/abertura.json: a música e, por quadro, o retângulo, a letra (a legenda), o efeito e o tempo.
"""
import difflib
import json
import pathlib
import re
import shutil
import subprocess
import unicodedata

import numpy as np
from PIL import Image, ImageFilter

ROOT = pathlib.Path(__file__).resolve().parents[1]
OUT = ROOT / "Data" / "UI" / "Abertura"
SHEETS = [ROOT / "Docs" / "Midia" / "Abertura" / "Abertura_Folha_1.jpg", ROOT / "Docs" / "Midia" / "Abertura" / "Abertura_Folha_2.png"]
SONG = ROOT / "Docs" / "Midia" / "Abertura" / "Acorde.mp3"
WORDS = ROOT / "Data" / "Narracao" / "hq" / "acorde_palavras.json"

# a letra de cada quadro (na ordem das folhas, da esquerda para a direita e de cima para baixo) e o efeito
STORY = [
    ("Antes do nome, antes da dor,\nhavia o mar… havia Gênese.", ""),
    ("E em seu coração, sob a luz primordial,\ncresceu Yggdrasil, a árvore ancestral.", ""),
    ("Suas raízes cruzavam a imensidão,\nlevando a seiva ao mundo em criação.", "glow"),
    ("Onde ela fluía, a vida florescia,\ne a terra respirava em harmonia.", ""),
    ("Mas sob a copa, o céu se retorceu,\ne uma sombra entre as estrelas se ergueu.", "spin"),
    ("Chamaram de Vórtice a ferida no céu,\numa porta esquecida sob um negro véu.\nEle atraía tudo que vivia,\ncomo a maré que a terra consumia.", "spin"),
    ("Então os antigos, temendo o fim,\ngravaram nove Selos na pedra sem fim.", ""),
    ("Nove Selos… nove destinos…\nNove Guardiões em sono profundo.\nEnquanto sonharem sob a pedra,\no Vórtice não tomará o mundo.", "glow"),
    ("Mil anos se passaram, Valdora adormeceu,\njunto à Fonte de Jade, sob o silêncio do céu.", ""),
    ("Até que, vinte invernos atrás,\numa estrela caiu… e nada foi igual jamais.\nNão era estrela, nem luz passageira,\nmas do Vórtice, uma lasca derradeira.", ""),
    ("Rasgou a terra, fez a Caldeira despertar,\ne o primeiro Selo não pôde suportar.", "impact"),
    ("Azhûr!\nO Colosso despertou!", "shake"),
    ("Azhûr!\nE em seu peito o fogo se acendeu!", "shake"),
    ("Desde então, a seiva foge para o céu,\ne os cristais brilham sob a noite cruel.", "glow"),
    ("As feras já não conhecem quem são,\ne o medo se espalha por toda a região.", "shake"),
    ("Nas muralhas, a guarda se ergueu,\ne Rhea vigia onde o sol se escondeu.\nNoite após noite, sem poder descansar,\nenquanto os Guardiões começam a sonhar.", ""),
    ("Sonham com o fogo, sonham com o fim…\nO Vórtice desperta, e chama por mim?", "spin"),
    ("Mas nem tudo o que atravessa é levado,\nnem todo destino está condenado.\nPois às vezes, do abismo e da escuridão,\no Vórtice devolve uma alma ao chão.", "glow"),
    ("Uma luz atravessa o véu,\ne uma alma cai do céu.\nNo Estádio do Despertar,\nsem passado para recordar.", ""),
    ("Sem nome, sem memória,\nsem saber sua própria história.\nSó uma arma, a seiva a conceder,\ne um destino que começa a nascer.", ""),
    ("Os antigos os chamaram Despertos,\nos únicos que podem chegar perto\ndos Guardiões sem perder a razão,", "glow"),
    ("sem se perder na corrupção.", "shake"),
    ("Você é um deles…\nA última esperança a renascer.", ""),
    ("Acorde.", "awake"),
]


def words(text):
    text = unicodedata.normalize("NFKD", text.lower())
    text = "".join(c for c in text if not unicodedata.combining(c))
    return re.findall(r"[a-z]+", text)


def grid(img):
    """Os quadros pelas sarjetas cor de papel: 4 colunas x 3 linhas."""
    a = np.asarray(img).astype(float)
    paper = np.abs(a - np.array([235, 225, 205.])).sum(2) < 70

    def cells(profile, n):
        gutter = profile > .55
        runs, start = [], None
        for i, g in enumerate(list(gutter) + [True]):
            if not g and start is None:
                start = i
            if g and start is not None:
                if i - start > len(profile) / (n * 3):
                    runs.append((start, i))
                start = None
        return runs

    xs, ys = cells(paper.mean(0), 4), cells(paper.mean(1), 3)
    if len(xs) != 4 or len(ys) != 3:
        raise RuntimeError("grade não achada: %d colunas, %d linhas" % (len(xs), len(ys)))
    h, w = a.shape[:2]
    return [[round(x0 / w, 4), round(y0 / h, 4), round(x1 / w, 4), round(y1 / h, 4)] for y0, y1 in ys for x0, x1 in xs]


def timings(count):
    """Em que segundo começa a letra de cada quadro: a primeira palavra do trecho, procurada em ordem na transcrição."""
    if not WORDS.exists():
        return [None] * count
    heard = []
    for seg in json.loads(WORDS.read_text(encoding="utf-8")):
        for w in seg["words"]:
            for token in words(w["w"]):
                heard.append((token, w["s"], w["e"]))
    starts, sung, cursor = [], [], 0
    for text, _ in STORY[:count]:
        key = words(text)[:4]
        best, best_score = None, 0
        # a melhor janela de 4 palavras daqui para a frente (até ~90 palavras adiante), que mais bate com o começo
        for i in range(cursor, min(len(heard), cursor + 90)):
            got = [t for t, _, _ in heard[i:i + 4]]
            score = sum(1 for k in key if k in got) + (1 if got and got[0] == key[0] else 0)
            # a transcrição às vezes parte a palavra cantada ("Acorde" -> "a cor de"): compara as letras juntas
            if "".join(got).startswith("".join(key)[:6]):
                score = max(score, 3)
            if score > best_score:
                best, best_score = i, score
        if best is None or best_score < 2:
            starts.append(None)
            sung.append(None)
            continue
        words_sung = sing(text, heard, best)
        # o quadro começa na primeira palavra dele de verdade (a janela de 4 palavras às vezes pega o "a" do verso
        # anterior, e o quadro entrava antes do fim da legenda de antes)
        starts.append(round(words_sung[0][0] if words_sung else heard[best][1], 2))
        sung.append(words_sung)
        cursor = best + max(1, len(words(text)) // 2)
    return starts, sung


def sing(text, heard, start):
    """O tempo (início, fim) de cada palavra do trecho (separadas por espaço ou linha), casando em ordem com as
    palavras ouvidas; uma palavra escrita pode ser 2 ou 3 ouvidas ("Acorde" = "a cor de"). As que não acharem par
    ficam entre as vizinhas, para a letra aparecer junto com o canto mesmo assim."""
    out, j = [], start
    for word in text.split():
        target = "".join(words(word))
        found = None
        if target:
            best = .6
            for m in range(j, min(len(heard), j + 5)):
                # a palavra não pode estar longe da anterior: o "a" de "esperança a renascer" casava com o "A" do
                # "A cor de…" meio minuto depois, e o fim da legenda daquele quadro nunca aparecia
                if out and out[-1] and heard[m][1] > out[-1][1] + 6:
                    break
                for n in (1, 2, 3):
                    if m + n > len(heard):
                        break
                    cand = "".join(t for t, _, _ in heard[m:m + n])
                    ratio = difflib.SequenceMatcher(None, target, cand).ratio()
                    if ratio > best:
                        best, found = ratio, (m, n)
        if found:
            m, n = found
            out.append([heard[m][1], heard[m + n - 1][2]])
            j = m + n
        else:
            out.append(None)
    # Distribui cada sequência não reconhecida pelo intervalo entre as palavras vizinhas.
    # Antes, cada palavra do mesmo trecho recebia o mesmo tempo; a legenda ficava parada e
    # depois revelava várias palavras juntas, parecendo que a cutscene acelerava.
    i = 0
    while i < len(out):
        if out[i] is not None:
            i += 1
            continue
        first = i
        while i < len(out) and out[i] is None:
            i += 1
        count = i - first
        prev = out[first - 1][1] if first > 0 and out[first - 1] else heard[start][1]
        # Às vezes o casamento aproximado encontra uma palavra dentro do trecho
        # que acabou de ficar sem tempos. Absorve esses tempos sobrepostos no mesmo
        # intervalo para não quebrar a ordem da legenda.
        while i < len(out) and out[i] is not None and out[i][0] <= prev + .02:
            out[i] = None
            i += 1
            count += 1
        nxt = out[i][0] if i < len(out) and out[i] else prev + .35 * (count + 1)
        if nxt <= prev:
            nxt = prev + .35 * (count + 1)
        step = (nxt - prev) / (count + 1)
        for offset in range(count):
            word_start = prev + step * (offset + .15)
            word_end = prev + step * (offset + .85)
            out[first + offset] = [word_start, max(word_start + .06, word_end)]
    # em ordem (a palavra nunca começa antes da anterior)
    for i in range(1, len(out)):
        if out[i][0] < out[i - 1][0]:
            out[i] = [out[i - 1][0] + .05, max(out[i - 1][0] + .1, out[i][1])]
    return [[round(a, 2), round(b, 2)] for a, b in out]


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    rects, sizes = [], []
    for n, src in enumerate(SHEETS, start=1):
        img = Image.open(src).convert("RGB")
        if img.width < 1600:   # a folha veio pequena: amplia (sem inventar detalhe) para o zoom não pixelar
            scale = 1800 / img.width
            img = img.resize((round(img.width * scale), round(img.height * scale)), Image.LANCZOS)
            img = img.filter(ImageFilter.UnsharpMask(radius=2, percent=60, threshold=2))
        img.save(OUT / f"folha_{n}.png")
        sizes.append(list(img.size))
        rects.append(grid(img))
        print("FOLHA", n, img.size, len(rects[-1]), "quadros")
    for old in OUT.glob("pagina_*.png"):
        old.unlink()
    subprocess.run(["ffmpeg", "-y", "-v", "error", "-i", str(SONG), "-ar", "44100", "-ac", "2", str(OUT / "MUS_Abertura.wav")], check=True)
    duration = float(subprocess.run(["ffprobe", "-v", "error", "-show_entries", "format=duration", "-of", "csv=p=0", str(SONG)],
                                    capture_output=True, text=True).stdout.strip())
    starts, sung = timings(len(STORY))
    pages, k = [], 0
    for n, sheet in enumerate(rects, start=1):
        panels = []
        for r in sheet:
            text, fx = STORY[k]
            # "sung": o tempo de cada palavra da letra (a legenda aparece letra por letra junto com o canto)
            panels.append({"rect": r, "text": text, "fx": fx, "at": starts[k], "sung": sung[k]})
            print("Q%02d %7s  %s" % (k + 1, starts[k], text.split("\n")[0]))
            k += 1
        # o tamanho de verdade (a Unreal pode arredondar a textura para potência de 2)
        pages.append({"texture": f"T_Folha_{n}", "size": sizes[n - 1], "panels": panels})
    doc = {"_doc": "Gerado por Tools/MakeIntroComic.py.", "music": "MUS_Abertura", "musicLength": round(duration, 2), "pages": pages}
    (OUT / "abertura.json").write_text(json.dumps(doc, ensure_ascii=False, indent=1), encoding="utf-8")


def captions_only():
    """Só a letra e o tempo das palavras, no abertura.json que já existe (as folhas e a música ficam como estão)."""
    doc = json.loads((OUT / "abertura.json").read_text(encoding="utf-8"))
    panels = [q for page in doc["pages"] for q in page["panels"]]
    starts, sung = timings(len(panels))
    for k, q in enumerate(panels):
        q["text"], q["fx"] = STORY[k]
        q["at"], q["sung"] = starts[k], sung[k]
    (OUT / "abertura.json").write_text(json.dumps(doc, ensure_ascii=False, indent=1), encoding="utf-8")
    print("LEGENDAS", len(panels), "quadros")


if __name__ == "__main__":
    import sys
    captions_only() if sys.argv[1:] == ["--legendas"] else main()
