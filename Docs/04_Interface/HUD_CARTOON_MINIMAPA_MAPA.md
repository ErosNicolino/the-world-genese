# HUD cartoon, minimapa nítido e mapa do mundo em tela cheia

> "Agora arrume o mini mapa e as huds da tela, não combinam mais com o jogo, e o mini mapa e o mapa em si abrindo não
> têm qualidade nem foco real, tem que ser bom mesmo."

Antes de implementar, fiz um esboço em cima de uma foto do jogo (`Docs/Images/hud_esboco`). O minimapa e o mapa foram aprovados de primeira; a HUD foi aprovada depois de corrigir os painéis (o brilho vazava como uma faixa cinza).

## O que estava ruim

- **Minimapa sem foco:** ele recortava o mapa do continente inteiro, com 1 ponto a cada 3 m. Num alcance de 70 m eram ~47 pontos de lado a lado, esticados no círculo: borrado e quadriculado.
- **Mapa grande pequeno:** um quadrado no meio da tela, com zoom só no herói e sem arrastar. O que não foi explorado era pergaminho vazio, e os ícones e nomes se amontoavam.
- **HUD de outro estilo:** moldura azul-noite com dourado de MMO, enquanto as janelas já eram cartoon.

## O que entrou

### HUD no estilo das janelas
- Peças desenhadas por `Tools/MakeHudCartoon.py` e importadas por `Tools/ImportHudCartoon.py` para `/Game/Vortex/UI/HudCartoon`: painéis de 9 partes, barras, botões de borracha, aro, selos, teclas e ícones do mapa.
- Contorno escuro grosso, base mais escura e o brilho dentro da borda. Letra cartoon (Luckiest Guy) com contorno.
- **Cartão do herói:**
  - retrato redondo com aro e selo do nível;
  - nome, raça e classe;
  - vida (pisca quando baixa), mana e XP em barras arredondadas, com os números.
- **Efeitos ativos:** pílulas embaixo do cartão (bênçãos, regras do portal, contra-ataque, fonte, cidade, efeitos da classe).
- **Bússola em fita:** N vermelho; ícones da missão, da cidade, do grupo, das marcações e do seu marcador. Mostra a distância só da marca mais perto do centro.
- **Barra de chefe e semi-chefe:** painel vermelho com retrato e nível, e barra de vida.
  - Chefe: as 3 fases com selos I, II e III e o nome da fase.
  - Outros: o que a criatura está fazendo, em vermelho quando é o golpe que vem.
- **Missão acompanhada:** cartão com "JORNADA" ou "MISSÃO", o título e cada objetivo com caixinha (vazia, marcada ou "voltar").
- **Avisos de saque e XP:** cartões que deslizam da direita. "NÍVEL N" e o fim do Estádio usam a letra grande.
- **Barra de habilidades:**
  - botões de borracha com o ícone e a tecla no canto;
  - a recarga escurece o botão de cima para baixo, com os segundos;
  - trancadas: cadeado e "NV 10";
  - custo de mana em azul (vermelho quando falta);
  - poções Q e R em botões redondos com a quantidade;
  - cargas de esquiva em bolinhas.

### Minimapa nítido
- **Mapa local** de 512 x 512 blocos (384 m) em volta do herói, 1 ponto por bloco de 0,75 m: **4x mais detalhe em cada direção** que antes.
  - Montado em outra thread, em prioridade baixa; é refeito quando o herói se aproxima da borda.
  - Mostra o relevo em patamares, a água em dois tons com espuma e costa, as copas das árvores (cor de cada espécie: outono, cerejeira, neve, mata élfica...), telhados e pedras.
- **Gira com a câmera** (o que está à frente fica para cima) e o **N anda na borda**. Opção em Opções → Interface → "Minimapa gira com a câmera" (Não = norte sempre para cima).
- Material `M_HudMinimapCartoon` (recorte redondo + giro). A cor sai já com a gama da tela: sem ela, o minimapa ficava bem mais escuro que o mapa grande.
- **Marcas:** cidade, lugares e portais descobertos, moradores ("!" e "?" das missões), criaturas vistas (chefe com coroa), grupo, objetivo da missão e o seu marcador (os dois últimos ficam na borda quando estão longe). No centro, a seta do herói.
- **Embaixo:** o alcance (40/70/120 m), a região (ou "Cidade do Santuário") e o dia com a hora.

### Mapa do mundo em tela cheia (M)
- Abre centrado no herói, no zoom de região. O mouse aparece e a câmera e o herói param.
- **Navegação:** arrastar move o mapa; a roda dá zoom no ponto embaixo do mouse.
- **Três níveis de zoom:** Mundo, Região e Perto. No Perto entra o **detalhe por bloco**: 1024 x 1024 blocos (768 m), montado em outra thread, com o mesmo desenho do minimapa e a névoa.
- **Névoa:** o que não foi explorado fica sob **nuvens pintadas**, com borda ondulada e sombra, deixando ver de leve o contorno do mundo. O pergaminho vazio saiu.
- **Mar:** quando a vista é mais larga que o mapa, a sobra é mar (antes a borda da textura esticava em faixas).
- **Marcador:** clique direito marca um ponto (clique direito em cima dele tira). Ele aparece no mapa, no minimapa e na bússola.
- **Legenda com filtros** (clique para ligar e desligar): Missões, Moradores, Lugares, Portais, Chefes vistos, Grupo e Meu marcador.
- **Coluna da esquerda:** os 3 níveis de zoom (clicáveis), o explorado e **CENTRALIZAR** (volta ao herói).
- **Nomes conforme o zoom:** regiões de longe (claras onde você já foi), lugares e moradores de perto. Passar o mouse num ícone mostra o nome.
- **Régua de escala** com um número redondo (10 m a 2 km).
- Com o mapa aberto, o clique esquerdo não ataca mais (o mouse é do mapa).

## Desempenho

Benchmark de passeio, meio-dia, padrão "alto", 1280x720:

| | FPS | Quadro do jogo | Render |
|---|---|---|---|
| Sem HUD | 68,1 | 9,3 ms | 11,7 ms |
| HUD nova (2 rodadas) | 57,2 e 61,5 | 10,8–11,6 ms | 13,1–13,2 ms |

A HUD nova custa ~1,6 a 2,4 ms por quadro. A antiga custava ~1,3 ms numa medição de outro dia, e o benchmark varia uns ±3 FPS entre rodadas.

Otimizações já feitas:
- os textos cartoon esperam e saem todos juntos no fim, num lote só;
- a barra de habilidades desenha em 3 passadas (peças iguais seguidas);
- as pílulas usam 3 pedaços em vez de 9;
- o pintor do mapa local roda em prioridade baixa.

Se quiser apertar mais, o próximo passo é desenhar as partes paradas da HUD numa textura só e refazê-la só quando algo muda.

## Fotos

| Esboço aprovado | No jogo |
|---|---|
| ![](../Images/hud_esboco/esboco_hud.png) | ![](../Images/hud_cartoon/hud_luta_noite.png) |

| Minimapa (de perto) | Mapa: região | Mapa: mundo |
|---|---|---|
| ![](../Images/hud_cartoon/minimapa_perto.png) | ![](../Images/hud_cartoon/hudmapa_mapa_regiao.png) | ![](../Images/hud_cartoon/hudmapa_mapa_mundo.png) |

| Mapa: perto (detalhe por bloco, ruínas, santuário e marcador) | Marcador no mapa |
|---|---|
| ![](../Images/hud_cartoon/hudmapa_mapa_perto.png) | ![](../Images/hud_cartoon/hudmapa_mapa_marcador.png) |

O herói do teste é novo (explorado 0%), por isso o mapa está quase todo sob nuvens.

## Testes

- **`Tools/RunMapShots.ps1`** (`-VortexMapShots`, novo, na suíte). Leva o herói à beira de um lago perto da cidade e:
  - confere que o mapa local do minimapa foi montado;
  - abre o mapa e fotografa Região e Mundo;
  - dá zoom até Perto e confere que o detalhe por bloco foi montado;
  - marca um ponto e confere o marcador no mapa e no minimapa;
  - fecha o mapa.

  Passou, 0 falhas.
- **Outros testes:** `RunHudShot` (HUD, janelas e pausa), `RunMountShots` e `RunWorldShots` (os dois abrem o mapa) — resultados no fim desta conversa.
- **Não testado por máquina:** arrastar com o mouse, clicar nos filtros e no CENTRALIZAR. O jogo de teste roda sem mouse de verdade, então isso precisa ser testado à mão.

## Para testar no jogo

1. Ande pelo mundo: o minimapa mostra lagos, margens, árvores e casas nítidos e gira com a câmera.
2. Nas opções (Interface), troque "Minimapa gira com a câmera" para Não: o norte fica para cima.
3. Aperte M:
   - arraste para mover;
   - use a roda para dar zoom onde o mouse está (no Perto, aparecem os blocos);
   - clique direito para marcar um ponto, depois veja o marcador no minimapa e na bússola;
   - clique nos filtros da legenda e no CENTRALIZAR.
4. Lute com um semi-chefe ou chefe: a barra vermelha aparece no alto.
5. Use habilidades: a recarga escurece o botão com os segundos.

## Como refazer a arte

```
python Tools/MakeHudCartoon.py
UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho>/Tools/ImportHudCartoon.py -unattended -nullrhi
```
