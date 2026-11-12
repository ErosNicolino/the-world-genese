# Estádio do Despertar (escolha da arma inicial)

**Pedido:** "já faça aquele coliseu para escolha de armas", igual ao do Godot (`Game/World/AwakeningStadium*.cs`, `Game/Core/Main.Stadium.cs`).

## Como funciona

- **Herói novo** (criado no menu): acorda **sem arma** no coliseu, longe do vale. O minimapa some, porque o vale fica longe.
- Da arena saem **4 alas**, cada uma com as armas da categoria girando em pedestais (sem a katana, como no Godot). São 24 armas.

  | Ala | Direção | Cor |
  |---|---|---|
  | Marcial | norte | vermelha |
  | Distância | leste | verde |
  | Exótica | sul | roxa |
  | Arcana | oeste | azul |

- **Perto de um pedestal:** um cartão no alto da tela mostra nome, ala, golpe do clique esquerdo e especial do botão direito, como o título grande do Godot.
- **E** pega a arma, devolvendo a anterior. A arma na mão some do pedestal; **E** de novo devolve.
- **Bonecos de treino:** 4 na arena. Aguentam qualquer golpe (a vida volta cheia), balançam ao apanhar e mostram o dano.
- **Portão dourado** (sudeste): com uma arma na mão, **E** leva ao vale com a faixa "Que comece a jornada". O herói fica parado no ar até o chão do vale carregar, para não cair.
- **Anão:** não pega armas de magia (aviso na tela).
- **Heróis antigos** (salvos antes) continuam "despertos", com a espada.
- **Testes:** os que entram direto no mapa, sem herói do menu, também ficam com a espada. `-VortexAwakening` força o estádio.

## Como foi montado

`AVortexAwakeningStadium` (`Source/Vortex/*/World/`):

- **Geometria:** as mesmas medidas do Godot, peça por peça:
  - platô de grama, colinas e ciprestes;
  - arena de areia com rosa dos ventos;
  - pódio com balaustrada e estandartes;
  - 8 braseiros com chama e luz;
  - 7 fileiras de arquibancada e a galeria de colunas com mastros e flâmulas;
  - fachada de três andares de arcos;
  - corredores com arcos e tapete na cor da ala;
  - salas com pilastras, estandartes e pérgola;
  - pedestais (basalto, mármore, bronze);
  - portal dourado.
- **Desenho:** tudo em malhas instanciadas por material (25 lotes), com colisão só no que se pisa ou bate.
- **Luz:** uma luz em cada sala e um facho suave em cada pedestal, porque as paredes altas tapam o sol.
- **Materiais:** as texturas PBR do Godot (`Assets/Stadium/Textures`) projetadas no mundo, como o triplanar dele: `Tools/ImportStadium.py` (`M_StadiumStone`, `MI_Stadium_*`, `M_StadiumGlow`).
- **Lugar:** longe do vale, fora do mapa (`AVortexAwakeningStadium::Origin`).
- **Rede:** cada máquina monta a sua cópia; o servidor confere a distância do pedestal e do portão e cria os bonecos.
- **Save:** `UVortexPlayerSave::bNeedsAwakening`, e na ficha `UVortexHeroSheet::ServerStadiumTake` e `ServerLeaveStadium`.

## Como testar

- **No jogo:** crie um herói novo no menu e entre no mundo. Visite as alas, pegue armas (E), teste nos bonecos e saia pelo portão dourado.
- **Automático:** `Tools/RunStadiumShot.ps1`. Acorda no estádio sem arma, pega o arco no pedestal, atira num boneco, sai e pousa no vale; mostra `VORTEX_STADIUM_SHOT_DONE PASS` e tira as fotos `estadio_*.png`.

## Correção: "buga ao escolher as armas; trocando de estande não pega mais nenhuma"

- **Causa:** cada pedestal guardava a definição da arma (`DA_<tipo>`) só com um ponteiro fraco. A coleta de lixo da Unreal, que roda de tempos em tempos, levava as definições das armas que não estavam na mão. A partir daí os pedestais diziam "Esta arma ainda não chegou ao Vortex" e o cartão sumia.
  - O teste reproduziu: depois de uma coleta, 22 das 24 armas falhavam.
  - **Correção:** o estádio segura as definições (`KeptDefinitions`).
- **Servidor:** antes ele recalculava o pedestal mais perto pela posição dele. Agora confere o pedestal escolhido pelo jogador, com folga de 4 m, porque no servidor a posição pode estar um pouco atrás.
- **Espada de Cinzas:** o `DA_DefaultSword` não tinha nome, ala nem "jogável", então o pedestal dela recusava. `Tools/FixSwordDefinition.py` completa esses dados sem mexer nos golpes.
- **Teste:** `Tools/RunStadiumPick.ps1` passa pelos 24 pedestais de ala em ala, aperta E em cada um e força a coleta de lixo no caminho. Resultado: `VORTEX_STADIUM_PICK PASS pegas=24 falhas=0`.

## Correção: texturas que não apareciam no jogo

Pedido: "crie já arte para as armas, textura etc., e do coliseu também textura bonita".

- **O que acontecia:** as texturas existiam, mas no jogo a Unreal trocava os materiais pelo **material padrão** (cinza quadriculado). Faltavam as marcações de uso:
  - `M_StadiumStone` e `M_StadiumGlow` (pedras, mármore, bronze, tecidos e brilhos do coliseu, desenhados em lotes) não tinham "Instanced Static Meshes";
  - `M_WeaponVertexColor` (todas as 25 armas, que são malhas Nanite) e `M_ProjectileGlow` não tinham "Nanite".
- **Correção:** `Tools/FixInstancedMaterials.py` marca esses materiais, os das peças de voxel e os das armas. Agora o coliseu aparece com mármore, blocos de calcário, travertino e basalto, e as armas com as cores dos modelos do Godot (o arco de bétula branco com as marcas escuras, por exemplo).
- **Luzes criadas em jogo:** as do estádio, da cidade, da fonte, das flechas e do corte à distância ficaram "móveis". Antes elas contavam como luz por construir.

## A jornada começa na cidade, e o jogo começa pelo menu

**Pedido:** "quando começo o jogo começo já num personagem padrão, aí tenho que sair pro menu... e quando começo e escolho a arma não inicio na cidade inicial".

- **Saindo do Estádio pelo portão dourado:** o herói pousa na **praça da Cidade do Santuário**, na frente da Fonte de Jade e de frente para ela. Antes ele voltava ao ponto do vale onde tinha nascido.
  - Se o mundo não tiver cidade, volta ao ponto do vale, como antes.
  - Teste: `RunStadiumShot` pousou em Y = 1120 (praça), e `estadio_vale.png` mostra a fonte.
- **Play no editor:** o mapa que o editor abre (`EditorStartupMap`) passou a ser o `L_Menu`.
- **Mundo aberto direto, sem passar pelo menu** (sem herói escolhido e sem ser um teste `-Vortex...`): o jogo vai sozinho para o menu. Antes começava com o herói padrão. Abrindo o `L_TerrainProto` direto, sai `VORTEX_START mundo aberto sem herói: indo para o menu`.

## Armas todas na arena e cartão em desenho animado (23/10)

**Pedido:** "melhore o estádio de escolhas de arma: muito confuso, muito longe cada coisa, onde ficam as armas meio feio"; depois, "e a mensagem que aparece na tela ser cartoon também, de cada arma". O usuário escolheu a opção "tudo na arena".

**O que estava ruim:**
- **Longe:** as armas ficavam em 4 salas no fim de corredores, a ~46 m do centro. Para ver as 23 era preciso ir e voltar 4 vezes.
- **Altas:** as armas flutuavam a mais de 3 m do chão.
- **Textos repetidos:** na arena, 3 textos diziam a mesma coisa.
- **Cartão:** era uma caixa cinza com texto comum.

**O que mudou** (`AVortexAwakeningStadium`, `AVortexAimHUD::DrawStadium`):
- **As 23 armas num anel dentro da arena** (a 13,3 m do centro, 10° entre uma e outra), em 4 grupos virados para o meio:

  | Grupo | Direção | Armas |
  |---|---|---|
  | Marcial | norte | 10 |
  | Distância | leste | 4 |
  | Exótica | sul | 4 |
  | Arcana | oeste | 5 |

- **Cada grupo:**
  - tapete na cor da ala embaixo de cada pedestal;
  - um estandarte grande no muro atrás do grupo, com o nome da ala em cima;
  - o nome de cada arma flutuando sobre ela.
- **As salas e os 4 corredores saíram.** O muro do pódio, a arquibancada e a fachada fecharam o anel; só a saída dourada (sudeste) continua aberta.
- **Braseiros:** passaram para os vãos entre os grupos.
- **Armas mais baixas:** cabem em 1,1 m e flutuam 15 cm acima do tampo (antes cabiam em 1,7 m e subiam a mais de 3 m).
- **Cartão da arma, em desenho animado:**
  - **fundo:** azul-noite, faixa na cor da ala e fios dourados;
  - **nome:** grande em Luckiest Guy (a letra dos números de dano), na cor da ala; pula ao chegar perto de outra arma;
  - **golpes:** "CLIQUE ESQUERDO" e "BOTÃO DIREITO: <especial>" em desenho animado, com a explicação em letra comum embaixo (lê melhor);
  - **embaixo:** "[E] PEGAR E TESTAR NOS BONECOS" pulsando.
- **Outros textos** também em desenho animado:
  - a dica da arena: "chegue perto de uma arma e aperte E · saída dourada a sudeste";
  - o "[E] COMEÇAR A JORNADA" da saída.
  - O aviso de chegada e a placa da arena não falam mais em "alas".

**Fotos:** [arena antes](../Images/estadio_novo/antes_arena.png) / [depois](../Images/estadio_novo/depois_arena.png); [cartão antes](../Images/estadio_novo/antes_pedestal.png) / [depois](../Images/estadio_novo/depois_pedestal.png). As outras (`arquibancada`, `arco_na_mao`, `treino`, `saida`, `vale`) estão em `Docs/Images/estadio_novo/` com `antes_` e `depois_`.

**Testes:**
- `VortexEditor` compilou.
- **`RunStadiumShot`:** `PASS`. Acorda sem arma, acha o pedestal do arco no grupo de distância, pega, atira num boneco, sai e pousa na praça. O teste agora procura o arco pelo nome, porque antes procurava na sala antiga.
- **`RunStadiumPick`:** `PASS pegas=23 falhas=0`. Passa pelos 23 pedestais com coleta de lixo forçada no caminho. O doc antigo dizia 24, mas são 24 tipos menos a katana.

**Riscos:**
- **Bonecos de treino:** continuam a 7 m do centro; de onde o herói acorda, dois aparecem na frente do grupo marcial.
- **Ala Marcial:** com 10 armas, ocupa de −45° a 45°, mais larga que as outras.
- **Nomes sobre as armas:** são texto 3D. De longe ficam pequenos, e pela arquibancada não dá para lê-los.

**Para você testar jogando:** criar um herói novo, olhar a arena ao acordar, passar pelos 4 grupos, pegar e devolver armas (E), bater nos bonecos e sair pelo portão dourado.
