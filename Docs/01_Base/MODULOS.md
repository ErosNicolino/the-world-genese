# Módulos do código (a separação para compilar menos)

Situação em 02/11/2026: ✅ `VortexWorld`, `VortexData` e `VortexUI` separados. Falta o combate e as criaturas (o plano).

## Por que

O código era um módulo só (`Source/Vortex`, ~51 mil linhas). Mudar um cabeçalho muito incluído recompilava quase
tudo (mais de 10 minutos; a compilação completa passa de 15). Com módulos em camadas, a Unreal só recompila o
módulo mudado e os que estão **em cima** dele. Os de baixo ficam como estavam.

## A regra das camadas

Um módulo só pode incluir cabeçalhos dos módulos **de baixo**. Quando o de baixo precisa de algo do de cima (tocar um
som, abrir um baú), ele chama um **gancho**: uma função que o de cima preenche ao abrir. É o caso do
`VortexWorldHooks`.

```
VortexUI              a interface: HUD, menu inicial, pausa, opções e as janelas
  └── Vortex          o jogo: herói, combate, criaturas, game mode, rede, saves
        ├── VortexWorld   o mundo: terreno, streamer, névoa, dia e noite, cidade, água, portais, marcos,
        │                 pontos de interesse, bichos do mato, e as opções do jogador
        └── VortexData    os catálogos dos JSON: aparência, rostos, RPG (itens, classes, perícias), tipos do save
```

Os dois de baixo não se conhecem: nenhum inclui o outro. O jogo não inclui a interface: chama ela pelos ganchos.

## VortexWorld (feito em 02/11)

- **Pasta:** `Source/VortexWorld`, com as mesmas subpastas de antes (`Public/World`, `Public/Framework`). Os
  `#include` continuam iguais (`"World/VortexTerrainStreamer.h"`).
- **Arquivos:**
  - `VortexTerrainData` (+ `VortexTerrainWorld.cpp`), `VortexTerrainStreamer`, `VortexFarTerrain`, `VortexMist`;
  - `VortexFountain`, `VortexForestLife`, `VortexWildlife`, `VortexTown`, `VortexWaterZone`, `VortexPortal`;
  - `VortexPointsOfInterest`, `VortexLandmarks`, `VortexDayNightSubsystem`;
  - `VortexProbe.h`, `VortexPlayerSettings`.
- **Novos no módulo:**
  - `World/VortexWorldHooks.h`: o que o mundo pede ao jogo. Toca som, lê o relógio do dia, abre e fecha os baús dos
    acampamentos e pontos de interesse, lê o céu da caldeira, gera a linha do teste do dia e cria o
    `AVortexWorldFeatures`. O jogo preenche em `Source/Vortex/Private/Framework/VortexWorldHookup.cpp`.
  - `World/VortexWorldLayout.h`: regras do mapa que os dois usam. A duração do dia, a cratera do Azhûr
    (`CalderaClearsVegetation`) e os acampamentos de bandidos (`BanditCampAt`). O `AVortexGameState`, o
    `UVortexCalderaSubsystem` e o `UVortexCreatureSubsystem` agora usam estas.
  - `Framework/VortexViewScales.h`: as escalas de grama e de distância das opções de gráficos. O `VortexGraphics.h`
    inclui este, então quem chamava `VortexGraphics::GrassDistanceScale()` não mudou.
- **Assets e saves:** as 15 classes que mudaram de módulo têm `[CoreRedirects]` no `Config/DefaultEngine.ini`
  (`/Script/Vortex.X` -> `/Script/VortexWorld.X`). O `L_TerrainProto` abre com o streamer certo.
- **Opções do jogador:** a seção do `GameUserSettings.ini` mudou de nome com o módulo. O
  `UVortexPlayerSettings::PostInitProperties` copia a seção antiga na primeira vez. No teste, copiou as 68 opções.
- **Conferido:** compila; o mapa abre com as classes redirecionadas; testes de fumaça do dia e da noite em rede e dos
  bandidos (ver "Testes" abaixo).

## VortexData (feito em 02/11)

- **Pasta:** `Source/VortexData`, com as subpastas de antes (`Framework`, `Rpg`, `Persistence`); os `#include` não mudaram.
- **Arquivos:** `VortexLook` (+ `VortexFaceStyles.h`), `VortexRpg`, `VortexSaveTypes.h`.
- **Sem ganchos:** os catálogos só leem os JSON e as texturas; não chamavam nada do jogo.
- **Saves:** os 4 structs refletidos (`FVortexIdentity`, `FVortexNpcState`, `FVortexWorldDelta`, `FVortexItemStack`)
  têm `+StructRedirects` no `Config/DefaultEngine.ini`.
- **O ganho é pequeno:** são ~1.500 linhas. Mexer no `VortexLook.h` ainda recompila quem inclui ele no módulo de cima;
  o que deixa de recompilar é o próprio catálogo. Serve mais para firmar a camada de baixo.

## VortexUI (feito em 02/11)

- **Pasta:** `Source/VortexUI`. Foram:
  - o HUD (`VortexAimHUD` + `VortexAimHUDCartoon.cpp`), o menu inicial (`VortexFrontEnd`), a pausa (`VortexPauseMenu`),
    as opções (`VortexOptionsPanel`), o retrato (`VortexPortrait`) e os enfeites (`VortexUiJuice`, `VortexUiKit`);
  - os `.cpp` das janelas: `VortexHeroWindow` (com a loja), `VortexDialogue`, `VortexSocialUi`, `VortexDevChat`,
    `VortexIntroComic`, `VortexAwakening`.
- **As janelas, pelo mesmo nome:** os cabeçalhos delas (`VortexHeroWindow.h`, `VortexDialogue.h`...) ficaram no jogo,
  e o jogo chama `VortexHeroWindow::Open` como antes. A definição no jogo (`Framework/VortexUiFacade.cpp`) passa para
  o gancho (`Framework/VortexUiHooks.h`); a interface preenche ao abrir (`VortexUI/Private/VortexUiModule.cpp`) com as
  funções de verdade, que viraram `VortexHeroWindowImpl::Open` etc. (`VortexUI/Private/VortexUiImpl.h`).
  - **Função nova numa janela:** declarar no cabeçalho do jogo, pôr na lista `SPEC` do
    `Tools/GenUiHooks.py` e rodar (`python Tools/GenUiHooks.py`): ele reescreve o `VortexUiHooks.h`, a fachada, o
    `VortexUiImpl.h` e o `VortexUiModule.cpp`. Depois, definir a função no `namespace ...Impl` do `.cpp` da janela.
- **O HUD:** o jogo só conhece `AVortexHudBase` (`Framework/VortexHudBase.h`, abstrata, com o que o herói, o controle,
  as opções e os testes perguntam). O `AVortexAimHUD` herda dela. O game mode não sabe a classe: a interface põe
  `HUDClass = AVortexAimHUD` quando ele começa (`FGameModeEvents::OnGameModeInitializedEvent`, no `InitGame`). Mudar
  o padrão do game mode ao abrir o módulo não pegou: os testes ficaram com o HUD vazio.
- **O menu inicial e a pausa:** o `UVortexGameInstance` pede o widget pelos ganchos `MakeFrontEnd`, `FrontEndNotice`
  e `MakePauseMenu`.
- **Ficaram no jogo:** as marcações do Z (`Framework/VortexPings.h`, o herói marca e o HUD desenha) e o contador dos
  sons de interface (`VortexAudio::UiSoundCount`/`LastUiSoundAt`: uma variável `inline` de cabeçalho vira uma cópia
  por módulo, então ela mora num `.cpp` do jogo). O palco 3D do menu (`AVortexMenuStage`) também ficou: é um ator.
- **Entra pelo target:** ninguém depende da interface, então ela está em `ExtraModuleNames` nos dois `*.Target.cs`.
- **Assets:** `+ClassRedirects` do `AVortexAimHUD` no `DefaultEngine.ini`.

## Como mover mais um pedaço

1. Ver o que os arquivos incluem dos módulos de cima. Cada uso vira uma destas três coisas:
   - a função desce junto, se ela também não depende de cima;
   - vira gancho;
   - a constante vai para um cabeçalho de baixo.
2. Mover os `.h`/`.cpp` com as mesmas subpastas e trocar `VORTEX_API` por `<MÓDULO>_API`.
3. Criar o `<Módulo>.Build.cs` e o arquivo do módulo (`IMPLEMENT_MODULE`). Pôr no `Vortex.uproject` e nas
   dependências de quem usa.
4. Pôr `+ClassRedirects` (e `StructRedirects`/`EnumRedirects`) no `DefaultEngine.ini` para cada tipo refletido.
5. Se a classe grava config (`UCLASS(config=...)`), migrar a seção como no `UVortexPlayerSettings`.
6. Compilar, abrir os mapas e rodar os testes de fumaça do que mudou.

## O plano dos próximos (do mais parado ao que mais muda)

1. **VortexWorld:** feito. Faltam no módulo de cima as partes do mundo que dependem do jogo: `VortexCaldera`,
   `VortexDungeons`, `VortexWorldFeatures`, `VortexCreatureSubsystem`, `VortexForage`, `VortexPopulationSubsystem`
   e `VortexAwakeningStadium`. Elas usam monstros, herói e missões.
2. **VortexData:** feito.
3. **VortexCombat + VortexCreatures:** precisam de ganchos para o herói (`AVortexCharacter`), porque o combate e
   os monstros usam ele. Antes, separar uma interface do herói (o que o combate pergunta: a ficha, a arma, a
   posição).
4. **VortexUI:** feito.

## Testes (02/11, depois da separação)

- **Compilação:**
  - a completa, depois de mexer no `VortexGameState.h`: ~18 min;
  - mexendo só no gancho (`VortexWorldHooks.h`, que os dois módulos incluem): 82 s;
  - mexendo só num `.cpp` do mundo: 37 s.
- **Mapas:** o `L_TerrainProto` abre com o `/Script/VortexWorld.VortexTerrainStreamer` (o redirecionamento funciona).
- `Tools/RunBanditSmoke.ps1`: **PASS**. Os acampamentos (o `BanditCampAt` desceu) e o baú do saque (o gancho
  `AddCampChest`): guardado, fechado com o bando de pé e aberto depois.
- `Tools/RunDayNightSmoke.ps1`: **PASS**. O relógio igual no servidor e no cliente (`relogio_T1000=553.784` nos dois).
  - No caminho, o teste falhava por tempo: o cliente travava uns segundos montando os efeitos e o servidor fechava
    antes de ele somar 1 s. Agora, no modo de teste, o cliente escreve a linha em todo quadro, e o script lê a última.
- **No cliente do teste:** a fonte, a cidade, os acampamentos e a névoa nascem do módulo novo, sem erro.

## Testes do VortexData (02/11)

- **Compilação completa** com o módulo novo: 14 min 25 s, sem erro.
- `Tools/RunSaveSmoke.ps1`: **PASS** (ida e volta do save do mundo e do jogador, e a recuperação do backup).
- `Tools/RunHeroSmoke.ps1`: **PASS** (heróis por mundo, recarga, escolhas de aparência, limite de 3, apagar).
- `Tools/RunPopulationSmoke.ps1`: **PASS** (15 postos, estado dos NPCs salvo e retomado).
- `Tools/RunShopSmoke.ps1`: **PASS** (itens do catálogo do RPG, compra e venda).

## Testes do VortexUI (02/11)

- **Compilação completa** com o módulo novo: ~11 min; depois, mexendo só na interface e num teste do jogo: 2,5 min.
- **Primeira rodada:** o HUD não entrou (`minimapa=0` nas fotos). O mapa de viajante da loja e a tela de carregamento
  das opções falharam por isso. Corrigido pondo o HUD no `InitGame` (ver acima).
- `Tools/RunMenuFlowSmoke.ps1`: **PASS** (criar, salvar, carregar, voltar à posição, menu: o menu inicial e a pausa
  pelos ganchos).
- `Tools/RunHudShot.ps1`: **PASS** (fotos com o HUD, o minimapa, o chat de testes, as janelas do herói e a pausa).
  - O teste do RPG nele esperava 2 pontos por nível; agora usa `VortexRpg::PointsPerLevel` (são 7 desde 02/11).
- `Tools/RunApplySmoke.ps1`: **PASS** (a tela de carregamento do "Aplicar" pela `AVortexHudBase`).
- `Tools/RunShopSmoke.ps1`: **PASS** (a janela da loja e o mapa de viajante revelando a carta no HUD).
- `Tools/RunHeroSmoke.ps1`: **PASS**.
