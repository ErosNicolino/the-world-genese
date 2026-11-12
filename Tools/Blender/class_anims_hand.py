# As animações dos 264 poderes de classe (07/11, o usuário: "recrie os poderes de cada classe do mesmo jeito das espadas:
# efeitos únicos e animações próprias"). Lido pelo Tools/Blender/MakePowerAnims.py (exec) com --classes.
#
# MOTIFS: os 25 movimentos base (trechos dos clipes do herói + camadas, como as espadas), cada um com o instante do golpe
# (fração do tempo). CLASS: uma linha por poder, escolhida a dedo: "movimento ritmo lado força [detalhes]".
#   ritmo: multiplica os tempos (0,8 rápido, 1,3 lento); lado: 1 ou -1 (espelha giros e inclinações); força: a amplitude.
#   detalhes: jump (salta no golpe), kneel (fica de joelho depois), spin180/spin360 (gira no todo), float (flutua),
#   wings (abre os braços no golpe), low (corpo baixo), back (recua depois), fwd (avança no golpe), shake (treme no golpe).
# (os agachados no Pegar ficam rasos: o clipe já dobra o corpo; fundo demais amassava)
# O instante do golpe (em segundos) vai para Data/Heroes/humano_Mixamo/classes.json: o poder espera por ele
# (UVortexClassSkills::ServerCast) e o efeito (Tools/Weapons/class_fx_hand.py) também conta dele.


def M_smash(t, s, p):
    return [("Atk_Martelo_1", 0, .45, .36 * t), ("Atk_Martelo_1", .45, 1, .28 * t), ("Jump_Land", .3, 1, .36 * t)], {
        "arms": [(0, 0), (.28, 90 * p), (.42, 0)], "lean": [(0, 0), (.28, -18 * p), (.45, 35 * p), (.7, 20), (1, 0)],
        "lift": [(0, 0), (.28, .08), (.45, -.22 * p), (.75, -.12), (1, 0)]}, .45


def M_sweep(t, s, p):
    return [("Atk_Martelo_2", 0, .45, .3 * t), ("Atk_Espada_3_Full", .25, .7, .3 * t), ("Atk_Espada_1", .7, 1, .35 * t)], {
        "twist": [(0, 0), (.3, -55 * s * p), (.48, 50 * s * p), (1, 0)], "spin": [(0, 0), (.3, -30 * s), (.5, 35 * s), (1, 0)],
        "lift": [(0, 0), (.3, -.15 * p), (.55, -.18 * p), (1, 0)], "lean": [(.3, 0), (.5, 18 * p), (1, 0)]}, .42


def M_thrust(t, s, p):
    return [("Atk_Espada_2", .3, .55, .15 * t), ("Atk_Adagas_3", .3, .8, .3 * t), ("Idle", 0, .3, .35 * t)], {
        "lean": [(0, 10), (.25, 28 * p), (.5, 15), (1, 0)], "push": [(0, 0), (.25, .3 * p), (.5, .15), (1, 0)],
        "armr": [(0, 10), (.25, 60 * p), (.55, 25), (1, 0)], "twist": [(0, -25 * s), (.25, 10 * s), (1, 0)]}, .28


def M_combo(t, s, p):
    segs = [("Atk_Adagas_1" if i % 2 == 0 else "Atk_Adagas_2", .1, .85, .2 * t) for i in range(3)] + [("Atk_Espada_1", .6, 1, .3 * t)]
    return segs, {"twist": [(0, -15 * s), (.22, 15 * s), (.44, -15 * s), (.66, 10 * s), (1, 0)], "lean": [(0, 10), (.66, 15 * p), (1, 0)],
                  "push": [(0, 0), (.22, .08), (.44, .14 * p), (.66, .2 * p), (1, 0)]}, .15


def M_spin(t, s, p):
    return [("Atk_Espada_3_Full", 0, 1, .55 * t), ("Atk_Espada_1", .6, 1, .3 * t)], {
        "spin": [(0, 0), (.1, -25 * s), (.62, 360 * s), (1, 360 * s)], "lift": [(0, -.08), (.2, -.2 * p), (.62, -.15), (.8, 0)],
        "lean": [(0, 10), (.4, 20 * p), (1, 0)], "arms": [(.2, 10), (.5, 25), (1, 0)]}, .35


def M_leap(t, s, p):
    return [("Jump_Start", 0, 1, .18 * t), ("Jump_Loop", 0, 1, .28 * t), ("Atk_Martelo_3_Full", .45, .8, .24 * t), ("Jump_Land", 0, 1, .35 * t)], {
        "lift": [(0, 0), (.12, -.15), (.32, .5 * p), (.48, .45 * p), (.6, -.3 * p), (.8, -.2), (1, 0)], "arms": [(0, 0), (.32, 95), (.5, 80), (.6, 0)],
        "lean": [(.3, -15), (.6, 40 * p), (.85, 25), (1, 0)]}, .6


def M_dash(t, s, p):
    return [("Atk_Espada_2", .3, .6, .12 * t), ("Atk_Adagas_3", .2, .9, .3 * t), ("Atk_Espada_1", .6, 1, .3 * t)], {
        "lean": [(0, 15), (.2, 35 * p), (.55, 20), (1, 0)], "push": [(0, .05), (.2, .3 * p), (.55, .15), (1, 0)],
        "lift": [(0, -.18), (.5, -.12), (.8, 0)], "twist": [(0, -20 * s), (.3, 10 * s), (1, 0)]}, .1


def M_retreat(t, s, p):
    return [("Jump_Start", .4, 1, .12 * t), ("Walk_Back", 0, .6, .35 * t), ("Jump_Land", .2, 1, .35 * t)], {
        "lift": [(0, -.15), (.15, .3 * p), (.45, .2), (.6, -.15), (1, 0)], "push": [(0, 0), (.45, -.3 * p), (1, 0)],
        "lean": [(0, 5), (.3, -20 * p), (.7, 10), (1, 0)]}, .12


def M_stomp(t, s, p):
    return [("Atk_Martelo_2", .15, .4, .3 * t), ("Jump_Land", 0, 1, .45 * t), ("Idle", 0, .3, .25 * t)], {
        "lift": [(0, 0), (.2, .14 * p), (.35, -.28 * p), (.65, -.15), (1, 0)], "lean": [(0, 0), (.2, -12), (.38, 25 * p), (1, 0)],
        "arms": [(0, 0), (.2, 40), (.36, -10), (1, 0)], "tilt": [(.36, 0), (.42, 4 * s), (.48, -4 * s), (.55, 0)]}, .35


def M_roar(t, s, p):
    return [("Grito", 0, .65, .95 * t), ("Idle", 0, .3, .25 * t)], {
        "lean": [(0, 0), (.2, 10), (.35, -22 * p), (.75, -15), (1, 0)], "wings": [(0, 0), (.3, 70 * p), (.75, 60), (1, 0)],
        "lift": [(0, -.1), (.35, .04), (1, 0)], "look": [(.25, 0), (.4, -25 * p), (.8, -15), (1, 0)]}, .35


def M_pray(t, s, p):
    return [("Pegar", .4, .55, .35 * t), ("Emote_Reverencia", .3, .6, .35 * t), ("LevelUp", .3, .7, .5 * t)], {
        "lift": [(0, 0), (.2, -.12 * p), (.5, -.1), (.75, 0)], "arms": [(0, 0), (.2, 35), (.5, 35), (.75, 95 * p), (1, 50)],
        "look": [(0, 0), (.2, 25), (.5, 20), (.75, -30), (1, 0)]}, .58


def M_raise(t, s, p):
    return [("Pegar", .4, .55, .35 * t), ("LevelUp", .1, .7, .6 * t), ("Idle", 0, .3, .2 * t)], {
        "lift": [(0, 0), (.25, -.1 * p), (.6, .08 * p), (1, 0)], "arms": [(0, 0), (.25, -15), (.6, 95 * p), (.85, 70), (1, 0)],
        "lean": [(0, 0), (.25, 25), (.6, -15 * p), (1, 0)]}, .6


def M_push(t, s, p):
    return [("Atk_Cajado_1", 0, .6, .3 * t), ("Atk_Cajado_3", .3, .8, .32 * t), ("Idle", 0, .3, .3 * t)], {
        "arms": [(0, 0), (.25, 20), (.42, 75 * p), (.7, 60), (1, 0)], "push": [(0, -.06), (.3, -.08), (.42, .18 * p), (1, 0)],
        "lean": [(0, -8), (.42, 18 * p), (1, 0)], "twist": [(0, 25 * s), (.42, -5 * s), (1, 0)]}, .42


def M_sky(t, s, p):
    return [("LevelUp", 0, .7, .75 * t), ("Idle", 0, .3, .3 * t)], {
        "arms": [(0, 0), (.45, 100 * p), (.8, 90), (1, 0)], "look": [(0, 0), (.45, -35 * p), (.8, -25), (1, 0)],
        "lift": [(0, 0), (.45, .1 * p), (1, 0)], "lean": [(0, 5), (.45, -15 * p), (1, 0)]}, .48


def M_channel(t, s, p):
    return [("Atk_Cajado_3", .05, .35, .6 * t), ("Atk_Cajado_3", .35, .05, .45 * t), ("Idle", 0, .3, .25 * t)], {
        "arms": [(0, 30), (.3, 75 * p), (.7, 70), (1, 0)], "lift": [(0, 0), (.3, -.1), (.7, -.12), (1, 0)],
        "tilt": [(.3, 0), (.36, 3), (.42, -3), (.48, 3), (.54, -3), (.6, 0)], "lean": [(0, 0), (.3, 12 * p), (1, 0)]}, .3


def M_point(t, s, p):
    return [("Emote_Apontar", .1, .6, .7 * t), ("Idle", 0, .3, .25 * t)], {
        "armr": [(0, 0), (.3, 85 * p), (.75, 75), (1, 0)], "lean": [(0, 0), (.3, 12 * p), (1, 0)],
        "twist": [(0, 20 * s), (.3, -10 * s), (1, 0)], "push": [(.2, 0), (.3, .08 * p), (1, 0)]}, .3


def M_throw(t, s, p):
    return [("Atk_Martelo_2", .4, 1, .25 * t), ("Atk_Adagas_3", .4, 1, .3 * t), ("Idle", 0, .3, .3 * t)], {
        "twist": [(0, -40 * s), (.35, 35 * s * p), (.7, 10 * s), (1, 0)], "lean": [(0, -8), (.35, 25 * p), (1, 0)],
        "armr": [(0, 90), (.35, 30 * p), (.7, 0)], "push": [(0, -.05), (.35, .15 * p), (1, 0)]}, .35


def M_guard(t, s, p):
    return [("Pegar", .3, .45, .3 * t), ("Idle", 0, .3, .55 * t)], {
        "arms": [(0, 0), (.25, 45 * p), (.8, 40), (1, 0)], "lift": [(0, 0), (.25, -.16 * p), (.8, -.14), (1, 0)],
        "lean": [(0, 0), (.25, 12), (.8, 10), (1, 0)], "tilt": [(.25, 0), (.3, 3 * s), (.35, -2 * s), (.4, 0)]}, .25


def M_dance(t, s, p):
    return [("Emote_Vitoria", .05, .3, .4 * t), ("Atk_Espada_3_Full", 0, 1, .45 * t), ("Emote_Saudar", .1, .35, .35 * t)], {
        "spin": [(0, 0), (.3, 0), (.65, 360 * s), (1, 360 * s)], "wings": [(.2, 0), (.45, 70 * p), (.7, 40), (1, 0)],
        "lift": [(0, 0), (.45, .1 * p), (.7, 0)], "tilt": [(0, 0), (.3, 8 * s), (.6, -8 * s), (1, 0)]}, .5


def M_meditate(t, s, p):
    return [("Pegar", .45, .55, .45 * t), ("LevelUp", .2, .5, .5 * t), ("Idle", 0, .3, .25 * t)], {
        "lift": [(0, 0), (.3, -.12 * p), (.6, -.1), (.85, 0)], "arms": [(0, 0), (.3, 30), (.5, 40), (.65, 70 * p), (1, 0)],
        "look": [(0, 0), (.3, 20), (.55, -10), (1, 0)]}, .5


def M_wings(t, s, p):
    return [("LevelUp", .1, .45, .35 * t), ("UnarmedJump", .3, .6, .4 * t), ("Jump_Land", .3, 1, .3 * t)], {
        "wings": [(0, 0), (.3, 85 * p), (.6, 80), (.8, 20), (1, 0)], "lift": [(0, 0), (.3, .2 * p), (.6, .25 * p), (.8, -.1), (1, 0)],
        "lean": [(0, 0), (.4, -12), (.8, 10), (1, 0)]}, .45


def M_punch(t, s, p):
    return [("Boxing", .1, .7, .55 * t), ("Atk_Espada_1", .7, 1, .25 * t)], {
        "lean": [(0, 15), (.5, 20 * p), (1, 0)], "push": [(0, 0), (.2, .1 * p), (.5, .15 * p), (1, 0)],
        "twist": [(0, -15 * s), (.2, 15 * s), (.4, -15 * s), (.6, 10 * s), (1, 0)]}, .18


def M_shot(t, s, p):
    return [("Arco_Draw", .2, 1, .35 * t), ("Arco_Release", 0, 1, .3 * t), ("Arco_Aim", .3, .5, .3 * t)], {
        "lean": [(0, 0), (.38, -6 * p), (1, 0)], "twist": [(0, -15 * s), (.38, -10 * s), (.6, 5 * s), (1, 0)], "lift": [(0, -.05), (1, 0)]}, .4


def M_kneelslam(t, s, p):
    return [("Atk_Martelo_1", .3, .9, .35 * t), ("Pegar", .5, .55, .4 * t), ("Pegar", .55, 1, .3 * t)], {
        "arms": [(0, 30), (.2, 85 * p), (.35, 0)], "lift": [(0, 0), (.2, .06), (.38, -.24 * p), (.75, -.2), (1, 0)],
        "lean": [(0, 0), (.2, -15), (.38, 38 * p), (.75, 30), (1, 0)], "tilt": [(.38, 0), (.43, 3), (.48, -3), (.53, 0)]}, .38


def M_cross(t, s, p):
    return [("Emote_Saudar", .1, .4, .35 * t), ("Grito", .25, .55, .5 * t), ("Idle", 0, .3, .25 * t)], {
        "arms": [(0, 0), (.3, 50), (.45, 0)], "wings": [(.3, 0), (.45, 85 * p), (.8, 60), (1, 0)],
        "lean": [(0, 0), (.3, 15), (.45, -20 * p), (1, 0)], "lift": [(.3, -.1), (.45, .08 * p), (1, 0)]}, .45


def M_twirl(t, s, p):
    return [("Atk_Cajado_1", 0, .5, .3 * t), ("Atk_Espada_3_Full", .1, .9, .35 * t), ("Atk_Cajado_3", .4, .9, .35 * t)], {
        "spin": [(0, 0), (.55, 360 * s), (1, 360 * s)], "arms": [(0, 0), (.3, 40), (.55, 80 * p), (1, 0)],
        "push": [(.45, 0), (.58, .15 * p), (1, 0)], "lift": [(0, 0), (.3, .06), (.55, -.08), (1, 0)]}, .55


MOTIFS = {k[2:]: v for k, v in dict(globals()).items() if k.startswith("M_")}


def mods(layers, hit, words):
    """Os detalhes por cima do movimento (a fração do golpe é hit)."""
    L = dict(layers)
    for w in words:
        if w == "jump": L["lift"] = [(0, 0), (max(0, hit - .2), .05), (hit, .35), (min(1, hit + .25), -.1), (1, 0)]
        elif w == "kneel": L["lift"] = list(L.get("lift", [(0, 0)])[:2]) + [(min(.95, hit + .1), -.15), (.9, -.13), (1, 0)]
        elif w.startswith("spin"): L["spin"] = [(0, 0), (min(.95, hit + .2), float(w[4:])), (1, float(w[4:]))]
        elif w == "float": L["lift"] = [(0, 0), (hit, .18), (.85, .15), (1, 0)]
        elif w == "wings": L["wings"] = [(0, 0), (hit, 80), (min(1, hit + .3), 50), (1, 0)]
        elif w == "low": L["lift"] = [(0, -.15), (.9, -.15), (1, 0)]
        elif w == "back": L["push"] = [(0, 0), (hit, 0), (min(1, hit + .25), -.25), (1, 0)]
        elif w == "fwd": L["push"] = [(0, 0), (hit, .25), (min(1, hit + .3), .1), (1, 0)]
        elif w == "shake": L["tilt"] = [(max(0, hit - .02), 0), (hit + .03, 5), (hit + .08, -5), (hit + .13, 4), (hit + .18, 0)]
    return L


def class_anim(code):
    """'movimento ritmo lado força [detalhes]' -> (trechos, camadas, golpe em fração)."""
    w = code.split()
    t, s, p = float(w[1]), int(w[2]), float(w[3])
    segs, layers, hit = MOTIFS[w[0]](t, s, p)
    return segs, mods(layers, hit, w[4:]), hit


CLASS = {
    # Guerreiro: o golpe pesado de cima, a investida, o grito e o último fôlego em guarda ferida
    "warrior_heavy_blow": "smash 1 1 1.1 shake", "warrior_battle_charge": "dash .9 1 1.2 fwd", "warrior_war_cry": "roar 1 1 1.1",
    "warrior_last_breath": "guard 1.2 -1 1 kneel",
    # Cavaleiro: o escudo empurrado, a intervenção, a postura e o juramento ajoelhado
    "knight_shield_bash": "push .9 1 1.2 fwd", "knight_intervene": "dash 1 -1 1 jump", "knight_defensive_stance": "guard 1 1 1.2",
    "knight_oath_of_protection": "pray 1.1 1 1 kneel",
    # Guardião: a provocação de braços abertos, a muralha, o impacto e o pisão que trava a passagem
    "guardian_taunt": "cross 1 1 1.2", "guardian_living_wall": "guard 1.2 -1 1.3 low", "guardian_protective_impact": "stomp 1 1 1.2",
    "guardian_none_shall_pass": "kneelslam 1.1 -1 1.2",
    # Patrulheiro: a flecha, a marca, o salto para trás e a chuva
    "ranger_piercing_arrow": "shot 1 1 1.1", "ranger_prey_mark": "point .9 -1 1", "ranger_retreat_leap": "retreat 1 1 1.1",
    "ranger_arrow_rain": "shot 1.1 -1 1 float",
    # Duelista: a estocada, o aparo, o passo lateral e o duelo (aponta a espada)
    "duelist_lunge": "thrust .85 1 1.2 fwd", "duelist_parry": "guard .7 1 .8", "duelist_sidestep": "retreat .8 -1 .8 spin90",
    "duelist_deadly_duel": "point 1 1 1.2 shake",
    # Assassino: a punhalada, a bomba de fumaça, a marca e a execução girando
    "assassin_shadow_stab": "thrust .8 -1 1.1 low", "assassin_smoke_bomb": "throw .9 1 1 back", "assassin_executioner_mark": "point .8 1 .9",
    "assassin_execution": "spin .9 -1 1.2 jump",
    # Mago elemental: o projétil, a explosão em volta, a convergência e a tempestade
    "elemental_bolt": "push .9 1 1", "elemental_burst": "kneelslam .9 1 1", "elemental_convergence": "channel 1 1 1.1",
    "elemental_storm": "sky 1.1 1 1.2 float",
    # Arcanista
    "arcane_missile": "push .8 -1 .9", "arcane_containment_rune": "point 1 1 1", "arcane_amplify": "twirl 1 1 1",
    "arcane_field": "raise 1 -1 1.1",
    # Alquimista: os frascos, o elixir (bebe), o composto e o laboratório
    "alchemist_volatile_flask": "throw .9 1 1.1", "alchemist_elixir": "meditate 1 1 .9", "alchemist_unstable_compound": "throw 1 -1 1.2 shake",
    "alchemist_field_lab": "raise 1.1 1 1 low",
    # Clérigo
    "cleric_restoring_light": "pray 1 1 1", "cleric_purify": "sky .9 1 .9", "cleric_holy_symbol": "raise 1 1 1",
    "cleric_miracle": "pray 1.2 -1 1.3 float",
    # Druida
    "druid_binding_roots": "kneelslam 1 1 1", "druid_natural_regrowth": "meditate 1.1 1 1", "druid_feral_form": "roar .9 1 1.3 low",
    "druid_living_forest": "raise 1.2 -1 1.2",
    # Xamã
    "shaman_war_totem": "stomp 1 1 1.1", "shaman_healing_totem": "kneelslam 1.1 -1 .9", "shaman_spirit_call": "raise 1 1 1.1 wings",
    "shaman_ancestor_dance": "dance 1 1 1.1",
    # Feiticeiro
    "sorcerer_mystic_blast": "channel .9 1 1.2", "sorcerer_familiar": "raise .9 1 .9", "sorcerer_curse": "point 1 -1 1.1 shake",
    "sorcerer_grand_conjuration": "sky 1.2 1 1.3 float",
    # Lâmina Arcana
    "arcblade_arcane_cut": "sweep .9 1 1.2", "arcblade_enchant": "channel 1 -1 .9", "arcblade_ethereal_step": "dash .8 1 1",
    "arcblade_transcendent": "cross 1 1 1.2 float",
    # Monge
    "monk_fist_sequence": "punch .8 1 1.1", "monk_repulsing_palm": "push .8 -1 1.3 fwd", "monk_chi_flow": "meditate 1 1 1",
    "monk_thousand_blows": "punch 1.1 -1 1.2 spin360",
    # Lanceiro
    "lancer_pierce": "thrust 1 1 1.3", "lancer_sweep": "spin 1 1 1.1", "lancer_spear_leap": "leap .9 1 1.1",
    "lancer_formation": "stomp 1 -1 1",
    # Bardo
    "bard_war_song": "dance 1 1 1", "bard_restoring_ballad": "dance 1.2 -1 .8", "bard_dissonant_note": "push .9 1 1.1 shake",
    "bard_grand_concert": "twirl 1.1 1 1.2 float",
    # Caçador de Monstros
    "hunter_study_prey": "point 1.1 1 .9 low", "hunter_hunting_bomb": "throw .9 1 1.1", "hunter_prepared_dodge": "retreat .8 -1 .9",
    "hunter_prepared_kill": "smash .9 -1 1.2 jump",
    # Bárbaro
    "barbarian_brutal_axe": "smash .9 1 1.3", "barbarian_war_mist": "stomp .9 -1 1.2", "barbarian_tribal_roar": "roar 1 1 1.3",
    "barbarian_mist_frenzy": "roar .8 -1 1.2 shake",
    # Martelo Rúnico
    "runehammer_seismic": "kneelslam .9 1 1.3", "runehammer_weight_rune": "stomp 1.1 1 1", "runehammer_protective_rune": "guard 1 1 1.1",
    "runehammer_judgement": "leap 1 -1 1.2",
    # Guardião do Musgo
    "moss_bark_skin": "guard 1.2 1 1 low", "moss_living_thorns": "roar 1 -1 .9", "moss_protective_roots": "meditate 1.1 1 1.1",
    "moss_forest_heart": "raise 1.2 1 1.2 kneel",
    # Andarilho do Vento
    "wind_cut": "sweep .8 1 1", "wind_celestial_step": "dash .8 -1 1 jump", "wind_updraft": "twirl .9 1 1.1",
    "wind_storm_dance": "dance .9 -1 1.3",
    # Tecelão de Jade
    "jade_shard": "push .9 1 1", "jade_flower": "meditate 1 -1 1", "jade_vital_link": "channel 1 1 1", "jade_palace": "raise 1.1 1 1.2",
    # Mago da Lua
    "moon_ray": "channel .9 -1 1.1", "moon_night_veil": "cross 1 1 .9", "moon_gravity": "sky 1 -1 1.1", "moon_eclipse": "raise 1.2 1 1.3 float",
    # Domador de Feras
    "tamer_feral_companion": "roar 1 1 1", "tamer_command_attack": "point .8 1 1.2 fwd", "tamer_blood_bond": "cross 1 -1 1",
    "tamer_pack_call": "roar 1.1 -1 1.2 wings",
    # Engenheiro Arcano
    "eng_turret": "kneelslam 1 1 .9", "eng_rune_mine": "throw 1 -1 .9 low", "eng_repair_drone": "push 1 1 .8", "eng_overload": "channel .9 1 1.2 shake",
    # Corsário
    "corsair_pistol_shot": "point .8 1 1.1 back", "corsair_cut": "sweep .9 -1 1.1", "corsair_hook": "throw .9 1 1.2 fwd",
    "corsair_bombardment": "point 1.1 -1 1.1 shake",
    # Pugilista
    "pug_lightning_jab": "punch .7 1 1", "pug_defense_break": "push .8 1 1.3 fwd", "pug_offensive_dodge": "retreat .8 1 1 spin180",
    "pug_relentless_combo": "punch 1 -1 1.3",
    # Caçador de Demônios
    "dh_profane_mark": "point 1 1 1", "dh_consecrated_chain": "throw 1 1 1.1", "dh_demonic_purge": "push 1 -1 1.2",
    "dh_infernal_sentence": "channel 1.1 1 1.3",
    # Místico do Véu
    "veil_duplicate": "retreat .9 -1 .9", "veil_veil": "cross 1 1 .9", "veil_nightmare": "point 1 -1 1 shake", "veil_dream_world": "twirl 1.2 1 1.1 float",
    # Arauto de Guerra
    "herald_war_banner": "kneelslam 1.1 1 1.1", "herald_advance": "point 1 1 1.2 fwd", "herald_defense": "guard 1 -1 1.1",
    "herald_supreme_command": "roar 1.1 1 1.3",
    # Espadachim Carmesim
    "crimson_cut": "sweep .8 1 1.2", "crimson_dance": "dance .9 1 1", "crimson_blood_for_blood": "cross .9 -1 1.1 shake",
    "crimson_bloody_rose": "spin .9 1 1.3 spin720",
    # Destruidor
    "destroyer_demolition": "smash 1 1 1.3 shake", "destroyer_shockwave": "stomp .9 1 1.3", "destroyer_momentum": "roar .9 -1 1.1",
    "destroyer_meteor": "leap .9 1 1.3",
    # Senhor da Guerra
    "warlord_commander_strike": "smash .9 -1 1.2", "warlord_regroup": "roar 1 1 1", "warlord_dominant_presence": "cross 1 1 1.3",
    "warlord_battlefield": "kneelslam 1.1 1 1.2",
    # Colosso
    "colossus_trample": "stomp 1.1 1 1.4", "colossus_stone_body": "guard 1.3 1 1.2 low", "colossus_intercept": "dash 1 1 1.2",
    "colossus_colossal_form": "roar 1.3 -1 1.4",
    # Imortal
    "immortal_persistent_blood": "meditate 1 1 1", "immortal_refuse_death": "guard 1 -1 1.1 kneel", "immortal_eternity_mark": "point 1 1 1",
    "immortal_rebirth": "pray 1.3 1 1.3 float",
    # Fantasma
    "phantom_spectral_pass": "dash .8 -1 1", "phantom_ghost_blade": "sweep .9 -1 1.1", "phantom_haunt": "point 1 1 .9",
    "phantom_thousand_blades": "spin 1 1 1.2 spin720",
    # Cronomante
    "chrono_delay": "point 1.2 1 1", "chrono_haste": "twirl .8 1 1", "chrono_echo": "channel .9 -1 1", "chrono_stop_time": "sky 1.3 1 1.2",
    # Oráculo
    "oracle_omen": "meditate 1.1 1 1", "oracle_fair_fate": "sky 1 -1 1", "oracle_dark_fate": "point 1.1 -1 1.1", "oracle_rewrite_fate": "pray 1.2 1 1.2",
    # Avatar Elemental
    "avatar_elemental_form": "channel 1 1 1.1 float", "avatar_primordial_blast": "kneelslam 1 1 1.3", "avatar_absorption": "meditate .9 -1 1",
    "avatar_manifestation": "roar 1.2 1 1.4 float",
    # Cavaleiro Abissal
    "abyss_cut": "smash 1 -1 1.2", "abyss_void_armor": "guard 1.1 1 1.1", "abyss_chains": "throw 1 1 1.2", "abyss_gate": "raise 1.2 -1 1.3",
    # Sangromante
    "blood_spear": "throw .9 1 1.1", "blood_drain": "channel 1 1 1.2", "blood_crimson_bond": "point 1 -1 1", "blood_sea": "raise 1.2 1 1.2 low",
    # Invocador Astral
    "astral_star_fragment": "sky .9 1 1", "astral_constellation": "twirl 1 1 1", "astral_comet": "point 1.1 1 1.2", "astral_zodiac_portal": "raise 1.2 -1 1.3 float",
    # Mestre das Correntes
    "chains_pull": "throw .9 -1 1.2 back", "chains_shackles": "throw 1 1 1", "chains_protective_link": "guard 1 1 1", "chains_prison": "kneelslam 1.1 1 1.1",
    # Arquimago
    "archmage_superior_magic": "push 1 1 1.3", "archmage_counterspell": "cross .8 1 1", "archmage_arcane_fold": "twirl .8 -1 .9",
    "archmage_grand_ritual": "sky 1.4 1 1.4 float",
    # Cavaleiro do Dragão
    "dragon_charge": "dash 1 1 1.3 jump", "dragon_scales": "guard 1 -1 1", "dragon_breath": "roar .9 1 1.2 fwd", "dragon_awakening": "wings 1.2 1 1.3",
    # Soberano Rúnico
    "sovereign_power_rune": "stomp 1 1 1", "sovereign_ward_rune": "stomp 1 -1 .9", "sovereign_prison_rune": "kneelslam 1 1 1",
    "sovereign_primordial_word": "cross 1.1 1 1.3",
    # Caminhante do Vazio
    "void_step": "dash .8 1 1", "void_dimensional_cut": "sweep .9 1 1.3", "void_linked_portal": "twirl 1 -1 1", "void_spatial_collapse": "point 1.2 1 1.3 shake",
    # Santo Guerreiro
    "saint_radiant_strike": "smash 1 1 1.2", "saint_shield_of_faith": "pray .9 1 1", "saint_martyrdom": "guard 1.1 1 1.3 kneel",
    "saint_celestial_intervention": "sky 1.2 1 1.3 float",
    # Senhor das Tempestades
    "storm_chain_lightning": "push .8 1 1.1 shake", "storm_gust": "sweep .9 1 1", "storm_cloud": "sky 1 -1 1", "storm_wrath_of_heavens": "sky 1.3 1 1.4 float",
    # Arconte Solar
    "solar_cut": "sweep .9 1 1.2", "solar_flash": "cross .8 1 1.2", "solar_aura": "pray 1.1 -1 1", "solar_descending_sun": "sky 1.3 1 1.4",
    # Devorador de Magia
    "devourer_mana_drain": "channel 1 -1 1", "devourer_silence": "point .9 1 1", "devourer_arcane_absorption": "guard 1 1 1", "devourer_null_zone": "stomp 1.1 -1 1.2",
    # Necromante
    "necro_raise_dead": "raise 1 1 1 low", "necro_corpse_explosion": "point .9 -1 1.1", "necro_soul_harvest": "sky 1 1 1.1", "necro_army_of_dead": "raise 1.3 1 1.4 kneel",
    # Herdeiro do Colosso
    "heir_titan_fist": "punch 1 1 1.3", "heir_giant_blood": "roar 1 1 1", "heir_titan_step": "stomp 1.2 -1 1.4", "heir_ancestral_awakening": "roar 1.3 1 1.4 float",
    # Senhor da Matilha
    "pack_call": "roar 1 -1 1", "pack_alpha_mark": "point .9 1 1", "pack_hunting_howl": "roar 1.1 1 1.2 low", "pack_great_hunt": "dash 1 1 1.3",
    # Cartógrafo do Vale
    "carto_eye": "point 1 1 .9", "carto_safe_route": "sky .9 -1 .9", "carto_mark_terrain": "stomp 1 1 .9", "carto_impossible_map": "twirl 1.2 1 1.2 float",
    # Mestre Forjador
    "forge_sharpen": "channel .9 1 1", "forge_reinforce": "guard 1 1 1", "forge_field_forge": "kneelslam 1.1 1 1.2", "forge_masterpiece": "sky 1.1 -1 1.2",
    # Guardião do Santuário
    "sanct_seal": "kneelslam 1.1 1 1", "sanct_absolute_protection": "guard 1.2 1 1.3", "sanct_judgement_of_invader": "point 1 1 1.1", "sanct_living_sanctuary": "pray 1.2 1 1.2",
    # Portador da Lua
    "moonb_crescent": "sweep 1 -1 1.2", "moonb_blessing": "meditate 1 1 1", "moonb_lunar_step": "dash .8 1 1 jump", "moonb_manifested_moon": "sky 1.2 1 1.3 float",
    # Rei sem Coroa
    "king_royal_command": "point 1 1 1.1", "king_authority": "cross 1 1 1.2", "king_rally": "roar 1 -1 1.1", "king_my_kingdom": "kneelslam 1.2 1 1.3",
    # Serafim da Aurora
    "seraph_dawn_light": "push 1 1 1.1", "seraph_celestial_wings": "wings .8 1 1.1 fwd", "seraph_divine_grace": "pray 1 -1 1", "seraph_first_dawn": "wings 1.3 1 1.4",
    # Arauto do Fim
    "end_first_seal": "point 1 1 1", "end_trumpet": "roar 1.1 1 1.3", "end_omen": "point 1.1 -1 1.1 shake", "end_apocalypse": "sky 1.4 1 1.4 float",
    # Vaso do Éter
    "ether_condensed": "push .9 1 1.1", "ether_astral_body": "channel 1 1 .9 float", "ether_transfer": "pray 1 -1 1", "ether_transcendence": "wings 1.2 1 1.2",
    # Soberano Primordial
    "primordial_create": "raise 1.1 1 1.2", "primordial_destroy": "point 1.1 1 1.3 shake", "primordial_balance": "meditate 1.1 1 1.1", "primordial_law": "kneelslam 1.3 1 1.4",
    # Herdeiro do Caos
    "chaos_fragment": "throw .9 -1 1.1", "chaos_rupture": "stomp 1 1 1.2", "chaos_rising_entropy": "channel 1 1 1.1 shake", "chaos_incarnate": "roar 1.2 -1 1.4 float",
    # Avatar do Destino
    "fate_favorable_thread": "twirl 1 1 1", "fate_cut_thread": "sweep .9 1 1.1", "fate_bound_destiny": "guard 1 -1 1.1", "fate_causality": "pray 1.2 1 1.2 float",
}
