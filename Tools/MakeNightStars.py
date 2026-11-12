"""Fase 15: mapa de estrelas do céu noturno (Data/Sky/NightStars.png), sempre igual (semente fixa).

Projeção equirretangular 2048x1024 (longitude x, latitude y). Cada estrela é um texel (algumas,
2x2), como os voxels do jogo; a densidade segue a área da esfera (mais esparsa perto dos polos
da imagem). Canais: R = brilho, G = fase do piscar, B = tom (0 azulado .. 1 amarelado).
Uma faixa mais densa e fraca imita a Via Láctea.

Uso: python Tools/MakeNightStars.py
"""
import math
import pathlib
import random

from PIL import Image

WIDTH, HEIGHT = 2048, 1024
STARS = 5200
TARGET = pathlib.Path(__file__).resolve().parents[1] / "Data" / "Sky" / "NightStars.png"


def main():
    rng = random.Random(1987)
    image = Image.new("RGB", (WIDTH, HEIGHT), (0, 0, 0))
    pixels = image.load()

    def put(x, y, brightness, size):
        phase = rng.randrange(256)
        tone = rng.randrange(256)
        for dx in range(size):
            for dy in range(size):
                px, py = (x + dx) % WIDTH, min(HEIGHT - 1, y + dy)
                old = pixels[px, py][0]
                pixels[px, py] = (max(old, brightness), phase, tone)

    for _ in range(STARS):
        # uniforme na esfera: z = cos(colatitude) uniforme
        z = rng.uniform(-1.0, 1.0)
        x = rng.randrange(WIDTH)
        y = int(math.acos(z) / math.pi * (HEIGHT - 1))
        magnitude = rng.random() ** 3.2
        brightness = int(40 + 215 * magnitude)
        put(x, y, brightness, 2 if magnitude > 0.82 else 1)
    # Via Láctea: faixa inclinada com estrelas fracas mais juntas
    for _ in range(9000):
        t = rng.uniform(0.0, 2.0 * math.pi)
        band = rng.gauss(0.0, 0.09)
        lon = t
        lat = math.asin(max(-1.0, min(1.0, 0.45 * math.sin(t) + band)))
        x = int(lon / (2.0 * math.pi) * WIDTH) % WIDTH
        y = int((math.pi / 2.0 - lat) / math.pi * (HEIGHT - 1))
        put(x, y, int(25 + 50 * rng.random() ** 2), 1)
    TARGET.parent.mkdir(parents=True, exist_ok=True)
    image.save(TARGET)
    print(f"VORTEX_NIGHT_STARS {STARS} -> {TARGET}")


if __name__ == "__main__":
    main()
