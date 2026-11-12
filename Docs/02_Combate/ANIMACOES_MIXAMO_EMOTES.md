# Animações do Mixamo no anão e no humano, barba que segue a cabeça e roda de emotes (30–31/10)

Situação: 🟡 humano e anão com o esqueleto completo do Mixamo (31/10), com fotos e testes passando; espera o seu teste
no jogo. As outras quatro raças estão trancadas.

## 31/10: o Mixamo como veio (substitui o "Como funciona" abaixo)

**Por que estava feio:** os clipes eram copiados para o esqueleto do aventureiro, que tem só 15 ossos.
- A coluna inteira virava um bloco, e a clavícula, a ponta do pé e os dedos sumiam.
- Cada osso copiava a direção, mas não o giro em volta de si: cotovelo, mão e joelho tortos.
- O jogo ainda mexia por cima: abria o braço, empurrava o ombro, inclinava o tronco e forçava a velocidade do clipe.

**Agora:**
1. **Blender (`Tools/Blender/MixamoToHero.py <raiz> <raça> [--clips anao] [--beard]`):**
   - a malha, o esqueleto (33 ossos) e os pesos vêm do FBX "With Skin" do Mixamo, no tamanho do herói antigo;
   - cada clipe só copia o giro que o Mixamo deu, osso por osso (o mesmo esqueleto, nada é convertido);
   - os cortes e os tempos dos golpes continuam, e o deslocamento para a frente sai;
   - conferido no Blender: o FBX original e o convertido ficam idênticos no mesmo quadro;
   - ossos extras sem peso, com os nomes que o jogo usa (`Hand_R`, `Socket_Weapon_R`, `Sword`, `Hand_L`, `Torso`,
     `Sheath`, `Head`, `Hips`, `Foot_L/R`), no mesmo lugar de antes em relação à mão: a arma pega igual;
   - `--beard` (anão): a barba e as tranças passam para a cabeça (o Mixamo prendia no peito);
   - sai `Data/Heroes/<raça>_Mixamo/Hero_<raça>.glb` e `clips.json`.
2. **Unreal (`Tools/ImportMixamoHeroes.py`, editor fechado):** `Hero_<raça>` com esqueleto próprio
   (`Hero_<raça>_Skeleton`, os ossos chegam como `mixamorig_*`) e os clipes `Anims/<raça>/MX_<raça>_<clipe>`.
   - A taxa dos clipes que andam ficou entre 0,8 e 1,35 (antes ia até 1,6).
   - O `ImportHeroes.py` e o `ImportHeroAnims.py` pulam as raças que têm a pasta `_Mixamo`.
3. **Jogo:**
   - `UVortexAnimInstance::Fits`: clipes de outro esqueleto (os do aventureiro: posturas de arma, habilidades, beber)
     não tocam no herói do Mixamo. As habilidades usam o golpe mais forte da arma, e beber usa o clipe `Beber`.
   - O tronco dos golpes começa em `mixamorig_Spine`, e a cabeça escondida em primeira pessoa é a do Mixamo.
   - O herói do Mixamo não abre o braço nem empurra o ombro; o quadril do Mixamo vem da animação.
4. **Antigos guardados:** os assets antigos do humano e do anão (malha, corpo sem cabeça e clipes no esqueleto do
   aventureiro) saíram de `Content` e estão no scratchpad da sessão (`ue_herois_antigos`).

**Humano:** usa os 13 clipes dele, e o resto vem do anão (74 no total). Ele não tem um Idle próprio: o parado é o do
anão, com as mãos juntas na frente. Baixar um Idle com o humano no Mixamo resolve.

**Mãos cruzadas (corrigido em 31/10):** os esqueletos do Mixamo não descansam igual de um arquivo para outro. O do
humano tem os braços ~45° girados em relação ao do anão; os FBX "sem pele" do anão diferem do "com pele" em 13-22° no
antebraço e na mão. Copiar o giro relativo ao descanso de cada osso cruzava as mãos. Agora cada osso segue no mundo o osso
do clipe, descontando a diferença entre as direções de descanso; no mesmo esqueleto o resultado é idêntico ao Mixamo.

**Pescoço torto e braço para dentro (corrigido em 01/11):** cada osso era alinhado só pela direção dele e ficava livre
para girar em volta dela. O peito (`Spine2`, que tem o pescoço e os dois ombros) torcia 60° no Idle e em todos os
clipes que vêm do anão: o pescoço saía de lado e um ombro entrava no corpo. Agora cada osso é alinhado por um
referencial inteiro (a direção dele, que no peito é a do pescoço, e a frente do corpo). Medido no Blender contra o FBX
original: todos os ossos a menos de 1°.

**Reimportar:** `ImportMixamoHeroes.py` apaga os clipes, a malha e o esqueleto antigos nessa ordem. Se um clipe ficou
sem esqueleto (uma importação que parou no meio), a Unreal não consegue nem apagar: tire `Hero_<raça>*.uasset` e
`Anims/<raça>` de `Content` com o editor fechado.

**Testes:** `RunHeroShots.ps1` PASS. Pés a 11,9 cm (humano) e 8,3 cm (anão) do chão, a altura do tornozelo.

## Armas: texturas (31/10)

As 5 armas do herói (as do Meshy, `Tools/ImportNewWeapons.py`) apareciam lisas e escuras na mão. Tinham dois defeitos:
- **Mapa de normais:** o Meshy (glTF) usa a convenção do OpenGL; sem inverter o verde (`flip_green_channel`), o relevo
  ficava ao contrário e a arma parecia pedra facetada.
- **Streaming:** a arma recém-equipada aparecia só com a cor média da textura. Agora as texturas das armas ficam
  inteiras na memória (`never_stream`).

- **Normais recalculadas (o principal):** a importação do glTF deixava "recalcular normais" ligado. A Unreal fazia uma
  normal por triângulo, e cada face dos blocos se partia em triângulos claros e escuros (pedra facetada). No modo de
  comando (`-run=pythonscript`) as funções de malha não fazem nada, então a correção roda no editor completo:
  `UnrealEditor.exe Vortex.uproject -ExecutePythonScript=<caminho>/Tools/FixWeaponNormals.py -unattended`, depois do
  `ImportNewWeapons.py`. `Tools/Blender/SharpWeaponEdges.py` separa as normais nas quinas acima de 35° (backup em
  `Data/VortexWeapons/Models/_backup_normais`).
- O teste com e sem o mapa de normais deu igual; o mapa ficou. A textura em si estava certa: com ela brilhando sozinha
  (sem luz), a lâmina saía prata.

Fotos: `RunWeaponCloseUp.ps1 -ExecCmds "r.MotionBlurQuality 0,r.DepthOfFieldQuality 0"` (sem o borrão do movimento).

## Clipes novos, criados e correções (01/11, terceira rodada)

**Clipes que você baixou** (`Data/Heroes/Mixamo/<raça>/clips/`):

| Raça | Andar | Correr | Disparada | Esquiva |
|---|---|---|---|---|
| Humano | Walking human (novo) | Slow Run human | Fast Run human | Stand To Roll humano (24-58, 0,6 s) |
| Anão | walk anao | Slow Run anao | anao_Sprint | Stand To Roll anao (24-58, 0,6 s) |

Os antigos continuam com outro nome (`WalkOld`, `RunOld`, `RunStartOld`).

**Clipes criados a partir deles** (`make_derived` no `MixamoToHero.py`):
- `Dodge_Back`: a cambalhota de trás para a frente;
- `Dodge_Left`/`Dodge_Right`: a cambalhota com o corpo virado 90° para o lado, voltando de frente no fim;
- `RunStart`: do parado para a corrida em 0,35 s; `RunStop`: a corrida freando e assentando no parado em 0,5 s.

O jogo toca o `RunStart` ao sair do parado indo para a corrida, e o `RunStop` ao parar depois de correr.

**Arco:** no Mixamo a mão esquerda segura o arco e a direita puxa a corda; o jogo prendia o arco na direita (o jeito
do aventureiro). Agora o conversor cria o osso `Bow_L` na mão esquerda, medido no quadro do arco puxado (arco em pé,
cabo na palma, corda para a mão direita), e `UVortexWeaponComponent::HandBoneFor` usa ele para os arcos nos heróis do
Mixamo.

**Buracos no corpo (o peito do anão):** ~1% dos triângulos do Mixamo vinham do avesso (954 no anão, 1341 no humano).
O Blender mostra os dois lados; a Unreal só a frente, e ficavam buracos. O conversor desvira essas faces.

**Armas no pedestal:** o facho de luz do pedestal (45 cd, na cor da ala) estourava a lâmina e tingia de rosa: agora
14 cd e quase branco. O brilho extra das texturas (`Bright` 1,15) voltou a 1.

## Elfo, demônio e morto-vivo no esqueleto do Mixamo (01/11)

Sem rigar no Mixamo: `Tools/Blender/RigFromMixamo.py <raiz> <raça>` (feito no Blender aberto, pelo MCP).
1. pega o esqueleto do humano do Mixamo (33 ossos) e move cada junta para a do modelo antigo da raça
   (`Data/Heroes/<raça>/Hero_<raça>.glb`, o esqueleto simples que já funcionava): quadril, coxas, joelhos, tornozelos,
   ombros, cotovelos e pulsos; a coluna dividida entre o quadril e o pescoço como no humano; cabeça e pés nas proporções
   do humano;
2. passa os pesos do modelo antigo: o tronco dividido entre as três vértebras pela altura (e os ombros), a cabeça entre
   pescoço e cabeça, o resto osso a osso;
3. grava `Data/Heroes/Mixamo/<raça>/<raça>.fbx` no formato de um FBX do Mixamo.

Depois o `MixamoToHero.py <raça> --clips humano,anao [--flatface]` faz o resto, com os clipes do humano e do anão.

| Raça | Quadril | Pescoço | Rosto animado |
|---|---|---|---|
| Elfo | (rigado no Mixamo: Walking/Run Forward elfo) | | sim, com ajuste |
| Demônio | 0,54 m | 0,91 m | sim, com ajuste |
| Morto-vivo | 0,60 m | 0,98 m | não (a caveira não tem o branco dos olhos; fica com os olhos acesos) |

Conferido no Blender: parado, andando, correndo, golpe de espada e samba, com ombros e pescoço no lugar.

**Elfo:** você rigou no Mixamo (`Walking elfo.fbx` e `Run Forward elfo.fbx`, com pele): ele usa esse esqueleto e
esses pesos (o meu encaixe ficou guardado no scratchpad, `elfo_rig_meu`), e esses dois clipes viram o andar e a
corrida dele.

**Rigs do Mixamo de todas as raças (01/11):** você rigou no Mixamo e baixou só a corrida de cada uma (e o andar do
bárbaro): `Run Forward demonio`, `Run Forward esqueleto` (morto-vivo), `gigante correndo` e `barbaro andando`. Elas usam
esse esqueleto e esses pesos (41 ossos, com alguns dedos: ficam parados); o resto das animações vem do humano e do anão
(`--clips humano,anao`). O meu encaixe (`RigFromMixamo.py`) ficou guardado no scratchpad (`rigs_meus`).
`BakeHeroFace.py --keeprect` mantém o enquadramento do rosto quando o rig muda (as medidas do ajuste continuam valendo).

**Rosto com ajuste por raça** (`Data/Heroes/<raça>_Mixamo/Face/ajuste.json`, em pixels do `head.png`): os olhos de cada
um (x0, y0, x1, y1) e a boca (centro x, centro y, largura, altura). No elfo a boca é uma linha fina e a busca caía no
queixo; no demônio o cabelo escuro encosta nos olhos e a busca juntava os dois (olhos e boca gigantes).
`clear_eyes` (caixas) pinta de pele o olho original antes de desenhar: no elfo o olho redesenhado é menor que o contorno
original (estava grande demais). O bárbaro também tem rosto animado (a boca fica entre as pinturas azuis). Os assets
antigos dessas raças (esqueleto do aventureiro) saíram de `Content` para o scratchpad (`ue_racas_antigas`).

## Memória da conversão (01/11)

Os FBX "With Skin" do Mixamo do anão trazem as texturas de 8K embutidas: só abrir um levava o Blender de 0,6 para
7 GB (e o PC de 16 GB travou). O `MixamoToHero.py` agora importa os FBX sem as imagens (devolve uma imagem de 1 px ao
importador), apaga malhas, esqueletos e imagens de cada clipe depois de usar, e a cor do herói vem de
`Data/Heroes/Mixamo/<raça>_cor.png` (2048 px). Pico medido: ~1,2 GB.

## Corrida com arma (01/11)

`Sword And Shield Run` (Mixamo, feito com o humano) vira o clipe `RunArmed` nos dois heróis: o anão usa o mesmo FBX
(os esqueletos do Mixamo têm os mesmos ossos, e o conversor acerta a diferença de pose). Correndo com arma de mão
(espada, adagas, martelo, cajado) o jogo usa ele no lugar da corrida sem arma, na taxa que casa o passo com o chão, e
sem o remendo do braço firme. O arco continua com a corrida normal (fica na mão esquerda). Andando com arma continua o
andar com o braço firme.

## Quinta rodada (01/11): sombra das armas, arco em pé, andar do anão

- **Triângulos pretos nas armas:** a textura e o modelo estão certos (no Blender, o arco do jogo e o original do Meshy
  ficam iguais). Era a sombra da própria arma sobre as milhares de faces pequenas dela: sem as sombras dinâmicas a arma
  fica igual ao Blender. As armas (na mão do herói, dos bandidos e no pedestal) não fazem mais sombra.
- **Arco como rifle:** medido na Unreal (`get_anim_pose_at_time` no fim do Arco_Draw), o eixo do comprimento do arco saía
  ao longo da mira e o da corda para cima: a Unreal lê o osso novo `Bow_L` com esses dois eixos trocados (os ossos antigos
  herdavam a correção do arquivo velho). `UVortexWeaponComponent::GripRotationFor` gira o arco meia volta em torno de
  (0,1,1) quando ele vai no `Bow_L` (no Blender, por isso, o arco aparece deitado no osso; no jogo fica em pé).
- **Andar do anão:** andando ele vai a 0,8 x 0,8 da velocidade (~0,96 m/s); a 1,2 m/s o andar dele tocava 2,2x.

**Ver as texturas no Blender:** importe o .glb da arma (`Data/VortexWeapons/Models/W_<arma>.glb`), crie um material
com `Data/VortexWeapons/Textures/Novas/T_N_<arma>.png` na cor, e use o modo de visualização "Renderizado" (o modo
"Material" não aparece quando o motor está em Workbench). O original do Meshy está em `Data/Modelos/Armas/<arma>/`.

## Quarta rodada (01/11): andar, arma firme, arco e dois lados

- **Andar (Caps Lock liga e desliga):** o jogo nunca andava; o movimento normal era a corrida a 4,6 m/s, e a corrida
  lenta do Mixamo (1,6 m/s) tocava quase 3x. Agora: andar 1,5 m/s (o clipe de andar), correr 3,8 m/s, disparada
  (Shift) 6,2 m/s; o anão a 80%. O andar vai junto no movimento salvo (`FLAG_Custom_2`), igual no servidor.
- **Clipe por intenção:** andando, o andar (ou o trote, se o passo não alcança); correndo e na disparada, entre a
  corrida lenta e a rápida, a que toca mais perto de 1,3x.
- **Arma firme:** correndo e andando com arma, o braço da arma fica 75% na pose do parado (as corridas do Mixamo são
  sem arma e jogavam a arma para todo lado). No arco, o braço esquerdo.
- **Arco:** o soltar acaba logo depois da flecha sair (quadros 125-134 do Shooting Arrow): o resto do clipe abaixava
  o arco e deitava ele na mão. A pegada `Bow_L` foi para o meio da palma (o modelo não tem dedos: caía no pulso).
- **Cambalhota sem "teletransporte":** a esquiva empurrava 4,25 m em 0,58 s, e a cambalhota do Mixamo rola ~1,9 m;
  além disso o clipe começava já no mergulho (o corpo aparecia deitado de repente). Agora a esquiva dura 0,7 s e anda
  ~3,3 m (o anão 80% disso), e o clipe vai dos quadros 16 a 60 (de pé inclinando até de pé de novo), entrando em 0,12 s.
- **Materiais com dois lados** (`M_WeaponTextured`, `M_HeroFace`, `M_HeroTextured`): os modelos do Meshy e do
  Mixamo têm faces com a ordem trocada; com um lado só ficavam triângulos pretos nas armas e buracos na roupa do anão.

## Passo casando com o chão (01/11)

O anão patinava: o jogo move os dois na mesma velocidade (andar 150, correr 460, disparar 620 cm/s), mas o passo dos
clipes do anão é curto (corrida de 173 cm/s no tamanho do modelo, e ele aparece a 70%), e a taxa estava travada em
1,35x.

Agora (`UVortexAnimInstance`):
- cada clipe guarda a própria velocidade (`RateScale` = base / velocidade do clipe, sem limite);
- o jogo multiplica pela escala do herói e toca na taxa que casa o pé com o chão, até 2,4x;
- a intenção escolhe o grupo (andar até 220, correr até 540, disparar acima); dentro dele vai o primeiro clipe que casa
  até 2x (andar: Walk, depois Jog; correr: Run, depois Sprint);
- o nado fica fora (o clipe é no lugar).

Medido (`RunHeroShots.ps1`): humano andando Walk 1,40x, correndo Run 1,59x, disparada Sprint 2,25x; anão andando Jog
1,80x, correndo Sprint 2,30x, disparada Sprint 2,40x (o limite: ainda escorrega ~25% na disparada).

**Anão a 80% (01/11, pedido seu):** `AVortexCharacter::RaceSpeed` (mapa em `ApplyHeroModel`, posto no servidor e nos
clientes) multiplica a velocidade a pé em `UVortexCharacterMovementComponent::GetMaxSpeed` (montado e nadando ficam
iguais). O limite da taxa subiu para 2,6x. Medido: anão anda a 368 cm/s; andando Jog 1,44x, correndo Sprint 1,84x,
disparada (520) Sprint 2,60x. Na disparada de verdade (720 x 0,8 = 576) ainda escorrega ~10%.

## Rosto animado, estilo Hytale (31/10)

O rosto troca de desenho, como no Hytale: os olhos e a boca são quadros projetados na frente da cabeça. O modelo não
deforma, então o estilo voxel fica intacto.

**Quadros** (8 de olhos e 8 de boca por raça):

| # | Olhos | Boca |
|---|---|---|
| 0 | aberto | neutra |
| 1 | meio fechado | A |
| 2 | fechado | O |
| 3 | olhando para a direita do herói | E |
| 4 | olhando para a esquerda do herói | sorriso |
| 5 | bravo (pálpebra em diagonal, sobrancelha em V) | dentes cerrados |
| 6 | feliz (^ ^) | M (fechada) |
| 7 | dor (> <, sobrancelha para cima) | grito |

**No jogo** (`AVortexCharacter::TickFace`, em todas as máquinas, só visual):

| Situação | Olhos | Boca |
|---|---|---|
| Normal | pisca a cada 2,5-5,5 s (meio, fechado, meio, em 0,17 s) e olha para onde a câmera vira | neutra |
| Golpe, defesa | bravo | dentes cerrados (grito no especial) |
| Levou dano | dor (0,55 s) | grito |
| Danças, comemorar, vitória | feliz e aberto alternando | sorriso |
| Rir | feliz | A e E alternando |
| Grito / provocar | bravo | grito / sorriso |
| Morto | fechados | O |

**Como é feito:**
1. `Tools/Blender/BakeHeroFace.py <raiz> <raça>`: a frente da cabeça numa imagem com as cores exatas da textura
   (`Data/Heroes/<raça>_Mixamo/Face/head.png`), mais o retângulo e os ossos em `face.json`.
2. `python Tools/MakeHeroFaces.py`:
   - acha os olhos pelo branco, a pele logo abaixo, as sobrancelhas acima e a boca (a linha escura na pele no humano;
     o vão entre o bigode e a barba no anão);
   - desenha os quadros em blocos do tamanho do pixel da textura: `eyes.png` e `mouth.png` (4x2);
   - a conferência sai em `Face/preview.png`.
3. `Tools/ImportHeroFace.py` (editor fechado):
   - texturas `T_<raça>_Eyes/Mouth` com pixel nítido;
   - material `M_HeroFace`: projeta os quadros pela posição de cada vértice antes da animação (`PreSkinnedPosition`),
     então o rosto acompanha a cabeça em qualquer clipe;
   - `MI_HeroFace_<raça>`: o retângulo passado do Blender para o espaço da malha pelos ossos.

O `ApplyHeroModel` usa o `MI_HeroFace_<raça>` (como material dinâmico) quando a raça tem; senão, o `MI_Hero_<raça>`.

**Teste:** `RunHeroShots.ps1` fotografa o rosto de perto em cada quadro (`heroi_<raça>_rosto_<n>.png`).
PASS. Os 8 quadros aparecem nos dois. Pontos que ainda dá para melhorar:
- o olhar para o lado do humano deixa uma faixa branca dupla no olho;
- no humano, a pálpebra cobre o fundo do olho e aparece um retângulo de pele um pouco mais claro em volta.

**Rosto liso (31/10, segunda rodada):** o modelo tinha os buracos dos olhos, das sobrancelhas e da boca, e o desenho caía
dentro deles (retângulo claro, faixa dupla). Agora:
- `MixamoToHero.py --flatface` empurra os vértices de cada região (`flat` em `face.json`) até o plano da pele em volta
  (calculado pela moldura de cada região, porque a testa e a bochecha não estão na mesma profundidade) e acerta as
  normais. O cabelo, o nariz, o bigode e a barba ficam como estão. Humano: olhos, sobrancelhas e boca. Anão: só os olhos.
- `MakeHeroFaces.py` redesenha o olho aberto do zero em blocos (cílios, branco, íris na cor do original, pupila e
  brilho). Olhar para os lados move a íris inteira, sem a faixa dupla.
- A boca do anão fica no vão de pele embaixo do bigode (antes caía na sombra do nariz). Só conta como bigode uma faixa
  alta sem pele; a linha da boca do humano é fina.
- **Cor acinzentada (corrigido):** a foto da cabeça saía pelo filtro de cor padrão do Blender (AgX), que desbota. Os
  quadros desenhados por cima da pele ficavam cinza. Agora a foto usa o filtro "Standard" (as cores exatas da
  textura). Para refazer: `BakeHeroFace.py <raiz> <raça> --force --model <glb com os buracos>` e `MakeHeroFaces.py`.
- `BakeHeroFace.py` não refaz a foto da cabeça se ela já existe: ela tem que ser a do modelo com os buracos (use
  `--force` só se o modelo original mudar).

**Profundidade:** os olhos ficam 8-12 cm atrás da frente da cabeça. A projeção pega até 13 cm (`depth` em `face.json`);
com 8 cm só as sobrancelhas mudavam. O material precisa de interpoladores de vértice para usar a posição antes da
animação nos pixels.

## Pedidos

- "A barba tá colada no peito, fica esquisito mexer a cabeça."
- "Veja animações legais e úteis e já pegue." / "Animações de brincadeira como dança, tipo do Fortnite."
- Andar e correr diferentes (o andar escolhido: Unarmed Walk Forward).

## Como funciona

1. **No Mixamo** o anão foi montado com o próprio modelo (marcadores no queixo, pulsos, cotovelos, joelhos e virilha).
   Os clipes ficam em `Data/Heroes/Mixamo/anao/` (os seus em `clips/`, as danças e gestos em `emotes/`).
2. **No Blender** (`Tools/Blender/MixamoToHero.py`, sem janela): cada clipe passa do esqueleto do Mixamo para o esqueleto
   do jogo (o do aventureiro).
   - Cada osso nosso copia o giro do osso do Mixamo que faz o mesmo papel, descontando a diferença entre as duas
     posturas de descanso. O Mixamo descansa em T; o nosso, de braços caídos.
   - O deslocamento para a frente sai, porque quem move o herói é o jogo. A velocidade que o clipe andava fica anotada
     em `Data/Heroes/anao_Anims/clips.json`.
   - O pulo foi cortado em três: impulso, no ar e pouso. O quadril não sobe, porque no jogo quem sobe é a cápsula.
   - O quadril sai dividido por (quadril do herói / quadril do aventureiro). Na Unreal o quadril é "AnimationScaled" e
     multiplica por essa conta; sem a divisão o anão flutuava ~4 cm. Medido no teste: o pé fica a 9,8 cm do chão,
     a altura do tornozelo.
   - Sai tudo num arquivo só: `Data/Heroes/anao_Anims/Hero_anao_Anims.glb`.
3. **Na Unreal** (`Tools/ImportHeroAnims.py`, editor fechado), os clipes viram
   `/Game/Vortex/Characters/Heroes/Anims/anao/MX_anao_<clipe>`.
   - A taxa de cada clipe que anda é a velocidade do jogo dividida pela do clipe, entre 0,8 e 1,6.
   - `UVortexAnimInstance::Clip` usa o clipe `MX_` da raça quando ele existe. Os outros heróis continuam com os do
     aventureiro.
   - Os clipes `A_<raça>_*` do redirecionador da Unreal (a corrida quebrada) não são mais usados.

## Barba

`Tools/Blender/RigHero.py --beard` passa a barba e as tranças para a cabeça, com só a ponta presa um pouco ao peito.
- **Como acha:** são os pedaços da frente, abaixo do queixo, com a cor do cabelo ou das contas douradas na textura.
- **Primeira pessoa:** o corpo sem cabeça perde a barba junto, então os fios na frente da câmera sumiram.

## Locomoção do anão

| Velocidade | Clipe |
|---|---|
| parado | Idle (o "parado" que você gostou) |
| até 150 | Walk (Unarmed Walk Forward) |
| 150–300 | Jog |
| 300–560 (o passo normal, 460) | Run |
| Shift (acima de 560) | Sprint, já inclinado: a inclinação extra do código desliga |
| pulo | Jump_Start, Jump_Loop, Jump_Land |
| nado | Swim, Swim_Idle |
| morte | Death |

## Roda de emotes (B)

- **Segurar B** abre a roda no meio da tela. O mouse aponta a fatia, e a câmera não gira enquanto isso.
- **Soltar B** toca o emote apontado. Soltar no centro cancela.
- **Rodinha do mouse:** troca a página entre **Danças** e **Gestos**.

| Página | Fatias |
|---|---|
| Danças | Hip-hop, Samba, Dança boba, Robô, Thriller, Macarena, Galinha, Twist, Swing, Salsa, Breakdance, Gangnam |
| Gestos | Acenar, Vitória, Comemorar, Palmas, Reverência, Rir, Pensar, Provocar, Saudar, Apontar, Guitarra, Sentar |

- **Quanto dura:** as danças repetem até você andar. Os gestos tocam uma vez.
- **O que encerra:** andar, pular, nadar, montar, esquivar, atacar ou cair.
- **Em grupo:** todos os jogadores veem, porque o emote é replicado pelo servidor.
- **Outros heróis:** um herói sem os clipes mostra as fatias apagadas.

## Braços de gorila (corrigido)

O esqueleto que o site montou deixava o "ombro" do anão na clavícula, 15 cm para dentro do peito. Com isso o braço
girava dali, fechava na frente do corpo e a mão entortava; era também por isso que o jogo empurrava o ombro 8 cm.

Agora o anão usa as juntas e os pesos do Mixamo (`Tools/Blender/RigHero.py --mixamo <FBX "With Skin">`), com o ombro no
lugar. O empurrão do ombro saiu: o anão ficou com 6° e 0 cm. O resto do jogo continua igual, porque os ossos têm os
mesmos nomes do aventureiro.

## Golpes e o resto

Cada golpe é um pedaço de um clipe do Mixamo. O pedaço estica ou encolhe para o instante do golpe (a mão mais rápida)
cair no meio da janela de dano da arma. Golpes `_Full` mexem o corpo inteiro; os outros só o tronco e os braços.

| Arma | 1º | 2º | 3º |
|---|---|---|---|
| Espada longa | corte de dentro (Stable Sword Inward Slash) | golpe vertical | giro (corpo inteiro) |
| Martelo | 1º da espada de 2 mãos | 2º da espada de 2 mãos | pulo com pancada no chão (corpo inteiro) |
| Adagas | 1º do combo de 1 mão | 2º do combo | 3º do combo |
| Cajado (não tinha) | golpe vertical | corte de dentro | 3º da espada de 2 mãos |
| Arco | puxar a corda e mirar (segurando) | soltar a flecha | |

| Ação | Clipe |
|---|---|
| Esquiva para a frente | a cambalhota do "correndo e rolando", em 0,62 s. As esquivas para trás e para os lados continuam as do aventureiro |
| Levar dano | o tranco do Receive Uppercut, só o tronco |
| Trocar de arma | sacar (desenbaiando) |
| Colher | se abaixar e pegar. O "Pegar" que veio do Mixamo estava parado, então foi feito com o fim do golpe pesado |
| Parado 14 s | espreguiça (Arm Stretching) e volta |

Ainda não ligados:

- **Por não terem uso no jogo:** Boxing, PunchingBag, Crawling, ClimbingLadder e as viradas.
- **Repetidos:** TwoHandedSwordDeath, Grito, Entry, HappyIdle e RunStart.
- **Beber:** saiu parado do Mixamo; a poção usa o gesto do aventureiro.

## Humano (30/10)

O humano montado no Mixamo (`Data/Heroes/Mixamo/humano/humano.fbx`) passou pelo mesmo caminho do anão:

- **Esqueleto:** `RigHero.py --mixamo humano.fbx`, com as juntas e os pesos do Mixamo.
- **Animações:** os 73 clipes do anão. Todos os clipes do Mixamo usam o mesmo esqueleto, então servem para o humano
  (`MixamoToHero.py humano --clips anao`). Isso inclui os golpes, a esquiva, os emotes e as danças.
- **No jogo:** o ombro dele também não é mais empurrado (5° e 0 cm).
- **Teste:** `RunHeroShots.ps1` PASS. O pé fica a 12 cm do chão (a altura do tornozelo) e ele dança pela roda.
- ~~Para depois: baixar os clipes no Mixamo com o humano~~ feito em 31/10 (abaixo).

### Clipes do próprio humano (31/10)

Os 11 FBX que você baixou no Mixamo com o humano estão em `Data/Heroes/Mixamo/humano/clips/`. O
`MixamoToHero.py humano --clips anao` agora usa primeiro os da raça e só completa com os do anão o que faltar (emotes,
danças, nado, cajado, esquiva...). Saíram 74 clipes, 13 deles do humano:

| FBX do humano | Clipe no jogo |
|---|---|
| Walking human | Walk |
| Fast Run human | Run e Sprint |
| Jumping humna | Jump_Start, Jump_Loop, Jump_Land |
| Standing Melee Attack Horizontal / Downward / 360 Low | Atk_Espada_1 / 2 / 3_Full |
| One Hand Club Combo | Atk_Martelo_1, 2, 3_Full |
| Dual Weapon Combo | Atk_Adagas_1, 2, 3 |
| Standing Draw Arrow | Arco_Draw (o soltar continua o do anão) |
| Standing React Death Backward | Death |

O "Jump front" entrou como `JumpFront`, guardado para um pulo correndo. Teste: `RunHeroShots.ps1` PASS.

## Raças e criação de herói (31/10)

- **Raças (01/11):** só o humano está liberado (`IsRaceOpen` em `VortexFrontEnd.cpp`); antes eram o humano e o anão. As outras cartas
  ficam escuras, com um cadeado e "EM BREVE", e não dá para escolher.
- **Criação:** ficou só o nome (com "Sortear nome"). Saíram os painéis de estilo e cores, o seletor de cor e os botões
  "Rosto" e "Surpreenda-me". A aparência é sorteada da raça.
- **Teste:** `RunFrontEndShot.ps1` PASS (`frontend_racas.png`, `frontend_criacao_humano.png`).

## Fotos

`Saved/Screenshots/heroi_anao_*.png`:

| Arquivo | O que mostra |
|---|---|
| `_andar` | andando devagar |
| `_corrida` | a corrida no passo normal |
| `_disparada` | a disparada no Shift |
| `_roda` | a roda de emotes aberta |
| `_danca` | dançando |

## Testes

- `Tools/RunHeroShots.ps1`: PASS nas seis raças. Com o esqueleto novo, o pé do anão parado fica a 8,6–9 cm do chão.
  - O anão toca os clipes novos e a roda escolhe e toca a dança (Thriller).
  - O teste agora anota a altura do pé e do quadril de cada raça.

## Correção no caminho

`UVortexAnimInstance` guardava os clipes da primeira raça vestida, e o jogo veste o humano antes da raça escolhida.
Por isso o anão não achava os clipes dele. Agora a lista recomeça quando a malha muda.

## O que testar

1. Ande devagar, no passo normal e no Shift com o anão. Andar, correr e disparar estão diferentes? O pé escorrega muito?
2. Vire a câmera e olhe para cima e para baixo em primeira e terceira pessoa. A barba acompanha a cabeça?
3. Segure **B**, escolha uma dança e solte. Ande para parar. Com um amigo: ele vê você dançar?
