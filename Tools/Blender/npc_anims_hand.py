# Os ofícios dos moradores da vila (08/11), feitos à mão em camadas por cima da pose parada (Idle), em laço: cada curva
# começa e termina no mesmo valor. {nome: (segundos de um ciclo, camadas)}. As camadas são as do build() do
# MakePowerAnims.py (lean, twist, tilt, look, lift, push, arms/armr/arml, elbows/elbr/elbl). Lido por
# MakePowerAnims.py --npc; o AVortexNpc toca pelo nome (MX_humano_<nome>), escolhido no Data/Population/npc_schedule.json.

def _breath(a=.012):
    return [(0, 0), (.5, -a), (1, 0)]


NPC_ANIMS = {
    # o ferreiro: segura a peça com a esquerda, levanta o martelo e bate na bigorna
    "Npc_Martelar": (1.1, {
        "lean": [(0, 18), (1, 18)], "look": [(0, 25), (1, 25)],
        "armr": [(0, 20), (.35, 95), (.5, 15), (.6, 12), (1, 20)], "elbr": [(0, 40), (.35, 75), (.5, 15), (1, 40)],
        "arml": [(0, 35), (1, 35)], "elbl": [(0, 60), (1, 60)],
        "twist": [(0, 0), (.35, -8), (.5, 5), (1, 0)], "lift": [(0, 0), (.45, 0), (.52, -.03), (.7, 0), (1, 0)]}),
    # o lojista no balcão: as mãos apoiadas, olha em volta devagar
    "Npc_Balcao": (4.0, {
        "lean": [(0, 14), (1, 14)], "arms": [(0, 35), (1, 35)], "elbows": [(0, 25), (1, 25)],
        "look": [(0, 8), (.5, 14), (1, 8)], "twist": [(0, -6), (.5, 6), (1, -6)], "lift": _breath()}),
    # costurando: as mãos juntas na frente, a direita puxa a linha
    "Npc_Costurar": (1.6, {
        "lean": [(0, 12), (1, 12)], "look": [(0, 30), (1, 30)],
        "arml": [(0, 45), (1, 45)], "elbl": [(0, 70), (1, 70)],
        "armr": [(0, 45), (.3, 62), (.55, 40), (1, 45)], "elbr": [(0, 70), (.3, 35), (.55, 80), (1, 70)], "lift": _breath(.006)}),
    # mexendo o caldeirão (as ervas, a sopa): a colher em roda
    "Npc_Mexer": (1.4, {
        "lean": [(0, 15), (1, 15)], "look": [(0, 28), (1, 28)], "armr": [(0, 50), (1, 50)], "elbr": [(0, 40), (1, 40)],
        "arml": [(0, 20), (1, 20)], "elbl": [(0, 30), (1, 30)],
        "twist": [(0, -12), (.25, 0), (.5, 12), (.75, 0), (1, -12)], "tilt": [(0, 0), (.25, 4), (.5, 0), (.75, -4), (1, 0)]}),
    # lendo (o mapa, o livro): o livro nas mãos e de vez em quando vira a página
    "Npc_Ler": (5.0, {
        "arms": [(0, 30), (1, 30)], "elbows": [(0, 62), (1, 62)], "look": [(0, 32), (1, 32)],
        "twist": [(0, -3), (.5, 3), (1, -3)], "armr": [(0, 0), (.8, 0), (.87, 22), (.94, 0), (1, 0)], "lift": _breath(.008)}),
    # conversando: gesticula com uma mão, depois com a outra, o corpo acompanha
    "Npc_Conversar": (4.0, {
        "armr": [(0, 0), (.15, 35), (.3, 20), (.45, 40), (.6, 5), (1, 0)], "elbr": [(0, 20), (.15, 60), (.45, 70), (.6, 30), (1, 20)],
        "arml": [(0, 0), (.5, 0), (.62, 30), (.78, 15), (.9, 0), (1, 0)], "elbl": [(0, 20), (.5, 20), (.62, 55), (.9, 20), (1, 20)],
        "twist": [(0, 0), (.2, 8), (.5, -6), (.75, 4), (1, 0)], "tilt": [(0, 0), (.3, 3), (.7, -3), (1, 0)],
        "look": [(0, 0), (.25, -5), (.5, 4), (1, 0)], "lift": _breath()}),
    # o padeiro sovando a massa: as duas mãos empurram e voltam
    "Npc_Sovar": (1.2, {
        "lean": [(0, 25), (1, 25)], "look": [(0, 30), (1, 30)], "arms": [(0, 40), (.5, 55), (1, 40)],
        "elbows": [(0, 60), (.5, 20), (1, 60)], "push": [(0, 0), (.5, .05), (1, 0)], "lift": [(0, 0), (.5, -.03), (1, 0)]}),
    # a carpinteira serrando: o braço direito vai e volta, o corpo junto
    "Npc_Serrar": (1.0, {
        "lean": [(0, 20), (1, 20)], "look": [(0, 25), (1, 25)], "armr": [(0, 40), (.5, 70), (1, 40)], "elbr": [(0, 80), (.5, 20), (1, 80)],
        "arml": [(0, 25), (1, 25)], "elbl": [(0, 30), (1, 30)], "push": [(0, 0), (.5, .04), (1, 0)], "twist": [(0, -6), (.5, 4), (1, -6)]}),
    # o cavalariço escovando o cavalo: a escova sobe e desce no lombo
    "Npc_Escovar": (1.3, {
        "armr": [(0, 60), (.5, 100), (1, 60)], "elbr": [(0, 50), (.5, 20), (1, 50)], "arml": [(0, 15), (1, 15)],
        "twist": [(0, 10), (.5, -6), (1, 10)], "look": [(0, -10), (.5, 0), (1, -10)], "lift": _breath()}),
    # a irmã rezando: as mãos juntas no peito, a cabeça baixa
    "Npc_Rezar": (5.0, {
        "arms": [(0, 30), (1, 30)], "elbows": [(0, 95), (1, 95)], "look": [(0, 30), (1, 30)], "lean": [(0, 8), (1, 8)],
        "lift": [(0, -.03), (.5, -.05), (1, -.03)]}),
    # o guarda de vigia: parado firme, olha para um lado, para o outro
    "Npc_Vigiar": (8.0, {
        "arms": [(0, 8), (1, 8)], "lift": _breath(),
        "twist": [(0, 0), (.2, 0), (.3, 25), (.45, 25), (.55, 0), (.7, 0), (.8, -25), (.92, -25), (1, 0)]}),
}
