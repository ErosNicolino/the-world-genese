# Os clipes de "vida" do herói (13/11), feitos à mão em camadas como os ofícios dos moradores (npc_anims_hand.py).
# {nome: (clipe de base, segundos ou None = a duração do clipe de base, camadas)}. Lido por MakePowerAnims.py --vida;
# o jogo escolhe pelo corpo e pelo momento em UVortexAnimInstance (VortexAnimInstance.cpp):
#   Idle_F / Idle_E   o parado da mulher e do elfo (em laço); Walk_F o andar da mulher (o quadril balança)
#   Vida_*            gestos de quem está parado há um tempo (uma vez; começam e acabam na pose parada)

def _breath(a=.008):
    return [(0, 0), (.5, -a), (1, 0)]


VIDA_ANIMS = {
    # mulher: o peso num quadril e o queixo um pouco erguido (a mão na cintura não saiu: as camadas não abrem o cotovelo para o lado)
    "Idle_F": ("Idle", 6.0, {
        "tilt": [(0, 4), (.5, 5), (1, 4)], "twist": [(0, -5), (.5, -3), (1, -5)],
        "look": [(0, -3), (1, -3)], "lift": _breath()}),
    # elfo: ereto, os braços um pouco para trás, o queixo erguido, quase sem balanço
    "Idle_E": ("Idle", 7.0, {
        "lean": [(0, -3), (1, -3)], "look": [(0, -5), (1, -5)], "arms": [(0, -6), (1, -6)],
        "twist": [(0, -3), (.5, 3), (1, -3)], "lift": _breath(.005)}),
    # olha em volta: o tronco e a cabeça para um lado, depois para o outro
    "Vida_Olhar": ("Idle", 4.6, {
        "twist": [(0, 0), (.18, 28), (.4, 28), (.55, 0), (.68, -24), (.86, -24), (1, 0)],
        "look": [(0, 0), (.18, -4), (.55, 0), (.68, 5), (1, 0)], "lift": _breath()}),
    # estica o pescoço e solta os ombros
    "Vida_Ombros": ("Idle", 3.4, {
        "arms": [(0, 0), (.25, -14), (.6, -10), (1, 0)], "tilt": [(0, 0), (.3, 7), (.6, -7), (1, 0)],
        "look": [(0, 0), (.3, -12), (.6, -8), (1, 0)], "lift": [(0, 0), (.3, .012), (.7, .006), (1, 0)]}),
    # confere a arma: levanta a mão direita e olha para ela
    "Vida_Arma": ("Idle", 3.8, {
        "armr": [(0, 0), (.2, 55), (.7, 55), (1, 0)], "elbr": [(0, 0), (.2, 70), (.45, 55), (.7, 70), (1, 0)],
        "look": [(0, 0), (.22, 22), (.7, 22), (1, 0)], "twist": [(0, 0), (.25, -8), (.7, -8), (1, 0)]}),
    # (16/11: o Vida_Beber saiu daqui: é feito à mão no Tools/Blender/MakeMountAnims.py; por camadas o braço abria para o lado)
    # o andar da mulher: o quadril de um lado para o outro a cada passo, os braços mais perto do corpo
    "Walk_F": ("Walk", None, {
        "tilt": [(0, -5), (.25, 0), (.5, 5), (.75, 0), (1, -5)], "twist": [(0, 4), (.5, -4), (1, 4)], "arms": [(0, -4), (1, -4)]}),
}
