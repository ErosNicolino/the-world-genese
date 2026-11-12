"""Os efeitos dos 264 poderes de classe (07/11, o usuário: "recrie os poderes de cada classe do mesmo jeito das espadas:
efeitos únicos para cada"), montados pelo AVortexPowerFx (as mesmas peças das espadas e dos arcos). Lido pelo
Tools/Weapons/BuildPowerFx.py, que escreve as receitas "classe_<id>" no Data/Rpg/power_fx.json.

Uma linha por poder, escolhida a dedo: as peças separadas por ";", cada uma "tipo:chave=valor,marca". O tipo é uma peça
do AVortexPowerFx (slash, ring, glyph, pillar, spikes, shards, orbs, rain, spiral, beam, feathers, bolt, trail, streak,
vanish, wave, cracks) ou "s.<textura>" (as texturas do EffectCraft: agua, poeira, brilho, faisca, fumaca, fogo, folhas,
gelo, luz). Chaves curtas: d = segundos depois do golpe (o instante da animação, Data/Heroes/humano_Mixamo/classes.json;
negativo = antes), t = segundos desde o começo, l = duração, c = cor (0 a 3 da paleta do elemento), f = cm à frente,
r = raio, n = quantidade, sz = tamanho; o resto vai como está (at=aim, sweep, tilt, style, spin, wide, height, pitch...).
Os números podem usar R (o raio do poder em cm), D (a duração), G (o alcance em cm) e L (o atraso do golpe do céu).
"@z" = no centro da zona (na mira, se o poder tem alcance; senão, no herói).
"""

PALETTE = {
    "arcane": [[.55, .35, 1], [.85, .7, 1], [.25, .1, .55], [.4, .8, 1]],
    "holy": [[1, .82, .38], [1, 1, .82], [1, .62, .2], [.95, .9, .7]],
    "fire": [[1, .42, .08], [1, .78, .25], [.85, .12, .03], [.3, .08, .03]],
    "steel": [[.75, .78, .85], [1, 1, 1], [.4, .43, .5], [.9, .75, .4]],
    "blood": [[.75, .05, .1], [1, .35, .35], [.3, 0, .05], [.95, .6, .6]],
    "star": [[1, .92, .55], [1, 1, .92], [.55, .6, 1], [.8, .5, 1]],
    "earth": [[.55, .42, .28], [.8, .68, .45], [.32, .25, .17], [.45, .75, .35]],
    "nature": [[.32, .78, .25], [.72, 1, .4], [.14, .45, .1], [.45, .3, .15]],
    "shadow": [[.4, .18, .7], [.75, .45, 1], [.12, .04, .25], [.05, .02, .08]],
    "wind": [[.7, .95, .85], [.95, 1, .98], [.35, .7, .6], [.85, .95, 1]],
    "time": [[.95, .8, .45], [1, .95, .75], [.5, .4, .75], [.4, .85, .9]],
    "jade": [[.25, .8, .55], [.7, 1, .8], [.1, .5, .35], [.95, .85, .5]],
    "moon": [[.75, .8, 1], [1, 1, 1], [.4, .45, .8], [.9, .85, .6]],
    "spirit": [[.45, .95, .9], [.85, 1, 1], [.2, .5, .6], [.7, .6, 1]],
    "lightning": [[.45, .7, 1], [.92, .96, 1], [.6, .35, 1], [1, 1, .7]],
    "bone": [[.9, .88, .78], [1, 1, .95], [.45, .42, .35], [.5, .9, .6]],
    "water": [[.15, .5, .9], [.6, .9, 1], [.05, .25, .55], [.85, .97, 1]],
    "poison": [[.5, 1, .2], [.8, 1, .5], [.2, .5, .1], [.7, .3, 1]],
}

FX = {
    # ---- Guerreiro
    "warrior_heavy_blow": "trail:l=.6,t=0; slash:sweep=120,tilt=75,r=R,c=1; cracks:lit,c=2,f=150,r=R*.8,n=6; s.faisca:f=150,n=6,r=40,rise=140; shards:lit,c=2,f=150,n=8",
    "warrior_battle_charge": "streak:c=0,fade=.5,rise=30; s.poeira:at=line,r=G,n=8,spread=.35; ring:c=1,f=G,r=200,n=10,d=.3; s.faisca:f=G,n=5,d=.3",
    "warrior_war_cry": "ring:c=0,r=R,n=16,style=1; s.fogo:at=ring,r=160,n=10,out=260,rise=80; spiral:c=1,wide=90,height=260,n=10,spin=500; pillar:c=2,sz=.5",
    "warrior_last_breath": "s.fumaca:n=6,r=80,rise=40,spread=.4; orbs:c=0,converge=1,r=260,n=12,sz=.6; spiral:c=1,wide=70,height=200,n=8,l=1.2; glyph:c=2,r=180,n=10,l=D*.3",
    # ---- Cavaleiro
    "knight_shield_bash": "slash:sweep=60,tilt=10,r=R,c=1; ring:c=0,f=120,r=R*.7,n=10; s.luz:f=120,n=2,r=20; shards:c=1,f=120,n=8,style=3",
    "knight_intervene": "streak:c=1,fade=.6; feathers:c=1,f=G*.5,r=200,n=10; pillar:c=0,f=G,sz=.6,d=.3; ring:c=0,f=G,r=220,n=12,d=.3",
    "knight_defensive_stance": "glyph:c=2,r=200,n=10,l=1.4,spin=40; shards:c=0,lit,n=10,style=3,l=1.2; ring:c=1,r=R*.6,n=12",
    "knight_oath_of_protection": "glyph:c=0,r=R,n=14,l=D,spin=20; pillar:c=1,n=4,at=ring,r=R,sz=.4,l=1.2; feathers:c=1,r=R*.7,n=14,l=2",
    # ---- Guardião
    "guardian_taunt": "ring:c=0,lit,r=R,n=18,style=1; s.poeira:at=ring,r=200,n=10,out=220; cracks:c=2,lit,r=300,n=6",
    "guardian_living_wall": "spikes:lit,c=0,at=ring,r=140,n=12,sz=1.6,style=2,l=1.6; s.poeira:at=ring,r=140,n=8,rise=60; shards:lit,c=2,n=8",
    "guardian_protective_impact": "cracks:lit,c=2,r=R,n=8; spikes:lit,c=0,at=ring,r=R*.7,n=10,style=2; ring:c=3,r=R,n=14; s.poeira:at=ring,r=R*.5,n=10,out=200",
    "guardian_none_shall_pass": "spikes:lit,c=0,at=ring,r=R,n=16,sz=1.8,style=2,l=D*.4; cracks:lit,c=2,r=R,n=10,l=2; glyph:c=3,r=R*.6,n=12,l=D",
    # ---- Patrulheiro
    "ranger_piercing_arrow": "s.folhas:f=80,n=4,r=40,rise=60,ahead=300; beam:c=1,long=1,r=420,sz=1.2; ring:c=0,f=100,r=90,n=8,style=1",
    "ranger_prey_mark": "glyph:c=0,at=aim,r=150,n=10,l=1.5,spin=-90; bolt:c=1,at=aim,height=300,sz=.6,l=.3; s.folhas:at=aim,n=6,r=120,rise=80",
    "ranger_retreat_leap": "s.poeira:n=8,r=100,out=150,rise=40,d=-.05; ring:c=1,r=R,n=12; feathers:c=0,r=150,n=8",
    "ranger_arrow_rain": "beam:c=1,pitch=70,r=260,sz=.8; rain:c=0,at=aim,r=R,n=18,d=L; glyph:c=2,at=aim,r=R,n=12,l=L+1; s.folhas:at=aim,r=R,n=10,d=L+.2,height=250,rise=-30,spread=.8",
    # ---- Duelista
    "duelist_lunge": "streak:c=1,fade=.35,spray=.4; slash:sweep=30,r=220,c=1,d=.05; s.faisca:f=G,n=4,d=.1",
    "duelist_parry": "slash:sweep=90,tilt=80,r=150,c=1,l=.25; s.faisca:f=60,height=110,n=5,r=20; ring:c=3,r=120,n=8",
    "duelist_sidestep": "vanish:l=.15; streak:c=0,fade=.3; s.brilho:n=4,r=60",
    "duelist_deadly_duel": "glyph:c=0,at=aim,r=200,n=12,l=2,style=1; slash:sweep=360,low=1,r=260,c=2; s.brilho:at=aim,n=6,r=150",
    # ---- Assassino
    "assassin_shadow_stab": "vanish:l=.12,d=-.15; slash:sweep=40,tilt=-30,r=200,c=1; s.fumaca:f=100,n=5,r=60; shards:c=2,f=120,n=6,style=2",
    "assassin_smoke_bomb": "s.fumaca:r=R,n=16,rise=20,spread=.5,sz=2.2,l=D*.6; orbs:c=2,r=R*.6,n=10,style=2; ring:c=0,r=R,n=12",
    "assassin_executioner_mark": "glyph:c=2,at=aim,r=140,n=10,l=2,style=1; orbs:c=0,at=aim,r=150,n=8,style=2,converge=1; s.fumaca:at=aim,n=4,r=60",
    "assassin_execution": "slash:sweep=360,r=R,c=0,count=2; shards:c=0,n=14,style=2; s.fumaca:n=8,r=R*.6,out=150; vanish:l=.1,d=-.2",
    # ---- Mago Elemental
    "elemental_bolt": "orbs:c=1,converge=1,r=160,n=8,sz=.5,t=0,l=.3; s.fogo:f=120,height=90,n=4,r=30,ahead=500; ring:c=0,f=100,r=90,n=8",
    "elemental_burst": "ring:c=0,r=R,n=18,style=1; s.fogo:at=ring,r=R*.4,n=12,out=R,rise=120; pillar:c=1,sz=.6; shards:c=2,n=12",
    "elemental_convergence": "orbs:c=0,converge=1,r=300,n=12,l=1.2; orbs:c=3,converge=1,r=260,n=10,l=1.2,spin=-300; spiral:c=1,wide=80,height=240,n=12",
    "elemental_storm": "glyph:c=0,at=aim,r=R,n=14,l=L+1.5,spin=60; rain:c=1,at=aim,r=R,n=16,d=L; s.fogo:at=aim,r=R,n=14,d=L,rise=100,spread=1.2; bolt:c=3,at=aim,d=L",
    # ---- Arcanista
    "arcane_missile": "orbs:c=1,follow=1,r=120,n=6,sz=.4,l=.6; s.brilho:f=100,height=90,n=4,r=30; ring:c=0,f=100,r=80,n=8,style=1",
    "arcane_containment_rune": "glyph:c=0,@z,r=R,n=12,l=D,spin=45,style=2; glyph:c=1,@z,r=R*.6,n=8,l=D,spin=-60; s.brilho:@z,r=R,n=8,spread=1.5",
    "arcane_amplify": "spiral:c=0,wide=100,height=300,n=14,spin=400; ring:c=1,r=R*.5,n=14,style=1; s.brilho:r=R*.4,n=10,rise=100",
    "arcane_field": "glyph:c=0,r=R,n=16,l=D,spin=25,style=3; orbs:c=1,r=R*.7,n=12,l=D*.4; s.brilho:r=R,n=12,rise=80,spread=2",
    # ---- Alquimista
    "alchemist_volatile_flask": "s.fogo:at=aim,n=10,r=150,out=150,rise=120,d=.4; shards:c=3,at=aim,n=10,d=.4; ring:c=0,at=aim,r=200,n=10,d=.4",
    "alchemist_elixir": "s.brilho:n=6,r=60,rise=120; spiral:c=1,wide=70,height=240,n=12; feathers:c=0,r=R*.5,n=10",
    "alchemist_unstable_compound": "s.fumaca:at=aim,n=10,r=180,out=120,d=.4,sz=1.6; orbs:c=1,at=aim,r=200,n=10,d=.4; shards:c=3,at=aim,n=12,style=2,d=.4",
    "alchemist_field_lab": "glyph:c=0,r=R,n=12,l=D,spin=30; pillar:c=1,n=3,at=ring,r=R*.6,sz=.35,l=1.5; s.fumaca:r=R*.6,n=8,rise=60,spread=2,sz=1.4",
    # ---- Clérigo
    "cleric_restoring_light": "pillar:c=1,sz=.8; feathers:c=1,r=R*.4,n=16; ring:c=0,r=R*.5,n=16; s.luz:n=4,r=60,rise=80",
    "cleric_purify": "ring:c=1,r=R*.5,n=14,style=1; s.brilho:at=ring,r=R*.4,n=10,rise=140; spiral:c=0,wide=80,height=260,n=10",
    "cleric_holy_symbol": "glyph:c=0,r=R,n=14,l=D,spin=30,style=2; pillar:c=1,sz=.5,l=1.2; feathers:c=3,r=R*.6,n=12,l=2",
    "cleric_miracle": "pillar:c=1,n=6,at=ring,r=R*.4,sz=.6,style=2; glyph:c=0,r=R*.6,n=16,l=2,spin=40; feathers:c=1,r=R*.5,n=22,l=2.2; s.luz:n=6,r=R*.3,rise=120",
    # ---- Druida
    "druid_binding_roots": "spikes:lit,c=3,at=aim,r=R,n=14,style=1,sz=1.3,d=L,l=1.6; cracks:lit,c=2,at=aim,r=R,n=8,d=L; s.folhas:at=aim,r=R,n=10,d=L,rise=120",
    "druid_natural_regrowth": "s.folhas:r=R*.5,n=14,rise=100,spread=1.2; spiral:c=1,wide=80,height=260,n=12; glyph:c=2,r=R*.4,n=12,l=1.5",
    "druid_feral_form": "spikes:lit,c=3,at=ring,r=120,n=10,style=1; s.folhas:at=ring,r=120,n=10,out=260,rise=100; shards:lit,c=2,n=10; ring:c=0,r=220,n=12",
    "druid_living_forest": "spikes:lit,c=3,r=R,n=18,sz=1.6,style=1,l=D*.4; s.folhas:r=R,n=16,height=260,rise=-20,spread=3; glyph:c=0,r=R,n=14,l=D",
    # ---- Xamã
    "shaman_war_totem": "pillar:c=2,lit,sz=.5,l=D*.5; ring:c=0,r=R,n=16,style=1; s.fogo:at=ring,r=R*.5,n=10,rise=120,spread=1",
    "shaman_healing_totem": "pillar:c=0,lit,sz=.5,l=D*.5; ring:c=1,r=R,n=14,style=1; s.agua:at=ring,r=R*.4,n=10,rise=180,gravity=300",
    "shaman_spirit_call": "orbs:c=0,follow=1,r=200,n=10,l=2.5,spin=200; spiral:c=1,wide=120,height=320,n=14; s.brilho:r=200,n=8,rise=120",
    "shaman_ancestor_dance": "ring:c=0,r=R*.5,n=16,style=1; orbs:c=1,follow=1,r=240,n=12,l=2; s.fumaca:at=ring,r=200,n=8,rise=60",
    # ---- Feiticeiro
    "sorcerer_mystic_blast": "orbs:c=1,converge=1,r=180,n=10,t=0,l=.3; beam:c=0,r=G/8,long=1,sz=2; beam:c=1,r=G/8,long=1,sz=1,d=.03; s.brilho:f=120,height=90,n=5,r=40",
    "sorcerer_familiar": "orbs:c=0,follow=1,r=140,n=6,style=2,l=2.5; s.fumaca:n=6,r=100,rise=60; glyph:c=2,r=140,n=8,l=1",
    "sorcerer_curse": "glyph:c=2,at=aim,r=150,n=10,l=1.6,style=1,spin=-120; orbs:c=0,at=aim,r=180,n=8,style=2,converge=1; s.fumaca:at=aim,n=6,r=80",
    "sorcerer_grand_conjuration": "glyph:c=2,r=R*.5,n=16,l=2,spin=60,style=3; orbs:c=0,follow=1,r=300,n=14,style=1,l=3; pillar:c=1,n=3,at=ring,r=R*.4,sz=.5; s.brilho:r=R*.4,n=12,rise=150",
    # ---- Lâmina Arcana
    "arcblade_arcane_cut": "slash:sweep=200,tilt=-15,r=R*1.5+200,c=0; beam:c=1,r=G/4.5,sz=1.6; s.brilho:at=line,r=G,n=8,spread=.4",
    "arcblade_enchant": "trail:c=1,l=D*.25,sz=1.2; spiral:c=0,wide=60,height=200,n=10; s.brilho:f=40,height=120,n=6,r=40",
    "arcblade_ethereal_step": "vanish:l=.2; streak:c=0,fade=.5,spray=.6; s.brilho:at=line,r=G,n=6,spread=.3",
    "arcblade_transcendent": "orbs:c=1,follow=1,r=160,n=10,style=1,l=2; slash:sweep=360,r=260,c=0,tilt=30; pillar:c=3,sz=.5",
    # ---- Monge
    "monk_fist_sequence": "s.faisca:f=110,height=100,n=8,r=40,spread=.6; ring:c=1,f=120,r=100,n=8,style=1; slash:sweep=40,r=180,c=0,count=4,small=1",
    "monk_repulsing_palm": "ring:c=1,f=140,r=R*.6,n=14,style=1; s.poeira:f=140,n=8,r=80,out=200; spiral:c=0,front=140,at=front,wide=60,height=150,n=8",
    "monk_chi_flow": "spiral:c=0,wide=70,height=240,n=14,spin=300; s.brilho:n=6,r=50,rise=120; glyph:c=3,r=R*.25,n=10,l=1.4",
    "monk_thousand_blows": "slash:sweep=360,r=R,c=1,count=6,small=1; s.faisca:r=R*.6,n=14,rise=120,spread=1.2; ring:c=0,r=R,n=16,style=1",
    # ---- Lanceiro
    "lancer_pierce": "beam:c=1,r=G/4.5,sz=1.3; s.faisca:at=line,r=G,n=6,spread=.25; ring:c=0,f=G,r=120,n=8,d=.1",
    "lancer_sweep": "slash:sweep=360,low=1,r=R,c=1; s.poeira:at=ring,r=R*.8,n=12,out=160; shards:lit,c=2,n=10",
    "lancer_spear_leap": "streak:c=1,fade=.6,rise=50; cracks:lit,c=2,f=G,r=300,n=7,d=.35; spikes:lit,c=0,f=G,at=ring,r=160,n=8,d=.35; s.poeira:f=G,at=ring,r=150,n=8,d=.35",
    "lancer_formation": "spikes:lit,c=0,at=ring,r=R*.9,n=16,sz=1.4,l=D*.4; glyph:c=3,r=R,n=14,l=D; ring:c=1,r=R,n=16",
    # ---- Bardo
    "bard_war_song": "ring:c=0,r=R*.6,n=14,style=1,l=1.2; orbs:c=1,follow=1,r=200,n=10,l=2.5,spin=250; s.fogo:at=ring,r=R*.5,n=8,rise=80,spread=1.5",
    "bard_restoring_ballad": "ring:c=1,r=R*.6,n=14,style=1,l=1.2; orbs:c=3,follow=1,r=200,n=10,l=2.5,spin=-200; s.agua:at=ring,r=R*.5,n=10,rise=150,gravity=250,spread=1.5",
    "bard_dissonant_note": "ring:c=0,f=150,r=150,n=10,style=1; beam:c=1,r=G/4.5,n=3,fan=1,spread=24,sz=1.2; s.brilho:at=line,r=G,n=8,spread=.3",
    "bard_grand_concert": "orbs:c=0,follow=1,r=280,n=14,style=1,l=3; ring:c=1,r=R*.6,n=18,style=2; feathers:c=1,r=R*.5,n=16,l=2; s.luz:n=4,r=80,rise=100",
    # ---- Caçador de Monstros
    "hunter_study_prey": "glyph:c=0,at=aim,r=130,n=8,l=2,spin=60; s.folhas:at=aim,n=4,r=60,rise=60; bolt:c=1,at=aim,height=200,sz=.5",
    "hunter_hunting_bomb": "s.fogo:at=aim,n=10,r=160,out=160,rise=120,d=.45; s.fumaca:at=aim,n=6,r=150,rise=80,d=.5; shards:lit,c=3,at=aim,n=10,d=.45",
    "hunter_prepared_dodge": "streak:c=0,fade=.4; s.poeira:n=6,r=80,out=140; feathers:c=1,r=120,n=6",
    "hunter_prepared_kill": "slash:sweep=140,tilt=70,r=R,c=0; shards:c=0,f=140,n=12,style=2; cracks:lit,c=2,f=140,r=200,n=5; s.faisca:f=140,n=6",
    # ---- Bárbaro
    "barbarian_brutal_axe": "slash:sweep=160,tilt=60,r=R,c=0; shards:c=0,f=150,n=12,style=2; cracks:lit,c=2,f=150,r=R*.7,n=6; s.faisca:f=150,n=6",
    "barbarian_war_mist": "s.fumaca:r=R,n=14,rise=30,spread=.6,sz=2; ring:c=0,r=R,n=14,style=1; spiral:c=1,wide=R*.3,height=200,n=12",
    "barbarian_tribal_roar": "ring:c=0,r=R,n=18,style=1; s.fogo:at=ring,r=200,n=12,out=R,rise=100; pillar:c=1,sz=.5; cracks:lit,c=3,r=260,n=6",
    "barbarian_mist_frenzy": "s.fumaca:n=8,r=120,rise=50,sz=1.5; orbs:c=0,follow=1,r=140,n=8,style=2,l=2; spiral:c=1,wide=70,height=220,n=10,spin=600",
    # ---- Martelo Rúnico
    "runehammer_seismic": "cracks:lit,c=2,f=160,r=R,n=9; spikes:lit,c=0,at=line,r=R,n=10,sz=1.2; glyph:c=3,f=160,r=180,n=10,l=1.2; s.poeira:f=160,n=8,r=150,out=180",
    "runehammer_weight_rune": "glyph:c=0,@z,r=R,n=12,l=D,spin=20,style=2; ring:c=1,@z,r=R,n=14; s.poeira:@z,r=R,n=10,rise=10,spread=2",
    "runehammer_protective_rune": "glyph:c=0,r=R*.4,n=12,l=1.5,spin=-40; shards:c=1,n=10,style=3,l=1.2; ring:c=3,r=R*.4,n=12",
    "runehammer_judgement": "pillar:c=1,sz=.8,style=2; cracks:lit,c=2,r=R,n=10; glyph:c=0,r=R,n=14,l=1.2,spin=80; spikes:lit,c=3,at=ring,r=R*.7,n=10,style=2",
    # ---- Guardião do Musgo
    "moss_bark_skin": "shards:lit,c=3,n=12,l=1.2; spiral:c=0,wide=70,height=220,n=10; s.folhas:n=6,r=80,rise=60",
    "moss_living_thorns": "spikes:lit,c=0,at=ring,r=110,n=12,style=1,sz=.9,l=1.4; s.folhas:at=ring,r=110,n=8,out=120; ring:c=1,r=180,n=10",
    "moss_protective_roots": "spikes:lit,c=3,at=ring,r=R*.3,n=12,style=1,sz=1.2,l=1.6; glyph:c=0,r=R*.3,n=10,l=1.6; s.folhas:r=R*.3,n=10,rise=80",
    "moss_forest_heart": "glyph:c=0,r=R,n=16,l=D,spin=15,style=1; pillar:c=1,sz=.5,l=1.5; spikes:lit,c=3,r=R,n=14,style=1,l=D*.3; s.folhas:r=R,n=12,height=240,rise=-20,spread=3",
    # ---- Andarilho do Vento
    "wind_cut": "slash:sweep=140,r=260,c=0,tilt=10; beam:c=1,r=G/4.5,sz=1,n=2,gap=.08; s.folhas:f=120,n=6,r=60,ahead=500",
    "wind_celestial_step": "streak:c=1,fade=.5,rise=80; feathers:c=0,f=G*.5,r=G*.5,n=10; ring:c=0,f=G,r=160,n=10,d=.3",
    "wind_updraft": "spiral:c=0,wide=R*.4,height=420,n=16,spin=700,flare=1; s.poeira:at=ring,r=R*.6,n=10,rise=260; feathers:c=1,r=R*.6,n=10",
    "wind_storm_dance": "spiral:c=0,wide=110,height=280,n=14,spin=800; slash:sweep=360,r=280,c=1,count=3; s.folhas:r=280,n=10,out=200,rise=100",
    # ---- Tecelão de Jade
    "jade_shard": "shards:c=0,f=100,n=6,style=1,sz=.6,t=0; orbs:c=1,converge=1,f=100,r=120,n=6,sz=.4,t=0,l=.3; s.brilho:f=120,height=90,n=3,r=20",
    "jade_flower": "glyph:c=0,r=R*.3,n=12,l=D*.4,spin=40,style=3; feathers:c=1,r=R*.3,n=14,l=2; s.brilho:r=R*.3,n=8,rise=100",
    "jade_vital_link": "bolt:c=1,hero=1,height=200,sz=.5; spiral:c=0,wide=80,height=220,n=10; ring:c=3,r=R*.4,n=12",
    "jade_palace": "pillar:c=0,lit,n=6,at=ring,r=R*.9,sz=.4,l=D*.4; glyph:c=1,r=R,n=16,l=D,spin=10,style=2; s.brilho:r=R,n=12,spread=3",
    # ---- Mago da Lua
    "moon_ray": "slash:sweep=200,tilt=80,r=180,c=1,t=0,l=.5; beam:c=0,r=G/8,long=1,sz=1.6; s.brilho:at=line,r=G,n=8,spread=.4",
    "moon_night_veil": "s.fumaca:r=R*.4,n=10,rise=20,sz=1.5; orbs:c=2,follow=1,r=200,n=10,style=2,l=2; slash:sweep=220,tilt=80,r=200,c=0",
    "moon_gravity": "glyph:c=2,at=aim,r=R,n=12,l=L+1,spin=-80; orbs:c=0,at=aim,r=R,n=12,converge=1,l=L+.4; ring:c=1,at=aim,r=R,n=14,d=L; s.brilho:at=aim,r=R*.5,n=8,d=L",
    "moon_eclipse": "glyph:c=2,r=R,n=16,l=D,spin=-15,style=1; slash:sweep=300,tilt=85,r=420,c=0,l=1.5; orbs:c=3,r=R*.6,n=14,l=D*.3; s.fumaca:r=R*.6,n=10,spread=2",
    # ---- Domador de Feras
    "tamer_feral_companion": "s.folhas:n=8,r=120,out=150,rise=80; ring:c=0,r=200,n=12; orbs:c=1,follow=1,r=140,n=6,l=1.5",
    "tamer_command_attack": "bolt:c=1,at=aim,height=250,sz=.6; glyph:c=0,at=aim,r=120,n=8,l=1.2; s.poeira:at=aim,n=6,r=80,out=100",
    "tamer_blood_bond": "spiral:c=0,wide=70,height=220,n=10,spin=400; orbs:c=1,converge=1,r=220,n=10; s.brilho:n=4,r=60",
    "tamer_pack_call": "ring:c=0,r=R*.5,n=16,style=1; s.poeira:at=ring,r=R*.4,n=12,out=200; orbs:c=1,follow=1,r=260,n=10,l=2; s.folhas:r=R*.4,n=8,rise=80",
    # ---- Engenheiro Arcano
    "eng_turret": "pillar:c=0,lit,f=G,sz=.4,l=2; glyph:c=1,f=G,r=120,n=8,l=1.5; s.faisca:f=G,n=8,r=50,rise=120",
    "eng_rune_mine": "glyph:c=0,at=aim,r=R,n=10,l=D*.2,spin=90,style=2; s.faisca:at=aim,n=6,r=60,d=.4; ring:c=1,at=aim,r=R,n=10,d=.4",
    "eng_repair_drone": "orbs:c=1,follow=1,r=120,n=4,l=2.5,spin=300; s.faisca:n=6,r=60,height=180,rise=40; spiral:c=0,wide=60,height=200,n=8",
    "eng_overload": "bolt:c=1,hero=1,height=300,sz=.8; bolt:c=3,hero=1,height=250,sz=.6,d=.12; s.faisca:n=12,r=100,rise=140; ring:c=0,r=180,n=12",
    # ---- Corsário
    "corsair_pistol_shot": "s.fumaca:f=110,height=110,n=4,r=30,rise=40; s.faisca:f=110,height=110,n=4,r=20; beam:c=1,r=G/4.5,sz=.6; s.fogo:at=aim,n=4,r=60,d=.15",
    "corsair_cut": "slash:sweep=180,r=R,c=1,tilt=-20; s.faisca:f=120,n=6,r=60; shards:c=3,f=120,n=8",
    "corsair_hook": "beam:c=2,r=G/4.5,sz=.8,l=.4; streak:c=1,fade=.4; s.faisca:f=G,n=4,d=.3",
    "corsair_bombardment": "s.fumaca:at=aim,r=R,n=12,d=L,rise=100,sz=2,spread=1.2; s.fogo:at=aim,r=R,n=14,d=L,out=150,rise=150,spread=1.2; shards:lit,c=3,at=aim,n=16,d=L; ring:c=0,at=aim,r=R,n=16,d=L",
    # ---- Pugilista
    "pug_lightning_jab": "bolt:c=1,f=120,hero=0,height=150,sz=.5,at=front; s.faisca:f=120,height=100,n=8,r=40,spread=.4; ring:c=0,f=120,r=90,n=8",
    "pug_defense_break": "ring:c=1,f=130,r=R*.6,n=12,style=1; shards:c=0,f=130,n=12,style=3; s.faisca:f=130,n=6,r=50",
    "pug_offensive_dodge": "streak:c=0,fade=.3; s.poeira:n=6,r=80,out=120; bolt:c=1,hero=1,height=150,sz=.4,d=.1",
    "pug_relentless_combo": "slash:sweep=50,r=200,c=1,count=6,small=1; bolt:c=0,f=130,at=front,height=200,sz=.6,d=.3; s.faisca:f=130,height=100,n=14,r=50,spread=1",
    # ---- Caçador de Demônios
    "dh_profane_mark": "glyph:c=2,at=aim,r=150,n=10,l=2,style=1,spin=-60; pillar:c=0,at=aim,sz=.35; s.luz:at=aim,n=3,r=40",
    "dh_consecrated_chain": "beam:c=1,r=G/4.5,sz=.7,n=2,gap=.05,spread=6; glyph:c=0,at=aim,r=120,n=8,l=1.2; s.faisca:at=aim,n=6,r=60",
    "dh_demonic_purge": "ring:c=0,f=150,r=150,n=10; beam:c=1,r=G/4.5,sz=2; feathers:c=1,at=line,r=G*.5,n=10; s.luz:at=line,r=G,n=6,spread=.3",
    "dh_infernal_sentence": "orbs:c=0,converge=1,r=200,n=10,t=0,l=.3; beam:c=0,r=G/4.5,long=1,sz=2.2; beam:c=3,r=G/4.5,long=1,sz=1,d=.05; pillar:c=1,at=aim,sz=.6,d=.1",
    # ---- Místico do Véu
    "veil_duplicate": "vanish:l=.2; s.fumaca:n=6,r=80,rise=30; shards:c=1,n=10,style=3",
    "veil_veil": "s.fumaca:r=R*.3,n=10,rise=20,sz=1.6; orbs:c=0,follow=1,r=160,n=8,style=2,l=2; glyph:c=2,r=140,n=8,l=1.2",
    "veil_nightmare": "orbs:c=0,at=aim,r=200,n=10,style=2,converge=1; s.fumaca:at=aim,n=8,r=150,rise=50,sz=1.6; glyph:c=2,at=aim,r=160,n=10,l=1.5,spin=-90",
    "veil_dream_world": "glyph:c=0,@z,r=R,n=16,l=D,spin=20,style=3; s.brilho:@z,r=R,n=14,rise=40,spread=3; orbs:c=1,@z,r=R*.6,n=12,l=D*.3",
    # ---- Arauto de Guerra
    "herald_war_banner": "pillar:c=0,f=G,sz=.3,l=D*.4,lit; ring:c=1,f=G,r=R,n=16,style=1; s.fogo:f=G,at=ring,r=R*.4,n=8,rise=80",
    "herald_advance": "ring:c=0,r=R*.5,n=14,style=1; s.poeira:at=ring,r=R*.4,n=10,out=260; feathers:c=1,r=200,n=8",
    "herald_defense": "ring:c=0,r=R*.5,n=14,style=1; shards:c=1,n=12,style=3,l=1.2; glyph:c=2,r=200,n=10,l=1.2",
    "herald_supreme_command": "pillar:c=0,sz=.7,style=2; ring:c=1,r=R*.6,n=18,style=2; feathers:c=0,r=R*.4,n=16,l=2; s.luz:n=4,r=80,rise=100",
    # ---- Espadachim Carmesim
    "crimson_cut": "slash:sweep=170,r=R,c=0,tilt=15; s.faisca:f=120,n=4,r=60; shards:c=0,f=130,n=12,style=2",
    "crimson_dance": "slash:sweep=360,r=220,c=1,count=2,tilt=25; feathers:c=0,r=200,n=12; s.brilho:n=4,r=80",
    "crimson_blood_for_blood": "orbs:c=0,converge=1,r=260,n=12; spiral:c=1,wide=70,height=220,n=10,spin=500; s.fumaca:n=4,r=60,rise=50",
    "crimson_bloody_rose": "slash:sweep=360,r=R,c=0,count=3,tilt=20; feathers:c=1,r=R*.7,n=18,l=2; shards:c=0,n=16,style=2; ring:c=2,r=R,n=16",
    # ---- Destruidor
    "destroyer_demolition": "cracks:lit,c=2,f=150,r=R,n=10; shards:lit,c=0,f=150,n=16; s.poeira:f=150,n=10,r=150,out=200; spikes:lit,c=0,f=150,at=ring,r=R*.5,n=8,style=2",
    "destroyer_shockwave": "spikes:lit,c=0,at=line,r=G*.4,n=14,sz=1.3; cracks:lit,c=2,r=G*.5,n=6; s.poeira:at=line,r=G,n=10,spread=.4",
    "destroyer_momentum": "s.fogo:n=8,r=100,rise=100; spiral:c=1,wide=70,height=220,n=10,spin=600; ring:c=0,r=180,n=12",
    "destroyer_meteor": "streak:c=0,fade=.7,rise=60,spray=1.4; s.fogo:at=line,r=G,n=10,rise=60,spread=.4; cracks:lit,c=3,f=G,r=360,n=9,d=.35; shards:lit,c=2,f=G,n=16,d=.35; ring:c=1,f=G,r=320,n=14,d=.35",
    # ---- Senhor da Guerra
    "warlord_commander_strike": "slash:sweep=130,tilt=70,r=R,c=1; cracks:lit,c=2,f=150,r=R*.6,n=6; s.faisca:f=150,n=6; pillar:c=3,f=150,sz=.4",
    "warlord_regroup": "ring:c=0,r=R*.5,n=16,style=1; shards:c=1,n=12,style=3,l=1.2; s.poeira:at=ring,r=R*.4,n=8,rise=60",
    "warlord_dominant_presence": "ring:c=0,r=R,n=18,style=2; s.fumaca:at=ring,r=R*.5,n=10,out=200,sz=1.5; orbs:c=2,r=R*.5,n=10,style=2",
    "warlord_battlefield": "glyph:c=0,r=R,n=18,l=D,spin=10,style=1; pillar:c=2,n=4,at=ring,r=R*.8,sz=.35,l=1.5; s.fogo:r=R,n=14,rise=60,spread=3",
    # ---- Colosso
    "colossus_trample": "cracks:lit,c=2,r=R,n=10; s.poeira:at=ring,r=R*.6,n=14,out=240,sz=2; shards:lit,c=0,n=14; ring:c=1,lit,r=R,n=16",
    "colossus_stone_body": "shards:lit,c=0,n=16,l=1.4; spikes:lit,c=2,at=ring,r=100,n=8,style=2; s.poeira:n=6,r=80,rise=40",
    "colossus_intercept": "streak:c=0,fade=.6,spray=1.2; s.poeira:at=line,r=G,n=8,spread=.35; ring:c=1,lit,f=G,r=220,n=12,d=.3",
    "colossus_colossal_form": "pillar:c=1,sz=.9,style=2; cracks:lit,c=2,r=R,n=12; spikes:lit,c=0,at=ring,r=R*.7,n=14,style=2,sz=1.6; s.poeira:at=ring,r=R*.5,n=14,out=260,sz=2",
    # ---- Imortal
    "immortal_persistent_blood": "orbs:c=0,converge=1,r=240,n=12; spiral:c=1,wide=70,height=220,n=10; s.brilho:n=4,r=50,rise=100",
    "immortal_refuse_death": "glyph:c=2,r=180,n=12,l=1.8,spin=-50,style=1; orbs:c=0,follow=1,r=140,n=8,l=2; s.fumaca:n=6,r=80,rise=40",
    "immortal_eternity_mark": "glyph:c=0,at=aim,r=150,n=10,l=2,spin=90,style=3; pillar:c=1,at=aim,sz=.35; s.luz:at=aim,n=3,r=40",
    "immortal_rebirth": "pillar:c=1,sz=.9,style=2; feathers:c=1,r=260,n=22,l=2.4; glyph:c=0,r=260,n=16,l=2.4,spin=40; s.luz:n=6,r=120,rise=140",
    # ---- Fantasma
    "phantom_spectral_pass": "vanish:l=.25; streak:c=0,fade=.6,spray=.6; s.fumaca:at=line,r=G,n=8,spread=.3",
    "phantom_ghost_blade": "slash:sweep=170,r=R,c=0,tilt=-25; slash:sweep=170,r=R*.8,c=1,tilt=25,d=.08; s.fumaca:f=120,n=5,r=70",
    "phantom_haunt": "orbs:c=0,at=aim,r=180,n=8,converge=1; s.fumaca:at=aim,n=6,r=100,rise=60; glyph:c=2,at=aim,r=140,n=8,l=1.4",
    "phantom_thousand_blades": "slash:sweep=360,r=R,c=0,count=7,small=1; shards:c=1,n=18,style=3; spiral:c=2,wide=R*.4,height=240,n=14,spin=900",
    # ---- Cronomante
    "chrono_delay": "glyph:c=2,at=aim,r=140,n=12,l=2,spin=-30,style=3; orbs:c=0,at=aim,r=160,n=8,l=1.5; s.brilho:at=aim,n=4,r=60",
    "chrono_haste": "spiral:c=0,wide=90,height=260,n=14,spin=900; ring:c=1,r=R*.4,n=14,style=1; s.brilho:r=R*.3,n=8,rise=100",
    "chrono_echo": "glyph:c=3,r=140,n=10,l=1.5,spin=200,style=3; shards:c=1,n=10,style=3; orbs:c=0,follow=1,r=120,n=6,l=1.5",
    "chrono_stop_time": "glyph:c=2,@z,r=R,n=18,l=D,spin=-10,style=3; glyph:c=0,@z,r=R*.5,n=10,l=D,spin=40; s.brilho:@z,r=R,n=12,rise=0,spread=2",
    # ---- Oráculo
    "oracle_omen": "orbs:c=0,follow=1,r=180,n=10,style=1,l=2; glyph:c=2,r=200,n=12,l=1.5,spin=60,style=3; s.brilho:n=6,r=80,rise=100",
    "oracle_fair_fate": "glyph:c=0,r=R*.3,n=14,l=1.5,spin=-40,style=3; feathers:c=1,r=R*.3,n=12; s.luz:n=3,r=60,rise=80",
    "oracle_dark_fate": "glyph:c=2,at=aim,r=150,n=10,l=1.6,spin=90,style=1; orbs:c=0,at=aim,r=180,n=8,style=2,converge=1; s.fumaca:at=aim,n=6,r=80",
    "oracle_rewrite_fate": "glyph:c=2,r=R*.4,n=16,l=2,spin=-90,style=3; spiral:c=0,wide=100,height=280,n=14,spin=-400; s.brilho:r=R*.3,n=10,rise=120",
    # ---- Avatar Elemental
    "avatar_elemental_form": "spiral:c=0,wide=90,height=280,n=16,spin=500; s.fogo:n=8,r=100,rise=140; orbs:c=1,follow=1,r=140,n=8,l=2",
    "avatar_primordial_blast": "ring:c=0,r=R,n=20,style=2; pillar:c=1,sz=.8,style=2; s.fogo:at=ring,r=R*.4,n=14,out=R,rise=150; cracks:lit,c=3,r=R,n=10",
    "avatar_absorption": "orbs:c=0,converge=1,r=320,n=14; orbs:c=3,converge=1,r=280,n=12,spin=-400; glyph:c=1,r=180,n=12,l=1.5",
    "avatar_manifestation": "pillar:c=1,sz=1,style=2; spiral:c=0,wide=140,height=400,n=18,spin=600,flare=1; s.fogo:r=200,n=14,rise=160; ring:c=2,r=320,n=18,style=2",
    # ---- Cavaleiro Abissal
    "abyss_cut": "slash:sweep=150,tilt=70,r=R,c=0; s.fumaca:f=140,n=6,r=80; cracks:c=1,f=140,r=R*.6,n=6",
    "abyss_void_armor": "shards:c=0,n=14,style=3,l=1.4; orbs:c=2,follow=1,r=130,n=8,style=2,l=2; s.fumaca:n=6,r=70,rise=40",
    "abyss_chains": "beam:c=2,r=G/4.5,sz=.8,n=3,fan=1,spread=20; glyph:c=0,at=aim,r=150,n=10,l=1.4,style=1; s.fumaca:at=aim,n=6,r=80",
    "abyss_gate": "glyph:c=2,r=R,n=16,l=D,spin=-20,style=1; pillar:c=0,sz=.7,l=1.5; orbs:c=3,r=R*.6,n=12,style=2,l=D*.3; s.fumaca:r=R*.6,n=12,rise=40,spread=3",
    # ---- Sangromante
    "blood_spear": "shards:c=0,f=100,n=6,style=1,sz=.7,t=0; orbs:c=1,converge=1,f=100,r=140,n=8,sz=.4,t=0,l=.3; s.brilho:f=110,height=100,n=3,r=20",
    "blood_drain": "beam:c=0,r=G/8,long=1,sz=1.4,l=1.2; orbs:c=1,converge=1,r=G*.5,n=14,l=1.2; spiral:c=2,wide=60,height=200,n=8",
    "blood_crimson_bond": "beam:c=0,r=G/4.5,sz=.6,n=4,fan=1,spread=40; orbs:c=1,converge=1,r=260,n=10; s.brilho:n=4,r=60",
    "blood_sea": "wave:c=0,lit,length=R*2,width=R*1.2,speed=900,height=60,d=0; glyph:c=2,r=R,n=16,l=D,spin=10,style=1; orbs:c=1,r=R*.6,n=12,l=D*.3",
    # ---- Invocador Astral
    "astral_star_fragment": "orbs:c=0,follow=1,r=140,n=6,style=1,l=2.5; s.brilho:n=6,r=80,height=200,rise=-30; pillar:c=1,sz=.35",
    "astral_constellation": "glyph:c=2,r=R*.3,n=14,l=2,spin=30,style=3; orbs:c=0,follow=1,r=200,n=12,style=1,l=2; s.brilho:r=R*.3,n=8",
    "astral_comet": "bolt:c=1,at=aim,height=900,sz=1.2,d=L-.3; s.fogo:at=aim,n=12,r=200,out=160,rise=150,d=L; shards:c=0,at=aim,n=16,d=L; ring:c=2,at=aim,r=R,n=16,d=L",
    "astral_zodiac_portal": "glyph:c=2,r=R*.5,n=18,l=2.5,spin=40,style=3; pillar:c=0,n=3,at=ring,r=R*.4,sz=.5,style=2; orbs:c=1,follow=1,r=300,n=14,style=1,l=3; s.brilho:r=R*.4,n=12,rise=120",
    # ---- Mestre das Correntes
    "chains_pull": "beam:c=0,r=G/4.5,sz=.8,l=.5; s.faisca:at=aim,n=6,r=60,d=.1; shards:c=1,f=150,n=6,style=3",
    "chains_shackles": "beam:c=0,r=G/4.5,sz=.7,n=2,gap=.06,spread=8; ring:c=2,at=aim,r=120,n=10,l=1.5; s.faisca:at=aim,n=6,r=50",
    "chains_protective_link": "ring:c=0,r=R*.3,n=16,style=1,l=1.2; shards:c=1,n=12,style=3; glyph:c=3,r=R*.3,n=12,l=1.4",
    "chains_prison": "spikes:lit,c=0,@z,at=ring,r=R,n=16,sz=1.4,style=2,l=D*.5; glyph:c=2,@z,r=R,n=14,l=D,style=1; ring:c=1,@z,r=R,n=16",
    # ---- Arquimago
    "archmage_superior_magic": "glyph:c=2,f=80,r=120,n=10,l=.6,t=0,spin=200; orbs:c=1,converge=1,r=200,n=12,t=0,l=.35; s.fogo:f=120,height=100,n=6,r=40,ahead=600",
    "archmage_counterspell": "ring:c=0,r=R,n=18,style=2; shards:c=1,n=14,style=3; glyph:c=3,r=R*.5,n=12,l=1,spin=-200",
    "archmage_arcane_fold": "vanish:l=.3; glyph:c=0,r=150,n=10,l=.8,spin=300; glyph:c=1,f=G,r=150,n=10,l=.8,spin=-300,d=.25; s.brilho:f=G,n=6,r=80,d=.25",
    "archmage_grand_ritual": "glyph:c=0,at=aim,r=R,n=18,l=L+1,spin=40,style=3; glyph:c=2,at=aim,r=R*.6,n=12,l=L+1,spin=-60; pillar:c=1,at=aim,n=5,at=ring,r=R*.7,sz=.5,d=L; s.brilho:at=aim,r=R,n=14,d=L,rise=150",
    # ---- Cavaleiro do Dragão
    "dragon_charge": "streak:c=0,fade=.6,rise=50,spray=1.2; s.fogo:at=line,r=G,n=10,rise=60,spread=.35; feathers:c=1,f=G*.5,r=200,n=10; ring:c=2,f=G,r=240,n=12,d=.3",
    "dragon_scales": "shards:lit,c=2,n=16,l=1.4; spiral:c=0,wide=80,height=240,n=12,spin=400; s.fogo:n=6,r=80,rise=100",
    "dragon_breath": "s.fogo:f=120,n=16,r=80,ahead=R*1.2,rise=40,spread=.6,sz=1.8; ring:c=0,f=200,r=R*.4,n=10; shards:c=1,f=R*.6,n=10",
    "dragon_awakening": "feathers:c=0,r=300,n=24,l=2.4; pillar:c=1,sz=1,style=2; ring:c=2,r=360,n=20,style=2; s.fogo:r=220,n=14,rise=160",
    # ---- Soberano Rúnico
    "sovereign_power_rune": "glyph:c=0,f=G,r=R,n=14,l=D,spin=30,style=2; pillar:c=1,f=G,sz=.4,l=1.2; s.fogo:f=G,r=R*.5,n=8,rise=80",
    "sovereign_ward_rune": "glyph:c=0,f=G,r=R,n=14,l=D,spin=-30,style=2; shards:c=1,f=G,n=10,style=3; ring:c=3,f=G,r=R,n=14",
    "sovereign_prison_rune": "glyph:c=2,@z,r=R,n=14,l=D,spin=60,style=1; spikes:lit,c=0,@z,at=ring,r=R,n=12,style=2,l=D*.4; s.poeira:@z,at=ring,r=R,n=8",
    "sovereign_primordial_word": "glyph:c=0,r=R,n=18,l=1.4,spin=100,style=3; pillar:c=1,n=4,at=ring,r=R*.6,sz=.5,style=2; ring:c=2,r=R,n=18,style=2; s.luz:n=4,r=100",
    # ---- Caminhante do Vazio
    "void_step": "vanish:l=.2; s.fumaca:n=6,r=70,rise=20; orbs:c=2,f=G,r=120,n=8,style=2,converge=1,d=.2",
    "void_dimensional_cut": "slash:sweep=200,r=R,c=1,tilt=80; shards:c=0,f=130,n=14,style=3; s.fumaca:f=130,n=6,r=80",
    "void_linked_portal": "glyph:c=0,r=160,n=12,l=1.2,spin=300,style=1; glyph:c=1,f=G,r=160,n=12,l=1.2,spin=-300; vanish:l=.25,d=.1",
    "void_spatial_collapse": "orbs:c=0,at=aim,r=R,n=14,style=2,converge=1,l=L+.3; glyph:c=2,at=aim,r=R,n=14,l=L+.6,spin=-120,style=1; ring:c=1,at=aim,r=R,n=16,d=L; shards:c=3,at=aim,n=16,style=3,d=L",
    # ---- Santo Guerreiro
    "saint_radiant_strike": "slash:sweep=140,tilt=75,r=R,c=1; pillar:c=0,f=150,sz=.5; s.luz:f=150,n=3,r=40; feathers:c=1,f=150,r=150,n=8",
    "saint_shield_of_faith": "glyph:c=0,r=R*.3,n=14,l=1.6,spin=30; shards:c=1,n=12,style=3,l=1.2; feathers:c=1,r=200,n=10",
    "saint_martyrdom": "pillar:c=0,sz=.7; ring:c=1,r=R,n=18,style=2; s.luz:at=ring,r=R*.4,n=8,rise=100; feathers:c=1,r=R*.6,n=14",
    "saint_celestial_intervention": "pillar:c=1,n=6,at=ring,r=R*.4,sz=.5,style=2; feathers:c=1,r=R*.5,n=22,l=2.4; glyph:c=0,r=R*.6,n=18,l=2,spin=40,style=2; s.luz:n=6,r=R*.3,rise=150",
    # ---- Senhor das Tempestades
    "storm_chain_lightning": "bolt:c=1,at=aim,height=500,sz=1; bolt:c=0,at=aim,height=400,sz=.8,d=.1; s.faisca:at=aim,n=10,r=150,rise=150; beam:c=1,r=G/4.5,sz=.6",
    "storm_gust": "spiral:c=0,front=200,at=front,wide=120,height=200,n=12,spin=900; s.poeira:at=line,r=G,n=10,spread=.3; feathers:c=1,at=line,r=G*.5,n=10",
    "storm_cloud": "s.fumaca:@z,r=R,n=14,height=380,rise=0,sz=2.4,l=D*.6,spread=2; bolt:c=1,@z,height=380,sz=.8,d=.5; bolt:c=0,@z,height=380,sz=.7,d=1.5; rain:c=3,@z,r=R,n=14,l=D*.4",
    "storm_wrath_of_heavens": "s.fumaca:at=aim,r=R,n=16,height=500,rise=0,sz=2.6,l=L+2,spread=1; bolt:c=1,at=aim,height=900,sz=1.4,d=L; bolt:c=0,at=aim,height=800,sz=1.2,d=L+.2; s.faisca:at=aim,r=R,n=16,d=L,rise=200; ring:c=2,at=aim,r=R,n=18,d=L",
    # ---- Arconte Solar
    "solar_cut": "slash:sweep=180,r=R,c=1,tilt=10; s.luz:f=130,n=3,r=50; ring:c=0,f=130,r=150,n=10",
    "solar_flash": "s.luz:n=6,r=R*.3,rise=0,sz=3; ring:c=1,r=R,n=20,style=2; orbs:c=0,r=R*.5,n=12,style=1",
    "solar_aura": "glyph:c=0,r=R,n=16,l=D,spin=20,style=2; orbs:c=1,follow=1,r=180,n=12,style=1,l=3; pillar:c=2,sz=.4,l=1.5",
    "solar_descending_sun": "orbs:c=1,at=aim,r=R*.4,n=12,style=1,l=L+.3; pillar:c=0,at=aim,sz=1.2,style=2,d=L; s.luz:at=aim,n=6,r=R*.4,sz=3,d=L; ring:c=2,at=aim,r=R,n=20,style=2,d=L",
    # ---- Devorador de Magia
    "devourer_mana_drain": "beam:c=0,r=G/8,long=1,sz=1,l=1; orbs:c=3,converge=1,r=G*.4,n=12,l=1; s.brilho:n=4,r=60",
    "devourer_silence": "glyph:c=2,at=aim,r=140,n=10,l=1.6,spin=0,style=1; shards:c=1,at=aim,n=8,style=3; ring:c=0,at=aim,r=150,n=10",
    "devourer_arcane_absorption": "orbs:c=0,converge=1,r=280,n=14; shards:c=1,n=10,style=3; glyph:c=3,r=150,n=10,l=1.4,spin=-100",
    "devourer_null_zone": "glyph:c=2,r=R,n=16,l=D,spin=-15,style=1; ring:c=0,r=R,n=18; orbs:c=3,r=R*.7,n=12,converge=1,l=D*.3",
    # ---- Necromante
    "necro_raise_dead": "cracks:lit,c=2,r=200,n=6; s.fumaca:n=8,r=120,rise=60,sz=1.5; shards:lit,c=0,n=12; glyph:c=3,r=200,n=10,l=1.6,style=1",
    "necro_corpse_explosion": "shards:lit,c=0,at=aim,n=18,d=L; s.fumaca:at=aim,n=10,r=R*.5,out=150,rise=80,d=L,sz=1.6; ring:c=3,at=aim,r=R,n=14,d=L",
    "necro_soul_harvest": "orbs:c=3,converge=1,r=R,n=16,l=1.2; spiral:c=1,wide=90,height=280,n=12; ring:c=2,r=R,n=16,style=1",
    "necro_army_of_dead": "cracks:lit,c=2,r=R*.6,n=10; glyph:c=3,r=R*.6,n=16,l=2.5,spin=-30,style=1; shards:lit,c=0,n=16; s.fumaca:r=R*.5,n=14,rise=60,sz=1.8,spread=1",
    # ---- Herdeiro do Colosso
    "heir_titan_fist": "ring:c=1,lit,f=130,r=R*.6,n=12,style=1; cracks:lit,c=2,f=130,r=R*.8,n=7; shards:lit,c=0,f=130,n=14; s.poeira:f=130,n=8,r=100,out=180",
    "heir_giant_blood": "spiral:c=3,wide=80,height=240,n=12; orbs:c=0,converge=1,r=240,n=10; s.poeira:n=6,r=80,rise=40",
    "heir_titan_step": "cracks:lit,c=2,r=R,n=12; spikes:lit,c=0,at=ring,r=R*.8,n=14,style=2,sz=1.4; s.poeira:at=ring,r=R*.5,n=14,out=260,sz=2",
    "heir_ancestral_awakening": "pillar:c=3,sz=1,style=2; spikes:lit,c=0,at=ring,r=R*.6,n=14,style=2,sz=1.8; cracks:lit,c=2,r=R,n=12; ring:c=1,r=R,n=20,style=2; s.poeira:at=ring,r=R*.5,n=14,out=280,sz=2.2",
    # ---- Senhor da Matilha
    "pack_call": "ring:c=0,r=R*.4,n=14,style=1; s.poeira:at=ring,r=R*.3,n=10,out=200; orbs:c=1,follow=1,r=220,n=8,l=2",
    "pack_alpha_mark": "glyph:c=0,at=aim,r=140,n=10,l=2,style=1; bolt:c=1,at=aim,height=250,sz=.6; s.poeira:at=aim,n=6,r=80",
    "pack_hunting_howl": "ring:c=0,r=R*.5,n=18,style=1; s.folhas:at=ring,r=R*.4,n=12,out=260; spiral:c=1,wide=80,height=260,n=10",
    "pack_great_hunt": "streak:c=0,fade=.6; beam:c=1,r=G/4.5,sz=.7,n=4,fan=1,spread=40; s.poeira:at=line,r=G,n=10,spread=.4",
    # ---- Cartógrafo do Vale
    "carto_eye": "ring:c=1,r=1200,n=24,style=1,l=1.2; glyph:c=0,r=200,n=12,l=1.5,spin=90,style=3; s.brilho:n=6,r=100,rise=100",
    "carto_safe_route": "spikes:c=1,at=line,r=G*.5+600,n=12,sz=.5; feathers:c=0,at=line,r=600,n=10; ring:c=0,r=R*.3,n=14",
    "carto_mark_terrain": "glyph:c=0,r=R,n=16,l=D,spin=5,style=3; pillar:c=1,n=4,at=ring,r=R,sz=.25,l=D*.3; s.folhas:r=R,n=10,spread=3",
    "carto_impossible_map": "ring:c=0,r=1600,n=28,style=2,l=1.5; glyph:c=2,r=300,n=18,l=2,spin=-60,style=3; orbs:c=1,r=300,n=14,style=1; s.brilho:r=300,n=12,rise=150",
    # ---- Mestre Forjador
    "forge_sharpen": "s.faisca:f=60,height=110,n=12,r=30,rise=100,spread=1; trail:c=1,l=1,sz=1; ring:c=0,r=R*.3,n=12",
    "forge_reinforce": "shards:lit,c=0,n=14,l=1.2; s.faisca:n=8,r=80,rise=120; glyph:c=3,r=R*.3,n=12,l=1.4",
    "forge_field_forge": "pillar:c=0,f=G,lit,sz=.5,l=D*.3; s.fogo:f=G,n=10,r=80,rise=100,spread=2; s.faisca:f=G,n=10,r=60,rise=160,spread=2; ring:c=1,f=G,r=R,n=14",
    "forge_masterpiece": "pillar:c=1,sz=.7,style=2; s.faisca:n=16,r=120,rise=200,spread=.6; shards:c=3,n=14,style=3; ring:c=0,r=R*.5,n=16,style=2",
    # ---- Guardião do Santuário
    "sanct_seal": "glyph:c=0,r=R,n=16,l=D,spin=15,style=2; pillar:c=1,n=4,at=ring,r=R,sz=.35,l=1.5; s.luz:r=R,n=8,spread=2",
    "sanct_absolute_protection": "shards:c=1,n=16,style=3,l=1.6; glyph:c=0,r=R*.3,n=14,l=2,spin=-30,style=2; feathers:c=1,r=200,n=12",
    "sanct_judgement_of_invader": "glyph:c=2,r=R*.3,n=12,l=1.6,spin=60,style=1; pillar:c=0,sz=.5; s.luz:n=4,r=60,rise=80",
    "sanct_living_sanctuary": "glyph:c=0,r=R,n=18,l=D,spin=10,style=2; pillar:c=1,n=6,at=ring,r=R*.8,sz=.4,l=2; feathers:c=1,r=R*.6,n=16,l=2.4; s.luz:r=R*.5,n=8,rise=100",
    # ---- Portador da Lua
    "moonb_crescent": "slash:sweep=200,tilt=85,r=R*1.6+180,c=1; beam:c=0,r=G/4.5,sz=1.6; s.brilho:at=line,r=G,n=8,spread=.4",
    "moonb_blessing": "slash:sweep=220,tilt=85,r=180,c=1; orbs:c=0,follow=1,r=150,n=8,style=1,l=2; s.brilho:n=6,r=80,rise=100",
    "moonb_lunar_step": "vanish:l=.15; streak:c=1,fade=.5; s.brilho:at=line,r=G,n=6,spread=.3; slash:sweep=200,tilt=85,f=G,r=200,c=0,d=.3",
    "moonb_manifested_moon": "orbs:c=1,r=120,n=4,style=1,sz=2.4,l=D*.3; glyph:c=2,r=R,n=18,l=D,spin=15,style=1; ring:c=0,r=R,n=18,style=2; s.brilho:r=R,n=12,rise=40,spread=3",
    # ---- Rei sem Coroa
    "king_royal_command": "orbs:c=0,follow=1,r=100,n=6,style=1,l=2; ring:c=1,r=R*.4,n=16,style=1; s.luz:n=3,r=60,rise=80",
    "king_authority": "ring:c=0,r=R,n=20,style=2; shards:c=3,n=14,style=3; pillar:c=1,sz=.6",
    "king_rally": "ring:c=0,r=R*.4,n=16,style=1; s.fogo:at=ring,r=R*.3,n=12,out=200,rise=100; feathers:c=1,r=R*.3,n=12",
    "king_my_kingdom": "glyph:c=0,r=R,n=20,l=D,spin=8,style=2; pillar:c=1,n=6,at=ring,r=R*.9,sz=.4,style=2,l=2; orbs:c=3,r=R*.5,n=14,style=1,l=D*.3; s.luz:r=R*.5,n=10,rise=120",
    # ---- Serafim da Aurora
    "seraph_dawn_light": "feathers:c=1,f=150,r=150,n=10; beam:c=0,r=G/4.5,sz=2,long=0; s.luz:at=line,r=G,n=8,spread=.3; ring:c=1,f=150,r=150,n=10",
    "seraph_celestial_wings": "feathers:c=1,r=200,n=20,l=1.6; streak:c=0,fade=.6,rise=60; s.luz:at=line,r=G,n=8,spread=.4",
    "seraph_divine_grace": "feathers:c=1,r=R*.3,n=16,l=2; glyph:c=0,r=R*.3,n=14,l=1.8,spin=30,style=2; shards:c=1,n=10,style=3",
    "seraph_first_dawn": "pillar:c=1,sz=1.1,style=2; feathers:c=1,r=R*.6,n=26,l=2.6; ring:c=0,r=R,n=22,style=2; ring:c=3,r=R*.6,n=18,d=.15; s.luz:n=8,r=R*.3,rise=150",
    # ---- Arauto do Fim
    "end_first_seal": "glyph:c=2,at=aim,r=160,n=12,l=2,spin=-40,style=1; pillar:c=0,at=aim,sz=.4; s.fumaca:at=aim,n=6,r=80",
    "end_trumpet": "ring:c=0,r=R,n=20,style=2; s.fumaca:at=ring,r=R*.5,n=12,out=220,sz=1.6; orbs:c=2,r=R*.5,n=12,style=2",
    "end_omen": "bolt:c=1,at=aim,height=600,sz=1; glyph:c=2,at=aim,r=160,n=10,l=1.6,style=1; orbs:c=0,at=aim,r=200,n=10,style=2,converge=1",
    "end_apocalypse": "s.fumaca:r=R,n=18,height=420,rise=0,sz=2.8,spread=1; bolt:c=1,hero=1,height=900,sz=1.4,d=.1; pillar:c=0,sz=1.2,style=2; cracks:c=1,r=R,n=12; ring:c=2,r=R,n=22,style=2",
    # ---- Vaso do Éter
    "ether_condensed": "orbs:c=1,converge=1,r=180,n=10,t=0,l=.35; glyph:c=2,f=80,r=100,n=8,l=.5,t=0,spin=300; s.brilho:f=120,height=100,n=4,r=30,ahead=500",
    "ether_astral_body": "orbs:c=0,follow=1,r=140,n=10,style=1,l=2; spiral:c=1,wide=80,height=260,n=12,spin=300; s.brilho:n=6,r=80,rise=60",
    "ether_transfer": "beam:c=1,pitch=80,r=200,sz=1; orbs:c=0,converge=1,r=R*.3,n=12; s.brilho:r=R*.3,n=8,rise=120",
    "ether_transcendence": "pillar:c=1,sz=.9,style=2; orbs:c=0,follow=1,r=200,n=14,style=1,l=3; feathers:c=1,r=220,n=16,l=2.2; ring:c=2,r=300,n=18,style=2",
    # ---- Soberano Primordial
    "primordial_create": "spikes:lit,c=0,f=G,at=ring,r=R,n=12,style=2,sz=1.4,l=D*.5; glyph:c=3,f=G,r=R,n=14,l=D,style=2; s.poeira:f=G,at=ring,r=R,n=8",
    "primordial_destroy": "orbs:c=1,at=aim,r=R*.5,n=12,converge=1,l=L; s.fogo:at=aim,n=16,r=R*.5,out=200,rise=180,d=L,sz=2; cracks:c=0,at=aim,r=R,n=10,d=L; ring:c=2,at=aim,r=R,n=18,style=2,d=L",
    "primordial_balance": "glyph:c=0,r=R*.3,n=14,l=2,spin=30,style=3; glyph:c=3,r=R*.2,n=10,l=2,spin=-30,style=3; spiral:c=1,wide=80,height=260,n=12",
    "primordial_law": "glyph:c=0,r=R,n=22,l=D,spin=6,style=3; pillar:c=1,n=6,at=ring,r=R*.7,sz=.5,style=2,l=2; ring:c=2,r=R,n=22,style=2; s.luz:r=R*.5,n=10,rise=120",
    # ---- Herdeiro do Caos
    "chaos_fragment": "shards:c=0,f=100,n=8,style=3,t=0; orbs:c=1,converge=1,r=160,n=8,t=0,l=.3; s.faisca:f=110,height=90,n=4,r=30",
    "chaos_rupture": "cracks:c=0,@z,r=R,n=10,l=D*.5; orbs:c=1,@z,r=R*.6,n=12,style=2,l=D*.4; shards:c=3,@z,n=14,style=3; ring:c=2,@z,r=R,n=16",
    "chaos_rising_entropy": "spiral:c=0,wide=90,height=260,n=14,spin=-700; orbs:c=3,follow=1,r=150,n=10,style=2,l=2; s.faisca:n=8,r=80,rise=100",
    "chaos_incarnate": "pillar:c=2,sz=1,style=2; spiral:c=0,wide=140,height=380,n=18,spin=800,flare=1.5; orbs:c=1,follow=1,r=240,n=14,style=2,l=3; shards:c=3,n=16,style=3; s.fumaca:r=220,n=10,rise=60",
    # ---- Avatar do Destino
    "fate_favorable_thread": "beam:c=1,pitch=85,r=220,sz=.4,n=3,gap=.1,spread=30; glyph:c=0,r=R*.3,n=14,l=1.6,spin=40,style=3; s.brilho:r=R*.3,n=8,rise=100",
    "fate_cut_thread": "slash:sweep=200,r=260,c=1,tilt=5; beam:c=0,r=G/4.5,sz=.4; glyph:c=2,at=aim,r=130,n=10,l=1.4,style=3",
    "fate_bound_destiny": "ring:c=0,r=R*.3,n=16,style=1,l=1.6; beam:c=1,pitch=85,r=220,sz=.3,n=4,gap=.08,spread=40; glyph:c=2,r=R*.3,n=12,l=1.8,style=3",
    "fate_causality": "glyph:c=2,r=R*.4,n=18,l=2.2,spin=-60,style=3; spiral:c=0,wide=100,height=300,n=14,spin=300; feathers:c=1,r=R*.3,n=14,l=2; s.luz:n=4,r=80,rise=120",
}
