# Vortex Unreal — Fase 6: terreno de prova, personagem e animação

**Data da consolidação:** 04/10/2026
**Estado:** **concluída com aprovação do usuário em 04/10/2026** ("perfeito, pode finalizar essa etapa"), depois de jogar no editor colisão, esquivas, troca de roupa, grama com vento e pisada. O que não foi validado e o que passa para outras fases está em "Estado de validação e pendências".

No roteiro original, a Fase 6 é a base da animação. O terreno foi antecipado a pedido do usuário para testar o personagem numa cena representativa. Os trabalhos antes registrados como 6T e 6A–6D fazem parte deste mesmo acompanhamento, não acrescentam fases ao roteiro 0–23.

## Sequência e entregas

| Trabalho | Resultado atual | Commit |
|---|---|---|
| Terreno de prova | Região do Godot exportada, montada em pedaços e medida | `2989548` |
| Herói animado | Malha com pele, esqueleto e 12 clipes; movimentos e mapa vistos pelo usuário | `be153f1`, `a7766f6` |
| Obstáculos e folhagem | Colisão próxima do jogador; plantas pequenas trocadas por cobertura no chão | `6a4bd32` |
| Cápsula e esquiva | Corpo físico de 1,40 m e quatro direções com previsão de rede | `7cc1e38` |
| Lumen | Iluminação global e reflexos configurados e medidos | `2422b96` |
| Personalização modular | Primeiras 18 malhas, troca local por categoria e Leader Pose | `706f723` |

## 1. Terreno de prova

### O que foi construído

- `ValleyWorld.ExportUE.cs` no Godot ganhou `--export-ue-terrain=<pasta>`, sem mudar a geração do jogo. Com a mesma semente e ordem de sorteio, produziu `Data/GodotTerrain/terrain.bin` e `props.bin`. A região fixa tem cerca de **1,1 × 1,1 km**, **3.721 pedaços de 18 m**, **2,1 milhões de células** e **131 mil plantas e pedras de 54 espécies**. O Godot continua preservado como referência; os saves não foram tocados.
- `Tools/ImportGodotProps.py` e `Tools/FixPropMaterials.py` importaram os 54 modelos `.glb` em `/Game/Vortex/Props`, com Nanite. `M_VoxelProp` combina cor do vértice e atlas como no shader Godot. `M_TerrainVertexColor` colore o chão e `M_Water` desenha a água.
- `AVortexTerrainStreamer` gera em tarefa de fundo a malha visual em degraus e entrega até dois pedaços por quadro. A colisão suave é separada: média dos quatro cantos de cada célula, com rampas em degraus de 25–50 cm e paredões de 1 m ou mais íngremes demais. A colisão do solo fica até dois pedaços dos jogadores.
- O subsistema de água recebe um `Provider` do terreno, permitindo nadar nos lagos. Props pequenos ficam em lotes até cerca de 32 m, moitas e pedras até 58 m; árvores ficam em lotes por região de 72 m até 300 m.
- `L_TerrainProto` traz sol, céu, luz ambiente e névoa. `Tools/RunTerrainBench.ps1` percorre a mata a 1280×720, sem limite de FPS, com câmera a 7 m; o Godot recebeu `--bench-wild` para medir o mesmo trajeto sem criaturas, moradores ou interface.

### Primeira comparação com o Godot

Medição na mesma máquina (RTX 2060 SUPER, 16 GB), antes das etapas posteriores de colisão de obstáculos e Lumen, com shaders da Unreal aquecidos:

| Métrica | Godot | Unreal inicial |
|---|---:|---:|
| FPS médio | 165–171 | 125–130 |
| FPS nos 1% piores | 32–40 | 85–88 |
| Pior quadro | 59–289 ms | 20–24 ms |
| Quadros acima de 25 ms | 8–11 | 0 |
| GPU por quadro | 1,7 ms | 6,3 ms |
| Thread do jogo | — | 2,2 ms |
| Entrega de um pedaço na thread do jogo | ~1,0 ms | ~1,5 ms; máximo 2–5 ms |

A Unreal teve média menor, mas quadros mais estáveis. A thread de render (~7,7 ms e ~1.000 desenhos) era o maior custo daquela rodada. Uma captura (`-Shot`) pausa cerca de 0,9 s e não pode entrar na conta dos piores quadros. A primeira execução após alterar materiais compila shaders e também não representa desempenho aquecido. Na comparação visual do mesmo ponto, o usuário considerou a imagem da Unreal mais bonita.

## 2. Herói, esqueleto e animação

- `Tools/ConvertGodotAdventurer.py` transforma os **20 nós-osso** do aventureiro em esqueleto e junta as **35 peças do visual padrão** numa malha com pele: cada vértice fica 100% preso ao osso correspondente, reproduzindo o movimento rígido do Godot. `--export-ue-look` forneceu `Data/GodotCharacter/default_look.json`.
- O conversor copia **12 clipes**: parado, andar, correr, início/loop/pouso do salto, esquivas frontal/traseira/esquerda/direita, golpe e morte. As posições são escaladas para o herói de **1,40 m**. `Tools/ImportAdventurer.py` importa malha, esqueleto, clipes e material `M_AdventurerPalette` (paleta × cor do vértice).
- `UVortexAnimInstance` seleciona clipes em C++ pelo estado do movimento, mantém o ritmo de andar/correr do Godot e mistura poses por 0,15 s. `AVortexCharacter` usa a malha; o cilindro cinza aparece só se o modelo faltar. `-VortexAnimShot` em `L_TerrainProto` grava `Saved/Screenshots/anim_*.png` com parado, corrida, ar e esquiva.
- O primeiro import deixou membros encolhidos e a cabeça sobre o tronco: o fator 0,367 estava na matriz dos ossos, enquanto os clipes já tinham translações reduzidas. A correção aplica escala somente às posições. Reimportar sobre o `.glb` esquelético mantinha o esqueleto antigo; os assets tiveram de ser apagados e importados novamente.
- Para passar `/Game/...` por Git Bash, usar `MSYS_NO_PATHCONV=1` ou PowerShell; sem isso o shell troca o caminho Unreal por um caminho Windows.

**Validação do usuário em 04/10/2026:** jogou no `L_TerrainProto`, aprovou o visual do mapa e descreveu os movimentos como “perfeitos”. Esse teste ocorreu antes das mudanças posteriores de obstáculos, cápsula, Lumen e peças modulares; elas ainda pedem revisão no editor.

## 3. Colisão de obstáculos e folhagem do chão

- O exportador autorizado grava `solids.bin` com **30.245 formas `SolidSpec`** (8.888.777 bytes). A cópia usada está em `Tools/GodotExporter/ValleyWorld.ExportUE.cs`. Na nova exportação, `terrain.bin` e `props.bin` mantiveram SHA-256 idêntico ao anterior: só o arquivo de obstáculos foi acrescentado.
- `FVortexTerrainData` lê as formas por pedaço. O streamer ativa colisão de árvores, raízes, pedras e troncos perto de cada jogador, prioriza o pedaço atual e monta no máximo um grupo por quadro; libera corpos ao afastar mais de três pedaços. Troncos usam cápsulas, formas esféricas usam esferas e caixas usam caixas. Rochas convexas ainda usam uma **caixa orientada envolvente**; seus pontos detalhados permanecem no arquivo para revisão futura.
- As plantas pequenas saíram dos lotes de objetos individuais. Elas e amostras adicionais em células secas viraram tufos baixos numa malha por pedaço, sem colisão e visíveis até cerca de 32 m. Árvores, rochas, troncos e arbustos continuam como objetos próprios.
- Validação: `dotnet build TheWorldGeneses.csproj` passou com um aviso preexistente em `WaterLife.cs`; build `VortexEditor` passou; `-VortexSolidProbe` registrou **`SOLID_PROBE PASS`** ao atingir um tronco ativo. O passeio mediu **134,3 FPS médios**, **70,5 FPS nos 1% piores**, pior quadro **26,8 ms**, **1 quadro acima de 25 ms**, thread do jogo **1,94 ms**, render **7,32 ms** e GPU **6,39 ms**. Antes de limitar a criação da colisão a um pedaço por quadro, havia 11 quadros acima de 25 ms.
- O usuário aceitou a grama por enquanto, mas observou que os tufos triangulares por célula são **feios e repetitivos**. Melhorar variação, distribuição, cor e movimento continua pendente.

## 4. Cápsula e quatro esquivas

- A cápsula passou de **42 cm de raio × 1,92 m** para **25 cm de raio × 1,40 m**, com pés alinhados à malha. A altura aprovada da câmera foi preservada. Os limites de nado passaram para entrada com 98 cm de profundidade, saída abaixo de 62 cm e flutuação com cerca de 98 cm da sola à superfície.
- A esquiva escolhe `Dodge`, `Dodge_Back`, `Dodge_Left` ou `Dodge_Right` em relação à câmera no instante do comando. O corpo fica voltado para a câmera durante o rolamento; a direção física segue a entrada. A direção visual entra no movimento salvo para correções da previsão de rede.
- A cápsula menor revelou um vão entre rampa e paredão no `L_MovementProbe`; o paredão foi aproximado. `Tools/AdjustCliffForHeroCapsule.py` registra a mudança e `Tools/CreateMovementProbeMap.py` a reproduz. Limites verticais dos testes e posição das capturas foram ajustados.
- Validação: build passou; `RunMovementSmoke.ps1`, `-StepProbe` e `-Matrix` passaram (cinco pistas: rampas de 20°, 40° e 50°, escada e piscina); `RunDodgeDirectionSmoke.ps1` passou nas quatro direções; `RunClientInputSmoke.ps1` passou normal e com **120 ms de atraso / 2% de perda**; `RunMovementNetworkSmoke.ps1` passou. Capturas de animação foram geradas, incluindo `Dodge_Left` com a câmera voltada de lado.
- Medição sem renderização após esta mudança: **0 personagens 0,635 ms**, **50 2,263 ms**, **150 5,555 ms** por quadro. Os números anteriores da Fase 5 não isolam o custo da cápsula, pois ainda não incluíam a malha animada.

## 5. Lumen

- `Config/DefaultEngine.ini` liga iluminação global e reflexos Lumen, campos de distância e sombras virtuais; o ray tracing por hardware fica desligado. A configuração é feita antes da inicialização e da compilação dos shaders. `Tools/RunTerrainBench.ps1` mede DX12 fora da tela, em janela oculta; o perfil `lumen` confirma GI/reflexos ativos e o perfil `leve` os desliga.
- Linha de base imediatamente anterior, sem Lumen: **135,4 FPS médios**, **68,9 FPS nos 1% piores**, pior quadro **26,5 ms**, render **7,26 ms**, GPU **6,35 ms**. Primeira execução com Lumen: **125,7 FPS**, 1% piores **40,5 FPS**, pior quadro **187,2 ms** durante preparação; não é resultado estável. Execução aquecida: **132,4 FPS médios**, **72,1 FPS nos 1% piores**, pior quadro **26,0 ms**, render **7,45 ms**, GPU **6,44 ms**, com `gi=1`, reflexos=1, sombras virtuais=1 e Nanite=1. Média cerca de 2,2% abaixo da linha de base nessa rodada.
- Capturas locais em `Saved/Screenshots/terrain_pre_lumen.png` e `terrain_bench.png` ficaram quase iguais. O chão e a água são `UProceduralMeshComponent`; a [documentação técnica da Epic](https://dev.epicgames.com/documentation/en-us/unreal-engine/lumen-technical-details-in-unreal-engine) descreve a geometria suportada pelo Lumen por software com malhas estáticas, instanciadas e Landscape. Árvores e rochas entram nessa representação; o chão procedural não recebe o mesmo tratamento. Converter o terreno exigirá trabalho próprio.
- Em DX11 fora da tela, Lumen iniciou (`gi=1`, reflexos=1) e mediu **96,5 FPS**, GPU **10,36 ms**, sem linha de base DX11 equivalente. [Sombras virtuais não são suportadas em DX11](https://dev.epicgames.com/documentation/en-us/unreal-engine/supported-features-by-rendering-path-for-desktop-with-unreal-engine), mesmo que o registro mostre a configuração solicitada. O editor usa `AbrirEditor_DX11.bat` porque sua interface apresenta rastros de janelas em DX12 nesta máquina; o jogo foi medido em DX12. A causa exata dos rastros ainda não foi isolada.

## 6. Primeira base de personalização modular

- `Tools/ConvertGodotWardrobe.py` lê `wardrobe.json` e o `Adventurer.glb` do Godot **sem escrever neles** e gera **18 malhas**: corpo, rosto fixo, alça e cinto, sete peças padrão e sete alternativas de cabelo, olhos, boca e roupa. O manifesto registra categoria e grupos. O conversor do aventureiro passou a aceitar chamadas em lote sem reler o GLB para cada peça.
- `Tools/ImportWardrobe.py` importa as malhas com `Adventurer_Skinned_Skeleton` pela pipeline Interchange, sem reimportar o herói animado por cima. `AVortexCharacter::SetWardrobePiece` aceita as opções importadas em sete categorias. Na primeira troca, cria **11 componentes** com **Leader Pose**, oculta a malha visual original e mantém sua animação como líder. Ao trocar a túnica, oculta alça e cinto como no Godot. Sem troca, usa a malha única aprovada e não paga o custo de componentes extras por NPC.
- Os componentes recebem o material da paleta original em tempo de execução; a pasta modular está incluída no empacotamento. `-VortexWardrobePreview` mostra as sete alternativas, e `-VortexWardrobeProbe` verifica componentes, Leader Pose e esqueleto comum.
- Validação: **`WARDROBE_IMPORTED 18`**, build passou, capturas padrão e alternativa alinhadas em `L_TerrainProto`, **`VORTEX_WARDROBE_PREVIEW PASS`** e **`VORTEX_WARDROBE_PROBE PASS parts=11`**.
- Benchmark sem renderização: **6 padrão 0,857 ms**, **50 padrão 2,326 ms**, **150 padrão 5,462 ms** por quadro; seis personagens com peças modulares forçadas **1,524 ms**. A primeira montagem com 11 componentes em *todos* os personagens custava 6,221 ms com 50 e 16,895 ms com 150; foi substituída pela montagem sob demanda. Os testes de 50/150 são de carga com personagens simulados, não uma meta de jogadores. Ainda falta medir seis personagens modulares renderizados na cena.
- A importação Interchange manteve materiais, texturas, esqueletos e assets de física auxiliares por peça (**90 assets, 28,6 MB**), embora as malhas usem o esqueleto comum e os componentes recebam o material correto. A limpeza deve respeitar as referências do importador para não quebrar a reimportação.

## 7. Correções após o teste do usuário (04/10/2026)

O usuário testou no editor e **atravessou todas as árvores**; a esquiva com Ctrl funcionou. O restante não foi testado por falta de instruções (ver "Como testar no editor" abaixo).

- **Causa da colisão:** em `FVortexTerrainData::Load`, a chave do pedaço era lida com `FIntPoint(S.Read<int32>(), S.Read<int32>())`. Em C++ a ordem de avaliação dos argumentos não é definida, e o MSVC lia da direita para a esquerda: **X e Y trocados**. Os obstáculos do pedaço (a, b) eram ativados quando o jogador estava no pedaço (b, a), longe dali. O `-VortexSolidProbe` passava porque só procurava *algum* tronco ativo, sem conferir se estava perto do jogador. Corrigido com leituras em linhas separadas.
- **Teste novo:** `Tools/RunSolidWalkSmoke.ps1` (`-VortexSolidWalkSmoke`) faz o herói **andar com entrada de movimento** contra 6 troncos e 3 rochas reais do terreno. Os troncos param o corpo a ~25 cm da casca (raio da cápsula). Rochas alongadas e giradas podem desviar o corpo pela face inclinada, como no Godot; o teste aceita parar ou desviar 80 cm ou mais. Resultado: **`SOLID_WALK_SMOKE_PASS failed=0`**. Com o erro antigo, o mesmo teste falhava (atravessava 7 m além do tronco).
- **Um corpo por pedaço, rochas com o formato real:** os obstáculos de cada pedaço viraram **um só `UProceduralMeshComponent`** com uma forma convexa por obstáculo, cozida fora da thread do jogo (`SetCollisionConvexMeshes`). Cilindros e esferas usam polígonos de 12 lados, como os do Godot, e as rochas usam a **casca convexa exportada** (antes, uma caixa envolvente). Antes era um componente por obstáculo.
- **Plantas reais de volta:** a etapa 6A tinha trocado *todas* as plantas pequenas (grama, flores, samambaias, trigo, juncos, girassóis, morangos, ~100 mil cópias) por tufos de 4 triângulos. Agora elas voltam a ser os modelos voxel do Godot em lote, sem sombra, até 32 m. O preenchimento entre elas ficou mais variado: 5 a 7 folhas curvas por tufo, altura, inclinação e tom próprios, base com a cor da célula, só em chão verde e em ~30% das células.
- **Montagem mais leve:**
  - grupos de objetos e tufos são calculados na tarefa de fundo;
  - cada espécie entra com `AddInstances` de uma vez;
  - `UInstancedStaticMeshComponent` no lugar do HISM (com Nanite, a árvore de grupos do HISM só custava montagem);
  - 1 pedaço entregue por quadro;
  - as regiões de árvores entram 2 por quadro (antes, uma fileira de ~13 no mesmo quadro).
- **Personagens fora da tela:** a pose não é calculada se ninguém está vendo (`OnlyTickPoseWhenRendered`). O herói controlado e os personagens com peças modulares (a malha base é a líder da pose) continuam animando sempre.
- **Teclas de teste no editor** (fora da versão final): **1 a 7** alternam cabelo, olhos, boca, camisa, calça, botas e luvas; **F** gira a câmera para ver o rosto.

### Medições (mesmo passeio, DX12, 1280x720, sem limite de FPS)

| Situação | FPS médio | 1% piores | Pior quadro | Quadros > 25 ms |
|---|---|---|---|---|
| Entrega do outro chat (6A–6D) | 132–134 | 70,5–72,1 | 26–27 ms | 1 |
| Colisão corrigida + corpo único por pedaço (sem plantas reais) | 133 | 79 | 17–20 ms | 0 |
| + plantas reais, 2 pedaços por quadro | 126 | 55–61 | 19–24 ms | 0 |
| + montagem em segundo plano e 1 pedaço por quadro (final) | 127,4 | 72,0 | 20,1 ms | 0 |
| Godot no mesmo passeio | 165 | 40 | — | — |

O FPS médio fica limitado pela **thread de render (~7,7 ms)**, não pela placa de vídeo.

**Lumen:** com Lumen desligado na inicialização (perfil `leve` agora usa `-ini:Engine`, porque o comando com o jogo rodando não muda nada), mediu **129 FPS / 1% piores 72,7**. As capturas com e sem Lumen ficaram **iguais** nesta cena aberta: o chão procedural não entra no Lumen por software, e o céu e o sol fazem quase toda a luz. O Lumen fica ligado (custo baixo aqui). Fazer o chão entrar no Lumen é uma decisão para o usuário: ray tracing por hardware (pesado na RTX 2060 SUPER) ou chão em malha estática/Landscape (perde a geração em tempo real).

**Custo por personagem (sem renderização):** 0 → 0,698 ms; 50 → **1,894 ms** (antes 2,326); 150 → **4,202 ms** (antes 5,462).

**Assets extras da importação:** os "90 assets" incluem as 18 malhas necessárias. As sobras reais são 72 arquivos pequenos (paleta, textura, física e esqueleto por peça, ~2,8 MB), e as malhas ainda apontam para a paleta delas. Não foram apagados: o ganho é pequeno e o risco de quebrar a reimportação é real.

**Correções de rede com 150 ms + 3%:** agora 6–7 no servidor (Fase 5: 1). Todas ocorrem nos primeiros 0,2 s, na queda do nascimento: a cápsula menor (6B) deixa o ponto de nascimento ~85 cm acima do chão, e cada passo em trânsito nessa queda conta como uma correção. Nenhuma ocorre durante corrida, salto ou esquiva. Confirmado que não vem da mudança de animação: o teste com a animação sempre ligada deu 7.

**Regressão (todos passaram):**
- movimento;
- matriz (rampas, escada, piscina);
- entrada do cliente sem atraso e com 150 ms + 3%;
- rede do framework e do movimento;
- saves com backup;
- quatro esquivas;
- `VORTEX_WARDROBE_PREVIEW PASS`;
- `VORTEX_WARDROBE_PROBE PASS parts=11`;
- `SOLID_WALK_SMOKE_PASS`.

### Como testar no editor

Abra pelo `AbrirEditor_DX11.bat`, depois abra `Content/Vortex/Maps/L_TerrainProto` e aperte **Play** (Alt+P).

1. **Colisão:** ande contra árvores, pedras grandes, tocos e troncos caídos. Você deve parar ou deslizar, nunca atravessar.
2. **Esquivas:** Ctrl + W, A, S ou D (frente, esquerda, trás, direita), conferindo se o rolamento vai para o lado certo.
3. **Roupas:** aperte 1 a 7 para alternar as peças e **F** para ver o rosto de frente.
4. **Grama e plantas:** veja se a mata ficou mais natural e se aparecem flores, samambaias e girassóis.

## 8. Segunda rodada de correções (04/10/2026)

O usuário testou a seção 7 no editor. Colisão, esquivas e câmera do rosto funcionaram. Dois problemas restaram:
- a troca de roupa deixava a peça antiga como uma silhueta preta, por baixo e por cima da nova;
- a grama ainda parecia "um único fio", e restavam plantas de chão antigas (a grama voxel, em blocos).

**O que foi feito:**
- **Troca de roupa:** a malha original (que só serve de pose para as peças) estava fora do passo de cor, mas continuava no **passo de profundidade**. Ali ela desenhava a peça antiga como uma silhueta escura. Agora ela fica escondida de verdade (`SetVisibility(false)`) e continua animando (`AlwaysTickPoseAndRefreshBones`). O teste `Tools/RunWardrobeShot.ps1` (`-VortexWardrobeShot`) fotografa o herói de frente com as peças padrão e com as alternativas. A foto de antes reproduzia o defeito; a de depois mostra só as peças novas.
- **Grama com vento:**
  - `Tools/CreateGrassMaterial.py` cria o `M_GrassWind`: folha de duas faces com luz atravessando (*Two Sided Foliage*) e vento calculado na placa de vídeo (onda por folha mais uma rajada lenta que atravessa o campo);
  - a cor é calculada no material (raiz escura, ponta no tom do tufo, ~8% das folhas secas), a partir de UV0 e UV1 gravados em cada vértice, porque a montagem rápida da malha não guarda a cor de vértice;
  - o material precisa da marca `used_with_instanced_static_meshes`, senão a Unreal usa o cinza padrão.
- **Touceiras:** 6 variantes montadas por código ao abrir o mapa (`BuildGrassMeshes`, sem asset importado). Cada uma junta três tufos de 6 a 10 folhas finas e curvas.
- **Onde a grama aparece:**
  - repetida por cópias (`UInstancedStaticMeshComponent`), que somem por cópia até 30 m;
  - 0 a 3 touceiras por célula verde, conforme um ruído de campo (manchas densas, ralas e clareiras);
  - a grama voxel do Godot (`grass_short`, `grass_tall`, `grass_seed`, ~72 mil cópias) virou essa grama nos mesmos lugares;
  - flores, samambaias, trigo, juncos e girassóis continuam como modelos voxel.
- **Tentativa descartada:** a primeira versão, com uma malha procedural de grama por pedaço, custava **5,4 ms** por pedaço na thread do jogo e trazia engasgos de volta (1% piores 35). Com as cópias, custa **0,6 ms**.
- `Config/DefaultGame.ini` passou a cozinhar `/Game/Vortex/Materials`, porque os materiais são carregados por caminho no código.

**Medição final (mesmo passeio):** **121,5 FPS**, **1% piores 67,3**, pior quadro **22,1 ms**, nenhum quadro acima de 25 ms, ~937 desenhos.

**Testes:** `SOLID_WALK_SMOKE_PASS`, `DODGE_DIRECTION_SMOKE_PASS`, `MATRIX_SMOKE_PASS`.

**Avaliação do usuário:** aprovou a grama, pediu-a menor e mais densa (seção 9). A troca de roupa funcionou.

## 9. Grama menor, mais densa e pisável (04/10/2026)

O usuário aprovou a grama com vento e pediu duas coisas: grama um pouco menor e em maior quantidade, e que ela reagisse a pisar e passar por cima.

- **Tamanho e quantidade:**
  - folhas com 16–38 cm (antes 24–54 cm);
  - 0 a 4 touceiras por célula, conforme o ruído do campo (antes 0 a 3);
  - a grama que substitui a voxel do Godot ficou menor (alta 1,1×, com semente 1,0×, baixa 0,8×).
- **Pisada:**
  - o `M_GrassWind` recebe a posição dos pés dos 4 personagens mais perto da câmera (`Player0..3`), inclusive jogadores remotos;
  - o streamer manda esses valores a cada quadro por uma instância dinâmica única do material, usada por todas as touceiras;
  - até ~75 cm do pé, a folha se abre para fora e deita, mais na ponta que na raiz;
  - só vale para grama na altura do pé (±1,2 m), então um degrau acima ou abaixo não é afetado;
  - `-VortexNoGrassStep` desliga a pisada para comparação.
- **Defeito encontrado e corrigido:** o parâmetro de vetor ligado pela saída padrão entrava com 3 valores e o material não compilava (a Unreal usava o material padrão, escuro). Agora o script liga a saída `RGBA` e para com erro se uma ligação falhar.
- **Teste:** `Tools/RunWardrobeShot.ps1` grava também `grass_step.png`, com o herói em pé num trecho de grama, visto de cima. A comparação com a pisada ligada e desligada mostra a grama aberta em volta do corpo.
- **Medição:** **122 FPS**, **1% piores 66–67**, pior quadro ~22 ms, nenhum quadro acima de 25 ms (igual à rodada anterior).
- **Testes:** esquivas e colisão passaram de novo.

**Avaliação do usuário:** aprovada ("perfeito").

## Estado de validação e pendências

**Pisada para até 8 personagens (pedido no fechamento):** o `M_GrassWind` tem `Player0..7`, e o streamer manda os pés dos 8 personagens mais perto da câmera. O material compila e a foto `grass_step.png` mostra o efeito com um personagem. **Não foi verificado visualmente com vários personagens ao mesmo tempo**; isso fica para o primeiro teste com mais jogadores.

**Aprovado pelo usuário jogando no editor (04/10/2026):**
- terreno e visual do mapa;
- movimentos;
- colisão com árvores, pedras e troncos;
- esquivas nas quatro direções;
- troca de roupa e rosto de frente (teclas 1 a 7 e F);
- grama com vento, menor e mais densa, e a pisada.

**Testes automáticos no fechamento (todos passaram):**

| Teste | Resultado |
|---|---|
| Passeio na mata (2 rodadas) | 121,4–121,8 FPS; 1% piores 66,4–66,8; pior quadro 22 ms; nenhum quadro acima de 25 ms |
| `SOLID_WALK_SMOKE` | Passou (6 troncos e 3 rochas) |
| `DODGE_DIRECTION_SMOKE` | Passou (4 direções) |
| `MOVEMENT_SMOKE` | Passou |
| `MATRIX_SMOKE` | Passou (rampas, escada, piscina) |
| Entrada do cliente, sem atraso | Passou (0 correções) |
| Entrada do cliente, 150 ms + 3% | Passou (correções só na queda do nascimento, ver seção 7) |
| Rede do framework e do movimento | Passou |
| Saves e backup | Passou |
| Custo por personagem | 0,693 / 1,852 / 4,160 ms com 0 / 50 / 150 |
| Peças modulares | `VORTEX_WARDROBE_PREVIEW PASS`, `VORTEX_WARDROBE_PROBE PASS parts=11` (seção 7) |

**Sem validação do usuário, por ter efeito pequeno nesta cena:** Lumen. As capturas com e sem Lumen ficaram iguais (seção 7). Fazer o chão entrar no Lumen fica como decisão futura do usuário (ray tracing por hardware ou chão em malha estática/Landscape).

**Passa para fases seguintes:**
- **Personalização:** restante do catálogo de peças (`rw_*`/`undead_rw_*` em `RacePieces.glb`, demais roupas, cabelos e armaduras), cores por instância, interface, save e **replicação da aparência** (hoje a troca é local).
- **Teste de rede:** nado dentro da água em rede; pisada com vários jogadores.
- **Lobo:** **Fase 10**, após a base de habilidades, combate e arma.
- **Mundo e ambiente:**
  - gerador de terreno próprio em C++;
  - texturas das 21 camadas do solo;
  - vento nas árvores;
  - efeitos;
  - estruturas;
  - otimização da thread de render (~8 ms, hoje o limite do FPS).
- **Limpeza da importação:** as sobras são pequenas (~2,8 MB), ver seção 7.
- **Teclas de teste** 1–7 e F: existem só fora da versão final e devem sair quando houver interface de personalização.

## Riscos, desempenho e reversão

- O terreno atual vem de uma **região fixa exportada** do Godot; o mundo final precisará do gerador em C++ ou de exportação mais ampla. A representação procedural limita o ganho visual do Lumen por software. O custo de campos de distância e a primeira preparação de shaders podem aumentar tempo de abertura, RAM e espaço em disco na máquina de 16 GB.
- Direção visual remota da esquiva e personalização ainda local são limitações conhecidas. A câmera da captura mostra as costas do herói; revisar o rosto no editor.
- Cada entrega acima ficou em commit próprio, listado na tabela. Reverter um commit específico retorna aquele comportamento no projeto Unreal; para desfazer dependências posteriores, reverter em ordem inversa. O Godot e os saves permanecem separados. No Godot, só foram acrescentados código de exportação/medição em `ValleyWorld.ExportUE.cs` e ferramentas autorizadas em `Tests/`; a geração normal e os saves não foram alterados.
