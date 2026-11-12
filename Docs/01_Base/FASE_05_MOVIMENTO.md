# Vortex Unreal — Fase 5: movimento

**Data:** 03/10/2026 (continuação do `FASE_05_MOVIMENTO_PROGRESSO.md`)
**Estado:** critérios automáticos aprovados e **validação visual feita pelo usuário em 04/10/2026**
(ver "Validação no teclado"). Terreno em pedaços e medição de cena foram para a fase do terreno
(aprovado pelo usuário). **Fase 5 concluída.**

## WHAT / O que foi alterado

- **Teste de entrada real do cliente** (iniciado na sessão anterior): commit próprio
  `905dab6`.
- **`UVortexCharacterMovementComponent`**: corrida e esquiva previstas pela rede.
  - Os dois viajam como bits do movimento salvo (`FSavedMove`, `FLAG_Custom_0/1`), não como RPC.
  - O estado da esquiva (tempo, direção, recarga, cargas) é guardado em cada passo e restaurado
    quando o cliente refaz passos após uma correção.
  - A corrida por RPC (`ServerSetSprinting`) foi removida.
- **Esquiva (Ctrl)** com os valores do Godot (`PlayerController.BeginDash`):
  - 0,58 s, curva de 4,4 a 9 m/s, recarga de 0,8 s, 5 cargas que voltam a cada 4 s, só no chão.
  - Direção = a apertada (relativa à câmera) ou para frente.
- **Água por dados** (`AVortexWaterZone` + `UVortexWaterSubsystem`): uma caixa com a altura da
  superfície. O movimento pergunta "tem água aqui e em que altura?". É o mesmo modelo do Godot
  (`WaterLevel`, lagos e rios gerados), pronto para o gerador de terreno registrar zonas por
  pedaço.
- **Nado próprio** (`MOVE_Custom` / `EVortexMovementMode::Swim`):
  - Por que próprio: o nado nativo da Unreal exige um `PhysicsVolume` de água no mapa (o
    `PhysSwimming` consulta `bWaterVolume` direto).
  - Comportamento: entra com 1,10 m de profundidade (da sola) e sai abaixo de 0,70 m. Boia com
    ~70% do corpo submerso, por uma mola com amortecimento. Nada a 3 m/s, Espaço sobe e sai
    andando por margens rasas.
- **Matriz de movimento**: `Tools/AddMovementMatrixLanes.py` acrescentou ao `L_MovementProbe`
  rampas de 20°, 40° e 50°, uma escada de 8 degraus de 25 cm e uma piscina com margem em rampa.
  O teste `-VortexMatrixSmoke` percorre todas.
- **Teste com atraso e perda**: `RunClientInputSmoke.ps1 -LagMs -LossPct` emula a rede
  (`NetEmulation.PktLag/PktLoss`) e conta as correções do servidor (`p.NetShowCorrections`).
- **Medição de custo**: `Tools/RunMoveBench.ps1` (`-VortexMoveBench=N`), uma amostra por quadro.
- **Scripts de rede**: servidor e cliente agora são sempre encerrados. Antes, no PowerShell 5,
  o `exit` dentro do `try` pulava o `finally`, e cada teste aprovado deixava dois processos de
  ~600 MB abertos (achei 14 de uma vez). Isso também explica parte da falta de memória
  registrada na sessão anterior.

## WHY / Por quê

- Corrida por RPC separado chega ao servidor depois dos passos já enviados. O cliente corre a
  7,2 m/s enquanto o servidor ainda anda a 4,6, e recebe correções seguidas (trancos).
- Água e nado precisam funcionar com água gerada, não só com volumes colocados à mão.

## DEPENDENCIES / Dependências

`UCharacterMovementComponent` nativo (movimento salvo, correção e repetição de passos). Nenhum
módulo novo. Nenhum asset Godot importado.

## VALIDATION / Validação

| Verificação | Resultado |
|---|---|
| Build `VortexEditor Win64 Development` | Passou |
| Build `Vortex Win64 Development` (sem símbolos, como no README) | Passou |
| Entrada do cliente: andar, correr, pular, esquivar, conferido no servidor | Passou (1 esquiva nos dois lados) |
| Mesmo teste com 150 ms + 3% de perda, 200 ms + 5% e 250 ms + 5% | Passou nos três |
| Rampa de 20° e de 40° sobem; rampa de 50° bloqueia | Passou (maxZ 316 / 602; parou em x=-21) |
| Escada de 8 degraus de 25 cm | Passou (maxZ 298) |
| Piscina: entra nadando, boia (não afunda) e sai andando pela margem | Passou (menor Z nadando = -106; o fundo seria -154) |
| Rampa suave + penhasco + salto (teste anterior) | Passou (x=1038, subida 296, maior degrau 2,8 cm) |
| Degraus de 30 e 60 cm (teste anterior) | Passou (x=607,9, z=128,0) |
| Rede: física do servidor vista pelo cliente | Passou |
| Rede: login, estado de jogo, slot e descritor do mundo | Passou |
| Saves: ida e volta e recuperação do backup | Passou |
| Nenhum processo da Unreal sobrando depois dos testes | Passou |

**Correções de posição recebidas pelo cliente** no teste de entrada (mesmo percurso; versão
anterior medida num worktree do commit `905dab6`):

| Rede emulada | Corrida por RPC (antes) | Previsão nativa (agora) |
|---|---|---|
| sem atraso | 32 | 1 |
| 150 ms + 3% | 34 | 1 |
| 250 ms + 5% | 17 (teste não terminou) | 1 |

A única correção restante acontece no nascimento do personagem (queda até o chão).

## PERFORMANCE / Medição

Sem renderização e sem limite de FPS, personagens andando em círculos e colidindo no mapa de
teste (`RunMoveBench.ps1`):

| Personagens | Quadro |
|---|---|
| 0 | 0,61 ms |
| 50 | 1,58 ms |
| 150 | 3,52 ms |

São ~19 µs por personagem. (Correção em 04/10: a primeira medição, de ~26 µs, usava um
cronômetro que rodava várias vezes por quadro e pesava mais os quadros longos. Agora a medição
roda exatamente uma vez por quadro.) Não é uma comparação de FPS com o Godot: falta uma cena equivalente
(terreno, vegetação, cidade), que depende da tecnologia de terreno.

## RISK / O que falta

1. ~~Validação visual no teclado.~~ Feita e aprovada pelo usuário (ver acima).
2. **Nado em rede:** a mecânica é a mesma de cliente e servidor (modo de movimento replicado),
   mas não há teste de rede específico na água. Os testes de rede atuais rodam no `L_Framework`.
3. **Integração com o terreno em pedaços e medição numa cena representativa:** passadas para a
   fase do terreno, com aprovação do usuário em 04/10/2026. Ver `FASE_06_TERRENO_PERSONAGEM.md`.
4. **Esquiva:** a invulnerabilidade e a animação entram com combate e animação. Hoje ela só
   move. A recarga de cargas não aparece na interface (ainda não há HUD).

## Validação no teclado (usuário, 04/10/2026)

Jogado no editor em DirectX 11 (`AbrirEditor_DX11.bat`), mapa `L_MovementProbe`:
- **Escada:** sobe andando.
- **Degraus:** o de 60 cm não sobe andando, só pulando, como esperado.
- **Rampa de 50°:** não sobe, escorrega, como esperado.
- **Avaliação geral:** "achei bom tudo".

Observação: com DirectX 12, o editor deixava "fantasmas" de janelas sobrepostas nesta máquina
(Windows 10, 2 monitores). Isso é um defeito de desenho do editor, não do projeto, e foi
contornado com `-dx11`.

## Como validar no editor

Reinicie o PC depois de aumentar o arquivo de paginação e feche o Godot e o navegador.

1. Abra `Vortex.uproject`.
2. Abra `Content/Vortex/Maps/L_MovementProbe` e aperte **Play** (Alt+P).
3. Teste os controles:
   - **WASD:** andar.
   - **Mouse:** câmera.
   - **Shift:** correr.
   - **Espaço:** pular (na água, subir).
   - **Ctrl:** esquivar.
4. Percorra as pistas, todas para +X a partir do começo de cada faixa:
   - **Y=0:** rampa suave sob os blocos e o penhasco.
   - **Y=1000:** degraus de 30 e 60 cm.
   - **Y=2000, 2600, 3200:** rampas de 20°, 40° e 50°.
   - **Y=3800:** escada.
   - **Y=4400:** piscina. A água é invisível no jogo; a caixa azul aparece só no editor.

Avalie se a câmera treme subindo os blocos, se a corrida e a esquiva parecem as do Godot e se o
nado é confortável.

## ROLLBACK / Reversão

Cada parte está em commit próprio. Reverter o commit desta entrega volta à corrida por RPC e
remove água, nado, matriz e medições. O teste de entrada do cliente fica no commit anterior. O
Godot e seus saves não foram tocados.
