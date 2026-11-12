# Continuar aqui (atualizado em 12/11/2026)

Onde o trabalho parou, para retomar numa conversa nova (ou numa conta nova). O passo a passo técnico do que está
abaixo fica em [02_Combate/PERSONAGEM_BASE_BLOCOS.md](02_Combate/PERSONAGEM_BASE_BLOCOS.md) e
[02_Combate/ANIMACOES_MIXAMO_EMOTES.md](02_Combate/ANIMACOES_MIXAMO_EMOTES.md).

## ONDE PARAMOS (12/11, manhã): ler primeiro

- **Tudo compilado e no Content, nada pendente no meio.** Nenhum processo da Unreal ou do Blender aberto.
- **Feito de 11/11 a 12/11, sem o usuário ter jogado ainda** (as seções abaixo, de cima para baixo):
  1. monstros menos colados e mais tipos por região (`Data/Population/spawn_tuning.json`);
  2. clima e estações (chuva, tempestade, neve, neblina) com a linha "ESTAÇÃO • TEMPO" embaixo do horário no minimapa;
  3. bichos e sons da cidade;
  4. bandidos no corpo de blocos (o normal, o capitão, o gordo, o pequeno e o cavaleiro renegado);
  5. quatro viajantes nas estradas;
  6. os bugs da rodada da noite (quinas do chão, grama, placas, brilho de metal, nível dos monstros nos portões,
     plantas nas saídas, volume inicial);
  7. FPS, casas fechadas (só as lojas abrem), ícones das lojas no mapa e as opções com o "Aplicar".
- **Instalador:** NÃO refeito, a pedido do usuário ("vamos arrumando umas coisas"). O de `Build/Installer` é das 19:52
  de 11/11 e não tem nada do que veio depois. Gerar com `Tools/PackageAlpha.ps1` só quando ele pedir. Nada enviado ao itch.
- **Próximo, quando o usuário pedir:**
  - monstros novos próprios de cada região (ele disse "vamos fazer depois");
  - o que ele achar jogando.
- **Não conferido:**
  - os sons da cidade e do clima (ninguém ouviu);
  - os viajantes andando;
  - a tempestade e a neblina (sem foto);
  - os mortos-vivos no campo de Morthúria.
- **Ideias ainda não pedidas:**
  - a chuva molhar o herói e as roupas;
  - os moradores procurarem abrigo na chuva;
  - neve no chão no inverno;
  - o ouriço e o guaxinim, que já estão importados.
- **Testes rápidos úteis:**
  - `Tools/RunWorldShots.ps1 -Only <prefixo> [-Extra '-VortexWeather=Rain']` (fotos);
  - `Tools/RunRoamBench.ps1` (FPS, ~3 min);
  - `Tools/RunBanditSmoke.ps1`;
  - `Tools/RunCreatureSmoke.ps1 -Species X`.

## Vídeo do jogo (12/11, pausado a pedido do usuário; compilado, ainda sem vídeo)

- O modo filme (`-VortexRoamBench -VortexFilm`, em `VortexRoamBench.cpp`) faz uma câmera passear pela vila em três
  tomadas: do alto à tarde, na rua no pôr do sol e na praça à noite com chuva. São 40 s, e cada quadro vira um PNG em
  `Saved/Film/frames`.
- `Tools/RecordFilm.ps1` roda o jogo escondido, com passo fixo de 30 FPS, sem usar a tela do PC, e junta os quadros em
  `Saved/Film/bruto.mp4`.
- A primeira tentativa, pela tela com o ffmpeg, pegou a tela do usuário em vez do jogo; o arquivo foi apagado. A
  segunda foi parada antes de salvar quadros.
- **Próximo:** rodar `RecordFilm.ps1`, conferir os quadros (a noite pode ficar escura demais; a rua pode atravessar
  casas) e editar uns 30 s com `Docs/Midia/Musicas/O_Despertar_de_Valdora.mp3`.

## Comandos de teste novos e bichos da vila de frente (12/11; compilado; comandos não testados no jogo)

- **Lista para o usuário:** [COMANDOS_DE_TESTE.md](COMANDOS_DE_TESTE.md). O chat abre com T.
- **Novos** (`VortexDevChat.cpp`): `/clima` (no PC de quem digita: `UVortexWeatherSubsystem::ForceWeather`/`ClearForcedWeather`, muda na hora), `/estacao`, `/dia N`, `/manha` `/meiodia` `/tarde` `/noite`, `/relogio N` (`SetDayTimeScale`), `/velocidade N` (`SetGlobalTimeDilation`), `/itens [palavra]` (procura no catálogo) e os nomes em português `/deus`, `/voar`, `/ir`, `/onde`, `/matar`, `/limpar`. `/ajuda` reescrito.
- **Bichos andando de costas:** a frente dos modelos do Cube World não é a mesma para todos (medido no Blender: gato, galinha, pintinho, cachorro e ouriço para um lado; porco, ovelha e guaxinim para o outro). `MeshYaw(Kind)` no `VortexTownLife.cpp`: −90 para os primeiros, +90 para os outros. Não conferido jogando.

## Monstros menos colados (12/11, manhã; compilado e passeio de FPS rodado; falta o usuário jogar)

Pedido do usuário: "sim os 2 (menos colados e mais tipos por região), mas os tipos dos outros vamos fazer depois".

- Os números saíram do código para `Data/Population/spawn_tuning.json` (ajustar não precisa compilar):
  - `group_chance` 0,55 -> **0,36**: cerca de 35% menos bandos por quadra de 72 m;
  - `pack_spread_cm` 300 -> **500 a 750 cm** entre o líder e os companheiros (de dia e de noite);
  - `bandit_camp_cm` 320 -> **450** em volta da fogueira; `night_chance` (bichos da noite) 0,30 -> **0,22**;
  - `regions`: a lista de espécies de cada região. Ganharam tipos que já existiam: Brasafenda e Raikor o goblin,
    Nivaluz o morcego e a gota, Aurivale o morcego, Verdalume o lobo, e **Morthúria os mortos-vivos e os esqueletos**.
- `RunRoamBench.ps1` rodou sem erro (64 FPS no editor). Morthúria não foi visitada no passeio.
- **Depois (pedido do usuário):** monstros novos, próprios de cada região.

## Clima, bichos e sons da cidade, bandidos novos e viajantes (12/11, madrugada; compilado, importado e fotografado; falta o usuário jogar; instalador NÃO refeito; NÃO enviado ao itch)

Pedido do usuário: "bichos na cidade e som de cada lugar, procure modelos na internet e pode usar; no mundo os viajantes com roupas únicas; crie os visuais dos bandidos (o pequeno, o gordo, os normais e os cavaleiros); faça o clima no jogo; embaixo do minimapa o horário e a estação; neve, chuva, tempestade".

- **Clima** (`UVortexWeatherSubsystem`, `Source/VortexWorld/.../VortexWeatherSubsystem.*`):
  - estação de 3 dias de jogo (Primavera, Verão, Outono, Inverno); o tempo sorteado a cada 6 horas de jogo pelo dia, pela janela e pela região da câmera (todas as máquinas veem o mesmo sem rede): Céu limpo, Nublado, Chuva, Tempestade, Neve, Nevasca e Neblina. Solcânion quase não chove, Brasafenda só fecha, Nivaluz neva, Lodonegro e Morthúria têm neblina; no inverno a chuva vira neve; o dia 1 é sempre limpo;
  - o céu fechado escurece o sol e o céu e tira cor (`UVortexDayNightSubsystem::Apply` lê `GetOvercast`); a neblina engrossa; relâmpagos com trovão na tempestade (`GetFlash` clareia o céu e a luz ambiente);
  - chuva e neve: dois cilindros em volta da câmera com o material `M_VortexPrecip` (texturas de `Tools/Weather/MakeWeatherTextures.py`), que somem debaixo de um teto (raio de 25 m para cima) e seguem a luz do dia;
  - sons: `amb_rain`, `amb_rain_storm`, `amb_wind_snow` e o trovão;
  - `-VortexWeather=Rain` (Storm, Snow, Blizzard, Fog, Cloudy, Clear) força um tempo; `-VortexNoWeather` desliga. `RunWorldShots.ps1` ganhou `-Extra`. Fotos com chuva e neve conferidas; tempestade e neblina não fotografadas.
- **HUD:** embaixo do "DIA 1 • 13:02", a linha "PRIMAVERA • CHUVA" (`VortexAimHUDCartoon.cpp`); a missão acompanhada desceu de 430 para 466.
- **Bichos na cidade** (`AVortexTownLife`, criado pelo `AVortexTown` com a lista "animals" da `vila.json`):
  - modelos do **Cube World Kit da Quaternius (CC0)**, baixados do poly.pizza para `modelando ainda/quaternius_cubos` (85 modelos; usamos galinha, pintinho, gato, cachorro, porco e ovelha; o ouriço e o guaxinim ficaram importados e sem uso). Os .glb vinham com uma esfera solta e o esqueleto em escala 100: `Tools/Blender/PrepTownAnimals.py` limpa e grava `Data/Town/Animals/*.glb`; `Tools/ImportTownLife.py` importa (o material precisa de `used_with_skeletal_mesh`, senão sai o cinza padrão);
  - 5 grupos de 4 galinhas (3 + pintinho) em terreiros de terra (sem grama), 4 gatos nos quintais, 3 cachorros nas ruas (só andam nas pedras), 3 ovelhas e 2 porcos num cercado a 24 m do estábulo (`BuildVila.py`); galinhas e gatos fogem do herói.
- **Sons** (`Tools/Sfx/MakeTownSounds.py` -> `Data/Sfx/Town`, importados pelo `ImportTownLife.py`):
  - bichos: gato, cachorro, ovelha (OpenGameArt, CC0, em `modelando ainda/sons_baixados/opengameart`), porco, galo e cavalo (SONNISS GDC 2024, da pasta do Godot), cacarejo sintetizado; o galo canta de manhã e o cachorro late mais à noite;
  - lugares: o fogo e a bigorna na forja (das 7 às 17 h), a conversa na taverna (mais forte no almoço e à noite), o sino da capela às 6, 12 e 18 h, o cavalo no estábulo e o burburinho do mercado de dia. **Não foram ouvidos** (as fotos não têm som).
- **Bandidos** (pedido "o pequeno, o gordo, os normais e os cavaleiros"): agora no corpo de blocos do herói (`VortexBlockBody::Apply`, que saiu do `VortexNpc.cpp` e ganhou a armadura opcional), com as roupas novas do `MakeBlockOutfit.py`: `bandido` (capuz preto e lenço vinho), `bandido_capitao` (casaco vinho comprido), `bandido_gordo` (o corpo 1,38x mais largo, avental de couro e bandana) e `bandido_pequeno` (0,84 do tamanho, capa e capuz); o `BanditShield` virou o **Cavaleiro Renegado**, de cota de malha (`A_malha_ferro`). Os golpes usam as animações do herói (`UVortexMonsterAnimInstance::Clip`: `MX_humano_Atk_Espada_*`, `Atk_Martelo_*`, `Arco_Release`). `RunBanditSmoke.ps1` passou inteiro; fotos `bandido_quatro.png` e `bandido_ataque.png`.
- **Viajantes** (`BuildVila.py`, na `npc_posts.json`): o mercador Osvaldo (roupa `viajante_mercador`, com mochila) e a peregrina Irene na estrada norte, o bardo Lírio (`viajante_bardo`) na leste e o camponês Tião (`viajante_campones`) na sul; saem do portão, vão até 200 m e voltam. **Não fotografados**; não têm falas próprias.
- **Falta:** o usuário jogar e ouvir; conferir os viajantes andando; a chuva molhar o herói (não liga); os moradores procurarem abrigo na chuva; neve no chão no inverno.

## Bugs vistos jogando, rodada da noite (11/11 para 12/11; compilado e fotografado; o instalador NÃO foi refeito, a pedido do usuário: o de Build/Installer é o das 19:52 de 11/11, sem estas correções; NÃO enviado ao itch)

- **Quinas dentro da muralha:** o anel nos cantos cortados era de placas giradas que se sobrepunham às da grade; agora são placas da grade em escadinha (`BuildVila.py`, até 3 m da diagonal), com o meio-fio nos degraus. Foto: `mundo_vila_quina.png`.
- **Grama atravessando:** o meio-fio e a estrada de fora entraram na máscara da grama; a máscara agora vale além da muralha (`GMask.Bounds` no `VortexTown.cpp`).
- **Placas das lojas:** o braço da placa pendurada entrava nas vigas das casas novas; nas casas do kit a tábua fica presa na parede em cima da porta, com 64 cm (`sign_flat` no `vila_models.json`). Foto: `mundo_vila_padaria.png`.
- **Brilho do metal mesmo na sombra:** no `M_ArmorBlock` o metal estava em **0,35** (o `ImportBlockArmor.py` dizia 0,12) e a rugosidade em 0,65; agora 0,05 e 0,9 (`Tools/ArmorMetalRough.py`, que mostra as constantes que achou). O molhado foi conferido: só o herói escreve os dados do molhado (30 e 31) e eles começam em 0. Falta o usuário ver no jogo.
- **Monstros fortes na saída:** `FVortexTerrainData::AreaLevel` media do centro antigo (0, 4) m e dava nível 1 só até 70 m; agora do centro da vila (18, 21) m, nível 1 até 220 m e +1 a cada 45 m.
- **Saída dos portões:** largo de terra batida (15 x 9 m, plano) e estrada de 6 m por 36 m em cada portão; "clareiras" na `vila.json` (até 48 m do portão, 30 m de largura) sem árvores, arbustos, pedras nem grama alta (`AVortexTown::Clears`, usada no `VortexTerrainStreamer.cpp`). Foto: `mundo_vila_portao_fora.png`.
- **Volume alto ao abrir:** uma vez por PC (`[Vortex.Audio] Defaults` no GameUserSettings.ini), o geral vai a no máximo 50% e a música a 60%; "Restaurar padrões" do áudio também.

## Melhorias de FPS, casas fechadas, ícones das lojas e opções novas (11/11, noite; compilado e importado; NÃO medido de novo, a pedido do usuário; instalador do Windows gerado às 19:52 de 11/11 em `Build/Installer/TheWorldGeneses_Alpha_1.0_Portal.exe`, 1.461 MB; NÃO enviado ao itch)

Pedidos do usuário: "fazer a lista toda" (da auditoria), "das casas pode deixar sem poder entrar em nenhuma, somente as que têm NPC, tipo as lojas", "ícones para elas no minimapa e mapa", e as opções: "organize melhor, separe e melhore os nomes e descrições, e faça realmente funcionar somente quando clicar Aplicar (às vezes consigo só mudar e sair e está aplicado, e trava um pouco); tem que ter o loading".

- **Qualidade geral** (`SetPreset`, `VortexGraphics.cpp`): Baixa e Média desligam a luz global e os reflexos do Lumen; Alta e Épica ligam. Na auditoria isso foi de 64 para 72 FPS na praça e de 63 para 91 fora da vila.
- **Modelo do DLSS** (opção nova, `DlssModel` no `VortexPlayerSettings.h`): Automático (o leve, preset E, nas RTX 20 e 30), Leve ou Nítido. O leve custa 0,8 ms em vez de 3,5 ms numa RTX 2060 SUPER. Falta ver se a imagem em movimento ficou aceitável.
- **Lumen mais barato fora da Épica:** a luz da cena atualiza na metade do ritmo (`r.LumenScene.DirectLighting.UpdateFactor` 64 e `Radiosity.UpdateFactor` 128): 0,7 ms de placa a menos. Os outros ajustes testados (só o campo global, sondas na metade) não renderam.
- **Vila:** o chão, o meio-fio, bancos, caixotes, floreiras e cercas saem da cena do Lumen e (menos o poste) não fazem sombra (`AVortexTown::Build`).
- **Moradores:** parados no posto, o movimento deixa de rodar a cada quadro (`AVortexNpc::Arrive`/`SetGoal`); a mais de 25 m da câmera não fazem sombra (`UVortexPopulationSubsystem::Update`, a cada 0,25 s).
- **HUD:** o minimapa e o mapa só varrem os personagens, não todos os atores, para achar as criaturas.
- **Casas fechadas** (`BuildCasasQuaternius.py`, `enter`): as 8 casas sem morador ficam com a porta do kit fechada (`Door_2_Flat`), sem móveis, escada, andar de cima nem luz de dentro; só a `vila_loja` (os 5 lojistas) continua de entrar, além da taverna, da capela e da forja (outros scripts). A porta fechada vai em `closed_door` no `vila_models.json`: o jogo não cria porta de abrir, mas o `BuildNpcNav.py` (onde os moradores dormem) e o caminho de pedra do `BuildVila.py` continuam usando.
- **Ícones no minimapa e no mapa** (`Tools/Town/MakeShopIcons.py` -> `Data/UI/HudCartoon/M_Loja_<SLUG>.png`, importados pelo `ImportHudCartoon.py`): o símbolo de cada placa num selo redondo. O HUD lê as construções com nome da `vila.json` (`TownPlaces` no `VortexAimHUDCartoon.cpp`). No mapa aparecem a partir do zoom 4 e com o nome a partir do 8; o ícone da cidade some de perto. Folha de prévia: `Docs/Images/icones_lojas.png`.
- **Opções** (`VortexGraphics.cpp`, `VortexOptionsPanel.cpp`):
  - o painel edita uma **cópia** (`Draft`) das opções; o jogo continua com as de verdade até o **Aplicar** (antes o mundo lia as opções ao vivo e a grama, a distância e a água mudavam na hora, com travada). Fechar sem aplicar descarta;
  - o Aplicar mostra "APLICANDO..." por cima do painel, espera 2 quadros (o aviso já está na tela quando o motor para) e só some quando o chão e a fila de carregamento terminam (`VortexGraphics::IsApplying`, no máximo 20 s); a tela de carregamento do HUD continua cobrindo o mundo;
  - abas novas: **Tela** (resolução, modo, brilho, campos de visão), **Qualidade** (recomendação, qualidade geral, distâncias, vegetação, água, neblina, detalhe e bordas), **Luz e efeitos** (Lumen, oclusão, sombras e efeitos de câmera), **Desempenho** (DLSS e modelo, resolução interna e dinâmica, limite de FPS e sincronia), Áudio, Controles e Interface; nomes e descrições reescritos;
  - o teste `RunApplySmoke.ps1` foi ajustado às abas novas e ao Aplicar com espera, mas **não foi rodado**.
- **Não feito:** as travadas de 150 a 400 ms ao nascer criaturas fora da vila (`Criaturas.Pensar` chegou a 329 ms) e juntar os materiais das casas num só.
- **Instalador:** `Tools/PackageAlpha.ps1` (Portal) em `Build/Installer`.

## Auditoria de FPS (11/11, noite; só medição, nada mudado no jogo)

Relatório em [FPS_AUDITORIA_2026-11-11.md](FPS_AUDITORIA_2026-11-11.md). Na praça (RTX 2060 SUPER, 1080p, Alta, DLSS Desempenho) o quadro leva 16,4 ms com três limites quase empatados: linha de desenho 16,4 ms, placa 14,0 ms e linha do jogo 13,8 ms. O que mais pesa: a luz global Lumen (5,1 ms de placa e 4,7 ms de linha de desenho), o modelo novo do DLSS (3,55 ms; o modelo E custa 0,79) e os moradores (4,4 ms da linha do jogo). A qualidade Baixa não muda o FPS porque não desliga o Lumen. Ordem sugerida: Baixa e Média sem Lumen, DLSS com o modelo leve, Lumen mais barato na Alta, moradores, HUD. Nada disso foi feito ainda.

## Vila maior, casas do kit da Quaternius e chão novo (11/11, tarde; compilado, importado e fotografado; falta o usuário jogar; NÃO enviado ao itch)

Pedidos do usuário: "faça as outras casas usando os modelos que tem", "muita casa colada, pode aumentar a vila e separe um pouquinho", "casas mais distantes da muralha", "o chão que combine com a vila, as texturas, os caminhos certinhos, os postes, a fonte".

- **Achado importante:** a face de fora da parede do kit é o **-y** da peça (a pedra); o +y é o reboco de dentro. A forja estava montada ao contrário (por isso "parecia papel"): as paredes foram viradas e as janelas viraram as do kit (moldura, vidro e venezianas). As vigas de madeira por fora saíram (a pedra já dá o relevo).
- **Casas** (`Tools/Blender/BuildCasasQuaternius.py`, com as funções comuns em `Tools/Blender/quaternius_kit.py`): `vila_casa_1` a `8` e a `vila_loja` refeitas com o kit, com os mesmos nomes. Cada uma é uma lista de paredes por lado numa grade de 2 m, o telhado do kit, a chaminé por dentro (lareira) até passar do telhado, móveis com colisão, porta de abrir, lampião e luz de dentro. Os sobrados (2, 4, 5, 6, 8 e a loja) têm escada com colisão em degraus, vão no piso de cima com guarda-corpo e quarto. A loja tem o balcão no meio (o lojista fica atrás, como antes) e toldo.
  - De 12 a 31 mil triângulos cada (as antigas, uns 8 mil). As peças mais pesadas são reduzidas no `DECIMATE` do `quaternius_kit.py` (o canto de pedra tinha 3.100 triângulos). **LOD não foi feito:** o `set_lods` devolve -1 no commandlet sem janela; se o FPS da vila cair, gerar os LODs com o editor aberto.
  - As variantes `_g` (sem janela lateral) não são mais usadas: as casas ficam a 2,5 m ou mais umas das outras.
- **Planta** (`Tools/Town/BuildVila.py`): `HALF` 62 -> 71 (a vila foi de 124 para 142 m; o centro e a fonte não mudaram), `STREETS` em ±27, `GAP` 2,5 m entre construções, `SETBACK` 1,5 m de recuo da rua e `BELT` 6 m de faixa verde com árvores entre a rua da muralha e as casas. São 40 casas e 5 lojas (eram 51 casas), 41 árvores (eram 7) e um caminho de pedra (`vila_rua_p`) da porta de cada casa até a rua. Os prédios de ofício mudaram de lugar junto com as ruas (a forja foi 3 m para oeste e 3 m para sul).
- **Chão e enfeites** (`Tools/Blender/BuildChaoQuaternius.py`, mesmos nomes de antes): `vila_rua_*` de pedra irregular, `vila_laje_*` de pedra cortada, `vila_borda_*` agora é um meio-fio, `vila_poste` de madeira com o lampião do kit (o vidro continua acendendo à noite), `vila_banco`, `vila_caixotes` e `vila_cerca` com peças do kit. A terra, a floreira, o poço, as barracas, a carroça, a muralha e os prédios grandes (taverna, prefeitura, capela, quartel, estábulo, carpintaria) continuam os antigos.
- **Fonte:** a pedra usa as texturas da vila (`AVortexFountain::Piece`); o bronze, a água e o cristal não mudaram.
- **Moradores:** `Tools/Town/BuildNpcNav.py` rodado (40 casas, 8 construções, nenhuma porta isolada).
- **Fotos:** `RunWorldShots.ps1 -Only vila` (`mundo_vila_alto`, `_praca`, `_rua`, `_rua_sul`, `_casas`) e `-Only forja`, em `Saved/Screenshots`. Os pontos estão em `VortexWorldShots.cpp`.
- **Para refazer tudo:** `blender -b --factory-startup -P Tools/Blender/BuildCasasQuaternius.py -- exportar` (idem `BuildChaoQuaternius.py` e `BuildForjaQuaternius.py`), `python Tools/Town/BuildVila.py`, `python Tools/Town/BuildNpcNav.py` e o `Tools/ImportVilaKit.py` no commandlet.
- **Backup de antes:** `Backups/vila_2026-11-11` (planta, modelos, FBX das casas antigas, postos e caminhos dos moradores).
- **FPS medido (11/11, editor, 1080p, `RunRoamBench.ps1 -ResX 1920 -ResY 1080`):**
  - parado na praça girando a câmera (`-Extra '-VortexRoamTown'`): 64,7 FPS de média, pior 1% em 48,2, um quadro só acima de 50 ms; a cena tem 1,36 milhão de triângulos e 1.794 desenhos. Não há número de antes neste modo;
  - passeio padrão (da cidade para o leste): 70,7 FPS de média (era 63,7) e 9 quadros acima de 50 ms (eram 24). O pior 1% depois de 8 s caiu de 18,5 para 13,1 por travadas de 150 a 400 ms entre 9 e 13 s de passeio, já fora da muralha: `Criaturas.Pensar` chegou a 329 ms e o Colosso nascendo a 91 ms. Não é a vila, mas o `Criaturas.Pensar` de 329 ms é novo e vale olhar.
  - Esses dois números são na qualidade salva do projeto (`Saved/Config/WindowsEditor/GameUserSettings.ini`): "Alto" (2 de 3) em quase tudo, distância de visão e terreno no máximo. **Com tudo no máximo** (`-Console 'sg.ShadowQuality 3,...'`, os 11 grupos em 3), parado na praça: 53,7 FPS de média, pior 1% em 37,7, placa de vídeo 17,5 ms (era 12).
  - Conclusão: as casas novas não derrubaram o FPS; os LODs não são urgentes.
- **Falta:** o usuário jogar (entrar nas casas, subir as escadas, falar com os lojistas atrás do balcão); medir no jogo empacotado (`-Packaged`); refazer com o kit os prédios grandes e a muralha, se aprovado.

## Bugs de 11/11 corrigidos (11/11, manhã; importado e fotografado; falta o usuário jogar; NÃO enviado ao itch)

- **Forja:** exportada do Blender (sem janela; 40.754 triângulos, 11 materiais com o vidro, 20 caixas), `BuildVila.py` e `ImportVilaKit.py` com `VORTEX_VILA_ONLY=vila_forja`. Fotos de `RunWorldShots.ps1 -Only forja` (`Saved/Screenshots/mundo_forja.png` e `mundo_forja_frente.png`): o Bram atrás da bigorna, a passagem livre até a porta, a porta no arco e a chaminé numa peça só (esticada, não ficou feia). O lampião não aparece no ângulo da foto: falta conferir jogando.
- **Metal das roupas:** `Tools/ArmorMetalRough.py` (commandlet) mudou no `M_ArmorBlock` a constante da rugosidade do metal de 0,36 para 0,65, sem refazer o material (o molhado e o fosco continuam). O `ImportBlockArmor.py` também já sai com 0,65.
- **Fada:** `escala` 2,0 no `fadas.json` (18 cm).

Os detalhes do que foi feito no script ficam abaixo.

## (Feito) bugs da forja que o usuário viu jogando (11/11, manhã)

Relatados com prints: o Bram martelando o ar; o lampião rente ao chão e virado para dentro, sem luz; sem porta e com coisas na frente da porta, impedindo a passagem; a chaminé e a brasa "cortadas"; paredes lisas e sem janela, "parece papel"; o reflexo do metal das roupas brilhando até na sombra; a fada pequena demais para ver.

- **Feito só no script** `Tools/Blender/BuildForjaQuaternius.py` (montado e conferido no Blender pelo MCP; **ainda não exportado nem importado**):
  - bigorna na frente do Bram em (-0,7; 4,1), porque ele fica em (-0,7; 3,4) olhando para a rua;
  - bancada na cerca da esquerda e caixote mais para fora, deixando livre a passagem da rua até a porta (x de 0,4 a 1,65);
  - porta de abrir no arco (`doors`: dobradiça em 0,4, largura 1,25, altura 2,2) e colisão da frente refeita no vão medido;
  - lampião na parede de fora, à direita da porta (`rot=180`, z = 1,2) e com luz (`lamps`);
  - fornalha com a brasa só na metade da frente e a chaminé numa peça só, de 1,08 a 7,2 m (sem a tampa no meio);
  - vidro da vila nas 3 janelas (`glass`) e vigas de madeira por fora nas emendas das paredes.
- **Para terminar:**
  1. `blender -b --factory-startup -P Tools/Blender/BuildForjaQuaternius.py -- exportar`, depois `python Tools/Town/BuildVila.py` e `Tools/ImportVilaKit.py` (commandlet), e `Tools/RunWorldShots.ps1 -Only forja`.
  2. Conferir na foto se a chaminé esticada (1,92x na altura) não ficou feia.
  3. Metal das roupas: no `M_ArmorBlock`, a rugosidade do metal é uma constante de 0,36 (o `B` do lerp ligado à rugosidade; `ImportBlockArmor.py`); subir para ~0,65. Não é o molhado (o `AddWetLook` só vale abaixo da linha da água).
  4. Fada: `escala` em `Data/Creatures/Fada/fadas.json` de 1,0 para ~2,0 (sem compilar).
- O Blender ficou aberto com o MCP ligado (aberto pelo PowerShell com `--python-expr`, que ativa o addon e chama `bpy.ops.blendermcp.start_server()`).

## Instalador do Windows de 11/11 (enviado ao itch)

- Versão `1.0.0-alpha+20261111.0133`, só o canal `windows-instalador` (o Linux continua na de 10/11). Leva tudo de 10 e 11/11: chão em pixel art, grama da Quaternius, rastros de vento, folhas caindo, `/spawn` em português, herói fosco, correções de FPS e travadas e a forja nova.
- Blender: o addon do MCP (`blender_mcp_addon`) foi ativado e salvo nas preferências do usuário; ao abrir o Blender, o servidor deve ligar sozinho.
- Comparação das texturas da forja: `Docs/Images/forja_textura_pixel_vs_original.png`.

## Forja de Bram com o kit da Quaternius (11/11; importada e fotografada; falta o usuário ver; enviada no instalador do Windows de 11/11)

- **Pacotes** (versões grátis, CC0): `modelando ainda/quaternius_vila` (Medieval Village MegaKit, 176 peças de encaixar numa grade de 2 m) e `modelando ainda/quaternius_objetos` (Fantasy Props MegaKit: bigorna, pedra de amolar, suporte de armas, bancada, barris...). Pro e Source são pagos e não foram comprados.
- **Forja:** `Tools/Blender/BuildForjaQuaternius.py`, com o mesmo desenho e o mesmo lugar da antiga.
  - Sala de pedra e reboco de 12 x 6 m, com porta em arco, janelas, telhado de telha e empenas.
  - Alpendre de tábua sobre pilares, com fornalha de tijolo, brasa e chaminé, bigorna no toco, pedra de amolar, suporte de armas, bancada, barris, balde, corrente, caixote e lampião.
  - Sai `Data/GodotTown/VilaKit/vila_forja.fbx` e a entrada `vila_forja` do `vila_models.json` (20 caixas de colisão, placa, lampião e chaminé). Depois: `Tools/Town/BuildVila.py` (só a forja e a fumaça mudaram) e `Tools/ImportVilaKit.py`.
  - Para ver no Blender sem janela, usei renders do Workbench; com o Blender aberto pelo MCP, dá para dar `exec` no script.
- **Texturas** (11/11, escolha do usuário: **a original do kit**): `Tools/Town/MakeQuaterniusTextures.py` leva as do kit para o jogo em 512 px (`T_Vila_Q_*`; com `-pixel`, faz a pixel art de 128 px e 14 tons). O `ImportVilaKit.py` põe o filtro suave nelas, e cada material do kit vira `MV_Q_<nome>`.
- **Pilares do alpendre** (11/11, o usuário viu que não encostavam no telhado): subiram de 2,55 para 2,90 m (`POST`); o telhado de tábua começa a 2,88 m em cima deles.
- **Blender e MCP:** se o app do Claude se atualizar, ele reinicia e fecha os programas que abriu, incluindo o Blender (11/11, à 01:46). Abrir o Blender pelo Explorer evita isso, mas aí o servidor do MCP não liga sozinho; o jeito que funciona é o `Start-Process` com `--python-expr` que ativa o addon e chama `bpy.ops.blendermcp.start_server()`. O script na pasta de início do Blender foi bloqueado pelo Claude (mudança permanente fora do projeto).
- **Cuidados** (já resolvidos no script):
  - o importador de glTF deixa as peças em quatérnio, e o giro era ignorado (`rotation_mode = "XYZ"`);
  - o kit guarda nas cores de vértice a máscara de desgaste do shader dele, e o telhado saía preto (as cores de vértice são apagadas antes de exportar).
- **A antiga** está em `Backups/forja_antiga_2026-11-11` e na função `forja` do `BuildVilaKit.py`; a linha `"vila_forja": forja` ficou comentada, para o kit antigo não sobrescrever a nova.
- **Fotos:** `Saved/Screenshots/mundo_forja.png` e `mundo_forja_frente.png` (`RunWorldShots.ps1 -Only forja`).
- **Próximo, se aprovado:** as outras casas com o mesmo kit e variações (padaria, alfaiataria, mercado).

## /spawn em português, FPS da grama e herói fosco (11/11; compilado; falta o usuário testar; NÃO enviado ao itch)

- **/spawn** (`VortexDevChat.cpp`): aceita o nome do bicho no jogo, sem acento, maiúscula nem espaço, e vale um pedaço do nome (`/spawn gota de brasa 5 3`, `/spawn lobo`, `/spawn azhur`) ou o id antigo (`SlimeFire`). Se o nome bate com mais de um bicho, a resposta lista as opções.
  - `/spawn lista` mostra todos. `/spawn fada [qtd]` chama fadas na frente do herói (`AVortexFairies::Summon`).
  - `/criar` faz o mesmo. Sem teste automático: falta usar no jogo.
- **FPS:** o passeio (`RunRoamBench`, 1080p) caiu de ~70 para ~50 FPS com a grama da Quaternius: a preparação do desenho subiu de 13,5 para 20 ms. A causa é que os tufos importados vêm com campo de distância, e com ~300 mil tufos ele pesa.
  - Desligado nos grupos de grama (`bAffectDistanceFieldLighting`/`bAffectDynamicIndirectLighting` no `BatchFor`), voltou a 14 ms de preparação e 11,5 ms de placa.
  - O que ainda derruba o pior 1% (5–6 FPS) são as travadas antigas ao nascer bicho (`Criaturas.Postos` até 55 ms, `Lava` até 89 ms, `Nascer.*`) e o começo do passeio.
- **Travadas (11/11):** a maior causa era carregar algo "na hora" (a textura da fada, a roupa e o cabelo de um morador, um ícone) com a fila de carregamento em segundo plano ainda cheia (1.200+ pacotes): a Unreal esvazia a fila inteira antes de continuar (`FlushAsyncLoading`) e trava de 100 a 500 ms.
  - A tela de carregamento (`AVortexAimHUD::DrawLoading`) agora também espera `GetNumAsyncPackages() == 0`, dentro do limite de 25 s que já existia. O passeio de teste espera o mesmo antes de medir (`VORTEX_ROAM esperou a carga`).
  - Passeio no editor, 1080p: média de 50,6 para 63,7 FPS; pior 1% depois de 8 s de 6 para 18,5 FPS; quadros acima de 50 ms de 131 para 24.
  - **No jogo empacotado** (`PackageAlpha.ps1 -Perf` + `RunRoamBench.ps1 -Packaged`, 1080p), depois de tudo: **74,9 FPS** de média (8 ms de jogo, 12,8 ms de preparação, 12 ms de placa); pior 1% depois de 8 s em 17,7 FPS (era 8,5 a 12); 13 quadros acima de 50 ms.
  - Também entrou: os moradores na pré-carga (cabelos, roupas, olhos `T_humano_*` e animações `_Npc_`), e a pré-carga passou a segurar todo asset que traz (antes, texturas e malhas fixas podiam ser descartadas).
  - O `AVortexFxWarmup` agora veste dois bandidos e desenha o brilho e a fumaça do saque (`M_DropRing`, `M_FxPuff`) na tela de carregamento: a roupa do primeiro bandido caiu de 370 para 1,8 ms, e o primeiro bicho vencido, que travava 1,8 s, ficou abaixo de 46 ms.
  - Fada: cada volta criava um material em cima do anterior (`MID_MID_MID...`): corrigido com `EmptyOverrideMaterials`. As 36 paletas são carregadas no começo.
  - As malhas feitas por código (folha, fita do vento, grama antiga, sementes, bichinhos) agora preenchem `UVChannelData`: sem isso, o pacote de medição travava num `ensure` do streaming de texturas.
  - **Ainda sobram:** um quadro de ~0,9 s logo no começo (4 s depois da tela de carregamento), a cratera da lava montada uma vez (70 ms) e o Colosso nascendo (116 ms).
- **Herói fosco** (`Tools/MatteHeroMaterials.py`): roupas, pele e cabelo brilhavam ao sol. Ficou especular 0,15 (era 0,5), rugosidade 0,95 (era 0,85) e o contorno de luz pela metade, em 11 materiais-mestre e 303 instâncias. O metal e o molhado continuam. Antes e depois: `Saved/Screenshots/heroi_humano.png`.

## Chão em pixel art e grama da Quaternius (10/11, madrugada; compilado e fotografado; falta o usuário jogar; NÃO enviado ao itch)

- **Pedido do usuário:** a grama do Stylized Nature MegaKit (Quaternius, CC0), o chão combinando com ela e "mais a cara do jogo" (o primeiro teste em 64 px ficou realista demais). Plano aceito: pixel art de 32 px, sombras nos blocos e grama nascendo da cor do chão.
- **Pacote:** `modelando ainda/quaternius_natureza` (zip Standard grátis e extraído em `pacote/`). Só a grama foi usada.
- **Grama:** `Tools/Blender/MakeGrassPack.py` põe os 4 tufos nos tamanhos do jogo (rasteira 18 cm, prado 30 e 38 cm, alta 56 cm) e `Tools/ImportGrassPack.py` cria `/Game/Vortex/Nature/Grass` (`SM_GrassPack_*` e `M_GrassPack`).
  - O `BuildGrassMeshes` (`VortexTerrainStreamer.cpp`) usa os 4 tufos nas 12 variantes quando eles existem. `-VortexOldGrass` volta à grama antiga.
  - Cor: a da célula do chão em 4 degraus pela altura (os mesmos tons do chão), palha só nas pontas das regiões secas e as manchas do `T_MacroNoise`, iguais às do chão. O vento e a pisada são os do `M_GrassWind`.
  - **Cuidado:** trocar o material pela lista `static_materials` no Python não grava (ficava a `WorldGridMaterial`, folhas listradas cor de palha); o script usa `set_material`.
- **Chão:** `Tools/MakeStylizedTerrainTiles.py` refaz os quadradinhos de grama (0, 1, 2, 3 e 18) e a beira (7) em 32 px, com 5 tons e tufinhos de luz e sombra. A média de cada um continua igual (`CellTint` = cor / média). O original fica em `Backups/terrain_tiles_32px_2026-11-10.png`.
  - Reimportar só a textura com `Tools/ReimportTerrainTiles.py`: o `ImportTerrainTiles.py` refaz o `M_TerrainTiles` e apagaria as manchas.
- **Sombras nos blocos** (`BuildMesh`, na cor dos vértices): cada canto do topo escurece com as vizinhas mais altas (até 56%), e as paredes escurecem descendo (até 40% a 3 blocos) e um pouco mais rente ao chão.
- **Fotos:** `Saved/Screenshots/grass_*.png` (`Tools/RunGrassShot.ps1`) e `estilo_valdora/aurivale/lodonegro.png`. FPS: a mesma cena deu 25 com a grama antiga e 27 com a nova (o PC estava mais lento nesta hora; nas fotos de grama a nova deu 85).
- **Rastros de vento no ar** (aprovados para fazer em 10/11): `AVortexWindTrails` (`Source/VortexWorld`), criado junto das fadas. São 10 fitas finas e brancas de 6 m (1 em cada 3 com laço), na frente da câmera, de 5 a 22 m e a 0,8–3,5 m do chão. Um pedaço claro corre pela fita, ela acende e apaga, e o vento a leva na mesma direção do vento da grama. De noite ficam a 20%.
  - O material `M_WindTrail` sai do `Tools/CreateWindTrailMaterial.py` e os ajustes ficam em `Data/World/vento.json`, sem compilar.
  - `-VortexNoWindTrails` desliga e `-VortexWindDebug` põe todas na frente, mais lentas, para as fotos `Saved/Screenshots/vento_*.png`.
- **Folhas caindo das árvores** (10/11; antes não existiam, só as sementes de dente-de-leão voavam): `AVortexFallingLeaves` (`Source/VortexWorld`), criado junto das fadas.
  - As folhas saem da copa das árvores a até 26 m da câmera (altura e largura vêm do modelo da árvore). A cor e a quantidade são do tipo da árvore (`Data/World/folhas.json`): outono e cerejeira soltam muito; pinheiros, mortas, cactos e gelo não soltam.
  - Na queda, o vento é o da grama (direção de 26,6° e a mesma rajada no ponto onde a folha está). A folha balança de lado como pêndulo: cai mais rápido no meio do arco e inclina nas pontas. No chão fica uns segundos e some; na água boia e vai com o vento.
  - O material `M_FallingLeaf` sai do `Tools/CreateFallingLeafMaterial.py`: pixel art 8x8 com 35% de luz própria (contra o céu a folha ficava preta). `-VortexNoFallingLeaves` desliga; `-VortexLeavesDebug` põe 3x mais e algumas na frente da câmera (fotos `Saved/Screenshots/folhas_*.png`).
  - **Cuidado:** um ISM cujas cópias nascem longe precisa do `UpdateBounds()` depois do `BatchUpdateInstancesTransforms`; sem ele, nenhuma folha era desenhada.
  - As sementes (`M_Seed`, `Tools/CreateForestLife.py`) agora voam na mesma direção do vento (antes +X do mundo).
- **Falta:** você ver e jogar (chão, grama, vento e folhas); o meio-dia em Valdora ficou verde-limão bem claro (luz, não textura); as casas e os objetos da Quaternius.

## Instalador novo no itch (10/11, à noite)

- Tudo o que está abaixo marcado "NÃO enviado ao itch" (mapa, atributos, IA dos monstros fases 1 a 3, espada inicial, lobo/goblin/cogumelo, fada) foi enviado nesta versão: `1.0.0-alpha+20261110.1952`, canais `windows-instalador` e `linux` (`Tools/PushItch.ps1`).
- Antes, os 4 testes que falhavam foram resolvidos (ver Raikor, espada e lobo abaixo). Ainda falta você jogar estas novidades.
- Próximo pedido do usuário: os pacotes da Quaternius (animações, móveis e objetos, casas da vila, grama e natureza). Ver a conversa de 10/11.

## Mapa, minimapa e bússola (10/11; compilado e fotografado; falta o usuário jogar; NÃO enviado ao itch)

- **Mapa grande (M)** (`VortexAimHUDCartoon.cpp`):
  - abre no zoom 10, centrado no herói (antes 3);
  - o detalhe por bloco aparece a partir do zoom 6 (antes 8), e a área de detalhe dobrou (`DetailCells` 2048 em `VortexAimHUD.h`, 1,5 km);
  - os nomes aparecem mais cedo: regiões até o zoom 7, lugares a partir do 5, missões a partir do 6, moradores a partir do 7 (o nome no 10).
- **Minimapa:** os alcances agora são 60, 100 e 160 m, começando em 100. Quem tinha o de 40 m salvo passa para 100.
- **Bússola (`DrawCompass`):**
  - mais larga;
  - riscos a cada 5°;
  - cada marca mostra a distância embaixo (em km quando passa de 1000 m);
  - os lugares descobertos a até 600 m entram fracos;
  - uma plaquinha no centro mostra o rumo ("L 90°") e a distância da marca na mira.
- Fotos: `Saved/Screenshots/mundo_mapa.png` (`Tools/RunWorldShots.ps1`) e `hud_dia.png` (`Tools/RunHudShot.ps1`).
- Raikor no `RunWorldShots` (resolvido em 10/11): o centro da região é um patamar pequeno numa encosta de ~47°, e o herói escorregava uns 26 m morro abaixo. Agora o teste pula pontos com encosta forte a até 8 m (`Steep` em `VortexWorldShots.cpp`; vale só para os pontos de pé, não para as câmeras altas). O log mostra também o terreno e onde o herói termina. Teste completo: 49 ok, PASS.

## Atributos em 8 grupos e save versão 5 (10/11; compilado; falta o usuário jogar; NÃO enviado ao itch)

- Os pontos agora vão em 8 grupos: Força, Magia, Vitalidade, Defesa, Resistência Mágica, Precisão, Agilidade e Vampirismo (`VortexRpg.h/.cpp`). Cada grupo sobe alguns dos 12 atributos oficiais.
- `UVortexPlayerSave::CurrentSchemaVersion` = 5. Ao jogar com um herói de save mais antigo, aparece "versão antiga… Apagar este herói?" (`VortexFrontEnd.cpp`).

## Só trajes prontos, cintura feminina e a Bunny (10/11; testado pelo usuário, "tá ótimo"; enviado ao itch, Windows e Linux)

- **Menu:** saíram a seção "Peças" e o busto (`VortexWearSlots` vazio, `DressBust` e `busto_tam` removidos). Os chapéus e acessórios também saíram. Ficam os 3+3 trajes iniciais, os 6 de elfo e 5 novos da folha H01 (masculino e feminino): `Traje_peregrino/escudeiro/mago/cacador/arauto_<m|f>` (`piece_designs.py`, `h01_tunic`).
- **Sem saia de tecido:** o manto da maga elfa é de blocos (`elf_robe`), e `"tecido"` saiu do `roupas.json`. Os NPCs não mudaram.
- **Corpo feminino:** `Feminino/Hero_humano` é cópia do masculino (mesmas juntas e pesos), com a cintura 14% mais fina (`Tools/Blender/female_waist.py`, `MakeFemaleBody.py`). As roupas das heroínas seguem a mesma curva. `HeroModelFor` usa essa malha no corpo feminino. Os NPCs continuam no corpo base.
- **Mão:** já é igual à do Hytale (100% no osso da mão, sem dedos); não foi travada como o pé por causa das armas.
- **Bunny** (NPC nova, a Jardineira da Fonte): roupa `bunny` no `MakeBlockOutfit.py`, `npc_extra.json`, posto no `BuildVila.py` (oeste da fonte), rotina `loja_tarde`, missões `v_bunny_canteiro` e `v_bunny_gotas` e o diálogo no `vortex_story.json`.
- **Cuidado:** o `Tools/ExtractNpcPosts.py` apagava o visual de blocos dos NPCs do Godot. Agora ele só acrescenta NPCs novos. O arquivo foi restaurado a partir do jogo instalado.
- **Sobras** do busto e do tecido (glb e o corpo antigo) estão em `modelando ainda/sobras_busto_tecido`.
- **Rodada 2 (10/11, depois do teste do usuário):**
  - Os 16 trajes (`Traje_*` e `Elfo_*`) foram redesenhados à mão no `MakeBlockOutfit.py` (`traje_*` e `elfo_*`, em OUTFITS; `SET = False`). As folhas H01 e E_sets ficaram só como guia.
  - Texturas: luz de cima para baixo, trama do pano, manchas do couro, reflexo do metal e `joint_shadow` em todas as roupas desenhadas. A máscara (azul = metal) e o relevo saem para todas; o `ImportBlockOutfits.py` usa o `M_ArmorBlock` quando os dois existem.
  - Cintura: só nos pontos da coluna e do quadril; antes entortava o ombro e as mangas.
  - Bola de fogo: a explosão (efeito do chefe, que repete) apaga em 1,8 s; os 3 efeitos entraram no `AVortexFxWarmup`.
  - Corrida: `Stride` 1,7 (a animação ~0,73x no mesmo chão).
  - `ImportBlockFemale.py`: apagar o `.uasset` antigo antes (ver o comentário no script).
  - A cintura quebrava quando o tronco girava (mirar, virar rápido): no `VortexAnimInstance.cpp`, depois do `ApplyPosture`, a torção da coluna se reparte em terços por Spine, Spine1 e Spine2. O peito termina no mesmo lugar. Para desfazer, basta tirar o bloco.
- **O usuário testa:** a heroína andando, correndo e com o arco no corpo novo, os trajes, a bola de fogo e a Bunny na praça. Depois vem o instalador novo.

## IA dos monstros, fase 1 (10/11; compilado, teste de criaturas passou; falta o usuário jogar; NÃO enviado ao itch)

- **Giro de 180° do cogumelo:** brigando, quem recua, ronda ou anda de lado continua olhando para o herói (o `Tick` do
  `VortexMonster.cpp` desliga o virar-para-onde-anda nesses casos). No começo do golpe só vira na hora se falta menos de 60°.
- **Fase 1:**
  - Ao ver o herói (sem ter apanhado), o monstro para, encara, mostra "!" e solta o grito da espécie (`alertaTempo`).
  - Sem herói, passeia em volta de casa com paradas.
  - Os golpes são sorteados entre os que cabem, com menos chance de repetir o último.
  - No bando, só 2 atacam de perto por vez (`UVortexGroupCoordinator`, grupo 1000000 + PackId); os outros cercam.
  - Cuspe e flecha miram adiantado; a flecha só sai com a linha livre (`CanSee`).
- **Ajustes:** `Data/Creatures/ia_monstros.json`, sem compilar. O lobo tem a IA dele (`VortexWolf.cpp`) e não mudou.
- **Fase 2 (10/11; compilado, teste de criaturas passou com 5 espécies; falta o usuário jogar):**
  - Desvio: com o herói carregando o especial, virado para o monstro e a até 5,2 m, o monstro pode pular de lado e para trás ("Desviou!"). A chance vem do `desvio_<Espécie>` no JSON; os pesados têm 0. Recarga de 3 s.
  - O guerreiro de escudo anda sempre virado para o herói.
  - Flanquear: no bando, se o herói está olhando para outro, este dá a volta e chega pelas costas (não vale para quem guarda distância).
  - Visão: o herói novo só é notado com a vista livre (`CanSee`) ou a menos de 5 m (`ouvir`).
  - Busca: quando perde o herói de vista, vai ao último lugar onde o viu e procura por 6 s ("?").
  - Goblin com metade da vida chama os goblins a até 30 m ("Chamou ajuda!"), uma vez por luta.
- **Fase 3 (10/11; compilado, teste completo de criaturas passou; falta o usuário jogar):** caminho em volta de casas,
  muros, pedras, água funda e barrancos (`Creatures/VortexPathGrid`).
  - No `MoveTo`, a linha reta é conferida a cada 0,4 s; bloqueada, roda a busca A* numa grade de 40 x 40 m (células de 1 m, testadas só onde a busca passa) e o monstro segue os pontos já encurtados.
  - Limites: uma busca a cada 0,75 s por monstro, 3 por quadro no total.
  - No teste, cada busca levou de 0,1 a 0,3 ms; o log mostra `VORTEX_PATH` nas 5 primeiras e um resumo a cada 50.
  - Não vale para os que voam, escalam ou estão na dungeon (sem terreno). `caminho: 0` no `ia_monstros.json` desliga.

## Especial da espada inicial (10/11; aplicado no Content; falta o usuário jogar; NÃO enviado ao itch)

- **Antes:** o Corte à Distância da Lâmina do Recruta (`DA_DefaultSword`) valia 16 vezes o golpe a cada 3,5 s (o padrão do C++), uns 60 de dano no nível 1. Os outros especiais comuns valem 2,6 a 3,4 vezes.
- **Agora:** 3 vezes o golpe, com 9 s de recarga (`Tools/FixStarterSwordSpecial.py`).
- `RunSwordSmoke` (resolvido em 10/11, passou): eram 3 conferências velhas. O combo tira 6 desde 07/11; o modelo da espada vem do `swords.json` (`W_espada_comum_1_blocos`), e não do `VisualMesh` do DataAsset; e com a defesa um golpe de 10 tira 6 (bloqueado, 3). As contas de bloqueio agora são relativas: bloqueado tira algo, e menos que sem bloqueio.

## Lobo, goblin e cogumelo com animações novas (10/11; compilado e importado; falta o usuário jogar; NÃO enviado ao itch)

- **Lobo Ametista:**
  - `Tools/Blender/RetargetWolf.py` passa as animações do lobo da Quaternius (CC0, `Data/Creatures/Quaternius_Animais/Wolf.gltf`): andar, galope, ataque, bote, morte, apanhar, parado e farejar. A **cauda não é copiada** (o usuário: "cuidado com a cauda"); ela segue o corpo com um balanço suave.
  - Especiais (`VortexWolf.cpp`): Mordida Dupla (`Combo`, a mordida que acerta às vezes emenda a segunda), Salto de Ametista (`Rear`: empina e salta onde o herói estava, com o chão marcado) e Explosão de Cristais (o alfa com menos da metade da vida: anel em volta dele, uma vez).
  - O ritmo do passo foi medido na pata (andar 110 e galope 265 cm/s).
  - Cópia do lobo de antes: `modelando ainda/sobras_lobo_antes_retarget` e `Lobo_Ametista_antes_retarget.blend`.
  - `RunWolfSmoke` e `RunPackSmoke` (resolvidos em 10/11, passaram): o `L_MovementProbe` fica nas coordenadas da vila, e desde 02/11 o lobo larga a caça dentro dela. Agora a regra da vila só vale se existir o `AVortexTown` no mundo (ele nasce com o terreno; `InTown` em `VortexWolf.cpp`). O teste do lobo sozinho espera 4 s, em vez de 3 (uivo de 1,8 s + bote).
- **Goblin:** animações do Mixamo dos heróis (`Tools/Blender/MixamoMonsterSource.py` + `MeshyBiped.py ... goblin+mixamo`): balança os braços, golpe de porrete. Cópia em `modelando ainda/sobras_goblin_antes_mixamo`.
- **Cogumelo Corrompido** (`Docs/Modelos_Meshy`, do tamanho do antigo, 1,3 m):
  - Animações do Mixamo (`mushroom+mixamo`, papéis à mão no `MeshyBiped.py`) e os especiais Raízes Corrompidas (a marca nasce embaixo do herói, `bAtTarget`) e Nuvem Corrompida (anel grande).
  - Rosto fechado, o original. A boca aberta com dentes (`AddMushroomJaw.py`) foi rejeitada pelo usuário.
  - `M_CogumeloCorrompido`: olhos (máscara `T_Cogumelo_Mascara`) que piscam e acendem antes do golpe, e manchas do chapéu pulsando.
  - O cogumelo antigo está em `modelando ainda/sobras_cogumelo_antigo`.
- **Importar de novo:** apagar antes a pasta em `Content/Vortex/Creatures/<Espécie>` (o import por cima não troca as malhas).

## A fada (10/11; compilado e fotografado; falta o usuário jogar; NÃO enviado ao itch)

- **O que é:** um enfeite como os pássaros (o usuário: "só voando, sem pegar; não dá para acertar, foge quando chegamos perto"). Fica em cada máquina e não passa pela rede. Tem ~9 cm, é de blocos e tem asas finas de libélula.
- **Modelo:** `Tools/Blender/MakeFairy.py`, no Blender aberto pelo MCP ou sem janela com `-- <0..5> jogo`. São 6 penteados (flor, tranças, orelha pontuda, coroa, pontas loiras, pontas lilás com borboleta) e 6 cores de cabelo (paletas `T_Fada_<v>_<cor>`).
  - Asas: textura lisa (`T_FadaAsa`).
  - Rosto: placa com 4 quadros (`T_FadaRosto`: normal, piscando, feliz, assustada), como o do herói.
  - Paleta sempre 8 x 4.
  - Cada face de bloco tem um quadradinho de UV, não um ponto: com área zero a Unreal escurecia as faces de lado.
- **Unreal:** `Tools/ImportFairy.py` cria `/Game/Vortex/Creatures/Fada` com `M_FadaCorpo`, `M_FadaAsa` (translúcida, dos dois lados) e `M_FadaRosto` (parâmetro `Quadro`). Antes de reimportar, apagar `SK_Fada_*.uasset` (e `M_Fada*` se mudar o material).
- **Jogo:** `AVortexFairies` (`Source/VortexWorld`), criado junto da `AVortexWildlife`.
  - Passeiam em volta de um ponto e param no ar; às vezes sorriem e piscam sozinhas.
  - Quando alguém chega a 4,5 m, fogem para cima, assustadas, e voltam depois em outro lugar.
  - Asas batendo por código, com o par de baixo atrasado. Sem sombra (com 9 cm a própria sombra a deixava preta).
  - Ajustes em `Data/Creatures/Fada/fadas.json`, sem compilar. `-VortexNoFairies` desliga.
- **Fotos:** `Tools/RunHeroShots.ps1 -Extra -VortexFairyShot -Map /Game/Vortex/Maps/L_TerrainProto` (no Git Bash, `MSYS_NO_PATHCONV=1`). Na primeira rodada depois de criar um material, os shaders ainda compilam e a foto sai escura; rodar de novo.

## Próximo: as raças que faltam no corpo de blocos (10/11)

Abertas no menu: humano e elfo (`IsRaceOpen` no `VortexFrontEnd.cpp`). Faltam anão, bárbaro, diabo e morto-vivo. O
**espírito fica para depois de todas elas** (o usuário, 10/11).

## Espírito (decidido em 09/11; adiado para depois das outras raças)

- **Espírito (raça jogável):** corpo do humano, translúcido, flutua. Depois da raça, escolhe o elemento, definitivo: fogo, água, gelo, natureza, raio, terra, vento, sombra ou sagrado (só esses 9). A cor do corpo vem do elemento. Restrição só em poderes e magias, e armas de outro elemento proibidas; armas sem elemento continuam livres. Falta contar as armas por elemento.
- **Fada (criatura passiva, não raça):** bem pequena (8 a 10 cm), modelo próprio detalhado, voa, pousa, foge; captura com pote e dá pó de fada.

## Chapéus e acessórios novos (09/11; compilado; falta o usuário olhar; NÃO enviado ao itch)

- **Chapéus e rosto/pescoço:** saem das folhas (`voxel_hat`, `voxel_face`, `voxel_read` em `piece_designs.py`), em fatias com a escala da cabeça do boneco. A janela do rosto fica livre. Cinto com bolsas sai da folha, mas descido para o quadril (`voxel_waist`); bolsas com alça à mão e pano no quadril; cinto de ervas e faixa com livro são à mão.
- **Cabelo em 3 modos** (`hair` em `pecas.json`: `tudo`, `franja`, `mostra`; `franjaZ`, corte em `M_BlockHair` por `ClipZ`, `Tools/PatchHairClip.py`, rodar uma vez depois de `ImportBlockHair.py`). Orelhas ficam à mostra com chapéu.
- **Acessórios em 3 espaços:** `peca_acessorio` (rosto e pescoço), `peca_cintura`, `peca_bracos`. Pano real em véus, pingentes, bolsas e capas: agora qualquer peça com `tecido` veste a malha `_tecido` (antes só a calça vestia, as capas não apareciam).
- **Decisão do usuário:** não haverá tela de troca de roupa. **A decidir:** deixar roupas e armaduras mais nítidas (menos tons e contorno), porque a média da pintura deixou tudo com cara de desfocado. O instalador vem depois disso.

## Multiplayer e salvamento auditados (09/11; compilado e testado; NÃO enviado ao itch)

- **Ficha e diário:** o servidor aplica só uma vez por herói (`bServerLoaded`), o dono só grava depois dessa confirmação, grava ao sair (`EndPlay`) e tenta de novo se falhar, avisando na tela.
- **Convidados:** o anfitrião guarda ficha e diário em `UVortexWorldSave::GuestHeroes` (conta da Epic + herói, ou `LAN_` + herói; na primeira carga, no autosave e no `Logout`) e essa cópia vence a do PC. Nos mundos criados a partir de 09/11 (`bGuestCopies`), herói desconhecido entra como novo (`DecideGuestSheet`). Limites: mundos antigos ainda confiam no PC na primeira entrada; na rede local a identidade é só o id do herói, que o cliente informa.
- **Queda de 14 min (relato do usuário, o anfitrião; sem causa):** o erro vermelho `SE_ECONNREFUSED` só diz que a conexão já estava fechada. A foto (08/11, 22:00) é anterior ao instalador das 22:17, que traz o `OnNetworkFailure` que não derruba o anfitrião quando um convidado cai. Não bate com o vencimento do login. Falta o `VortexRede.log` do anfitrião numa queda nova.
- **Diário de rede (`VortexRede.log`, também no Shipping):** motivo do fechamento da Epic (`EPIC P2P FECHOU`), interrupção, conexão direta ou por relay, `FALHA DE REDE` e `FALHA DE VIAGEM` sem filtro, `ERRO FATAL`, `AVISO DO MOTOR`, `TRAVADA` acima de 3 s com memória, início e fim da sessão. Acima de 2 MB vira `VortexRede.anterior.log`.
- **"Pure virtual function being called" (09/11):** reproduzido ao fechar o jogo (2 de 2 no `RunOnlineLobbySmoke`): os `static` de `UVortexOnline::Deinitialize` destruíam o P2P da Epic depois dos módulos dela. Agora ficam sem destruir, e o teste seguinte não repetiu o erro (uma rodada).
- **Outros:** pegar item mede até o próprio item; a raça só é aceita na primeira vez; save decodificado uma vez.
- **Epic:** renova o login ao vencer (`AddNotifyAuthExpiration`), fecha sala anterior ou com falha, repetição da busca no relógio do motor. `ProtocolVersion` foi para 2: versões antigas não se encontram com a nova.
- **Passaram:** `RunSaveSmoke`, `RunSocialSmoke`, `RunFrameworkNetworkSmoke`, `RunOnlineProbe`, `RunOnlineLobbySmoke`. `RunSaveSmoke` também confere a decisão da cópia (`VORTEX_GUEST_COPY`) e `RunOnlineProbe` refaz o login (`VORTEX_ONLINE_RENEW`). **Sem teste:** o aviso real de vencimento da Epic, a falha de gravação em disco e dois PCs reais.

## Elfo e saias de tecido real (09/11; compilado e fotografado; falta o usuário jogar; NÃO enviado ao itch)

- **Elfo:** usa o corpo, as animações e as roupas do humano (`HeroModelFor`), liberado no menu (`IsRaceOpen`). Na criação: Pele (clara, dark elfo), Orelhas (curta, longa, para trás, caída) e olhos por pele. No look: `elfo_pele`, `elfo_orelha` e `peca_orelha` (a malha, preenchida no `Normalized`).
- **Gerar:** `python Tools/MakeElfSkins.py`, `Tools/MakeBlockFace.py humano --estilo elfo_*|dark_*` e `bash Tools/Blender/MakeAllElf.sh` (usa as folhas `E_*.png`). Trajes em `Data/Looks/roupas.json` (`raca`), peças em `pecas.json` (`race`).
- **Tecido:** a saia é um tubo sem emenda em malha à parte (`<id>_tecido`), simulado com Chaos Cloth. O Python da Unreal não cria tecido: depois de importar, rodar `-run=VortexMakeCloth` (`VortexMakeClothCommandlet.cpp`: prende a cintura, solta a barra, cria as cápsulas das pernas). Importar de novo descarta o tecido.
- **Importar:** `ImportBlockFaces.py`, `ImportBlockOutfits.py`, `VORTEX_PECAS=1` + `ImportBlockArmor.py`, depois o comando acima.
- **Gerar tudo:** `bash Tools/Blender/MakeAllClothes.sh` (peças, trajes élficos, armaduras, heróis e NPCs; 250 itens). Depois: os 3 importadores, `VORTEX_PECAS=1` no das peças e por fim `-run=VortexMakeCloth`.
- **Defeito das "linhas puxadas" (09/11):** as faces viradas por `normal_flip` trocavam a ordem dos cantos e embaralhavam a UV; agora a UV vai por vértice (`MakeBlockOutfit.py`). Nas folhas élficas, a medida usa só o que é roupa com certeza (`strict`).
- **Armaduras (09/11):** material e relevo do elmo, ombreiras e joelheiras por conjunto (`MATERIAL`, `helm_relief`, `limb_relief` e `relief` em `armor_designs.py`), sombra nos encaixes (`joint_shadow`), mapa de relevo `_normal`, luz nas bordas e metal no canal azul da máscara (`M_ArmorBlock`, `ImportBlockArmor.py`). Tecido real: tabardo da guarda real, 6 capas e saiote.
- **Conferido:** `Tools/RunHeroShots.ps1` e `Tools/RunElfChecks.ps1` passaram; fotos em `Docs/Midia/Roupas/elfo_unreal.png`, `elfo_menu.png` e `armaduras_unreal.png`. Menu do elfo certo em pele, orelha e olhos. 9 personagens: 115 fps com e sem vestido (offscreen). **Não conferido:** efeito visível do vento (o teste foi inconclusivo), multiplayer, cada uma das 250 peças. Os trajes élficos não têm mapa de relevo (usam `M_HeroTextured`).

## Roupas separadas no jogo (09/11; compilado; falta o usuário jogar; NÃO enviado ao itch)

- 6 espaços no criador (painel Aparência > Peças, com setas): camisa, parte de cima, calça, calçado, chapéu, acessório.
- A escolha fica em `Look.Pieces["peca_<espaço>"]`, o mapa que já é salvo e vai pela rede: sem mudar o `VortexLook.h`;
  heróis antigos seguem iguais. Lista em `Data/Looks/pecas.json`, código em `Framework/VortexPieces.h`, `VortexLook.cpp`
  e `VortexCharacter.cpp` (`ApplyWearParts`). A rede agora aceita até 32 entradas (antes 16: derrubaria o jogador).
- Importar de novo: `VORTEX_PECAS=1` + `Tools/ImportBlockArmor.py`, com o `-script=` em barras normais (a `\4` da pasta quebra).
- Regras: a peça esconde a região do traje (camisa=tronco, calça=pernas, calçado=pés, chapéu=cabeça); a armadura vestida
  esconde a peça do mesmo lugar; chapéu com `hidesHair` esconde o cabelo; chapéu e acessório somem em primeira pessoa.
- Falta: não há tela para trocar a roupa de um herói já criado; o humano é a única raça com peças; chapéus em degraus.

## Rede, menu, arena e roupas (08/11, noite; local, NÃO enviado ao itch a pedido do usuário)

- **Rede:** convidado não fica mais insistindo em voltar a uma sala encerrada (`ClearHostRetry`, `bRetryToWorld`) e a seleção de herói zera ao chegar numa sala. Queda de um convidado não derruba o anfitrião (`OnNetworkFailure`). O erro "Pure virtual function being called" segue sem causa confirmada: precisa do `CrashContext.runtime-xml` do PC que caiu.
- **Menu:** Esc volta a página (`PageBack`, `OnPreviewKeyDown`); botão "Colar código" na tela de entrar.
- **Arco:** alvo atrás da arma usa a direção da mira, não a do corpo (`LaunchProjectile`).
- **Nascimento e arena:** heróis nascem espalhados; dentro do Estádio ninguém toma dano (`CanHarm`); o renascer na fonte (`VortexHeroSheet.cpp:1312`) não foi espalhado. Mais moradores perto da fonte (`VortexNpcRoutine.cpp`).
- **Roupas:** braço e perna em anéis, para dobrar sem mostrar pele (`MakeBlockOutfit.py`); as 22 roupas foram refeitas e importadas (backup: `Backups/roupas_2026-11-08`). As 84 armaduras usam o mesmo gerador e ainda não foram refeitas.
- **Roupas novas:** prompts em [05_Referencias/PROMPTS_ROUPAS.md](05_Referencias/PROMPTS_ROUPAS.md). As folhas R01 a R06 ficaram modernas para o herói; a versão atual é por peças (camisa, parte de cima, calça, calçado). O jogo ainda só troca a roupa inteira: o sistema de peças depende do OK do usuário.
- **Pé rígido (09/11; compilado, falta o usuário jogar):** `FVortexAnimInstanceProxy::Evaluate` mantém `LeftFoot`, `RightFoot` e `ToeBase` na pose de descanso; o joelho continua dobrando. Para desfazer, basta tirar esse bloco.
- **Peças separadas, teste (08/11):** `Tools/Blender/piece_designs.py` (ids `P_<nome>`, rodados pelo `MakeBlockOutfit.py`) gera em `Data/Heroes/humano_Mixamo/Pecas`. As 80 estão geradas (09/11), com o catálogo em `Data/Looks/pecas.json` (espaço, nome, público e `hidesHair`, regravado por `python Tools/Blender/piece_designs.py`) e fotos em `Docs/Midia/Roupas/catalogo_*.png`. Roupas e calçados usam a pintura das folhas (`modelando ainda/Futuras modelagens/Roupas`, `piece_ref_setup`/`piece_sample`); chapéus e acessórios são pintados à mão. A pintar melhor: chapéus (forma em degraus), blusa azul bordada (tom pálido) e punhos de algumas camisas. Ainda não importadas na Unreal: falta o sistema de 6 espaços (camisa, cima, calça, calçado, chapéu, acessório), o criador, o salvamento e o multiplayer. Pedido do usuário: chapéu que precisar esconde o cabelo.

## Slime refeito (08/11; compilado, com fotos; falta o usuário jogar)

Gota redonda e macia; o rosto (olhos, boca, língua, bochechas, sobrancelha brava) é desenhado na própria gelatina pelo UV da malha
(`FACE` no `Tools/ImportSlime.py`, dados primitivos 8 e 11-14), então amassa junto e nunca descola. Antes eram peças separadas (o bug).
Molas mais moles e base que se espalha no chão em `VortexSlimeLook.cpp`. O clarão de golpe não vale para o slime (não acompanha a
deformação). Foto de frente: `Tools/RunCreatureSmoke.ps1 -Species Slime -SlimeFace`. Backup: `Backups/slime_2026-11-08`.

## Moradores, portas e golpes dos monstros (08/11; compilado; falta o usuário jogar)

- **Moradores:** gestos variados ao ver o herói e conversas em balões entre eles (`gestos` e `conversas` no `Data/Population/npc_schedule.json`,
  sem compilar; `TickChat` no `VortexNpc.cpp`, o balão no `DrawNpcTags` da HUD). Quem não tinha fala respondia "..."; agora usa as da hora. Preso: desiste em ~5 s, não 12.
- **Portas:** mais grossas; a animação não pulava mais para o fim (a vila tica a cada 0,5 s parada: `ToggleDoor` volta ao tick por quadro).
- **Inventário em primeira pessoa:** a vitrine mostra o corpo inteiro (`VortexHeroWindow::IsOpen` no `SetViewModelVisible`).
- **Golpes dos monstros:** `AVortexMonsterFx` ganhou funil animado, círculo no chão e nuvens macias (`Tools/ImportMonsterFx.py`). O tornado do morcego usa o alcance real, nasce do terreno e fere em 3 toques depois do impacto.

## Ícones e modelos dos itens (08/11; falta o usuário jogar)

O prompt único de 185 ícones (24 folhas, I01 a I24) está em [04_Interface/PROMPT_ICONES_ITENS.md](04_Interface/PROMPT_ICONES_ITENS.md).
- **Folhas:** em `Data/UI/SkillIcons/Folhas`, cortadas (`python Tools/SliceIconSheet.py I01 ...`) e importadas (`Tools/ImportItemIcons.py`).
  A I15 chegou em partes: `I15` tem as 6 peças do Dragão e do Grifo; `Q01` tem as Perneiras de Quitina e as Botas de Garra. Todas importadas.
  Rodar `Tools/BuildItemIconPrompt.py` restaura a I15 original no `folhas.json`. Os ícones antigos estão em `Backups/icones_2026-11-08`.
- **Itens no chão:** os 31 materiais, poções e amuletos são o desenho do ícone com volume, em blocos (decisão do usuário, depois de
  ver um objeto modelado do zero): `Tools/Weapons/CutDropItems.py` (tira o fundo) → `Tools/Blender/MakeDropItems.py` →
  `Tools/ImportBlockSword.py` com `VORTEX_ONLY=item`. O `AVortexItemDrop` carrega `W_item_<id>_blocos`.
- **Efeito do drop:** coluna de luz, círculo pixelado animado no chão e cubinhos subindo, na cor da raridade e mais fortes nas
  altas (`Tools/ImportDropFx.py` cria `M_DropBeam`, `M_DropRing` e `M_DropMote`; a força de cada um fica no `BuildLook`).
  Foto: `Tools/RunWeaponCloseUp.ps1 -Types arco_de_betula -BowFirstPerson` (`perto_arco_de_betula_1p_itens.png`).

A flecha do Arco de Bétula é `W_flecha_betula_blocos` (`Tools/Blender/MakeBetulaArrow.py`), usada por todo arco sem flecha própria.

## Arco na primeira pessoa (08/11; compilado, com fotos; falta o usuário jogar)

Pedido do usuário: "fazer igual o minecraft". Na primeira pessoa aparece só o arco, sem braços, preso à câmera (`ViewRight`
filho da `FollowCamera`; a pose fica em `UVortexWeaponParts::TickBowString`, valores `At` e `Want`): embaixo à direita
descansando; puxando, em diagonal, com a flecha apontando para a mira e tremendo de leve na carga cheia. Os braços do corpo
somem só para quem joga (`bHideArms`). Fotos: `Tools/RunWeaponCloseUp.ps1 -Types arco_de_betula -BowFirstPerson`.

## Cotovelos e joelhos do herói (03/11; o usuário confirmou no jogo em 08/11)

Braço e perna são uma peça só, com dobra macia no cotovelo e no joelho (`LEG_SMOOTH` no `RigBlockCharacter.py`). As peças
de roupa dos membros copiam os pesos do vértice mais próximo do corpo (`MakeBlockOutfit.py`). A versão rígida está em
`Backups/joelho_rigido_2026-11-03`. O corpo feminino usa a mesma malha (desde 05/11), então não precisa de importação à parte.

## itch.io (02/11)

10/11 (pedido do usuário): instaladores novos enviados, com os trajes redesenhados, a cintura, a Bunny, a bola de fogo e a corrida.
Windows `..._Portal.exe` 1.433 MB (patch de 166 MB, versão 1.0.0-alpha+20261109.2013). Linux `..._Linux.run` 1.276 MB
(enviado inteiro, 1,24 GB). O Linux pegou um erro que só o clang dá (laço `const TPair<...>&` sobre `->Values` no
`VortexLook.cpp`: usar `const auto&`).

Instaladores novos de 08/11, à noite: `BuildInstallerTheWorldGeneses_Alpha_1.0_Portal.exe` (1,39 GB) e `..._Linux.run` (1,23 GB), com o slime, os moradores, os ícones e o arco novos; sem o Arsenal
(só no editor). Enviado ao itch em 08/11 (canal windows-instalador, versão 1.0.0-alpha+20261108.1350; patch de 175 MB).
Os dois canais foram enviados ao itch em 08/11 (versão 1.0.0-alpha+20261108.2126).

O jogo está em https://eros-nicolino.itch.io/the-world-geneses (página restrita). Para mandar uma versão nova, depois
do `PackageAlpha.ps1`:
`powershell -ExecutionPolicy Bypass -File Tools/PushItch.ps1 -Target eros-nicolino/the-world-geneses`.

Ele envia só os executáveis (03/11, pedido do usuário: nada de .zip na página):
- `windows-instalador`: o `.exe` Portal;
- `linux`: o `.run` do Linux.

`-Folder` também manda a pasta do jogo (canal `windows`, que aparece como `.zip`). O butler não apaga canais: o
`windows` antigo sai à mão no painel do itch (Edit game → Uploads → apagar o arquivo do canal `windows`).

O butler fica em `..\butler` e já está logado.

## Travadas, tela de carregamento e instalador (08/11, tarde)

- Desempenho e a tela de carregamento nova (as pinturas da abertura; cobre na hora ao entrar, ao sair do Despertar e
  em saltos grandes): [03_Mundo/DESEMPENHO_TRAVADAS_CARREGAMENTO.md](03_Mundo/DESEMPENHO_TRAVADAS_CARREGAMENTO.md).
- O Arsenal só no editor (`#if WITH_EDITOR` no menu e no `OpenArsenal`).
- Instalador do Windows refeito e no itch; o PDF [O_QUE_FALTA.pdf](O_QUE_FALTA.pdf) atualizado para 08/11.
- Falta: medir o jogo empacotado (`Tools/PackageAlpha.ps1 -Perf`, depois `Tools/RunRoamBench.ps1 -Packaged`), só com o
  usuário fora do jogo (o empacotamento junto com o jogo aberto deu crash de memória).

## Moradores vivos (08/11; compilado, testado com fotos no `RunNpcShots`; falta o usuário jogar)

Rotina de dia e noite (`Data/Population/npc_schedule.json`), caminhos pela vila (`Tools/Town/BuildNpcNav.py`), 11
animações de ofício (Blender, `MakePowerAnims.py --npc`), reação ao herói (olhar, parar o serviço, cumprimentar), falas
por hora, 16 passantes (10 adultos e 6 crianças) e voz no diálogo (`Tools/MakeNpcVoices.py`). Tudo em
[03_Mundo/NPC_ROTINA.md](03_Mundo/NPC_ROTINA.md). À noite as lojas fecham (o lojista dorme). `-VortexNoRoutines` desliga.

## Relevo por região (08/11, madrugada; só dados, com fotos; falta o usuário jogar)

Cada região com a sua cara (mesas em camadas em Solcânion, picos em Raikor, basalto em degraus em Brasafenda, agulhas
em Morthúria...), sulcos de erosão descendo das montanhas, rios serpenteando e vales de largura variável. Tudo no
`Tools/World/BakeWorld.py` (que no fim carimba os lugares marcantes de novo). O mapa de antes está em
`Saved/WorldBake/antes_relevo_0711/` (para voltar, é só copiar de volta para `Data/GodotWorld`). Detalhes e fotos em
[03_Mundo/RELEVO_POR_REGIAO.md](03_Mundo/RELEVO_POR_REGIAO.md). As pontes e as cabanas mudaram de lugar junto com os rios.

## Lugares marcantes e raízes (07/11, noite; compilado, com fotos; falta o usuário jogar)

Pedido: "faça uns 6 espalhados no inicial bem grandes e marcantes e lindos" e as raízes da Árvore do Mundo "espalhadas
pelo mapa todo". Tudo em [03_Mundo/LUGARES_MARCANTES.md](03_Mundo/LUGARES_MARCANTES.md), fotos em
`Docs/Images/lugares_marcantes.png` e `raizes_arvore_do_mundo.png`.

- 6 lugares a 0,8-1,2 km da cidade, com relevo carimbado no mapa (`Tools/World/StampMarvels.py`), caminhos de terra
  até eles e peças de blocos (`Tools/Blender/MakeMarvels.py` → `Tools/ImportMarvels.py`), montados pelo `AVortexLandmarks`.
- 50 raízes: 14 sobre rios estreitos (com rampas invisíveis para andar por cima) e 36 saindo do chão.
- Paredões altos do terreno com rocha em faixas (antes a textura esticava).
- Fotos: `Tools/RunWorldShots.ps1 -Only lugar_`.
- Conferido depois: atravessar andando a Ponte de Raiz e uma raiz de rio (`RunWorldShots.ps1 -Only lugar_andar`), a
  cachoeira (achando a borda de verdade do paredão) e os veios de seiva (laranja, sem estourar no branco).
- Falta: o usuário jogar (os caminhos, as subidas) e ver de noite.

## Rodada de 07/11, de manhã (o 3º teste do usuário; a conferir nas fotos e jogando)

O usuário mandou fotos com bugs e pediu: interiores melhores (com a prancha `modelando ainda/Futuras modelagens/Cidade/
Casas e edifícios/Estudos de interiores residenciais.jpg`) e texturas novas para cada móvel; escadas inúteis ou saindo
para fora; quinas dos telhados; a porta da forja; cipós flutuando; cadeiras e postes jogados; a emenda pedra/grama e os
quadrados do chão; o tutorial (o F não defende; faltavam primeira pessoa, cavalo etc.); a HUD e os menus no estilo das
imagens das habilidades ("bonito, como os jogos fazem"); portas de abrir com tecla; placas com fonte medieval e símbolo.

- **FPS:** o log da sessão dele (Saved/Logs/Vortex.log) deu ~12 FPS, mas era a 1ª vez depois de importar tudo
  (a Unreal compilando shaders e malhas: "AssetCompile", "Waiting for skinned assets"). Medido no caminho de verdade
  (`Tools/RunRealFps.ps1`: Continuar → herói → mundo, com o save dele, a HUD medida por parte): 72 FPS em 1920x1080 sem
  os shaders, ~56-63 com eles. O "careca sem roupa" foi na janela de teste, também com os cabelos ainda compilando.
- **Shaders escuros:** com a luz global (Lumen) o céu vaza mais nas sombras (0,6) e a exposição sobe +0,6 EV
  (`VortexDayNightSubsystem.cpp`).
- **Tutorial:** sem o F; 16 passos (com C, V, Q, X, Z, B, M, I); segue fora do Estádio até o fim. O F saiu da lista do F1.
- **Interiores** (`BuildVilaKit.py`: `Furn`, `interior`, `upstairs_room`): cama de madeira com cabeceira, colcha e
  travesseiros; guarda-roupa; lareira de pedra até o teto; estante com livros; baú; criado-mudo com vela; cadeiras com
  encosto; tapete com borda; estandartes e quadro. Os sobrados têm escada até um vão no piso de cima (com colisão,
  guarda-corpo e o quarto); a taverna tem os quartos de aluguel em cima. Texturas novas do Poly Haven
  (`MakeVilaTextures.py Piso Movel Colcha Tapete Lareira Couro Livros PedraPiso RuaGrama`, acertadas no
  `Tools/Town/FixInteriorTextures.py`).
- **Os 25 cm do andar de cima:** o lampião, a hera, a placa e os móveis eram feitos com as medidas do andar de cima
  (que avança): ficavam soltos na frente da parede (os "cipós flutuando") e a escada saía para fora. Agora usam as do
  térreo. A hera nasce no chão.
- **Telhados:** tábua de beiral nas pontas (cobre o fim das fileiras de telha).
- **Forja:** refeita (telheiro com forja, coifa, fole, bigorna, têmpera, rebolo; a casa de pedra com porta de verdade e
  as armas do Bram; telhados sem vão).
- **Chão:** as pedras da beira de cada placa emendam com as da vizinha (placas sem giro); faixas de transição
  pedra/grama (`vila_borda_*`, 93); quintais só com floreiras; os postes virados para a rua.
- **Portas:** a folha é do jogo (`vila_porta`, no `AVortexTown`): E abre e fecha (a HUD avisa); casas começam
  fechadas, lojas, taverna e capela abertas.
- **Placas:** `Tools/Town/MakeSigns.py` desenha cada uma (madeira, o nome em Cinzel dourada, o símbolo: bigorna, caneca,
  ferradura...), importadas em `/Game/Vortex/Town/Vila/Placas` (`M_VilaPlaca`), na tábua `vila_placa` do kit (a
  imagem com UV feito à mão; o cubo da Unreal deitava e espelhava a imagem).
- **HUD:** `Tools/MakeHudFantasy.py` refaz as peças `H_*` (painéis, encaixes, aros, barras, tecla) no estilo dos ícones
  (azul-noite e ouro); as antigas em `Data/UI/HudCartoon_antigo`.

- **Janelas:** `Tools/MakeUiKitFantasy.py` refaz as peças `W_*` (moldura, botões, abas, fita, encaixes) no mesmo estilo;
  as antigas em `Data/UI/KitCartoon_antigo`. A letra da HUD e das janelas virou a Cinzel 900 (um pouco menor: é mais
  larga; o texto saía das caixas).
- **Depois (o 4º pedido, com uma imagem de referência):**
  - Placas penduradas: `vila_placa_pendurada` (braço de ferro com volutas, correntes, tábua de escudo com moldura de
    ferro), as imagens novas do `MakeSigns.py` (madeira escura, espiral do Vortex, o símbolo em bronze, o nome curto).
  - Telhados telha por telha (`gable_roof`): barro em capa e canal (com o barro liso `T_Vila_TelhaLisa`, do
    `FixInteriorTextures.py`), madeira desencontrada, palha em camadas. Casas de 20 a 28 mil triângulos; 75-79 FPS nas fotos.
  - Retrato da HUD (`VortexPortrait.cpp`, `_v3`): de frente, com folga em cima e embaixo, fundo azul-noite; só tira a
    foto com os modelos do herói prontos (logo depois de importar saía só com os olhos).

- **Janelas pelas referências do usuário (4 imagens: herói, inventário, diário de missões, loja):**
  - Loja refeita (`SVortexShopWindow`, `VortexHeroWindow.cpp`): o retrato do vendedor (`VortexPortrait::CaptureFace`,
    a mesma foto do herói tirada do morador; sem o morador por perto, a inicial), o nome na fita, a fala no pergaminho,
    a grade "Itens à venda" (5 colunas, o preço embaixo, vermelho sem dinheiro) e "Seus itens (vender)" (4 colunas),
    clicar escolhe, e os botões grandes Comprar (verde), Vender (vermelho) e Tudo; suas moedas embaixo.
  - Diário de missões: as duas páginas de pergaminho (`W_Parchment`, `Av::Parchment()`), letra escura (`InkText`).
  - Fitas dos títulos em vermelho-vinho (eram laranja); o fundo do boneco azul-noite (era azul-claro); os encaixes com
    o miolo e o aro na cor da raridade e a quantidade maior; usar = botão verde, desequipar = vermelho.
  - Descartar item não existe no jogo (precisaria de uma chamada nova no `VortexHeroSheet.h`): ficou de fora.

- **Depois ("agora gostei"):** luzes da noite mais fracas (postes 38, janelas 9, interiores 8, vidro do lampião 2,5);
  a porta freia no fim e tem som (`door_open`/`door_close`, sintetizados no `Tools/MakeDoorSounds.py`, importados pelo
  `ImportExtraSfx.py`); os guardas olham para dentro da vila (`BuildVila.py`); os postos fixos de monstro saem a 45 m da
  muralha (eram 15: uma matilha de 2 lobos nascia sempre num portão) e, vencidos, voltam a 6-20 m de onde nasceram
  (`bWander`); no tutorial o V (ombro) vem antes do C (primeira pessoa).

- **As 35 espadas** ([02_Combate/ESPADAS.md](02_Combate/ESPADAS.md)): recortadas das pranchas
  (`Tools/Weapons/SliceSwordSheets.py`), em blocos na moldura da espada do jogo (`Tools/Blender/MakePixelSwords.py`,
  importadas pelo `ImportBlockSword.py`), com nome, ícone (`BuildSwords.py`) e um poder cada (`Data/Rpg/swords.json`,
  `VortexSwords.cpp`). A Lâmina do Recruta é a espada inicial. Raridade nova: Divina (e a Única ganhou cor).
  Foto de teste: `Tools/RunWeaponCloseUp.ps1 -Types "item:asa_da_fenix,item:genese"`.
  - **Animação, efeito e som de cada poder:** o poder usa a animação do secundário da arma de onde veio (o redemoinho
    gira como o machado, o golpe no chão bate como o martelo: `VortexSwords::SpecialClip`, pelo ícone
    `special_<arma>`). No começo do poder (`VortexSwords::OnSpecialPose`, chamado pela animação em todas as máquinas)
    a lâmina ganha o rastro do elemento (`NS_ArrowTrail_*`) e o chão a explosão do elemento (`NS_Free_Spells_*`), e
    toca o som da espada (`sw_<chave>`: 34 sons sintetizados pelo `Tools/MakeSwordSounds.py`, golpe + elemento +
    brilho da raridade; importados pelo `ImportExtraSfx.py`).
- **Avisos do baú e da inscrição (07/11):** abrir um baú, ler uma inscrição, rezar no santuário, os convites de grupo
  e o portal mostravam "NÍVEL N / +2 pontos de atributo" (o tipo 2 dos avisos é o "em destaque", e a HUD nova
  desenhava o nível para todos). Agora só o aviso "Nível N!" mostra o nível; os outros aparecem com o texto deles num
  painel no alto (`VortexAimHUDCartoon.cpp`), e as moedas ganhas seguem no canto ("+N moedas").

- **Os 35 arcos (07/11)** ([02_Combate/ARCOS.md](02_Combate/ARCOS.md)): das pranchas, em blocos, sem corda (a corda é
  da Unreal), cada um com a sua flecha (a ponta no formato do elemento) e um poder; três poderes novos de arco: leque
  (volley), flecha que atravessa (pierce) e flecha que explode (explosive). Tudo importado; fotos com
  `RunWeaponCloseUp.ps1 -Types "item:arco_da_fenix,..."` (a ferramenta agora equipa o tipo de arma do item).
- **Do teste do usuário (07/11, à noite):**
  - a vitrine do herói (janela I e mochila) estava escura e esticada: o `M_UiDoll` ganhou o parâmetro de brilho
    `Bright` 1,55 (`Tools/MakeUiDollBright.py`; o `ImportUiKitCartoon.py` já cria assim) e a imagem fica em 3:4
    (`DollView`);
  - a página TÉCNICAS mostrava as 4 técnicas do Godot por nível (nunca feitas aqui; a defesa do F não existe): agora
    mostra o golpe e o poder do botão direito da arma na mão (nome, texto, mana e recarga); o F saiu dos comandos;
  - o rastro no ar do 1º golpe do combo estava inclinado ao contrário (`VortexHitFx::SwingTrail`: a ida e a volta são
    a mesma diagonal). Falta o usuário conferir jogando.
  - Comandos da Unreal pelo PowerShell perdem o "4" de "4 - Projetos" no `-script=`: rodar pelo bash.

- **As 21 armaduras (07/11, à noite)** ([02_Combate/ARMADURAS.md](02_Combate/ARMADURAS.md)): das pranchas do usuário
  (`modelando ainda/Futuras modelagens/Armaduras`), 84 peças com atributos, passiva própria e bônus de conjunto; em
  blocos com a pintura da prancha projetada e em camadas; trocam a parte da roupa que cobrem (os 6 trajes do herói
  agora têm 4 materiais) e o capacete troca o cabelo (capuzes com a franja na cor do cabelo). Fotos no jogo:
  `Docs/Images/armaduras/no_jogo.png`. As passivas compilaram e carregam (`VORTEX_ARMOR 84 peças`), mas ainda não
  foram testadas em luta: falta o usuário jogar com elas.

- **Teste do usuário (07/11, à noite):** as armaduras com lados faltando (faces viradas para dentro: corrigido no
  gerador, as 84 e os 6 trajes refeitos) e os poderes "sempre o mesmo giro dourado" (as animações emprestadas eram do
  esqueleto antigo). Agora cada espada e cada arco tem a sua animação, feita no Blender, e o seu efeito, montado em
  código (ver ESPADAS.md, "Animações e efeitos próprios"). Falta o usuário jogar com eles.

- **Poderes um por um (07/11, à tarde):** o usuário quer cada poder detalhado e único (animação no Blender, efeito na
  Unreal, texturas do EffectCraft). Feito o lote 1 (Maré Baixa e Marco do Guardião, ver ESPADAS.md "Poderes feitos à
  mão"); e à noite os outros 66 (cada um com animação, efeito e, quando o texto pedia, a mecânica: veneno, queimadura,
  beber a vida, lentidão, prender, explosão; ver ESPADAS.md). Falta o usuário jogar com eles. Também: o travamento ao usar poderes (o som e o pré-carregamento) e a mochila
  que se refazia a cada clique num item (agora só a moldura e o painel mudam).

- **Arsenal, o modo de teste (07/11, à noite):** botão "Arsenal" no menu: mapa leve com todas as espadas, arcos,
  outras armas e as 21 armaduras em manequins, bonecos a 5, 10 e 20 m, E equipa, mana infinita e poder sem recarga,
  nada grava (ver 02_Combate/ARSENAL.md). Falta o usuário entrar pelo menu e testar.

- **Balanceamento, nível de item e saque no chão (07/11, à noite):** o golpe da arma multiplica (os poderes valem o que
  dizem, os tipos ficam perto), escala de cada tipo no balance.json; a mesma arma em qualquer nível ("<id>@<nível>"), com
  trava de nível; o saque cai no chão rodando com o facho da raridade (bichos, baús, "Jogar no chão"). Ver
  02_Combate/BALANCEAMENTO_ARMAS.md e SAQUE_NO_CHAO.md. Falta o usuário jogar com isso.

- **Poderes de classe refeitos (07/11, à noite):** os 264, cada um com animação no Blender e efeito próprio (ver
  02_Combate/COMBATE_CHEFE_CLASSES.md, o fim). Falta: compilar com o editor fechado, importar as animações
  (`VORTEX_ANIMS=classes`) e conferir no jogo.

- **Economia (07/11, à noite):** forja do Bram (subir o nível do item com moedas e material), loja no nível do herói e
  poções que curam uma parte da vida, com preço pelo nível (ver 02_Combate/ECONOMIA.md).

Ainda falta: a água do poço quadriculada; as espadas claras escuras na sombra; os lados das armaduras pegam a borda da
prancha (riscados de perto).

## A vila nova (06/11; compilada, importada e com fotos; falta o usuário jogar)

A vila expandida foi refeita do zero, pelas referências de `modelando ainda/Futuras modelagens/Cidade`.

- **Planta:** `Tools/Town/BuildVila.py` gera `Data/GodotTown/vila.json` (o `AVortexTown` lê no lugar de
  town.json + town_extra.json) e a foto `Docs/Images/vila/planta_vila.png`.
  - Muralha de 8 lados (cerca de 160 x 160 m), 9 torres e 3 portões: o norte, com o nome da cidade; o sul; e o leste,
    para o Estádio.
  - Avenidas, ruas e o anel por dentro da muralha. Praça central com a fonte, mercado, praça do poço e campo de treino.
  - 79 casas (8 modelos) e os prédios: prefeitura, taverna, quartel, carpintaria, forja, capela, estábulo e 5 lojas
    com as placas de antes.
  - Moinho e hortas fora do portão norte.
- **Terreno plano:** `FVortexTerrainData::Load` aplaina o chão dentro do "flatten" da planta, com uma rampa de 16 m nas
  bordas (log `VORTEX_TOWN_FLAT`).
- **Kit:** `Tools/Blender/BuildVilaKit.py` gera `Data/GodotTown/VilaKit/*.fbx` e as caixas de colisão em
  `vila_models.json`.
  - Casas em enxaimel com base de pedra, janelas, venezianas, jardineiras, sacadas, águas-furtadas, chaminés e
    lampiões.
  - Entra-se em 3 construções: o sobrado (`vila_casa_2`), a casa-loja e a taverna. Têm móveis, lareira e balcão.
  - Muralha, torres e portão com blocos de pedra em relevo (`stone_courses`).
  - Ruas em placas de 3 m de paralelepípedo, com pedras de alturas diferentes e juntas finas; as fileiras continuam de
    uma placa para a outra.
- **Fotos sem o jogo:** `Tools/Blender/PreviewVilaKit.py`.
- **Texturas:** `Tools/Town/MakeVilaTextures.py` baixa do Poly Haven (CC0) com o normal map; vidro, folha e brasa são
  desenhados.
- **Na Unreal:** `Tools/ImportVilaKit.py` cria o `M_VilaKit` (textura × cor da peça, Bright, Glow e Nrm) e os
  `MI_Vila_*`, e liga com `set_material`.
- **Fumaça:** sai de todas as 87 chaminés (posição em `chimneys` do kit; `smoke` com 5 números = fumaça fina).
- **Fotos no jogo:** `Tools/RunTownShot.ps1`. As câmeras estão em `Data/Tools/town_shots.json`, que se muda sem
  compilar.
- **Correções depois do primeiro teste (06/11):**
  - **Muralha:** sem vão nas pontas, porque os trechos cobrem tudo até as torres. Ficou mais clara (blocos e reboco).
  - **Portal da primeira dungeon:** saiu do portão leste e foi para fora da muralha, em (120, -20) m
    (`VortexTerrainStreamer.cpp`).
  - **Bichos:** os postos fixos que caíam dentro da vila nova vão para 130-190 m do centro
    (`VortexCreatureSubsystem.cpp`, que usa `AVortexTown::Contains`).
  - **Casas:**
    - piso a 20 cm e degrau baixo;
    - o alicerce tem colisão (antes o herói afundava no piso);
    - não ficam mais em cima das ruas, porque a borda do quarteirão é a borda das placas;
    - fundos com porta, alpendre, lenha, barril e vasos.
  - **Placas com nome:** ficam lisas na parede, sem a plaquinha do modelo que cortava o texto.
  - **Praça:** o piso é de lajes 3D.
  - **Enfeites antigos do Godot:** foram trocados pelos do kit (banco, floreira, poste, barraca, caixotes, poço,
    quadro, cerca e carroça).
  - **Fumaça:** está mais grossa. Os números ficam no `smoke` do `vila.json`.
  - **NPCs:**
    - lojistas atrás do balcão (Ilsa, Nicolly, Rosa e Garrick) e Marta na taverna;
    - Bram na forja, Rhea na porta do quartel e Ducci na carroça;
    - Lary e Julia dão a volta na fonte.

    O `BuildVila.py` escreve esses lugares em `Data/Population/npc_posts.json`. O jogo salvo não prende mais o NPC no
    lugar velho.
- **PRÓXIMO PASSO: o que o usuário pediu depois do 2º teste (06/11).** Ainda não foi feito; começar por aqui.
  - **Tamanho:** diminuir a vila. O tamanho do padrão que vimos antes estava ótimo, e esta ficou grande demais.
  - **Casas coladas:** muitas casas se encostam pelas laterais e ficam com janelas na parede colada. Nas casas
    geminadas, a lateral encostada não pode ter janela. A capela está sem porta.
  - **Interiores:** fazer em todas as casas, bonitos e combinando com o estilo de cada uma.
  - **Natureza em volta:** árvores, pedras e grama flutuando, sem ligar com a cidade. Conferir a rampa do aplainamento
    (blend) e os enfeites do terreno perto da borda.
  - **Chão:** às vezes aparece um chão de cascalho aleatório (a placa `vila_terra`? o campo de treino?).
  - **Bloqueios invisíveis:** o herói bate em coisa invisível no caminho. Suspeitas:
    - caixas de colisão de peças que saíram;
    - o tronco das árvores;
    - a colisão da `vila_carroca` ou da barraca;
    - o degrau.
  - **Luz:** a iluminação está bugada. Vários postes não iluminam nada, nem de noite nem de dia (ver as luzes de
    `lamps` no `AVortexTown::Tick`).
  - **Postes:** feios; remodelar. Pesquisar na internet como as cidades medievais dos jogos fazem.
  - **Vidros das janelas:** que sejam vidro de verdade (translúcido, com reflexo).
  - **Poço:** água de verdade dentro dele.
  - **Guardas:** em todas as entradas, nos 3 portões.
  - **Fora da muralha:** sem plantação nem construção. Tirar as hortas e o moinho.
  - **Telhados:** algumas texturas bugadas nos telhados e nas partes de cima (UV/escala dos telhados e oitões).
- **Feito em 07/11, de madrugada (a vila de novo):**
  - **Tamanho:** a vila tem 124 m (`HALF` 62). O centro foi para x = 18 e as ruas têm 6 m: tudo cai na grade das placas
    de 3 m.
  - **Prédios:** ficam no canto do quarteirão. São 50 casas.
  - **Fora da muralha:** o moinho e as hortas saíram.
  - **Casas:**
    - todas de entrar, com interior no estilo de cada uma (`interior()`):
      - casa: lareira, mesa com bancos, cama, armário e escada;
      - chalé: fogão de canto com caldeirão, cama de palha, ervas e sacos;
      - loja: balcão de 95 cm e prateleiras;
    - móveis na escala do herói e com colisão;
    - os barris da loja saíram de trás da porta;
    - teto de tábuas entre os andares.
  - **Janelas:** vidro de verdade, com furo na parede (`wall_face`) e o `M_VilaVidro` translúcido.
  - **Luzes:** luz de dentro sempre acesa e fraca (`inner` no `vila_models.json`, lida pelo `AVortexTown`).
  - **Laterais coladas:** as variantes `_e`, `_d` e `_g` de cada casa não têm janela no lado do vizinho. O
    `BuildVila.py` escolhe a variante pelo vizinho.
  - **Capela:** porta aberta (a torre cobria) e interior com bancos, altar, velas e tapete.
  - **Poço:** com a água do lago (`M_VortexWater`).
  - **Postes:** poste novo (base de pedra, haste com anéis, braço com voluta e lampião pendurado). O vidro do lampião
    acende à noite (um MID do `MI_Vila_Lanterna`). A luz dos postes passou de 30 para 70 cd, com raio de 9 m.
  - **NPCs:**
    - guardas: 2 em cada portão;
    - novos: Tomás (padeiro), Irmã Célia (capela), João (estábulo), Edda (carpintaria) e Bento (anda pela avenida).
  - **Bloqueios invisíveis:** eram as colisões das árvores e pedras do Godot (`solids.bin`) dentro da vila. Agora saem
    (`CreateSolidCollision`).
  - **Natureza flutuando:** as plantas, pedras e as colisões delas acompanham o chão aplainado (`FVortexTerrainData`).
  - **Cascalho aleatório:** as camadas 4, 5 e 6 do terreno dentro da vila viram grama.
  - **FPS nas fotos de teste (~25):** é igual com a vila nova desligada (`vila.json` renomeado). Também não muda sem
    as luzes de dentro (`-VortexTownNoInner`), sem o vidro translúcido (`-VortexTownNoGlass`) ou sem os NPCs
    (`-VortexNoPopulation`). Então não é a vila: é o estado da máquina nos testes escondidos. Conferir o FPS jogando
    de verdade. O `RunTownShot.ps1 -Extra "<flags>"` passa as chaves.
  - **Ainda falta:**
    - a água do poço aparece quadriculada no teste (é a água do terreno nessa qualidade);
    - os telhados (texturas) ainda não foram revistos de perto;
    - HUD e menus novos (PhotoCraft/FigCraft).
  - **Tutorial:** passo a passo no Estádio, com 10 passos. Cada um mostra a tecla, uma barra e o check verde, e passa
    sozinho para o próximo. Backspace pula; o F1 ainda mostra a lista completa (`DrawControls` com Alpha -1).
- **Feito em 06/11, à noite (compilado; falta o usuário testar):**
  - **Corrida sem arma:** a disparada (braços rápidos) só toca com o Shift (`VortexAnimInstance.cpp`).
  - **Arco:** o corpo fica virado para a mira enquanto o botão está apertado e vira para a câmera ao soltar
    (`EndPrimaryAttack`). O "virar para onde anda" não briga mais com a mira (`VortexCharacterMovementComponent.cpp`).
- **Achados para a vila (ainda não feitos):**
  - **Capela sem porta:** a torre do sino (a caixa de `d/2-3.2` a `d/2+.2`, que começa no chão) cobre a porta. Fazer a
    torre começar acima da porta.
  - **Água do poço:** usar `/Game/Vortex/Materials/M_VortexWater` (a mesma da fonte).
  - **Postes:** a luz dos lampiões é 30 cd e a das janelas 9 (`AVortexTown::Tick`); subir. O vidro do lampião precisa
    brilhar à noite (um MID do `M_VilaKit` com Glow, trocado no Tick).
  - **Plano para diminuir a vila:**
    - `HALF` 62, `CUT` 20, `STREETS` [-24, 0, 24];
    - prefeitura 17x12 e quartel 16x11;
    - tirar o moinho e as hortas (sem `FLAT_OUT` e sem os spinners).
  - **Casas coladas:** variantes de cada casa sem janela na lateral encostada (sufixos `_e`, `_d`, `_g`), escolhidas
    no `BuildVila.py` pelo vizinho.
- **Mais pedidos (06/11, à noite):**
  - **Conferir cada casa:** ver todas de cima e as laterais de cada uma.
  - **Entradas:** o chão certinho com as entradas das casas e das lojas.
  - **Colisão:** uma loja tem madeira na frente da porta, mas dá para atravessar.
  - **Móveis:** alguns muito grandes para o personagem.
  - **NPCs:** fazer mais alguns para completar a vila.
  - **HUD e menus (PhotoCraft/FigCraft):** estilos novos, como o inventário, combinando com as imagens das skills e
    com a cara do jogo (hoje estão sem qualidade).
- **Também para depois (o usuário, 06/11):**
  - **Corrida sem arma:** a velocidade e a distância da corrida ficaram certas, mas sem arma na mão a animação volta a
    ficar frenética. Provavelmente a passada (Stride) ou a corrida do pé (FootRun) do estado "sem arma" não recebeu o
    mesmo ajuste. Ver `VortexAnimInstance.cpp` e `VortexCharacterMovementComponent.cpp`.
  - **Tutorial (F1 no começo):** o atual está ruim. Refazer como passo a passo: ensina uma coisa por vez, o jogador
    faz, aparece o "check" e passa para a próxima. Com a cara do jogo, no visual da HUD (FigCraft para desenhar).
  - **Arco atirando de costas:** dá para olhar para trás, virar a câmera rápido e atirar para o lado oposto ao do
    personagem. Ao mirar ou atirar, o corpo tem que virar para onde a câmera aponta (ou não deixa atirar enquanto não
    vira).
- **Falta:**
  - conferir os NPCs nos prédios novos jogando (a Rosa, a Rhea e o Ducci ainda estão nos lugares da vila antiga);
  - trocar os enfeites antigos do Godot (bancos, lampiões, barracas) por novos no estilo do kit;
  - conferir a colisão andando.

## Feito em 05/11 (compilado; falta o usuário testar jogando)

- **Flecha encaixada no arco** (`UVortexWeaponParts::TickBowString`): ela mede a malha da flecha e se estica do ponto
  da corda até passar da madeira do cabo (12% do comprimento do arco, no mínimo 8 cm). Antes supunha 80 cm e ainda
  encolhia pela escala do arco.
- **Barris e caixotes:**
  - só a espada quebra (`StrikeInArc` confere se o `WeaponId` tem "espada"); as outras armas, a flecha e a magia não;
  - a flecha crava no barril e rola junto (o tiro agora procura também os corpos com física, `ECC_PhysicsBody`).
- **Corpo feminino:** usa a mesma malha do masculino (`HeroModelFor` e `VortexNpc`). Muda só a textura (o top,
  `T_humano_Pixel_F`) e os olhos com cílios, então as animações e a pegada do arco ficam iguais às do masculino. A
  malha `Feminino/Hero_humano` ficou no Content, sem uso.
  - **Conferir:** as roupas femininas foram feitas no corpo de cintura mais fina e podem encostar no corpo.
- **Locomoção:**
  - passadas mais longas: a corrida cobre 1,5x (era 1,4) e o andar 1,08x;
  - a animação um pouco mais lenta: `Stride` 1,37 correndo e 1,16 andando (eram 1,18 e 1);
  - o rolamento vai 15% mais longe no mesmo tempo (`RollReach`).

## Onde parei (04/11)

Atualização baseada no último teste do usuário. Só registrar; não alterar o jogo até novo pedido.

- **Flecha no arco carregado:** está pequena. Aumentar o tamanho e alinhar corretamente: a ponta deve ficar depois da curvatura da madeira, e a outra extremidade deve encostar na própria corda, como numa montagem real.
- **Barris quebráveis:** corrigir a interação: só a espada deve quebrá-los; flechas estão atravessando os barris.
- **Vila expandida:** refazer a vila inteira do zero, mantendo a expansão, elevando a qualidade de tudo e removendo os bugs atuais: elevações indevidas, elementos antigos misturados e construções/entradas incoerentes (por exemplo, um poço numa entrada).
- **Corpo feminino:** usar a mesma configuração de corpo/animações do masculino, pois o corpo é idêntico; corrigir as animações quebradas, especialmente a do arco, que hoje difere do masculino.
- **Locomoção masculina:** reduzir um pouco a velocidade da animação, aumentar a distância percorrida para combinar com passadas mais longas e ampliar também o deslocamento do rolamento.

## Onde parei (03/11, manhã)

O resumo para o usuário está em dois PDFs, que se refazem sozinhos:
- **O que falta:** [O_QUE_FALTA.pdf](O_QUE_FALTA.pdf) (feito a partir de `O_QUE_FALTA.html`; abrir no Edge e imprimir em
  PDF, ou `msedge --headless --print-to-pdf`).
- **As 66 classes e as 264 habilidades:** [05_Referencias/CLASSES_E_HABILIDADES.pdf](05_Referencias/CLASSES_E_HABILIDADES.pdf),
  feito por `python Tools/MakeClassesPdf.py` (lê `Data/Rpg/rpg.json`). Cada habilidade tem um número (ex.: 12.3).
  O usuário vai dizer uma por uma quais refazer ("todas feias ou bugadas").

**Próximos passos, nesta ordem:**
1. Refazer as habilidades das classes, conforme o usuário for dizendo.
2. Arco na primeira pessoa (o que ficou pendente abaixo).
3. Ver em foto o herói molhado e o rosto do slime; gerar os instaladores com a vila nova quando o usuário pedir.

(A expansão existente está registrada abaixo, mas deverá ser substituída por uma vila expandida refeita inteira do zero, conforme a atualização de 04/11.)

## Pedidos para o futuro (03/11, o usuário testou; NÃO fazer antes de ele pedir)

O usuário pediu só para anotar, porque os créditos da semana estão acabando. Ele ainda vai conferir mais coisas.

- **Locomoção do corpo masculino:** reduzir um pouco a velocidade da animação de caminhada, aumentar a distância percorrida para que as passadas sejam mais longas e aumentar também a distância do rolamento.
- **Slime:** o rosto continua bugado e o corpo ainda está duro. Ele quer um bem parecido com o estilo do Slime Rancher
  ou do Rimuru Tempest: bem mole, gelatinoso e fofo, com rosto simples e expressivo (estilo parecido, sem copiar o
  personagem).
- **Vila:** está bem bugada. Refazer a vila expandida inteira do zero, melhorando a qualidade de tudo. Corrigir as elevações, a mistura de construções antigas com novas e incoerências como um poço numa entrada. A vila expandida feita em 03/11 não atende e deverá ser substituída.
- **Corpo feminino:** usar o mesmo corpo e as mesmas animações do masculino, pois o corpo é idêntico. Corrigir o estado atual quebrado, especialmente a animação e a pegada do arco, que diferem do masculino.
- **Herói molhado:** quase não muda (o `UVortexWetness` de 03/11 só escurece e dá brilho abaixo da linha da água).
  Ideias, pelo que os jogos costumam fazer:
  - **O material** (o que Uncharted, The Last of Us, RDR2 e Zelda BotW/TotK fazem):
    - escurecer mais a cor (a água tira a luz difusa) e baixar bem a aspereza, para o brilho especular aparecer;
    - a linha molhada com uma borda irregular (ruído), e não reta;
    - secar de cima para baixo e em manchas, não tudo de uma vez.
    - No nosso estilo de blocos, dá para fazer em degraus de pixel: a borda da linha em "escadinha" de texel.
  - **Gotas:** pingos caindo do corpo, dos braços e do cabelo enquanto está molhado (Niagara de cubinhos azuis, como
    os outros efeitos do jogo). Mais pingos logo ao sair da água, diminuindo até secar.
  - **Pegadas molhadas:** marcas de pé escuras no chão por uns segundos (decal em pixel), que somem com o tempo.
  - **Cabelo e roupa:**
    - o cabelo molhado fica mais escuro e liso, com menos volume (ex.: trocar o "Looseness" e baixar as mechas);
    - a roupa molhada também escurece.
  - **Respingos:** o herói solta respingo ao correr molhado ou ao sair da água correndo.
  - **Chuva:** a mesma molhadeira serve para a chuva, molhando de cima para baixo.
  - **Opcional:**
    - chacoalhar o corpo, que os bichos fazem (o cachorro da Julia e o lobo);
    - um ícone de "molhado" na HUD, se no futuro molhado mudar algo do jogo (por exemplo, menos dano de fogo e mais de
      raio).
- **Armaduras com raridade:** as referências já estão em `Docs/Images/armaduras` (11 imagens). As raridades vêm do nome
  de cada arquivo:
  - **normal:** Apprentice Wool, Iron Watch, Trailwalker Leather;
  - **incomum:** Herbalist's Robe, Mossrunner e a ficha Sentinela/Mossrunner/Herbalista;
  - **raro:** Conceitos de Armaduras Voxel Fantásticas;
  - **épico:** Catálogo de Armaduras Voxel Fantásticas;
  - **único:** Tríptico de Armaduras Voxel Fantásticas;
  - **lendário:** Armaduras Lendárias de Fogo em Pixel Art;
  - **divino:** Conjuntos Divinos dos Selos.

  Fazer como as roupas de blocos (`Tools/Blender/MakeBlockOutfit.py`, cascas em volta das partes do corpo, textura
  tirada da ficha), ligadas aos espaços de equipamento (`VortexRpg::EEquip`: Head, Chest, Legs, Boots) e à cor da
  raridade. Os prompts antigos estão em `Docs/05_Referencias/PROMPTS_ARMADURAS_E_MUSICA.md`.
- **Armas:** o usuário ainda vai mandar as referências.
- **Abertura como pergaminho (04/11):** o usuário vai gerar as imagens com os prompts de
  `Docs/03_Mundo/CUTSCENE_PERGAMINHO_PROMPTS.md`:
  - `P0`: o rolo fechado;
  - `P1` a `P8`: os trechos 4:1, com 3 cenas cada (ou `P1a/b/c`... se o gerador não fizer 4:1).

  Quando chegarem, mudar o `VortexIntroComic.cpp`: o rolo abre, o pergaminho desenrola para os lados e a câmera anda
  de cena em cena no tempo dos versos. Hoje o código mostra as 32 cenas soltas de `Data/UI/Abertura/Cinema`.
- **Usar os apps do ArtCraft (instalados em 03/11, ver o CLAUDE.md):**
  - **Criação do mundo:** a vila nova e o terreno, com MaterialCraft (pedra, madeira, telhado, chão) e TextureCraft
    (retoques à mão), reduzidos para pixel art. Os sons de ambiente vêm do SoundCraft.
  - **HUD:** refazer para combinar com o estilo de blocos e pixel art do jogo.
    - Desenhar as telas no FigCraft.
    - Ícones e molduras no VectorCraft e no PhotoCraft.
    - Animações dos menus e das barras no EffectCraft.
  - **Habilidades das classes:** junto da refação das 264 habilidades (o PDF de classes):
    - ícones de cada habilidade (PhotoCraft e VectorCraft);
    - efeitos em sequência de quadros (EffectCraft);
    - sons (SoundCraft);
    - os números de dano e recarga numa planilha (GridCraft).

## A vila maior (03/11, tarde; compilada, fotos do `RunTownShot` conferidas; falta o usuário jogar)

O usuário aprovou o plano.
- **Planta:** `Data/GodotTown/town_extra.json`, gerada por `python Tools/BuildTownExpansion.py`. Não editar à mão.
  - Ela soma à planta do Godot (`town.json`), que não muda.
  - A muralha passou de x −40..40, z −24..56 para x −62..62, z −34..80 (cerca de 124 × 114 m, o platô plano sem os
    lagos). Os portões norte e sul foram para a muralha nova e entraram os do leste e do oeste.
  - Bairros:
    - artesãos (oeste: carpintaria, curtume, alquimia);
    - quartel com campo de treino e estábulo (leste);
    - mercado com 6 barracas e a capela (norte);
    - o moinho e as hortas, fora da muralha, ao sul.
  - Ruas de pedra ligadas às antigas, postes, bancos, jardineiras, árvores e mastros com bandeiras.
  - A planta vista de cima sai de um script de conferência (o desenho com as peças, as ruas e os barris).
- **Construções novas, em blocos de 12,5 cm:** `Tools/Blender/BuildTownBuildings.py`, que gera
  `Data/GodotTown/Models/*.glb` e as caixas em `town_models_extra.json`.
  - Os modelos: moinho e as pás (que giram), capela, estábulo, quartel, mastro, 3 bandeiras, barril, caixote, feno,
    boneco de treino e horta.
  - Na Unreal: `Tools/ImportTownBuildings.py`. Ele cria o `MI_<nome>`, filho do `M_TownVoxel` (o pano usa o
    `M_TownCloth`, que balança com o vento).
  - **Pegadinhas da cor (custaram várias tentativas):**
    - A Unreal não importa a cor dos vértices desses glb. A cor vai numa paleta (64 × 64, cada cor nos quatro cantos
      espelhados) e cada face aponta para o pixel dela.
    - O material lê sempre o mip 0.
    - Sem LODs e sem Nanite.
    - **Principal:** o material precisa de `used_with_instanced_static_meshes`. Sem isso, a vila (que desenha em
      instâncias) troca o material e tudo fica marrom.
- **Código da vila:** `AVortexTown` (`LoadPlan`).
  - Lê as duas plantas e passa os limites novos para o `Contains`.
  - Monta as pás que giram (`URotatingMovementComponent`), as bandeiras, a fumaça da chaminé do quartel e a luz de 38
    janelas à noite.
  - O terreno não põe árvores nem pedras dentro da vila nova (`VortexTerrainStreamer.cpp`).
- **Barris de água, de vinho e caixotes:** `AVortexBreakable` (`Source/Vortex/.../World/VortexBreakable.*`), 27 na
  vila, que vão pela rede.
  - Física no servidor: o herói empurra e eles rolam.
  - Na água, boiam e fazem o mergulho.
  - Atualmente golpe (`StrikeInArc`), flecha (`Stick`) ou magia (`StartBurst`) quebram; conforme atualização de 04/11, corrigir para que somente a espada quebre os barris (flechas hoje atravessam). As tábuas voam, o líquido espirra em
    cubinhos e fica uma poça brilhante que se espalha e seca em 20 s. O barril volta inteiro em 3 minutos.
  - **Pegadinha:** o terreno só tem colisão perto dos jogadores. A física só liga com alguém a 25 m, e o barril que
    cair do mapa volta para o lugar dele.
  - Foto: `cidade_barris_quebrados.png` (as tábuas no chão e as poças).
- **Herói molhado:** `UVortexWetness`, criado no `BeginPlay` do herói, guarda até onde a água chegou no corpo (dados
  primitivos 30 e 31).
  - `Tools/AddWetLook.py` pôs o "molhado" nos materiais-mestre do herói: abaixo da linha, escurece até 55% e fica
    liso e brilhante.
  - Seca em ~45 s.
  - **Ainda não foi visto em foto.**
- **Boca do slime:** o `M_SlimeFace` não compilava (uma variável se chamava `line`, palavra reservada do HLSL). Foi
  corrigido; antes o jogo usava o material padrão.

## Rodada de 03/11, manhã (compilada; falta o usuário testar jogando)

- **Arco:**
  - Em pé na mão fora do tiro: `UVortexWeaponComponent::TickBowGrip`. Ele gira o arco em volta da mão até ficar em
    pé; atirando, volta à pegada medida no `Arco_Draw`.
  - Carga: `full` 1,1 s, `min` 0,3x, `max` 1,7x, `min_draw` 0,35 s e `gap` 0,5 s (`VortexWeaponKit.cpp`).
  - O tiro fraco cai: `ArrowCharge` no `LaunchProjectile`, `Drop` no projétil (2,6x a gravidade no fraco, 0,45x no
    cheio). Só o cheio compensa a queda na mira.
  - Anel da carga na mira: `AVortexAimHUD::DrawWeaponAim`.
  - A flecha encaixada na corda enquanto puxa: `UVortexWeaponParts::TickBowString` (`Nocked`).
  - Correndo e puxando: o quadril leva 35% do giro do `Arco_Draw` e o tronco o resto (`VortexAnimInstance.cpp`).
- **Flechas na rede:** o projétil manda `LaunchFrom`, `LaunchDir` e `LaunchTime`. Cada máquina refaz o voo desde o
  arco; até 0,15 s de atraso começa do zero. `NetPriority` 3.
- **Arco na primeira pessoa (resolvido em 08/11, ver o topo):** `TickViewBow` (deixar o arco da tela em pé) está escrito, mas desligado em
  `AVortexCharacter::Tick`. Nas fotos (`perto_arco_de_betula_1p*.png`), o arco sumia parado e continuava torto
  puxando.
  - O `ViewOffset` do projétil já faz o desenho da flecha sair do arco da tela em primeira pessoa.
  - Falta: o arco certo na tela, a mão que puxa não tampar o meio e a flecha encaixada visível. A flecha carregada também está pequena: aumentar e posicionar a ponta depois da curvatura da madeira e a extremidade oposta encostada na corda.
- **Cajado:**
  - Caps Lock trava a mira (`ToggleLock`, `ServerSetLock`, `LockTarget`). A câmera acompanha o alvo, as magias vão
    nele e as vinhas também (`AimTarget`).
  - Sem o cajado, o Caps continua sendo "correr sempre".
- **Cura no aliado da mira:** `HealTarget`. A HUD marca quem vai receber; sem ninguém na mira, cura o herói e o grupo.
- **Bola de fogo:** miolo, casca, `NS_Fire_Medium` em volta e `NS_MagmaCore_Explosion` ao bater (`VortexProjectile.cpp`).
- **Slime:**
  - Gelatinoso: as molas são moles e o alto do corpo tem inércia. São os dados primitivos 9 e 10 do material, que
    também ondula a superfície e o braço do bote.
  - Rosto animado: a boca saiu da malha (`BuildSlime.py`, imprime `SLIME_FACE`) e virou o `M_SlimeFace` num plano.
    Ela tem humor, abertura e língua (`UVortexSlimeLook::TickFace`); as sobrancelhas aparecem bravo.
  - Backup da malha antiga: `Backups/SM_Slime_2026-11-03.glb`.
  - Ainda não foi visto em foto: o herói tampa o slime nas fotos do `RunCreatureSmoke`.
- **Ping:**
  - Mostra o nome do herói (`UVortexSocial::GetHeroName`) e a cor do jogador (`GetPlayerColor`, 8 cores).
  - Shift+Z troca a cor, que fica salva no GameUserSettings (`[Vortex] PlayerColor`).
  - Os outros heróis e os pings aparecem na cor de cada um no minimapa e no mapa grande.
- **Nome do cavalo:** ao domar, `AskMountName` abre a janela (`VortexSpellChoice::OpenHorseName`, um gancho novo no
  `GenUiHooks.py`). O nome fica em `MountName` (vai pela rede e fica no save) e aparece no lugar de "SEU CAVALO".
- **Placas da vila:** uma tábua com moldura por trás do nome, do tamanho do texto (`VortexTown.cpp`).
- **Cratera sem o lago:** no `FVortexTerrainData::Load`, as células dentro do raio da caldeira que ficavam abaixo da
  água sobem para `WaterLevel + 60`. O log diz `VORTEX_CALDERA_FLOOR`. Ainda não foi visto em foto.
- **Primeira pessoa olhando para baixo:** os olhos ficam na frente do osso `Spine2` (`TickFirstPerson`).
- **Linux:** o clang recusava 2 coisas que o Windows aceitava, `VortexMist.cpp` e `VortexVolcanicBoss.cpp`.
  Corrigidas.

## Rodada de 03/11 (compilada e testada no `RunCreatureSmoke`; falta o usuário testar jogando)

- **Cavalo do amigo:** nos outros jogos o cavalo ficava solto do lado do amigo montado. Agora cada quadro confere e
  prende de novo (`AVortexMonster::Tick`).
- **Interagir perto de um amigo:** NPC, baú, portal e cavalo vêm antes. O menu do amigo só abre colado (1,9 m) e de
  frente para ele.
- **Cura do cajado no duelo:** não cura ninguém em duelo (devolve a mana) e a recarga é 1,6x maior.
- **Cajado:** dano 1,4 (era 1,15).
- **Regeneração de mana:** os pontos em Mana também regeneram (`mana_regen_per_mana` em `balance.json`, 0,04).
- **Tutorial de comandos:** um cartão à esquerda com as teclas. Aparece sozinho no Estádio do Despertar; F1 mostra e
  esconde em qualquer lugar (`AVortexAimHUD::DrawControls`).
- **Corrida:** passo mais longo (o clipe toca 18% mais devagar e corre 1,4x); o rolamento vai um pouco mais longe (335).
- **Música da cratera:** `MUS_Azhur` (da pasta `Docs/musicas futuras`) toca dentro da cratera do primeiro chefe.
- **Sons dos monstros** (o usuário: "muitos mobs com o som do lobo"):
  - a causa: todo bicho que acertava o herói tocava a mordida do lobo. Agora cada um tem o seu golpe
    (`<bicho>_strike`);
  - vozes novas (apanhar, morrer e gritar ao atacar) para goblin, esqueleto, morto-vivo, aranha e sapo;
  - feitas por `Tools/MakeCombatSfx.py` com sons CC0 do OpenGameArt (`Data/Sfx/CombatCC0`, licenças lá dentro).
- **Sons das armas:** o martelo ganhou o próprio whoosh pesado (antes usava o da espada). Adagas, cajado (lançar, tiro
  e acerto) e arco (puxar e flecha cravando; antes a flecha tocava o acerto da espada) ganharam sons novos. A espada
  continua com as gravações do Godot.
- **Slime gelatinoso** (o usuário: "estilo Slime Rancher, bem amoeba"):
  - molas moles, que passam do ponto e voltam;
  - o alto do corpo fica para trás quando ele arranca, freia ou apanha (inércia);
  - a superfície ondula sempre, cada gota no seu tempo;
  - o bote tem preparo, sai com folga e volta chacoalhando.
  - Código em `VortexSlimeLook.cpp`; o material em `Tools/ImportSlime.py` (dados primitivos 9 e 10).
- **Pasta organizada:** os modelos do Meshy e os `.zip` das armas soltos em `Docs/` foram para `Docs/Modelos_Meshy/`.
  Os temporários do `Saved/` e as cópias `.blend1` foram para `../_Descartavel` (a lista está no `LEIA-ME.md` de lá).
- Instaladores novos (Windows e Linux) no itch.
- **Falta:** testar o Azhûr em rede.

## Azhûr, o golem vulcânico (02/11, de madrugada)

O chefe da Caldeira foi refeito pelo documento de design do usuário. Tudo está em
[03_Mundo/AZHUR_GOLEM_VULCANICO.md](03_Mundo/AZHUR_GOLEM_VULCANICO.md):
- 20 animações naturais (7 novas) e as mãos consertadas;
- 12 habilidades, 2 fases (troca em 40%), IA com estados e atordoamento;
- 9 sistemas Niagara;
- os números em `Data/Creatures/azhur.json`;
- a cratera mais limpa.

`RunBossShots` PASS. Ainda não foi testado em rede.

## Rodada de 02/11 à tarde (compilada e importada; falta o usuário testar no jogo)

- **Armas iniciais em blocos:** pixel art em cubos no tamanho, no ângulo e na pegada das do Meshy.
  - A espada (`Tools/Blender/MakeBlockSword.py`) e as outras (`MakeBlockWeapons.py`: adagas, martelo, cajado, arco).
  - Na Unreal: `Tools/ImportBlockSword.py`, que importa todas e liga as definições. `VORTEX_SWORD=meshy` volta às do
    Meshy.
  - A brasa, as gemas e o cristal ficam num 2º material que brilha (`MI_WN_<arma>_blocos_brasa`).
  - **Por que as do Meshy ficavam escuras e com "diamantes":** a malha é um mosaico de milhares de triângulos e a
    pixel art repintada era escura. Fotos de antes e depois: `Saved/Comparar_Espada/comparar_espada.png`.
  - **Pegadinha do Blender:** trocar o espaço de cor de uma imagem depois de preencher os pixels apaga tudo. O mapa
    de rugosidade saía todo 0 (espelho), e a lâmina parecia de vidro e pontilhada na Unreal.
- **Balanceamento:** [02_Combate/BALANCEAMENTO.md](02_Combate/BALANCEAMENTO.md), com os números em
  `Data/Rpg/balance.json`.
  - Os pontos rendem cada vez menos no mesmo atributo, numa curva que cresce com o nível.
  - A defesa divide o dano.
  - O dano base sobe com o nível; as criaturas escalam pelo nível da região.
- **Criaturas:**
  - **Sorteio:** a cada partida as do mato mudam de lugar e de espécie (`SpawnSalt`; nos testes fica 0).
  - **Renascer:** depois de vencidas, voltam 6 a 20 m de onde nasceram.
  - **Vila:** nenhum bicho entra nela (o lobo também larga a caça na vila).
- **Animações:**
  - **Corrida:** +30%, com o clipe no ritmo natural (1,05x em vez de 1,3x). Com a arma, a disparada também entra.
  - **Lobo:** o passo medido na pata no chão (andar 160, correr 380 cm/s × escala 1,2).
  - **Golem:** o andar sem o gingado que abria as pernas para o lado (`Data/Modelos/Golem_Musgoso/Golem_Skinned.glb`;
    backup em `Backups/golem_2026-11-02`).
  - **Pegar:** o "Picking Up" de verdade (`anao_Pegar.fbx`, quadros 22 a 168 em 1,7 s; antes era um pedaço do golpe
    pesado, e o herói parecia deitar). O masculino e o feminino foram refeitos e importados.
- **Sola do pé:** era cor de pele na textura do corpo (`humano_cor(_f).png`). Agora é de solado escuro (backup em
  `Backups/corpo_sola_2026-11-02`).
- **NPCs:**
  - **Lexy, nova:** a cabeleireira do Santuário, na frente da casa da rua leste (8,5; 40 m).
    - O visual: o suéter rosa de oncinha (`lexy()` e `leopard()` em `MakeBlockOutfit.py`), o short rosa e o coturno
      longo. O cabelo é o `camadas_mechas`, com o topo rosa pela cor de vértice.
    - 3 missões na história (`v_lexy_*`): a tinta de cristal, os morcegos do salão e "o rosa que aponta" (fala com a
      Ilsa).
  - **Juh, removida:** era a Julia duplicada. As missões dela passaram para a Julia (`MERGED_NPCS` em
    `Tools/BuildStoryData.py`), e o `ExtractNpcPosts.py` pula a Juh.
  - **Cachorro da Julia:** cabeça grande e fofa, orelhas longas, olhos maiores, língua de fora e 25% maior, como em
    `modelando ainda/referencia.jpeg` (`AVortexPet::Build`).
- **Testes:** `RunPopulationSmoke -Shot` tira fotos de perto da Lexy e da Julia (`npc_lexy.png`, `npc_julia.png`).

### Segunda leva de 02/11 à tarde (compilada e importada; os testes de fumaça passaram; falta o usuário jogar)

- **Arco novo:** estava deitado. A curva foi para o outro plano e a pegada ficou na mão (`MakeBlockWeapons.py`).
- **Lexy:**
  - Saiu de dentro da pedra: agora fica em (5,5; 40 m) (`npc_posts.json`).
  - Cabelo chanel com degradê de rosa para preto (`Tools/Blender/MakeLexyHair.py`, `bob_degrade_mechas`; a cor de
    vértice é FLOAT_COLOR, porque a de byte é sRGB e o degradê saía todo rosa).
- **Lobo:** não trava mais depois do ataque (a trava da ação dura só o golpe: mordida 0,7 s, bote 1 s).
- **Chuva de flechas:** as flechas caem rápido (4200 cm/s), e 75% delas vão num inimigo da área (`RainLanding`).
- **Martelo:** o especial não engasga mais na 1ª vez (os efeitos de onda são carregados antes, `VortexHitFx`).
- **Azhûr:** as mãos tortas foram endireitadas no `RuneColossus_Skinned.glb` (backup em `Backups`).
- **Minimapa e mapa grande:** mostram a vila, com o chão de pedra, os telhados e o contorno (`AVortexTown::MapKindAt`).
- **Baú novo** (`Tools/Blender/MakeChest.py` → `Tools/ImportChest.py`):
  - em blocos, com o comum (madeira e ferro) e o raro (vinho e ouro, com gemas);
  - a tampa abre em 0,7 s, passa um pouco do ponto e fica aberta (o diário marca o baú);
  - sai um brilho de tesouro; as moedas de dentro brilham.
- **Cachorro da Julia:** refeito do zero em blocos (`Tools/Blender/MakeDog.py` → `Tools/ImportDog.py`;
  `AVortexPet` monta as peças).
- **Goblin novo** (o do Meshy, `Docs/Modelos_Meshy/goblin simples.zip`, extraído em `Data/Modelos/Goblin_Meshy`):
  - `Tools/Blender/MeshyBiped.py` (papel `goblin`) passa para ele os clipes do goblin antigo, e a corrida é a do
    próprio site.
  - Ganhou um porrete de blocos na mão direita (o antigo tinha um).
  - Na Unreal: `VORTEX_SPECIES=Goblin` no `Tools/ImportMeshyMonsters.py`. O goblin antigo está em
    `Backups/goblin_antigo_2026-11-02`.
  - Vale também para o Brutamontes e o Ladino (mesmo modelo). `RunCreatureSmoke -Species Goblin,...` passou.
  - **Pegadinha (corrigida):** o esqueleto do FBX do site vem girado 90° no objeto. No glb a Unreal montava o goblin
    amassado e deitado, e o porrete saía quebrado. O `MeshyBiped.py` agora aplica o giro e a escala nos ossos e na
    malha antes de animar.
- **Arco, 2ª correção:** estava em pé, mas "de lado". Agora a curva fica no mesmo plano e para o mesmo lado do arco
  do Meshy (`bend_side` em `MakeBlockWeapons.py`).
- **Cotovelo do herói (homem e mulher):** desde 01/11 o braço era feito de blocos rígidos (um por osso), e o cotovelo
  abria um vão quando dobrava. A pedido do usuário, voltou a ser um bloco só, com dobra macia no cotovelo e no pulso
  (`ARM_SMOOTH` em `RigBlockCharacter.py`; ele aceita que amasse um pouco nas danças). A perna continua rígida.
  - Refeito: `RigBlockCharacter.py` (e `--feminino`) → `humano.fbx` → `MixamoToHero.py` (os dois) →
    `ImportMixamoHeroes.py` (`VORTEX_RACES=humano`) e `ImportBlockFemale.py`.
  - O backup da versão rígida está em `Backups/cotovelo_rigido_2026-11-02`.
  - Ainda sem foto do jogo: o usuário pediu o instalador antes.
- **Multiplayer: convidado preso no cenário do menu:**
  - **Causa:** o código que começa com L é de sala de rede local (traz o IP da casa do anfitrião). De outra casa ele
    não conecta. O `JoinByAddress` escondia o menu e esperava em silêncio, sem nada no `VortexRede.log`.
  - **Correção:** o menu fica com "Conectando..."; com 20 s sem resposta, volta ao menu com um aviso de que o código
    com L só serve no mesmo Wi-Fi e que, de casas diferentes, a sala precisa ser a online da Epic (5 letras).
  - O log agora registra a tentativa e a desistência. O anfitrião também registra o tipo de sala e o código.
  - A sala online da Epic funciona (`RunOnlineLobbySmoke` PASS).
  - **Por que a sala do amigo abriu na rede local (corrigido):** depois de criar um herói no mundo multiplayer, o
    menu refazia a lista de heróis com `OpenHeroes`, que zerava `bSelectedWorldOnline`, e a sala abria na rede
    local. Agora `OpenHeroes(mundo, anfitrião, online)` mantém a sala online (o mesmo vale para excluir um herói).
- **Rodada da noite de 02/11 (multiplayer e combate; o usuário testou com um amigo):**
  - **Cavalo nos outros jogos:**
    - o cavaleiro aparecia em pé dentro do cavalo: a suavização da rede repunha a malha (`CacheInitialMeshOffset` em
      `ApplyBodyShape`);
    - ele tremia: montado, a cópia simulada fica sem suavização;
    - ao descer, ficava para sempre na altura da sela: o `Tick` zera o `MountLift` quando não há mais cavalo preso.
  - **XP em grupo** (`balance.json`: `groupRange`, `groupXpBonus`, `groupHealthPerHero`):
    - cada herói vivo a até 40 m ganha o XP inteiro, +10% por companheiro;
    - o bicho leva menos dano com mais heróis perto (vida ×1,5 com dois).
  - **Amigo que sai e volta:** volta com o último herói viajante usado (`LastTravellerHero` no
    GameUserSettings), e não com um herói novo de nível 1.
  - **Heróis entre mundos:** num mundo multiplayer, a lista do anfitrião mostra também os heróis viajantes.
  - **Duelo:**
    - o dano usa a ficha dos dois (`pvpDamage` 0,5); antes era o dano puro da arma, 1 por golpe;
    - **domo de energia** (`AVortexDuelDome`, material `Tools/MakeDuelDome.py`): os dois não saem, os outros
      (heróis e bichos) são empurrados para fora, e nada fere de fora para dentro.
  - **PvP aberto:**
    - herói contra herói fora do grupo e fora da vila (`CanHarm`);
    - quem mata alguém de fora do grupo, que não era caveira, fica com **caveira por 3 dias do jogo**
      (`UVortexHeroSheet::MarkOutlaw`, salva em horas);
    - na HUD, o nome fica vermelho com a caveira em cima.
    - **Limite:** a amizade fica no save de cada PC; o servidor só sabe do grupo.
  - **Cajado:**
    - dano ×1,15 (era 0,73);
    - magia mais rápida (55 m/s) e maior; sai da altura da mão (antes saía rente ao chão);
    - o cipó acha o inimigo mais perto da linha da mira;
    - **escolha de magia** na primeira vez do botão direito: Cura, Vinhas ou Bola de Fogo (`VortexSpellChoice`,
      `StaffSpell` na ficha e no save). Só uma fica.
  - **Golem:** as coxas abriam quase na horizontal em todos os clipes menos o andar. Corrigido no glb e reimportado;
    o backup está em `Backups/golem_2026-11-02b`.
  - **Foto da HUD:** cabeça e ombros, três quartos, fundo azul em degradê. O arquivo agora é `<id>_v2.png`, então as
    antigas são refeitas.
  - **Diagonal acelerando (era a cavalo, pulando sem parar, como o "bunny hop"):** no ar, a velocidade a cavalo fica
    limitada à do galope (`UVortexCharacterMovementComponent::CalcVelocity`).
  - **Herói "voando" ao montar e flutuando depois:** nos outros jogos o cavalo bloqueava heróis (o `ECR_Overlap` só
    valia no servidor). Além disso, `RefreshPresentation` religava a colisão do cavalo montado e ele empurrava o
    cavaleiro para cima. Agora: sem colisão enquanto montado, o cavalo nunca bloqueia heróis, e o cavaleiro ignora o
    cavalo (`IgnoreActorWhenMoving`). Ainda não há teste de montaria em rede.
  - **Instalador travando:** a janela usava `AllowsTransparency` (o WPF desenha tudo no processador). Agora a janela é
    opaca, com os cantos retos, e o Vórtice anima a 30 quadros por segundo (`Tools/Installer/Portal/Portal.cs`).
- **Chuva de flechas:** com as flechas puxando para os inimigos, umas 15 das 20 acertavam o mesmo bicho, a 1,3× o
  dano cada uma (uns 20 golpes). Agora é 0,3 por flecha, uns 5 golpes num alvo (`VortexWeaponKit.cpp`).
- **Corpos das raças para o Mixamo** (`modelando ainda/`, feitos por `Tools/Blender/MakeRaceBodies.py`):
  - `corpo_elfo`, `corpo_anao`, `corpo_barbaro`, `corpo_esqueleto` e `corpo_demonio` (.fbx).
  - Os itens separados: `item_orelhas_elfo`, `item_chifres_demonio` e `item_cranio_esqueleto`.
  - **Falta:** pôr os itens no menu (como os cabelos), depois que o usuário passar os corpos no Mixamo.

## Módulos (02/11, de madrugada, com a permissão do usuário)

- O código começou a ser separado em módulos para compilar menos. O primeiro é o `VortexWorld` (terreno, streamer,
  névoa, dia e noite, cidade, água, portais, marcos, pontos de interesse e as opções do jogador), embaixo do `Vortex`.
- O passo a passo, os ganchos (`VortexWorldHooks`), os redirecionamentos das classes e o plano dos próximos estão em
  [01_Base/MODULOS.md](01_Base/MODULOS.md).
- O backup do código de antes está em `Backups/source_2026-11-02`.

## NPCs no corpo de blocos (02/11, em andamento)

- **Código:** `AVortexNpc::ApplyLook` (`ApplyBlockBody` em `Creatures/VortexNpc.cpp`). O NPC humano usa o corpo de
  blocos do herói (masculino ou `Feminino/`), com o rosto (`block_face`), o cabelo de voxel (`block_hair`, com a física
  e `hair_color`) e uma roupa de blocos: `npc_outfit` (a roupa só dele, `Roupa_<id>`, fora do menu) ou `block_outfit`
  (uma do menu). Quem não é humano continua no boneco antigo (aventureiro).
- **Dados:** `Data/Population/npc_posts.json` (`_doc_blocos` explica). Mudar não precisa compilar.
- **Lary e Julia:** roupas próprias pelas fotos (`modelando ainda/lary referencia.jpeg` e `referencia.jpeg`), em
  `MakeBlockOutfit.py` (`lary()`, `julia()`).
  - Peças novas do gerador: `skirt()` (saia rodada em faixas), `glasses()` (óculos de barrinhas em volta dos olhos),
    `sparkle()` (paetê), `stripes()`, `dots()`, `pendant()` (a estrela), `chain()`. As roupas sem ficha ganharam luz
    em cima e contorno nas beiradas.
  - A Julia usa o cabelo `liso_longo_mechas`: o liso longo com as pontas e umas mechas marcadas pela cor de vértice
    (R = 0), pintadas pelo `M_BlockHairMechas` com `hair_streak`. O `corpo: "npc"` em `cabelos.json` tira o cabelo do
    menu.
  - O cachorrinho dela (`AVortexPet`) já é o bichon branco da foto.
- **Os outros 13:** cada um com a roupa própria (`juh()` ... `marta()` em `MakeBlockOutfit.py`). Peças novas:
  `panel()` (avental, tabardo, capa), `back_block()` (aljava, mochila, tubo de mapas), `head_wrap()` (turbante).
  As alturas do corpo em texels estão num comentário antes de `juh()`. Para refazer:
  `blender -b --factory-startup -P Tools/Blender/MakeBlockOutfit.py -- jogo <id>` e `Tools/ImportBlockOutfits.py`.
- **Saias no passo:** abaixo do quadril, a saia (`skirt()`) é feita de duas metades, cada uma rígida na sua coxa (como
  nos jogos de blocos). Inteira no quadril, a perna atravessava; presa às duas coxas, rasgava no meio. Os painéis
  abaixo do quadril (aventais, tabardo, capas) seguem a média das duas coxas (até 60%).
- **Falta testar no jogo:**
  - as saias e os aventais andando;
  - se a animação do NPC (Idle/Walk/Run do Mixamo) bate com a velocidade dele (o ramo de NPC do
    `UVortexAnimInstance` usa Speed/240 no andar);
  - as mechas azuis da Julia.
- **Correção em todas as roupas:** a parte de cima do tronco pegava o peso da cabeça. Quando ela virava, puxava a
  roupa para cima. Agora o tronco da roupa só segue coluna e quadril. As 6 do menu foram refeitas (backup em
  `Backups/roupas_2026-11-02`).

## Rodada de 02/11 (compilada; falta o usuário testar no jogo)

- **Aviso de golpe dos monstros** (`Combat/VortexTelegraph.cpp`): a área cinza virou anéis de blocos (a borda na cor
  do golpe e o preenchimento que cresce até a hora do acerto). A onda dourada e a explosão (do golpe secundário do
  martelo) só saem nos chefes (`bBigShow`).
- **Rosto "fofo"** (`Tools/MakeBlockFace.py`): os olhos ficaram do tamanho normal, só com os dois brilhos.
- **Primeira abertura** (`VortexGraphics::FirstRunSetup`, chamado no `VortexGameInstance`): o jogo aplica a qualidade
  recomendada do PC, liga a neblina e pergunta no menu se quer os shaders ativos. Fica marcado em
  `GameUserSettings.ini` ([Vortex.Graphics] AutoSetup / AskShaders).
- **Neblina com shaders** (`World/VortexMist.cpp`): a neblina volumetrica cobria o céu. Agora ela começa na distância da
  névoa e vai só até 8 km (`VolumetricFogStartDistance` / `VolumetricFogDistance`), mais fraca.
- **Assobio do cavalo:** gerado (`Tools/MakeWhistle.py` -> `Data/Audio/horse_whistle_v1..3.wav`, importado por
  `Tools/ImportExtraSfx.py`), tocado em `AVortexCharacter::Whistle()`.
- **Corrida 8% mais rápida** (`FootRun = 1.08f` em `VortexCharacterMovementComponent.cpp`).
- **Texturas novas das armas (aprovadas, feitas):** tudo em pixel art, no estilo da Espada de Cinzas repintada.
  As antigas estão em `Backups/armas_texturas_2026-11-02`.
  - As 6 do Meshy (textura própria): `Tools/Blender/RestyleWeaponTexture.py` -> `Data/VortexWeapons/Textures/Novas/T_N_*.png`.
    A espada é pintada por parte (lâmina, sulco de brasa, fuligem, guarda e pomo com ouro, couro vinho); as outras pelo
    material de cada cor (`parts_by_color`; a bétula e o cajado com a madeira clara). Vários modelos com a mesma
    textura vão juntos com `;` (o arco ametista e a flecha).
  - As 28 dos materiais compartilhados (as outras 17 armas): `Tools/MakePixelWeaponMaterials.py` ->
    `SourceArt/Blender_Personagem_Armas/04_Texturas/T_<material>{,_normal,_rough}.png` (64 px que se repetem, ampliados 8x).
  - Na Unreal: `Tools/ImportVortexWeapons.py` e DEPOIS `Tools/ImportNewWeapons.py` (o segundo devolve a espada longa
    ao DA_DefaultSword). As cores entram com o filtro Nearest (pixel nítido).
- **Corpo feminino e arco:** a malha feminina era de antes do osso `Bow_L` (o arco caía no Socket_Weapon_R, o bug
  antigo). Refeita com `MixamoToHero.py -- <raiz> humano --clips anao --skin "modelando ainda/personagem_base_rig_f.fbx"
  --out Data/Heroes/humano_Mixamo/Feminino` e importada (`Tools/ImportBlockFemale.py`). **Sempre que o masculino for
  refeito, refazer o feminino também.**
- **Montaria:** o cavalo era preso 12 cm atrás do herói, e ele sentava na cabeça. Agora a sela (osso `Saddle`) fica
  embaixo do quadril (`AVortexMonster::ApplyRiding`, log `VORTEX_MOUNT sela x=.. quadril x=..`).
- **Corrida:** +15% (era +8%). O corpo não muda a velocidade.
- **Nível:** 7 pontos de atributo por nível (`VortexRpg::PointsPerLevel`, era 2).
- **Arco:** dano 2x (`PrimaryDamageScale` 0,75 -> 1,5): a flecha carregada vale ~2 golpes de espada.
- **Flechas cravadas:** entram 30 cm no bicho (15 na parede), sem ficar penduradas pela ponta, e somem em 3 s.

## O que foi feito na última conversa (31/10 e 01/11)

### Heróis do Mixamo
- **Esqueleto e animações:** os heróis usam o esqueleto completo do Mixamo "como veio" (`Tools/Blender/MixamoToHero.py`,
  `Tools/ImportMixamoHeroes.py`).
- **Animações derivadas:** esquiva para os lados e para trás, arrancada, freada e corrida com arma.
- **Rosto:** rosto animado no estilo Hytale (material `M_HeroFace`; `Tools/ImportHeroFace.py`).
- **Raças:** só o humano está liberado no menu; as outras estão com cadeado.

### O humano de blocos
O humano agora é o personagem de blocos feito no Blender (`Tools/Blender/MakeBlockCharacter.py` e
`RigBlockCharacter.py`), com as animações do Mixamo que o usuário fez.

- **Rosto:**
  - olhos grandes no estilo da referência, sem boca (`Tools/MakeBlockFace.py`);
  - a versão feminina tem cílios (`--feminino`).
- **Movimento:**
  - andar é o normal;
  - Shift faz a corrida lenta;
  - Caps Lock liga "correr sempre";
  - a corrida rápida foi tirada (`Data/Heroes/Mixamo/humano/fora.json`).
- **Versão feminina:** o mesmo corpo e as mesmas animações, com top e cílios (`Tools/MakeBlockFemale.py`).

### Cabelos separados
26 cabelos separados (11 masculinos, 15 femininos, mais careca), em blocos de meio pixel da cabeça.

- **Gerador:** `Tools/Blender/MakeBlockHair.py <estilo>`. Para o jogo: `-- jogo <estilo>`.
- **Importação:** `Tools/ImportBlockHair.py` → `/Game/Vortex/Characters/Heroes/humano/Hair/Hair_<estilo>`.
- **Cor:** escolhida pelo jogador (`M_BlockHair` = cor × tom).
- **Física:** nos compridos, rabos, chiquinhas e tranças (`UVortexHairComponent`). Colidem com cabeça, pescoço, costas e
  ombros, com folga maior quando há armadura.
- **Menu de criação:** painel "Aparência" com corpo (masculino ou feminino), cabelo, cores prontas e cor livre (matiz,
  saturação, brilho e `#RRGGBB`).

**Situação:** tudo compilado e importado.

**Teste do usuário (01/11, à noite):**
- O cabelo saía da cabeça ou ficava torto, em todos os estilos.
- A cor mudava sozinha.
- Muitos estilos não ficaram como ele pediu.

**Corrigido no mesmo dia:**
- **Atraso de um quadro:** era a causa do cabelo saindo da cabeça.
- **Cor:** a cor voltava ao padrão da peça antiga do Godot.

O passo a passo está em [PERSONAGEM_BASE_BLOCOS.md](02_Combate/PERSONAGEM_BASE_BLOCOS.md).

**Segundo teste do usuário:** os cabelos estavam cinza, e o moicano se soltou depois de combate e giros rápidos.
- **Cinza:** a importação tinha posto o material cinza da Unreal nos 26 cabelos.
- **Moicano:** o cabelo sem física agora segue o corpo como as roupas.

As duas correções foram compiladas. O usuário ainda vai testar de novo.

**Feito em 01/11 à noite (o usuário ainda vai testar no jogo):**
- **Cabelos refeitos (2ª versão):** `Tools/Blender/MakeStrandHair.py`. São blocos com a forma das mechas: mechas
  penteadas pela cabeça, espetos em degraus (o moicano punk, uma crista estreita), cachos, coques, rabos e tranças.
  A franja nunca desce abaixo das sobrancelhas. Os 26 foram exportados e importados (`ImportBlockHair.py`).
- **Membros rígidos:** `RigBlockCharacter.py`. Braço, antebraço, mão, coxa e canela são blocos separados, cada um num
  osso, sobrepostos 2 cm na junta. A canela entra 2 cm no pé, e o pé inteiro fica no Foot. Antes, o joelho e o
  cotovelo dobravam como borracha e o pé quebrava ao meio nas danças. O Hero_humano foi refeito e importado
  (`MixamoToHero.py humano --clips anao` e `ImportMixamoHeroes.py`). O backup do corpo antigo está em
  `Backups/corpo_blocos_2026-11-01`.
- **Corpo feminino:** `RigBlockCharacter.py --feminino` faz um busto pequeno, um bloco com o desenho do top, e a
  cintura 9% mais fina. A malha vai para `/Game/.../humano/Feminino/Hero_humano` (`MixamoToHero.py --skin --out` e
  `Tools/ImportBlockFemale.py`), no mesmo esqueleto. O `HeroModelFor(raça, aparência)` usa essa malha no corpo
  feminino.
- **Arco:** o clipe `Arco_Draw` gira o quadril -91° (o arqueiro de lado), e só o tronco era aplicado, então o braço
  do arco apontava para o lado.
  - Parado, o arco vai de corpo inteiro.
  - Nos clipes só de tronco, o giro do quadril do clipe em volta do eixo de cima passa para o tronco. Isso vale para
    todos os golpes (`VortexAnimInstance.cpp`).
- **Tudo compilado.**

**3º teste do usuário (02/11, madrugada): ficou "perfeito"; os pedidos novos foram feitos, compilados e importados (falta o teste):**
- **Topo dos ombros e sola das roupas:** essas faces não aparecem em nenhuma vista da ficha; agora usam a cor média dos
  lados da peça (`PIECE_AVG` no `MakeBlockOutfit.py`).
- **Rolada que teleportava:** o clipe Dodge foi cortado do quadro 16 do "Quick Roll" e já começava com o quadril 1,38 m
  à frente. Agora o `MixamoToHero.py` desconta o deslocamento horizontal do primeiro quadro em todos os clipes. Nenhum
  clipe anda mais de 30 cm (o pulo e o Pegar, menos de 12 cm).
- **Montaria:** o herói do Mixamo não tinha o clipe Ride e ficava em pé, flutuando. Foi criado o clipe derivado "Ride",
  sentado na sela (`RIDE` no `MixamoToHero.py`: coxas para a frente e abertas, joelho dobrado, braços nas rédeas).
- **Primeira pessoa:** correndo, os olhos vão um pouco para a frente (`RunAhead`). Há a opção "Campo de visão (1ª
  pessoa)" (60–120°) nas configurações, guardada em `GameUserSettings.ini [Vortex.Camera]`, sem mudar o `.h` das
  configurações.
- **Corte do golpe:** o rastro usava o `ReachCm` do Godot, bem maior que o acerto. Agora usa o `MeleeReach`, o mesmo
  alcance do acerto.
- **Habilidades dos monstros:** `AVortexMonsterFx` desenha os efeitos de cada espécie com cubinhos, no estilo do jogo:
  - cogumelo: esporos em nuvem de fumaça venenosa; o cuspe pinga e estoura numa poça borbulhante;
  - morcego: o guincho vira um mini tornado de vento;
  - golem e gota de ferro: a pancada no chão solta poeira;
  - sapo: o salto respinga lama.

  Chamado pelo `MulticastMoveFx`. Ainda não foi visto no jogo.
- **Rostos:** 8 estilos (padrão, olhos azuis, olhos verdes, sereno, determinado, fofo, pontinhos, sardas), nos dois
  corpos.
  - `Tools/MakeBlockFace.py --estilo <id> [--feminino]` e `Tools/ImportBlockFaces.py` geram os
    `T_humano_Eyes[_F]_<id>`.
  - A lista fica em `Data/Looks/rostos.json` e a escolha em `Look.Pieces["block_face"]`; há a seção "Rosto" no painel.
  - O cabeçalho é à parte (`VortexFaceStyles.h`), para não recompilar o jogo inteiro.
- **Travamento ao abrir o mundo:** em toda sessão a Unreal recompilava os mesmos 16 efeitos de pacotes antigos (2 a 6 s
  cada). `Tools/ResaveNiagara.py` salvou os 16 já compilados na 5.8; a lista está em `Data/Tools/niagara_resave.txt`.
  Sobram travões curtos de shaders na primeira vez.

**Ajustes sem compilar (01/11):** a regra "compilar menos" está no CLAUDE.md. Os ajustes saíram do código e foram
para arquivos que o jogo lê ao abrir:
- `Data/Looks/cabelos.json`: os cabelos, o nome no menu, o corpo e a física de cada um;
- `Data/Looks/roupas.json`: as roupas, o nome e o corpo;
- `Data/UI/menu_palco.json`: a luz de preenchimento e a exposição das telas do herói.

Para mudar esses valores basta editar o arquivo e abrir o jogo de novo. A compilação dos dois `.cpp` levou 28 s, contra
mais de 10 min quando se mexe num `.h` muito usado.

**Separar o código em módulos:** fica para depois. São vários dias para desfazer as dependências entre Framework,
Combat, World e Rpg.

**Pendentes novos:**
- **Luz do menu de criação:** a frente ficava clara demais e as costas escuras demais (contraste alto: o sol de
  frente e pouca luz atrás). Feito e compilado, falta o teste do usuário. Nas telas do herói (a lista e a criação),
  em `AVortexMenuStage`:
  - uma luz suave, sem sombra, anda 60 cm acima da câmera (`Fill`, 35 cd);
  - a exposição fica 0,35 mais baixa (-0,73).

  Se ainda ficar desigual, os ajustes são esses dois números.
- **Roupas (feito em 01/11, falta o teste no jogo):** 3 masculinas (aventureiro, guerreiro, mago viajante) e 3
  femininas (arqueira, sacerdotisa, aventureira), da ficha `modelando ainda/Ficha de Trajes RPG em Pixel Art.png`.
  - **Gerador:** `Tools/Blender/MakeBlockOutfit.py`. Cada peça é uma casca de bloco em volta de uma parte do corpo
    (tronco, cueca, braço, antebraço, coxa, canela, pé), no eixo do osso, com uma folga; mais as caixas soltas (bolsa,
    capuz). As peças dos membros vão inteiras no osso da parte.
  - **Textura tirada da própria ficha:** o herói vai para a pose da ficha (os braços abaixados até a largura bater) e
    cada texel pega a cor da vista onde aparece. A vista de lado da ficha mostra o lado esquerdo, com a frente para a
    esquerda da imagem. Onde a ficha mostra o fundo, fica a pintura do código. A pele só fica no decote. Um
    limpa-pontos tira os pixels soltos da borda do desenho.
  - **Primeira versão:** pintada só pelo código, o usuário achou feia e com poucos detalhes.
  - **No jogo:** `Tools/ImportBlockOutfits.py` leva para `/Game/.../humano/Roupas/Roupa_<id>`, com `T_Roupa_<id>` e
    `MI_Roupa_<id>` (pai `M_HeroTextured`). `FVortexLookCatalog::BlockOutfits()` e `Look.Pieces["block_outfit"]`; a
    padrão é a primeira do corpo. `AVortexCharacter::ApplyBlockOutfit` veste com `SetLeaderPoseComponent`. No painel
    "Aparência" há a seção "Roupa".
  - **A melhorar:** ainda aparecem manchas onde o braço da ficha não bate com o nosso (o dourado nas mangas do mago).
  - **2º teste (01/11):** o usuário gostou das roupas e dos cabelos, mas correndo a roupa não dobrava. A camisa era uma
    caixa só, e os cantos de cima pegavam o peso do braço. Corrigido:
    - as peças do tronco têm fatias de 2 texels e os pesos só da coluna e do quadril;
    - as dos membros vão inteiras no osso da parte (`PART_BONE`);
    - conferido no Blender com a corrida e o hip-hop.
  - **Arco:** estava ao contrário (as pontas curvando para longe e a corda do lado de fora). `GripRotationFor` ganhou
    meia volta em torno do comprimento do arco: o cabo fica para o alvo e a corda para o corpo. Compilado, falta o
    teste.

**Falta refazer os estilos.** (Feito acima, na 2ª versão; o texto abaixo é o diagnóstico da 1ª.) A comparação com as referências mostrou que os femininos e masculinos de 6 a 15 estão
quase todos errados.
- **Problemas:** curtos demais, quadrados como um capacete e com a mesma "cortina" em arco nos femininos.
- **Mais perto da referência:** raspado e undercut.
- **Novo jeito, pedido pelo usuário:** os cabelos não precisam ser só de cubos. Agora serão mechas, blocos que afinam,
  se inclinam e se curvam, em tamanhos diferentes. A cor, a física e o caminho para a Unreal continuam iguais.
- **Onde fazer:** no Blender aberto, pelo MCP, não na Unreal.

**Próximo passo:**
1. Esperar o usuário testar no jogo a correção da cor e do cabelo saindo da cabeça.
2. Abrir o Blender MCP e fazer dois testes do novo estilo: Moicano punk e Maria chiquinha.
3. Com os dois aprovados, refazer os outros.

**Faltam as referências dos estilos 1 a 5** de cada folha: as imagens vieram cortadas. Os estilos 6 a 15 já estão
nas imagens que o usuário mandou em 01/11.

## O que esperar do usuário (o teste dele)

- **Física:** se a física dos cabelos está no ponto. O "Looseness" de cada cabelo fica em
  `FVortexLookCatalog::BlockHairs()` (`Source/Vortex/Private/Framework/VortexLook.cpp`).
- **Seletor de cor:** se responde bem. O antigo era "bugadinho"; este é novo.
- **Animação:** se algum cabelo entra no corpo nas animações.
- **Femininos 1 a 4** (Liso longo, Ondulado longo, Cacheado longo, Meio preso): foram feitos só pelos nomes, porque a
  imagem de referência veio cortada.

## Pendentes (pedidos antigos, ainda não feitos)

- **Espada no jogo:** a espada aparece muito diferente do modelo do Blender em todo lugar. Em andamento: ver "Texturas
  novas das armas" na rodada de 02/11.
- **Pé do personagem de blocos:** sobrepor o pé na perna para esconder o vão do tornozelo (foi só oferecido).
- **Ofertas antigas:** os monstros Seryth e o sapo, e os poderes do Azhur.
- **Bandidos:** conferir no editor a pegada do escudo e da flecha
  ([02_Combate/CONTINUAR_ARSENAL.md](02_Combate/CONTINUAR_ARSENAL.md)).
