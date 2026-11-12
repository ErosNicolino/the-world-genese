# Abertura em HQ: "Acorde"

A história do mundo contada em **24 quadros de HQ** (2 folhas de 12) ao som da canção **"Acorde"**, uma balada orquestral de fantasia, épica e sombria, com 6 min 13 s. A câmera vai de quadro em quadro **no tempo da música**: cada quadro chega quando o trecho dele começa a ser cantado. A história é a de [HISTORIA_DO_MUNDO](HISTORIA_DO_MUNDO.md): Gênese, a Yggdrasil, o Vórtice, os nove Selos, a estrela que caiu, Azhûr, o vale ferido e os Despertos.

## Quando aparece
- **Herói novo** acordando no Estádio do Despertar: a HQ abre sozinha, uma vez por herói neste computador (fica guardado em `IntroSeen`, nas opções locais).
- **Menu de pausa → A HISTÓRIA**: assiste de novo quando quiser.
- **Só o botão PULAR** sai da HQ: vai para o fim, com a música sumindo. Teclas e cliques fora dele não fazem nada (27/10: o herói novo chegava ao mundo apertando espaço e clicando, cada toque passava um quadro e a HQ acabava em segundos).
- **O mundo fica quieto** enquanto a HQ toca: a música do jogo abaixa, o ambiente (pássaros, vento, água) some e os efeitos ficam quase mudos (`VortexAudio::SetCutscene`). A canção é da categoria Música, então segue o volume de música das opções.

## No multiplayer
**Cada jogador vê a sua HQ.** Ela é só da máquina de cada um: a tela, o relógio, a canção e o silêncio do mundo não passam pela rede. Quem entra num mundo vê a própria abertura quando o herói novo dele acorda, sem pausar o jogo nem mudar nada para os outros. Pular ou fechar também só vale para quem apertou. Enquanto alguém assiste, o herói dele fica parado no Estádio, como se estivesse lendo.

Testado com `Tools/RunIntroNetworkSmoke.ps1` (anfitrião com `?listen` e um cliente pelo IP, agora na suíte):
- os dois abrem a HQ em momentos diferentes, com 2 jogadores conectados;
- no mesmo instante, um estava aos 8,9 s e o outro aos 3,8 s (cada um com o seu relógio);
- o anfitrião fechou a dele e a do cliente continuou aberta;
- depois o cliente fechou a sua.

**Passou.**

## A animação
- **Começa no escuro**: durante a introdução instrumental (~11 s) a tela fica preta, com um fio de luz verde respirando no meio.
- Logo antes do primeiro verso, a **folha 1 chega deslizando e girando** sobre a mesa (com sombra, saindo do preto), e os quadros **"pintam" um a um**: o papel some de cima para baixo, com uma borda de tinta. O zoom no primeiro quadro começa junto com "Antes do nome…".
- A **câmera desliza até o quadro** do trecho cantado e se aproxima devagar (5%), puxando para um canto diferente em cada quadro. Os outros quadros escurecem.
- **Legenda de HQ, cantada**: caixa creme com contorno de tinta, faixa laranja e sombra dura, com um verso por linha, centralizado, na letra cartoon. Cada palavra aparece **letra por letra no momento em que é cantada** (o tempo de cada palavra vem da transcrição). A linha já fica no lugar dela e as letras surgem, sem a frase ficar andando. Sem a canção, vira máquina de escrever.
- **Efeitos por quadro**:
  - **brilho**: pulso verde-jade (raízes, Selos, cristais, a alma que volta);
  - **gira**: a folha oscila de leve (o céu se retorcendo, o Vórtice);
  - **clarão + tremor**: a estrela rasgando a terra;
  - **tremor**: Azhûr e as feras.
- Entre as folhas, **a folha 1 vira** (encolhe presa pela borda, escurecendo) e mostra a folha 2 por baixo, que pinta os quadros dela.
- **No "Acorde"**: a câmera mergulha nos olhos do herói, tudo clareia em branco, aparece **ACORDE** e o branco some, devolvendo o jogo.

## Os quadros e a música

| Quadro | Começa em | Primeiro verso | Efeito |
|---|---|---|---|
| 1 | 0:14.4 | Antes do nome, antes da dor, | — |
| 2 | 0:27.8 | E em seu coração, sob a luz primordial, | — |
| 3 | 0:42.2 | Suas raízes cruzavam a imensidão, | brilho |
| 4 | 0:54.7 | Onde ela fluía, a vida florescia, | — |
| 5 | 1:12.7 | Mas sobre a copa, o céu se retorceu, | gira |
| 6 | 1:26.4 | Chamaram de Vórtice a ferida no céu, | gira |
| 7 | 1:46.9 | Então os antigos, temendo o fim, | — |
| 8 | 1:54.3 | Nove Selos… nove destinos… | brilho |
| 9 | 2:24.5 | Mil anos se passaram, Valdora adormeceu, | — |
| 10 | 2:37.4 | Até que, vinte invernos atrás, | — |
| 11 | 2:58.7 | Rasgou a terra, fez a Caldeira despertar, | clarão + tremor |
| 12 | 3:06.6 | Azhûr! O Colosso despertou! | tremor |
| 13 | 3:13.4 | Azhûr! E em seu peito o fogo se acendeu! | tremor |
| 14 | 3:26.1 | Desde então, a seiva foge para o céu, | brilho |
| 15 | 3:32.6 | As feras já não conhecem quem são, | tremor |
| 16 | 3:38.6 | Nas muralhas, a guarda se ergueu, | — |
| 17 | 3:53.6 | Sonham com fogo, sonham com o fim… | gira |
| 18 | 4:07.7 | Mas nem tudo o que atravessa é levado, | brilho |
| 19 | 4:33.8 | Uma luz atravessa o véu, | — |
| 20 | 4:47.3 | Sem nome, sem memória, | — |
| 21 | 5:07.7 | Os antigos os chamaram Despertos, | brilho |
| 22 | 5:18.3 | sem se perder na corrupção. | tremor |
| 23 | 5:27.6 | Você é um deles… | — |
| 24 | 6:01.0 | Acorde. | fim em branco |

Os tempos saem sozinhos: o `faster-whisper` transcreve a canção com o tempo de cada palavra (`Data/Narracao/hq/acorde_palavras.json`), e o `Tools/MakeIntroComic.py` procura o começo de cada trecho, em ordem. Ele tolera os erros da transcrição: "vorte se" é Vórtice, "celo" é Selo e "a cor de" é Acorde. Depois casa cada palavra da letra com a ouvida (campo `sung` do `abertura.json`: início e fim de cada palavra); uma palavra escrita pode ser 2 ou 3 ouvidas, e as que não acham par ficam entre as vizinhas.

## Fotos

| Os quadros pintando | A folha pronta | Legenda no começo do verso | Legenda mais adiante |
|---|---|---|---|
| ![](../Images/abertura/2_pinta.png) | ![](../Images/abertura/3_pagina.png) | ![](../Images/abertura/4_quadro_legenda.png) | ![](../Images/abertura/5_legenda_cantada.png) |

A legenda de perto: só o que já foi cantado aparece ![](../Images/abertura/legenda_cantada_zoom.png)

| Clarão da estrela | A folha virando | O herói | O fim |
|---|---|---|---|
| ![](../Images/abertura/6_impacto.png) | ![](../Images/abertura/8_vira.png) | ![](../Images/abertura/10_heroi.png) | ![](../Images/abertura/12_acorde.png) |

Sequência inteira do teste: [sequencia.png](../Images/abertura/sequencia.png).

## Arquivos
- **Fontes:**
  - `Docs/Midia/Abertura/Abertura_Folha_1.jpg` e `Abertura_Folha_2.png`: as 2 folhas (4 × 3 quadros), geradas no ChatGPT com um prompt por folha no estilo combinado (quadrinho europeu à mão, nanquim, guache e papel);
  - `Docs/Midia/Abertura/Acorde.mp3`: a canção (feita no Suno).
- **`Tools/MakeIntroComic.py`**:
  - a letra de cada quadro e o efeito (lista `STORY`);
  - copia as folhas para `Data/UI/Abertura/folha_N.png` (amplia se a imagem vier com menos de 1600 px);
  - acha os quadros pelas sarjetas;
  - converte a canção para `MUS_Abertura.wav`;
  - acha os tempos;
  - grava `Data/UI/Abertura/abertura.json`, com o tamanho real das folhas (a textura importada pode ser arredondada).
- **`Tools/ImportIntroComic.py`**: as folhas viram `/Game/Vortex/UI/Abertura/T_Folha_N` (interface, com mipmaps) e a canção vira `/Game/Vortex/Audio/Music/MUS_Abertura`.
- **`VortexIntroComic.{h,cpp}`** (Framework): a tela.
  - Monta o roteiro pelos tempos da canção (`BuildFromSong`).
  - O tempo vem do relógio do sistema, para não atrasar em relação à música.
  - Sem a canção, cai num modo com o tempo de leitura e o tema do jogo baixinho.
- Onde ela abre:
  - `VortexAimHUD::DrawStadium`: herói novo;
  - `VortexPauseMenu`: o botão A HISTÓRIA;
  - `UVortexMusicSubsystem::SetDucked`: abaixa a música do jogo.

## Testes
- **`Tools/RunIntroShots.ps1`** (`-VortexIntroShots`, agora na suíte): abre a HQ, fotografa e confere:
  - que a introdução fica no escuro;
  - a folha chegando e pintando;
  - a câmera no quadro 2 com a legenda aparecendo junto com o canto;
  - o clarão do quadro 11;
  - a folha virando;
  - a folha 2 pintando;
  - o herói;
  - o branco e o "ACORDE";
  - que ela fecha sozinha e devolve o jogo.

  **Passou, 0 falhas.** O teste mede o tempo pelo relógio do sistema, porque o jogo de teste roda com passo fixo, mais rápido que o tempo real.
- **Não testado por máquina:**
  - o som: o teste roda sem áudio, então a sincronia com a música precisa ser vista e ouvida no jogo;
  - o clique com o mouse e a abertura com um herói novo de verdade.

## Para testar no jogo
1. Crie um herói novo: ao acordar no Estádio, a HQ abre. Assista um pouco e veja se cada quadro chega junto com o verso.
2. Aperte **Espaço** e clique fora do botão: nada muda, a HQ continua no tempo da música.
3. Aperte **PULAR**: vai para o "Acorde" e volta ao jogo.
4. No jogo, abra a pausa → **A HISTÓRIA**: a HQ abre de novo.
5. Com o mesmo herói, saia e entre de novo: a HQ não abre sozinha outra vez.

## Como refazer
```
python Data/Narracao/hq/_transcrever.py Docs/Midia/Abertura/Acorde.mp3 Data/Narracao/hq/acorde_palavras.json   (só se a música mudar; ~20 min)
python Tools/MakeIntroComic.py
UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho>/Tools/ImportIntroComic.py -unattended -nullrhi
```
Para trocar uma imagem, substitua a folha em `Docs/` (mesma grade 4 × 3) e rode os dois últimos comandos. Para trocar a letra de um quadro, edite `STORY` no `MakeIntroComic.py`.

## Versões anteriores
A primeira versão tinha 5 páginas de 4 quadros e um narrador falado, gerado no VoiceStudio (`Data/Narracao/final`, `Tools/MakeNarration.py`). Ela ficou de lado quando a canção chegou, mas a tela ainda sabe tocar narração por quadro se o `abertura.json` tiver o campo `voice`.
