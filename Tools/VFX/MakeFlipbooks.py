"""As texturas animadas dos efeitos dos poderes (07/11, o usuário: "pode usar o EffectCraft para criar efeito"), feitas no
EffectCraft pela linha de comando (um script no estilo do After Effects por efeito) e juntadas numa folha de 4 x 4
quadros (a Unreal toca quadro a quadro: M_PowerFlipbook, AVortexPowerFx).

python Tools/VFX/MakeFlipbooks.py [nome ...]   ->   Data/VFX/Flipbooks/T_FB_<nome>.png (512 x 512, 16 quadros de 128)

Cada efeito: as partículas do CC Particle Systems II num quadro preto transparente, com o brilho (Glow) por cima; o nome
diz o tipo: agua (o respingo que sobe e cai), poeira (a nuvem de terra que abre e sobe), brilho (as estrelinhas que
estouram), faisca (as faíscas rápidas), fumaca (a fumaça escura que sobe devagar), fogo (as chamas que sobem), folhas
(as folhas que caem rodando), gelo (os cristais que estouram), raio (o clarão do raio), luz (a explosão de luz).
"""
import os
import subprocess
import sys

from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
CLI = r"C:\Program Files\ArtCraft\EffectCraft\effectcraft-cli.exe"
OUT = os.path.join(ROOT, "Data", "VFX", "Flipbooks")
WORK = os.path.join(OUT, "_trabalho")
SIZE, FRAMES, FPS = 128, 16, 16

# nome -> (partículas, brilho): os valores do CC Particle Systems II (os mesmos ids do painel)
FX = {
    "agua": ({"longevity": .8, "pos": [64, 112], "rx": 22, "ry": 2, "anim": 2, "vel": 4.2, "grav": 2.2, "extra": .6, "type": 2,
              "bsize": 1.6, "dsize": .6, "var": 40, "bcol": [.85, .97, 1], "dcol": [.25, .6, .95], "rate": 9}, 6),
    "poeira": ({"longevity": 1.0, "pos": [64, 110], "rx": 30, "ry": 4, "anim": 3, "vel": 1.6, "grav": -.25, "extra": .4, "type": 2,
                "bsize": 2.6, "dsize": 4.2, "var": 50, "bcol": [.78, .66, .5], "dcol": [.45, .38, .3], "rate": 7, "opacity": 70}, 0),
    "brilho": ({"longevity": .7, "pos": [64, 64], "rx": 4, "ry": 4, "anim": 0, "vel": 2.4, "grav": .2, "extra": 1, "type": 4,
                "bsize": .9, "dsize": .1, "var": 60, "bcol": [1, 1, .85], "dcol": [1, .8, .3], "rate": 6}, 14),
    "faisca": ({"longevity": .5, "pos": [64, 80], "rx": 3, "ry": 3, "anim": 0, "vel": 5, "grav": 1.2, "extra": 1, "type": 0,
                "bsize": .6, "dsize": .2, "var": 30, "bcol": [1, .95, .7], "dcol": [1, .45, .1], "rate": 10}, 10),
    "fumaca": ({"longevity": 1.0, "pos": [64, 108], "rx": 16, "ry": 4, "anim": 3, "vel": 1.0, "grav": -.6, "extra": .3, "type": 2,
                "bsize": 2.2, "dsize": 4.6, "var": 40, "bcol": [.3, .28, .32], "dcol": [.12, .11, .14], "rate": 6, "opacity": 75}, 0),
    "fogo": ({"longevity": .8, "pos": [64, 112], "rx": 18, "ry": 3, "anim": 6, "vel": 1.8, "grav": -.9, "extra": .8, "type": 2,
              "bsize": 2.0, "dsize": .4, "var": 40, "bcol": [1, .9, .5], "dcol": [.9, .2, .02], "rate": 9}, 10),
    "folhas": ({"longevity": 1.0, "pos": [64, 20], "rx": 40, "ry": 6, "anim": 4, "vel": .8, "grav": .7, "extra": 1, "type": 6,
                "bsize": 1.0, "dsize": .8, "var": 40, "bcol": [.5, .9, .3], "dcol": [.2, .55, .15], "rate": 5}, 0),
    "gelo": ({"longevity": .7, "pos": [64, 70], "rx": 5, "ry": 5, "anim": 0, "vel": 3.4, "grav": .9, "extra": 1, "type": 6,
              "bsize": 1.2, "dsize": .3, "var": 50, "bcol": [.95, 1, 1], "dcol": [.45, .75, 1], "rate": 8}, 8),
    "luz": ({"longevity": .6, "pos": [64, 64], "rx": 2, "ry": 2, "anim": 0, "vel": 1.6, "grav": 0, "extra": 1, "type": 2,
             "bsize": 3.0, "dsize": .2, "var": 30, "bcol": [1, 1, .9], "dcol": [1, .85, .4], "rate": 8}, 18),
}


def script(name, p, glow):
    """O script do efeito: a composição, o sólido com as partículas e o brilho; o nascimento só no começo (um estouro)."""
    js = """
var c = app.project.items.addComp("%(name)s", %(S)d, %(S)d, 1, %(dur)f, %(fps)d);
var s = c.layers.addSolid([0,0,0], "p", %(S)d, %(S)d, 1, %(dur)f);
var e = s.Effects.addProperty("CC Particle Systems II");
function P(path, v) { var parts = path.split("/"); var q = e; for (var i = 0; i < parts.length; i++) q = q.property(parts[i]); q.setValue(v); return q; }
P("longevity", %(longevity)f);
P("producer/position", [%(px)f, %(py)f]); P("producer/radiusX", %(rx)f); P("producer/radiusY", %(ry)f);
P("physics/animation", %(anim)d); P("physics/velocity", %(vel)f); P("physics/gravity", %(grav)f); P("physics/extra", %(extra)f);
P("particle/particleType", %(type)d); P("particle/birthSize", %(bsize)f); P("particle/deathSize", %(dsize)f);
P("particle/sizeVariation", %(var)f); P("particle/maxOpacity", %(opacity)f);
P("particle/birthColor", [%(b0)f, %(b1)f, %(b2)f, 1]); P("particle/deathColor", [%(d0)f, %(d1)f, %(d2)f, 1]);
var r = P("birthRate", %(rate)f); r.setValueAtTime(0, %(rate)f); r.setValueAtTime(0.18, %(rate)f); r.setValueAtTime(0.25, 0);
%(glow)s
app.project.save(new File("%(proj)s"));
""" % {"name": name, "S": SIZE, "dur": FRAMES / FPS, "fps": FPS, "longevity": p["longevity"], "px": p["pos"][0], "py": p["pos"][1],
       "rx": p["rx"], "ry": p["ry"], "anim": p["anim"], "vel": p["vel"], "grav": p["grav"], "extra": p["extra"], "type": p["type"],
       "bsize": p["bsize"], "dsize": p["dsize"], "var": p["var"], "opacity": p.get("opacity", 100), "b0": p["bcol"][0], "b1": p["bcol"][1],
       "b2": p["bcol"][2], "d0": p["dcol"][0], "d1": p["dcol"][1], "d2": p["dcol"][2], "rate": p["rate"],
       "glow": ('var g = s.Effects.addProperty("Glow"); try { g.property("Glow Radius").setValue(%f); } catch (x) {}' % glow) if glow else "",
       "proj": os.path.join(WORK, name + ".ecproj").replace("\\", "/")}
    return js


def build(name):
    p, glow = FX[name]
    os.makedirs(WORK, exist_ok=True)
    jsx = os.path.join(WORK, name + ".jsx")
    open(jsx, "w", encoding="utf-8").write(script(name, p, glow))
    proj = os.path.join(WORK, name + ".ecproj")
    r = subprocess.run([CLI, "script", jsx, "--empty"], capture_output=True, text=True)
    if not os.path.exists(proj):
        raise SystemExit("script falhou %s: %s %s" % (name, r.stdout[-400:], r.stderr[-400:]))
    frames = []
    for k in range(FRAMES):
        f = os.path.join(WORK, "%s_%02d.png" % (name, k))
        subprocess.run([CLI, "--project", proj, "render-frame", "--comp", name, "--frame", str(k), "--out", f, "--transparent"],
                       capture_output=True, text=True)
        frames.append(Image.open(f).convert("RGBA").resize((SIZE, SIZE)))
    atlas = Image.new("RGBA", (SIZE * 4, SIZE * 4), (0, 0, 0, 0))
    for k, im in enumerate(frames):
        atlas.alpha_composite(im, ((k % 4) * SIZE, (k // 4) * SIZE))
    out = os.path.join(OUT, "T_FB_%s.png" % name)
    atlas.save(out)
    lit = sum(1 for k, im in enumerate(frames) if im.getbbox())
    assert lit >= 8, "%s: só %d quadros com desenho" % (name, lit)
    print("flipbook %-8s %d quadros com desenho -> %s" % (name, lit, out))


if __name__ == "__main__":
    for n in (sys.argv[1:] or FX):
        build(n)
