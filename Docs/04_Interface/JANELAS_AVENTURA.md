# Janelas do herói refeitas: kit "Aventura"

Todas as janelas dentro do mundo foram refeitas com um kit de peças em pixel art. Isso vale para as abas Herói, Mochila, Técnicas, Classe, Alquimia e Missões, e também para a loja. As janelas agora usam moldura de madeira com ouro e rebites, fundo de pedra, fitas vermelhas nos títulos, abas de madeira, slots fundos e a letra cartoon (Luckiest Guy).

## O que mudou
- **Kit novo:** são 22 peças, geradas por `Tools/MakeUiKitAventura.py` em `Data/UI/KitAventura`.
  - Para importar: `Tools/ImportUiKitAventura.py`, que salva em `/Game/Vortex/UI/KitAventura` com filtro de pixel.
  - No código: `VortexUiKit::Aventura` (Frame, Stone, Panel, Slot, Card, Ribbon, Button, Bar…).
- **Herói ao vivo:** uma câmera filma o herói, que aparece na janela.
  - Material `M_UiDoll`, criado por `Tools/MakeUiDollMaterial.py`.
  - Para girar o herói, arraste com o mouse.
  - Ele aparece nas abas Herói e Mochila.
- **Herói:**
  - à esquerda: o herói, o nível, a barra de XP e o cartão da raça;
  - no meio: os atributos em 3 grupos (Ataque, Sobrevivência, Magia), com botões "+" dourados;
  - à direita: blocos de combate.
- **Mochila:**
  - os equipamentos ficam em volta do herói;
  - filtros e uma grade de 5×6 itens;
  - o detalhe mostra o item sob o mouse e a diferença para o item que está vestido;
  - clique direito ou duplo clique usa o item.
- **Técnicas:** são 4 cartões, e os trancados mostram o nível em que liberam. Os comandos aparecem com teclas desenhadas.
- **Classe:** cartão grande da classe; as 4 habilidades têm cadeado e "NÍVEL N".
- **Alquimia:** receitas em cartões, com os ingredientes e o botão "Preparar".
- **Missões:** a lista tem ! e ?. O detalhe mostra objetivos, recompensas e o botão "Acompanhar na tela".
- **Loja:** moldura igual às janelas, preço com moeda e botões cartoon.

## Antes e depois
| Aba | Antes | Depois |
|---|---|---|
| Herói | ![](../Images/janelas/antes_heroi.png) | ![](../Images/janelas/depois_personagem.png) |
| Mochila | ![](../Images/janelas/antes_mochila.png) | ![](../Images/janelas/depois_mochila.png) |
| Técnicas | ![](../Images/janelas/antes_tecnicas.png) | ![](../Images/janelas/depois_habilidades.png) |
| Classe | ![](../Images/janelas/antes_classes.png) | ![](../Images/janelas/depois_classe.png) |
| Alquimia | — | ![](../Images/janelas/depois_alquimia.png) |
| Missões | — | ![](../Images/janelas/depois_missoes.png) |

## Testes
- `RunHudShot`: passou e fotografa as 6 abas.
- `RunShopSmoke`: passou.
- A suíte completa não foi rodada.

## Para testar no jogo
- Abra a janela com I ou Tab.
- Arraste o herói para girar.
- Passe o mouse nos itens da mochila e compare com o que está vestido.
- Use clique direito em uma poção.
- Gaste pontos com o "+".
- Abra a loja de um comerciante.
