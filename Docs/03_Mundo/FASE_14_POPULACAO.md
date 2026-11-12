# Fase 14 — população do vale (aguardando teste no editor)

## O que mudou e por quê

- **Dados locais mínimos.** `Tools/ExtractNpcPosts.py` lê `the-world-geneses/data/npcs.json` **somente para leitura**; o arquivo do Godot ficou com o mesmo tamanho e a mesma data. O script grava `Data/Population/npc_posts.json` com ID, nome, título, posto, direção e patrulha dos 14 NPCs. As coordenadas passam para o Unreal em cm (x do Godot → X, z → Y), e a direção do Godot vira o yaw `atan2(−cos θ, −sin θ)`. Aparência, falas e diálogos continuam no Godot até as fases próprias.
- **Estado separado do ator.** `FVortexNpcState` guarda, por ID estável, a posição, o giro, o ponto da patrulha, o número de aparições e se o NPC já saiu do posto. O estado vive em `UVortexPopulationSubsystem`, só no servidor e só em mapas com terreno, e entra no save do mundo (`UVortexWorldSave::NpcStates`). Esse campo é novo, com valor padrão vazio, então saves da versão 1 continuam válidos. IDs de um save que não existem nos dados são preservados na próxima gravação.
- **Aparição perto dos jogadores.**
  - O NPC aparece quando um jogador chega a menos de **64 m** e o chão de colisão daquele ponto já existe. São no máximo 2 por atualização, a cada 0,25 s.
  - Some quando todos os jogadores estão a mais de **78 m**, mesma regra do Godot. A diferença entre as duas distâncias evita que ele apareça e suma sem parar na divisa.
  - Ao sumir, guarda onde estava. Longe de todos, o NPC não é simulado e o estado fica parado.
  - O chão é garantido pela Fase 13: o servidor mantém colisão até 4 pedaços (≥ 72 m) de cada jogador e só a solta além de 5 pedaços (≥ 90 m).
- **Fora do protótipo.** Sahil e Amal (dunas, ~2,2 km a oeste) e Tavian e Lyra (Verdalume, ~2,9 km a leste) existem só como estado. Nunca ganham ator, porque não há chão exportado lá.
- **`AVortexNpc`.**
  - Usa o aventureiro e a animação do herói. A animação agora também aceita personagens que não são o herói, só para andar, correr, parar e cair.
  - Lary e Juh seguem a patrulha do Godot a 1,6 m/s, com pausas de 2 a 5 s, estáveis por ID e ponto. Os demais ficam no posto, virados para a direção original, sem tick.
  - No cliente, o NPC fica oculto até o chão local daquele ponto carregar.

## Testes

- Compilação `VortexEditor Win64 Development`: passou.
- **`RunPopulationSmoke.ps1` (novo), jogo local:** passou.
  - Os 10 NPCs do protótipo apareceram na praça em 1,25 s; nenhum dos 4 de fora apareceu; nunca houve dois atores com o mesmo ID.
  - Longe, todos sumiram e o estado ficou parado.
  - Histerese com a Rhea: a 70 m ela não apareceu, a 50 m apareceu, de volta a 70 m continuou e a 85 m sumiu, com exatamente uma aparição.
  - De volta à praça, a Lary reapareceu onde estava (até 25 cm) e no mesmo ponto da patrulha. Ela tinha andado 3,35 m até o ponto 1.
  - O estado dos 14 foi gravado no save do mundo e lido de volta igual. O slot de teste é apagado no fim.
- **`RunPopulationSmoke.ps1 -Network`, servidor dedicado + cliente:** passou. O cliente viu exatamente os 10 IDs do protótipo (`bram, ducci, garrick, ilsa, juh, lary, marta, nicolly, rhea, rosa`), com 22 aparições e 12 sumiços, os mesmos números do servidor.
- **`RunPopulationSmoke.ps1 -Shot`:** [foto da praça](../Images/fase14_praca.png), com render ligado.
- Regressões: `RunSaveSmoke`, `RunSwordSmoke`, `RunCameraSmoke`, `RunDodgeDirectionSmoke`, `RunWolfNetworkSmoke`, `RunFrameworkNetworkSmoke` e `RunStreamNetworkSmoke`: todas passaram.

## Custo

| Medida | Local | Dedicado |
|---|---|---|
| Atualização da população (a cada 0,25 s), média | 0,38–0,40 ms | 0,17 ms |
| Atualização, pior caso | 4,7–7,6 ms (com duas aparições) | 1,9 ms |
| Uma aparição (ator com malha esquelética) | 2,1–6,3 ms; o maior é a primeira, que carrega a malha | 1,0 ms |

`RunTerrainBench -Profile alto`: 74,8 FPS, 1% pior 57,4, ou seja, igual à Fase 13. O trajeto do benchmark não passa pela praça, então isso mostra só que a população não custa nada longe dos NPCs. O custo de desenhar 10 personagens perto não foi medido isoladamente.

## Riscos

- Todos os NPCs têm a aparência básica do aventureiro. Peças, cores, falas, diálogo e interação ficam para as próximas fases de NPC.
- A patrulha não desvia de obstáculos (no Godot havia `Keep`/`stuck`). Os trechos atuais da praça estão livres; uma patrulha que cruze árvores ou rochas precisa disso.
- Até 2 aparições por atualização podem somar ~12 ms num quadro no pior caso local. Se pesar no editor, reduza `SpawnsPerUpdate` para 1; a praça leva ~2,5 s para encher.
- Com servidor dedicado e o NPC parado no posto, o giro só é aplicado na criação. Ainda não há lógica de olhar para o jogador.
- O save do mundo ganhou o campo, mas o jogo ainda não grava sozinho. O mundo temporário não é salvo automaticamente, e carregar um mundo salvo deve chamar `ReadFrom` quando a seleção de mundos existir.

## Reversão

Reverter o commit desta fase remove a população, o NPC, os dados locais e o campo novo do save. A animação volta a aceitar só o herói. As Fases 12 e 13 não dependem desta. Para desligar sem reverter, use `-VortexNoPopulation` na linha de comando.

## Avaliação pendente no editor

No `L_TerrainProto`, ande até a praça (perto de 0, 15 m):
- os moradores devem aparecer em pé no chão, sem cair;
- Lary e Juh devem passear;
- afaste-se além de ~80 m e volte: eles devem estar onde ficaram;
- repita como cliente.

**A fase não está declarada concluída antes desse teste.**
