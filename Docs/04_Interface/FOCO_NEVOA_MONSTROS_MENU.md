# Foco no alvo, névoa da borda, monstros que revidam, dano em desenho animado e menu novo (23/10)

## Pedidos

1. O avanço automático até o inimigo deixava o herói "preso, andando sozinho". No lugar: uma marcação de foco no inimigo, com opção de tirar e deixar a tela livre.
2. Uma névoa densa que sempre esconda a borda do mundo sendo gerado (dava para ver as árvores se formando).
3. Monstros melhores:
   - no combo, ficavam parados levando golpe;
   - o morcego, depois de um golpe forte (como o secundário das manoplas), ia para trás sem parar.
4. Números de dano maiores, em estilo de desenho animado.
5. Menu novo:
   - Jogar, Multiplayer, Configurações, Conquistas e Sair;
   - um jogador: até 5 mundos, 1 herói por mundo;
   - multiplayer: mundos separados, até 3 heróis;
   - na sala, cada um cria o seu herói e marca pronto, e o anfitrião dá o pronto final.

## O que mudou

### 1. Foco no alvo, sem andar sozinho

- **Avanço:** o avanço automático (`LungeToward`) saiu. O herói só se **vira** para o inimigo ao bater.
- **Marcação de foco:** quatro cantos dourados que respiram em volta do inimigo para onde o golpe de perto vai (`AVortexAimHUD::DrawFocus`). É o mesmo inimigo que o golpe escolhe (`AVortexCharacter::PreviewMeleeTarget`).
  - Aparece só com arma de perto, na terceira pessoa e sem mirar.
- **Opção:** Configurações → Interface → **Marcar o alvo do golpe** (Sim/Não). Desligada, a tela fica livre.
- **30/10: a marca saiu** (pedido do usuário). Também saiu a opção do menu. Ficou só a troca de foco com uma virada rápida do mouse. O golpe agora prefere o inimigo ao alcance da arma, em vez de um preso no foco que esteja longe. Detalhes em [ARMAS_HEROIS_SLIME](../02_Combate/ARMAS_HEROIS_SLIME.md).
- **Teste do golpe de perto:** o golem fica agora ao alcance da lâmina (era 2,4 m da borda, a distância do alcance antigo).

### 2. Névoa densa na borda do mundo

- **A causa:** a névoa só era regulada depois que o horizonte inteiro terminava de montar. Até lá, ou seja, enquanto o mundo se formava, valia a do mapa. Depois ela ficava fina (0,09), e de cima dos morros (afinava rápido com a altura) dava para ver o mundo se formando.
- **Agora:**
  - a névoa é regulada **desde o primeiro quadro** (`AVortexFarTerrain::TuneFog`);
  - limpa até 60 m e densa depois (0,45), fechando antes da borda do terreno detalhado (~108 m) e das árvores que nascem (150 m);
  - quase igual em qualquer altura (queda 0,025, era 0,12);
  - no máximo 98,5%, então as montanhas distantes ficam só em silhueta.
- **Como foi conferido:** testes com 0,45, 1,5, 3, 5 e 12. Começando a 5 m a névoa engole tudo; com início em 60 m, ela já fecha por volta de 70–80 m.

### 3. Monstros que revidam

- **Fúria (poise), como nos jogos de ação** (`AVortexMonster::OnHealthChanged`):
  - depois de 3, 4 ou 5 golpes seguidos (os grandes aguentam mais), o monstro **revida na hora**, com os golpes de perto prontos;
  - os ágeis (que voam ou são rápidos) às vezes **saltam para trás e de lado** antes;
  - por 1,6 s ele não encolhe nem é atordoado, e os golpes comuns não o atordoam por mais 1 s.
- **Lobos (`AVortexWolf`):** no 3º golpe seguido, saltam para trás ou mordem de volta.
- **Menos congelamento:** a pausa de impacto em quem apanha caiu de 65 ms para 35 ms. Quem bate continua com 65 ms; no crítico, 100 ms.
- **Entre um ataque e outro:** o monstro ronda de lado mais rápido (60% da velocidade, era 40%), para não parecer parado.
- **Morcego empurrado:** o empurrão tira o morcego do modo de voo. Antes ele ficava preso fora do voo e deslizava sem parar. Agora freia no ar e volta a voar assim que desacelera.

### 4. Números de dano em desenho animado

- **Fonte:** Luckiest Guy, uma letra de desenho animado. Baixada do repositório oficial do Google Fonts, licença Apache 2.0, e guardada com a licença em `Content/Vortex/UI/Fonts/`.
- **Visual:** contorno escuro grosso e sombra deslocada.
- **Tamanhos:** 80 no golpe comum (era 36), 112 no crítico (era 50) e 52 no dano no herói (era 30).
- **Animação:**
  - o número "estoura": cresce passando do tamanho e volta em 0,22 s;
  - pula para o lado num arco e some no fim;
  - o crítico é laranja, treme ao nascer e leva "CRÍTICO" em cima.

### 5. Menu novo

- **Início:** Jogar, Multiplayer, Configurações, Conquistas e Sair, em lista (o Sair saiu do canto).
- **Jogar:** os mundos de um jogador, até 5, com **1 herói** cada.
  - O mundo abre direto, sem sala.
  - Criar o mundo leva à criação do herói dele.
  - O cartão "Continuar" leva ao último mundo de um jogador.
- **Multiplayer:**
  - **Criar sala:** os mundos de multiplayer, outros 5, separados dos de um jogador, com até **3 heróis** cada. "Abrir sala" leva à sala de espera.
  - **Entrar na sala:** o código de um amigo, como antes.
- **Sala de espera:** a que já existia. Os heróis ficam lado a lado num arco; cada amigo entra, cria o herói e marca "Estou pronto". O botão do anfitrião virou **"Pronto! Começar aventura"** e só funciona com todos prontos.
- **Conquistas:** 14 conquistas tiradas dos heróis salvos neste computador, com barra de progresso:
  - criar herói;
  - níveis 5, 25 e 50;
  - o Despertar;
  - 5, 15 e 31 missões;
  - domar cavalo;
  - 5% e 25% do mapa descoberto;
  - fazer um amigo;
  - criar um mundo de multiplayer;
  - 1.000 moedas.

  O troféu foi desenhado no Blender (`Tools/Blender/MakeTrophyIcon.py`) e importado por `Tools/ImportMenuIcons.py`.
- **Save:** o mundo ganhou `bMultiplayer`. Mundos antigos aparecem em Jogar. Um mundo antigo que já tenha 2 ou 3 heróis continua com eles, mas não ganha outros.

## Fotos

| | Antes | Depois |
|---|---|---|
| Menu (início, mundos, heróis, multiplayer, conquistas) | `Docs/Images/menu_novo/antes_*.png` | `Docs/Images/menu_novo/depois_*.png` |
| Névoa no horizonte (Nivaluz, Raikor, Aurivale) | [folha: em cima antes, embaixo depois](../Images/nevoa_ia_dano/folha_nevoa.png) | |
| HUD em combate (números e foco) | [folha: em cima antes, embaixo depois](../Images/nevoa_ia_dano/folha_hud.png) | |
| Início do menu | [foto](../Images/menu_novo/antes_inicio.png) | [foto](../Images/menu_novo/depois_inicio.png) |

## Validação

Ver a seção "Resultados" no fim.

## Riscos

- **Fúria:** monstros que revidam deixam a luta mais difícil. Se ficar pesado, o ajuste é o `Poise` em `OnHealthChanged` (3/4/5 golpes).
- **Névoa densa:** o horizonte distante agora é só silhueta; a miragem da Yggdrasil (a 15 km) continua visível porque fica além do corte da névoa.
- **Mundos antigos:** abertos em Jogar, deixam de abrir sala. Para jogar com amigos, é preciso criar um mundo em Multiplayer.
- **Conquistas:** são calculadas na hora, abrindo os saves dos heróis. Com muitos heróis a página pode levar um instante para abrir.
- **`VortexFrontEnd.cpp`:** já tinha 812 linhas de mudanças suas não commitadas; as minhas foram feitas por cima, sem desfazer nenhuma.

## Para você testar jogando

- **Combate:**
  - a marcação de foco e a opção de desligá-la;
  - o combo num goblin ou num lobo (devem revidar ou saltar);
  - o golpe forte no morcego;
  - os números.
- **Névoa:** de cima de um morro, olhar a borda; não deve dar para ver o chão e as árvores se formando.
- **Menu:**
  - Jogar → criar mundo → criar herói → jogar;
  - Multiplayer → Criar sala → abrir sala → amigo entra, cria o herói, marca pronto → "Pronto! Começar aventura";
  - Conquistas.

## Resultados

- `VortexEditor` compilou.
- **Suíte completa:** 27 de 27 testes passaram.
- **Menu e saves:** passaram `RunHeroSmoke`, `RunMenuFlowSmoke`, `RunSaveSmoke` e `RunMenuNetworkSmoke`.
  - `RunHeroSmoke` foi atualizado: 3 heróis num mundo de multiplayer, 1 só num mundo de um jogador e as duas listas separadas.
- **Sala de espera (`RunLobbySmoke`):**
  - com 2 convidados, **passou**: entram, criam o herói viajante, marcam pronto, o anfitrião começa e todos chegam ao mundo;
  - com 3 convidados, um não consegue criar o viajante, porque este PC já tem um viajante seu e o limite é 3 por computador. É uma limitação do teste neste PC, não do jogo. Nenhum herói seu foi apagado.
- **Golpe de perto:** o herói se vira e acerta o golem ao alcance da lâmina, sem andar sozinho (`RunFeatureShots`). Também passaram `RunSwordSmoke`, `RunCombatSmoke`, `RunWolfSmoke`, `RunPackSmoke`, `RunCreatureSmoke`, `RunCameraSmoke`, `RunWolfNetworkSmoke` e `RunClientInputSmoke`.
- **Passeio a galope (720p, Alto):** 137 FPS; pior quadro depois da carga, 44 ms.
- **Névoa:** densidades de 0,45 a 12 testadas. A névoa entra no primeiro quadro (`VORTEX_FOG` no log, 5 s antes da foto) e, com início em 60 m, fecha por volta de 70–80 m.
- **Ainda sem foto:** as páginas Multiplayer e Conquistas. Foram postas no teste de fotos do menu interno, mas o `RunFrontEndShot` não passa por elas; abra no jogo.
- **Visual:** o troféu é branco e chapado, enquanto os outros ícones do menu são voxels 3D. Funciona, mas destoa um pouco; dá para refazer no Blender no mesmo estilo.

## Morcego na altura certa (23/10, tarde)

**Pedido:** "o morcego inimigo está voando muito alto, difícil de acertar ou bugado; coloque numa altura boa".

**Causas:**
- **Altura:** rondando o herói, ele pairava a ~2,25 m do chão. O golpe de espada aceita ~2,2 m de diferença de altura, então ele ficava no limite e saía do alcance a cada descida do terreno.
- **Bug fora do vale:** a medida de chão que ele usava (`GroundAt`) só conhece as células do vale e, no continente, devolve "sem dado" (−300). O morcego então afundava até o chão e ficava raspando nele.

**O que mudou** (`AVortexMonster::Tick`, voo do morcego):
- **Brigando:** o centro dele fica na altura da cabeça do herói, ~1,4 m acima dos pés.
- **No ataque:** desce até o peito (~0,9 m).
- **Sozinho:** ~1,3 m acima do chão.
- **O chão** é o maior entre o das células do vale e o relevo do continente (`QuickGround`), então vale em todo o mapa.

**Teste:** `RunCreatureSmoke -Species Bat` ganhou a conferência "voando na altura certa" (entre 0,4 e 2 m acima dos pés do herói).
- **Resultado:** 138 cm. O morcego ataca, é vencido e dá XP. `PASS`.
- **Foto:** [o morcego na altura da cabeça](../Images/morcego/depois_altura.png).

**Para você testar jogando:** lutar com morcegos com espada e com arco, no vale e fora dele.
