# Fauna e flora: pássaros, peixes e plantas vivas

> "Pode fazer então pelos pássaros e peixes, e faça as plantas de diferentes tipos etc."

Antes, o mundo só tinha três animais calmos (coelho, cervo e cavalo) e, no ar, borboletas, sementes e vaga-lumes. A flora já era rica (19 árvores e ~45 plantas), mas cada espécie era a mesma cópia repetida e nenhuma planta reagia a quem passava.

## O que entrou

### Pássaros (um por região)

| Região | Pássaro | Onde fica |
|---|---|---|
| Valdora, Aurivale | Pardal | chão e céu |
| Brasafenda | Tiziu-de-brasa (vermelho, cauda acesa) | chão e céu |
| Raikor | Andorinha-da-tempestade (azul, cauda em V) | chão e céu |
| Nivaluz | Pardal-da-neve | chão e céu |
| Solcânion | Abutre, planando bem alto | céu |
| Lodonegro | Garça (na água rasa) e corvos no céu | água rasa e céu |
| Morthúria | Corvo (aparece também de noite) | chão e céu |
| Verdalume | Pássaro-lume (azul-turquesa, cauda acesa) | chão e céu |
| Mar e costa | Gaivota | céu |

- **No céu**, até 3 bandos dão voltas devagar. Os pássaros pequenos batem as asas e planam em rajadas. Os grandes (abutre, gaivota, garça) planam quase sempre.
- **No chão**, até 3 grupos de 3 a 6 pássaros pulam, bicam o chão, olham em volta e piam.
  - Quando alguém chega a ~6,5 m, **o bando inteiro levanta voo** com bater de asas, fugindo para o lado oposto.
  - Depois pousam de novo em outro lugar, nascendo atrás da câmera para não surgirem do nada na frente.
- **De noite** somem, menos os corvos.

### Peixes (um por região)

| Região | Peixe |
|---|---|
| Valdora, Raikor | truta (às vezes carpa-dourada) |
| Aurivale, Solcânion | carpa-dourada |
| Nivaluz | peixe-gelo |
| Lodonegro, Morthúria | bagre-do-lodo (com bigodes) |
| Verdalume | peixe-lume (barbatanas acesas) |
| Brasafenda | peixe-brasa |
| mar | sardinhas, em cardumes grandes |

- Cardumes de 3 a 6 peixes (sardinhas: 8 a 14) nadam na água funda, perto da superfície, onde dá para ver.
- **Fogem** de quem chega a ~5 m.
- De vez em quando **um peixe pula** para fora e cai com respingo e um "plic".
- 1 cardume em 20 tem um **peixe dourado**.

### Plantas

- **Tamanhos diferentes:** flores, cogumelos, samambaias e moitas variam de 0,78x a 1,28x, um pouco mais baixas ou mais esticadas e inclinadas até 5°. Cada lugar é sempre igual. Pedras e construções não mudam.
- **Cores diferentes:** tulipa, sino, lavanda, papoula, flor alpina, flor élfica e cogumelo vermelho saem em 4 cores de pétala (a original e 3 variações). O caule continua verde.
- **Dormideira:** as flores pequenas e as samambaias **se fecham** (encolhem para a base) quando alguém pisa perto (~1,7 m) e abrem de novo quando a pessoa se afasta. Vale para tulipa, margarida, sino, lavanda, papoula, dente-de-leão, trevo, samambaias e flores mágicas.
- **Plantas mágicas acendem:** cogumelo aceso, lanterna do pântano, flor das almas, broto de cristal, flor de brasa, flor de geada, samambaia da lua, flor élfica e cristal da tempestade brilham mais (e pulsam) quando alguém chega a ~4,5 m.
- **Esporos:** passando correndo por cima de um cogumelo, sai uma nuvem de esporos. Os cogumelos acesos e os do pântano soltam esporos acesos.
- **Plantas da água:**
  - Vitórias-régias na água rasa, algumas com flor. Há bem mais no pântano; em Verdalume a flor é acesa; nenhuma no gelo, na brasa ou no mar.
  - Algas no fundo: verdes, escuras no lodo, vermelhas na brasa e acesas em Verdalume.

### Sons novos

3 pios de passarinho, grasnado do corvo, grito da gaivota, bater de asas do bando e "plic" do peixe.

Na primeira versão eram todos sintetizados. Depois viraram gravações: pios, corvo e asas de gravações CC0, e o peixe do SONNISS. Só a gaivota continua sintetizada. Detalhes em [SONS_GRAVADOS_E_MIXAGEM](SONS_GRAVADOS_E_MIXAGEM.md).

## Como funciona

- **`AVortexWildlife`** (`World/VortexWildlife.h/.cpp`), criado junto da `AVortexForestLife`. É só enfeite: cada máquina tem os seus bichos, sem nada pela rede. `-VortexNoWildlife` desliga.
  - Pássaros, peixes e plantas da água são blocos montados no C++ (material `/Game/Vortex/Life/M_LifeBlocks`). A cor vem do vértice; vértice com alfa 0 brilha.
  - As asas e a cauda do peixe são cópias à parte que giram a cada quadro.
- **`M_VoxelProp`** ganhou 3 chaves (`Tools/ImportWildlifeAssets.py`), desligadas por padrão e ligadas só nas espécies certas:
  - `Shy` (dormideira): usa os pés que a grama já recebe (`MPC_GrassStep`).
  - `GlowNear`: as plantas mágicas acendem perto de alguém.
  - `Variety`: a cor das pétalas, pelo parâmetro `PetalHue`. O `AVortexTerrainStreamer` sorteia a cor pelo lugar e põe cada cor num lote à parte.
  - O script também tira os nós soltos que rodadas antigas deixavam no material (eram 220).
- **Por que a cor é por lote:** os dados por cópia (PerInstanceRandom, posição da cópia, CustomData e VertexInterpolator) chegavam **iguais para todas as cópias** na cor das plantas do terreno (Nanite). Só o movimento (WPO) recebia o valor certo. Por isso a dormideira funcionou de primeira e a cor não.
  - Achado importante: a variação de tom por cópia que `ImproveVegetationMaterials.py` colocou antes provavelmente nunca funcionou pelo mesmo motivo.
- **Lagos não são mar:** o mapa das regiões marca lagos e rios como "mar". Para escolher pássaro, peixe e planta, a água usa a região da terra em volta (até 60 m).

## Desempenho

Mesmo passeio do benchmark (`RunTerrainBench -Profile alto -Hour 12`):

| | FPS | Quadro do jogo |
|---|---|---|
| Sem a fauna nova | 73,6 | 7,75 ms |
| Com a fauna nova | 72,6 | 8,20 ms |

Custo de ~0,2 ms por quadro, dentro do ruído da medição.

## Fotos

| Sem os bichos | Com os bichos (pardais no capim) |
|---|---|
| ![](../Images/fauna/fauna_vale_passaros_antes.png) | ![](../Images/fauna/fauna_vale_passaros.png) |

| Água sem os bichos | Cardume no lago |
|---|---|
| ![](../Images/fauna/fauna_vale_agua_antes.png) | ![](../Images/fauna/fauna_vale_agua.png) |

| Pássaros-lume em Verdalume | Peixes-lume em Verdalume | Garças e corvos no pântano |
|---|---|---|
| ![](../Images/fauna/fauna_verdalume_passaros.png) | ![](../Images/fauna/fauna_verdalume_agua.png) | ![](../Images/fauna/fauna_pantano_passaros.png) |

| Vitória-régia com flor | O bando levantando voo | Esporos de cogumelo aceso |
|---|---|---|
| ![](../Images/fauna/fauna_pantano_plantas_agua.png) | ![](../Images/fauna/fauna_susto.png) | ![](../Images/fauna/fauna_esporos.png) |

| Lavandas em duas cores (herói longe) | Herói em cima: as flores se fecham |
|---|---|
| ![](../Images/fauna/fauna_flor_longe.png) | ![](../Images/fauna/fauna_flor_perto.png) |

## Testes

`Tools/RunWildlifeSmoke.ps1` (`-VortexWildlifeSmoke`, entra na suíte) visita um lago perto da cidade, o pântano de Lodonegro e Verdalume. Em cada lugar:
- tira foto sem e com os bichos;
- confere pássaros no chão e no céu, o cardume, as vitórias-régias e as algas.

Depois confere que o bando levanta voo quando o herói chega perto e que os esporos saem. Por fim fotografa uma flor com o herói longe e em cima dela.

Resultado: **passou duas vezes seguidas, 0 falhas**.

## Também nesta rodada

- **"O menu continua piscando quando coloco pontos":** cada "+" refazia a janela do herói inteira.
  - Agora os números da página do herói (atributos, pontos livres, "No combate" e o aviso da aba) são lidos a cada quadro.
  - Logo depois de um "+" a janela não se refaz.
  - O teste da HUD (`RunHudShot`) gasta 2 pontos com a janela aberta e confere que ela foi refeita 0 vezes e que os pontos caíram de 2 para 0. Passou.
- **"As aranhas estão penduradas no céu":** as aranhas saíram do mundo aberto por enquanto (`bWorldSpiders = false` em `VortexCreatureSubsystem.cpp`).
  - As das teias não nascem mais.
  - Os portais passam a ser guardados só por esqueletos.
  - A espécie continua existindo para as dungeons e os testes.

## Para testar no jogo

1. Ande pelo campo perto da cidade: deve haver pardais pulando no capim. Chegue perto e eles voam.
2. Olhe um lago de cima da margem: deve haver peixes nadando, e de vez em quando um pula.
3. Pise numa flor ou samambaia: ela se fecha. Ao se afastar, abre de novo.
4. Corra por cima de um cogumelo: sai uma nuvem de esporos.
5. Vá ao pântano (Lodonegro) e a Verdalume para ver garças, vitórias-régias e os bichos acesos.
6. Gaste pontos de atributo com a janela do herói aberta: a janela não deve piscar.

## Ideias para depois

- Pássaros pousando nas árvores e nos telhados.
- Corujas à noite.
- Bichos pequenos: esquilo, caranguejo na praia, sapinhos.
- Pesca.
- Um animal hostil por região: urso branco em Nivaluz, crocodilo no pântano.
