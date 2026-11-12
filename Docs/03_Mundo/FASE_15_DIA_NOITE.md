# Fase 15 — dia e noite, lua com fases, estrelas e efeitos do sol (aguardando teste no editor)

## O que mudou e por quê

O ciclo segue o `DayNight.cs` do Godot (lido sem modificar): **30 min de dia** (6h às 18h) e **8 min de noite**. O sol nasce a leste e passa inclinado 38° para o sul. Uma só luz faz o papel do sol de dia e da lua à noite.

- **Relógio do mundo** (`AVortexGameState`). O servidor guarda só o ponto de partida (hora, dia, instante e velocidade). Toda máquina calcula a mesma hora a partir do tempo de servidor replicado, sem tráfego por quadro.
  - A hora e o dia vão para o save do mundo (`DayClock`, `DayNumber`); um mundo novo começa às 8h do dia 1.
  - **HUD:** "Dia N  HH:MM" no canto superior direito.
  - **Console** (servidor ou jogo local): `Vortex.Hora 20.5`, `Vortex.Tempo.Escala 60` (60 = um minuto por segundo; 0 = parado).
  - **Testes:** `-VortexHour=` e `-VortexDayScale=`.
- **Na tela** (`UVortexDayNightSubsystem`, em cada máquina que desenha; o servidor dedicado não faz nada):
  - **Sol e lua:** cores do Godot (dia, dourado, luar azulado). A elevação da luz tem mínimo de 8°, para as sombras não esticarem. A luz só gira a cada 0,3° (≈3 s de jogo), porque cada giro refaz as sombras virtuais.
  - **Céu:** o SkyAtmosphere segue a luz. À noite o céu fica azul-escuro, e no crepúsculo continua claro um pouco depois do pôr do sol.
  - **Luz ambiente:** o SkyLight (captura em tempo real) ganha o tom do Godot e um reforço à noite e no crepúsculo, como o ambiente quase constante do Godot.
  - **Névoa:** a cor da Fase 12 de dia, dourada no nascer e no pôr do sol, azul-escura à noite.
  - **Exposição fixa**, como no Godot, para a noite não clarear sozinha. O projeto usa a faixa linear antiga de exposição (os limites do mapa são 0,03–8), não EV100; o valor fixo **0,5** foi calibrado para o meio-dia ter o mesmo brilho da exposição automática da Fase 12 (luminância média 96,8 contra ~97).
- **Ao olhar para o sol:** raios de luz entre troncos e copas (light shafts com oclusão), reflexo de lente e brilho. São mais fortes e dourados no nascer e no pôr do sol. Os raios saem do disco verdadeiro do sol.
- **Noite** (pedido do jogador):
  - **estrelas** em blocos, no estilo voxel, que piscam devagar, giram com o céu, somem perto do horizonte e só aparecem com o céu escuro; inclui uma faixa fraca de Via Láctea (`Tools/MakeNightStars.py`, sempre igual);
  - **lua com fases:** ciclo de 8 dias de jogo, começando cheia no dia 1; a fase ilumina o disco como uma esfera, com manchas em blocos;
  - **halo** azulado em volta da lua, proporcional à parte iluminada;
  - **luar:** a luz da lua vai de 25% na nova a 100% na cheia, e faz raios fracos e azulados entre as copas;
  - tudo é desenhado numa cúpula aditiva de 20 km em volta da câmera (`M_NightSky`, criado por `Tools/CreateNightSky.py`). O disco do céu da Unreal é apagado à noite, porque a lua com fase vem da cúpula.
- **Nuvens:** as nuvens de voxel portadas do Godot foram trocadas por nuvens volumétricas realistas, a pedido do jogador; veja `CORRECAO_GRAMA_CEU_REALISTA.md`.
- **Mapa:** `Terrain_Sun` passou de Stationary para Movable (`Tools/TuneDayNight.py`). Sem isso, o sol não gira durante o jogo.

## Fotos (mesma vista do vale; `RunDayNightShot.ps1`)

| Hora | | Hora | |
|---|---|---|---|
| 06:12 | [foto](../Images/fase15_0612.png) | 17:42 | [foto](../Images/fase15_1742.png) |
| 08:00 | [foto](../Images/fase15_0800.png) | 18:36 | [foto](../Images/fase15_1836.png) |
| 12:00 | [foto](../Images/fase15_1200.png) | 21:00 | [foto](../Images/fase15_2100.png) |
| 16:30 | [foto](../Images/fase15_1630.png) | 01:00 | [foto](../Images/fase15_0100.png) |

- **Olhando o sol:** [07:00](../Images/fase15_sol_0700.png) e [17:18](../Images/fase15_sol_1718.png).
- **Nuvens:** [12:00](../Images/fase15_nuvens_1200.png), [17:36](../Images/fase15_nuvens_1736.png), [21:00](../Images/fase15_nuvens_2100.png).
- **Lua às 22h:** [cheia, dia 1](../Images/fase15_lua_cheia.png), [minguante fina, dia 3](../Images/fase15_lua_minguante.png), [nova, dia 4](../Images/fase15_lua_fina.png).

## Testes

- Compilação: passou.
- **`RunDayNightSmoke.ps1` (novo), servidor dedicado + cliente:** passou.
  - As conversões hora ↔ relógio voltam iguais.
  - O dia vira com o relógio acelerado.
  - A hora vai e volta do save.
  - O cliente calcula exatamente o mesmo relógio que o servidor (552,422 s nos dois para o mesmo instante).
- **`RunDayNightShot.ps1` (novo):** as fotos acima.
- Regressões: `RunSaveSmoke`, `RunCameraSmoke`, `RunWolfSmoke`, `RunFrameworkNetworkSmoke`, `RunStreamNetworkSmoke`, `RunPopulationSmoke` e `RunFogShot`: todas passaram.

## Desempenho (`RunTerrainBench -Profile alto`; `-Hour` é novo)

| | FPS | 1% pior | pior quadro |
|---|---|---|---|
| Antes (luz fixa) | 73,8–74,0 | 56,5–56,7 | 22,0–22,5 ms |
| 8h, relógio correndo | 71,7 | 56,4 | 23,3 ms |
| 12h | 73,8 | 55,3 | 22,2 ms |
| 22h | 77,9 | 57,2 | 22,3 ms |
| 12h com nuvens volumétricas (ver correção da grama e do céu) | 70,2–70,4 | 53,2–55,3 | 22,6 ms |

Com o sol baixo há mais sombra longa, e o giro a cada ~3 s refaz as sombras virtuais: custa ~2 FPS às 8h. O 1% pior não mudou.

## Riscos

- A exposição fixa deixa interiores e sombras mais escuros do que a automática, como no Godot. Ajuste em `UVortexDayNightSubsystem::ExposureLuminance`, ou teste outros valores com `RunDayNightShot -Exposure`.
- O reflexo de lente da Unreal aparece pouco com este céu; os raios aparecem bem. A força geral está em `SunGlare` (`-VortexSunGlare=`).
- Quem passar de ~20 km da origem pelo mapa ainda fica dentro da cúpula, porque ela acompanha a câmera.
- Estrelas e lua não recebem névoa: perto do horizonte elas apenas desaparecem gradualmente.
- As nuvens não recebem um tratamento especial de névoa: as mais distantes ficam veladas pela névoa da Fase 12. São opacas e não fazem sombra, como no Godot.

## Reversão

Reverter o commit desta fase volta à luz fixa de dia: o sol volta a ser Stationary no mapa, e saem o relógio, a cúpula noturna e os comandos. A névoa, o terreno e a população não dependem desta fase. Para desligar sem reverter, use `-VortexNoDayNight`.

## Avaliação pendente no editor

Teste estes pontos e diga o que achou:
- use `Vortex.Tempo.Escala 60` para ver um dia inteiro em ~38 s;
- confira o amanhecer e o pôr do sol, a noite (se ficou escura demais ou clara demais), a lua e as estrelas, e o efeito ao olhar para o sol entre as árvores;
- veja `Vortex.Hora 22` em dias diferentes, para comparar as fases da lua;
- repita como cliente.

**A fase não está declarada concluída antes desse teste.**
