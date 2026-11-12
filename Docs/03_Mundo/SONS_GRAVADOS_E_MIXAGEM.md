# Sons: gravações reais, ambientes por região e mixagem

> "Os sons, dê uma melhorada neles de tudo, pode pesquisar na internet se necessário."

## O que estava errado

- **A Unreal tocava os sons antigos.** O projeto Godot já tinha gravações reais (Freesound, SONNISS e Ovani, todas com licença de uso comercial) para 128 sons, com **4 variações cada**, em `Assets/Sfx/recorded`. Só a versão antiga e única de cada som tinha vindo para a Unreal.
- **Volumes desencontrados:** até 20 dB de diferença dentro da mesma categoria (golpe alto ao lado de golpe baixinho).
- **9 ambientes estalavam** na volta do laço.
- **11 golpes começavam com mais de 50 ms de silêncio**, então o som chegava atrasado.
- **Os ambientes eram sintetizados**, e os de região (deserto, neve, pântano, vulcão, mar, cidade...) **nunca tocavam**: só dia, noite, vento e água estavam ligados.
- **Os bichos não tinham voz própria:** todo bicho apanhando fazia o mesmo "creature_hit".

## O que mudou

- **Gravações reais com variações:** os 128 sons gravados vieram com as 4 tomadas.
  - O jogo sorteia uma tomada a cada vez e nunca repete a mesma em seguida.
  - Exemplos: espada, golpes, passos (grama, pedra, areia, lama e, agora, **água**), pulo, queda, rolamento, poção, cura, magias das classes, todos os sons do chefe e as vozes dos bichos.
- **Voz de cada espécie:** lobo, gota, golem, cogumelo, morcego, cervo, coelho, bandido e cavalo gritam com a própria voz ao apanhar e ao morrer. O "puf" de sumir continua, mais baixo. Quem não tem voz gravada (esqueletos, goblins...) continua com o baque comum.
- **Passos:** na água rasa o passo chapinha, e na neve (chão branco) usa o passo de neve.
- **Ambientes reais (SONNISS GDC 2024),** 40 s em estéreo, com a volta sem emenda:

  | Ambiente | Gravação |
  |---|---|
  | dia | borda de mata na primavera, com pássaros |
  | noite | mata de noite com vento, e corujas ao fundo |
  | vento | vento na mata da Noruega |
  | água | rio calmo |
  | mar | ondas nas pedras |
  | tempestade | chuva com trovões |
  | neve | vento frio uivando |
  | deserto | campo aberto, vento e ronco grave |
  | pântano | insetos no campo e água mexendo |
  | assombrado | mata à noite com textura sinistra |
  | vulcão e lava | ronco de caverna e fogo crepitando |
  | cidade | praça de cidade |
  | élfico | mata com pica-paus ao longe |
  | Yggdrasil | vento no álamo e mata |

- **Som de cada lugar:** por cima do dia, da noite e do vento entra o ambiente da região onde você está.
  - mar → ondas;
  - Brasafenda → vulcão;
  - Raikor → tempestade;
  - Nivaluz → vento frio;
  - Solcânion → deserto;
  - Lodonegro → pântano;
  - Morthúria → assombrado;
  - Verdalume → mata élfica;
  - Cidade do Santuário → praça.

  Onde o lugar manda, os passarinhos do dia ficam mais baixos (no deserto, quase nada). Lagos não contam como mar. O ambiente que sumiu para de tocar.
- **Fauna com gravações reais:**
  - pios: recortados de 2 gravações CC0;
  - corvo: gravação CC0 feita no Cairo;
  - bando levantando voo: o "Large Wings Flap" (CC0) em 4 camadas fora de passo;
  - peixe pulando: impacto na água do SONNISS.

  A **gaivota continua sintetizada**: não achei gravação de gaivota com licença limpa. O pacote "CC0" que existe foi tirado de um vídeo do YouTube, e as gravações do xeno-canto proíbem uso comercial.
- **Mixagem:**
  - **Volume por categoria** (média do trecho que soa): passos -27, ambientes -24, golpes no ar -21, moedas e missões -20, magias e vozes -19, acertos -17, chefe -15. As 4 tomadas de um som recebem o mesmo ganho.
  - **Pico máximo de -1 dB**, arredondado sem estalar.
  - **Começo e fim:** corta o silêncio do começo e do fim, com rampas de 2 ms e 15 ms.
  - **Distância pelo tipo:** passos, pios, peixe e nado ~16 m; a maioria ~30 m; chefe, trovão, meteoro, uivo, sino e portal ~70 m. De longe o som fica abafado (o ar come os agudos).
  - **Sem empilhar:** no máximo 4 cópias do mesmo som a cada 0,12 s (um golpe de área em 10 bichos não estoura).
- **Créditos:** `Data/Sfx/CREDITOS_SONS.txt` junta os créditos do Godot (as gravações CC BY pedem crédito) com os sons CC0 novos e as bibliotecas. O arquivo vai solto ao lado do jogo empacotado (`Content/Credits`, `DirectoriesToAlwaysStageAsNonUFS` em `DefaultGame.ini`).

A interface (sons cartoon, Kenney CC0) e a música não mudaram.

## Como refazer

Na raiz do projeto: os dois primeiros comandos sem o editor; o terceiro com o editor fechado.
1. `python Tools/BuildSfxMaster.py`
   - Precisa de numpy, scipy e soundfile.
   - Lê os sons originais sem mexer neles e grava em `Data/SfxMaster`.
   - Gera também a página de antes e depois.
2. `python Tools/MakeWildlifeSounds.py`
   - Só se quiser refazer a gaivota sintetizada.
3. `UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho>/Tools/ImportSfxMaster.py -unattended -nullrhi`
   - Importa tudo em `/Game/Vortex/Audio`.

Os sons CC0 baixados ficam em `Data/Sfx/WildlifeCC0` (com `LICENCAS.txt`). As gravações SONNISS e as do Godot são lidas de `the-world-geneses` (`Assets/Sfx/recorded` e `_Externos/Sonniss GDC 2024`).

## Para ouvir antes e depois

Abra **`Saved/SfxPreview/index.html`** no navegador. São 364 sons, com o antigo à esquerda e o novo à direita:
- `nome_gravado` é a primeira das 4 tomadas;
- os ambientes mostram 15–20 s.

## Testes

`RunWildlifeSmoke` agora também confere:
- as 4 tomadas gravadas;
- a voz de cada espécie (e que o esqueleto não tem voz própria);
- que os sons novos existem;
- que o ambiente do pântano e o da mata élfica tocam nas regiões certas.

Os outros testes rodados:
- `RunWolfSmoke` passou;
- a suíte rápida completa (`RunAllTests.ps1`) passou: **30 de 30**, em 30 min.

Os testes rodam sem placa de som (`-nosound`). Eles conferem o que toca e com que volume, mas **não dá para ouvir**: o ouvido final é seu.

## Para testar no jogo

1. Golpeie um lobo várias vezes: os golpes e os ganidos variam a cada vez.
2. Vença um coelho, um cervo e um golem: cada um tem o próprio grito.
3. Ande na água rasa: o passo chapinha.
4. Fique parado no campo de dia, depois de noite: pássaros, depois mata noturna com corujas.
5. Vá ao mar, ao pântano (Lodonegro), a Verdalume e à cidade: o ambiente muda devagar.
6. Use um golpe de área em vários bichos: o som não estoura.
7. Nas opções, o volume de "Ambiente" deve mexer só nos ambientes.

## Ainda para depois

- As 3 músicas (vila, noite e luta) continuam as provisórias.
- A gaivota pede uma gravação de verdade.
