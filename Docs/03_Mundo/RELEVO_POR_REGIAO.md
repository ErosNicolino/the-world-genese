# Relevo por região, erosão e rios que serpenteiam (08/11)

O pedido: "os relevos estão bem simples, não está igual às imagens". Tudo é feito no `Tools/World/BakeWorld.py` (só dados:
não precisa compilar). O mapa de antes está guardado em `Saved/WorldBake/antes_relevo_0711/`.

![Antes e agora (sombreado, colorido por região)](../Images/relevo_antes_depois.png)

## Cada região com a sua cara

| Região | O relevo |
|---|---|
| Planícies de Valdora | campos ondulados e alguns platôs com borda de pedra |
| Brasafenda | basalto em degraus (patamares de 7 m) e arestas quebradas; o vulcão continua |
| Picos de Raikor | picos e arestas afiadas (cristas em várias escalas), até ~230 m |
| Nivaluz | colinas largas e redondas, como gelo antigo, e alguns picos |
| Solcânion | mesas de paredão reto em camadas de 6 m e morros isolados de topo plano |
| Aurivale | pradaria de colinas suaves e compridas |
| Lodonegro | pântano baixo com montinhos e ilhas |
| Morthúria | arestas, agulhas de pedra (até 30 m) e buracos afundados no chão |
| Verdalume | morros grandes e redondos de floresta |

As feições são torcidas por um ruído largo, então nada sai em bolha redonda nem em linha reta.

## Erosão

Os córregos pequenos, que ainda não são rio (o mesmo cálculo de para onde a água escorre), cavam sulcos em V que descem
das montanhas até os rios. São mais fundos onde o morro é alto (até ~20% da altura). Param 1 m acima da água para não
virar poça, e não entram nos lugares construídos.

## Rios e vales

- **Rios:** o traçado é torcido por dois ruídos (curvas largas de até ~40 m para cada lado e voltas menores). Antes eram
  retas de 45° com cantos arredondados. É torcida a distância até o rio, não a linha, então o rio nunca se parte.
- **Vales:** a largura muda ao longo do rio (35 a 95 m) e a borda é torta e arredondada. Antes todo vale tinha 70 m e uma
  quina, e de cima pareciam pílulas.

## Fotos (meio-dia, câmera alta a 220 m de cada região)

![As 9 regiões](../Images/relevo_regioes.png)

De cima para baixo, da esquerda para a direita: Valdora, Brasafenda, Raikor, Nivaluz, Solcânion, Aurivale, Lodonegro,
Morthúria e Verdalume. A névoa do jogo apaga o fundo; o mapa sombreado acima mostra melhor.

`Tools/RunWorldShots.ps1 -Only relevo_` refaz as fotos.

## Conferido

- O passeio do continente inteiro (`RunWorldShots.ps1`): de pé em todas as regiões, rios, horizontes, vilas, covis, a
  ponte e a cabana com baú.
- Os 6 lugares marcantes foram carimbados de novo por cima do mapa novo. A Ponte de Raiz e uma raiz de rio continuam
  dando para atravessar andando.
- As raízes de rio mudaram de lugar (os rios mudaram). O teste agora lê a posição delas no `world_features.json`.

## Mudou junto

As pontes (60) e as cabanas (70) são sorteadas de novo a cada mapa, porque dependem dos rios: estão em outros lugares.

## Não conferido

- jogando de verdade (o usuário): subir as mesas e os picos, e os rios novos de perto;
- o desempenho com o relevo mais alto em Raikor (não medi o FPS).
