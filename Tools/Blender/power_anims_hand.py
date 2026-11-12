# As animações dos poderes feitas à mão, uma por arma (07/11, o usuário: "BEM detalhado cada habilidade, uma por uma").
# Lido pelo Tools/Blender/MakePowerAnims.py (exec): HAND_DATA = {"espada_<chave>" | "arco_<chave>": (trechos, camadas)}
# e, nas de segurar e soltar (overload), "<...>_carga" = a pose de carregar em laço.
#
# trechos: [(clipe, de, até, segundos)] emendados; camadas: {nome: [(fração do tempo, valor)]} (ver build()).
# O instante do golpe de cada uma casa com o poder (VortexWeaponSpecials: o "delay" dele ou o tempo fixo do tipo):
#   dash = na hora (o salto sai em 0 s); flurry = 0,1 s + um a cada (tempo / golpes); sweep, shockwave, holyslam = 0,62 s;
#   reap = 0,3 s; whirl = 0,3 s + a cada (tempo - 0,2) / golpes; runeseal = 0,32 s (o tiro; explode "delay" depois);
#   orbit = 0,45 s; firewall = 0,55 s; groundslam = o "delay"; arcos: volley, pierce, explosive e fireball = 0,3 s;
#   arrowrain = 0,55 s; burst3 = 0,06 s + a cada (tempo / flechas); heal = na hora.

DR, RL, AIM = ("Arco_Draw", 0, 1), ("Arco_Release", 0, 1), ("Arco_Aim", .3, .5)


def shots(n, each, end):
    """n puxadas e solturas rápidas (as rajadas) e o fim; each = segundos de cada uma."""
    segs = []
    for _ in range(n):
        segs += [("Arco_Draw", .55, 1, each * .55), ("Arco_Release", 0, .7, each * .45)]
    return segs + [end]


HAND_DATA = {
    # =================================== ESPADAS ===================================
    # Espada de Treino (dash, sai em 0 s): a estocada de esgrima de aluno: braço da espada esticado, corpo baixo e reto,
    # o pé de trás arrasta; volta para a guarda de treino
    "espada_comum_2": ([("Atk_Espada_2", .3, .55, .12), ("Atk_Adagas_3", .35, .8, .3), ("Atk_Espada_1", .75, 1, .2), ("Idle", 0, .3, .25)], {
        "lean": [(0, 12), (.15, 26), (.45, 22), (.7, 5), (1, 0)], "push": [(0, .05), (.15, .3), (.45, .26), (.7, 0), (1, 0)],
        "lift": [(0, -.16), (.45, -.14), (.75, 0)], "twist": [(0, -30), (.2, 8), (.6, 0), (1, 0)], "armr": [(0, 15), (.15, 55), (.5, 45), (.75, 0)]}),
    # Agulha do Viajante (flurry 5, golpes em .1 .34 .58 .82 1.06): cinco agulhadas, um passinho para a frente a cada uma,
    # e no fim a reverência do viajante
    "espada_comum_3": ([("Atk_Espada_2", .2, .4, .1)] + [("Atk_Adagas_3" if i % 2 else "Atk_Adagas_1", .3, .7, .24) for i in range(5)] +
                       [("Emote_Reverencia", .15, .45, .45)], {
        "lean": [(0, 8), (.1, 18), (.78, 16), (.9, -5), (.95, 20), (1, 0)], "push": [(0, 0)] + [(.06 + i * .155, .06 * (i + 1)) for i in range(5)] + [(.85, 0)],
        "armr": [(0, 0)] + [(.06 + i * .155 + .05, 50 if i % 2 == 0 else 25) for i in range(5)] + [(.82, 0)],
        "lift": [(0, -.08), (.8, -.1), (1, 0)], "twist": [(i * .155, (-10 if i % 2 else 10)) for i in range(6)] + [(1, 0)]}),
    # Velha Companheira (sweep, 0,62 s): a velha espada pesa: puxa para trás devagar, o giro largo, e depois apoia a mão no
    # joelho, cansado, e respira
    "espada_comum_4": ([("Atk_Martelo_2", 0, .45, .42), ("Atk_Espada_3_Full", .3, .7, .3), ("Pegar", .35, .5, .35), ("Idle", .2, .5, .3)], {
        "twist": [(0, 0), (.3, -60), (.45, 50), (.55, 40), (.75, 10), (1, 0)], "spin": [(0, 0), (.3, -30), (.47, 35), (.7, 20), (1, 0)],
        "lift": [(0, 0), (.3, -.12), (.45, -.22), (.7, -.18), (.85, -.08), (1, 0)],
        "lean": [(0, 0), (.3, -12), (.45, 22), (.7, 30), (.82, 28), (1, 0)], "tilt": [(.7, 0), (.78, 4), (.86, -3), (.94, 2), (1, 0)]}),
    # Cutelo do Lenhador (groundslam, 0,7 s): o machadeiro: ergue o cutelo com as duas mãos bem alto, desce como em tora,
    # a lâmina presa no chão, e um tranco para soltar
    "espada_comum_5": ([("Atk_Martelo_1", 0, .45, .38), ("Atk_Martelo_1", .45, .75, .32), ("Pegar", .45, .52, .3), ("Atk_Espada_1", .7, 1, .35)], {
        "arms": [(0, 0), (.2, 80), (.28, 95), (.45, 40), (.52, 0)], "lean": [(0, 0), (.25, -22), (.52, 42), (.72, 38), (.8, 15), (1, 0)],
        "lift": [(0, 0), (.25, .06), (.52, -.26), (.73, -.26), (.85, -.05), (1, 0)], "twist": [(.73, 0), (.77, -18), (.82, 14), (.88, -6), (1, 0)]}),
    # Foice-Verde do Bosque (reap, 0,3 s): o ceifeiro: agacha rente à grama e roda uma volta inteira, a lâmina cortando
    # baixo; levanta com a espada no ombro
    "espada_incomum_1": ([("Atk_Espada_3_Full", 0, 1, .55), ("Atk_Martelo_2", .1, .3, .2), ("Emote_Vitoria", .05, .2, .3)], {
        "lift": [(0, -.1), (.15, -.38), (.5, -.34), (.7, 0)], "spin": [(0, 0), (.1, -25), (.45, 340), (.52, 360), (1, 360)],
        "lean": [(0, 10), (.2, 28), (.5, 20), (.7, -5), (1, 0)], "armr": [(.55, 0), (.7, 50), (1, 0)]}),
    # Espada de Cobre Batido (shockwave, 0,62 s): o ferreiro: duas marteladas curtas de aquecimento e a terceira com tudo,
    # a espada batendo como num malho
    "espada_incomum_2": ([("Atk_Martelo_2", .25, .5, .15), ("Atk_Martelo_2", .25, .5, .15), ("Atk_Martelo_3_Full", .35, .8, .32), ("Jump_Land", .2, 1, .4)], {
        "arms": [(0, 40), (.07, 5), (.14, 45), (.21, 5), (.4, 85), (.53, 0), (1, 0)], "lean": [(0, 8), (.07, 18), (.14, 5), (.21, 18), (.4, -15), (.53, 38), (.75, 25), (1, 0)],
        "lift": [(0, 0), (.4, .08), (.53, -.26), (.8, -.18), (1, 0)], "tilt": [(.53, 0), (.58, 3), (.63, -3), (.7, 0)]}),
    # Ferrão da Noite (dash, 0 s, veneno): o escorpião: a espada por cima da cabeça como um ferrão, o bote de baixo para
    # cima e o recuo agachado
    "espada_incomum_5": ([("Atk_Adagas_2", .1, .5, .15), ("Atk_Adagas_3", .25, .9, .3), ("Walk_Back", .45, 1, .35), ("Idle", 0, .2, .15)], {
        "lift": [(0, -.32), (.2, .04), (.5, -.2), (.75, -.25), (1, 0)], "lean": [(0, 30), (.25, 35), (.5, 15), (.8, 10), (1, 0)],
        "armr": [(0, 85), (.2, 45), (.5, 15), (.8, 30), (1, 0)], "push": [(0, 0), (.2, .32), (.5, .05), (1, 0)], "twist": [(0, 20), (.25, -15), (.6, 0)]}),
    # Safira Gélida (runeseal, tiro 0,32 s): desenha a runa no ar com a ponta (um círculo com o braço), aponta a espada e o
    # selo voa; fica em guarda vendo a runa
    "espada_raro_1": ([("Atk_Cajado_1", 0, .6, .22), ("Atk_Adagas_3", .35, .7, .18), ("Emote_Apontar", .2, .55, .4), ("Idle", 0, .3, .2)], {
        "armr": [(0, 30), (.08, 75), (.16, 50), (.24, 75), (.33, 40), (.6, 30), (1, 0)], "twist": [(0, -30), (.1, 25), (.2, -20), (.3, 10), (1, 0)],
        "lean": [(0, 0), (.3, 16), (.6, 8), (1, 0)], "lift": [(.3, 0), (.4, -.1), (.8, -.06), (1, 0)]}),
    # Lua Crescente (whirl 2 s, 4 golpes): a dança da lua: dois giros, um salto girando no meio, e termina arqueada para
    # trás com a espada no alto, como a lua crescente
    "espada_raro_2": ([("Atk_Espada_3_Full", 0, 1, .55), ("UnarmedJump", .25, .65, .5), ("Atk_Espada_3_Full", 0, 1, .55), ("Emote_Vitoria", .1, .3, .45)], {
        "spin": [(0, 0), (.27, 360), (.5, 720), (.78, 1080), (1, 1080)], "lift": [(0, -.06), (.25, 0), (.38, .3), (.5, 0), (.78, -.06), (1, 0)],
        "arms": [(0, 25), (.78, 30), (.88, 85), (1, 40)], "lean": [(0, 8), (.78, 5), (.88, -22), (1, 0)], "wings": [(.3, 0), (.4, 40), (.5, 0)]}),
    # Trovão Partido (overload: segura e solta): soltando, a estocada que dispara o raio, o tranco para trás
    "espada_raro_3": ([("Atk_Adagas_3", .35, 1, .3), ("Walk_Back", .4, .9, .35), ("Idle", 0, .3, .2)], {
        "push": [(0, .15), (.25, -.22), (.65, -.05), (1, 0)], "lean": [(0, 22), (.25, -12), (.6, 0)], "armr": [(0, 60), (.3, 25), (.7, 0)],
        "tilt": [(0, 4), (.08, -4), (.16, 3), (.24, 0)]}),
    "espada_raro_3_carga": ([("Atk_Cajado_3", .05, .3, .5), ("Atk_Cajado_3", .3, .05, .5)], {
        "arms": [(0, 75), (.5, 88), (1, 75)], "lift": [(0, -.1), (.5, -.14), (1, -.1)], "tilt": [(0, 3), (.25, -3), (.5, 3), (.75, -3), (1, 3)]}),
    # Presa da Serpente (flurry 7, a cada 0,17 s): a cobra: corpo baixo balançando de lado a lado em S e sete botes
    "espada_raro_4": ([("Atk_Adagas_3" if i % 2 else "Atk_Adagas_1", .3, .75, .17) for i in range(7)] + [("Walk_Back", .5, 1, .3), ("Idle", 0, .2, .15)], {
        "tilt": [(i / 9, (14 if i % 2 else -14)) for i in range(8)] + [(1, 0)], "lift": [(0, -.22), (.8, -.2), (1, 0)],
        "push": [(0, 0)] + [(i / 9 + .05, .14 if i % 2 == 0 else .02) for i in range(7)] + [(.85, 0)], "lean": [(0, 25), (.8, 22), (1, 0)],
        "armr": [(i / 9 + .04, 45 if i % 2 == 0 else 15) for i in range(7)] + [(.85, 0)]}),
    # Coração de Ametista (orbit, 0,45 s): ergue a espada de pé na frente do rosto (a reza), os braços sobem com os
    # cristais e ele dá uma volta inteira olhando os cristais
    "espada_raro_5": ([("Sacar", .3, .8, .25), ("LevelUp", .1, .6, .55), ("Atk_Cajado_2", .6, 1, .3)], {
        "arms": [(0, 10), (.2, 40), (.4, 85), (.75, 70), (1, 0)], "spin": [(0, 0), (.35, -30), (.85, 360), (1, 360)],
        "look": [(.3, 0), (.45, -25), (.85, -20), (1, 0)], "lift": [(0, 0), (.45, .06), (1, 0)]}),
    # Monolito Desperto (groundslam, 0,75 s): o salto bem alto, cai de ponta com as duas mãos como uma pedra caindo, fica
    # agachado e levanta devagar, pesado
    "espada_epico_1": ([("Jump_Start", 0, 1, .2), ("Jump_Loop", 0, 1, .3), ("Atk_Martelo_3_Full", .45, .8, .25), ("Jump_Land", 0, 1, .35), ("Pegar", .5, 1, .35)], {
        "lift": [(0, 0), (.1, -.16), (.3, .55), (.42, .5), (.52, -.34), (.75, -.3), (1, 0)], "arms": [(0, 0), (.3, 95), (.45, 85), (.52, 0)],
        "lean": [(0, 0), (.3, -15), (.52, 42), (.8, 30), (1, 0)], "tilt": [(.52, 0), (.56, 4), (.6, -4), (.66, 0)]}),
    # Escritura do Eclipse (runeseal, 0,32 s): escreve no ar: a ponta risca três traços (os giros do tronco), a palma
    # empurra a runa e ela voa
    "espada_epico_2": ([("Atk_Cajado_1", 0, .5, .14), ("Habilidade_Corte", .5, .8, .16), ("Atk_Cajado_2", .3, .8, .35), ("Emote_Apontar", .3, .6, .3)], {
        "twist": [(0, -35), (.07, 30), (.14, -25), (.21, 20), (.3, 0), (1, 0)], "armr": [(0, 40), (.05, 80), (.1, 30), (.15, 80), (.2, 40), (.32, 70), (.6, 30), (1, 0)],
        "lean": [(0, -5), (.32, 18), (.7, 5), (1, 0)], "arml": [(.25, 0), (.33, 70), (.6, 40), (1, 0)]}),
    # Lâmina Sanguínea (reap, 0,3 s): o giro rasteiro e depois leva a lâmina à boca, como quem bebe (a vida volta)
    "espada_epico_3": ([("Atk_Espada_3_Full", 0, 1, .48), ("Beber", .25, .6, .55)], {
        "spin": [(0, 0), (.08, -20), (.45, 360), (1, 360)], "lift": [(0, -.15), (.15, -.35), (.45, -.25), (.6, 0)],
        "lean": [(0, 20), (.3, 26), (.5, 5), (.75, -12), (1, 0)], "look": [(.5, 0), (.7, -15), (.9, -10), (1, 0)]}),
    # Fragmento do Vórtice (orbit, 0,45 s): a espada para o céu, o raio desce nela (o corpo treme), e corta para baixo
    # soltando os fragmentos
    "espada_epico_4": ([("LevelUp", 0, .5, .38), ("Grito", .2, .5, .35), ("Atk_Espada_1", .5, 1, .32)], {
        "arms": [(0, 0), (.25, 95), (.62, 90), (.75, 10), (1, 0)], "tilt": [(.3, 0), (.36, 5), (.42, -5), (.48, 5), (.54, -5), (.6, 0)],
        "lift": [(0, 0), (.3, .05), (.62, .08), (.78, -.18), (1, 0)], "lean": [(.62, -10), (.78, 30), (1, 0)]}),
    # Ceifadora do Vazio (whirl 2 s, 5 golpes): o redemoinho que acelera: começa lento e vai girando cada vez mais rápido,
    # flutuando um pouco, e para de uma vez
    "espada_epico_5": ([("Atk_Espada_3_Full", 0, 1, .65), ("Atk_Espada_3_Full", 0, 1, .55), ("Atk_Espada_3_Full", 0, 1, .55), ("Atk_Martelo_2", .6, 1, .35)], {
        "spin": [(0, 0), (.3, 300), (.55, 720), (.83, 1440), (1, 1440)], "lift": [(0, 0), (.2, .08), (.8, .14), (.88, -.18), (1, 0)],
        "lean": [(0, 5), (.8, 12), (.88, 30), (1, 0)], "arms": [(0, 10), (.5, 25), (.8, 30), (1, 0)]}),
    # Suspiro de Azhûr (firewall, 0,55 s): o colosso respira fundo (inclina para trás, peito aberto), varre a espada de lado
    # e grita soltando a muralha
    "espada_lendario_1": ([("Grito", 0, .3, .3), ("Atk_Espada_2", .2, 1, .3), ("Grito", .3, .6, .5), ("Idle", 0, .3, .2)], {
        "lean": [(0, 0), (.22, -22), (.42, 28), (.75, 15), (1, 0)], "twist": [(0, -45), (.25, -50), (.45, 45), (.8, 20), (1, 0)],
        "wings": [(0, 0), (.22, 35), (.4, 0)], "lift": [(.4, 0), (.5, -.12), (.8, -.08), (1, 0)]}),
    # Aurora Eterna (holyslam, 0,62 s): ergue a espada para o nascer do sol, sobe nas pontas, bate no chão e fica de joelho
    "espada_lendario_2": ([("LevelUp", .1, .5, .32), ("Atk_Martelo_1", .4, 1, .32), ("Pegar", .5, .8, .45), ("Idle", 0, .3, .2)], {
        "arms": [(0, 0), (.25, 95), (.42, 80), (.48, 0)], "lift": [(0, 0), (.25, .14), (.48, -.22), (.8, -.2), (1, 0)],
        "lean": [(0, 0), (.25, -20), (.48, 32), (.8, 25), (1, 0)], "look": [(0, 0), (.25, -30), (.45, 10), (1, 0)]}),
    # Asa da Fênix (dash 9 m, 0 s): a fênix: salta de braços abertos como asas, plana pela frente e pousa cortando
    "espada_lendario_3": ([("Jump_Start", .3, 1, .1), ("UnarmedJump", .3, .7, .4), ("Atk_Espada_1", .5, 1, .3), ("Idle", 0, .3, .2)], {
        "lift": [(0, -.2), (.1, .32), (.5, .26), (.65, -.15), (1, 0)], "wings": [(0, 20), (.1, 85), (.5, 80), (.62, 10), (1, 0)],
        "lean": [(0, 15), (.12, 35), (.5, 28), (.65, 15), (1, 0)], "arms": [(.5, 0), (.6, 40), (.75, 0)]}),
    # Sopro de Valdora (sweep, 0,62 s): o vento do vale: dá uma volta inteira juntando o vento e solta no corte largo,
    # inclinado como quem empurra uma ventania
    "espada_lendario_4": ([("Atk_Martelo_2", 0, .4, .2), ("Atk_Espada_3_Full", 0, 1, .48), ("Atk_Espada_1", .7, 1, .3), ("Idle", 0, .3, .2)], {
        "spin": [(0, 0), (.15, -60), (.52, 300), (.65, 360), (1, 360)], "lean": [(0, 0), (.4, 10), (.55, 28), (.75, 20), (1, 0)],
        "lift": [(0, 0), (.3, -.15), (.55, -.2), (1, 0)], "arms": [(.5, 20), (.6, 55), (.8, 20), (1, 0)]}),
    # Coroa do Sol (shockwave, 0,62 s): ergue a espada ao sol, puxa para trás e aponta: o raio de sol sai na linha
    "espada_lendario_5": ([("LevelUp", .1, .45, .32), ("Atk_Espada_1", .4, .8, .3), ("Emote_Apontar", .2, .6, .45), ("Idle", 0, .3, .15)], {
        "arms": [(0, 0), (.25, 95), (.4, 70), (.5, 0)], "armr": [(.5, 0), (.55, 70), (.85, 60), (1, 0)], "lean": [(0, 0), (.25, -18), (.52, 18), (.85, 10), (1, 0)],
        "look": [(0, 0), (.25, -30), (.5, 0)], "push": [(.45, 0), (.55, .08), (.8, .05), (1, 0)]}),
    # Fenda Esmeralda (runeseal 0,32 s, explode 1,1 s depois e prende): crava a ponta no chão e arrasta para a frente
    # abrindo a fenda, solta e pula para trás
    "espada_unico_1": ([("Atk_Martelo_1", .3, .8, .25), ("Pegar", .4, .6, .3), ("Walk_Back", .3, 1, .35), ("Idle", 0, .3, .2)], {
        "lean": [(0, 0), (.2, 38), (.5, 30), (.65, 5), (1, 0)], "lift": [(0, 0), (.2, -.25), (.5, -.22), (.65, 0)],
        "push": [(.2, 0), (.5, .2), (.7, -.1), (1, 0)], "arms": [(0, 30), (.2, 0), (.5, -10), (.65, 0)]}),
    # Lágrima do Inverno (whirl 2 s, 6 golpes, congela): a nevasca: gira três vezes com os braços subindo aos poucos e
    # termina com a espada no alto, girando-a na mão
    "espada_unico_2": ([("Atk_Espada_3_Full", 0, 1, .55), ("Atk_Espada_3_Full", 0, 1, .55), ("Atk_Espada_3_Full", 0, 1, .55), ("LevelUp", .3, .55, .45)], {
        "spin": [(0, 0), (.78, 1080), (1, 1080)], "arms": [(0, 10), (.5, 35), (.78, 60), (.9, 90), (1, 30)],
        "lift": [(0, -.12), (.25, .02), (.5, -.12), (.78, .05), (.9, .1), (1, 0)], "tilt": [(0, 0), (.25, 8), (.5, -8), (.75, 6), (1, 0)]}),
    # Raiz da Primeira Árvore (groundslam, 0,8 s): a reverência à árvore, as duas mãos na espada e crava fundo, devagar;
    # segura enquanto as raízes sobem e puxa a espada de volta
    "espada_unico_3": ([("Emote_Reverencia", .1, .4, .35), ("Atk_Martelo_1", .4, .9, .3), ("Pegar", .5, .5, .5), ("Pegar", .5, 1, .35)], {
        "arms": [(0, 0), (.22, 0), (.3, 85), (.4, 50), (.54, 0)], "lift": [(0, 0), (.3, .05), (.54, -.24), (.85, -.22), (1, 0)],
        "lean": [(0, 15), (.22, 25), (.3, -15), (.54, 40), (.85, 35), (1, 0)], "tilt": [(.6, 0), (.65, 3), (.7, -3), (.75, 2), (.8, 0)]}),
    # Silêncio Branco (dash 10 m, 0 s, some): o iaidō: agachado com a mão na bainha, some (o corpo some no avanço) e
    # aparece lá na frente já de espada estendida, e guarda devagar
    "espada_unico_4": ([("Sacar", .6, .3, .12), ("Atk_Espada_2", .3, .7, .2), ("Atk_Espada_1", .6, 1, .3), ("Sacar", .65, 1, .5)], {
        "lift": [(0, -.3), (.15, -.25), (.35, -.2), (.6, -.1), (1, 0)], "armr": [(0, 0), (.25, 0), (.35, 70), (.6, 60), (.85, 10), (1, 0)],
        "lean": [(0, 25), (.35, 20), (.7, 5), (1, 0)], "twist": [(0, -35), (.35, 25), (.7, 10), (1, 0)]}),
    # Sucata do Andarilho (overload: segura e solta): soltando, o arremesso de ombro, como quem joga a lasca de cristal
    "espada_unico_5": ([("Atk_Martelo_2", .55, 1, .25), ("Atk_Adagas_3", .4, 1, .3), ("Idle", 0, .3, .2)], {
        "twist": [(0, -40), (.25, 35), (.6, 10), (1, 0)], "lean": [(0, -8), (.25, 25), (.6, 10), (1, 0)], "armr": [(0, 90), (.25, 40), (.6, 0)],
        "push": [(0, -.05), (.3, .15), (1, 0)]}),
    "espada_unico_5_carga": ([("Pegar", .4, .55, .5), ("Pegar", .55, .4, .5)], {
        "lift": [(0, -.2), (.5, -.24), (1, -.2)], "lean": [(0, 25), (.5, 30), (1, 25)], "tilt": [(0, 4), (.2, -4), (.4, 4), (.6, -4), (.8, 4), (1, 4)],
        "armr": [(0, 20), (.5, 35), (1, 20)]}),
    # Juramento Celeste (holyslam, 0,62 s): a saudação do cavaleiro (a espada de pé no rosto), ergue ao céu, bate com as
    # duas mãos e se curva
    "espada_divino_1": ([("Emote_Saudar", .1, .4, .28), ("LevelUp", .3, .5, .2), ("Atk_Martelo_1", .45, 1, .34), ("Emote_Reverencia", .5, .8, .4)], {
        "arms": [(0, 0), (.2, 30), (.38, 95), (.5, 0)], "lift": [(0, 0), (.38, .12), (.52, -.28), (.7, -.2), (1, 0)],
        "lean": [(0, 0), (.38, -18), (.52, 34), (.8, 22), (1, 0)], "look": [(.3, 0), (.38, -30), (.5, 10), (1, 0)]}),
    # Tridente do Primeiro Selo (runeseal 0,32 s, explode 0,9 s depois): planta a arma em pé, gira-a na mão, estoca o selo
    # e ergue a arma esperando a água subir
    "espada_divino_2": ([("Atk_Cajado_2", .1, .6, .2), ("Atk_Adagas_3", .3, .8, .2), ("Atk_Cajado_3", .5, 1, .4), ("Idle", 0, .3, .2)], {
        "twist": [(0, -30), (.1, 30), (.2, -20), (.3, 0)], "push": [(.15, 0), (.3, .2), (.5, 0)], "lean": [(0, 0), (.3, 22), (.6, -8), (1, 0)],
        "arms": [(.4, 0), (.6, 85), (.85, 70), (1, 0)]}),
    # Estrela Guia (orbit, 0,45 s): aponta para o céu, desenha um círculo com a espada sobre a cabeça e as estrelas descem
    "espada_divino_3": ([("Emote_Apontar", .1, .5, .3), ("LevelUp", .2, .7, .5), ("Emote_Vitoria", .1, .3, .35)], {
        "armr": [(0, 0), (.2, 100), (.75, 95), (1, 0)], "twist": [(.3, 0), (.42, 30), (.54, -30), (.66, 30), (.75, 0)],
        "look": [(0, 0), (.2, -35), (.75, -30), (1, 0)], "lift": [(0, 0), (.4, .08), (1, 0)]}),
    # Voz do Santuário (heal, na hora): ajoelha com a espada fincada, as mãos juntas na reza, e levanta os braços ao céu
    "espada_divino_4": ([("Pegar", .4, .6, .35), ("Emote_Reverencia", .3, .6, .4), ("LevelUp", .3, .7, .5)], {
        "lift": [(0, 0), (.2, -.18), (.55, -.16), (.75, 0), (1, .03)], "arms": [(0, 0), (.2, 40), (.55, 40), (.8, 95), (1, 60)],
        "look": [(0, 0), (.2, 25), (.55, 20), (.8, -30), (1, 0)], "lean": [(0, 0), (.2, 15), (.55, 10), (.8, -12), (1, 0)]}),
    # Gênese (shockwave, 0,62 s): o começo do mundo: flutua de braços bem abertos, junta tudo no peito e solta o corte
    # imenso para a frente
    "espada_divino_5": ([("LevelUp", 0, .5, .3), ("Atk_Espada_3_Full", .35, .75, .25), ("Atk_Martelo_3_Full", .6, .9, .3), ("Emote_Vitoria", .1, .3, .35)], {
        "lift": [(0, 0), (.25, .3), (.45, .25), (.55, -.15), (.8, -.08), (1, 0)], "wings": [(0, 0), (.25, 85), (.4, 10), (.55, 0)],
        "arms": [(.35, 20), (.45, 80), (.55, 0), (1, 0)], "lean": [(0, 0), (.25, -20), (.55, 35), (.8, 15), (1, 0)]}),

    # =================================== ARCOS ===================================
    # Arco Curto (burst3: 3 flechas em .06 .21 .36): três disparos rápidos dando um passinho para a frente a cada um
    "arco_comum_1": (shots(3, .15, ("Arco_Aim", .3, .5, .4)), {
        "push": [(0, 0), (.2, .05), (.4, .1), (.6, .12), (1, 0)], "lean": [(0, 4), (.6, 8), (1, 0)], "twist": [(0, -6), (.2, 6), (.4, -6), (.6, 0)]}),
    # Arco do Batedor (volley 3, 0,3 s): o batedor agacha atrás de uma moita, puxa e solta varrendo, e levanta olhando
    "arco_comum_2": ([("Arco_Draw", .2, 1, .3), ("Arco_Release", 0, 1, .3), ("Arco_Aim", .3, .5, .3)], {
        "lift": [(0, -.28), (.35, -.25), (.6, -.1), (1, 0)], "twist": [(0, -14), (.35, -12), (.5, 12), (1, 0)], "look": [(.6, 0), (.75, -10), (1, 0)]}),
    # Arco de Galho Seco (pierce, 0,3 s): de joelho, puxa fundo até a orelha, a madeira range, solta e o tranco joga para trás
    "arco_comum_3": ([("Arco_Draw", .1, 1, .3), ("Arco_Release", 0, 1, .35), ("Idle", 0, .3, .3)], {
        "lift": [(0, -.3), (.32, -.3), (.6, -.15), (1, 0)], "push": [(.3, 0), (.42, -.2), (.7, -.05), (1, 0)], "lean": [(0, 8), (.3, 10), (.42, -14), (1, 0)]}),
    # Arco Remendado (arrowrain, 0,55 s): o arco remendado: mira bem alto, o arco treme, solta e se encolhe esperando a chuva
    "arco_comum_5": ([("Arco_Draw", 0, 1, .45), ("Arco_Release", 0, 1, .3), ("Arco_Aim", .5, .7, .35)], {
        "lean": [(0, 0), (.35, -28), (.6, -20), (.8, 0), (1, 0)], "tilt": [(.2, 0), (.27, 3), (.34, -3), (.41, 3), (.48, 0)], "lift": [(.6, 0), (.8, -.1), (1, 0)]}),
    # Arco do Salgueiro (volley 4, 0,3 s): balança como o salgueiro ao vento, puxa e solta virando o corpo devagar
    "arco_incomum_1": ([("Arco_Draw", .1, 1, .3), ("Arco_Release", 0, 1, .35), ("Arco_Aim", .3, .5, .35)], {
        "tilt": [(0, -10), (.3, 6), (.6, -6), (1, 0)], "twist": [(0, -20), (.3, -16), (.6, 18), (1, 0)], "lean": [(0, -5), (.5, 5), (1, 0)]}),
    # Arco de Espinheiro (burst3: 3 em .06 .21 .36): três flechas recuando um passo a cada uma
    "arco_incomum_2": (shots(3, .15, ("Walk_Back", .4, 1, .4)), {
        "push": [(0, 0), (.2, -.06), (.4, -.12), (.6, -.18), (1, 0)], "lift": [(0, -.1), (.6, -.12), (1, 0)], "lean": [(0, 8), (1, 0)]}),
    # Arco de Cobre Rubro (explosive, 0,3 s): um salto para trás e solta no ar, a flecha em brasa
    "arco_incomum_3": ([("Walk_Back", 0, .4, .2), ("Arco_Release", 0, 1, .35), ("Jump_Land", .3, 1, .35)], {
        "lift": [(0, 0), (.2, .22), (.4, .15), (.6, -.12), (1, 0)], "push": [(0, 0), (.3, -.2), (.6, -.25), (1, 0)], "lean": [(0, 0), (.3, -10), (1, 0)]}),
    # Arco da Correnteza (pierce, 0,3 s): a postura larga, o corpo flui da trás para a frente como a água e solta empurrando
    "arco_incomum_4": ([("Arco_Draw", .1, 1, .3), ("Arco_Release", 0, 1, .35), ("Arco_Aim", .3, .5, .3)], {
        "push": [(0, -.12), (.3, .1), (.5, .18), (1, 0)], "lift": [(0, -.15), (.5, -.2), (1, 0)], "twist": [(0, -25), (.35, 10), (1, 0)], "lean": [(0, -8), (.4, 14), (1, 0)]}),
    # Arco do Caçador (arrowrain, 0,55 s): o caçador ajoelha, mira alto, solta e levanta já procurando a presa
    "arco_incomum_5": ([("Arco_Draw", 0, 1, .45), ("Arco_Release", 0, 1, .3), ("Arco_Aim", .3, .6, .4)], {
        "lift": [(0, -.3), (.6, -.28), (.85, 0)], "lean": [(0, 0), (.35, -26), (.6, -18), (.85, 0)], "twist": [(.7, 0), (.82, 20), (.92, -15), (1, 0)]}),
    # Arco da Névoa Azul (runeseal, 0,32 s): puxa devagar com a névoa, solta a runa e fica parado, a mão aberta
    "arco_raro_1": ([("Arco_Draw", .2, 1, .3), ("Arco_Release", 0, 1, .3), ("Atk_Cajado_2", .6, 1, .35)], {
        "lift": [(0, 0), (.45, -.08), (1, 0)], "arml": [(.45, 0), (.6, 60), (.9, 50), (1, 0)], "lean": [(0, 0), (.35, 8), (1, 0)]}),
    # Arco da Lua Prateada (volley 5, 0,3 s): arqueada para trás como a lua crescente, solta o leque e volta
    "arco_raro_2": ([("Arco_Draw", .1, 1, .3), ("Arco_Release", 0, 1, .35), ("Emote_Vitoria", .1, .25, .35)], {
        "lean": [(0, -20), (.3, -25), (.5, -5), (.8, -10), (1, 0)], "twist": [(0, -20), (.3, -20), (.5, 20), (1, 0)], "lift": [(0, .02), (1, 0)]}),
    # Arco do Relâmpago (overload): soltando, o tranco do raio que joga para trás
    "arco_raro_3": ([("Arco_Release", 0, 1, .35), ("Walk_Back", .5, 1, .3), ("Idle", 0, .3, .2)], {
        "push": [(0, .05), (.2, -.25), (.6, -.08), (1, 0)], "lean": [(0, 10), (.2, -15), (.6, 0)], "tilt": [(0, 5), (.06, -5), (.12, 4), (.18, 0)]}),
    "arco_raro_3_carga": ([("Arco_Draw", .75, 1, .5), ("Arco_Draw", 1, .75, .5)], {
        "lean": [(0, 6), (.5, 9), (1, 6)], "tilt": [(0, 3), (.25, -3), (.5, 3), (.75, -3), (1, 3)], "lift": [(0, -.08), (1, -.08)]}),
    # Arco da Geada (pierce, 0,3 s): agacha, puxa com o hálito de gelo, solta e escorrega para trás no gelo
    "arco_raro_4": ([("Arco_Draw", .1, 1, .3), ("Arco_Release", 0, 1, .35), ("Idle", 0, .3, .3)], {
        "lift": [(0, -.2), (.3, -.22), (.6, -.12), (1, 0)], "push": [(.3, 0), (.6, -.3), (1, 0)], "look": [(0, 10), (.3, 0)]}),
    # Arco de Jade (burst3: 3 em .06 .21 .36): o arqueiro marcial, cada flecha com um giro do tronco e a pose final
    "arco_raro_5": (shots(3, .15, ("Emote_Saudar", .1, .4, .45)), {
        "twist": [(0, -25), (.15, 20), (.3, -20), (.45, 15), (.6, 0)], "lift": [(0, -.12), (.5, -.1), (1, 0)], "lean": [(0, 6), (1, 0)]}),
    # Arco Ametista (orbit, 0,45 s): ergue o arco deitado acima da cabeça e os cristais se formam em volta
    "arco_epico_1": ([("LevelUp", 0, .6, .5), ("Arco_Aim", .3, .5, .35)], {
        "arms": [(0, 0), (.4, 90), (.7, 80), (1, 0)], "spin": [(0, 0), (.6, 180), (1, 360)], "look": [(.2, 0), (.4, -25), (.7, 0)]}),
    # Arco da Noite Violeta (runeseal, 0,32 s): mira com o arco deitado de lado, solta e dá meia volta no lugar
    "arco_epico_2": ([("Arco_Draw", .2, 1, .3), ("Arco_Release", 0, 1, .3), ("Arco_Aim", .3, .5, .35)], {
        "tilt": [(0, -25), (.4, -25), (.6, 0)], "spin": [(.5, 0), (.85, 360), (1, 360)], "lift": [(0, -.1), (1, 0)]}),
    # Sussurro da Lua Sangrenta (explosive, 0,3 s): salta alto e solta lá de cima, pousa agachada
    "arco_epico_3": ([("Jump_Start", .3, 1, .12), ("Arco_Release", 0, 1, .35), ("Jump_Land", 0, 1, .4)], {
        "lift": [(0, -.15), (.15, .4), (.4, .38), (.6, -.25), (1, 0)], "lean": [(0, 0), (.3, 18), (.6, 10), (1, 0)], "wings": [(.4, 0), (.6, 30), (1, 0)]}),
    # Arco Fragmentado (burst3: 5 em 0,6 s): cinco disparos rápidos, o corpo tremendo de energia
    "arco_epico_4": (shots(5, .12, ("Arco_Aim", .3, .5, .35)), {
        "tilt": [(i / 10, (4 if i % 2 else -4)) for i in range(10)] + [(1, 0)], "lean": [(0, 6), (1, 0)], "lift": [(0, -.06), (1, 0)]}),
    # Arco do Eclipse (pierce, 0,3 s): agacha, os braços puxam até atrás da cabeça, solta e o tranco arrasta para trás
    "arco_epico_5": ([("Arco_Draw", .05, 1, .3), ("Arco_Release", 0, 1, .4), ("Idle", 0, .3, .3)], {
        "lift": [(0, -.28), (.3, -.3), (.6, -.15), (1, 0)], "push": [(.3, 0), (.45, -.35), (.7, -.2), (1, 0)], "lean": [(0, 15), (.3, 20), (.45, -18), (1, 0)]}),
    # Arco de Magma Rúnico (firewall, 0,55 s): atira para o chão lá na frente, e ergue o punho: a muralha sobe
    "arco_lendario_1": ([("Arco_Draw", 0, 1, .35), ("Arco_Release", 0, 1, .25), ("Grito", .2, .5, .5)], {
        "lean": [(0, 10), (.4, 20), (.6, -10), (1, 0)], "arml": [(.5, 0), (.65, 95), (.9, 90), (1, 0)], "lift": [(.5, 0), (.6, .06), (1, 0)]}),
    # Arco do Grifo Dourado (volley 7, 0,3 s): o salto de asas abertas e o leque solto no alto
    "arco_lendario_2": ([("Jump_Start", .3, 1, .12), ("Arco_Release", 0, 1, .35), ("Jump_Land", 0, 1, .35)], {
        "lift": [(0, -.15), (.15, .35), (.45, .3), (.65, -.15), (1, 0)], "wings": [(0, 30), (.15, 80), (.3, 0), (.6, 60), (1, 0)], "lean": [(0, 10), (1, 0)]}),
    # Arco da Fênix (fireball, 0,3 s): abre os braços como asas, junta à frente e solta a bola de fogo
    "arco_lendario_3": ([("LevelUp", .1, .4, .2), ("Arco_Release", 0, 1, .35), ("Idle", 0, .3, .3)], {
        "wings": [(0, 0), (.2, 85), (.35, 0)], "push": [(.25, -.05), (.4, .2), (.7, 0)], "lean": [(0, -10), (.4, 20), (1, 0)], "lift": [(0, 0), (.2, .08), (.45, -.08), (1, 0)]}),
    # Arco do Sol Nascente (explosive, 0,3 s): ergue o arco do chão ao horizonte como o sol nascendo, e solta
    "arco_lendario_4": ([("Arco_Draw", 0, 1, .3), ("Arco_Release", 0, 1, .35), ("Emote_Vitoria", .1, .25, .35)], {
        "lean": [(0, 25), (.3, 0), (.6, -10), (1, 0)], "lift": [(0, -.25), (.3, 0), (.6, .05), (1, 0)], "look": [(0, 20), (.3, 0)]}),
    # Arco do Bosque Ancestral (arrowrain, 0,55 s): o tiro em arco bem alto e o punho erguido chamando os galhos
    "arco_lendario_5": ([("Arco_Draw", 0, 1, .45), ("Arco_Release", 0, 1, .3), ("Grito", .25, .5, .45)], {
        "lean": [(0, 0), (.38, -32), (.55, -20), (.75, 0)], "arml": [(.55, 0), (.7, 95), (.95, 85), (1, 0)], "lift": [(.6, 0), (.75, .05), (1, 0)]}),
    # Arco de Braços Gêmeos (burst3: 6 em 0,6 s, em pares): os pares, virando de um lado e do outro
    "arco_unico_1": (shots(6, .1, ("Arco_Aim", .3, .5, .3)), {
        "twist": [(0, -18), (.15, -18), (.25, 18), (.4, 18), (.5, -18), (.65, -18), (.75, 0)], "lean": [(0, 5), (1, 0)], "lift": [(0, -.08), (1, 0)]}),
    # Arco do Espelho (volley 6, 0,3 s): vira para um lado, solta, e no reflexo vira para o outro, simétrico
    "arco_unico_2": ([("Arco_Draw", .2, 1, .3), ("Arco_Release", 0, 1, .3), ("Arco_Aim", .3, .5, .35)], {
        "twist": [(0, -35), (.3, -30), (.45, 35), (.7, -20), (1, 0)], "spin": [(.45, 0), (.7, -20), (1, 0)], "tilt": [(0, 8), (.45, -8), (1, 0)]}),
    # Arco da Raiz Primeva (runeseal, 0,32 s): atira no chão e bate o pé: a raiz responde
    "arco_unico_3": ([("Arco_Draw", .2, 1, .3), ("Arco_Release", 0, 1, .25), ("Atk_Martelo_2", .5, .9, .4)], {
        "lean": [(0, 15), (.4, 20), (.7, 5), (1, 0)], "lift": [(.55, 0), (.65, .12), (.75, -.2), (1, 0)], "tilt": [(.75, 0), (.8, 3), (.85, -3), (.9, 0)]}),
    # Arco Sem Sombra (pierce 60 m, 0,3 s): sereno: de pé, reto, puxa calmo, solta sem tranco e respira
    "arco_unico_4": ([("Arco_Draw", .1, 1, .3), ("Arco_Release", 0, 1, .45), ("Idle", 0, .4, .35)], {
        "lean": [(0, 0), (1, 0)], "look": [(.4, 0), (.6, 8), (1, 0)], "lift": [(0, .03), (.4, .03), (1, 0)]}),
    # Arco do Viajante Perdido (heal, na hora): o viajante cansado ajoelha, bebe do cantil e levanta
    "arco_unico_5": ([("Pegar", .4, .55, .3), ("Beber", .2, .7, .6), ("Idle", 0, .4, .3)], {
        "lift": [(0, 0), (.2, -.28), (.65, -.26), (.85, 0)], "lean": [(0, 0), (.2, 12), (.65, 5), (1, 0)], "look": [(.3, 0), (.5, -15), (.7, 0)]}),
    # Asas da Aurora (volley 9, 0,3 s): asas abertas, sobe e solta o leque bem largo
    "arco_divino_1": ([("LevelUp", .1, .4, .2), ("Arco_Release", 0, 1, .35), ("Emote_Vitoria", .1, .3, .35)], {
        "wings": [(0, 0), (.2, 85), (.32, 0), (.7, 70), (1, 0)], "lift": [(0, 0), (.2, .18), (.5, .05), (1, 0)], "twist": [(.3, -25), (.5, 25), (1, 0)]}),
    # Arco da Árvore do Mundo (arrowrain, 0,55 s): atira reto para cima e ajoelha com a mão no chão
    "arco_divino_2": ([("Arco_Draw", 0, 1, .45), ("Arco_Release", 0, 1, .3), ("Pegar", .4, .55, .45)], {
        "lean": [(0, 0), (.38, -40), (.55, -30), (.7, 25), (1, 15)], "lift": [(.55, 0), (.75, -.16), (1, -.1)], "arml": [(.6, 0), (.75, -30), (1, -20)]}),
    # Arco das Estrelas (orbit, 0,45 s): puxa para o céu, solta a estrela para cima e as estrelas descem em volta
    "arco_divino_3": ([("Arco_Draw", 0, 1, .35), ("Arco_Release", 0, 1, .3), ("LevelUp", .4, .7, .35)], {
        "lean": [(0, 0), (.35, -45), (.6, -30), (.8, 0)], "look": [(0, 0), (.35, -35), (.8, -20), (1, 0)], "spin": [(.6, 0), (1, 360)]}),
    # Arco do Primeiro Rio (pierce, 0,3 s): a puxada imensa com as duas pernas abertas, solta e o rio arrasta para trás
    "arco_divino_4": ([("Arco_Draw", 0, 1, .3), ("Arco_Release", 0, 1, .4), ("Idle", 0, .3, .3)], {
        "lift": [(0, -.25), (.3, -.28), (.6, -.12), (1, 0)], "push": [(.3, 0), (.5, -.38), (.8, -.15), (1, 0)], "lean": [(0, -12), (.3, -12), (.45, 15), (1, 0)]}),
    # Arco do Vendaval Eterno (explosive, 0,3 s): gira o corpo juntando o vento e solta no fim do giro
    "arco_divino_5": ([("Arco_Draw", 0, 1, .3), ("Arco_Release", 0, 1, .35), ("Arco_Aim", .3, .5, .3)], {
        "spin": [(0, -360), (.32, 0), (1, 0)], "lift": [(0, -.1), (.32, -.05), (.5, .05), (1, 0)], "lean": [(0, 10), (.32, 5), (.5, -10), (1, 0)]}),
}
