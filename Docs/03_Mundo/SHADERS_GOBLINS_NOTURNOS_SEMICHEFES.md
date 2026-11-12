# Shaders mais leves, 3 goblins, monstros da noite, semi-chefes e polimento (23/10, noite)

## Pedidos

O usuário estava esperando um amigo para testar e pediu tudo o que dava para fazer sem ele:

1. Os shaders estavam "pesados e muito fortes".
2. Três tipos de goblin (gordo, forte, magro), das imagens novas.
3. A base dos monstros noturnos e dos semi-chefes de cada região.
4. Polimento:
   - o letreiro da região cobria o nome dos monstros;
   - a mata à noite estava escura demais;
   - avisos de rede no anfitrião;
   - faltavam fotos das páginas Multiplayer e Conquistas.

## 1. Shaders

**Medição** (passeio a galope, 720p, o mesmo caminho):

| | FPS | Tempo do processador para a placa de vídeo |
|---|---|---|
| Shaders desligados | 105 | 8,6 ms |
| Shaders ligados | 69 (−35%) | 13,6 ms |

- **O que pesa:** quase todo o custo é da **luz global** e dos **reflexos** (Lumen). Brilho, raios de sol e desfoque custam pouco.
- **O que deixava "forte":** com a luz global, as copas e as sombras ficavam quase **pretas** (o Lumen tratava o céu como bloqueado embaixo das árvores), o brilho das luzes estava no máximo e a névoa volumétrica embaçava.

**O que mudou** (`AVortexDayNightSubsystem`, `VortexGraphics`):

| Ajuste | Antes | Agora |
|---|---|---|
| Luz global | sombras sem céu | o céu "vaza" um pouco por baixo das copas (`LumenSkylightLeaking` 0,3): as sombras deixam de ficar pretas |
| Brilho das luzes (bloom) | qualidade 5 | qualidade 3 e intensidade em 60% |
| Desfoque de movimento | forte | 0,25 e no máximo 2 |
| Névoa volumétrica (grade) | 8 px e 128 fatias | 12 px e 96 fatias (cerca de metade do custo) |

**Resultado:**
- **Visual:** ligado, fica parecido com o desligado, só com mais profundidade.
- **Custo:** o FPS com os shaders ligados quase não mudou (69). O peso é o Lumen.
- **Recomendação:** deixar **Luz global** e **Reflexos** desligados e usar Brilho e Raios de sol, que são leves.

**Fotos** (de dia, mesma posição):

| Shaders desligados | Ligados (antes) | Ligados (agora) |
|---|---|---|
| [foto](../Images/shaders/antes_shaders0_dia.png) | [foto](../Images/shaders/antes_shaders1_dia.png) | [foto](../Images/shaders/depois_shaders1_dia.png) |

**A HUD:**
- **Custo medido:** ~23 FPS neste passeio (105 com ela, 129 sem). Os passeios antigos davam 125–135, então ou a HUD ficou mais pesada nos últimos dias, ou os passeios antigos eram sem ela. Não deu para saber qual.
- **Placas de vida:** cada uma passou a ser **um desenho só** (eram ~25 pedaços por placa), mas isso não mudou o FPS.
- **Próximo passo:** descobrir o que mais pesa na HUD.

## 2. Três goblins

Com o modelo atual do goblin, até chegarem os novos (`Data/Creatures/Referencias/goblin_*`):

| Tipo | Imagem | Como é |
|---|---|---|
| **Goblin** (o "forte") | `goblin_forte_frente` | o de sempre: porrete e pedras |
| **Goblin Brutamontes** (o "gordo") | `goblin_gordo_frente` | 1,3x maior, lento (300), 1,7x a vida, porretada pesada que atordoa um pouco; não joga pedras |
| **Goblin Ladino** (o "magro") | `goblin_magro_frente` | 0,85x, rápido (520), pouca vida, pica e recua, pedras a cada 3,5 s, foge com pouca vida |

- **Bandos:** o chefe continua sendo o Goblin; os outros membros sorteiam entre os três.
- **XP, saque e missões:** as variações contam como **Goblin**, como os bandidos já faziam.
- **Detalhe técnico:** um campo novo, `Model`, deixa uma espécie usar a malha e as animações de outra.
- **Teste:** `RunCreatureSmoke -Species Goblin,GoblinBrute,GoblinScout` passou. Os três nascem, atacam e dão XP. O Brutamontes tirou 59 de vida do herói, o Goblin 31 e o Ladino 27.

## 3. Monstros da noite e semi-chefes

**Noturnos** (`UVortexCreatureSubsystem`):
- **Onde e quando:** cerca de 30% das quadras de 72 m ganham um bando que só nasce **das 19h às 5h**, 2 níveis acima da região. De manhã ele vai embora, menos se estiver brigando.
- **O vale também tem.** A 90 m da cidade não nasce nenhum.
- **Por região** (bichos que já existem, até chegarem os do bestiário):

  | Região | Noturnos |
  |---|---|
  | Valdora | morcegos e lobos |
  | Aurivale | morcegos e lobos |
  | Verdalume | lobos |
  | Solcânion | morcegos |
  | Lodonegro | sapos e morcegos |
  | Raikor | lobos e morcegos |
  | Brasafenda | morcegos |
  | Nivaluz | lobos |
  | Morthúria | morcegos e golens |

**Semi-chefes "ANCESTRAL"** (um por região, lugar fixo em chão seco):

| Região | Semi-chefe (por enquanto) | Nível |
|---|---|---|
| Valdora | Golem Musgoso Ancestral (~220 m a oeste da cidade) | 12 |
| Aurivale | Cogumelo Errante Ancestral | 20 |
| Verdalume | Golem Musgoso Ancestral | 28 |
| Solcânion | Goblin Brutamontes Ancestral | 34 |
| Lodonegro | Sapo-Gigante Ancestral | 38 |
| Raikor | Golem Musgoso Ancestral | 44 |
| Brasafenda | Golem Musgoso Ancestral | 54 |
| Nivaluz | Morcego de Ametista Ancestral | 58 |
| Morthúria | Saqueador Brutamontes Ancestral | 68 |

- **Força:** 6x a vida, 1,5x o dano, 1,6x o tamanho, não é atordoado e tem a **barra de vida no alto da tela**, como os chefes.
- **Recompensa:** 5x o XP, saque sorteado 3 vezes e 5x as moedas.
- **Volta:** renasce 15 min depois de vencido.
- **Detalhes técnicos:** `AVortexMonster::MakeElite`, `IVortexCreature::IsElite`.
- **Quando os modelos novos chegarem:** é só trocar a espécie na tabela `Elites` (o Rei Javali, o Urso-do-Mel...).

## 4. Polimento

- **Letreiro da região:** subiu para logo abaixo da bússola e ficou um pouco menor. Antes cobria o nome e a vida dos monstros à frente.
- **Mata à noite:**
  - **Problema:** o piso da exposição automática não deixava o olho se acostumar no escuro.
  - **Agora:** à noite, em lugar escuro, a imagem clareia até ~1,4x. No campo aberto ela continua igual.
- **Avisos de rede** (o teste de streaming com servidor dedicado):

  | Aviso | Causa | Correção |
  |---|---|---|
  | ~1.000 "ProceduralMeshComponent NOT Supported" | o herói pisava num pedaço de terreno sem endereço de rede | cada pedaço de colisão do terreno tem **nome fixo** (`TerrainGround_X_Y`, `TerrainSolids_X_Y`), igual no servidor e no cliente |
  | ~570 "AttachTo ... is not static" | as caixas dos marcos são fixas e a raiz deles não era | a raiz ficou fixa |

  O total de avisos do servidor caiu de **818 para 30**. Os que sobraram são do editor da Unreal (inofensivos).
- **Fotos do menu:**
  - `RunFrontEndShot` passa agora pelas páginas Multiplayer, salas, entrar com código, Conquistas e mundos.
  - Ele também apaga o mundo de teste no fim. Antes ficava um "Mundo da foto" a cada execução: era o que aparecia no "Continuar" da tela inicial do usuário. Ficaram **3** desses nos saves; não foram apagados, aguardando o OK do usuário.

## Validação

Ver "Resultados" no fim.

## Para você testar jogando

- **Shaders:** ligar e desligar cada um em Opções > Shaders, de dia e à noite.
- **Goblins:** `/spawn GoblinBrute 5`, `/spawn GoblinScout 5`, ou um bando de goblins no campo.
- **Noturnos:** `/time 21` e andar pelo vale; `/time 7` e ver os bichos da noite irem embora.
- **Semi-chefe de Valdora:** `/tp -220 90` (com `/god` se quiser).
- **Mata à noite:** `/time 22` dentro da mata.

## Resultados

- `VortexEditor` compilou.
- **Suíte completa: 27 de 27 testes passaram** (25 min), entre eles criaturas e missões, combate, cavalo, as nove regiões, população e todos os de rede.
- **Testes específicos:**
  - `RunCreatureSmoke` com os 3 goblins: passou.
  - `RunMovementNetworkSmoke`: passou.
  - `RunStreamNetworkSmoke` (servidor dedicado): passou, com 30 avisos (eram 818).
  - `RunHudShot` com os shaders ligados: passou.
  - `RunFrontEndShot`: passou, com as páginas novas.
- **Fotos do menu:** [Multiplayer](../Images/menu_paginas/multiplayer.png), [salas](../Images/menu_paginas/salas.png), [entrar com código](../Images/menu_paginas/entrar_codigo.png), [Conquistas](../Images/menu_paginas/conquistas.png), [mundos](../Images/menu_paginas/mundos.png).
- **Não testado automaticamente:** os noturnos e os semi-chefes em jogo (eles nascem perto do jogador conforme a hora; os testes não ficam à noite andando pelo mapa) e a mata à noite. Teste jogando.
- **Saves de teste que ficaram:** 3 mundos "Mundo da foto" (do teste de fotos do menu, que agora apaga o seu) e 2 mundos de multiplayer "Sala de teste" (do teste da sala de espera). Não foram apagados.
