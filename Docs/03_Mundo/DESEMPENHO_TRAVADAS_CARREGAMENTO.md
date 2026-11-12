# Travadas, tela de carregamento nova e gerente de importância (08/11)

Os pedidos: "melhorar o FPS do meu jogo, me dê ideias e como os jogos fazem" e depois "faça tudo". No meio, um a mais:
uma tela de carregamento mais bonita e o bug do mundo aparecendo antes dela ("quando eu termino de escolher a arma,
aparece o mundo e depois a tela de carregamento").

## Como estava (passeio de 1 min, 1080p, executável do editor)

- **Média:** ~72 FPS com o jogo andando normal. Quem segura é a preparação do desenho no processador (13,5 ms por
  quadro); a placa de vídeo usa 10,2 ms.
- **Travadas:** grandes nos primeiros 25 s, com o pior 1% dos quadros em 3,9 FPS. As piores: cavalo 1,4 s, Colosso 1,3 s,
  bandido grandão 0,5 s.

## Por que travava

Medido com cronômetros em cada etapa do nascimento de um bicho e a lista do que a Unreal carrega do disco "na marra"
(`VORTEX_SYNC_LOAD` no passeio):

1. **A fila da pré-carga.** As malhas já eram pedidas em segundo plano ao entrar no mundo, mas o bicho que nascia antes da
   malha chegar pedia de novo "na hora". Aí a Unreal parava tudo até esvaziar a fila inteira. O Colosso gastava 907 ms
   só nisso.
2. **Coisas fora da lista.** O andar dos bandidos, a gota nova (Slime), os materiais da Caldeira e do Azhûr, a névoa
   roxa dos monstros, o rastro das flechas e as variações gravadas dos sons.
3. **Só no editor.** Com pouca memória livre, o editor monta a malha de um personagem na primeira vez que ela aparece
   ("AssetCompile... SkinnedAsset"). Foi a trava de 2,5 s de um bandido. No jogo empacotado isso não existe.

## O que mudou

- **Nenhum bicho nasce antes da hora** (como os jogos fazem com os "spawns" esperando os recursos). Enquanto a pré-carga
  não termina, nenhum nasce. Espécie fora da lista é pedida em segundo plano e nasce no giro seguinte, quando chegar.
  - O Colosso foi de 907 ms para 3 ms para pegar a malha.
  - Os bandidos pararam de travar.
- **A pré-carga cobre tudo** o que o passeio mostrou carregando na hora (a lista acima).
- **Sons:** um som que ainda está chegando não toca dessa vez, em vez de travar o quadro esperando a fila.
- **Shaders** (como a tela de "otimizando shaders" do Fortnite): no jogo empacotado, cada material prepara o shader quando
  a peça carrega, e a peça só aparece com ele pronto (`r.PSOPrecaching`, `ProxyCreationWhenPSOReady`). A tela de
  carregamento espera essa fila (até 15 s).
- **Gerente de importância** (como o Significance Manager dos jogos grandes): um bicho longe de todo mundo pensa e anda
  menos vezes por segundo, e o movimento soma o tempo que passou, então a velocidade não muda. Brigando, sempre a cada
  quadro. `-VortexNoSignificance` desliga.

  | Distância | Pensa e anda |
  |---|---|
  | até 30 m | a cada quadro |
  | até 60 m | 30 vezes por segundo |
  | até 120 m | 10 vezes por segundo |
  | mais longe | 4 vezes por segundo |

- **Moradores:** a mais de 70 m não são desenhados (com 38 na vila, conta).
- **Já estavam feitos** (não precisou mexer):
  - grama e plantas pequenas sem sombra, cada tipo sumindo na sua distância;
  - a luz global (Lumen) desligada por padrão, só se o jogador ligar.

## A tela de carregamento nova

![A tela de carregamento](../Images/tela_carregamento.png)

- **Fundo:** uma das cenas pintadas da abertura, a da região para onde o herói vai (a Yggdrasil no vale, a vila na neve,
  a cratera de lava, o coliseu...), com um zoom lento.
- **Na tela:** o nome do lugar em dourado com a linha de enfeite e o losango, a barra fina com um brilho correndo, o anel
  de runas girando no canto, a moldura dourada e uma dica.
- **O bug corrigido:** a tela só começava quando o HUD percebia que o Despertar tinha acabado, e ainda clareava por 0,17 s.
  Nesse meio-tempo o herói já estava no vale e o mundo pela metade aparecia. Agora ela abre cobrindo tudo, na hora:
  - ao entrar no mundo;
  - ao sair do Despertar;
  - em qualquer salto grande do herói (mais de 40 m de um quadro para o outro: renascer, portais).
- **Só sai quando:** o chão em volta está montado, a pré-carga dos bichos terminou e os shaders estão prontos (no jogo
  empacotado).
- **Testes automáticos** (`-Vortex...`): seguem como antes, sem a tela ao entrar.

## Medir

- **No editor:** `Tools\RunRoamBench.ps1 -ResX 1920 -ResY 1080`.
- **No jogo empacotado** (o número de verdade): `Tools\PackageAlpha.ps1 -Perf` faz o pacote de medição em
  `Build\PerfTest` (Development, sem instalador; o do itch em `Build\Alpha` não muda). Depois
  `Tools\RunRoamBench.ps1 -Packaged -ResX 1920 -ResY 1080`.

| | Antes | Agora (editor) |
|---|---|---|
| FPS andando normal | ~72 | ~72 |
| Colosso para pegar a malha | 907 ms | 3 ms |
| Bandido | 0,4 a 0,5 s | sem travada |

O jogo empacotado ainda não foi medido: o empacotamento foi parado porque, junto com o jogo aberto, deu crash de
memória no PC. Medir com o jogo fechado.

## Ficou para depois (o que ainda dá para ganhar)

- **Reaproveitar criaturas** (guardar em vez de destruir): a medição mostrou que o custo de nascer era esperar o
  carregamento, não criar o ator. Com isso resolvido, sobra pouco para ganhar; fica para se o pacote ainda mostrar
  travadas ao nascer.
- **Menos pedidos de desenho** (~970 por quadro): juntar as peças da vila em poucas malhas e um bloco simplificado de
  longe (HLOD), imagens planas para as árvores distantes. É o que ainda segura os ~72 FPS, mas é uma obra maior.
