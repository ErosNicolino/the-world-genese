"""Os efeitos Niagara dos poderes das espadas e dos arcos (14/11), um por tipo de poder: /Game/Vortex/Fx/Poderes/NS_Pw<tipo>.
Rodar com o editor aberto e o Monolith ligado:  python BuildPowerFx.py [tipo ...]   (sem nome = todos)
O jogo (VortexMonsterFx.cpp) solta o sistema preso ao herói, virado para onde ele olha, com User.Color = a cor do elemento.
Medidas: o centro é o meio do herói; o chão fica 88 cm abaixo; +X é a frente."""
import sys
import fx
from sheet import sheet

G = -86          # o chão
WHITE = (1, 1, 1)
DARK = (.05, .04, .03)
POP = ((0, 0), (.15, 1), (.6, .9), (1, 0))      # aparece rápido e some
HOLD = ((0, 0), (.1, 1), (.8, 1), (1, 0))


def ring(s, name, delay=0, tex='circle_02', size=110, to=5.0, life=.5, glow=5, z=G, color='user', spin=0):
    s.emit(name, tex, count=1, delay=delay, life=(life, life), size=(size, size), grow=((0, .25), (.25, to * .75), (1, to)), fade=((0, 1), (.5, .8), (1, 0)), facing='chao',
           rot=(0, 0), spin=spin, offset=(0, 0, z), blend='Tr', glow=min(glow, 1.3), color=color)


def rune(s, name, tex, delay=0, size=420, life=1.2, spin=90, glow=4, z=G, grow=((0, .5), (.2, 1), (1, 1.08)), color='user'):
    s.emit(name, tex, count=1, delay=delay, life=(life, life), size=(size, size), grow=grow, fade=HOLD, facing='chao', rot=(0, 0), spin=spin, offset=(0, 0, z), blend='Tr', glow=min(glow, 1.3), color=color)


def sparks(s, name, delay=0, count=28, speed=(350, 800), size=(28, 70), life=(.3, .65), gravity=-600, tex='star_07', offset=(0, 0, 0), glow=7, color='user', radius=10, drag=1.2):
    s.emit(name, tex, count=count, delay=delay, life=life, size=size, speed=speed, gravity=gravity, offset=offset, glow=glow, color=color, radius=radius, drag=drag,
           grow=((0, 1), (1, .3)), fade=((0, 1), (.7, 1), (1, 0)))


def flash(s, name, delay=0, size=260, offset=(0, 0, 0), tex='flare_01', life=.16, glow=8, color=WHITE):
    s.emit(name, tex, count=1, delay=delay, life=(life, life), size=(size, size), grow=((0, .4), (.3, 1.3), (1, 1.6)), fade=((0, 1), (.4, 1), (1, 0)), rot=(0, 0), offset=offset,
           glow=glow, color=color)


def glow(s, name, delay=0, size=420, life=.7, offset=(0, 0, 0), tex='circle_05', glow=.9, color='user', grow=((0, .5), (.3, 1), (1, 1.2))):
    s.emit(name, tex, count=1, delay=delay, life=(life, life), size=(size, size), grow=grow, fade=POP, rot=(0, 0), offset=offset, glow=min(glow, .5) if tex == 'circle_05' else glow, color=color)


def scorch(s, name, delay=0, size=380, life=1.8, tex='scorch_02'):
    s.emit(name, tex, blend='Tr', glow=1, count=1, delay=delay, life=(life, life), size=(size, size), fade=((0, 0), (.05, .85), (.7, .7), (1, 0)), facing='chao', rot=(0, 360),
           offset=(0, 0, G + 2), color=DARK)


def dust(s, name, delay=0, count=10, speed=(120, 320), size=(110, 200), offset=(0, 0, G + 20), tex='smoke_04', color=(.45, .38, .3), life=(.5, .9)):
    s.emit(name, tex, blend='Tr', glow=1, count=count, delay=delay, life=life, size=size, speed=speed, gravity=60, offset=offset, color=color, radius=40, drag=2.5,
           grow=((0, .5), (1, 1.8)), fade=((0, 0), (.15, .55), (1, 0)))


def slash(s, name, tex='slash_02', delay=0, size=420, life=.28, facing='chao', rot=(0, 0), spin=0, offset=(60, 0, 0), glow=6, color='user', to=1.35):
    s.emit(name, tex, count=1, delay=delay, life=(life, life), size=(size, size), grow=((0, .6), (.3, 1), (1, to)), fade=((0, 1), (.5, 1), (1, 0)), facing=facing, rot=rot, spin=spin,
           offset=offset, blend='Tr', glow=min(glow, 1.3), color=color)


def wave(s, name, tex='circle_02', delay=0, size=260, speed=1400, life=.5, glow=5, color='user', z=0, to=1.7, facing='frente', rot=(0, 0)):
    """Uma imagem que sai voando para a frente (onda, anel de disparo)."""
    s.emit(name, tex, base='cone', count=1, delay=delay, life=(life, life), size=(size, size), speed=(speed, speed), cone=((1, 0, 0), 0), drag=0, gravity=0,
           grow=((0, .5), (1, to)), fade=((0, 1), (.6, .9), (1, 0)), facing=facing, rot=rot, offset=(40, 0, z), blend='Tr', glow=min(glow, 1.3), color=color)


def jet(s, name, tex='star_07', delay=0, count=24, speed=(700, 1500), angle=14, size=(28, 70), life=(.2, .45), axis=(1, 0, 0), offset=(40, 0, 0), glow=7, color='user', gravity=0):
    """Estouro num cone (faíscas para a frente, para cima...)."""
    s.emit(name, tex, base='cone', count=count, delay=delay, life=life, size=size, speed=speed, cone=(axis, angle), drag=1.0, gravity=gravity, offset=offset, glow=glow, color=color,
           grow=((0, 1), (1, .3)), fade=((0, 1), (.7, 1), (1, 0)))


def rise(s, name, tex='flame_06', delay=0, rate=40, dur=.5, speed=(250, 520), angle=10, size=(110, 190), life=(.4, .8), offset=(0, 0, G), glow=5, color='user', radius=120, blend='Tr'):
    """Jorro contínuo para cima (chamas, luz que sobe)."""
    s.emit(name, tex, base='fonte', blend=blend, rate=rate, dur=dur, delay=delay, life=life, size=size, speed=speed, cone=((0, 0, 1), angle), drag=.4, gravity=0, offset=offset,
           glow=min(glow, 1.5), color=color, rot=(-12, 12), grow=((0, .6), (.4, 1.1), (1, .5)), fade=POP, shape=('Cylinder', {'radius': radius, 'height': 4}) if radius else None)


# ---------------------------------------------------------------------------------------------------------------------
def Pwdash(s):          # a investida: riscos e faíscas para trás, um corte na frente
    slash(s, 'Corte', 'slash_04', size=380, facing='frente', offset=(120, 0, 10), life=.26, rot=(0, 0))
    wave(s, 'Rastro', 'trace_01', size=520, speed=-900, life=.4, facing='chao', to=1.2, glow=4)
    jet(s, 'Faiscas', count=26, axis=(-1, 0, 0), angle=18, speed=(500, 1200), offset=(0, 0, -20))
    dust(s, 'Poeira', count=8, offset=(-40, 0, G + 20))
    flash(s, 'Clarao', size=200, offset=(90, 0, 10))


def Pwgroundslam(s):    # a pancada no chão: a carga desce, o chão racha e levanta
    rune(s, 'Carga', 'magic_02', size=460, life=.3, spin=260, grow=((0, 1.1), (1, .35)), glow=4)
    scorch(s, 'Marca', delay=.25)
    ring(s, 'Anel', delay=.25, size=130, to=5.6, life=.7)
    ring(s, 'Anel2', delay=.33, size=120, to=4.0, life=.7, tex='circle_04', color=WHITE, glow=3)
    flash(s, 'Clarao', delay=.25, size=380, offset=(0, 0, G + 40))
    sparks(s, 'Faiscas', delay=.25, count=34, offset=(0, 0, G + 10), speed=(450, 950))
    jet(s, 'Pedras', 'dirt_02', delay=.25, count=14, axis=(0, 0, 1), angle=50, speed=(450, 800), size=(60, 120), life=(.5, .8), offset=(0, 0, G), gravity=-1400, color=(.5, .42, .33), glow=2)
    dust(s, 'Poeira', delay=.27, count=14, speed=(250, 520))


def Pwruneseal(s):      # o selo de runas: três círculos girando e uma coluna de luz
    rune(s, 'Selo', 'magic_03', size=560, life=1.5, spin=70)
    rune(s, 'Selo2', 'magic_01', delay=.1, size=360, life=1.3, spin=-130, color=WHITE, glow=3)
    rune(s, 'Selo3', 'magic_02', delay=.2, size=680, life=1.1, spin=40, glow=3)
    ring(s, 'Anel', delay=.2, size=130, to=5.0, life=.7, tex='circle_04')
    rise(s, 'Luz', 'circle_05', delay=.15, rate=30, dur=.9, speed=(160, 380), size=(40, 90), radius=190, glow=5)
    rise(s, 'Estrelas', 'star_07', delay=.15, rate=30, dur=.9, speed=(120, 300), size=(30, 60), radius=220, glow=7, color=WHITE)
    glow(s, 'Brilho', delay=.2, size=560, life=1.0, offset=(0, 0, G + 60))


def Pwshockwave(s):     # a onda de choque: três arcos saindo para a frente
    for i, d in enumerate((.2, .28, .36)):
        wave(s, 'Onda%d' % i, 'circle_02', delay=d, size=240 + i * 50, speed=1500 - i * 150, life=.5, color='user' if i != 1 else WHITE, glow=5 if i != 1 else 3)
    jet(s, 'Faiscas', delay=.2, count=30, angle=20, speed=(800, 1700))
    wave(s, 'Risco', 'trace_01', delay=.2, size=600, speed=700, life=.45, facing='chao', to=1.3, glow=4)
    flash(s, 'Clarao', delay=.2, size=260, offset=(80, 0, 0))
    dust(s, 'Poeira', delay=.22, count=8, offset=(80, 0, G + 20))


def Pwwhirl(s):         # o redemoinho: lâminas de vento girando em volta
    rune(s, 'Giro', 'twirl_01', size=560, life=.9, spin=720, z=-10, glow=5)
    rune(s, 'Giro2', 'twirl_03', delay=.05, size=440, life=.8, spin=-560, z=30, color=WHITE, glow=3)
    rune(s, 'Giro3', 'twirl_02', delay=.1, size=660, life=.7, spin=600, z=-50, glow=4)
    ring(s, 'Anel', delay=.15, size=130, to=4.6, life=.5, tex='circle_03')
    s.emit('Vento', 'star_07', count=40, delay=.05, life=(.4, .8), size=(28, 64), speed=(120, 320), radius=230, gravity=260, glow=7, grow=((0, 1), (1, .3)), drag=.5)
    dust(s, 'Poeira', delay=.1, count=12, speed=(200, 420))


def Pworbit(s):         # as órbitas: um círculo arcano e estrelas rodando
    rune(s, 'Circulo', 'magic_02', size=480, life=1.3, spin=220)
    rune(s, 'Circulo2', 'magic_01', delay=.05, size=320, life=1.2, spin=-300, z=20, color=WHITE, glow=3)
    ring(s, 'Anel', size=120, to=4.4, life=.6, tex='circle_01', glow=4)
    s.emit('Estrelas', 'star_07', count=24, delay=.05, life=(.6, 1.1), size=(50, 100), speed=(40, 140), radius=210, gravity=120, glow=8, color=WHITE, spin=300, grow=((0, .4), (.3, 1), (1, .2)))
    glow(s, 'Brilho', size=520, life=.9, offset=(0, 0, 0))
    rise(s, 'Fios', 'trace_01', rate=18, dur=.7, speed=(200, 420), size=(120, 220), radius=200, glow=4)


def Pwflurry(s):        # a rajada de golpes: quatro riscos seguidos e um corte final
    for i, (d, r, z) in enumerate(((0, 25, 30), (.1, -40, -10), (.2, 60, 20), (.28, -20, 0))):
        slash(s, 'Risco%d' % i, 'scratch_01', delay=d, size=330, life=.18, facing='frente', rot=(r, r), offset=(110, 0, z), color='user' if i % 2 == 0 else WHITE, glow=6)
        jet(s, 'Faisca%d' % i, delay=d, count=9, angle=35, speed=(400, 900), offset=(110, 0, z), life=(.15, .3))
    slash(s, 'Final', 'slash_03', delay=.34, size=460, life=.26, facing='frente', offset=(120, 0, 5))
    flash(s, 'Clarao', delay=.34, size=260, offset=(120, 0, 5))


def Pwsweep(s):         # a varredura: um arco largo rente ao chão
    slash(s, 'Arco', 'slash_02', delay=.1, size=620, life=.36, facing='chao', spin=300, offset=(0, 0, -10))
    slash(s, 'Arco2', 'slash_04', delay=.15, size=520, life=.34, facing='chao', rot=(120, 120), spin=300, offset=(0, 0, 10), color=WHITE, glow=3)
    ring(s, 'Anel', delay=.15, size=130, to=4.8, life=.45, tex='circle_03', z=-30)
    s.emit('Faiscas', 'star_07', count=32, delay=.15, life=(.25, .5), size=(28, 64), speed=(500, 1000), radius=60, gravity=-200, glow=7, grow=((0, 1), (1, .3)), offset=(0, 0, -10), drag=1.2)
    dust(s, 'Poeira', delay=.17, count=10, speed=(250, 480))


def Pwreap(s):          # a ceifada: um giro completo e um corte alto
    slash(s, 'Giro', 'slash_04', delay=.1, size=640, life=.5, facing='chao', spin=720, offset=(0, 0, 0))
    slash(s, 'Corte', 'slash_01', delay=.15, size=520, life=.3, facing='frente', rot=(-35, -35), offset=(110, 0, 20))
    ring(s, 'Anel', delay=.2, size=130, to=5.0, life=.4, tex='circle_02', z=-20)
    flash(s, 'Clarao', delay=.2, size=300, offset=(100, 0, 20), tex='star_04')
    sparks(s, 'Faiscas', delay=.2, count=26, speed=(450, 950), offset=(60, 0, 10))
    glow(s, 'Sombra', delay=.1, size=520, life=.5)


def Pwoverload(s):      # a sobrecarga: energia estoura do corpo
    rune(s, 'Circulo', 'magic_03', size=520, life=.9, spin=260)
    glow(s, 'Nucleo', size=560, life=.6, grow=((0, .2), (.3, 1.2), (1, 1.6)))
    flash(s, 'Clarao', delay=.05, size=420)
    ring(s, 'Anel', delay=.1, size=130, to=5.4, life=.5, tex='circle_04')
    ring(s, 'Anel2', delay=.18, size=120, to=3.8, life=.45, color=WHITE, glow=3, z=-20)
    sparks(s, 'Raios', delay=.05, count=10, speed=(150, 420), size=(170, 300), tex='spark_01', gravity=0, life=(.15, .32))
    sparks(s, 'Faiscas', delay=.05, count=30, speed=(500, 1200), gravity=0, life=(.2, .45))
    s.emit('Estrelas', 'star_07', count=18, delay=.1, life=(.3, .6), size=(50, 110), speed=(200, 520), radius=40, glow=8, color=WHITE, spin=200, grow=((0, 1), (1, .2)))


def Pwholyslam(s):      # o golpe sagrado: um feixe desce do céu e abre um selo
    rune(s, 'Selo', 'magic_01', size=520, life=1.2, spin=60)
    s.emit('Feixe', 'trace_01', count=3, delay=.12, life=(.4, .4), size=(800, 800), rot=(0, 0), offset=(0, 0, 280), glow=6, grow=((0, .3), (.4, 1), (1, .8)), fade=((0, 0), (.4, 1), (1, 0)),
           color=WHITE)
    glow(s, 'Luz', size=520, life=.9, offset=(0, 0, 160))
    ring(s, 'Anel', delay=.3, size=130, to=5.4, life=.5, tex='circle_04', color=WHITE, glow=4)
    ring(s, 'Anel2', delay=.36, size=120, to=4.0, life=.5)
    flash(s, 'Clarao', delay=.3, size=460, offset=(0, 0, G + 60))
    sparks(s, 'Faiscas', delay=.3, count=30, offset=(0, 0, G + 10), speed=(400, 900), tex='star_07', color=WHITE, gravity=-300)
    rise(s, 'Sobe', 'circle_05', delay=.3, rate=30, dur=.5, speed=(180, 420), size=(30, 70), radius=200, glow=5)
    scorch(s, 'Marca', delay=.3, size=340, life=1.4, tex='scorch_01')


def Pwtidewave(s):      # a onda da maré: uma parede de água avança
    for i, d in enumerate((.1, .22)):
        wave(s, 'Onda%d' % i, 'circle_01', delay=d, size=380 - i * 60, speed=1200, life=.6, to=1.9, glow=4, color='user' if i == 0 else WHITE)
    rune(s, 'Giro', 'twirl_03', delay=.1, size=520, life=.8, spin=160)
    jet(s, 'Gotas', delay=.1, count=40, angle=38, speed=(500, 1200), size=(50, 100), life=(.4, .7), gravity=-900, offset=(60, 0, -20))
    jet(s, 'Espuma', 'smoke_01', delay=.12, count=12, angle=30, speed=(300, 700), size=(120, 200), life=(.4, .7), offset=(60, 0, -40), color=WHITE, glow=1.5)
    ring(s, 'Anel', delay=.1, size=120, to=4.0, life=.5, tex='circle_03')


def Pwfirewall(s):      # a parede de fogo: chamas sobem em volta
    scorch(s, 'Marca', delay=.1, size=420, tex='scorch_01')
    ring(s, 'Anel', delay=.1, size=130, to=4.6, life=.5)
    rise(s, 'Chamas', 'flame_06', delay=.1, rate=50, dur=.7, speed=(220, 480), size=(170, 280), radius=200, glow=5)
    rise(s, 'Labareda', 'fire_02', delay=.1, rate=26, dur=.7, speed=(120, 300), size=(150, 240), radius=190, glow=3)
    rise(s, 'Chamas2', 'flame_05', delay=.15, rate=30, dur=.6, speed=(200, 460), size=(120, 200), radius=160, glow=4, color=(1, .9, .6))
    rise(s, 'Brasas', 'circle_05', delay=.1, rate=40, dur=.8, speed=(300, 700), angle=22, size=(12, 28), radius=200, glow=8)
    rise(s, 'Fumaca', 'smoke_07', delay=.3, rate=12, dur=.7, speed=(160, 320), size=(160, 260), radius=180, blend='Tr', color=(.12, .1, .1), glow=1, life=(.7, 1.1))
    flash(s, 'Clarao', delay=.1, size=380, offset=(0, 0, G + 60), tex='fire_01', life=.3)


def Pwheal(s):          # a cura: um selo suave e luz subindo pelo corpo
    rune(s, 'Selo', 'magic_01', size=460, life=1.3, spin=60)
    ring(s, 'Anel', size=120, to=3.8, life=.7, tex='circle_01', glow=4)
    rise(s, 'Luz', 'circle_05', rate=30, dur=.9, speed=(120, 280), size=(30, 70), radius=110, glow=5)
    rise(s, 'Cruzes', 'star_07', delay=.1, rate=22, dur=.9, speed=(140, 320), size=(40, 80), radius=120, glow=8, color=WHITE)
    glow(s, 'Brilho', size=460, life=1.0)


def Pwvolley(s):        # a saraivada: um círculo na frente do arco e três anéis de disparo
    slash(s, 'Mira', 'magic_02', size=260, life=.4, facing='frente', spin=220, offset=(90, 0, 30), glow=5, to=1.2)
    for i, d in enumerate((.05, .12, .19)):
        wave(s, 'Anel%d' % i, 'circle_02', delay=d, size=150, speed=1700, life=.3, to=2.0, z=30, color='user' if i != 1 else WHITE)
    jet(s, 'Faiscas', count=30, angle=24, speed=(900, 2000), offset=(90, 0, 30), life=(.15, .35))
    flash(s, 'Clarao', size=220, offset=(90, 0, 30), tex='muzzle_02')


def Pwpierce(s):        # a flecha perfurante: uma lança de luz reta
    wave(s, 'Lanca', 'trace_01', size=900, speed=2600, life=.35, facing='chao', to=1.2, glow=7, z=30)
    wave(s, 'Estrela', 'star_04', size=260, speed=2400, life=.3, to=1.4, z=30, color=WHITE)
    for i, d in enumerate((0, .06)):
        wave(s, 'Anel%d' % i, 'circle_02', delay=d, size=120, speed=900 + i * 500, life=.28, to=2.4, z=30)
    jet(s, 'Faiscas', count=22, angle=10, speed=(1200, 2600), offset=(90, 0, 30), life=(.15, .3))
    flash(s, 'Clarao', size=240, offset=(90, 0, 30))


def Pwburst3(s):        # a rajada tripla: três disparos em sequência
    for i, d in enumerate((0, .15, .3)):
        wave(s, 'Anel%d' % i, 'circle_02', delay=d, size=140, speed=1500, life=.24, to=2.0, z=30, color='user' if i != 1 else WHITE)
        jet(s, 'Faisca%d' % i, delay=d, count=12, angle=18, speed=(900, 1800), offset=(90, 0, 30), life=(.12, .28))
        flash(s, 'Clarao%d' % i, delay=d, size=180, offset=(90, 0, 30), tex='muzzle_02', life=.1)


def Pwarrowrain(s):     # a chuva de flechas: um selo no alto e riscos caindo
    rune(s, 'Selo', 'magic_03', size=520, life=1.2, spin=80)
    rune(s, 'Ceu', 'magic_02', size=420, life=1.0, spin=-120, z=320, color=WHITE, glow=3)
    glow(s, 'Luz', size=520, life=.8, offset=(0, 0, 320))
    s.emit('Chuva', 'trace_01', base='fonte', rate=60, dur=.7, delay=.1, life=(.22, .3), size=(170, 240), rot=(0, 0), speed=(1500, 1900), cone=((0, 0, -1), 4), drag=0, gravity=0,
           offset=(0, 0, 320), glow=6, shape=('Cylinder', {'radius': 210, 'height': 4}), fade=((0, 1), (.8, 1), (1, 0)))
    ring(s, 'Anel', delay=.1, size=120, to=4.6, life=.6, tex='circle_04')
    rise(s, 'Impactos', 'star_02', delay=.25, rate=30, dur=.6, speed=(60, 160), size=(50, 100), radius=210, glow=7, color=WHITE, life=(.15, .3))


def Pwexplosive(s):     # a flecha explosiva: o estouro na saída
    flash(s, 'Clarao', size=360, offset=(90, 0, 30))
    glow(s, 'Fogo', tex='fire_01', size=380, life=.5, offset=(90, 0, 30), glow=5)
    wave(s, 'Anel', 'circle_04', size=170, speed=1200, life=.3, to=2.2, z=30)
    jet(s, 'Faiscas', count=34, angle=40, speed=(600, 1500), offset=(90, 0, 30), life=(.2, .45))
    jet(s, 'Fumaca', 'smoke_04', count=8, angle=40, speed=(150, 380), size=(140, 220), life=(.5, .8), offset=(90, 0, 30), color=(.25, .22, .2), glow=1)


def Pwfireball(s):      # a bola de fogo: um círculo arcano e a chama saindo
    slash(s, 'Mira', 'magic_01', size=300, life=.5, facing='frente', spin=240, offset=(90, 0, 30), glow=5, to=1.25)
    glow(s, 'Fogo', tex='fire_02', size=380, life=.5, offset=(100, 0, 30), glow=5)
    wave(s, 'Bola', 'fire_01', size=260, speed=1300, life=.4, facing=None, to=1.3, z=30, glow=6)
    jet(s, 'Brasas', delay=.03, count=30, angle=28, speed=(700, 1600), offset=(90, 0, 30), life=(.2, .45))
    flash(s, 'Clarao', size=300, offset=(90, 0, 30))


def PwCharge(s):        # segurando o poder: a energia converge para o herói
    rune(s, 'Circulo', 'magic_02', size=520, life=.7, spin=300, grow=((0, 1.1), (1, .4)))
    glow(s, 'Nucleo', size=300, life=.7, grow=((0, .3), (1, 1.2)))
    s.emit('Fios', 'star_07', count=30, life=(.35, .6), size=(28, 64), speed=(-520, -300), radius=240, glow=7, grow=((0, 1), (1, .3)), drag=0)
    ring(s, 'Anel', size=520, to=.25, life=.55, tex='circle_02', glow=4)


def PwBasic(s):         # as armas sem poder próprio: um corte limpo
    slash(s, 'Corte', 'slash_02', size=420, life=.25, facing='frente', offset=(110, 0, 10))
    jet(s, 'Faiscas', delay=.03, count=14, angle=35, speed=(400, 900), offset=(110, 0, 10), life=(.15, .3))
    flash(s, 'Clarao', size=200, offset=(110, 0, 10))


ALL = [Pwdash, Pwgroundslam, Pwruneseal, Pwshockwave, Pwwhirl, Pworbit, Pwflurry, Pwsweep, Pwreap, Pwoverload, Pwholyslam, Pwtidewave, Pwfirewall, Pwheal, Pwvolley, Pwpierce,
       Pwburst3, Pwarrowrain, Pwexplosive, Pwfireball, PwCharge, PwBasic]

if __name__ == '__main__':
    want = set(sys.argv[1:])
    for build in ALL:
        if want and build.__name__ not in want:
            continue
        system = fx.Sys('NS_' + build.__name__)
        build(system)
        system.done()
        end = max(system.end, .6)
        sheet('NS_' + build.__name__, [round(end * f, 2) for f in (.08, .25, .42, .6, .85)])
    print('FALHAS:', len(fx.FAILS))
    for fail in fx.FAILS:
        print('  ', fail)
