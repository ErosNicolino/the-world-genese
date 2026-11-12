# Pontos de interesse e portal de dungeon

Atende a dois pedidos: "tem pouca construção, ruínas e essas coisas espalhadas pelo jogo" e "crie também um efeito de portal no jogo, que vou fazer umas dungeons tipo Solo Leveling".

## Pontos de interesse (`AVortexPointsOfInterest`)
- **Onde aparecem:** cerca de 1 em cada 2,5 quadras de 72 m ganha um ponto. O sorteio vem da posição (`PoiAt`), então todos os jogadores veem o mesmo.
- **Onde não aparecem:** a menos de 150 m da cidade, perto dos marcos (vilas, Yggdrasil, covis), das pontes e cabanas, em quadra de acampamento de bandidos, na água e em barranco.
- **Como são montados:** em blocos com colisão quando alguém chega a 200 m, e desmontados a 280 m. Cada tipo de construção fica no código dele, em `Build`.
- **No mapa:** todos aparecem com o nome quando a região é descoberta.

| Tipo | O que tem | Para que serve |
|---|---|---|
| Ruínas | piso quebrado, muros com falhas, colunas (uma caída), entulho | baú escondido (raro em 20%) **ou** pedra com inscrição: E lê uma frase da história do mundo |
| Torre de vigia | torre de 6 m com janelas, escada de 20 degraus de 30 cm em volta, ameias e bandeira | no topo o mapa revela 300 m em volta (normalmente 110 m) |
| Santuário | plataforma de dois degraus, obelisco com runa, cristal flutuando e girando, luz pulsando | E: bênção por 5 min, a mesma em cada santuário. **Força**: +25% ataque. **Vida**: +25% vida máxima. **Sorte**: +10% crítico e +25% saque e moedas. Recarga de 10 min. |
| Mina abandonada | morro de pedra com a boca do túnel escura, escoras, trilho e carrinho de minério | baú guardado por um bando de 3 a 4 goblins (chefe, brutamontes, batedores) |
| Cemitério | muro baixo com portão, lápides, cruzes, árvore seca, mausoléu | baú guardado por um morto-vivo de dia; à noite levantam mais 3 (com um arqueiro esquelético) |
| Caravana tombada | carroça quebrada, roda caída, caixotes, barris e sacos | baú pequeno sem guarda |
| Cais de pesca | píer de tábuas entrando na água, estacas, barco a remo, barris, lampião | decoração (é onde a pesca vai entrar) |

**O que aparece em cada região:**
- **Raikor:** mais minas e torres.
- **Morthúria e o pântano:** mais cemitérios.
- **Aurivale:** mais caravanas.
- **Verdalume:** mais ruínas e santuários.
- **Com água por perto:** cais.

**Cores de cada região:**
- **Vale:** pedra clara.
- **Brasafenda:** basalto com brasa.
- **Raikor:** granito.
- **Nivaluz:** pedra branca e gelo.
- **Solcânion:** arenito.
- **Aurivale:** pedra dourada.
- **Lodonegro:** pedra com lodo.
- **Morthúria:** pedra roxa.
- **Verdalume:** pedra élfica com musgo.

**Mortos-vivos (novos):**
- **Tipos:** `Undead` ("MORTO-VIVO ERRANTE", espada) e `UndeadArcher` ("ARQUEIRO ESQUELÉTICO", arco).
- **Visual:** o boneco do aventureiro na raça morto-vivo do criador de heróis, em trapos escuros (`UVortexDressComponent::UndeadLook`).
- **Luta:** são mais lentos e mais duros que os bandidos.

**Baús:** os baús guardados só abrem com os guardas por perto vencidos ("O baú ainda está guardado! (N de pé)"). Sempre dão equipamento e mais moedas. Ficam marcados como abertos no save de cada herói, como os das cabanas.

## Portal de dungeon (`AVortexPortal`)
- **O visual:**
  - um rasgo oval em pé (3,2 × 5,2 m);
  - o redemoinho em pixels grandes girando para dentro, em duas camadas que giram em sentidos contrários (`M_PortalSwirl`);
  - a borda de blocos acesos tremendo e 80 faíscas em blocos puxadas em espiral para o centro;
  - um círculo girando no chão e 12 pedras flutuando em volta;
  - uma luz pulsando na cor do ranque.
- **O som:** o portal abre "rasgando" e depois fica um zumbido em laço. Os sons misturam o pacote Sci-Fi Sounds da Kenney (CC0, em `Data/Sfx/PortalKenney`) com um zumbido e um brilho sintetizados aqui (`Tools/MakePortalAssets.py`).
- **Cor, brilho e tamanho por ranque:** quanto mais escuro, mais difícil. O vermelho é o mais difícil e o maior.

  | Ranque | Cor | Tamanho | Quantos no mundo | Monstros em volta |
  |---|---|---|---|---|
  | E | azul-claro (o brilho forte) | 1x | 40% | 2 |
  | D | azul | 1,15x | 25% | 3 |
  | C | índigo | 1,3x | 16% | 4 |
  | B | roxo | 1,5x | 10% | 5 |
  | A | vinho | 1,75x | 6% | 6 |
  | S | **vermelho-sangue** | 2,2x | 3% | 7 |

- **Vermelho (S):** quem entra não sai até vencer a dungeon. Nos outros dá para voltar (`AVortexPortal::IsNoEscape`). O aviso na tela antes de entrar diz isso.
- **Onde estão:**
  - um de ranque E a ~95 m do centro da cidade, a leste (`DungeonId = dungeon_inicial`);
  - outros espalhados pelo continente, cerca de 1 a cada 14 quadras (`GateAt`). Ranques altos aparecem um pouco mais longe da cidade.
  - O servidor abre cada portal quando alguém chega a 250 m. Todos aparecem no mapa ("Portal C", "Portal VERMELHO"...).
- **Guardas:** em volta de cada portal há aranhas e esqueletos. Quanto maior o ranque, mais deles e mais fortes (+3 níveis por ranque).
- **Entrar:** com E perto do portal. Se ele tiver `Destination`, leva até lá. Sem destino, avisa que "a dungeon do outro lado ainda está se formando".
- **Para as suas dungeons:** dê um `DungeonId` e um `Destination` ao portal. Para abrir um portal por código, use `AVortexPortal::Open(World, chão, giro, ranque, dungeon)`.

## Monstros dos portais
- **Aranha do Portal (`Spider`):**
  - **O modelo:** feito no Blender (`Tools/MakeSpiderBlender.py`), 219 blocos.
    - **Corpo:** cefalotórax com carapaça, quelíceras e presas de osso, palpos e 8 olhos vermelhos acesos.
    - **Abdômen:** listras em V roxas, rachaduras de energia do portal em ciano e fiandeiras.
    - **Pernas:** 3 segmentos com juntas, faixas de tarântula, pelos e garras.
  - **Animações:** parada, andar, ataque (empina e crava as presas), bote, golpe e morte (vira de patas para cima).
  - **Como luta:** é rápida, morde e dá bote.
  - **Escala:** sobe paredes, troncos e barrancos durante a perseguição e passa por cima. Num teto (nas dungeons), anda de cabeça para baixo atrás do herói e **desce no fio** em cima dele.
  - **Pendurada:** perto das teias algumas ficam penduradas no fio, lá no alto, e caem quando alguém passa embaixo.
- **Esqueleto Guerreiro (`Skeleton`, espada e escudo) e Esqueleto Arqueiro (`SkeletonArcher`):** o boneco humanoide na raça morto-vivo, todo cor de osso (`UVortexDressComponent::SkeletonLook`).

## Teias
- **Onde ficam:** 2 a 4 juntas, em pé (entre dois galhos secos) ou no chão (`WebsAt`). Aparecem mais nas matas, no pântano e em Morthúria.
- **Encostou, ficou preso:** anda a 12% da velocidade e não pula. Aparece o aviso "Preso na teia! Ataque para cortar".
- **Cortar:** 3 golpes cortam a teia (o golpe corta a que estiver na frente). Ela volta a crescer em 90 s.

## Alinhamento com o mundo
Cada bloco das construções tem 75 cm, o mesmo tamanho do bloco do terreno. O centro de cada construção fica no meio de uma célula do chão, e o giro é sempre de 90 em 90 graus, então as bordas acompanham a grade do mapa. O material `M_PoiBlocks` mostra a cor de cada bloco; antes, o material do terreno distante puxava tudo para a cor da terra.

## Dungeons dos portais (em construção)

Por enquanto, entrar num portal só mostra o aviso **"em construção"**. O E perto do portal mostra
"Portal • ranque X • EM CONSTRUÇÃO" e não leva a lugar nenhum.

Parte do código das dungeons já está pronta e compila, mas o ator ainda **não é criado** no mundo:

- **`AVortexDungeons`** (`World/VortexDungeons.h/.cpp`): cada portal leva a um mapa diferente, que sai do id `dg_<semente>_<ranque>_<nível>`.
  - Biomas por ranque:

    | Ranque | Biomas |
    |---|---|
    | E | caverna, gruta de cristais |
    | D | mina, caverna alagada |
    | C | ninho de aranhas, catacumbas |
    | B | selva, pântano tóxico |
    | A | gelo, vulcão, cidade afundada |
    | S | abismo vermelho, trono do portal |

  - Salas: de 5 (ranque E) a 13 (ranque S), ligadas por corredores.
  - Em cada dungeon: chefe, baú do tesouro e baú do chefe, e aranhas penduradas no teto das cavernas.
  - No portal vermelho, o portal de volta só aparece depois que o chefe cai.
- **Regras dos portais**: sem poções, escuridão, sem regeneração, vida frágil, fúria, inimigos rápidos, bênção de XP, sorte, desafio do tempo e eco.
  - As regras já estão ligadas na ficha do herói (`SetDungeon`/`HasDungeonRule`) e na HUD.
  - Elas só valem quando a dungeon estiver aberta.
- **`AVortexMonster::SetDungeon`**: o monstro da dungeon não espera o terreno (a dungeon fica fora do mapa). Também aplica os multiplicadores de fúria e velocidade.
- **Teias fixas**: `AVortexPointsOfInterest::AddPinnedWebs`.

Para ligar, falta:

- criar o `AVortexDungeons` no `VortexTerrainStreamer`;
- chamar `Dungeons->Enter` no `ServerEnterPortal`;
- testar, tirar fotos e escrever o documento de ideias de monstros por bioma.

## Fotos
| Portal | Ruínas | Torre | Santuário |
|---|---|---|---|
| ![](../Images/poi/poi_portal.png) | ![](../Images/poi/poi_ruinas.png) | ![](../Images/poi/poi_torre.png) | ![](../Images/poi/poi_santuario.png) |

| Mina | Cemitério | Teias (com aranha no fio) | Monstros do portal |
|---|---|---|---|
| ![](../Images/poi/poi_mina.png) | ![](../Images/poi/poi_cemiterio.png) | ![](../Images/poi/poi_teia.png) | ![](../Images/poi/poi_monstros.png) |
## Comandos de teste (chat com T)
- `/portal [E-S]`: abre um portal na sua frente.
- `/poi [ruinas|torre|santuario|mina|cemiterio|caravana|cais]`: leva você ao mais perto daquele tipo.

## Testes
`Tools/RunPoiSmoke.ps1`:
- confere que existe um ponto de cada tipo;
- fotografa todos e o portal (`Saved/Screenshots/poi_*.png`);
- confere a bênção do santuário, o topo da torre, os baús guardados da mina e do cemitério e o baú da caravana;
- faz nascer a aranha e os esqueletos;
- confere que a teia prende e que 3 golpes soltam.
