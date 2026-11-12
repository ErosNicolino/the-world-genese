# As poses de cada família de arma. Executado dentro de BuildWeaponClips.py (usa family, k, HITS).
# Espaço das mãos: (direita, frente, cima) em metros a partir da base do tronco; ombros em (±.25, 0, .18).
# "tip"/"edge"/"side" no espaço do corpo; "pole" (cotovelo) no espaço do tronco.


# ---- ajudantes ---------------------------------------------------------------------------------------
def H(at, tip, edge=None, side=None, pole=None):
    """Uma mão: onde a pegada fica, para onde a ponta vai e para onde vai o fio (o lado largo da arma)."""
    h = {"at": at, "tip": tip}
    if edge is not None:
        h["edge"] = edge
    if side is not None:
        h["side"] = side
    if pole is not None:
        h["pole"] = pole
    return h


LEFT_EASY = H((-.21, .10, -.03), (0, .4, -1), side=(-1, 0, 0), pole=(-1, -.3, -.4))     # mão esquerda solta, meio dobrada


def P(base, **over):
    out = dict(base)
    out.update(over)
    return out


def strike(hold, wind, hit, follow, t_wind, t_hit, t_end, name=None, hit_at=None):
    """Golpe: preparo, acerto (rápido, ease-out), passa um pouco e volta à pose de segurar."""
    keys = [k(0, **hold), k(t_wind, **wind), k(t_hit, ease="out", **hit), k(t_hit + (t_end - t_hit) * .35, **follow), k(t_end, **hold)]
    if name:
        HITS[name] = hit_at if hit_at is not None else t_hit
    return keys


def breathe(hold, period=2.4, lift=3):
    """Pose de segurar (o andar e o correr vêm de baixo): respira de leve."""
    t = hold.get("torso", (0, 0, 0))
    up = P(hold, torso=(t[0], t[1] - lift * .5, t[2]))
    return [k(0, **hold), k(period / 2, **up), k(period, **hold)]


# ---- instrumento (alaúde) ------------------------------------------------------------------------------
# Atravessado na frente da barriga: a mão direita no corpo do instrumento, a esquerda no braço (que sobe
# para a esquerda). Golpe: um acorde forte (o instrumento sobe e aponta para o alvo, o tronco acompanha).
LUTE = dict(torso=(8, 2, 0), head=(-6, 4, 0),
            r=H((.06, .19, .00), (-1, .25, .55), side=(0, 1, .1), pole=(1, -.2, -.8)),
            l={"on": .30, "pole": (-.8, -.2, -.8)})


@family("Lute", "instrumento")
def lute():
    strum_up = P(LUTE, torso=(14, -4, 0), r=H((.08, .2, .05), (-1, .45, .65), side=(0, 1, .2), pole=(1, -.2, -.8)))
    strum_down = P(LUTE, torso=(-6, 6, 0), r=H((.04, .23, -.02), (-1, .5, .35), side=(0, 1, -.1), pole=(1, -.2, -.8)))
    song = [k(0, **LUTE)]
    t = 0
    for i in range(4):
        song += [k(t + .3, **P(LUTE, torso=(12, -6, 4), head=(-10, -6, 6))), k(t + .6, **P(LUTE, torso=(0, 6, -4), head=(-4, 8, -6)))]
        t += .6
    return {
        "Vx_Lute_Hold": (breathe(LUTE), True),
        "Vx_Lute_Attack_01": (strike(LUTE, strum_up, strum_down, LUTE, .14, .24, .5, "Vx_Lute_Attack_01"), False),
        "Vx_Lute_Attack_02": (strike(LUTE, strum_down, strum_up, LUTE, .14, .24, .5, "Vx_Lute_Attack_02"), False),
        "Vx_Lute_Attack_03": (strike(LUTE, P(strum_up, torso=(20, -10, 0)), P(strum_down, torso=(-10, 12, 0)), LUTE, .25, .4, .85, "Vx_Lute_Attack_03"), False),
        "Vx_Lute_Special": (song, True),
    }


# ---- orbe ----------------------------------------------------------------------------------------------
# A esfera na palma da mão direita, na frente do peito; a esquerda aberta perto, como quem canaliza.
ORB = dict(torso=(4, 2, 0),
           r=H((.17, .21, .06), (0, .25, 1), side=(1, 0, 0), pole=(1, -.4, -.6)),
           l=H((-.12, .22, .02), (.3, .6, .5), side=(0, 0, 1), pole=(-1, -.3, -.5)))


@family("Orb", "orbe")
def orb():
    pull = P(ORB, torso=(16, -4, 0), r=H((.22, .08, .12), (0, .1, 1), side=(1, 0, 0), pole=(1, -.4, -.6)))
    push = P(ORB, torso=(-8, 8, 0), r=H((.10, .31, .12), (0, 1, .35), side=(1, 0, 0), pole=(1, -.2, -.8)),
             l=H((-.10, .27, .06), (.3, 1, .2), side=(0, 0, 1), pole=(-1, -.3, -.5)))
    raise_ = P(ORB, torso=(0, -8, 0), head=(0, -12, 0), r=H((.16, .10, .44), (0, 0, 1), side=(1, 0, 0), pole=(1, -.2, -.3)),
               l=H((-.16, .12, .36), (0, .2, 1), side=(0, 1, 0), pole=(-1, -.2, -.3)))
    return {
        "Vx_Orb_Hold": (breathe(ORB), True),
        "Vx_Orb_Attack_01": (strike(ORB, pull, push, P(push, torso=(-4, 6, 0)), .14, .24, .5, "Vx_Orb_Attack_01"), False),
        "Vx_Orb_Attack_02": (strike(ORB, P(pull, torso=(10, -2, 0)), P(push, torso=(-14, 8, 0)), push, .14, .24, .5, "Vx_Orb_Attack_02"), False),
        "Vx_Orb_Attack_03": (strike(ORB, raise_, P(push, torso=(-4, 12, 0)), push, .3, .45, .85, "Vx_Orb_Attack_03"), False),
        "Vx_Orb_Special": ([k(0, **ORB), k(.35, **raise_), k(.8, **P(raise_, torso=(0, -10, 0))), k(1.1, **ORB)], False),
    }


# ---- grimório ------------------------------------------------------------------------------------------
# O livro aberto na mão direita, na altura do peito, virado para o herói; a esquerda aponta o alvo
# quando conjura (o círculo aparece sobre o inimigo). Secundário: a esquerda varre de lado (a parede).
BOOK = dict(torso=(6, 4, 0), head=(-4, 8, 0),
            r=H((.12, .22, .06), (-.35, .5, .35), side=(.2, -.2, 1), pole=(1, -.3, -.6)),
            l=H((-.14, .18, .02), (0, .4, -.6), side=(-1, 0, 0), pole=(-1, -.3, -.4)))


@family("Book", "livro_da_aurora")
def book():
    point = P(BOOK, torso=(-8, 4, 0), head=(4, 0, 0),
              l=H((-.10, .32, .16), (.05, 1, .1), side=(0, 0, 1), pole=(-1, -.2, -.4)))
    gather = P(BOOK, torso=(12, -2, 0), l=H((-.06, .20, .10), (.6, .4, .4), side=(0, 0, 1), pole=(-1, -.2, -.4)))
    sweep_a = P(BOOK, torso=(25, 2, 0), l=H((-.30, .20, .10), (-1, .6, 0), side=(0, 0, 1), pole=(-1, -.2, -.4)))
    sweep_b = P(BOOK, torso=(-25, 6, 0), l=H((.10, .30, .10), (1, .6, 0), side=(0, 0, 1), pole=(-1, -.2, -.4)))
    return {
        "Vx_Book_Hold": (breathe(BOOK), True),
        "Vx_Book_Attack_01": (strike(BOOK, gather, point, point, .14, .26, .55, "Vx_Book_Attack_01"), False),
        "Vx_Book_Attack_02": (strike(BOOK, gather, point, point, .14, .26, .55, "Vx_Book_Attack_02"), False),
        "Vx_Book_Attack_03": (strike(BOOK, P(gather, torso=(18, -6, 0)), P(point, torso=(-12, 8, 0)), point, .25, .42, .85, "Vx_Book_Attack_03"), False),
        "Vx_Book_Special": (strike(BOOK, sweep_a, sweep_b, sweep_b, .3, .55, 1.0, "Vx_Book_Special"), False),
    }


# ---- varinha ------------------------------------------------------------------------------------------
WAND = dict(torso=(-4, 2, 0),
            r=H((.20, .19, .02), (.1, 1, .25), side=(1, 0, 0), pole=(1, -.4, -.5)), l=LEFT_EASY)


@family("Wand", "varinha")
def wand():
    up = P(WAND, torso=(10, -2, 0), r=H((.23, .07, .20), (.1, .1, 1), side=(1, 0, 0), pole=(1, -.3, -.5)))
    flick = P(WAND, torso=(-10, 6, 0), r=H((.12, .32, .12), (0, 1, .02), side=(1, 0, 0), pole=(1, -.2, -.8)))
    charge = P(WAND, torso=(20, -6, 0), head=(-12, 4, 0), r=H((.27, -.04, .26), (0, -.3, 1), side=(1, 0, 0), pole=(1, -.4, -.3)),
               l=H((-.05, .18, .12), (.4, .5, .5), side=(0, 0, 1), pole=(-1, -.3, -.4)))
    blast = P(WAND, torso=(-14, 10, 0), r=H((.08, .33, .14), (0, 1, .05), side=(1, 0, 0), pole=(1, -.2, -.8)),
              l=H((-.02, .26, .08), (.3, 1, 0), side=(0, 0, 1), pole=(-1, -.3, -.4)))
    side_up = P(up, r=H((.27, .12, .14), (1, .3, .5), side=(0, 0, 1), pole=(1, -.3, -.5)))
    HITS["Vx_Wand_Special"] = .1
    return {
        "Vx_Wand_Hold": (breathe(WAND), True),
        "Vx_Wand_Attack_01": (strike(WAND, up, flick, flick, .1, .17, .36, "Vx_Wand_Attack_01"), False),
        "Vx_Wand_Attack_02": (strike(WAND, side_up, flick, flick, .1, .17, .36, "Vx_Wand_Attack_02"), False),
        "Vx_Wand_Attack_03": (strike(WAND, up, P(flick, torso=(-16, 8, 0)), flick, .16, .26, .5, "Vx_Wand_Attack_03"), False),
        "Vx_Wand_Charge": ([k(0, **charge), k(.4, **P(charge, torso=(22, -8, 0))), k(.8, **charge)], True),
        "Vx_Wand_Special": ([k(0, **charge), k(.1, ease="out", **blast), k(.3, **blast), k(.6, **WAND)], False),
    }


# ---- cajado ---------------------------------------------------------------------------------------------
# Em pé ao lado do corpo na mão direita; o tiro de vinha sai da ponta com as duas mãos empurrando o
# cajado para a frente. Secundário (prisão de vinhas): ergue o cajado e crava o pé no chão.
STAFF = dict(torso=(-4, 0, 0),
             r=H((.25, .13, .06), (.05, .15, 1), side=(1, 0, 0), pole=(1, -.3, -.4)), l=LEFT_EASY)


@family("Staff", "cajado_de_ilsa")
def staff():
    aim = P(STAFF, torso=(-14, 6, 0), r=H((.15, .24, .06), (0, 1, .42), side=(1, 0, 0), pole=(1, -.2, -.6)),
            l={"on": .22, "pole": (-1, -.2, -.5)})
    back = P(STAFF, torso=(10, -4, 0), r=H((.20, .12, .10), (0, .4, 1), side=(1, 0, 0), pole=(1, -.2, -.6)),
             l={"on": .22, "pole": (-1, -.2, -.5)})
    lift = P(STAFF, torso=(0, -10, 0), head=(0, -10, 0), r=H((.18, .10, .42), (0, .1, 1), side=(1, 0, 0), pole=(1, -.2, -.3)),
             l={"on": .12, "pole": (-1, -.2, -.3)})
    plant = P(STAFF, torso=(0, 18, 0), hips=(0, 6, 0), legs={"bend": 18, "drop": .05},
              r=H((.12, .26, .10), (0, .25, 1), side=(1, 0, 0), pole=(1, -.2, -.6)), l={"on": .10, "pole": (-1, -.2, -.5)})
    HITS["Vx_Staff_Special_Full"] = 1.2
    return {
        "Vx_Staff_Hold": (breathe(STAFF), True),
        "Vx_Staff_Attack_01": (strike(STAFF, back, aim, aim, .14, .26, .55, "Vx_Staff_Attack_01"), False),
        "Vx_Staff_Attack_02": (strike(STAFF, back, P(aim, torso=(-20, 8, 0)), aim, .14, .26, .55, "Vx_Staff_Attack_02"), False),
        "Vx_Staff_Attack_03": (strike(STAFF, lift, aim, aim, .3, .45, .9, "Vx_Staff_Attack_03"), False),
        "Vx_Staff_Special_Full": ([k(0, **STAFF), k(.35, **lift), k(.8, **P(lift, torso=(0, -14, 0))), k(1.2, ease="out", **plant),
                                   k(1.45, **plant), k(1.8, **STAFF)], False),
    }


# ---- catalisador (garrafa) -------------------------------------------------------------------------------
FLASK = dict(torso=(-4, 0, 0), r=H((.21, .14, -.02), (0, .2, 1), side=(1, 0, 0), pole=(1, -.3, -.5)), l=LEFT_EASY)


@family("Flask", "catalisador")
def flask():
    wind = P(FLASK, torso=(28, -8, 0), head=(-20, 0, 0), r=H((.27, -.16, .30), (0, -.6, 1), side=(1, 0, 0), pole=(1, -.6, -.2)),
             l=H((-.18, .22, .10), (0, 1, .2), side=(0, 0, 1), pole=(-1, -.2, -.4)))
    throw = P(FLASK, torso=(-24, 12, 0), head=(18, -4, 0), r=H((.04, .30, .20), (0, 1, .5), side=(1, 0, 0), pole=(1, -.2, -.6)))
    lob_wind = P(FLASK, torso=(10, -10, 0), r=H((.24, .02, -.08), (0, -.2, -1), side=(1, 0, 0), pole=(1, -.4, -.4)))
    lob = P(FLASK, torso=(-6, -8, 0), head=(0, -14, 0), r=H((.12, .22, .34), (0, .6, 1), side=(1, 0, 0), pole=(1, -.2, -.4)))
    return {
        "Vx_Flask_Hold": (breathe(FLASK), True),
        "Vx_Flask_Attack_01": (strike(FLASK, wind, throw, throw, .2, .32, .62, "Vx_Flask_Attack_01"), False),
        "Vx_Flask_Special": (strike(FLASK, lob_wind, lob, lob, .25, .42, .8, "Vx_Flask_Special"), False),
    }


# ---- maça sagrada ----------------------------------------------------------------------------------------
# Pancadas de verdade: de cima para baixo, de lado e um golpe no chão; sem corte de espada.
MACE = dict(torso=(-6, 2, 0), r=H((.21, .16, .00), (.15, .5, 1), side=(1, 0, 0), pole=(1, -.3, -.5)), l=LEFT_EASY)


@family("Mace", "maca_sagrada")
def mace():
    over_w = P(MACE, torso=(18, -10, 0), r=H((.24, -.02, .36), (.2, -.8, .6), side=(1, 0, 0), pole=(1, -.4, -.2)))
    over_h = P(MACE, torso=(-14, 16, 0), r=H((.10, .30, .02), (0, 1, -.5), side=(1, 0, 0), pole=(1, -.2, -.6)))
    side_w = P(MACE, torso=(34, 0, 0), r=H((.30, .02, .20), (1, -.2, .3), side=(0, 0, 1), pole=(1, -.5, -.2)))
    side_h = P(MACE, torso=(-36, 6, 0), r=H((-.08, .28, .10), (-1, .5, .1), side=(0, 0, 1), pole=(1, -.2, -.6)))
    slam_w = P(MACE, torso=(6, -12, 0), head=(0, -10, 0), r=H((.12, .04, .42), (0, -.4, 1), side=(1, 0, 0), pole=(1, -.2, -.3)),
               l={"on": -.06, "pole": (-1, -.2, -.3)})
    slam_h = P(MACE, torso=(0, 28, 0), hips=(0, 8, 0), legs={"bend": 22, "drop": .07},
               r=H((.06, .30, -.06), (0, .7, -1), side=(1, 0, 0), pole=(1, -.2, -.6)), l={"on": -.06, "pole": (-1, -.2, -.5)})
    HITS["Vx_Mace_Attack_03_Full"] = .5
    HITS["Vx_Mace_Special_Full"] = .62
    return {
        "Vx_Mace_Hold": (breathe(MACE), True),
        "Vx_Mace_Attack_01": (strike(MACE, over_w, over_h, P(over_h, torso=(-10, 18, 0)), .2, .32, .62, "Vx_Mace_Attack_01"), False),
        "Vx_Mace_Attack_02": (strike(MACE, side_w, side_h, side_h, .2, .32, .62, "Vx_Mace_Attack_02"), False),
        "Vx_Mace_Attack_03_Full": ([k(0, **MACE), k(.32, **slam_w), k(.5, ease="out", **slam_h), k(.7, **slam_h), k(1.0, **MACE)], False),
        "Vx_Mace_Special_Full": ([k(0, **MACE), k(.4, **P(slam_w, torso=(0, -16, 0))), k(.62, ease="out", **P(slam_h, torso=(0, 34, 0))),
                                  k(.95, **slam_h), k(1.3, **MACE)], False),
    }


# ---- adagas duplas ---------------------------------------------------------------------------------------
DAGGER = dict(torso=(0, 6, 0),
              r=H((.19, .19, -.01), (.1, 1, .35), edge=(0, 0, 1), pole=(1, -.4, -.5)),
              l=H((-.19, .19, -.01), (-.1, 1, .35), edge=(0, 0, 1), pole=(-1, -.4, -.5)))


@family("Dagger", "adagas_gemeas", "adagas_gemeas_L")
def dagger():
    r_w = P(DAGGER, torso=(22, 2, 0), r=H((.29, .08, .16), (1, .4, .3), edge=(-1, .3, 0), pole=(1, -.4, -.3)))
    r_h = P(DAGGER, torso=(-22, 8, 0), r=H((.0, .29, .04), (-.6, 1, .1), edge=(-1, 0, 0), pole=(1, -.2, -.6)))
    l_w = P(DAGGER, torso=(-22, 2, 0), l=H((-.29, .08, .16), (-1, .4, .3), edge=(1, .3, 0), pole=(-1, -.4, -.3)))
    l_h = P(DAGGER, torso=(22, 8, 0), l=H((.0, .29, .04), (.6, 1, .1), edge=(1, 0, 0), pole=(-1, -.2, -.6)))
    cross_w = P(DAGGER, torso=(0, -4, 0), r=H((.24, .10, .30), (.6, .3, 1), edge=(-1, 0, -1), pole=(1, -.3, -.3)),
                l=H((-.24, .10, .30), (-.6, .3, 1), edge=(1, 0, -1), pole=(-1, -.3, -.3)))
    cross_h = P(DAGGER, torso=(0, 14, 0), r=H((-.10, .30, -.02), (-.8, .8, -.4), edge=(-1, 0, -1), pole=(1, -.2, -.6)),
                l=H((.10, .28, -.04), (.8, .8, -.4), edge=(1, 0, -1), pole=(-1, -.2, -.6)))
    # avanço (secundário): corpo baixo e para a frente, adagas para trás (como correndo), tesourada no fim
    dash_a = P(DAGGER, torso=(0, 24, 0), hips=(0, 10, 0), legs={"bend": 20, "step": -18, "drop": .05},
               r=H((.27, -.10, .02), (.2, -1, .1), edge=(0, 0, 1), pole=(1, .2, -.4)),
               l=H((-.27, -.10, .02), (-.2, -1, .1), edge=(0, 0, 1), pole=(-1, .2, -.4)))
    dash_b = P(dash_a, r=H((.30, .14, .10), (1, .6, .1), edge=(-1, .3, 0), pole=(1, -.3, -.4)),
               l=H((-.30, .14, .10), (-1, .6, .1), edge=(1, .3, 0), pole=(-1, -.3, -.4)))
    dash_c = P(dash_a, torso=(0, 16, 0), r=H((-.08, .30, .06), (-1, .7, 0), edge=(-1, 0, 0), pole=(1, -.2, -.6)),
               l=H((.08, .29, .04), (1, .7, 0), edge=(1, 0, 0), pole=(-1, -.2, -.6)))
    HITS["Vx_Dagger_Special_Full"] = .22
    return {
        "Vx_Dagger_Hold": (breathe(DAGGER), True),
        "Vx_Dagger_Attack_01": (strike(DAGGER, r_w, r_h, r_h, .08, .15, .34, "Vx_Dagger_Attack_01"), False),
        "Vx_Dagger_Attack_02": (strike(DAGGER, l_w, l_h, l_h, .08, .15, .34, "Vx_Dagger_Attack_02"), False),
        "Vx_Dagger_Attack_03": (strike(DAGGER, cross_w, cross_h, cross_h, .18, .3, .6, "Vx_Dagger_Attack_03"), False),
        "Vx_Dagger_Special_Full": ([k(0, **DAGGER), k(.06, **dash_a), k(.16, **dash_b), k(.24, ease="out", **dash_c), k(.36, **dash_c), k(.55, **DAGGER)], False),
    }


# ---- alabarda ---------------------------------------------------------------------------------------------
# Duas mãos, na diagonal, com o fio (a lâmina do machado) para a frente. Golpes: varrida larga de lado,
# corte de cima para baixo (com o fio, não com as costas) e estocada; secundário: corte vertical forte.
HALBERD = dict(torso=(-14, 2, 0), head=(10, 0, 0),
               r=H((.20, .10, -.04), (-.25, .55, 1), edge=(0, 1, -.3), pole=(1, -.3, -.5)),
               l={"on": .36, "pole": (-.8, .2, -1)})


@family("Halberd", "alabarda")
def halberd():
    sweep_w = P(HALBERD, torso=(36, 0, 0), head=(-30, 0, 0), r=H((.26, .04, .08), (1, -.1, .45), edge=(0, 1, 0), pole=(1, -.3, -.4)), l={"on": .30, "pole": (-.6, .4, -1)})
    sweep_h = P(HALBERD, torso=(-38, 8, 0), head=(30, 0, 0), r=H((.04, .22, .02), (-1, .75, .15), edge=(-1, -.2, 0), pole=(1, -.2, -.6)), l={"on": .30, "pole": (-.6, .4, -1)})
    chop_w = P(HALBERD, torso=(0, -12, 0), head=(0, -6, 0), r=H((.16, .06, .30), (.05, -.35, 1), edge=(0, 1, .2), pole=(1, -.2, -.3)), l={"on": .30, "pole": (-.8, .2, -.6)})
    chop_h = P(HALBERD, torso=(-4, 20, 0), hips=(0, 4, 0), r=H((.10, .26, -.02), (0, 1, -.35), edge=(0, -.2, -1), pole=(1, -.2, -.6)), l={"on": .30, "pole": (-.8, .2, -1)})
    thrust_w = P(HALBERD, torso=(-28, 0, 0), r=H((.22, -.02, -.02), (0, 1, .1), edge=(0, 0, 1), pole=(1, -.3, -.5)), l={"on": .36, "pole": (-.4, .3, -1)})
    thrust_h = P(HALBERD, torso=(-6, 10, 0), r=H((.10, .28, .04), (0, 1, .05), edge=(0, 0, 1), pole=(1, -.2, -.6)), l={"on": .18, "pole": (-.4, .3, -1)})
    big_w = P(chop_w, torso=(0, -18, 0), r=H((.14, .02, .42), (0, -.6, 1), edge=(0, 1, .3), pole=(1, -.2, -.2)))
    big_h = P(chop_h, torso=(0, 30, 0), hips=(0, 10, 0), legs={"step": -20, "bend": 16, "drop": .05}, root=(0, .12, 0),
              r=H((.08, .28, -.08), (0, 1, -.6), edge=(0, -.4, -1), pole=(1, -.2, -.6)))
    HITS["Vx_Halberd_Special_Full"] = .62
    return {
        "Vx_Halberd_Hold": (breathe(HALBERD), True),
        "Vx_Halberd_Attack_01": (strike(HALBERD, sweep_w, sweep_h, sweep_h, .24, .38, .75, "Vx_Halberd_Attack_01"), False),
        "Vx_Halberd_Attack_02": (strike(HALBERD, chop_w, chop_h, chop_h, .26, .4, .75, "Vx_Halberd_Attack_02"), False),
        "Vx_Halberd_Attack_03": (strike(HALBERD, thrust_w, thrust_h, thrust_h, .3, .44, .95, "Vx_Halberd_Attack_03"), False),
        "Vx_Halberd_Special_Full": ([k(0, **HALBERD), k(.42, **big_w), k(.62, ease="out", **big_h), k(.9, **big_h), k(1.25, **HALBERD)], False),
    }


# ---- machados duplos ----------------------------------------------------------------------------------------
DAXE = dict(torso=(0, 4, 0),
            r=H((.21, .16, .0), (.15, .5, 1), edge=(0, 1, 0), pole=(1, -.4, -.5)),
            l=H((-.21, .16, .0), (-.15, .5, 1), edge=(0, 1, 0), pole=(-1, -.4, -.5)))


@family("DualAxes", "machados_duplos", "machados_duplos_L")
def dual_axes():
    r_w = P(DAXE, torso=(20, -6, 0), r=H((.25, -.02, .34), (.3, -.6, .8), edge=(0, 1, .3), pole=(1, -.4, -.2)))
    r_h = P(DAXE, torso=(-16, 12, 0), r=H((.06, .29, .02), (-.2, 1, -.5), edge=(0, -.3, -1), pole=(1, -.2, -.6)))
    l_w = P(DAXE, torso=(-20, -6, 0), l=H((-.25, -.02, .34), (-.3, -.6, .8), edge=(0, 1, .3), pole=(-1, -.4, -.2)))
    l_h = P(DAXE, torso=(16, 12, 0), l=H((-.06, .29, .02), (.2, 1, -.5), edge=(0, -.3, -1), pole=(-1, -.2, -.6)))
    both_w = P(DAXE, torso=(0, -10, 0), r=r_w["r"], l=l_w["l"])
    both_h = P(DAXE, torso=(0, 16, 0), r=r_h["r"], l=l_h["l"])
    throw_w = P(DAXE, torso=(0, -12, 0), head=(0, -6, 0), r=H((.27, -.12, .30), (.2, -1, .4), edge=(0, 0, 1), pole=(1, -.2, -.2)),
                l=H((-.27, -.12, .30), (-.2, -1, .4), edge=(0, 0, 1), pole=(-1, -.2, -.2)))
    throw_h = P(DAXE, torso=(0, 16, 0), r=H((.18, .30, .14), (.2, 1, .1), edge=(0, 0, -1), pole=(1, -.2, -.6)),
                l=H((-.18, .30, .14), (-.2, 1, .1), edge=(0, 0, -1), pole=(-1, -.2, -.6)))
    clips = {"Vx_DualAxes_Hold": (breathe(DAXE), True)}
    names = ["Vx_DualAxes_Attack_%02d" % i for i in range(1, 7)]
    seq = [(r_w, r_h), (l_w, l_h), (r_w, r_h), (l_w, l_h), (both_w, both_h), (both_w, both_h)]
    times = [(.16, .26, .5)] * 4 + [(.22, .34, .62), (.28, .42, .82)]
    for n, (w, h), (a, b, c) in zip(names, seq, times):
        clips[n] = (strike(DAXE, w, h, h, a, b, c, n), False)
    clips["Vx_DualAxes_Special"] = (strike(DAXE, throw_w, throw_h, throw_h, .25, .38, .7, "Vx_DualAxes_Special"), False)
    return clips


# ---- machado ----------------------------------------------------------------------------------------------
AXE = dict(torso=(-6, 2, 0), r=H((.21, .15, .0), (.15, .45, 1), edge=(0, 1, 0), pole=(1, -.3, -.5)), l=LEFT_EASY)


@family("Axe", "machado")
def axe():
    over_w = P(AXE, torso=(20, -12, 0), head=(-14, -4, 0), r=H((.24, -.04, .38), (.2, -.7, .7), edge=(0, .3, 1), pole=(1, -.4, -.2)),
               l=H((-.14, .20, .10), (0, 1, .2), side=(0, 0, 1), pole=(-1, -.2, -.4)))
    over_h = P(AXE, torso=(-14, 20, 0), hips=(0, 4, 0), r=H((.08, .30, .0), (0, 1, -.55), edge=(0, -.3, -1), pole=(1, -.2, -.6)))
    diag_w = P(AXE, torso=(34, -4, 0), r=H((.29, .0, .28), (1, -.3, .6), edge=(-.4, .3, .8), pole=(1, -.5, -.2)))
    diag_h = P(AXE, torso=(-34, 12, 0), r=H((-.06, .27, -.02), (-1, .5, -.4), edge=(-.6, -.2, -.8), pole=(1, -.2, -.6)))
    flat_w = P(AXE, torso=(40, 0, 0), head=(-30, 0, 0), r=H((.30, .0, .14), (1, -.2, .15), edge=(-.2, 1, 0), pole=(1, -.5, -.2)))
    flat_h = P(AXE, torso=(-42, 8, 0), head=(30, 0, 0), r=H((-.10, .27, .10), (-1, .6, .05), edge=(-1, -.3, 0), pole=(1, -.2, -.6)))
    # giro (secundário): o machado estendido para o lado, o corpo inteiro roda (2 s)
    spin = [k(0, **AXE)]
    arm_out = H((.28, .10, .10), (1, .15, .05), edge=(0, 1, 0), pole=(1, -.2, -.5))
    for i in range(18):
        spin.append(k(.12 + i * .118, ease="linear", **P(AXE, root=(-90 * i, 0, 0), torso=(-10, 6, 0), legs={"bend": 10, "spread": 8}, r=arm_out)))
    spin.append(k(2.35, **AXE))
    HITS["Vx_Axe_Special_Full"] = .3
    return {
        "Vx_Axe_Hold": (breathe(AXE), True),
        "Vx_Axe_Attack_01": (strike(AXE, over_w, over_h, over_h, .3, .44, .82, "Vx_Axe_Attack_01"), False),
        "Vx_Axe_Attack_02": (strike(AXE, diag_w, diag_h, diag_h, .3, .44, .82, "Vx_Axe_Attack_02"), False),
        "Vx_Axe_Attack_03": (strike(AXE, flat_w, flat_h, flat_h, .38, .54, 1.05, "Vx_Axe_Attack_03"), False),
        "Vx_Axe_Special_Full": (spin, False),
    }


# ---- espadão ------------------------------------------------------------------------------------------------
# Duas mãos no cabo (a esquerda abaixo da direita), lâmina em pé na frente; cortes lentos e pesados com o
# fio na frente do movimento. Secundário: varrida de 180° que empurra.
GSWORD = dict(torso=(-10, 2, 0), head=(8, 0, 0),
              r=H((.14, .17, .0), (.12, .35, 1), edge=(0, 1, 0), pole=(1, -.3, -.6)),
              l={"on": -.12, "pole": (-.8, -.2, -.8)})


@family("Greatsword", "espadao")
def greatsword():
    d_w = P(GSWORD, torso=(36, -10, 0), head=(-26, 0, 0), r=H((.26, -.04, .34), (.6, -.6, .55), edge=(-.5, .3, .8), pole=(1, -.4, -.2)))
    d_h = P(GSWORD, torso=(-36, 16, 0), head=(26, 0, 0), hips=(-8, 4, 0), r=H((-.04, .26, -.04), (-.9, .5, -.45), edge=(-.5, -.3, -.8), pole=(1, -.2, -.6)))
    h_w = P(GSWORD, torso=(-30, -6, 0), head=(24, 0, 0), r=H((-.08, .18, .20), (-1, -.2, .3), edge=(.2, 1, 0), pole=(1, -.3, -.4)))
    h_h = P(GSWORD, torso=(36, 8, 0), head=(-28, 0, 0), r=H((.30, .14, .10), (1, .6, .1), edge=(1, -.3, 0), pole=(1, -.4, -.4)))
    v_w = P(GSWORD, torso=(0, -16, 0), head=(0, -8, 0), r=H((.14, .02, .40), (.05, -.7, .7), edge=(0, .3, 1), pole=(1, -.2, -.2)))
    v_h = P(GSWORD, torso=(-4, 26, 0), hips=(0, 8, 0), legs={"bend": 14, "drop": .04}, r=H((.08, .28, -.06), (0, 1, -.5), edge=(0, -.3, -1), pole=(1, -.2, -.6)))
    sw_w = P(GSWORD, torso=(60, -2, 0), hips=(30, 0, 0), head=(-50, 0, 0), legs={"bend": 12, "spread": 10},
             r=H((.30, -.04, .10), (1, -.4, .15), edge=(-.2, 1, 0), pole=(1, -.5, -.2)))
    sw_h = P(GSWORD, torso=(-62, 8, 0), hips=(-30, 0, 0), head=(50, 0, 0), legs={"bend": 12, "spread": 10},
             r=H((-.12, .24, .10), (-1, .3, .05), edge=(-.3, -1, 0), pole=(1, -.2, -.6)))
    HITS["Vx_Greatsword_Special_Full"] = .62
    return {
        "Vx_Greatsword_Hold": (breathe(GSWORD), True),
        "Vx_Greatsword_Attack_01": (strike(GSWORD, d_w, d_h, d_h, .38, .54, 1.0, "Vx_Greatsword_Attack_01"), False),
        "Vx_Greatsword_Attack_02": (strike(GSWORD, h_w, h_h, h_h, .38, .54, 1.0, "Vx_Greatsword_Attack_02"), False),
        "Vx_Greatsword_Attack_03_Full": (strike(GSWORD, v_w, v_h, v_h, .5, .68, 1.3, "Vx_Greatsword_Attack_03_Full"), False),
        "Vx_Greatsword_Special_Full": ([k(0, **GSWORD), k(.42, **sw_w), k(.62, ease="out", **sw_h), k(.9, **sw_h), k(1.3, **GSWORD)], False),
    }


# ---- espada e escudo ----------------------------------------------------------------------------------------
SS = dict(torso=(-8, 2, 0),
          r=H((.21, .14, .0), (.15, .4, 1), edge=(0, 1, 0), pole=(1, -.3, -.5)),
          l=H((-.17, .22, .06), (0, .1, 1), side=(0, 1, 0), pole=(-1, -.3, -.5)))


@family("SwordShield", "espada_escudo", "espada_escudo_L")
def sword_shield():
    a_w = P(SS, torso=(26, -6, 0), r=H((.27, .0, .32), (.6, -.4, .8), edge=(-.4, .3, .8), pole=(1, -.4, -.2)))
    a_h = P(SS, torso=(-26, 10, 0), r=H((-.02, .28, -.02), (-.8, .6, -.3), edge=(-.5, -.3, -.8), pole=(1, -.2, -.6)))
    b_w = P(SS, torso=(-22, 0, 0), r=H((.0, .20, .14), (-1, .2, .2), edge=(.2, 1, 0), pole=(1, -.3, -.4)))
    b_h = P(SS, torso=(28, 6, 0), r=H((.30, .14, .08), (1, .6, .1), edge=(1, -.4, 0), pole=(1, -.4, -.4)))
    c_w = P(SS, torso=(10, -10, 0), r=H((.22, -.02, .36), (.1, -.5, 1), edge=(0, .4, 1), pole=(1, -.3, -.2)))
    c_h = P(SS, torso=(-10, 18, 0), hips=(0, 4, 0), r=H((.10, .30, .02), (0, 1, -.4), edge=(0, -.4, -1), pole=(1, -.2, -.6)))
    guard = P(SS, torso=(6, 4, 0), r=H((.24, .02, .02), (.2, -.3, 1), edge=(0, 1, 0), pole=(1, -.4, -.4)),
              l=H((-.06, .27, .18), (0, .1, 1), side=(0, 1, 0), pole=(-1, -.3, -.3)))
    return {
        "Vx_SwordShield_Hold": (breathe(SS), True),
        "Vx_SwordShield_Attack_01": (strike(SS, a_w, a_h, a_h, .16, .28, .55, "Vx_SwordShield_Attack_01"), False),
        "Vx_SwordShield_Attack_02": (strike(SS, b_w, b_h, b_h, .16, .28, .6, "Vx_SwordShield_Attack_02"), False),
        "Vx_SwordShield_Attack_03": (strike(SS, c_w, c_h, c_h, .3, .46, .95, "Vx_SwordShield_Attack_03"), False),
        "Vx_SwordShield_Guard": ([k(0, **guard), k(1.2, **P(guard, torso=(6, 6, 0))), k(2.4, **guard)], True),
    }


# ---- corrente com lâmina e chicote ---------------------------------------------------------------------------
# A corrente e o chicote balançam por conta própria (física no jogo, G7); o braço faz o gesto: girar,
# chicotear para a frente e puxar.
CHAIN = dict(torso=(-6, 2, 0), r=H((.22, .14, .02), (.3, .6, -.6), side=(1, 0, 0), pole=(1, -.3, -.5)), l=LEFT_EASY)


def lash(hold, name, t=(.18, .3, .55), side=False):
    if side:
        w = P(hold, torso=(30, -2, 0), r=H((.30, .02, .22), (1, -.2, .4), side=(0, 0, 1), pole=(1, -.5, -.2)))
        h = P(hold, torso=(-30, 8, 0), r=H((-.04, .28, .10), (-1, .7, 0), side=(0, 0, 1), pole=(1, -.2, -.6)))
    else:
        w = P(hold, torso=(14, -10, 0), head=(-10, 0, 0), r=H((.26, -.08, .40), (.2, -.6, .8), side=(1, 0, 0), pole=(1, -.4, -.2)))
        h = P(hold, torso=(-12, 14, 0), r=H((.12, .31, .12), (0, 1, -.2), side=(1, 0, 0), pole=(1, -.2, -.6)))
    return strike(hold, w, h, h, *t, name=name)


@family("Chain", "correntes")
def chain():
    whirl = [k(0, **CHAIN)]
    for i in range(6):
        a = math.radians(i * 120)
        whirl.append(k(.1 + i * .12, ease="linear", **P(CHAIN, torso=(-10, 0, 0),
                     r=H((.14 + .1 * math.cos(a), .18, .36 + .08 * math.sin(a)), (math.cos(a), .2, math.sin(a)), side=(0, 1, 0), pole=(1, -.3, -.2)))))
    whirl.append(k(.95, **CHAIN))
    HITS["Vx_Chain_Attack_03"] = .5
    quick = [k(0, **CHAIN)]
    t = 0
    for i in range(3):
        keys = lash(CHAIN, None, (t + .08, t + .15, t + .27), side=bool(i % 2))
        quick += keys[1:4]
        t += .27
    quick.append(k(t + .15, **CHAIN))
    HITS["Vx_Chain_Special"] = .15
    return {
        "Vx_Chain_Hold": (breathe(CHAIN), True),
        "Vx_Chain_Attack_01": (lash(CHAIN, "Vx_Chain_Attack_01"), False),
        "Vx_Chain_Attack_02": (lash(CHAIN, "Vx_Chain_Attack_02", side=True), False),
        "Vx_Chain_Attack_03": (whirl, False),
        "Vx_Chain_Special": (quick, False),
    }


WHIP = dict(torso=(-6, 2, 0), r=H((.22, .14, .02), (.3, .6, -.6), side=(1, 0, 0), pole=(1, -.3, -.5)), l=LEFT_EASY)


@family("Whip", "chicote")
def whip():
    throw = P(WHIP, torso=(-10, 10, 0), r=H((.12, .31, .22), (0, 1, .35), side=(1, 0, 0), pole=(1, -.2, -.6)))
    pull = P(WHIP, torso=(16, -10, 0), r=H((.24, -.06, .14), (0, -.3, 1), side=(1, 0, 0), pole=(1, -.4, -.3)),
             l=H((-.04, .2, .12), (.5, .5, .2), side=(0, 0, 1), pole=(-1, -.3, -.4)))
    wind = P(WHIP, torso=(14, -8, 0), r=H((.26, -.06, .38), (.2, -.6, .8), side=(1, 0, 0), pole=(1, -.4, -.2)))
    HITS["Vx_Whip_Special"] = .3
    return {
        "Vx_Whip_Hold": (breathe(WHIP), True),
        "Vx_Whip_Attack_01": (lash(WHIP, "Vx_Whip_Attack_01", (.16, .28, .5)), False),
        "Vx_Whip_Attack_02": (lash(WHIP, "Vx_Whip_Attack_02", (.16, .28, .5), side=True), False),
        "Vx_Whip_Attack_03": (lash(WHIP, "Vx_Whip_Attack_03", (.3, .46, .8)), False),
        "Vx_Whip_Special": ([k(0, **WHIP), k(.18, **wind), k(.3, ease="out", **throw), k(.5, **throw), k(.75, **pull), k(1.0, **WHIP)], False),
    }


# ---- foice ---------------------------------------------------------------------------------------------------
# Duas mãos no cabo, a lâmina no alto; colheitas largas (o fio vai na frente). Giro (secundário): o corpo
# inteiro roda 360° em 0,6 s.
SCY = dict(torso=(-12, 2, 0), head=(8, 0, 0),
           r=H((.19, .10, -.04), (-.2, .3, 1), edge=(-.6, .8, 0), pole=(1, -.3, -.5)),
           l={"on": .36, "pole": (-.8, .2, -1)})


@family("Scythe", "foice")
def scythe():
    a_w = P(SCY, torso=(40, -4, 0), head=(-30, 0, 0), r=H((.27, .02, .10), (1, -.2, .5), edge=(-.2, 1, 0), pole=(1, -.4, -.4)), l={"on": .30, "pole": (-.6, .4, -1)})
    a_h = P(SCY, torso=(-40, 10, 0), head=(30, 0, 0), r=H((.0, .24, .0), (-1, .6, .2), edge=(-.3, -1, 0), pole=(1, -.2, -.6)), l={"on": .30, "pole": (-.6, .4, -1)})
    c_w = P(SCY, torso=(0, -14, 0), r=H((.16, .04, .32), (0, -.4, 1), edge=(0, 1, .3), pole=(1, -.2, -.3)), l={"on": .30, "pole": (-.8, .2, -.6)})
    c_h = P(SCY, torso=(-4, 24, 0), hips=(0, 6, 0), legs={"bend": 12}, r=H((.10, .26, -.04), (0, 1, -.4), edge=(0, -.4, -1), pole=(1, -.2, -.6)), l={"on": .30, "pole": (-.8, .2, -1)})
    spin = [k(0, **SCY), k(.1, **P(a_w, legs={"bend": 12, "spread": 10}))]
    for i in range(1, 5):
        spin.append(k(.12 + i * .11, ease="linear", **P(SCY, root=(-90 * i, 0, 0), torso=(-20, 6, 0), legs={"bend": 12, "spread": 10},
                                                      r=H((.24, .16, .06), (-.3, 1, .1), edge=(-1, -.2, 0), pole=(1, -.2, -.5)), l={"on": .30, "pole": (-.6, .4, -1)})))
    spin.append(k(.85, **SCY))
    HITS["Vx_Scythe_Special_Full"] = .3
    return {
        "Vx_Scythe_Hold": (breathe(SCY), True),
        "Vx_Scythe_Attack_01": (strike(SCY, a_w, a_h, a_h, .2, .34, .6, "Vx_Scythe_Attack_01"), False),
        "Vx_Scythe_Attack_02": (strike(SCY, P(a_h, torso=(-30, 0, 0)), P(a_w, torso=(36, 8, 0)), a_w, .2, .34, .6, "Vx_Scythe_Attack_02"), False),
        "Vx_Scythe_Attack_03": (strike(SCY, c_w, c_h, c_h, .32, .5, .95, "Vx_Scythe_Attack_03"), False),
        "Vx_Scythe_Special_Full": (spin, False),
    }


# ---- chakrams ---------------------------------------------------------------------------------------------------
CHK = dict(torso=(0, 4, 0),
           r=H((.20, .18, .02), (.3, 1, 0), side=(0, 0, 1), pole=(1, -.4, -.5)),
           l=H((-.20, .18, .02), (-.3, 1, 0), side=(0, 0, 1), pole=(-1, -.4, -.5)))


@family("Chakram", "chakrams", "chakrams_L")
def chakram():
    r_w = P(CHK, torso=(-30, 0, 0), r=H((-.04, .18, .14), (-1, .2, 0), side=(0, 0, 1), pole=(1, -.3, -.4)))
    r_h = P(CHK, torso=(20, 6, 0), r=H((.28, .26, .12), (1, .8, 0), side=(0, 0, 1), pole=(1, -.3, -.5)))
    l_w = P(CHK, torso=(30, 0, 0), l=H((.04, .18, .14), (1, .2, 0), side=(0, 0, 1), pole=(-1, -.3, -.4)))
    l_h = P(CHK, torso=(-20, 6, 0), l=H((-.28, .26, .12), (-1, .8, 0), side=(0, 0, 1), pole=(-1, -.3, -.5)))
    big_w = P(CHK, torso=(0, -14, 0), head=(0, -8, 0), r=H((.12, .0, .42), (0, -.4, 1), side=(0, 1, 0), pole=(1, -.2, -.2)),
              l=H((-.12, .0, .42), (0, -.4, 1), side=(0, 1, 0), pole=(-1, -.2, -.2)))
    big_h = P(CHK, torso=(0, 20, 0), r=H((.12, .31, .10), (0, 1, -.2), side=(0, 0, 1), pole=(1, -.2, -.6)),
              l=H((-.12, .31, .10), (0, 1, -.2), side=(0, 0, 1), pole=(-1, -.2, -.6)))
    return {
        "Vx_Chakram_Hold": (breathe(CHK), True),
        "Vx_Chakram_Attack_01": (strike(CHK, r_w, r_h, r_h, .14, .24, .45, "Vx_Chakram_Attack_01"), False),
        "Vx_Chakram_Attack_02": (strike(CHK, l_w, l_h, l_h, .14, .24, .45, "Vx_Chakram_Attack_02"), False),
        "Vx_Chakram_Attack_03": (strike(CHK, P(r_w, l=l_w["l"], torso=(0, -4, 0)), P(r_h, l=l_h["l"], torso=(0, 8, 0)), r_h, .2, .32, .7, "Vx_Chakram_Attack_03"), False),
        "Vx_Chakram_Special": (strike(CHK, big_w, big_h, big_h, .35, .5, .9, "Vx_Chakram_Special"), False),
    }


# ---- pistola rúnica -----------------------------------------------------------------------------------------------
# Cabo na mão, cano para a frente; mirando, o braço estica; cada tiro dá um coice para cima.
PISTOL = dict(torso=(-6, 2, 0), r=H((.21, .19, .02), (0, 1, -.25), side=(1, 0, 0), pole=(1, -.3, -.6)), l=LEFT_EASY)


@family("Pistol", "pistola_runica")
def pistol():
    aim = P(PISTOL, torso=(-14, 2, 0), head=(10, 0, 0), r=H((.12, .33, .19), (0, 1, 0), side=(1, 0, 0), pole=(1, -.2, -.7)))
    kick = P(aim, r=H((.13, .28, .23), (0, 1, .45), side=(1, 0, 0), pole=(1, -.2, -.7)))

    def shot(name, t=(.06, .12, .3)):
        HITS[name] = t[0]
        return [k(0, **aim), k(t[0], **aim), k(t[1], ease="out", **kick), k(t[2], **aim)]
    charge = P(aim, l=H((.04, .27, .14), (.4, .6, .3), side=(0, 0, 1), pole=(-1, -.3, -.5)))
    HITS["Vx_Pistol_Special"] = .3
    return {
        "Vx_Pistol_Hold": (breathe(PISTOL), True),
        "Vx_Pistol_Attack_01": (shot("Vx_Pistol_Attack_01"), False),
        "Vx_Pistol_Attack_02": (shot("Vx_Pistol_Attack_02"), False),
        "Vx_Pistol_Attack_03": (shot("Vx_Pistol_Attack_03", (.12, .2, .5)), False),
        "Vx_Pistol_Special": ([k(0, **aim), k(.25, **charge), k(.35, ease="out", **P(kick, l=charge["l"])), k(.6, **aim)], False),
    }


# ---- besta --------------------------------------------------------------------------------------------------------
XBOW = dict(torso=(-8, 2, 0),
            r=H((.12, .22, .06), (0, 1, .1), side=(0, 0, 1), pole=(1, -.3, -.6)),
            l={"on": .18, "pole": (-1, -.2, -.6)})


@family("Crossbow", "besta")
def crossbow():
    aim = P(XBOW, torso=(-10, 0, 0), head=(6, 0, 0), r=H((.10, .24, .20), (0, 1, 0), side=(0, 0, 1), pole=(1, -.2, -.6)))
    kick = P(aim, torso=(-10, -6, 0), r=H((.11, .20, .23), (0, 1, .3), side=(0, 0, 1), pole=(1, -.2, -.6)))

    def shots(n, name, gap, end):
        HITS[name] = .06
        keys = [k(0, **aim)]
        t = 0
        for i in range(n):
            keys += [k(t + .06, **aim), k(t + .1, ease="out", **kick)]
            t += gap
        keys.append(k(max(end, t + .25), **aim))
        return keys
    return {
        "Vx_Crossbow_Hold": (breathe(XBOW), True),
        "Vx_Crossbow_Attack_01": (shots(1, "Vx_Crossbow_Attack_01", .3, .8), False),
        "Vx_Crossbow_Special": (shots(3, "Vx_Crossbow_Special", .15, .8), False),
    }


# ---- arco ---------------------------------------------------------------------------------------------------------
# O arco na mão direita (o braço do arco estica para a frente), a corda virada para o herói e a curva
# para o alvo; a esquerda puxa a corda até o queixo.
# Segurar o clique carrega (Vx_Bow_Draw); soltar dispara (Vx_Bow_Release).
BOW = dict(torso=(-8, 2, 0), r=H((.22, .14, -.04), (.1, .5, 1), edge=(0, -1, 0), pole=(1, -.3, -.5)), l=LEFT_EASY)


@family("Bow", "arco_de_betula")
def bow():
    raise_ = P(BOW, torso=(-24, 0, 0), head=(20, 0, 0),
               r=H((.10, .31, .20), (0, 0, 1), edge=(0, -1, 0), pole=(1, -.2, -.6)),
               l=H((.04, .28, .19), (0, -1, 0), side=(0, 0, 1), pole=(-1, .2, -.4)))
    drawn = P(raise_, torso=(-34, -2, 0), head=(30, 0, 0),
              l=H((-.04, .10, .26), (0, -1, 0), side=(0, 0, 1), pole=(-1, .6, -.1)))
    loose = P(drawn, l=H((-.10, .02, .28), (-.3, -1, .2), side=(0, 0, 1), pole=(-1, .6, -.1)))
    rain = P(BOW, torso=(-20, -18, 0), head=(14, -6, 0), r=H((.10, .22, .38), (0, .3, 1), edge=(0, -1, .3), pole=(1, -.2, -.4)),
             l=H((-.02, .06, .34), (0, -1, -.3), side=(0, 0, 1), pole=(-1, .5, -.1)))
    rain_loose = P(rain, l=H((-.10, -.02, .36), (-.3, -1, 0), side=(0, 0, 1), pole=(-1, .5, -.1)))
    HITS["Vx_Bow_Release"] = .02
    HITS["Vx_Bow_Special"] = .55
    return {
        "Vx_Bow_Hold": (breathe(BOW), True),
        "Vx_Bow_Draw": ([k(0, **raise_), k(.5, ease="out", **drawn), k(.8, **drawn), k(1.6, **P(drawn, torso=(-34, -3, 0)))], False),
        "Vx_Bow_Release": ([k(0, **drawn), k(.06, ease="out", **loose), k(.3, **loose), k(.6, **raise_)], False),
        "Vx_Bow_Special": ([k(0, **BOW), k(.2, **raise_), k(.5, **rain), k(.58, ease="out", **rain_loose), k(.9, **rain), k(1.2, **BOW)], False),
    }


# ---- beber poção (só o braço esquerdo e a cabeça: a arma continua na direita) ------------------------
# A garrafinha na mão esquerda sobe até a boca, a cabeça inclina para trás, três goles e desce.
# O efeito da poção entra no fim dos goles (1,25 s); cancelar antes não gasta a poção.
@family("Drink", "catalisador", "pocao_vermelha")
def drink():
    rest = dict(l=H((-.21, .12, -.02), (0, .3, 1), side=(-1, 0, 0), pole=(-1, -.3, -.5)))
    lift = dict(head=(0, -4, 0), l=H((-.10, .24, .22), (0, .15, 1), side=(-1, 0, 0), pole=(-1, -.2, -.6)))
    sip = dict(head=(0, -20, 0), torso=(0, -4, 0), l=H((-.03, .27, .31), (0, -.75, .7), side=(-1, 0, 0), pole=(-1, -.1, -.7)))
    gulp = dict(head=(0, -15, 0), torso=(0, -3, 0), l=H((-.03, .26, .30), (0, -.7, .75), side=(-1, 0, 0), pole=(-1, -.1, -.7)))
    HITS["Vx_Drink_LeftArm"] = 1.25
    return {
        "Vx_Drink_LeftArm": ([k(0, **rest), k(.3, **lift), k(.48, **sip), k(.66, **gulp), k(.84, **sip), k(1.02, **gulp), k(1.2, **sip),
                              k(1.4, **lift), k(1.65, **rest)], False),
    }
