# Fase 13 — streaming de terreno com vários jogadores (aguardando teste no editor)

## O que mudou e por quê

Antes, `UpdateWanted` e `UpdateTrees` olhavam só para `Players[0]`. Num servidor com dois jogadores afastados, o segundo ficava sem chão de colisão, a não ser perto do primeiro. Além disso:
- as tarefas prontas eram entregues em ordem arbitrária;
- uma tarefa lançada antes de o jogador sair da área ainda podia anexar o pedaço;
- o servidor dedicado montava malha, objetos, grama e árvores que ninguém via.

Agora `AVortexTerrainStreamer` trabalha com a **união das áreas** de todos os jogadores, em dois níveis:

| Nível | Para quem | Alcance | O que monta |
|---|---|---|---|
| Completo | quem desenha a tela nesta máquina (anfitrião, cliente, jogo local) | `StreamRadius` = 6 pedaços | malha, água, objetos, grama, árvores, chão |
| Só chão | cada jogador simulado aqui; no servidor, inclusive os remotos | `SimulationRadius` = 4 pedaços (≥ 72 m) | chão de colisão; corpos de árvores e rochas até `CollisionRadius` = 2 |

- **Servidor dedicado:** não tem ninguém desenhando a tela, então não monta nada visual nem árvores. A tarefa de fundo também pula a malha visual e os objetos.
- **Prioridade:** primeiro o chão em volta de qualquer jogador, depois o mais perto de alguém. Vale tanto para o lançamento das tarefas quanto para a entrega das prontas.
- **Orçamento por quadro:**
  - 1 pedaço completo (`ChunksPerFrame`) e até 4 pedaços só de chão;
  - 2 malhas de colisão novas (`CollisionMeshesPerFrame`) e 1 lote de corpos sólidos;
  - 3 regiões de árvores; só o primeiro preenchimento do jogo sai de uma vez;
  - 6 pedaços soltos (`RetirePerFrame`).
- **Liberação com folga:** um pedaço só sai quando passa um pedaço além do alcance de todos. Se não há mais quem desenhe a tela perto, mas ainda há alguém sendo simulado, o pedaço perde só a parte visual. Os componentes saem aos poucos, por filas.
- **Tarefas antigas:** quando uma tarefa termina, o pedaço é conferido com a área desejada *atual*. Se saiu, o resultado é descartado (`StaleDiscarded`) e nada é anexado. Um pedaço que tinha só chão e passou a precisar de malha espera a tarefa anterior terminar e ganha uma nova.
- **Memória:** cada pedaço guarda só os dados de colisão. Antes guardava a malha visual inteira depois de entregue.
- **Teletransporte longe:** saltar longe montava todas as regiões de árvores e soltava 100+ pedaços no mesmo quadro, com pico de 87,7 ms no streamer. Com as filas, o pico caiu para 9–15 ms, e o máximo restante é o primeiro preenchimento de árvores ao entrar no jogo.

## Testes

- Compilação `VortexEditor Win64 Development`: passou.
- **`RunStreamNetworkSmoke.ps1` (novo), servidor dedicado + 2 clientes:** passou. Um jogador vai para A (−300, −80) m e o outro para B (0, 0), a 316 m. O servidor teve chão nos dois pontos em 0,07 s, sem nenhum pedaço visual. Depois o segundo jogador saltou entre pontos distantes a cada 0,25 s e voltou para perto de A: a área de B foi liberada, `outside_max=0` durante todo o teste, e 64 tarefas velhas foram descartadas em vez de anexadas. Cada cliente avisou quando teve chão e malha no próprio pedaço, inclusive em B.
- **`RunStreamNetworkSmoke.ps1 -Listen`, anfitrião + 1 cliente:** passou. O anfitrião tem malha só em volta dele (188 pedaços visuais); em volta do cliente distante, só chão (269 pedaços carregados no total).
- Regressões: `RunWolfNetworkSmoke`, `RunPackSmoke` (5 lobos), `RunWolfSmoke`, `RunSolidWalkSmoke` (9 obstáculos), `RunFrameworkNetworkSmoke` e `RunMovementSmoke -Matrix`: todas passaram. As vistas de `RunFogShot` ficaram iguais às da Fase 12.

## Desempenho e memória

`RunTerrainBench.ps1 -Profile alto`, aquecido, 1280×720. O script agora amostra a memória do processo a cada 0,5 s.

| | FPS | 1% pior | pior quadro | privada (pico) | conjunto de trabalho (pico) |
|---|---|---|---|---|---|
| Fase 12 | 74,4–74,5 | 53,1–53,4 | 24,5–25,2 ms | 6,24–6,26 GB | 3,25 GB |
| Fase 13 | 74,5–74,7 | **56,8–57,5** | **21,9–22,1 ms** | 6,13–6,17 GB | 3,19–3,21 GB |

O custo médio do streamer por quadro é 0,55 ms. O máximo de 13,3 ms vem do primeiro preenchimento das árvores, ao entrar no jogo.

Na rede, o servidor dedicado gasta 0,2–0,6 ms em média por quadro, com máximo de 1,3–13 ms, e usa ~1,7 GB.

Clientes com memória medida depois de coleta de lixo e `Trim` do alocador:

| Cliente | Memória |
|---|---|
| Parado (188 pedaços) | ~2,02–2,05 GB, estável |
| Depois de 8 saltos | 2,50 GB |
| Depois de 32 saltos | 2,89 GB |
| Depois de 96 saltos | 3,00 GB, subindo ~1,6 MB por salto e estabilizando |

Os componentes e malhas vivos acompanham os pedaços carregados (169 pedaços → 2 940 componentes / 276 malhas). Não há objeto vazando. O excedente é retenção da engine e do alocador nas primeiras visitas.

## Riscos

- O servidor dedicado não força coleta de lixo durante o teste. A memória dele subiu de 1,70 para 2,05 GB em 96 saltos, mas isso não foi medido depois da coleta. Convém medir uma sessão longa com jogadores viajando de verdade.
- Na rede, o teste usa `-nullrhi`, ou seja, sem placa de vídeo. O custo de render com dois jogadores numa máquina com tela só foi medido no modo local.
- Um jogador remoto no anfitrião tem chão até 4 pedaços, mas árvores e rochas só até 2. Lobos e NPCs a mais de ~36–54 m dele atravessam esses obstáculos no servidor; era assim antes também.
- Pedaços na fila de liberação continuam desenhados por alguns quadros, longe e dentro da névoa.

## Reversão

Reverta o commit desta fase. O streamer volta a olhar só para o primeiro jogador; a névoa e os limites da Fase 12 não mudam. Para ajustar sem código: `StreamRadius`, `SimulationRadius`, `CollisionRadius`, `ChunksPerFrame`, `CollisionMeshesPerFrame` e `RetirePerFrame` ficam editáveis no ator do streamer.

## Avaliação pendente no editor

Teste no editor com 2 jogadores (Play As Listen Server ou Client):
- andem para longe um do outro e confiram se o segundo nunca cai;
- confiram se a paisagem aparece sem buracos ao correr;
- confiram se teletransportes e mortes não provocam travadas.

**A fase não está declarada concluída antes desse teste.**
