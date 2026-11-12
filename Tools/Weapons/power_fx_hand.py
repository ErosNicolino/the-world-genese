"""Os efeitos dos poderes feitos à mão, um por arma (07/11, o usuário: "BEM detalhado cada habilidade, uma por uma, com
efeitos, bem únicos"). Lido pelo Tools/Weapons/BuildPowerFx.py: FX = {"espada_<chave>" | "arco_<chave>": receita}; a de
carregar (overload) é "<...>_carga" (aparece enquanto segura).

Cada receita: as cores e as peças do AVortexPowerFx (Source/Vortex/Private/Combat/VortexPowerFx.cpp). Os tempos ("delay",
em segundos desde o começo do poder) casam com a animação (Tools/Blender/power_anims_hand.py) e com o golpe do poder.
Peças: slash (o corte no ar), trail (o rastro na lâmina), streak (o rastro do corpo no avanço), vanish (o herói some),
ring, glyph (a runa no chão), pillar (coluna; "wall": em fila de lado), spikes (saem do chão), shards (estilhaços),
orbs, rain ("up": sobe), spiral ("wide"/"height", parada num ponto com "front"/"at"), beam ("pitch" para o céu, "gap"
entre eles, "spread" o leque), feathers, bolt (raio do céu; "hero": em cima do herói), wave (a onda na linha da mira),
cracks (rachaduras) e sprites (as texturas animadas do EffectCraft: agua, poeira, brilho, faisca, fumaca, fogo, folhas,
gelo, luz; "at": "line" em fila na frente, "wall" em fila de lado, "ring" em anel). "at": "aim" = onde a mira bate no
chão; "front": N = N cm à frente; "lit": 1 = sólido com luz e sombra (pedra, madeira, água) em vez de aceso.
"""


def P(t, **k):
    k["type"] = t
    return k


FIRE = [[1, .42, .08], [1, .78, .25], [.85, .12, .03], [.3, .08, .03]]
ICE = [[.5, .82, 1], [.9, .98, 1], [.25, .55, .95], [.75, .9, 1]]
HOLY = [[1, .82, .38], [1, 1, .8], [1, .62, .2], [.95, .9, .7]]
VOID = [[.5, .2, .95], [.85, .5, 1], [.18, .04, .35], [.06, .02, .1]]
STORM = [[.4, .68, 1], [.9, .95, 1], [.6, .35, 1], [1, 1, .7]]
WATER = [[.06, .3, .6], [.25, .62, .95], [.85, .95, 1], [.04, .18, .38]]
LEAF = [[.32, .78, .25], [.72, 1, .4], [.14, .45, .1], [.45, .3, .15]]

FX = {
    # =================================== ESPADAS ===================================
    # Espada de Treino: a estocada de aluno: o corte fino de ponta, o rastro de poeira no chão do treino e a lasca de
    # madeira do boneco no fim
    "espada_comum_2": {"colors": [[.85, .8, .7], [1, 1, .95], [.55, .44, .32]], "parts": [
        P("trail", color=1, life=.5, size=.9),
        P("streak", color=0, life=.5, size=.7, until=.7, fade=.35, rise=20, height=-70, spray=.5),
        P("sprites", tex="poeira", life=.6, count=7, radius=450, size=1.0, rise=40, at="line", spread=.3),
        P("slash", color=1, delay=.18, life=.25, sweep=40, radius=230, tilt=5, count=1),
        P("shards", color=2, lit=1, delay=.25, life=.8, count=6, size=.7, small=1, front=520, seed=3),
    ]},
    # Agulha do Viajante: cinco riscos finos de prata, uma faísca na ponta a cada agulhada, as agulhas de luz que voam
    "espada_comum_3": {"colors": [[.78, .84, .95], [1, 1, 1], [.4, .55, .85]], "parts": [
        P("trail", color=1, life=1.2, size=.7),
        P("slash", color=0, delay=.08, life=1.1, sweep=28, radius=200, count=5, small=1, tilt=12),
        P("sprites", tex="faisca", delay=.1, life=.35, count=5, radius=40, size=.8, rise=60, front=140, spread=1.0),
        P("shards", color=1, delay=.15, life=1.1, count=10, size=.6, style=1, front=150, seed=12),
        P("sprites", tex="brilho", delay=1.25, life=.5, count=3, radius=60, size=.9, rise=80, spread=.1),
    ]},
    # Velha Companheira: o corte largo enferrujado, a poeira em leque, as pedrinhas e lascas de ferrugem que pulam
    "espada_comum_4": {"colors": [[.6, .44, .3], [.88, .82, .72], [.4, .34, .3], [.75, .5, .3]], "parts": [
        P("trail", color=1, delay=.2, life=.6, size=1.1),
        P("slash", color=3, delay=.48, life=.3, sweep=210, radius=400, tilt=-8, count=1),
        P("sprites", tex="poeira", delay=.6, life=.9, count=10, radius=260, size=1.6, out=220, rise=30, at="ring", spread=.1),
        P("ring", color=2, lit=1, delay=.6, life=.6, count=12, radius=330, size=.8),
        P("shards", color=3, lit=1, delay=.62, life=.9, count=10, size=.7, seed=4, front=200),
    ]},
    # Cutelo do Lenhador: o golpe na tora: lascas de madeira, a rachadura curta, as farpas em anel e as folhas que caem
    "espada_comum_5": {"colors": [[.48, .32, .16], [.78, .58, .32], [.3, .2, .1], [.88, .82, .7]], "parts": [
        P("trail", color=1, delay=.25, life=.5, size=1.0),
        P("cracks", color=2, lit=1, delay=.7, life=1.3, count=5, radius=260, size=1.3, seed=8, front=90),
        P("shards", color=1, lit=1, delay=.7, life=1.0, count=14, size=.8, style=1, front=90, seed=19),
        P("spikes", color=0, lit=1, delay=.72, life=.9, count=8, radius=170, size=.8, style=1, at="ring", front=90, seed=2),
        P("sprites", tex="poeira", delay=.7, life=.8, count=6, radius=120, size=1.3, out=120, rise=50, front=90, spread=.08),
        P("sprites", tex="folhas", delay=.8, life=1.4, count=5, radius=250, size=1.3, height=260, rise=-30, spread=.4),
    ]},
    # Foice-Verde do Bosque: o corte rasteiro de 360°, a grama cortada voando em anel, as folhas girando no ar
    "espada_incomum_1": {"colors": LEAF, "parts": [
        P("trail", color=1, life=.6, size=1.0),
        P("slash", color=0, delay=.12, life=.4, sweep=360, radius=320, low=1, count=1),
        P("spikes", color=2, lit=1, delay=.3, life=.7, count=16, radius=300, size=.6, style=1, at="ring", seed=5),
        P("sprites", tex="folhas", delay=.3, life=1.1, count=10, radius=260, size=1.2, out=260, rise=120, gravity=200, at="ring", spread=.15),
        P("feathers", color=1, delay=.35, life=1.4, count=12, radius=320, size=.8, seed=9),
    ]},
    # Espada de Cobre Batido: as duas marteladas soltam faísca; a terceira manda placas de cobre saindo do chão em fila,
    # faíscas na trilha e a coluna de cobre no fim
    "espada_incomum_2": {"colors": [[.85, .45, .2], [1, .72, .38], [.5, .26, .12], [1, .95, .7]], "parts": [
        P("sprites", tex="faisca", delay=.1, life=.35, count=3, radius=30, size=.9, rise=100, front=110, spread=.02),
        P("sprites", tex="faisca", delay=.3, life=.35, count=3, radius=30, size=.9, rise=100, front=110, spread=.02),
        P("spikes", color=0, lit=1, delay=.62, life=1.0, count=12, radius=250, size=1.1, at="line", seed=6),
        P("sprites", tex="faisca", delay=.62, life=.5, count=8, radius=650, size=1.0, rise=140, at="line", spread=.35),
        P("ring", color=1, delay=.62, life=.45, count=8, radius=160, size=.8, front=120),
        P("pillar", color=1, delay=.95, life=.6, size=.5, at="end", radius=260),
    ]},
    # Ferrão da Noite: o rastro roxo do bote, a fumaça escura no caminho e as gotas verdes de veneno no fim
    "espada_incomum_5": {"colors": [[.42, .15, .62], [.55, 1, .25], [.15, .05, .25]], "parts": [
        P("trail", color=1, life=.6, size=1.0),
        P("streak", color=0, life=.5, size=.9, until=.7, fade=.5, rise=30, height=-20),
        P("sprites", tex="fumaca", life=.9, count=7, radius=550, size=1.3, rise=50, at="line", spread=.3),
        P("shards", color=1, delay=.25, life=.9, count=12, size=.6, style=2, front=600, seed=14),
        P("orbs", color=1, delay=.3, life=.8, count=8, radius=140, size=.7, style=2, front=600),
    ]},
    # Safira Gélida: a ponta brilha desenhando a runa; a runa de gelo gira no ponto da mira e, 1,5 s depois, estoura em
    # espinhos de gelo, cristais e o estouro de gelo
    "espada_raro_1": {"colors": ICE, "parts": [
        P("sprites", tex="brilho", life=.4, count=4, radius=40, size=.8, rise=40, front=60, height=150, spread=.25),
        P("glyph", color=0, delay=.4, life=1.5, count=10, radius=320, size=1.0, spin=90, at="aim"),
        P("spikes", color=3, lit=1, delay=1.82, life=1.1, count=14, radius=300, size=1.3, style=1, at="aim", seed=4),
        P("shards", color=1, delay=1.82, life=1.1, count=14, size=1.0, style=1, at="aim", seed=11),
        P("sprites", tex="gelo", delay=1.82, life=.7, count=8, radius=200, size=2.0, rise=60, at="aim", spread=.08),
        P("ring", color=2, delay=1.82, life=.5, count=14, radius=320, size=1.0, at="aim"),
    ]},
    # Lua Crescente: os cortes de prata a cada volta, as pequenas luas que sobem girando e o brilho no salto
    "espada_raro_2": {"colors": [[.75, .8, 1], [1, 1, 1], [.45, .5, .85]], "parts": [
        P("trail", color=1, life=2.0, size=1.0),
        P("slash", color=0, delay=.2, life=1.8, sweep=200, radius=300, count=4, tilt=18),
        P("orbs", color=1, delay=.3, life=1.8, count=10, radius=260, size=.9, style=1, follow=1, spin=200),
        P("sprites", tex="brilho", delay=.8, life=.6, count=6, radius=120, size=1.2, rise=60, height=140, spread=.2),
        P("ring", color=2, delay=1.7, life=.6, count=16, radius=320, size=.9, style=1),
    ]},
    # Trovão Partido, carregando: o raio cai na espada erguida e as faíscas juntam em volta
    "espada_raro_3_carga": {"colors": STORM, "parts": [
        P("bolt", color=1, life=.35, size=1.0, hero=1, height=600, seed=3),
        P("orbs", color=0, life=.6, count=10, radius=200, size=.5, converge=1),
        P("sprites", tex="faisca", life=.4, count=5, radius=60, size=.8, rise=80, height=180, spread=.3),
    ]},
    # Trovão Partido, soltando: o feixe do raio, o clarão e o raio que cai no alvo
    "espada_raro_3": {"colors": STORM, "parts": [
        P("beam", color=1, life=.4, size=1.6, radius=400, count=1),
        P("sprites", tex="luz", life=.4, count=2, radius=20, size=1.6, rise=0, front=120, height=100, spread=.02),
        P("bolt", color=0, delay=.15, life=.4, size=1.2, at="aim", seed=8),
        P("sprites", tex="faisca", delay=.2, life=.5, count=8, radius=150, size=1.2, rise=150, at="aim", spread=.1),
        P("ring", color=2, delay=.2, life=.45, count=10, radius=260, size=.9, at="aim"),
    ]},
    # Presa da Serpente: sete botes verdes em zigue-zague, as gotas de veneno e a névoa verde no chão
    "espada_raro_4": {"colors": [[.35, .85, .2], [.75, .3, 1], [.15, .5, .1], [.9, 1, .6]], "parts": [
        P("trail", color=0, life=1.3, size=.9),
        P("slash", color=0, delay=.08, life=1.2, sweep=60, radius=230, count=7, small=1, tilt=25),
        P("shards", color=3, delay=.2, life=1.2, count=14, size=.6, style=2, front=180, seed=7),
        P("sprites", tex="fumaca", delay=.3, life=1.2, count=6, radius=200, size=1.4, rise=20, front=150, spread=.8),
        P("orbs", color=1, delay=.4, life=1.0, count=6, radius=160, size=.5, style=2, front=180),
    ]},
    # Coração de Ametista: os cristais roxos brotam do chão em volta, sobem e ficam girando; o anel roxo e o brilho
    "espada_raro_5": {"colors": [[.6, .3, .95], [.9, .65, 1], [.35, .12, .6]], "parts": [
        P("spikes", color=0, lit=1, delay=.3, life=1.0, count=10, radius=180, size=1.0, style=1, at="ring", seed=8),
        P("shards", color=1, delay=.45, life=1.0, count=10, size=.9, style=1, seed=20),
        P("orbs", color=1, delay=.45, life=1.2, count=12, radius=180, size=.8, follow=1, spin=300),
        P("ring", color=2, delay=.45, life=.6, count=14, radius=280, size=.9, style=1),
        P("sprites", tex="brilho", delay=.5, life=.6, count=6, radius=150, size=1.0, rise=90, spread=.3),
    ]},
    # Monolito Desperto: a queda: o chão racha largo, pedras enormes sobem em anel, poeira grossa, lascas de pedra e as
    # runas verde-água da lâmina acendem numa coluna
    "espada_epico_1": {"colors": [[.46, .44, .4], [.35, .9, .8], [.3, .28, .25], [.6, .58, .55]], "parts": [
        P("trail", color=1, delay=.3, life=.5, size=1.2),
        P("cracks", color=2, lit=1, delay=.75, life=1.8, count=12, radius=520, size=1.8, seed=5, front=80),
        P("spikes", color=3, lit=1, delay=.78, life=1.3, count=12, radius=380, size=1.6, style=2, at="ring", seed=9, front=80),
        P("sprites", tex="poeira", delay=.75, life=1.1, count=14, radius=260, size=2.2, out=260, rise=40, at="ring", spread=.1, front=80),
        P("shards", color=0, lit=1, delay=.75, life=1.2, count=16, size=1.1, seed=31, front=80),
        P("pillar", color=1, delay=.75, life=.8, size=.6, front=80),
        P("ring", color=1, delay=.75, life=.5, count=16, radius=520, size=1.0, front=80),
    ]},
    # Escritura do Eclipse: os traços roxos no ar, a runa escura girando no alvo com um círculo preto dentro; explode em
    # coluna roxa, fumaça e placas de sombra
    "espada_epico_2": {"colors": VOID, "parts": [
        P("sprites", tex="brilho", life=.35, count=5, radius=60, size=.8, rise=0, front=80, height=150, spread=.3),
        P("glyph", color=0, delay=.35, life=1.6, count=12, radius=380, size=1.1, spin=-120, style=1, at="aim"),
        P("glyph", color=2, delay=.45, life=1.4, count=8, radius=220, size=.9, spin=160, at="aim"),
        P("orbs", color=1, delay=.6, life=1.2, count=10, radius=380, size=.8, converge=1, style=2),
        P("pillar", color=0, delay=1.82, life=.8, size=.7, style=2, at="aim", radius=200),
        P("sprites", tex="fumaca", delay=1.82, life=1.0, count=10, radius=260, size=2.0, out=150, rise=60, at="aim", spread=.1),
        P("shards", color=3, delay=1.82, life=1.0, count=14, size=1.0, style=3, at="aim", seed=6),
    ]},
    # Lâmina Sanguínea: o corte vermelho em volta, o sangue que espirra em anel e as gotas que voltam para o herói
    # (a vida bebida)
    "espada_epico_3": {"colors": [[.78, .05, .1], [1, .32, .32], [.3, 0, .05]], "parts": [
        P("trail", color=1, life=.6, size=1.1),
        P("slash", color=0, delay=.08, life=.4, sweep=360, radius=360, low=1, count=1),
        P("shards", color=0, delay=.3, life=.9, count=16, size=.7, style=2, seed=13),
        P("ring", color=2, delay=.3, life=.5, count=14, radius=360, size=.9),
        P("orbs", color=1, delay=.5, life=.7, count=14, radius=420, size=.7, converge=1, spin=-200),
        P("sprites", tex="brilho", delay=.9, life=.5, count=4, radius=30, size=.9, rise=60, height=120, spread=.1),
    ]},
    # Fragmento do Vórtice: o raio desce na espada erguida, os fragmentos azuis orbitam e soltam faíscas
    "espada_epico_4": {"colors": STORM, "parts": [
        P("bolt", color=1, delay=.12, life=.45, size=1.2, hero=1, height=700, seed=4),
        P("bolt", color=2, delay=.3, life=.3, size=.9, hero=1, height=500, seed=9),
        P("shards", color=0, delay=.45, life=1.0, count=10, size=.8, style=1, seed=7),
        P("orbs", color=0, delay=.45, life=1.3, count=12, radius=220, size=.7, follow=1, spin=320),
        P("sprites", tex="faisca", delay=.2, life=.5, count=8, radius=80, size=1.0, rise=100, height=180, spread=.3),
        P("ring", color=2, delay=.45, life=.5, count=14, radius=260, size=.9, style=1),
    ]},
    # Ceifadora do Vazio: o redemoinho de blocos escuros, os cortes de sombra que aceleram, os orbes do vazio e a
    # fumaça; o anel escuro na parada
    "espada_epico_5": {"colors": VOID, "parts": [
        P("spiral", color=2, life=2.0, count=14, size=1.2, wide=140, height=280, spin=400, flare=1),
        P("slash", color=0, delay=.2, life=1.8, sweep=360, radius=320, count=5, low=1),
        P("orbs", color=1, delay=.3, life=1.8, count=10, radius=260, size=.7, style=2, follow=1, spin=400),
        P("sprites", tex="fumaca", delay=.3, life=1.6, count=10, radius=200, size=1.5, rise=40, spread=1.4),
        P("ring", color=3, delay=1.85, life=.5, count=16, radius=380, size=1.0, style=2),
    ]},
    # Suspiro de Azhûr: o sopro do colosso: chamas saindo do herói para a frente, as brasas subindo e os pilares de fogo
    # erguendo a muralha no ponto da mira
    "espada_lendario_1": {"colors": FIRE, "parts": [
        P("trail", color=1, delay=.1, life=.6, size=1.2),
        P("sprites", tex="fogo", delay=.4, life=.8, count=10, radius=80, size=1.8, ahead=700, rise=60, front=120, spread=.2),
        P("pillar", color=0, delay=.55, life=1.2, count=9, size=.8, wall=1, at="aim", radius=450),
        P("sprites", tex="fogo", delay=.55, life=1.3, count=12, radius=450, size=2.2, rise=60, wall=1, at="aim", spread=.25),
        P("rain", color=1, delay=.6, life=1.6, count=14, radius=400, size=.8, up=1, at="aim"),
    ]},
    # Aurora Eterna: a luz do amanhecer: a coluna dourada desce, a runa do sol, anéis rosa e dourado, penas de luz e as
    # partículas que sobem
    "espada_lendario_2": {"colors": [[1, .62, .45], [1, .9, .62], [.55, .9, .8], [1, .48, .7]], "parts": [
        P("sprites", tex="luz", delay=.1, life=.5, count=2, radius=20, size=1.4, height=230, rise=0, spread=.1),
        P("pillar", color=1, delay=.5, life=1.0, size=.8, front=110),
        P("glyph", color=0, delay=.62, life=1.2, count=12, radius=300, size=1.0, spin=60, front=110),
        P("ring", color=3, delay=.62, life=.6, count=16, radius=420, size=1.0, front=110),
        P("ring", color=2, delay=.75, life=.6, count=16, radius=300, size=.9, front=110),
        P("feathers", color=1, delay=.65, life=1.5, count=14, radius=300, size=.9, front=110, seed=2),
        P("rain", color=1, delay=.7, life=1.4, count=12, radius=320, size=.7, up=1, front=110),
    ]},
    # Asa da Fênix: o voo em fogo: o rastro de brasa do corpo, as penas de fogo caindo no caminho e as chamas no chão
    "espada_lendario_3": {"colors": FIRE, "parts": [
        P("streak", color=0, life=.6, size=1.3, until=.65, fade=.6, rise=60, height=0, spray=1.4),
        P("feathers", color=1, delay=.1, life=1.4, count=16, radius=500, size=1.1, front=450, seed=6),
        P("sprites", tex="fogo", life=1.0, count=10, radius=900, size=1.7, rise=50, at="line", spread=.35),
        P("trail", color=1, delay=.3, life=.5, size=1.1),
        P("ring", color=2, delay=.45, life=.5, count=12, radius=260, size=.9, front=900),
    ]},
    # Sopro de Valdora: o vento do vale: um redemoinho de vento à frente, o corte largo verde-claro, as folhas
    # arremessadas para longe e o anel de vento
    "espada_lendario_4": {"colors": [[.62, .95, .72], [.92, 1, .95], [.3, .7, .35], [.5, .8, .3]], "parts": [
        P("spiral", color=1, delay=.1, life=1.0, count=12, size=.9, wide=110, height=260, spin=500),
        P("slash", color=0, delay=.5, life=.35, sweep=260, radius=420, tilt=-10, count=1),
        P("sprites", tex="folhas", delay=.62, life=1.1, count=14, radius=200, size=1.4, out=650, rise=150, at="ring", spread=.1),
        P("ring", color=1, delay=.62, life=.6, count=18, radius=500, size=1.0, style=1),
        P("feathers", color=3, delay=.65, life=1.2, count=10, radius=500, size=.8, seed=11),
    ]},
    # Coroa do Sol: a coroa de sóis sobre a cabeça, o feixe de sol na linha, a luz na trilha e a coluna no fim
    "espada_lendario_5": {"colors": HOLY, "parts": [
        P("orbs", color=1, delay=.1, life=.8, count=10, radius=90, size=.6, style=1, follow=1, spin=300),
        P("beam", color=0, delay=.62, life=.6, size=2.2, radius=270, long=1, count=1),
        P("sprites", tex="luz", delay=.62, life=.6, count=8, radius=1200, size=1.4, rise=40, at="line", spread=.25),
        P("glyph", color=2, delay=.6, life=.8, count=10, radius=200, size=.9, spin=90, front=120),
        P("pillar", color=1, delay=.85, life=.7, size=.6, front=1200),
    ]},
    # Fenda Esmeralda: o risco verde no chão da mira, as raízes que sobem em volta (e prendem) quando explode, a coluna
    # esmeralda e as folhas
    "espada_unico_1": {"colors": [[.1, .85, .45], [.6, 1, .7], [.05, .4, .2], [.38, .26, .14]], "parts": [
        P("trail", color=1, life=.5, size=1.0),
        P("cracks", color=0, delay=.4, life=1.6, count=7, radius=420, size=1.6, at="aim", seed=3),
        P("glyph", color=2, delay=.4, life=1.1, count=12, radius=420, size=1.0, spin=40, at="aim"),
        P("spikes", color=3, lit=1, delay=1.42, life=2.4, count=16, radius=360, size=1.5, style=1, at="aim", seed=8),
        P("pillar", color=1, delay=1.42, life=.8, size=.8, at="aim"),
        P("sprites", tex="folhas", delay=1.42, life=1.2, count=10, radius=300, size=1.4, out=150, rise=160, gravity=200, at="aim", spread=.1),
    ]},
    # Lágrima do Inverno: a nevasca: o redemoinho de flocos, os cristais de gelo saindo a cada volta, a neve caindo em
    # volta e o anel de gelo no fim
    "espada_unico_2": {"colors": ICE, "parts": [
        P("spiral", color=1, life=2.0, count=16, size=.8, wide=150, height=300, spin=-420, flare=.8),
        P("slash", color=0, delay=.2, life=1.8, sweep=300, radius=380, count=6, tilt=10),
        P("sprites", tex="gelo", delay=.3, life=1.8, count=12, radius=300, size=1.4, rise=60, spread=1.5),
        P("rain", color=3, delay=.2, life=2.0, count=18, radius=420, size=.6),
        P("shards", color=1, delay=1.75, life=1.0, count=14, size=1.0, style=1, seed=17),
        P("ring", color=2, delay=1.75, life=.6, count=18, radius=420, size=1.0, style=1),
    ]},
    # Raiz da Primeira Árvore: as raízes rompem o chão em duas ondas (perto e longe), o chão racha num raio enorme, os
    # brotos verdes, a poeira e a luz verde onde a espada entrou
    "espada_unico_3": {"colors": [[.42, .28, .15], [.4, .9, .35], [.3, .2, .1], [.65, .5, .3]], "parts": [
        P("trail", color=1, delay=.3, life=.6, size=1.1),
        P("cracks", color=2, lit=1, delay=.8, life=2.0, count=12, radius=600, size=1.8, seed=21, front=90),
        P("spikes", color=0, lit=1, delay=.82, life=1.4, count=10, radius=250, size=1.6, style=1, at="ring", seed=4, front=90),
        P("spikes", color=3, lit=1, delay=.98, life=1.4, count=16, radius=480, size=1.8, style=1, at="ring", seed=7, front=90),
        P("spikes", color=1, lit=1, delay=1.1, life=1.2, count=14, radius=420, size=.6, seed=12, front=90),
        P("sprites", tex="poeira", delay=.8, life=1.1, count=12, radius=300, size=2.0, out=200, rise=40, at="ring", spread=.2, front=90),
        P("pillar", color=1, delay=.8, life=.9, size=.45, front=90),
        P("sprites", tex="folhas", delay=1.0, life=1.5, count=8, radius=400, size=1.3, height=250, rise=-20, spread=.4),
    ]},
    # Silêncio Branco: o herói some; só uma linha de luz branca risca o caminho, cacos de espelho no ar e, quando ele
    # aparece, o corte enorme atrasado
    "espada_unico_4": {"colors": [[.95, .97, 1], [1, 1, 1], [.7, .75, .85]], "parts": [
        P("vanish", delay=.04, life=.3),
        P("streak", color=1, delay=.02, life=.6, size=.5, until=.55, fade=.5, rise=0, height=0, spray=.2),
        P("sprites", tex="brilho", delay=.05, life=.6, count=8, radius=1000, size=.9, rise=20, at="line", spread=.25),
        P("shards", color=2, delay=.3, life=1.0, count=12, size=.9, style=3, front=500, seed=9),
        P("slash", color=1, delay=.4, life=.3, sweep=300, radius=420, tilt=-15, count=1),
        P("trail", color=0, delay=.35, life=.6, size=.9),
    ]},
    # Sucata do Andarilho, carregando: a sucata faísca na mão, raios pequenos tremendo em volta
    "espada_unico_5_carga": {"colors": [[1, .85, .3], [.5, .8, 1], [.45, .4, .35], [1, 1, .8]], "parts": [
        P("sprites", tex="faisca", life=.45, count=8, radius=70, size=.9, rise=90, height=120, front=60, spread=.4),
        P("bolt", color=1, life=.3, size=.6, hero=1, height=200, seed=5),
        P("orbs", color=0, life=.45, count=8, radius=160, size=.45, converge=1),
    ]},
    # Sucata do Andarilho, soltando: a lasca voa e estoura no alvo em faíscas, pedaços de metal e dois raios tortos
    "espada_unico_5": {"colors": [[1, .85, .3], [.5, .8, 1], [.45, .4, .35], [1, 1, .8]], "parts": [
        P("beam", color=1, life=.35, size=1.0, radius=380, count=2, gap=.05, spread=8),
        P("sprites", tex="faisca", delay=.2, life=.6, count=14, radius=260, size=1.4, out=200, rise=180, at="aim", spread=.15),
        P("shards", color=2, lit=1, delay=.2, life=1.1, count=14, size=.9, at="aim", seed=4),
        P("bolt", color=0, delay=.2, life=.35, size=.9, at="aim", height=400, seed=2),
        P("bolt", color=1, delay=.32, life=.35, size=.8, at="aim", height=350, seed=13),
        P("sprites", tex="luz", delay=.2, life=.4, count=2, radius=30, size=2.0, at="aim", height=80, spread=.05),
    ]},
    # Juramento Celeste: a runa celeste imensa, três colunas de luz, as estrelas que sobem, as penas e o clarão
    "espada_divino_1": {"colors": [[1, .85, .45], [1, 1, .85], [.55, .72, 1]], "parts": [
        P("sprites", tex="brilho", delay=.05, life=.5, count=4, radius=30, size=.9, height=170, front=40, rise=20, spread=.2),
        P("glyph", color=0, delay=.6, life=1.4, count=14, radius=480, size=1.2, spin=45, style=2, front=110),
        P("pillar", color=1, delay=.6, life=1.1, count=6, radius=300, size=.6, style=2, front=110),
        P("orbs", color=2, delay=.62, life=1.3, count=12, radius=380, size=.8, style=1, front=110, spin=150),
        P("ring", color=1, delay=.62, life=.6, count=16, radius=300, size=1.1, style=2, front=110),
        P("feathers", color=1, delay=.7, life=1.6, count=16, radius=380, size=1.0, front=110, seed=4),
        P("sprites", tex="luz", delay=.62, life=.5, count=3, radius=60, size=2.2, front=110, height=60, spread=.05),
    ]},
    # Tridente do Primeiro Selo: a runa d'água enorme no alvo, o anel de ondas, e quando explode a coluna de água sobe com
    # os respingos e o gêiser de gotas
    "espada_divino_2": {"colors": WATER, "parts": [
        P("trail", color=1, life=.6, size=1.0),
        P("glyph", color=1, delay=.35, life=1.0, count=14, radius=520, size=1.2, spin=-90, style=2, at="aim"),
        P("ring", color=2, delay=.5, life=.8, count=16, radius=520, size=1.0, style=1, at="aim"),
        P("pillar", color=0, lit=1, delay=1.22, life=1.1, size=1.0, style=2, at="aim", radius=260),
        P("sprites", tex="agua", delay=1.22, life=.9, count=14, radius=320, size=2.4, out=200, rise=260, gravity=500, at="aim", spread=.12),
        P("rain", color=2, delay=1.25, life=1.3, count=16, radius=360, size=.8, up=1, at="aim"),
    ]},
    # Estrela Guia: a estrela do céu: a runa estrelada no chão, as estrelas que descem em volta, os orbes de estrela que
    # ficam girando e o brilho
    "espada_divino_3": {"colors": [[1, .92, .5], [1, 1, .9], [.6, .7, 1]], "parts": [
        P("sprites", tex="brilho", delay=.1, life=.6, count=5, radius=40, size=1.0, height=230, rise=30, front=30, spread=.3),
        P("glyph", color=2, delay=.45, life=1.2, count=12, radius=260, size=1.0, spin=120, style=3),
        P("rain", color=1, delay=.45, life=1.5, count=16, radius=420, size=.8),
        P("orbs", color=0, delay=.45, life=1.6, count=16, radius=240, size=.8, style=1, follow=1, spin=250),
        P("sprites", tex="luz", delay=.5, life=.6, count=6, radius=300, size=1.2, rise=0, at="ring", spread=.4),
    ]},
    # Voz do Santuário: a runa verde-dourada embaixo de quem reza, a espiral de luz que sobe, penas brancas e a coluna suave
    "espada_divino_4": {"colors": [[1, .95, .6], [.6, 1, .65], [1, 1, 1]], "parts": [
        P("glyph", color=1, life=1.5, count=12, radius=260, size=1.0, spin=40),
        P("spiral", color=0, delay=.2, life=1.4, count=14, size=.8, wide=90, height=320, spin=200),
        P("feathers", color=2, delay=.6, life=1.6, count=14, radius=260, size=1.0, seed=3),
        P("pillar", color=0, delay=.8, life=.8, size=.6),
        P("sprites", tex="luz", delay=.8, life=.7, count=6, radius=150, size=1.2, rise=120, spread=.4),
    ]},
    # Gênese: o começo do mundo: a coluna de luz atrás, três feixes de cores do mundo na linha, as pedras de luz que saem
    # do chão na trilha, a runa e a coluna no fim
    "espada_divino_5": {"colors": [[1, .95, .75], [.6, .85, 1], [1, .6, .85], [.7, 1, .7]], "parts": [
        P("orbs", color=0, delay=.1, life=.7, count=12, radius=240, size=.8, style=1, converge=1),
        P("beam", color=0, delay=.62, life=.7, size=2.5, radius=380, long=1, count=3, gap=.06, spread=10),
        P("spikes", color=3, delay=.62, life=1.0, count=16, radius=650, size=1.0, at="line", seed=10),
        P("ring", color=2, delay=.62, life=.6, count=16, radius=320, size=1.1, style=2, front=150),
        P("glyph", color=1, delay=.6, life=.9, count=12, radius=260, size=1.0, spin=-60, front=150),
        P("pillar", color=1, delay=.95, life=.8, size=.8, style=2, front=1700, radius=200),
        P("sprites", tex="luz", delay=.62, life=.7, count=10, radius=1700, size=1.6, rise=40, at="line", spread=.3),
    ]},

    # =================================== ARCOS ===================================
    # Arco Curto: três riscos de flecha um atrás do outro, uma faísca de corda a cada um
    "arco_comum_1": {"colors": [[.85, .75, .55], [1, 1, .9], [.5, .4, .3]], "parts": [
        P("beam", color=1, delay=.06, life=.5, size=.7, radius=400, count=3, gap=.15),
        P("sprites", tex="faisca", delay=.06, life=.3, count=3, radius=20, size=.6, rise=40, front=80, height=60, spread=.3),
        P("ring", color=2, delay=.4, life=.4, count=8, radius=120, size=.6, front=150),
    ]},
    # Arco do Batedor: o leque de três riscos, a poeira levantada pelo agachar e folhas secas que voam
    "arco_comum_2": {"colors": [[.55, .65, .35], [.95, .95, .8], [.45, .35, .2]], "parts": [
        P("sprites", tex="poeira", life=.7, count=5, radius=80, size=1.2, rise=30, spread=.2),
        P("beam", color=1, delay=.3, life=.45, size=.8, radius=400, count=3, fan=1, spread=24),
        P("sprites", tex="folhas", delay=.3, life=.9, count=4, radius=60, size=1.0, ahead=300, rise=60, front=100, spread=.1),
        P("shards", color=2, delay=.3, life=.6, count=6, size=.5, small=1, front=110, seed=4),
    ]},
    # Arco de Galho Seco: a flecha forte: o risco longo de madeira, as farpas que estalam do arco e a poeira do tranco
    "arco_comum_3": {"colors": [[.6, .45, .28], [.95, .9, .75], [.35, .25, .15]], "parts": [
        P("beam", color=1, delay=.3, life=.55, size=1.4, radius=420, long=1, count=1),
        P("shards", color=0, lit=1, delay=.3, life=.8, count=8, size=.6, style=1, front=60, seed=8),
        P("sprites", tex="poeira", delay=.32, life=.7, count=5, radius=60, size=1.2, out=-100, rise=30, spread=.1),
        P("ring", color=2, delay=.3, life=.4, count=8, radius=110, size=.6, front=90),
    ]},
    # Arco Remendado: a flecha sobe ao céu (o risco para cima) e a chuvinha cai no ponto da mira levantando poeira
    "arco_comum_5": {"colors": [[.7, .62, .45], [1, .95, .8], [.45, .38, .28]], "parts": [
        P("beam", color=1, delay=.55, life=.4, size=.8, radius=260, pitch=60, count=1),
        P("rain", color=0, delay=.85, life=1.6, count=10, radius=320, size=.8, at="aim"),
        P("sprites", tex="poeira", delay=1.0, life=1.2, count=8, radius=300, size=1.2, rise=30, at="aim", spread=1.0),
        P("glyph", color=2, delay=.6, life=1.8, count=8, radius=320, size=.7, spin=30, at="aim"),
    ]},
    # Arco do Salgueiro: as quatro flechas-folha em leque, as folhas que acompanham e as fitas verdes do vento
    "arco_incomum_1": {"colors": LEAF, "parts": [
        P("beam", color=0, delay=.3, life=.5, size=.9, radius=400, count=4, fan=1, spread=30),
        P("sprites", tex="folhas", delay=.3, life=1.0, count=8, radius=120, size=1.1, ahead=500, rise=60, front=100, spread=.15),
        P("feathers", color=1, delay=.2, life=1.2, count=10, radius=200, size=.8, seed=7),
        P("spiral", color=1, life=.7, count=8, size=.6, wide=60, height=200, spin=300),
    ]},
    # Arco de Espinheiro: três flechas de espinho, espinhos escuros que brotam na frente e gotas roxas de veneno
    "arco_incomum_2": {"colors": [[.2, .45, .15], [.7, .3, .95], [.45, .7, .25]], "parts": [
        P("beam", color=2, delay=.06, life=.5, size=.8, radius=400, count=3, gap=.15),
        P("spikes", color=0, lit=1, delay=.1, life=.8, count=8, radius=120, size=.6, style=1, front=110, seed=4),
        P("shards", color=1, delay=.1, life=.8, count=10, size=.5, style=2, front=120, seed=9),
        P("sprites", tex="fumaca", delay=.2, life=.8, count=4, radius=80, size=1.1, rise=20, front=80, spread=.3),
    ]},
    # Arco de Cobre Rubro: a ponta acende em brasa no salto, o risco de fogo e a explosão no alvo (fogo, lascas de cobre,
    # anel de brasa)
    "arco_incomum_3": {"colors": [[1, .45, .15], [1, .8, .35], [.6, .28, .12], [.3, .1, .05]], "parts": [
        P("orbs", color=1, life=.35, count=8, radius=120, size=.5, converge=1),
        P("beam", color=0, delay=.3, life=.4, size=1.0, radius=420, count=1),
        P("sprites", tex="fogo", delay=.5, life=.8, count=10, radius=200, size=2.0, out=150, rise=120, at="aim", spread=.1),
        P("shards", color=2, lit=1, delay=.5, life=1.0, count=12, size=.8, at="aim", seed=5),
        P("ring", color=0, delay=.5, life=.5, count=12, radius=260, size=.9, at="aim"),
        P("sprites", tex="fumaca", delay=.6, life=1.2, count=5, radius=150, size=1.6, rise=80, at="aim", spread=.3),
    ]},
    # Arco da Correnteza: o corpo flui: a água gira em volta, a flecha deixa uma trilha de respingos na linha e o anel de
    # água na frente
    "arco_incomum_4": {"colors": WATER, "parts": [
        P("spiral", color=1, life=.5, count=10, size=.7, wide=70, height=180, spin=-300),
        P("beam", color=1, delay=.3, life=.55, size=1.4, radius=420, long=1, count=1),
        P("sprites", tex="agua", delay=.3, life=.6, count=10, radius=1400, size=1.4, rise=120, gravity=400, at="line", spread=.4),
        P("ring", color=2, delay=.3, life=.5, count=10, radius=140, size=.8, style=1, front=120),
    ]},
    # Arco do Caçador: a marca do caçador no alvo, a flecha que sobe e a chuva de flechas com a poeira das flechadas
    "arco_incomum_5": {"colors": [[.6, .5, .3], [1, .95, .8], [.8, .3, .15]], "parts": [
        P("glyph", color=2, delay=.2, life=2.2, count=10, radius=380, size=.8, spin=-40, at="aim"),
        P("beam", color=1, delay=.55, life=.4, size=.9, radius=260, pitch=60, count=1),
        P("rain", color=0, delay=.85, life=1.7, count=16, radius=380, size=.9, at="aim"),
        P("sprites", tex="poeira", delay=1.0, life=1.4, count=10, radius=360, size=1.2, rise=30, at="aim", spread=1.2),
    ]},
    # Arco da Névoa Azul: a névoa que sai do arco, a runa d'água no alvo envolta em névoa e o estouro de água 1,5 s depois
    "arco_raro_1": {"colors": [[.4, .65, .9], [.85, .95, 1], [.15, .35, .7]], "parts": [
        P("sprites", tex="fumaca", life=.8, count=5, radius=90, size=1.3, rise=20, height=60, spread=.3),
        P("glyph", color=0, delay=.4, life=1.5, count=10, radius=300, size=1.0, spin=70, at="aim"),
        P("sprites", tex="fumaca", delay=.4, life=1.4, count=8, radius=280, size=1.6, rise=10, at="ring", spread=.6),
        P("sprites", tex="agua", delay=1.82, life=.8, count=12, radius=250, size=2.2, out=150, rise=240, gravity=500, at="aim", spread=.1),
        P("ring", color=1, delay=1.82, life=.5, count=14, radius=300, size=1.0, style=1, at="aim"),
    ]},
    # Arco da Lua Prateada: a meia-lua de prata atrás do arqueiro, o leque de cinco flechas de luz e as luzinhas
    "arco_raro_2": {"colors": [[.8, .85, 1], [1, 1, 1], [1, .9, .6]], "parts": [
        P("slash", color=0, life=.6, sweep=200, radius=180, tilt=80, count=1),
        P("beam", color=1, delay=.3, life=.55, size=.9, radius=420, count=5, fan=1, spread=50),
        P("orbs", color=1, delay=.2, life=.8, count=8, radius=140, size=.5, style=1, follow=1),
        P("sprites", tex="brilho", delay=.3, life=.6, count=6, radius=100, size=1.0, ahead=400, rise=40, front=100, spread=.2),
    ]},
    # Arco do Relâmpago, carregando: o raio cai no arco, as faíscas na flecha
    "arco_raro_3_carga": {"colors": STORM, "parts": [
        P("bolt", color=1, life=.35, size=.9, hero=1, height=600, seed=7),
        P("sprites", tex="faisca", life=.45, count=6, radius=40, size=.8, rise=60, front=70, height=60, spread=.4),
        P("orbs", color=0, life=.6, count=8, radius=160, size=.45, converge=1),
    ]},
    # Arco do Relâmpago, soltando: o risco do raio, o clarão no arco e o raio que cai no alvo com o anel
    "arco_raro_3": {"colors": STORM, "parts": [
        P("beam", color=1, life=.4, size=1.4, radius=420, count=1),
        P("sprites", tex="luz", life=.35, count=2, radius=20, size=1.4, front=80, height=60, spread=.02),
        P("bolt", color=0, delay=.2, life=.4, size=1.1, at="aim", seed=12),
        P("sprites", tex="faisca", delay=.22, life=.5, count=8, radius=160, size=1.1, rise=150, at="aim", spread=.1),
        P("ring", color=2, delay=.22, life=.45, count=10, radius=260, size=.9, at="aim"),
    ]},
    # Arco da Geada: o hálito de gelo, a lança de gelo e uma trilha de espinhos de gelo brotando na linha
    "arco_raro_4": {"colors": ICE, "parts": [
        P("sprites", tex="gelo", life=.5, count=3, radius=30, size=.8, rise=30, front=40, height=90, spread=.2),
        P("beam", color=1, delay=.3, life=.55, size=1.5, radius=420, long=1, count=1),
        P("spikes", color=3, lit=1, delay=.32, life=1.2, count=14, radius=450, size=.8, style=1, at="line", seed=6),
        P("shards", color=0, delay=.35, life=.8, count=8, size=.6, style=1, front=200, seed=3),
    ]},
    # Arco de Jade: três flechas de jade, os orbes de jade subindo e a espiral verde da pose final
    "arco_raro_5": {"colors": [[.25, .8, .55], [.7, 1, .8], [.1, .5, .35]], "parts": [
        P("beam", color=0, delay=.06, life=.5, size=.9, radius=400, count=3, gap=.15),
        P("orbs", color=1, delay=.1, life=1.0, count=8, radius=140, size=.6, style=1, follow=1, spin=250),
        P("spiral", color=2, delay=.45, life=.8, count=10, size=.7, wide=80, height=220, spin=280),
        P("sprites", tex="brilho", delay=.5, life=.6, count=4, radius=80, size=1.0, rise=60, spread=.3),
    ]},
    # Arco Ametista: os cristais roxos se formam e sobem em volta, ficam girando; o anel e os brilhos
    "arco_epico_1": {"colors": [[.62, .32, .95], [.92, .68, 1], [.35, .12, .6]], "parts": [
        P("shards", color=1, delay=.3, life=1.0, count=10, size=.8, style=1, seed=33),
        P("spikes", color=0, lit=1, delay=.35, life=.9, count=8, radius=160, size=.9, style=1, at="ring", seed=2),
        P("orbs", color=1, delay=.45, life=1.3, count=12, radius=200, size=.7, follow=1, spin=-280),
        P("ring", color=2, delay=.45, life=.6, count=14, radius=260, size=.9, style=1),
        P("sprites", tex="brilho", delay=.45, life=.6, count=6, radius=140, size=1.0, rise=90, spread=.3),
    ]},
    # Arco da Noite Violeta: a runa da noite gira no alvo, os orbes escuros caem nela e explode em coluna violeta e fumaça
    "arco_epico_2": {"colors": VOID, "parts": [
        P("glyph", color=0, delay=.4, life=1.5, count=12, radius=340, size=1.0, spin=-110, style=1, at="aim"),
        P("orbs", color=1, delay=.6, life=1.2, count=10, radius=340, size=.7, style=2, converge=1),
        P("pillar", color=0, delay=1.82, life=.8, size=.7, at="aim"),
        P("sprites", tex="fumaca", delay=1.82, life=1.0, count=10, radius=240, size=1.8, out=150, rise=60, at="aim", spread=.1),
        P("shards", color=3, delay=1.82, life=.9, count=10, size=.8, style=3, at="aim", seed=4),
    ]},
    # Sussurro da Lua Sangrenta: a lua de sangue atrás no salto, a flecha e a explosão em estrelas vermelhas no alvo
    "arco_epico_3": {"colors": [[.85, .08, .15], [1, .45, .45], [.35, 0, .08], [1, .85, .85]], "parts": [
        P("slash", color=0, delay=.05, life=.6, sweep=220, radius=200, tilt=80, count=1),
        P("beam", color=1, delay=.3, life=.4, size=1.0, radius=420, count=1),
        P("shards", color=0, delay=.5, life=1.1, count=18, size=.8, style=2, at="aim", seed=8),
        P("sprites", tex="brilho", delay=.5, life=.7, count=10, radius=300, size=1.5, out=200, rise=100, at="aim", spread=.1),
        P("ring", color=2, delay=.5, life=.5, count=14, radius=380, size=1.0, at="aim"),
        P("orbs", color=3, delay=.5, life=.8, count=10, radius=300, size=.6, at="aim"),
    ]},
    # Arco Fragmentado: cinco riscos azuis rápidos, um raio pequeno em cima do arqueiro e faíscas
    "arco_epico_4": {"colors": STORM, "parts": [
        P("beam", color=0, delay=.06, life=.7, size=.9, radius=400, count=5, gap=.12),
        P("bolt", color=1, delay=.1, life=.4, size=.6, hero=1, height=300, seed=3),
        P("sprites", tex="faisca", delay=.06, life=.7, count=8, radius=40, size=.8, rise=80, front=80, height=60, spread=.6),
        P("shards", color=2, delay=.2, life=.8, count=10, size=.5, style=1, front=150, seed=6),
    ]},
    # Arco do Eclipse: o eclipse no chão embaixo do arqueiro, a flecha escura enorme e o anel escuro na frente
    "arco_epico_5": {"colors": VOID, "parts": [
        P("glyph", color=2, life=1.0, count=12, radius=200, size=1.0, spin=-90, style=1),
        P("orbs", color=1, life=.4, count=10, radius=180, size=.5, style=2, converge=1),
        P("beam", color=3, delay=.3, life=.6, size=2.4, radius=420, long=1, count=1),
        P("beam", color=0, delay=.32, life=.5, size=1.2, radius=420, long=1, count=1),
        P("ring", color=1, delay=.3, life=.5, count=12, radius=180, size=1.0, style=2, front=150),
    ]},
    # Arco de Magma Rúnico: a flecha cai no chão da mira, as runas acendem, o chão racha em brasa e a muralha de magma sobe
    "arco_lendario_1": {"colors": FIRE, "parts": [
        P("beam", color=1, delay=.3, life=.35, size=.9, radius=400, count=1),
        P("glyph", color=0, delay=.45, life=1.2, count=10, radius=420, size=.9, spin=30, at="aim"),
        P("cracks", color=0, delay=.5, life=1.6, count=8, radius=420, size=1.5, at="aim", seed=6),
        P("pillar", color=2, delay=.55, life=1.3, count=9, size=.8, wall=1, at="aim", radius=400),
        P("sprites", tex="fogo", delay=.55, life=1.3, count=12, radius=400, size=2.2, rise=60, wall=1, at="aim", spread=.25),
        P("rain", color=1, delay=.6, life=1.6, count=14, radius=380, size=.8, up=1, at="aim"),
    ]},
    # Arco do Grifo Dourado: as penas douradas do salto, o leque de sete flechas e o anel dourado no pouso
    "arco_lendario_2": {"colors": [[1, .78, .3], [1, .95, .7], [.75, .5, .15]], "parts": [
        P("feathers", color=0, life=1.4, count=18, radius=260, size=1.1, seed=5),
        P("beam", color=1, delay=.3, life=.55, size=1.0, radius=420, count=7, fan=1, spread=60),
        P("sprites", tex="brilho", delay=.3, life=.6, count=6, radius=100, size=1.0, ahead=400, rise=40, front=100, spread=.2),
        P("ring", color=2, delay=.6, life=.5, count=14, radius=260, size=.9, style=1),
    ]},
    # Arco da Fênix: as asas de fogo abrem, as brasas juntam à frente, as penas de fogo e o estouro de fogo onde a bola
    # cai
    "arco_lendario_3": {"colors": FIRE, "parts": [
        P("feathers", color=1, life=1.2, count=16, radius=220, size=1.1, seed=12),
        P("orbs", color=0, life=.35, count=10, radius=200, size=.6, converge=1),
        P("sprites", tex="fogo", delay=.25, life=.6, count=6, radius=60, size=1.6, ahead=300, rise=40, front=100, height=80, spread=.1),
        P("sprites", tex="fogo", delay=.75, life=.9, count=12, radius=300, size=2.4, out=200, rise=150, at="aim", spread=.1),
        P("ring", color=2, delay=.75, life=.5, count=14, radius=300, size=1.0, at="aim"),
        P("shards", color=1, delay=.75, life=1.0, count=12, size=.8, at="aim", seed=3),
    ]},
    # Arco do Sol Nascente: o sol que nasce sobre o arco, a flecha de luz e a explosão de luz no alvo (clarão, anel,
    # coluna)
    "arco_lendario_4": {"colors": HOLY, "parts": [
        P("orbs", color=1, life=.6, count=10, radius=120, size=.6, style=1, follow=1, spin=200),
        P("beam", color=0, delay=.3, life=.45, size=1.2, radius=420, count=1),
        P("sprites", tex="luz", delay=.5, life=.6, count=4, radius=100, size=2.6, at="aim", height=80, spread=.08),
        P("ring", color=1, delay=.5, life=.6, count=16, radius=420, size=1.1, style=2, at="aim"),
        P("pillar", color=2, delay=.5, life=.8, size=.8, at="aim"),
        P("feathers", color=1, delay=.55, life=1.2, count=10, radius=300, size=.8, at="aim", seed=8),
    ]},
    # Arco do Bosque Ancestral: a flecha sobe, a runa verde no alvo, a chuva de galhos (madeira com luz e sombra), os
    # galhos que ficam cravados e as folhas
    "arco_lendario_5": {"colors": [[.45, .3, .15], [.4, .9, .35], [.3, .2, .1]], "parts": [
        P("beam", color=1, delay=.55, life=.4, size=1.0, radius=260, pitch=60, count=1),
        P("glyph", color=1, delay=.6, life=2.0, count=12, radius=520, size=1.0, spin=-30, at="aim"),
        P("rain", color=0, lit=1, delay=.85, life=1.8, count=24, radius=520, size=1.1, at="aim"),
        P("spikes", color=2, lit=1, delay=1.1, life=1.6, count=14, radius=480, size=.8, style=1, at="aim", seed=5),
        P("sprites", tex="folhas", delay=.9, life=1.6, count=12, radius=480, size=1.3, height=300, rise=-40, at="aim", spread=1.0),
    ]},
    # Arco de Braços Gêmeos: seis riscos em pares, dois raios gêmeos e as faíscas de cada par
    "arco_unico_1": {"colors": STORM, "parts": [
        P("beam", color=0, delay=.06, life=.75, size=.9, radius=400, count=6, gap=.1),
        P("bolt", color=1, delay=.1, life=.3, size=.6, hero=1, height=280, seed=1),
        P("bolt", color=2, delay=.35, life=.3, size=.6, hero=1, height=280, seed=2),
        P("sprites", tex="faisca", delay=.06, life=.7, count=8, radius=40, size=.8, rise=80, front=80, height=60, spread=.6),
    ]},
    # Arco do Espelho: dois cortes de espelho cruzados atrás, o leque de seis flechas para os dois lados e cacos de espelho
    "arco_unico_2": {"colors": [[.8, .7, 1], [1, 1, 1], [.5, .4, .8]], "parts": [
        P("slash", color=1, life=.5, sweep=160, radius=200, tilt=60, count=2),
        P("beam", color=0, delay=.3, life=.55, size=1.0, radius=420, count=6, fan=1, spread=70),
        P("shards", color=1, delay=.3, life=1.0, count=14, size=.9, style=3, front=120, seed=9),
        P("sprites", tex="brilho", delay=.35, life=.6, count=8, radius=200, size=1.0, rise=60, front=150, spread=.3),
    ]},
    # Arco da Raiz Primeva: a flecha no chão, a runa verde, e a raiz que rompe o chão do alvo (raízes grossas, rachaduras,
    # folhas)
    "arco_unico_3": {"colors": [[.42, .28, .15], [.4, .9, .35], [.28, .18, .1], [.62, .48, .3]], "parts": [
        P("beam", color=1, delay=.3, life=.35, size=.9, radius=400, count=1),
        P("glyph", color=1, delay=.4, life=1.5, count=12, radius=420, size=1.0, spin=50, at="aim"),
        P("cracks", color=2, lit=1, delay=1.82, life=1.6, count=9, radius=420, size=1.6, at="aim", seed=13),
        P("spikes", color=0, lit=1, delay=1.82, life=1.4, count=14, radius=320, size=1.8, style=1, at="aim", seed=3),
        P("spikes", color=3, lit=1, delay=1.95, life=1.2, count=10, radius=180, size=1.2, style=1, at="aim", seed=9),
        P("sprites", tex="folhas", delay=1.85, life=1.2, count=10, radius=300, size=1.3, out=150, rise=180, gravity=200, at="aim", spread=.1),
    ]},
    # Arco Sem Sombra: a flecha de luz que vai muito longe (dois feixes finos e longos), a luz que fica no caminho e os
    # cacos brancos
    "arco_unico_4": {"colors": [[1, .97, .85], [1, 1, 1], [.9, .85, .6]], "parts": [
        P("beam", color=1, delay=.3, life=.8, size=.9, radius=700, long=1, count=1),
        P("beam", color=2, delay=.33, life=.7, size=1.8, radius=700, long=1, count=1),
        P("sprites", tex="luz", delay=.3, life=.8, count=10, radius=4000, size=1.2, rise=0, at="line", spread=.5),
        P("shards", color=1, delay=.3, life=.8, count=8, size=.6, style=3, front=80, seed=2),
    ]},
    # Arco do Viajante Perdido: o fôlego frio: a runa azul-clara embaixo, a espiral de flocos, os flocos caindo devagar
    # e o brilho
    "arco_unico_5": {"colors": [[.65, .85, 1], [.95, 1, 1], [.4, .65, .95]], "parts": [
        P("glyph", color=0, life=1.4, count=10, radius=240, size=.9, spin=30),
        P("spiral", color=1, delay=.3, life=1.2, count=12, size=.7, wide=80, height=280, spin=150),
        P("feathers", color=1, delay=.4, life=1.6, count=12, radius=240, size=.7, seed=6),
        P("sprites", tex="brilho", delay=.6, life=.7, count=6, radius=120, size=1.0, rise=100, spread=.4),
    ]},
    # Asas da Aurora: as asas abrem em penas da aurora, o leque de nove flechas de luz, o anel e o clarão
    "arco_divino_1": {"colors": [[1, .6, .5], [1, .95, .7], [.55, .9, .85], [1, .5, .8]], "parts": [
        P("feathers", color=3, life=1.4, count=20, radius=280, size=1.2, seed=1),
        P("feathers", color=2, delay=.1, life=1.3, count=14, radius=220, size=1.0, seed=2),
        P("beam", color=1, delay=.3, life=.6, size=1.1, radius=420, count=9, fan=1, spread=70),
        P("ring", color=0, delay=.3, life=.6, count=16, radius=300, size=1.0, style=2),
        P("sprites", tex="luz", delay=.3, life=.5, count=3, radius=40, size=1.8, front=100, height=80, spread=.05),
    ]},
    # Arco da Árvore do Mundo: a flecha reta ao céu, a runa da árvore no alvo, a coluna verde, a chuva imensa de flechas,
    # as raízes e as folhas
    "arco_divino_2": {"colors": [[.4, .85, .35], [.85, 1, .6], [.42, .28, .15], [.2, .55, .2]], "parts": [
        P("beam", color=1, delay=.55, life=.5, size=1.2, radius=300, pitch=80, count=1),
        P("glyph", color=0, delay=.6, life=2.2, count=16, radius=600, size=1.2, spin=25, style=2, at="aim"),
        P("pillar", color=0, delay=.7, life=1.0, size=.9, style=2, at="aim", radius=300),
        P("rain", color=1, delay=.9, life=1.8, count=30, radius=600, size=1.0, at="aim"),
        P("spikes", color=2, lit=1, delay=1.0, life=1.6, count=16, radius=540, size=1.2, style=1, at="aim", seed=4),
        P("sprites", tex="folhas", delay=.9, life=1.8, count=14, radius=560, size=1.4, height=320, rise=-40, at="aim", spread=1.2),
    ]},
    # Arco das Estrelas: a estrela que sobe (o feixe ao céu), as estrelas que descem em volta, os orbes de estrela e o
    # brilho
    "arco_divino_3": {"colors": [[1, .92, .55], [1, 1, .92], [.55, .65, 1]], "parts": [
        P("beam", color=1, delay=.45, life=.5, size=1.1, radius=300, pitch=75, count=1),
        P("rain", color=0, delay=.7, life=1.5, count=16, radius=400, size=.8),
        P("orbs", color=0, delay=.6, life=1.5, count=14, radius=220, size=.7, style=1, follow=1, spin=-250),
        P("glyph", color=2, delay=.6, life=1.2, count=12, radius=240, size=.9, spin=100, style=3),
        P("sprites", tex="brilho", delay=.7, life=.7, count=8, radius=300, size=1.1, rise=-30, height=200, spread=.6),
    ]},
    # Arco do Primeiro Rio: a puxada junta a água em volta, a flecha imensa abre um rio que corre na linha (a onda) e os
    # respingos
    "arco_divino_4": {"colors": WATER, "parts": [
        P("spiral", color=1, life=.5, count=12, size=.8, wide=90, height=200, spin=-350),
        P("beam", color=2, delay=.3, life=.6, size=2.0, radius=420, long=1, count=1),
        P("wave", color=0, delay=.3, life=1.6, length=2400, width=300, speed=2200, height=140, lit=1),
        P("sprites", tex="agua", delay=.3, life=.6, count=8, radius=80, size=2.0, rise=200, gravity=500, front=120, spread=.1),
    ]},
    # Arco do Vendaval Eterno: o giro junta o vento, a flecha, e no alvo um tornado de folhas e vento com o anel
    "arco_divino_5": {"colors": [[.65, .95, .75], [.92, 1, .95], [.35, .7, .4], [.5, .8, .3]], "parts": [
        P("spiral", color=1, life=.4, count=10, size=.7, wide=70, height=200, spin=600),
        P("beam", color=0, delay=.3, life=.4, size=1.0, radius=420, count=1),
        P("spiral", color=1, delay=.5, life=1.6, count=18, size=1.1, wide=160, height=420, spin=700, flare=1.5, at="aim"),
        P("sprites", tex="folhas", delay=.5, life=1.4, count=14, radius=300, size=1.5, out=400, rise=250, at="aim", spread=.6),
        P("ring", color=0, delay=.5, life=.7, count=18, radius=560, size=1.1, style=2, at="aim"),
        P("sprites", tex="poeira", delay=.5, life=1.2, count=10, radius=400, size=1.8, out=200, rise=60, at="ring", spread=.3),
    ]},
}
