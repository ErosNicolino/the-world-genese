"""As animações do Azhûr (02/11, refeitas): importado pelo BuildAzhur.py, que define X, Y, Z, key() e o esqueleto.

Convenções (eixos do mundo, na pose de descanso; a frente é +Y e a direita do Azhûr é +X):
- X positivo: braço e coxa vão para a frente/para cima; peito e quadril com X negativo inclinam para a frente;
- Y: o braço direito abre para fora com Y negativo, o esquerdo com Y positivo; peito com Y inclina de lado;
- Z: o tronco gira; Z negativo leva o ombro direito para trás (vira à direita).

Todas seguem a mesma ideia de peso de um colosso de pedra de 7 m: antecipação clara (o jogador lê o golpe), o corpo
inteiro vai junto (o quadril antes do peito, o peito antes do braço), o impacto é seco e curto, o braço continua um
pouco depois (sobra de movimento) e a volta é lenta, com uma assentada no fim. Os tempos de impacto ficam no
Data/Creatures/azhur.json (o jogo marca o chão e acerta nesses instantes).
"""
import math

X = Y = Z = None   # o BuildAzhur.py troca pelos vetores dele
key = None


def setup(x, y, z, key_fn):
    global X, Y, Z, key
    X, Y, Z, key = x, y, z, key_fn


def blank():
    return {"rot": {}, "move": {}}


def add(p, bone, steps):
    p["rot"][bone] = p["rot"].get(bone, []) + steps


def move(p, offset):
    x, y, z = p["move"].get("Hips", (0, 0, 0))
    p["move"]["Hips"] = (x + offset[0], y + offset[1], z + offset[2])


def core(p, s):
    p.setdefault("scale", {})["Core"] = s


def shards(p, t, seconds, fall=0.0, spread=0.0):
    p["rot"]["ShardRing"] = [(Z, 90 * t / seconds)]
    for i in range(4):
        a = math.radians(45 + 90 * i)
        p["move"]["Shard_%d" % (i + 1)] = (math.cos(a) * spread, math.sin(a) * spread,
                                         .18 * math.sin(2 * math.pi * (t / seconds) * 2 + i * 1.6) - fall)


def guard(p, k=1.0):
    """A guarda de descanso: braços um pouco à frente e dobrados (os punhos pesados não pendem retos)."""
    add(p, "UpperArm_R", [(X, 8 * k), (Y, -7 * k)])
    add(p, "UpperArm_L", [(X, 8 * k), (Y, 7 * k)])
    add(p, "Forearm_R", [(X, 16 * k)])
    add(p, "Forearm_L", [(X, 16 * k)])


def stance(p, k=1.0):
    """Base larga: joelhos levemente dobrados, o peso baixo."""
    add(p, "Thigh_R", [(X, 8 * k), (Y, -4 * k)])
    add(p, "Thigh_L", [(X, 8 * k), (Y, 4 * k)])
    add(p, "Shin_R", [(X, -14 * k)])
    add(p, "Shin_L", [(X, -14 * k)])
    add(p, "Foot_R", [(X, 6 * k)])
    add(p, "Foot_L", [(X, 6 * k)])
    move(p, (0, 0, -.12 * k))


# ---- movimento -------------------------------------------------------------------------------------

def idle(t, T=3.2):
    """Parado: respira pesado, o peso passa devagar de uma perna para a outra e a cabeça vigia em volta."""
    p = blank()
    w = 2 * math.pi * t / T
    shift = math.sin(w)
    stance(p)
    guard(p)
    add(p, "Hips", [(Y, 2.5 * shift), (Z, 2 * math.sin(w + .6))])
    add(p, "Chest", [(X, -3 + 1.2 * math.sin(2 * w)), (Y, -1.8 * shift), (Z, -2.5 * math.sin(w + 1.2))])
    add(p, "Head", [(Z, 9 * math.sin(w * .5 + .4)), (X, -4 + 2 * math.sin(w + 2))])
    add(p, "Thigh_R", [(X, 3 * max(0.0, shift))])
    add(p, "Thigh_L", [(X, 3 * max(0.0, -shift))])
    add(p, "Shin_R", [(X, -5 * max(0.0, shift))])
    add(p, "Shin_L", [(X, -5 * max(0.0, -shift))])
    add(p, "UpperArm_R", [(X, 3 * math.sin(w + .8))])
    add(p, "UpperArm_L", [(X, 3 * math.sin(w + 2.4))])
    add(p, "Hand_R", [(X, 6 + 4 * math.sin(w * 2))])
    add(p, "Hand_L", [(X, 6 + 4 * math.sin(w * 2 + 1))])
    move(p, (.05 * shift, 0, .025 * math.sin(2 * w)))
    core(p, 1 + .08 * math.sin(2 * w))
    shards(p, t, T)
    return p


def walk(t, T=1.9):
    """Andar de colosso: passo longo e lento, o pé bate e fica (o corpo cai no apoio), o quadril gira para a perna que
    vai, os ombros ao contrário, os braços balançam pesados e atrasados."""
    p = blank()
    w = 2 * math.pi * t / T
    s, c = math.sin(w), math.cos(w)
    stance(p, .6)
    add(p, "Thigh_R", [(X, 30 * s)])
    add(p, "Thigh_L", [(X, -30 * s)])
    # o joelho dobra na volta (a perna sobe por baixo) e estica antes de pousar
    add(p, "Shin_R", [(X, -46 * max(0.0, math.sin(w + 1.15)) ** 1.4)])
    add(p, "Shin_L", [(X, -46 * max(0.0, math.sin(w + 1.15 + math.pi)) ** 1.4)])
    # o pé: a ponta sobe antes de pousar e empurra no fim do apoio
    add(p, "Foot_R", [(X, 16 * max(0.0, math.sin(w - .6)) - 12 * max(0.0, -math.sin(w - .3)))])
    add(p, "Foot_L", [(X, 16 * max(0.0, -math.sin(w - .6)) - 12 * max(0.0, math.sin(w - .3)))])
    add(p, "Hips", [(Z, 8 * s), (Y, 5 * c), (X, -2)])
    add(p, "Chest", [(Z, -12 * s), (Y, -4 * c), (X, -6 + 1.5 * math.cos(2 * w))])
    add(p, "Shoulder_R", [(Y, 3 * c)])
    add(p, "Shoulder_L", [(Y, 3 * c)])
    add(p, "UpperArm_R", [(X, -18 * math.sin(w - .35) + 8), (Y, -9)])
    add(p, "UpperArm_L", [(X, 18 * math.sin(w - .35) + 8), (Y, 9)])
    add(p, "Forearm_R", [(X, 20 - 9 * math.sin(w - .7))])
    add(p, "Forearm_L", [(X, 20 + 9 * math.sin(w - .7))])
    add(p, "Head", [(X, -4 + 2.5 * math.cos(2 * w + .5)), (Z, 4 * s)])
    # o corpo cai no apoio (duas vezes por ciclo) e balança para a perna de apoio
    drop = math.cos(2 * w)
    move(p, (.12 * s, 0, -.1 + .09 * drop))
    core(p, 1.04)
    shards(p, t, T)
    return p


def awaken(t, T=3.0):
    """Desperta (e a troca de fase): ajoelhado com as mãos no chão, empurra o chão, levanta em dois tempos e ruge."""
    p = blank()
    up = key([(0, 0.0), (.5, .1), (1.3, .65), (1.75, 1.0)], t)
    roar = key([(1.75, 0.0), (2.05, 1.0), (2.6, 1.0), (3.0, .0)], t)
    kneel = 1 - up
    add(p, "Thigh_R", [(X, 30 * kneel)])
    add(p, "Thigh_L", [(X, 85 * kneel)])
    add(p, "Shin_R", [(X, -95 * kneel)])
    add(p, "Shin_L", [(X, -100 * kneel)])
    add(p, "Foot_R", [(X, 50 * kneel)])
    add(p, "Chest", [(X, -45 * kneel + 22 * roar), (Z, 6 * kneel)])
    add(p, "Head", [(X, -20 * kneel + 30 * roar)])
    # as mãos apoiadas no chão, depois o rugido com os braços abertos
    add(p, "UpperArm_R", [(X, 40 * kneel + 30 * roar), (Y, -10 - 65 * roar)])
    add(p, "UpperArm_L", [(X, 40 * kneel + 30 * roar), (Y, 10 + 65 * roar)])
    add(p, "Forearm_R", [(X, 10 + 35 * roar)])
    add(p, "Forearm_L", [(X, 10 + 35 * roar)])
    add(p, "Hand_R", [(X, -30 * kneel)])
    add(p, "Hand_L", [(X, -30 * kneel)])
    move(p, (0, .25 * kneel, -1.15 * kneel + .12 * roar))
    stance(p, up)
    core(p, .5 + .5 * up + .7 * roar)
    shards(p, t, T, fall=1.4 * kneel)
    return p


def hit(t, T=.75):
    """Leva o golpe: o tronco recua, a cabeça vira, um pé escorrega para trás e ele se firma de novo."""
    p = blank()
    k = key([(0, 0.0), (.09, 1.0), (.3, .55), (.75, 0.0)], t)
    stance(p)
    guard(p)
    add(p, "Chest", [(X, 14 * k), (Z, 8 * k), (Y, 4 * k)])
    add(p, "Head", [(X, 10 * k), (Z, -12 * k)])
    add(p, "UpperArm_R", [(X, -14 * k), (Y, -12 * k)])
    add(p, "UpperArm_L", [(X, -8 * k), (Y, 16 * k)])
    add(p, "Thigh_L", [(X, -10 * k)])
    add(p, "Shin_L", [(X, -10 * k)])
    move(p, (0, -.22 * k, -.06 * k))
    core(p, 1 + .7 * k)
    shards(p, t, T)
    return p


def stun(t, T=2.4):
    """Atordoado (laço): o corpo cede para a frente, um joelho quase no chão, a cabeça pende e ele cambaleia em
    círculos, tentando se firmar. O núcleo pisca fraco."""
    p = blank()
    w = 2 * math.pi * t / T
    add(p, "Thigh_R", [(X, 18)])
    add(p, "Thigh_L", [(X, 48)])
    add(p, "Shin_R", [(X, -30)])
    add(p, "Shin_L", [(X, -70)])
    add(p, "Chest", [(X, -26 + 4 * math.sin(w)), (Y, 6 * math.sin(w + .5)), (Z, 7 * math.cos(w))])
    add(p, "Head", [(X, -24 + 6 * math.sin(w + 1)), (Z, 10 * math.sin(w))])
    add(p, "UpperArm_R", [(X, -6 + 6 * math.sin(w + 1.5)), (Y, -5)])
    add(p, "UpperArm_L", [(X, 22 + 4 * math.sin(w)), (Y, 18)])
    add(p, "Forearm_L", [(X, 35)])
    add(p, "Hand_L", [(X, -20)])
    move(p, (.12 * math.sin(w), .1 * math.cos(w), -.7))
    core(p, .55 + .25 * max(0.0, math.sin(w * 3)))
    shards(p, t, T, fall=.9)
    return p


# ---- golpes físicos --------------------------------------------------------------------------------

def overhead(p, t, side, t0):
    """Um punho erguido acima da cabeça que desce de cima no chão à frente (o soco de um gigante). Começa em t0;
    impacto em t0 + .78. Devolve o quanto bateu (para o núcleo)."""
    s = 1 if side == "R" else -1
    lt = t - t0
    wind = key([(0, 0.0), (.6, 1.0), (.7, 1.0), (.8, 0.0)], lt)
    hitk = key([(.66, 0.0), (.8, 1.0), (1.05, .9), (1.45, 0.0)], lt)
    add(p, "Hips", [(Z, -s * 8 * wind + s * 7 * hitk), (X, -10 * hitk)])
    add(p, "Chest", [(Z, -s * 22 * wind + s * 16 * hitk), (X, 12 * wind - 38 * hitk), (Y, -s * 6 * wind)])
    add(p, "Head", [(X, -8 * wind + 16 * hitk)])
    add(p, "UpperArm_" + side, [(X, 160 * wind + 86 * hitk), (Y, -s * (14 * wind + 6))])
    add(p, "Forearm_" + side, [(X, 60 * wind + 4 * hitk)])
    add(p, "Hand_" + side, [(X, 20 * wind - 10 * hitk)])
    other = "L" if side == "R" else "R"
    add(p, "UpperArm_" + other, [(X, -18 * wind + 26 * hitk), (Y, s * (18 * wind + 10))])
    add(p, "Forearm_" + other, [(X, 26 + 18 * wind)])
    # a perna do lado contrário avança e dobra para segurar a pancada; a outra estica atrás
    add(p, "Thigh_" + other, [(X, 30 * hitk + 8 * wind)])
    add(p, "Shin_" + other, [(X, -40 * hitk)])
    add(p, "Thigh_" + side, [(X, -14 * hitk)])
    move(p, (s * .08 * wind, .42 * hitk - .05 * wind, -.5 * hitk + .08 * wind))
    return hitk


def punch(t, T=1.6):
    p = blank()
    stance(p)
    k = overhead(p, t, "R", 0.0)
    core(p, 1 + .5 * k)
    shards(p, t, T)
    return p


def hook(t, T=1.9):
    """Punho Devastador: soco horizontal. Gira o tronco para trás com o braço direito aberto, avança o pé esquerdo e
    varre o punho na altura do peito do herói, da direita para a esquerda, continuando o giro depois do impacto."""
    p = blank()
    stance(p)
    wind = key([(0, 0.0), (.7, 1.0), (.82, 1.0), (.95, 0.0)], t)
    swing = key([(.8, 0.0), (1.0, 1.0), (1.2, 1.15), (1.9, 0.0)], t)
    step = key([(.35, 0.0), (.85, 1.0), (1.5, 1.0), (1.9, 0.0)], t)
    add(p, "Hips", [(Z, -14 * wind + 22 * swing)])
    add(p, "Chest", [(Z, -30 * wind + 44 * swing), (X, -6 * swing + 4 * wind), (Y, 8 * wind - 6 * swing)])
    add(p, "Head", [(Z, 12 * wind - 14 * swing), (X, -6 * swing)])
    # o braço: aberto para o lado e para trás, depois cruza na frente quase reto (o alcance é o braço inteiro)
    add(p, "UpperArm_R", [(X, 25 * wind + 70 * swing), (Y, -70 * wind - 15 * swing + 6), (Z, 30 * swing)])
    add(p, "Forearm_R", [(X, 40 * wind + 12 * swing)])
    add(p, "Hand_R", [(Y, 8 * swing)])
    add(p, "UpperArm_L", [(X, 22 * wind - 10 * swing), (Y, 20 * wind + 26 * swing)])
    add(p, "Forearm_L", [(X, 40)])
    add(p, "Thigh_L", [(X, 26 * step)])
    add(p, "Shin_L", [(X, -26 * step)])
    add(p, "Thigh_R", [(X, -10 * step)])
    move(p, (-.12 * swing, .3 * step, -.18 * step))
    core(p, 1 + .45 * swing)
    shards(p, t, T)
    return p


def sweep(t, T=2.4):
    """Varredura de Pedra: agacha, leva o braço direito estendido e baixo para trás e varre um arco enorme rente ao
    chão (meio círculo à frente), o corpo todo girando sobre a perna esquerda."""
    p = blank()
    stance(p)
    crouch = key([(0, 0.0), (.8, 1.0), (1.6, 1.0), (2.4, 0.0)], t)
    wind = key([(0, 0.0), (.8, 1.0), (.9, 1.0), (1.0, 0.0)], t)
    arc = key([(.92, 0.0), (1.38, 1.0), (1.6, 1.08), (2.4, 0.0)], t)
    add(p, "Hips", [(Z, -24 * wind + 40 * arc), (X, -8 * crouch)])
    add(p, "Chest", [(Z, -36 * wind + 58 * arc), (X, -22 * crouch), (Y, 10 * wind - 8 * arc)])
    add(p, "Head", [(Z, 18 * wind - 22 * arc), (X, 14 * crouch)])
    add(p, "UpperArm_R", [(X, 35 * crouch + 25 * arc), (Y, -75 * wind - 40 * arc * (1 - arc) * 4 - 20), (Z, 45 * arc)])
    add(p, "Forearm_R", [(X, 6)])
    add(p, "UpperArm_L", [(X, 20 * crouch), (Y, 30 * crouch)])
    add(p, "Forearm_L", [(X, 45)])
    add(p, "Thigh_R", [(X, 30 * crouch), (Y, -14 * crouch)])
    add(p, "Thigh_L", [(X, 40 * crouch), (Y, 10 * crouch)])
    add(p, "Shin_R", [(X, -45 * crouch)])
    add(p, "Shin_L", [(X, -55 * crouch)])
    move(p, (0, .1 * arc, -.7 * crouch))
    core(p, 1 + .4 * arc)
    shards(p, t, T, fall=.3 * crouch)
    return p


def stomp(t, T=1.8):
    """Pisada Sísmica: o peso vai para a esquerda, a perna direita sobe alta com os braços abrindo para equilibrar,
    segura no alto e desce com tudo; o corpo afunda no impacto e volta."""
    p = blank()
    stance(p)
    shift = key([(0, 0.0), (.4, 1.0), (1.3, 1.0), (1.8, 0.0)], t)
    lift = key([(.25, 0.0), (.82, 1.0), (.95, 1.0), (1.05, 0.0)], t)
    slam = key([(1.0, 0.0), (1.07, 1.0), (1.3, .8), (1.8, 0.0)], t)
    add(p, "Hips", [(Y, 10 * shift - 4 * slam), (X, -6 * slam)])
    add(p, "Chest", [(Y, -12 * shift), (X, 8 * lift - 18 * slam)])
    add(p, "Head", [(X, -10 * lift + 12 * slam)])
    add(p, "Thigh_R", [(X, 78 * lift), (Y, -8 * lift)])
    add(p, "Shin_R", [(X, -80 * lift + 10 * slam)])
    add(p, "Foot_R", [(X, 20 * lift)])
    add(p, "Thigh_L", [(X, 12 * slam)])
    add(p, "Shin_L", [(X, -6 * lift - 26 * slam)])
    add(p, "UpperArm_R", [(Y, -45 * lift - 10 + 20 * slam), (X, 14 * lift)])
    add(p, "UpperArm_L", [(Y, 50 * lift + 10 - 20 * slam), (X, 14 * lift)])
    add(p, "Forearm_R", [(X, 30 * lift)])
    add(p, "Forearm_L", [(X, 30 * lift)])
    move(p, (-.22 * shift, 0, .1 * lift - .35 * slam))
    core(p, 1 + .6 * slam)
    shards(p, t, T, fall=-.2 * lift + .3 * slam)
    return p


def seismic(t, T=2.4):
    """Martelo Colossal: junta os punhos, ergue os dois acima da cabeça arqueando o corpo para trás, segura o tempo de
    ler, e despenca dobrando os joelhos."""
    p = blank()
    stance(p)
    raise_ = key([(0, 0.0), (1.1, 1.0), (1.26, 1.0), (1.42, 0.0)], t)
    smash = key([(1.3, 0.0), (1.46, 1.0), (1.85, .9), (2.4, 0.0)], t)
    add(p, "Hips", [(X, 6 * raise_ - 12 * smash)])
    add(p, "Chest", [(X, 16 * raise_ - 48 * smash)])
    add(p, "Head", [(X, 20 * raise_ - 14 * smash)])
    for side, s in (("R", -1), ("L", 1)):
        add(p, "UpperArm_" + side, [(X, 172 * raise_ + 50 * smash), (Y, s * (10 - 14 * raise_))])
        add(p, "Forearm_" + side, [(X, 28 * raise_ + 12 * smash)])
        add(p, "Hand_" + side, [(Y, -s * 18 * raise_)])
    for side in ("R", "L"):
        add(p, "Thigh_" + side, [(X, -6 * raise_ + 38 * smash)])
        add(p, "Shin_" + side, [(X, 8 * raise_ - 58 * smash)])
    move(p, (0, -.12 * raise_ + .3 * smash, .22 * raise_ - .62 * smash))
    core(p, 1 + .6 * raise_ + .4 * smash)
    shards(p, t, T, fall=-.6 * raise_ + .4 * smash)
    return p


def combo(t, T=3.6):
    """Combo Sísmico: punho direito de cima, punho esquerdo de cima e os dois juntos (impactos em .78, 1.68 e 2.95)."""
    p = blank()
    stance(p)
    a = overhead(p, t, "R", 0.0) if t < 1.45 else 0.0
    b = overhead(p, t, "L", .9) if .9 <= t < 2.35 else 0.0
    if t >= 1.9:
        lt = t - 1.9
        up = key([(0, 0.0), (.75, 1.0), (.88, 1.0), (1.0, 0.0)], lt)
        down = key([(.92, 0.0), (1.05, 1.0), (1.35, .9), (1.7, 0.0)], lt)
        add(p, "Chest", [(X, 14 * up - 46 * down)])
        add(p, "Head", [(X, 16 * up - 14 * down)])
        for side, s in (("R", -1), ("L", 1)):
            add(p, "UpperArm_" + side, [(X, 165 * up + 52 * down), (Y, s * (8 - 12 * up))])
            add(p, "Forearm_" + side, [(X, 26 * up + 10 * down)])
        for side in ("R", "L"):
            add(p, "Thigh_" + side, [(X, 36 * down)])
            add(p, "Shin_" + side, [(X, -56 * down)])
        move(p, (0, .3 * down, .18 * up - .6 * down))
        core(p, 1 + .5 * up + .5 * down)
    else:
        core(p, 1 + .5 * max(a, b))
    shards(p, t, T)
    return p


def throw(t, T=2.1):
    """Meteoro Vulcânico: agacha e arranca uma rocha do chão com as duas mãos, ergue acima da cabeça, recua o tronco e
    arremessa com o corpo todo, dando um passo à frente."""
    p = blank()
    stance(p)
    down = key([(0, 0.0), (.6, 1.0), (.78, 1.0), (1.05, 0.0)], t)
    up = key([(.75, 0.0), (1.15, 1.0), (1.32, 1.0), (1.45, 0.0)], t)
    fling = key([(1.38, 0.0), (1.52, 1.0), (1.7, 1.0), (2.1, 0.0)], t)
    add(p, "Chest", [(X, -50 * down + 22 * up - 30 * fling)])
    add(p, "Hips", [(X, -14 * down + 4 * up - 6 * fling)])
    add(p, "Head", [(X, -10 * down + 18 * up - 8 * fling)])
    for side, s in (("R", -1), ("L", 1)):
        add(p, "UpperArm_" + side, [(X, 45 * down + 175 * up + 70 * fling), (Y, s * 14)])
        add(p, "Forearm_" + side, [(X, 30 * down + 30 * up - 10 * fling)])
    for side in ("R", "L"):
        add(p, "Thigh_" + side, [(X, 34 * down)])
        add(p, "Shin_" + side, [(X, -50 * down)])
    add(p, "Thigh_L", [(X, 24 * fling)])
    add(p, "Shin_L", [(X, -20 * fling)])
    add(p, "Thigh_R", [(X, -12 * fling)])
    move(p, (0, -.1 * up + .35 * fling, -.5 * down + .08 * up - .15 * fling))
    core(p, 1 + .5 * up)
    shards(p, t, T)
    return p


def charge(t, T=2.0):
    """Investida: abaixa o tronco, raspa o pé, dispara em passadas largas com os ombros à frente e freia."""
    p = blank()
    lean = key([(0, 0.0), (.45, 1.0), (1.4, 1.0), (2.0, 0.0)], t)
    paw = key([(.15, 0.0), (.3, 1.0), (.45, 0.0)], t)
    run = key([(.45, 0.0), (.55, 1.0), (1.25, 1.0), (1.4, 0.0)], t)
    brake = key([(1.3, 0.0), (1.45, 1.0), (2.0, 0.0)], t)
    w = 2 * math.pi * t / .55
    add(p, "Chest", [(X, -40 * lean + 14 * brake)])
    add(p, "Hips", [(X, -12 * lean)])
    add(p, "Head", [(X, 24 * lean)])
    add(p, "Thigh_R", [(X, 40 * math.sin(w) * run + 12 * lean - 14 * paw)])
    add(p, "Thigh_L", [(X, -40 * math.sin(w) * run + 12 * lean + 20 * brake)])
    add(p, "Shin_R", [(X, -55 * max(0.0, math.sin(w + 1.2)) * run - 20 * paw)])
    add(p, "Shin_L", [(X, -55 * max(0.0, math.sin(w + 1.2 + math.pi)) * run - 30 * brake)])
    add(p, "UpperArm_R", [(X, -50 * lean + 25 * math.sin(w) * run), (Y, -22)])
    add(p, "UpperArm_L", [(X, -50 * lean - 25 * math.sin(w) * run), (Y, 22)])
    add(p, "Forearm_R", [(X, 30)])
    add(p, "Forearm_L", [(X, 30)])
    move(p, (0, -.2 * brake, -.28 * lean - .12 * abs(math.sin(w)) * run))
    core(p, 1 + .3 * lean)
    shards(p, t, T)
    return p


# ---- habilidades do núcleo -------------------------------------------------------------------------

def core_burst(t, T=2.8):
    """Coração Incandescente: se fecha (braços cruzados na frente do núcleo, curvado, tremendo) enquanto o núcleo
    carrega, e abre de uma vez: peito para a frente, braços para trás, cabeça erguida (a explosão em 1.75)."""
    p = blank()
    stance(p)
    close = key([(0, 0.0), (.6, 1.0), (1.6, 1.0), (1.72, 0.0)], t)
    open_ = key([(1.65, 0.0), (1.78, 1.0), (2.2, .8), (2.8, 0.0)], t)
    tremble = math.sin(t * 38) * close * key([(.6, 0.0), (1.6, 1.0)], t)
    add(p, "Chest", [(X, -24 * close + 20 * open_ + 1.5 * tremble)])
    add(p, "Head", [(X, -14 * close + 24 * open_)])
    for side, s in (("R", -1), ("L", 1)):
        add(p, "UpperArm_" + side, [(X, 70 * close - 28 * open_), (Y, -s * 36 * close + s * 70 * open_), (Z, s * 30 * close)])
        add(p, "Forearm_" + side, [(X, 90 * close + 10 * open_)])
    for side in ("R", "L"):
        add(p, "Thigh_" + side, [(X, 18 * close)])
        add(p, "Shin_" + side, [(X, -30 * close)])
    move(p, (.02 * tremble, -.15 * open_, -.3 * close + .1 * open_))
    core(p, 1 + 1.1 * key([(.3, 0.0), (1.65, 1.0), (1.8, 1.6), (2.4, 0.0)], t))
    shards(p, t, T, fall=-.5 * close, spread=1.6 * open_)
    return p


def beam(t, T=4.2):
    """Raio do Núcleo: firma as pernas abertas, joga os ombros para trás e empurra o peito (o feixe sai do núcleo de
    1.0 a 3.4), virando o tronco devagar da direita para a esquerda; depois cede."""
    p = blank()
    brace = key([(0, 0.0), (.8, 1.0), (3.5, 1.0), (4.2, 0.0)], t)
    turn = key([(1.0, -1.0), (3.4, 1.0)], t) if 1.0 <= t <= 3.4 else (-1.0 if t < 1.0 else 1.0)
    shake = math.sin(t * 30) * key([(1.0, 0.0), (1.2, 1.0), (3.3, 1.0), (3.5, 0.0)], t)
    for side, s in (("R", -1), ("L", 1)):
        add(p, "Thigh_" + side, [(X, 10 * brace), (Y, s * 14 * brace)])
        add(p, "Shin_" + side, [(X, -22 * brace)])
        add(p, "UpperArm_" + side, [(X, -30 * brace), (Y, s * 42 * brace)])
        add(p, "Forearm_" + side, [(X, 50 * brace)])
        add(p, "Hand_" + side, [(X, -20 * brace)])
    add(p, "Hips", [(Z, 14 * turn * brace)])
    add(p, "Chest", [(X, 14 * brace + .8 * shake), (Z, 24 * turn * brace)])
    add(p, "Head", [(X, -6 * brace), (Z, 6 * turn * brace)])
    move(p, (0, -.12 * brace, -.35 * brace))
    core(p, 1 + 1.3 * brace)
    shards(p, t, T, spread=.6 * brace)
    return p


def shed(t, T=2.4):
    """Armadura Derretida: encolhe e treme (a pedra rachando), depois arqueia e solta os braços para fora de uma vez
    (os fragmentos voam em 1.35)."""
    p = blank()
    stance(p)
    curl = key([(0, 0.0), (.5, 1.0), (1.25, 1.0), (1.38, 0.0)], t)
    burst = key([(1.3, 0.0), (1.42, 1.0), (1.8, .7), (2.4, 0.0)], t)
    tremble = math.sin(t * 42) * curl
    add(p, "Chest", [(X, -30 * curl + 16 * burst + 2 * tremble), (Y, 1.5 * tremble)])
    add(p, "Head", [(X, -22 * curl + 26 * burst)])
    for side, s in (("R", -1), ("L", 1)):
        add(p, "UpperArm_" + side, [(X, 40 * curl - 10 * burst), (Y, -s * 10 * curl + s * 85 * burst)])
        add(p, "Forearm_" + side, [(X, 95 * curl + 15 * burst)])
        add(p, "Hand_" + side, [(X, 25 * curl)])
    for side in ("R", "L"):
        add(p, "Thigh_" + side, [(X, 22 * curl)])
        add(p, "Shin_" + side, [(X, -36 * curl)])
    move(p, (.03 * tremble, 0, -.38 * curl + .12 * burst))
    core(p, 1 + .5 * curl + .8 * burst)
    shards(p, t, T, fall=-.4 * curl, spread=3.0 * burst)
    return p


def eruption(t, T=2.6):
    """Erupção Vulcânica: ergue os braços abertos chamando a lava (as colunas sobem de 1.0 a 2.0), bate um pé e ruge."""
    p = blank()
    stance(p)
    call = key([(0, 0.0), (.8, 1.0), (2.0, 1.0), (2.6, 0.0)], t)
    stamp = key([(.85, 0.0), (1.0, 1.0), (1.3, 0.0)], t)
    pulse = math.sin(t * 6) * call
    add(p, "Chest", [(X, 18 * call + 2 * pulse)])
    add(p, "Head", [(X, 28 * call)])
    for side, s in (("R", -1), ("L", 1)):
        add(p, "UpperArm_" + side, [(X, 40 * call), (Y, s * (80 * call + 4 * pulse))])
        add(p, "Forearm_" + side, [(X, 50 * call)])
        add(p, "Hand_" + side, [(X, -30 * call)])
    add(p, "Thigh_R", [(X, 40 * key([(.4, 0.0), (.8, 1.0), (.95, 0.0)], t))])
    add(p, "Shin_R", [(X, -40 * key([(.4, 0.0), (.8, 1.0), (.95, 0.0)], t))])
    move(p, (0, 0, .1 * call - .2 * stamp))
    core(p, 1 + .9 * call)
    shards(p, t, T, fall=-.8 * call, spread=.8 * call)
    return p


def cataclysm(t, T=3.0):
    """Cataclismo (fase 1 e 2): ergue os braços abertos com o núcleo crescendo e despenca os dois punhos (em 1.9)."""
    p = blank()
    stance(p)
    rise = key([(0, 0.0), (1.4, 1.0), (1.65, 1.0), (1.88, 0.0)], t)
    slam = key([(1.75, 0.0), (1.9, 1.0), (2.35, .9), (3.0, 0.0)], t)
    add(p, "Chest", [(X, 22 * rise - 54 * slam)])
    add(p, "Head", [(X, 32 * rise - 20 * slam)])
    for side, s in (("R", -1), ("L", 1)):
        add(p, "UpperArm_" + side, [(X, 40 * rise + 46 * slam), (Y, s * (115 * rise + 18 * slam + 6))])
        add(p, "Forearm_" + side, [(X, 25 * rise + 15 * slam)])
    for side in ("R", "L"):
        add(p, "Thigh_" + side, [(X, 42 * slam)])
        add(p, "Shin_" + side, [(X, -66 * slam)])
    move(p, (0, .2 * slam, .4 * rise - .72 * slam))
    core(p, 1 + 1.1 * rise + .3 * slam)
    shards(p, t, T, fall=-1.2 * rise)
    return p


def ultimate(t, T=5.6):
    """Despertar do Núcleo: cai sobre um joelho com as mãos no núcleo e carrega (o chão treme, 0.6 a 3.4), levanta
    abrindo os braços num rugido (3.4 a 4.0) e despenca os dois punhos no chão (impacto em 4.45)."""
    p = blank()
    kneel = key([(0, 0.0), (.6, 1.0), (3.3, 1.0), (3.75, 0.0)], t)
    roar = key([(3.4, 0.0), (3.85, 1.0), (4.1, 1.0), (4.32, 0.0)], t)
    slam = key([(4.3, 0.0), (4.45, 1.0), (4.9, .9), (5.6, 0.0)], t)
    charge_ = key([(.6, 0.0), (3.3, 1.0)], t)
    tremble = math.sin(t * 34) * kneel * charge_
    add(p, "Thigh_R", [(X, 30 * kneel + 40 * slam)])
    add(p, "Thigh_L", [(X, 82 * kneel + 40 * slam)])
    add(p, "Shin_R", [(X, -90 * kneel - 64 * slam)])
    add(p, "Shin_L", [(X, -96 * kneel - 64 * slam)])
    add(p, "Foot_R", [(X, 45 * kneel)])
    add(p, "Chest", [(X, -30 * kneel + 26 * roar - 54 * slam + 2 * tremble)])
    add(p, "Head", [(X, -24 * kneel + 34 * roar - 18 * slam)])
    for side, s in (("R", -1), ("L", 1)):
        add(p, "UpperArm_" + side, [(X, 75 * kneel + 40 * roar + 46 * slam), (Y, -s * 30 * kneel + s * (110 * roar + 18 * slam + 6)), (Z, s * 25 * kneel)])
        add(p, "Forearm_" + side, [(X, 95 * kneel + 25 * roar + 15 * slam)])
    move(p, (.03 * tremble, .2 * slam, -1.05 * kneel + .35 * roar - .72 * slam))
    core(p, 1 + 1.6 * charge_ * kneel + 1.4 * roar + .3 * slam)
    shards(p, t, T, fall=-1.4 * charge_ * kneel, spread=1.2 * roar)
    return p


def death(t, T=3.6):
    """Morte: o núcleo apaga aos poucos, ele cambaleia para trás, cai de joelhos, apoia uma mão, tomba para a frente e
    as pedras em volta caem."""
    p = blank()
    stagger = key([(0, 0.0), (.6, 1.0), (1.0, 1.0), (1.3, 0.0)], t)
    kneel = key([(.9, 0.0), (1.7, 1.0)], t)
    lean = key([(1.6, 0.0), (2.2, 1.0)], t)
    fall = key([(2.4, 0.0), (3.1, 1.0)], t)
    add(p, "Chest", [(X, 20 * stagger - 22 * kneel - 20 * lean), (Z, 10 * stagger)])
    add(p, "Head", [(X, 25 * stagger - 30 * kneel), (Z, -14 * stagger)])
    add(p, "Thigh_R", [(X, 88 * kneel)])
    add(p, "Thigh_L", [(X, 80 * kneel)])
    add(p, "Shin_R", [(X, -108 * kneel)])
    add(p, "Shin_L", [(X, -100 * kneel)])
    add(p, "Hips", [(X, -70 * fall), (Z, 6 * lean)])
    add(p, "UpperArm_R", [(X, 20 * kneel + 55 * lean + 40 * fall), (Y, -22 * stagger - 6)])
    add(p, "UpperArm_L", [(X, 30 * kneel + 40 * fall), (Y, 24 * stagger + 6)])
    add(p, "Forearm_R", [(X, 10 * lean)])
    move(p, (0, -.35 * stagger + .4 * lean + 1.0 * fall, -1.25 * kneel - .9 * fall))
    core(p, max(.05, 1.4 - 1.35 * key([(0, 0.0), (3.2, 1.0)], t)))
    shards(p, t, T, fall=4.5 * key([(.4, 0.0), (1.5, 1.0)], t))
    return p


# nome do clipe -> (função, segundos, em laço)
CLIPS = [
    ("Idle", idle, 3.2, True), ("Walk", walk, 1.9, True), ("Awaken", awaken, 3.0, False), ("Hit", hit, .75, False),
    ("Stun", stun, 2.4, True), ("Punch", punch, 1.6, False), ("Hook", hook, 1.9, False), ("Sweep", sweep, 2.4, False),
    ("Stomp", stomp, 1.8, False), ("Seismic", seismic, 2.4, False), ("Combo", combo, 3.6, False), ("Throw", throw, 2.1, False),
    ("Charge", charge, 2.0, False), ("CoreBurst", core_burst, 2.8, False), ("Beam", beam, 4.2, False), ("Shed", shed, 2.4, False),
    ("Eruption", eruption, 2.6, False), ("Cataclysm", cataclysm, 3.0, False), ("Ultimate", ultimate, 5.6, False),
    ("Death", death, 3.6, False),
]
