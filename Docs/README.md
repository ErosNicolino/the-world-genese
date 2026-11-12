# The World Gêneses no Unreal: painel do projeto

Atualizado em 23/10/2026. Esta é a página de entrada. Ela mostra o que já foi feito, o que espera o seu teste no editor e o que ainda falta. Os relatórios completos estão nas pastas abaixo.

**Para testar tudo de uma vez:** [TESTAR_TUDO.md](TESTAR_TUDO.md) — `Tools\RunAllTests.ps1` roda todos os testes automáticos, e o guia traz um passeio de ~20 min no editor.

## Como a pasta está organizada

| Pasta | O que tem |
|---|---|
| [01_Base](01_Base) | Projeto, framework, IDs/save/rede, movimento, terreno de prova e personagem (Fases 2 a 6) |
| [02_Combate](02_Combate) | GAS, combate, espada, lobo, matilha, câmera e especial da espada (Fases 7 a 11 e correções); as 35 espadas ([ESPADAS](02_Combate/ESPADAS.md)) e os 35 arcos ([ARCOS](02_Combate/ARCOS.md)); as 21 armaduras ([ARMADURAS](02_Combate/ARMADURAS.md)); o modo de teste ([ARSENAL](02_Combate/ARSENAL.md)); a conferência do balanceamento das armas ([BALANCEAMENTO_ARMAS](02_Combate/BALANCEAMENTO_ARMAS.md)) e o nível de item; o saque no chão ([SAQUE_NO_CHAO](02_Combate/SAQUE_NO_CHAO.md)); a economia ([ECONOMIA](02_Combate/ECONOMIA.md)) |
| [03_Mundo](03_Mundo) | Névoa, streaming, população, dia e noite, grama e céu, água, floresta viva (Fases 12 a 15 e melhorias); os 6 lugares marcantes, os caminhos e as raízes da Árvore do Mundo ([LUGARES_MARCANTES](03_Mundo/LUGARES_MARCANTES.md)); o relevo de cada região, a erosão e os rios que serpenteiam ([RELEVO_POR_REGIAO](03_Mundo/RELEVO_POR_REGIAO.md)); os moradores com rotina, ofícios, passantes e voz ([NPC_ROTINA](03_Mundo/NPC_ROTINA.md)) |
| [04_Interface](04_Interface) | HUD, menu com palco 3D, seleção de raça e criação de herói |
| [05_Referencias](05_Referencias) | Inventário dos pacotes de `VortexReferencia` e onde cada um pode servir |
| [Images](Images) | Fotos usadas nos relatórios (antes e depois) |
| [Midia](Midia) | A canção e as folhas da HQ de abertura (`Abertura`), outras músicas e vídeos |
| [A_Lenda_de_Genese_e_o_Jogo.pdf](A_Lenda_de_Genese_e_o_Jogo.pdf) | A história do mundo pela canção "Acorde": o que o jogo já tem e o que falta |

O plano geral e a auditoria do Godot ficam fora desta pasta, em [`../../AUDITORIA_FASE_1.md`](../../AUDITORIA_FASE_1.md).

**Legenda**

| Marca | Significado |
|---|---|
| ✅ | Aprovado por você jogando no editor |
| 👍 | Você viu e gostou, ou pediu ajustes que já foram feitos; falta só confirmar |
| 🟡 | Pronto, com testes automáticos passando; espera o seu teste no editor |
| ⬜ | Ainda não começou |

---

## 1. O que já foi feito

### Roteiro original (fases 0 a 15)

| Fase | O que é | Situação | Relatório |
|---|---|---|---|
| 0 | Cópia de segurança do Godot e dos saves | ✅ | [Auditoria](../../AUDITORIA_FASE_1.md) |
| 1 | Auditoria e plano de migração | ✅ | [Auditoria](../../AUDITORIA_FASE_1.md) |
| 2 | Projeto C++ do Unreal 5.8 | ✅ (fases seguintes autorizadas) | [FASE_02_PROJETO](01_Base/FASE_02_PROJETO.md) |
| 3 | Framework: sessão, mapa multiplayer | ✅ | [FASE_03_FRAMEWORK](01_Base/FASE_03_FRAMEWORK.md) |
| 4 | IDs, saves versionados, mundo replicado | ✅ | [FASE_04_IDS_SAVE_REDE](01_Base/FASE_04_IDS_SAVE_REDE.md) |
| 5 | Movimento: corrida, esquiva, degraus, pulo, água e nado | ✅ (04/10) | [FASE_05_MOVIMENTO](01_Base/FASE_05_MOVIMENTO.md) · [andamento](01_Base/FASE_05_MOVIMENTO_PROGRESSO.md) |
| 6 | Terreno do vale, herói animado, colisão, roupas, grama com vento e pisada | ✅ (04/10) | [FASE_06_TERRENO_PERSONAGEM](01_Base/FASE_06_TERRENO_PERSONAGEM.md) |
| 7 | Atributos e habilidades (GAS): vida e mana (o vigor saiu em 09/10), recuperar | 🟡 | [FASE_07_GAS](02_Combate/FASE_07_GAS.md) |
| 8 | Combate com dano decidido no servidor | 🟡 | [FASE_08_COMBATE](02_Combate/FASE_08_COMBATE.md) |
| 9 | Espada: combo de 3 golpes, defesa e defesa perfeita | 🟡 | [FASE_09_ESPADA](02_Combate/FASE_09_ESPADA.md) |
| 10 | Primeiro lobo hostil | 🟡 | [FASE_10_LOBO](02_Combate/FASE_10_LOBO.md) |
| 11 | Matilha com vez de ataque | 🟡 | [FASE_11_MATILHA](02_Combate/FASE_11_MATILHA.md) |
| 12 | Névoa de distância e limites do vale | 👍 ("a névoa está boa") | [FASE_12_NEVOA](03_Mundo/FASE_12_NEVOA.md) |
| 13 | Streaming do terreno com vários jogadores | 🟡 | [FASE_13_STREAMING](03_Mundo/FASE_13_STREAMING.md) |
| 14 | NPCs do vale aparecendo perto dos jogadores | 🟡 | [FASE_14_POPULACAO](03_Mundo/FASE_14_POPULACAO.md) |
| 15 | Dia e noite, lua com fases, estrelas, efeitos do sol | 👍 | [FASE_15_DIA_NOITE](03_Mundo/FASE_15_DIA_NOITE.md) |

### Correções e melhorias pedidas no caminho

| O que é | Situação | Relatório |
|---|---|---|
| Correções iniciais: terreno em DX12, lobo nascendo no chão | 🟡 | [CORRECAO_INICIAL_TERRENO_LOBO_ESPADA](02_Combate/CORRECAO_INICIAL_TERRENO_LOBO_ESPADA.md) |
| Encaixe da espada na mão e câmera de ombro | 🟡 | [CORRECAO_CAMERA_ESPADA](02_Combate/CORRECAO_CAMERA_ESPADA.md) |
| Câmera de mira, troca de ombro (V) e zoom | 🟡 | [CORRECAO_CAMERA_MIRA](02_Combate/CORRECAO_CAMERA_MIRA.md) |
| Lobo: primeira tentativa (registro histórico) | substituída pela linha abaixo | [CORRECAO_LOBO_PARCIAL](02_Combate/CORRECAO_LOBO_PARCIAL.md) |
| Lobo: cores, olhos que brilham, movimento suave | 🟡 | [CORRECAO_LOBO_CORES_MOVIMENTO](02_Combate/CORRECAO_LOBO_CORES_MOVIMENTO.md) |
| Câmera que ficava para trás e herói entrando no lobo | 🟡 | [CORRECAO_CAMERA_SOLTA_COLISAO_LOBO](02_Combate/CORRECAO_CAMERA_SOLTA_COLISAO_LOBO.md) |
| Especial da espada (corte à distância, botão direito), defesa no F, lagos parados, menos borboletas | 🟡 | [ESPECIAL_ESPADA_AGUA_PARADA](02_Combate/ESPECIAL_ESPADA_AGUA_PARADA.md) |
| Revisão das 23 armas: poses e golpes próprios por família, 1ª pessoa com braços, tiros saindo da arma na mira, mana, secundários novos, grimório/chicote/corrente animados, modelos e 97 sons | 🟡 (esperando teste no jogo) | [REVISAO_ARMAS](02_Combate/REVISAO_ARMAS.md) |
| Animações do Mixamo no anão (andar, trote, corrida, disparada, pulo, nado, morte, golpes da espada/martelo/adagas/cajado/arco, esquiva, tranco, sacar, colher), esqueleto do Mixamo (fim dos braços de gorila), barba que segue a cabeça, roda de emotes no B com 12 danças e 12 gestos vistos por todos (30/10) | 🟡 | [ANIMACOES_MIXAMO_EMOTES](02_Combate/ANIMACOES_MIXAMO_EMOTES.md) |
| Azhûr novo (Meshy) com animações fluidas e o soco de cima, a cratera do primeiro Selo (chão de basalto com lava, paredões sem escalada, runas que reacendem), avisos de golpe com runas e onda de choque, cavalo no assobio como no Red Dead e herói sentado na sela (30/10) | 🟡 | [CALDEIRA_AZHUR_NOVO](03_Mundo/CALDEIRA_AZHUR_NOVO.md) |
| Lobo, veado e cavalos com as animações do pacote gratuito da Quaternius (CC0): galope, mordida, bote, comer, tranco e morte passados para os nossos modelos no Blender (30/10) | 🟡 | [ANIMAIS_QUATERNIUS](03_Mundo/ANIMAIS_QUATERNIUS.md) |
| Noite mais clara, lampiões/tochas/santuário/portal mais fracos, névoa roxa de corrupção nos monstros hostis (30/10) | 🟡 | [NOITE_LUZES_NEVOA_ROXA](03_Mundo/NOITE_LUZES_NEVOA_ROXA.md) |
| Armas novas (espada longa, martelo, adagas duplas, cajado, arco com corda na Unreal e flecha nova), heróis fixos por raça (6 modelos no esqueleto do aventureiro, Demônio), slime de gelatina em 4 tipos (jade, brasa, peçonhenta, ferro), golpe que não pegava colado, corpo morto que prendia, marca de alvo removida (30/10) | 🟡 | [ARMAS_HEROIS_SLIME](02_Combate/ARMAS_HEROIS_SLIME.md) |
| Arsenal (392 itens, 9 conjuntos, elementos), painel do item, lojas dos 9 comerciantes do Godot e mapas das caravanas | 👍 arsenal aprovado; lojas e mapas 🟡 | [ARSENAL_E_LOJAS](02_Combate/ARSENAL_E_LOJAS.md) · [continuar](02_Combate/CONTINUAR_ARSENAL.md) |
| Sensibilidade do mouse (`[` e `]`) e cenário aparecendo mais longe | 🟡 | [CORRECAO_CONTROLES_DISTANCIAS](03_Mundo/CORRECAO_CONTROLES_DISTANCIAS.md) |
| Grama realista, nuvens volumétricas e céu | 👍 ("adorei tudo") | [CORRECAO_GRAMA_CEU_REALISTA](03_Mundo/CORRECAO_GRAMA_CEU_REALISTA.md) |
| Água cristalina dos lagos | 👍 (o tremido foi corrigido depois) | [AGUA_CRISTALINA](03_Mundo/AGUA_CRISTALINA.md) |
| Floresta viva: sons, borboletas, sementes, vaga-lumes | 👍 (borboletas reduzidas a pedido) | [FLORESTA_VIVA](03_Mundo/FLORESTA_VIVA.md) |
| HUD com a arte do Godot: herói, minimapa, alvo, barra, dano | 👍 (barra refeita a pedido) | [HUD](04_Interface/HUD.md) |
| Menu num canto vivo do vale (luz do jogo, câmera livre), seleção de raça animada e criação de herói (392 peças, peças próprias por raça, cores por paleta e cor livre HEX) | 🟡 | [MENU_CRIACAO_HEROI](04_Interface/MENU_CRIACAO_HEROI.md) |
| Sons do jogo (passos, golpes, lobo) e músicas do Godot | 🟡 | [SONS_E_MUSICA](03_Mundo/SONS_E_MUSICA.md) |
| Água estilizada: turquesa no raso, opaca no fundo, espuma na margem | 🟡 | [AGUA_ESTILIZADA](03_Mundo/AGUA_ESTILIZADA.md) |
| Cards de raça visíveis de novo, "Cancelar" na criação, cosméticos em 3 lugares (chapéu, rosto, costas) | 🟡 | [MENU_CRIACAO_HEROI](04_Interface/MENU_CRIACAO_HEROI.md) |
| Níveis, XP, atributos valendo no combate, saque, mochila e equipamento; janela do herói (I/Tab) | 🟡 | [JANELA_DO_HEROI](04_Interface/JANELA_DO_HEROI.md) |
| NPC Julia (peças novas: cabelo com pontas azuis, óculos, gargantilha, vestido, meias) com o cachorrinho que segue; aparência de todos os NPCs | 🟡 (aguardando teste no editor) | [NPC_JULIA](03_Mundo/NPC_JULIA.md) |
| Cidade do Santuário (vila inicial do Godot) e a Fonte de Jade (água do Vortex, jatos, cura e renascimento) | 🟡 (aguardando teste no editor) | [CIDADE_DO_SANTUARIO](03_Mundo/CIDADE_DO_SANTUARIO.md) |
| Opções gráficas: shaders (desligados por padrão), água em 4 níveis, gráficos avançados | 🟡 (aguardando teste no editor) | [OPCOES_GRAFICAS](04_Interface/OPCOES_GRAFICAS.md) |
| Multiplayer online pela Epic (sala com código de 5 letras, P2P com relay) | 🟡 (precisa de 2 PCs) | [MULTIPLAYER_EPIC](01_Base/MULTIPLAYER_EPIC.md) |
| Sala de espera até 8 jogadores + herói viajante do convidado | 🟡 (testado na rede local; aguardando teste no editor) | [SALA_DE_ESPERA](01_Base/SALA_DE_ESPERA.md) |
| Desempenho: sol e pisada da grama sem custo por quadro, resolução interna com TSR (75%), contador de FPS; 1080p 61 → 76 FPS | 🟡 | [DESEMPENHO_FPS](03_Mundo/DESEMPENHO_FPS.md) |
| Rosto do morto-vivo: o crânio volta a ficar sobre a cabeça (era uma placa sem cabeça atrás) | 🟡 | [MENU_CRIACAO_HEROI](04_Interface/MENU_CRIACAO_HEROI.md) |
| Os 12 atributos oficiais do Godot (sem vigor); atordoamento e contra-ataque na defesa perfeita; fim do travamento no primeiro poder | 🟡 | [CRIATURAS_MISSOES_ARMAS](03_Mundo/CRIATURAS_MISSOES_ARMAS.md) |
| Criaturas e chefes do Godot (12 espécies, cavalos), história do mundo, 31 missões, diálogo no E, abas Classes, Alquimia e Missões | 🟡 | [CRIATURAS_MISSOES_ARMAS](03_Mundo/CRIATURAS_MISSOES_ARMAS.md) · [HISTORIA_DO_MUNDO](03_Mundo/HISTORIA_DO_MUNDO.md) |
| HUD de missão, grupo e efeitos; mapa do vale no M; volumes de áudio; save automático; desvio de obstáculos; 25 armas refeitas com texturas e o escudo | 🟡 | [CRIATURAS_MISSOES_ARMAS](03_Mundo/CRIATURAS_MISSOES_ARMAS.md) |
| Golpe de perto sem mira (alcance até a borda do monstro), atordoamento de vez em quando, matilha que cerca e ataca em pinça, monstros que rondam | 🟡 | [COMBATE_CHEFE_CLASSES](02_Combate/COMBATE_CHEFE_CLASSES.md) |
| Azhûr refeito no Blender, barra em 3 fases, Caldeira com céu vermelho, lava que queima e chuva de meteoros | 🟡 | [COMBATE_CHEFE_CLASSES](02_Combate/COMBATE_CHEFE_CLASSES.md) |
| Classes do Godot: a cena das 6 cartas no nível 10 (bronze, prata, ouro, diamante) e as 264 habilidades nas teclas 1 a 4 | 🟡 | [COMBATE_CHEFE_CLASSES](02_Combate/COMBATE_CHEFE_CLASSES.md) |
| Soco Sério das manoplas, primeira pessoa (C), nado com animação e respingo, excluir mundos (até 5) | 🟡 | [COMBATE_CHEFE_CLASSES](02_Combate/COMBATE_CHEFE_CLASSES.md) |
| Fim das travadas (criaturas, névoa do mapa, horizonte, lava, sons: pior quadro 686 → 55 ms), miragem da Yggdrasil vista de longe, o nome The World Gêneses no menu e no carregamento, coliseu sem plantas | 🟡 | [DESEMPENHO_MIRAGEM_NOME](03_Mundo/DESEMPENHO_MIRAGEM_NOME.md) |
| Alpha 1.0: instalador (`Tools\PackageAlpha.ps1`), sombras clássicas por padrão (62–79 → 89 FPS no jogo instalado), mais claro sem a luz global, neblina do FogArea nos vales, manoplas certas em primeira pessoa | 🟡 | [ALPHA_1_0](04_Interface/ALPHA_1_0.md) |
| Instalador "Portal" (`Tools\Installer\Portal`): janela própria com o Vórtice animado, os 9 Selos acendendo, a lenda passando e a música do instalador; desinstalador "Selar o portal". Gera `Build\Installer\TheWorldGeneses_Alpha_1.0_Portal.exe` | 🟡 (testado no modo /demo) | `Tools\Installer\Portal\BuildPortal.ps1` |
| Abertura em modo cinema: cada quadro em tela cheia com câmera lenta, verso inteiro na Cinzel (sem a legenda palavra por palavra), relógio real da canção, abre sozinha na 1ª entrada de cada herói e só o PULAR sai | 🟡 (testes `RunIntroShots` e `RunIntroSyncTest` passando) | `Docs/Imagens_Cinema_e_Golpes.pdf` (32 cenas para gerar) |
| Instalador para Linux (`.run` com zenity ou terminal, atalho no menu e Desinstalar.sh): `Tools\PackageAlpha.ps1 -Platform Linux` | 🟡 (não testado num Linux de verdade) | `Tools\Installer\Linux` |
| Lança e alabarda com alcance e cone maiores; chakram com metade do dano por passada (ida e volta) e especial 1,2×; mouse desenhado na barra no lugar de "E" e "D"; 330 ícones novos de habilidades e classes | 🟡 | `Docs/04_Interface/PROMPTS_ICONES_HABILIDADES.md` |
| Desempenho profissional (Nanite fora das malhas de cubos, objetos agrupados por região, sombras clássicas, resolução dinâmica, preset pelo PC: 86 → 116 FPS no Alto), janelas e opções no estilo da HUD, mapa em desenho | 🟡 | [DESEMPENHO_PROFISSIONAL](03_Mundo/DESEMPENHO_PROFISSIONAL.md) |
| Adaga inicial (modelo `fantasy dagger`) no lugar da Espada de Cinzas, ajustada no Blender; golpes 1, 2, 3 e a defesa refeitos (sem salto, lâmina fora das pernas) | 🟡 | [ADAGA_INICIAL](02_Combate/ADAGA_INICIAL.md) |
| 11 bugs achados e corrigidos: rede (o convidado ignorava as correções do servidor), travadas dos bandidos (235 → 49 ms), primeira pessoa, pedra do goblin, avisos do estádio, saves, sons e testes | 🟡 | [BUGS_23_10](03_Mundo/BUGS_23_10.md) |
| Golpes com alcance de verdade (era 2,7× maior), avanço até o alvo, recuo no acerto, câmera de ação atrás do herói com atraso suave, primeira pessoa com a arma embaixo e de lado por família | 🟡 | [CAMERA_GOLPES_PRIMEIRA_PESSOA](02_Combate/CAMERA_GOLPES_PRIMEIRA_PESSOA.md) |
| Iluminação (exposição como o olho, crepúsculo, noite sem azul-neon, neve sem estourar, sombras de contato) e vegetação (manchas de tom no terreno e nas árvores, cada árvore com o seu tom, copas somem perto da câmera) sem perder FPS | 🟡 | [ILUMINACAO_VEGETACAO](03_Mundo/ILUMINACAO_VEGETACAO.md) |
| Marcação de foco no alvo (sem andar sozinho, opção para desligar), névoa densa escondendo a borda do mundo, monstros que revidam no combo e morcego que volta a voar, números de dano em desenho animado, menu novo (Jogar / Multiplayer / Configurações / Conquistas / Sair; mundos de um jogador com 1 herói e de multiplayer com até 3) | 🟡 | [FOCO_NEVOA_MONSTROS_MENU](04_Interface/FOCO_NEVOA_MONSTROS_MENU.md) |
| Barra do alto só nos chefes, placas de vida animadas, troca de foco virando o mouse, chat de comandos no T (/god, /fly, /level, /tp por região...), letreiro do lugar ao entrar numa região ou cidade, 12 ícones novos do menu | 🟡 | [BARRAS_FOCO_CHAT_LETREIRO](04_Interface/BARRAS_FOCO_CHAT_LETREIRO.md), [PROMPTS_ICONES_CHATGPT](04_Interface/PROMPTS_ICONES_CHATGPT.md) |
| Crítico com explosão vermelha e "CRÍTICO!"; monstros do começo nível 1 (curva de nível nova e postos de fora das muralhas) | 🟡 | [CRITICO_E_NIVEIS_DO_COMECO](02_Combate/CRITICO_E_NIVEIS_DO_COMECO.md) |
| Estádio do Despertar: as 23 armas num anel dentro da arena (4 grupos coloridos, sem corredores), armas mais baixas com nome em cima, cartão da arma em desenho animado | 🟡 | [ESTADIO_DO_DESPERTAR](02_Combate/ESTADIO_DO_DESPERTAR.md) |
| Morcego voando na altura da cabeça do herói (e sem afundar fora do vale) | ✅ | [FOCO_NEVOA_MONSTROS_MENU](04_Interface/FOCO_NEVOA_MONSTROS_MENU.md) |
| Prompts das imagens dos monstros para virar 3D (poses para rig) e bestiário novo: comuns, noturnos, semi-chefes e chefes das 9 regiões, noturnos do continente e 3 dungeons de portal | 🟡 | [PROMPTS_MONSTROS_3D](03_Mundo/PROMPTS_MONSTROS_3D.md) |
| Fauna por região: coelhos, cervos e cavalos com quantidade, mistura, manada e pelagem de cada bioma (nenhum na terra dos mortos); prompts de 23 animais novos | 🟡 | [PROMPTS_MONSTROS_3D](03_Mundo/PROMPTS_MONSTROS_3D.md) |
| Shaders mais leves (copas sem preto, brilho e desfoque suaves), 3 goblins (brutamontes, comum, ladino), monstros da noite, semi-chefes "Ancestral" das 9 regiões, letreiro mais alto, mata à noite mais clara, avisos de rede de 818 para 30, fotos das páginas Multiplayer e Conquistas | 🟡 | [SHADERS_GOBLINS_NOTURNOS_SEMICHEFES](03_Mundo/SHADERS_GOBLINS_NOTURNOS_SEMICHEFES.md) |
| Cliques nas janelas não viram mais ataque (o chat cobria a tela), habilidades da classe liberam nos níveis 10/15/20/25 com cadeado, água rasa com ondas, gotas, som, tchibum e passo mais lento; opinião sobre as telas | 🟡 | [CLIQUES_HABILIDADES_AGUA](04_Interface/CLIQUES_HABILIDADES_AGUA.md) |
| Janelas do herói e loja refeitas com o kit "Aventura" (madeira, pedra, fitas, letra cartoon) e herói ao vivo que gira | ⚪ substituído | [JANELAS_AVENTURA](04_Interface/JANELAS_AVENTURA.md) |
| Janelas cartoon: azul vivo e cantos redondos, botões de borracha, estrelinhas nos cliques, janela que pula, "+1" e sons cartoon (Kenney CC0 + sintetizados); depois menu, pausa, opções, conversa e janelas de jogador no mesmo estilo | 🟡 | [JANELAS_CARTOON](04_Interface/JANELAS_CARTOON.md) |
| Bandidos terminados: roupas corrigidas, baú guardado em cada acampamento (só abre com o bando vencido), teste na suíte; lobos em matilha de 1 a 3 com alfa maior | 🟡 | [BANDIDOS](02_Combate/BANDIDOS.md) |
| Mundo com mais coisas: ruínas, torres, santuários (bênçãos), minas, cemitérios (mortos-vivos), caravanas e cais; portais de dungeon (cor e tamanho pela dificuldade, vermelho raro e sem saída); aranhas que escalam e descem no fio, esqueletos, teias que prendem | 🟡 | [PONTOS_DE_INTERESSE_E_PORTAL](03_Mundo/PONTOS_DE_INTERESSE_E_PORTAL.md) |
| Fauna e flora: pássaros por região (no céu e no chão, fogem do herói), peixes que nadam, fogem e pulam, vitórias-régias e algas, flores de vários tamanhos e cores, dormideira que se fecha, plantas mágicas que acendem, esporos de cogumelo; janela do herói sem piscar ao gastar pontos; aranhas fora do mundo aberto por enquanto | 🟡 | [FAUNA_E_FLORA](03_Mundo/FAUNA_E_FLORA.md) |
| Sons refeitos: as gravações reais do Godot com 4 variações (golpes, passos, magias, chefe, vozes de cada bicho), ambientes gravados (SONNISS) por região, pios/corvo/asas de gravações CC0, volume igual por categoria, sem silêncio no começo, distância por tipo, sem empilhar; créditos no jogo; prévia de antes e depois | 🟡 | [SONS_GRAVADOS_E_MIXAGEM](03_Mundo/SONS_GRAVADOS_E_MIXAGEM.md) |
| HUD cartoon (cartão do herói, bússola em fita, barra de chefe, missão, avisos, barra de habilidades), minimapa nítido (1 ponto por bloco, gira com a câmera) e mapa do mundo em tela cheia (arrastar, zoom no mouse, detalhe por bloco, nuvens no inexplorado, marcador, filtros) | 🟡 | [HUD_CARTOON_MINIMAPA_MAPA](04_Interface/HUD_CARTOON_MINIMAPA_MAPA.md) |
| Revisão das 264 habilidades de classe: pressa que zerava as recargas, alcance igual ao do texto, zonas na mira, bênçãos "só sua", nomes, ícones recortados de novo e escolhidos por habilidade, 8 gestos do Godot (com a variante de cada arma); teste que lança todas | 🟡 | [REVISAO_HABILIDADES](02_Combate/REVISAO_HABILIDADES.md) |
| HQ de abertura "Acorde": 24 quadros em 2 folhas que pintam, câmera de quadro em quadro no tempo da canção, legenda com os versos, clarão, folha virando e o "ACORDE" final; abre no primeiro despertar e pela pausa (A HISTÓRIA) | 🟡 | [ABERTURA_QUADRINHOS](03_Mundo/ABERTURA_QUADRINHOS.md) |

### O que dá para fazer no jogo hoje

| Ação | Tecla |
|---|---|
| Andar | W A S D |
| Correr | Shift |
| Pular | Espaço |
| Esquiva (5 cargas) | Ctrl |
| Combo da espada | Botão esquerdo |
| Especial da arma (espada: corte à distância) | Botão direito |
| Defesa e mira por cima do ombro | Segurar F |
| Recuperar vida (custa 10 de mana) | H |
| Habilidades da classe (depois do Despertar, no nível 10) | 1 2 3 4 |
| Poções de vida e de mana (da mochila) | Q / R |
| Janela do herói: Herói, Mochila, Técnicas, Classes, Alquimia, Missões | I ou Tab (abas também em 1 a 6 com a janela aberta) |
| Falar com um morador, interagir com outro jogador (grupo, troca, duelo, amizade), abrir baú, colher, dar fruta ao cavalo, montar e desmontar | E |
| Assobiar para o cavalo | X |
| Marcar ponto para o grupo (numa criatura: "perigo") | Z |
| Mapa do vale (descoberto aos poucos; roda do mouse dá zoom) | M |
| Trocar o ombro da câmera | V |
| Primeira pessoa (liga e desliga) | C |
| Zoom | Roda do mouse |
| Sensibilidade do mouse | `[` e `]` |

Comandos de console:

| Comando | O que faz |
|---|---|
| `Vortex.Hora 20` | Muda a hora do mundo |
| `Vortex.Tempo.Escala 60` | Acelera o relógio |
| `Vortex.HUD 0` / `1` | Esconde ou mostra a HUD |
| `Vortex.Mouse.Sensibilidade` | Ajusta a sensibilidade do mouse |

Só durante o desenvolvimento: teclado numérico 1 a 7 troca as roupas, F9 mostra o rosto.

---

## 2. O que espera o seu teste no editor

Nada com 🟡 ou 👍 é declarado concluído antes de você jogar. O que mais vale conferir:

- **Combate (Fases 7 a 11):** combo, defesa no F, defesa perfeita, recuperar no H, o especial no botão direito, um lobo e uma matilha (3 a 7 lobos). Teste também como cliente.
- **Lobo:** cores, olhos, patas nos degraus e se a perseguição ficou difícil demais.
- **Streaming (Fase 13):** dois jogadores (Listen Server) andando para longe um do outro; o segundo não pode cair nem ver buracos.
- **NPCs (Fase 14):** a praça perto de (0, 15 m); Lary e Juh passeando; sair a mais de 80 m e voltar.
- **Água:** se o lago parou de tremer ao girar a câmera.
- **HUD:** a barra nova, o retrato, o minimapa e a barra do lobo em luta.
- **Travadas, miragem e nome (10/10):** andar e galopar sem as paradas de meio segundo, a Yggdrasil como miragem no horizonte, o menu e o carregamento com o nome novo e o coliseu limpo. A lista está em [DESEMPENHO_MIRAGEM_NOME](03_Mundo/DESEMPENHO_MIRAGEM_NOME.md).
- **Lava, níveis, morte, multiplayer e janelas (10/10):** lava em desenho com correnteza e fumaça, fumaça do vulcão, nível das criaturas pela distância com placa colorida pela dificuldade, tela de morte, tela de carregamento, grupo (8), troca, duelo e amizade, e as janelas no estilo da HUD. A lista está em [LAVA_GRUPO_INTERFACE](04_Interface/LAVA_GRUPO_INTERFACE.md).
- **Continente (10/10):** sair do vale para qualquer lado (não há mais borda), as nove regiões, as emendas a leste e a oeste e o mapa do continente. A lista do que conferir está em [CONTINENTE](03_Mundo/CONTINENTE.md).
- **Etapa de 10/10 (2ª parte):** HUD sem piscar, retrato fixo, bússola, marcações no Z, mapa com névoa e zoom, Azhûr com mais detalhe, armas em primeira pessoa, rastro dos golpes, coletáveis e o cavalo (domar, montar, assobio). A lista do que conferir está em [COLETAVEIS_CAVALO_HUD](03_Mundo/COLETAVEIS_CAVALO_HUD.md).
- **Etapa de 10/10:** golpe de perto sem mira, matilha, Azhûr e a Caldeira, a cena das cartas, as habilidades de classe, Soco Sério, primeira pessoa, nado e excluir mundos. A lista do que conferir está em [COMBATE_CHEFE_CLASSES](02_Combate/COMBATE_CHEFE_CLASSES.md).
- **Etapa de 09/10:** atributos, defesa perfeita, criaturas, missões, diálogo, mapa no M, armas novas e volumes de áudio. A lista completa do que conferir está em [CRIATURAS_MISSOES_ARMAS](03_Mundo/CRIATURAS_MISSOES_ARMAS.md).

---

## 3. O que falta

### 3.1 Próximas etapas, na ordem combinada

| # | Etapa | O que entra | Situação |
|---|---|---|---|
| 1 | **Continente** | Primeira parte pronta ([CONTINENTE](03_Mundo/CONTINENTE.md)): o mundo inteiro do Godot em volta do vale (7,5 × 7,5 km, mar sem fim), as 9 regiões com chão, árvores, enfeites e criaturas próprios, rios e lagos do mapa, o mapa do continente com névoa, rios da nascente ao mar com vales, montanhas e o vulcão no horizonte, pontes e cabanas com baú, a Yggdrasil com a aldeia élfica, as vilas das Dunas e dos Caçadores e os covis de Seryth e Thorun. A fumaça do vulcão e a miragem da Yggdrasil também já existem. Falta: moradores das vilas regionais e as células de lava de Brasafenda desenhadas no chão | 🟡 em andamento |
| 2 | **Fase 16 — a cidade** | Praça, portões norte e sul, muralhas e torres, a pousada "Taverna do Javali Dourado", casas, lojas, sino, postes, árvores e objetos (`town_*.glb` do Godot); som da cidade (`amb_town` já importado) | ⬜ |
| 3 | **Fases 17 a 19** | Os cartões da introdução e a revelação da cidade; percurso completo jogável (diálogo e missões já existem desde 09/10) | ⬜ |
| 4 | **Fases 20 a 22** | Anfitrião com 2 clientes, save e reconexão, perfil de desempenho e comparação com o Godot (o "vertical slice") | ⬜ |
| 5 | **Fase 23** | Conteúdo em escala (o resto do jogo, por área), só depois do slice aprovado | ⬜ |

### 3.2 Sistemas que ainda não existem

Hoje eles aparecem vazios ou provisórios no jogo:

- **Escolha da arma inicial (o Despertar do Godot):** precisa das outras famílias de arma (arco, lança, martelo, cajado) com golpes de verdade; hoje só a espada tem.
- **Ascensões do nível 50 e classes únicas:** as habilidades da classe (1 a 4) já funcionam; a escolha da Ascensão ainda não. As técnicas da espada dos níveis 7 e 10 (Escapada, Arco de Cinzas) também faltam.
- **Outras armas e seus especiais:** os 25 modelos novos já estão na mão, mas só a espada tem golpes próprios.
- **Lojas** (Ilsa, Ducci, Garrick): o diálogo avisa que chegam em breve.
- **Buffs de verdade:** hoje a HUD mostra só os estados que existem (contra-ataque, Fonte, cidade).
- **Bandidos** do Godot.

### 3.3 Ajustes e riscos conhecidos

Os detalhes estão em cada relatório.

- **Fonte da HUD:** é a Roboto da engine. A Cinzel, livre, deixaria igual à arte do Godot; é só pedir para baixar.
- **Horizonte visto do alto** de morros: aparece uma linha suave entre o chão distante e o céu. O continente resolve isso.
- **Amanhecer:** às 06:12 ainda fica escuro dentro da mata.
- **Noite sob as árvores:** mais legível desde 23/10 (exposição em faixa e luar mais forte), mas a mata fechada ainda é escura; ver [ILUMINACAO_VEGETACAO](03_Mundo/ILUMINACAO_VEGETACAO.md).
- **Matilha em mata fechada:** os pontos de volta podem cair atrás de árvores; falta medir com 7 lobos no mundo aberto.
- **Memória em sessões longas** com jogadores viajando: ainda não foi medida.
- **Especial em rede:** ainda não foi testado com um cliente de verdade.
- **Desvio de obstáculos** usa sensores (sem NavMesh): em mata muito fechada um lobo ainda pode dar voltas antes de achar o caminho.
- **Golpes com as armas novas:** a pegada segue o Godot, mas cada golpe só foi conferido por fotos.
- **Pisada da grama com vários personagens** ao mesmo tempo: não foi conferida.

---

## 4. Regras do trabalho

- O projeto Godot (`the-world-geneses`) e os saves são **só lidos**, nunca alterados.
- `_Externos/CubeWorld-reference-nao-licenciado/` não é usado.
- `Config/DefaultInput.ini` foi modificado pelo editor antes deste trabalho: fica preservado e fora dos commits.
- Cada fase ou correção tem um commit e um relatório aqui, com o que mudou, por quê, testes, desempenho, riscos e como voltar atrás.
- Uma fase só é declarada concluída depois do seu teste no editor.

## 5. Testes automáticos

Ficam em `Tools/` e rodam no PowerShell com o editor fechado.

| Área | Scripts |
|---|---|
| Movimento | `RunMovementSmoke` (`-StepProbe`, `-Matrix`), `RunClientInputSmoke` (`-LagMs`, `-LossPct`), `RunMovementNetworkSmoke`, `RunDodgeDirectionSmoke`, `RunSolidWalkSmoke`, `RunMoveBench` |
| Base | `RunFrameworkNetworkSmoke`, `RunSaveSmoke` |
| Combate | `RunCombatSmoke`, `RunSwordSmoke`, `RunSwordShot`, `RunWeaponShot`, `RunWeaponCloseUp`, `RunCreatureSmoke` (criaturas, missões e fotos da interface), `RunFeatureShots` (golpe de perto, Soco Sério, primeira pessoa, nado), `RunBossShots` (Azhûr e a Caldeira), `RunClassShots` (cartas e habilidades), `RunWolfSmoke`, `RunWolfNetworkSmoke`, `RunWolfMotionProbe` (`-Fps`), `RunWolfVisualShot`, `RunPackSmoke` |
| Câmera | `RunCameraSmoke`, `RunCameraShot`, `RunCameraSequenceShot` |
| Mundo | `RunTerrainBench` (`-Profile alto\|leve\|lumen`, `-Hour`, `-Hud`, `-Shot`), `RunRoamBench` (passeio a galope com o tempo de cada sistema), `RunRegionSmoke`, `RunFogShot`, `RunStreamNetworkSmoke` (`-Listen`, `-Jumps`), `RunPopulationSmoke` (`-Network`, `-Shot`), `RunDayNightShot` (`-Only`, `-Extra`), `RunDayNightSmoke`, `RunGrassShot` |
| Interface | `RunHudShot` (inclui o teste de RPG), `RunFrontEndShot`, `RunWardrobeShot`, `RunHeroSmoke`, `RunMenuFlowSmoke`, `RunMenuNetworkSmoke` |
| Epic online (precisa de internet) | `RunOnlineProbe` |

**Desempenho de referência** (`RunTerrainBench -Profile alto -Hour 12`, 1280×720): cerca de 65 a 70 FPS. A HUD custa cerca de 1 FPS. No passeio (`RunRoamBench`), depois da carga inicial: pior quadro 55 ms, pior 1% em 32 FPS.
