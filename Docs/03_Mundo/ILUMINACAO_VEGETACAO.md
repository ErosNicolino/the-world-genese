# Iluminação, vegetação e texturas do terreno (23/10)

## Motivo

O usuário pediu uma iluminação melhor (sol, céu, névoa, exposição, sombras, dia e noite), uma vegetação melhor (grama, árvores, floresta) e texturas melhores no terreno e nos objetos, sem derrubar o FPS. Partiu do que já estava aprovado: [FASE_15_DIA_NOITE](FASE_15_DIA_NOITE.md), [CORRECAO_GRAMA_CEU_REALISTA](CORRECAO_GRAMA_CEU_REALISTA.md) ("adorei tudo") e [FASE_12_NEVOA](FASE_12_NEVOA.md) ("a névoa está boa").

O estilo voxel foi mantido: as melhorias trabalham por cima dos atlas de blocos, sem trocar por texturas realistas. Nenhuma textura de fora foi baixada; o ruído usado foi gerado no projeto.

## O que estava ruim (fotos de antes)

- **18:36 quase preto:** com o sol logo abaixo do horizonte, a luz ambiente só existia numa janela estreita, e o chão ficava preto embaixo de um céu ainda claro.
- **Noite (21:00, 22:00) azul-neon:** árvores pretas, areia e água num azul saturado; dentro da mata não se via nada (risco conhecido do painel).
- **Hora dourada (16:30, 17:42):** a exposição abria 2× e tudo ficava amarelo, as copas cor de limão.
- **Meio-dia:** copas e grama saturadas demais.
- **Neve (Nivaluz):** estourada de branco, porque a exposição era totalmente fixa.
- **Terreno:** cada bloco de grama igual ao vizinho em todo o vale; de longe, a repetição aparecia.
- **Floresta:** a mesma árvore repetida, com o mesmo tom.
- **Câmera nas copas:** a tela ficava toda de blocos de folha (bug 4 de [BUGS_23_10](BUGS_23_10.md)).

## O que mudou

### Iluminação (`UVortexDayNightSubsystem`)

- **Exposição como o olho, numa faixa estreita** em volta do valor calibrado (0,5):
  - o mínimo continua o calibrado, então cenas escuras ficam como antes e a noite não vira dia;
  - cenas muito claras (neve, areia ao meio-dia) escurecem até 1,8×;
  - no crepúsculo e à noite, o céu claro na tela não escurece o resto (a faixa quase fecha).
- **A exposição abre um pouco:**
  - na hora dourada, até 1,55× (era 2×);
  - no crepúsculo logo depois do pôr do sol;
  - à noite (+0,85 EV, o olho acostumado ao escuro).
- **Crepúsculo:** a luz ambiente vale numa janela bem mais larga em volta do pôr do sol, e o reforço passou de 2,5 para 4.
- **Noite:**
  - luar, luz ambiente e névoa noturna menos saturados (sem o azul-neon);
  - o luar ficou um pouco mais forte (0,32 do sol na lua cheia; era 0,27).
- **Cor:** saturação de 0,96 (0,90 na hora dourada). As copas deixam de ficar verde-limão.
- **Sombras de contato** no sol (na tela, baratas): pés, pedras e troncos "assentam" no chão.

### Vegetação e terreno (`Tools/ImproveVegetationMaterials.py`)

- **Manchas de tom em escala grande** no terreno (`M_TerrainTiles`) e em todos os objetos (`M_VoxelProp`):
  - partes mais claras e quentes e partes mais escuras e frias, em manchas de ~40 m e ~9 m;
  - vêm de um ruído contínuo gerado no projeto (`Tools/Blender/MakeMacroNoise.py` → `Data/Textures/T_MacroNoise.png`) lido pela posição no mundo.
- **Cada cópia com o seu tom:** árvores, arbustos, flores e pedras instanciados variam um pouco de brilho e de tom (`PerInstanceRandom`). Uma floresta de carvalhos deixa de ser o mesmo carvalho repetido.
- **Copas e arbustos perto da câmera somem com pontilhado:** 25 materiais `MI_tree_*` e `MI_bush_*` passam para `M_VoxelPropFade`. A partir de 1 m da câmera eles vão sumindo e somem por completo a 2,5 m. A sombra deles não some.
- Rodar o script de novo não acumula efeito (o nó final é marcado e refeito).

## Fotos (mesmas vistas; `RunDayNightShot`, `RunWorldShots`, `RunGrassShot`)

| | Antes | Depois |
|---|---|---|
| Dia inteiro (06:12, 08:00, 12:00, 16:30, 17:42, 18:36, 21:00, 01:00, floresta às 22h) | [folha](../Images/iluminacao/folha_antes.png) | [folha](../Images/iluminacao/folha_depois.png) |
| 17:42, 18:36, 21:00 lado a lado (em cima antes, embaixo depois) | [folha](../Images/iluminacao/folha_crepusculo_antes_depois.png) | |
| Neve (Nivaluz), Solcânion e Brasafenda (antes à esquerda) | [folha](../Images/iluminacao/folha_regioes_antes_depois.png) | |
| Meio-dia, campo aberto e floresta (em cima antes, embaixo depois) | [folha](../Images/vegetacao/folha_antes_depois.png) | |

Cada hora também está em `Docs/Images/iluminacao/antes_*.png` e `Docs/Images/vegetacao/depois_*.png`. A neve sozinha: [antes](../Images/iluminacao/antes_regiao_nivaluz.png) e [depois](../Images/iluminacao/depois_regiao_nivaluz.png).

## Desempenho (720p, Alto, sem limite de FPS)

| Medida | Antes | Depois |
|---|---|---|
| Passeio na mata (`RunTerrainBench -Profile alto -Hour 12`), só a iluminação nova | 155,1 FPS | 148,0 FPS |
| Mesmo passeio, depois da vegetação | 148,0 FPS | **148,8 FPS** |
| Passeio a galope (`RunRoamBench`), média | 128 FPS | **135 FPS** |
| Passeio a galope, pior quadro depois da carga | 235 ms | **43 ms** |

- As melhorias de vegetação não custaram FPS: uma ou duas leituras de textura a mais por pixel.
- A diferença de 155 → 148 no passeio na mata apareceu entre medições de dias diferentes, com o PC mais ocupado (o editor ficou aberto em parte das medições). As sombras de contato são a única peça nova de iluminação com custo de placa de vídeo, e ele é pequeno.
- O pior quadro do passeio na mata (~230 ms) é a carga inicial, igual antes e depois.

## Riscos

- **Pontilhado das copas:** usa material mascarado só em árvores e arbustos. Em placas fracas pode custar um pouco, e o sumiço ainda não foi visto jogando. As duas fotos em que a câmera "entrava na vegetação" eram a parede da cabana e um barranco, que continuam entrando.
- **Noite:** ficou mais legível, mas dentro da mata fechada ainda é escura. Se quiser mais claro, aumente o fator `Night` (0,55) em `VortexDayNightSubsystem.cpp`.
- **Exposição em faixa:** em cenas muito claras ela agora se adapta em ~1 s. Pode dar uma "respirada" ao sair da sombra para a areia, como num olho.
- **Arquivo alterado por você:** `VortexDayNightSubsystem.cpp` já tinha mudanças suas não commitadas (reforço sem GI e brilho). As minhas foram feitas por cima, sem mexer nas suas.

## Para você testar jogando

- **Passar um dia inteiro** (`Vortex.Tempo.Escala 60`): o pôr do sol, a "hora azul" e a noite com e sem lua.
- **Visitar a região de neve e o deserto** ao meio-dia.
- **Andar dentro da mata com zoom curto:** as copas devem sumir em pontilhado quando a câmera encosta.
- **Olhar o vale de cima:** os tons da grama e das árvores devem variar em manchas, sem formar padrões.

## Reversão

- **Iluminação:** reverter `VortexDayNightSubsystem.cpp` aos números anteriores (os seus ajustes não commitados estão no mesmo arquivo).
- **Materiais:** os assets `M_TerrainTiles`, `M_VoxelProp`, `M_VoxelPropFade` e `MI_tree_*`/`MI_bush_*` estão no git, então dá para voltar com `git checkout` neles. Também dá para ligar a cor dos dois materiais de novo direto ao nó de multiplicação original e devolver os `MI_tree_*`/`MI_bush_*` ao pai `M_VoxelProp`.
