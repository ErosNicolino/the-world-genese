# Lugares marcantes, caminhos e raízes da Árvore do Mundo (07/11)

O pedido: "faça uns 6 espalhados no inicial bem grandes e marcantes e lindos" e "faça também essas raízes espalhadas
pelo mapa todo" (o item 5 do PDF do continente: lugares moldados à mão, com caminhos).

## Os 6 lugares (perto da cidade, 0,8 a 1,2 km)

| Lugar | Onde (m) | O relevo | As peças |
|---|---|---|---|
| Mirante da Cachoeira | (930, 30) | platô de 38 m com borda irregular, paredão, lago embaixo, rampa até o alto | cachoeira (esticada do alto até o lago), espuma, mirante em ruínas no alto |
| Colina do Anel de Pedras | (470, -800) | colina em terraços de 44 m, trilha subindo | anel de 12 pedras (algumas caídas), altar, duas vigas |
| Arco do Vento | (-470, -820) | crista de 34 m cortada por uma passagem de 22 m | arco de rocha ligando as duas paredes da passagem |
| Lago Espelho | (-1180, 120) | lago com ilha e 6 pedras para atravessar | torre em ruínas na ilha |
| Ponte de Raiz | (800, 470) | desfiladeiro com rio no fundo | uma raiz enorme atravessando (dá para andar por cima) |
| Ninho dos Cristais | (-20, -950) | cratera com borda e uma entrada | cristais verdes acesos, 7 menores em volta, luz verde |

Cada um tem nome na tela ao chegar ("lugar lendário"), alfinete no mapa depois de descoberto, clareira sem árvores e
monta a 1,5 km (é visto de longe).

## Os caminhos

De fora da cidade até a entrada de cada lugar, pelo trecho menos íngreme (A* no mapa), alisados. No chão é terra batida
e sem plantas (`world_flags.bin` = 4 → a camada Path do terreno).

## As raízes da Árvore do Mundo (50)

- 14 atravessando rios estreitos, como pontes naturais (dá para andar por cima);
- 36 saindo do chão, 4 por região, em três formas: o laço, a serpente e o feixe.

Ficam longe da cidade (mais de 650 m), dos lugares e uma da outra; montam a 600 m. Casca marrom com veios de seiva
dourada acesos.

## Paredões de pedra

Os paredões altos do terreno (mais de 1,9 m) agora são desenhados em faixas de uma célula, com rocha embaixo da borda de
grama: antes eram uma textura só esticada do alto até embaixo.

## Como refazer

1. O relevo, os caminhos e as raízes: `python Tools/World/StampMarvels.py` (o `BakeWorld.py` já chama no fim). Parte
   sempre do mapa sem os lugares (`world_height_base.bin`, `world_flags_base.bin`); a prévia sai em
   `Saved/WorldBake/previa_lugares.png`.
2. As peças: `Tools/Blender/MakeMarvels.py` (blocos com a cor no vértice) → `Data/World/Marvels/SM_*.glb`.
3. Para a Unreal (editor fechado): `Tools/ImportMarvels.py` → `/Game/Vortex/World/Marvels` (materiais M_MarvelBlock,
   M_MarvelGlow e M_MarvelWater).
4. A montagem: `AVortexLandmarks` (lê `world_features.json`: "marvels" e "roots").
5. Fotos: `Tools/RunWorldShots.ps1 -Only lugar_` (câmera alta olhando cada lugar; `Saved/Screenshots/mundo_lugar_*.png`).

Ajustar a posição de um lugar: a lista `MARVELS` no `StampMarvels.py` e rodar de novo (não precisa compilar).

## Fotos

![Os 6 lugares ao meio-dia](../Images/lugares_marcantes.png)

Da esquerda para a direita, de cima para baixo: Mirante da Cachoeira, Anel de Pedras, Arco do Vento, Lago Espelho, Ponte
de Raiz e Ninho dos Cristais.

![Duas raízes](../Images/raizes_arvore_do_mundo.png)

O mapa com os relevos e os caminhos (laranja):

![Prévia](../Images/previa_lugares.png)

## Conferido nas fotos e nos testes (07/11, noite)

![A cachoeira de perto e o herói atravessando a raiz do rio](../Images/cachoeira_e_raiz_andando.png)

- **Andar por cima:** `RunWorldShots.ps1 -Only lugar_andar` atravessa de uma margem à outra andando, e no meio confere
  que os pés estão acima da água. A Ponte de Raiz passou: 64 m, pés a 18,9 m com a água a 2 m. A raiz do rio também:
  pés a 5,5 m no meio.
- **A cachoeira:** não aparecia, porque o lago carimbado comeu a frente do paredão (a borda ficou 6 a 12 m para trás) e
  o jogo pegava a altura do alto já na beira do lago. Agora ele procura o ponto mais alto até 30 m atrás e a primeira
  borda nessa altura. A água ficou mais grossa (cobre a rampa de 6 m da queda), azul e mais opaca, com brilho próprio,
  para não sumir na névoa.
- **Os veios de seiva:** laranja mais escuro e brilho 2 (antes 3); não saem mais brancos.
- **As raízes de rio:** só onde as duas margens ficam a no máximo 30 m, com a raiz no meio. Antes uma caiu na beira de
  um lago largo.

## Ainda não conferido

- jogando de verdade (o usuário): o caminho até cada lugar, a subida da colina do Anel e a rampa do Mirante;
- de noite (a luz verde dos Cristais e os veios acesos).
