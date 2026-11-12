# Inventário de `VortexReferencia/Content` e onde cada pacote pode servir

Levantamento de 05/10/2026, feito só por leitura: nada foi alterado no projeto de referência. Os nomes indicam pacotes gratuitos do Fab. Antes de copiar qualquer asset para o Vortex, é preciso confirmar a licença de cada pacote. Até agora, só um pacote ajudou a decidir algo no Vortex: o GoodSky, cujas texturas foram exportadas temporariamente para comparar, sem nada copiado (ver abaixo).

**Atenção — esqueleto:** os pacotes de animação usam o manequim da Unreal (Manny/Quinn), enquanto o herói do Vortex usa o esqueleto do aventureiro do Godot. Usar essas animações exige redirecionamento (IK Retargeter) e conferência pose a pose.

## Céu, luz e atmosfera

| Pacote | Conteúdo | Uso no Vortex |
|---|---|---|
| **GoodSky** (15 MB) | Cúpula de céu pintada: 6 panoramas de nuvem (cúmulo, cirro, nublado, faixa do horizonte), textura do zênite, lua, estrelas, tempestade com função de luz de relâmpago, curvas de cor por hora | A direção de arte do jogo é mundo em blocos com natureza realista; as nuvens agora são as **volumétricas da Unreal**. Fica como referência para **clima**: céu nublado e tempestade (relâmpago como função de luz), e talvez uma camada alta e fraca de cirros |
| **FogArea** (28 MB) | Volumes locais de névoa com ruído, sombras e feixes | Referência da Fase 12. Futuro: névoa localizada em pântanos, cavernas, a Caldeira e a "bruma" dos lobos |
| **PWL_Light_Manager** (235 MB) | Gerenciador de luz com dados por estação: sol, céu, atmosfera, nuvens volumétricas, névoa, exposição e tonemapper | Ideia para **regiões e estações**: uma tabela de luz por região (como `RegionFog` e `Caldera` no `DayNight.cs` do Godot) |
| **100_Luts_Pack_V2** (17 MB) | 100 LUTs de cor ("Adventure", "Afternoon"…) | **Correção de cor** por região ou momento (Caldeira, noite, interior da pousada); barato e reversível |

## Efeitos visuais

| Pacote | Conteúdo | Uso no Vortex |
|---|---|---|
| **Vefects** (655 MB, 19 mapas) | Golpes, dash, impacto, "impact frames", tremor de câmera, fogo, gosma | **Combate (Fases 8–9):** brilho de acerto, rastro da esquiva, pausa de impacto, tremor no golpe forte. É o pacote mais útil para a sensação do combate |
| **Basic_VFX** (33 MB) | Materiais de energia, eletricidade, veneno, anéis | Habilidades e status (veneno, eletricidade) |
| **_SplineVFX / SplineParticles** (75 + 67 MB) | Efeitos ao longo de curvas: raio, fogo, gelo, sagrado, vento, borboletas, moedas, bolhas | Magias em feixe; trilhas de vento; moedas voando até o jogador no loot |
| **Realistic_Starter_VFX_Pack_Vol2** (111 MB) | Sangue, bolhas, **borboletas**, **dente-de-leão** | **Floresta viva:** borboletas e sementes de dente-de-leão no vento (o estilo precisa ser simplificado) |
| **Downwash_Dust_FX** (5 MB) | Poeira no chão e rastro de fumaça | Poeira na esquiva, na aterrissagem e no salto do lobo |
| **ArrowTrail** (70 MB) | Rastros de flecha (natureza, gelo, sagrado) | Arco do Godot (`PlayerController.Bow.cs`), quando for migrado |
| **FreeParticle_SoftTofu** (11 MB) | Brilhos suaves | Itens no chão, vaga-lumes à noite |

## Animação (precisa de redirecionamento)

| Pacote | Conteúdo | Uso no Vortex |
|---|---|---|
| **DynamicFalling** (68 MB) | Rolamentos com root motion em 8 direções | Referência para a esquiva e para queda ou rolamento após queda alta |
| **SpearAnimationsV2** (76 MB) | Lança: golpes, estocadas, movimentação, ABP com IK | Arma de lança, se entrar no jogo |
| **FreeAnimationLibrary / FreeAnimationsPack** (188 + 497 MB) | Sentar, abrir e chutar porta, conversar, comer, levitar, chutes | **NPCs e cidade (Fases 16–18):** moradores sentados, conversando e comendo; abrir a porta da pousada |
| **DeadBodies_Poses_nikoff** (148 MB) | Poses de corpo caído | Poses finais de morte de NPC e jogador |
| **Attaku** | Poses de dedos | Mãos segurando objetos (detalhe) |
| **Locodrome** | Ferramenta de teste de locomoção | Inspiração para um mapa de teste de locomoção |
| **SampleAnimationPack** (496 MB) | Fuzil (mira, andar) | Pouco útil: o jogo não tem armas de fogo |
| **MC_Sample** (160 MB) | Exemplo de personagem com luvas (texturas PBR) | Pouco útil para o estilo voxel |

## Som e música (fase de áudio)

| Pacote | Conteúdo | Uso no Vortex |
|---|---|---|
| **SmallSoundKit** (127 MB) | Ambientes: floresta, rio com pássaros, deserto, caverna, masmorra | **Ambiente por região e hora**: floresta de dia, rio no lago; dunas e Caldeira depois |
| **Free_Sounds_Pack** (66 MB) | Pássaros, chuva, riacho, vento, forja, bolsa, livro | Chuva e vento (clima), forja do Bram, sons de inventário |
| **Essential_Foosteps_SK / AutoFootsteps / A_Surface_Footstep** (49 + 5 + 351 MB) | Passos por superfície (concreto, água, madeira…), notifies automáticos, demonstração com material físico | **Passos do herói, NPCs e lobo** por tipo de chão (grama, areia, pedra, água), usando a camada de cada célula do terreno |
| **FreeModularMagicSFX** (97 MB) | Canto, conjuração, feixe, impacto, cura, barreira | Habilidades do GAS (cura, barreira, projéteis) |
| **FreeAtmosGameMusic** (331 MB) | Músicas: floresta encantada, oriental, faroeste, nativa | Trilha da floresta e da cidade, para avaliar com o jogador |

## Próximos usos sugeridos, em ordem

1. **Floresta viva** (continuação da Fase 15): borboletas e sementes ao vento, vaga-lumes à noite e o ambiente sonoro da floresta (SmallSoundKit), trocando com a hora.
2. **Passos por superfície** para o herói, os NPCs e o lobo.
3. **Combate:** acertos, esquiva e impactos com Vefects e Downwash.
4. **Clima:** chuva, vento e tempestade com relâmpago (GoodSky + Free_Sounds_Pack).
5. **Cidade:** animações de NPC redirecionadas e LUT da pousada.
