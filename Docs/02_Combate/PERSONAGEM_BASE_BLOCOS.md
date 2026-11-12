# Personagem base de blocos (01/11)

Situação: 🟢 é o humano do jogo (rigado, com as animações do Mixamo e o rosto animado); 🟡 o cabelo separado está pronto no
Blender, falta entrar no jogo e no menu.

## Pedido

Fazer no Blender, pelo MCP, o personagem da imagem de referência (frente, lado, costas e o outro lado), idêntico, em
A-pose (pernas separadas e braços longe do corpo) para rigar e animar depois.

## Como foi feito

`Tools/Blender/MakeBlockCharacter.py` (rodado no Blender aberto):

- **Medidas:** saíram da imagem de referência, em pixels dela (187 px = 1,40 m):

  | Parte | Largura | Profundidade | Altura |
  |---|---|---|---|
  | Cabeça | 46 | 44 | 44 |
  | Tronco | 42 | 24 | 56 |
  | Cueca | 44 | 26 | 23 |
  | Braço | 20 | 22 | 69 |
  | Perna | 21 | 22 | 45 |
  | Pé (4 px à frente) | 22 | 26 | 19 |

- **Textura:** cada bloco tem a própria textura em pixel, com o filtro "Closest" e um texel de ~5,7 px da referência:
  - pele com ruído leve, mais escura embaixo e na borda de baixo;
  - topo da cabeça mais escuro;
  - linha do peito e do abdômen;
  - cueca clara com o cós e as dobras.
- **A-pose:** braços abertos 40° (girando no ombro, com 2 px de folga do tronco) e pernas abertas 7° (girando no
  quadril). Os pés ficam retos no chão. A pose é a de descanso da malha.
- **Frente:** olha para -Y.
- **Partes separadas:** cada parte é um objeto (`Cabeca`, `Tronco`, `Cueca`, `Braco_L/R`, `Perna_L/R`, `Pe_L/R`),
  prontas para ganhar ossos.

Arquivo: `Data/Modelos/PersonagemBase/personagem_base.blend`.

## Virou o humano do jogo (01/11)

As animações que você fez no Mixamo com ele (`modelando ainda/`) entraram como `Data/Heroes/Mixamo/humano/clips/humano_<clipe>.fbx`:

| Arquivo | Clipe |
|---|---|
| Breathing Idle / Walking / Running / Fast Run | Idle / Walk / Run / Sprint |
| Correndo com espada | RunArmed |
| Quick Roll To Run (16-43) | Dodge (e as derivadas para trás e para os lados) |
| pulando com espada | Jump_Start, Jump_Loop, Jump_Land |
| 1 golpe / segundo golpe / ataque giratório | Atk_Espada_1 / 2 / 3_Full |
| levando dano / morte com uma espada na mão | Hit / Death |
| puxando corda do arco | Arco_Draw |
| habilidade de corte, mirando com o arco, level up, as outras duas mortes | guardados (Habilidade_Corte, Arco_Aim, LevelUp, Death_Forward, Death_Left) |

O resto (emotes, danças, nado, as outras armas) vem do anão. O esqueleto e os pesos são o `personagem_base_rig.fbx`
(`humano.fbx`); a cor é a `T_PersonagemBase.png` (`humano_cor.png`), na Unreal `T_humano_Pixel` com pixel nítido
(`pixel.json` na pasta). O humano antigo ficou guardado no scratchpad (`humano_antigo_backup`).

**Rosto:** o rosto do boneco é liso; `Tools/MakeBlockFace.py` desenha do zero os olhos e as sobrancelhas numa grade de
16 x 16 (meio pixel da cabeça), sobre fundo transparente, com as 8 expressões de sempre.

- **Estilo (01/11, a referência "bonitinha"):** olhos grandes de 4 x 5 casas, escuros, com o contorno de cima mais escuro,
  a parte de baixo mais clara e um brilho comprido do lado de fora; sobrancelhas finas de 3 casas por cima da parte de
  dentro. Entre os olhos ficam 4 casas (o vão de um nariz, sem nariz).
- **Sem boca:** os 8 quadros da boca são vazios (o `TickFace` continua igual).
- **Expressões:** piscar meio e fechado (uma linha), olhando para os lados (o olho inteiro anda uma casa), bravo
  (sobrancelhas descendo para o nariz e a pálpebra baixa), feliz (∩ com as bochechas coradas) e dor (> <).

## Andar e correr (01/11)

- **Normal:** andar (o `Walking`, 150 cm/s). Agora é o padrão (`bWantsToWalk = true`).
- **Shift:** a corrida lenta (`Running` = clipe Run), `SprintSpeed` 460 (era 620), no ritmo do clipe. Com arma na mão
  continua a corrida com espada (RunArmed). A arrancada e a freada só aparecem na corrida.
- **Caps Lock:** troca para correr sempre (a 380 cm/s, com o mesmo clipe Run); outra vez volta a andar.
- **A corrida rápida saiu:** `Data/Heroes/Mixamo/humano/fora.json` (`{"clipes": ["Sprint"]}`) faz o
  `MixamoToHero.py` pular o clipe, nem o do humano nem o que viria do anão. O `MX_humano_Sprint` e o
  `humano_Sprint.fbx` foram guardados no scratchpad (`sprint_humano_backup`). O importador agora apaga os clipes do glb
  que não estão no clips.json.
- **A cavalo:** o galope ficou na velocidade de antes (`WalkSpeed * 2,85`, os 620 x 1,75 de antes).

## Cabelos separados (01/11)

Cada cabelo é uma malha à parte que o herói veste por cima do corpo e que o jogador escolhe e pinta no menu.

**Como é feito:** `Tools/Blender/MakeBlockHair.py <estilo>`.

- **Encaixe:** a caixa da cabeça sai dos vértices presos só ao osso `mixamorig:Head` (0,344 x 0,329 x 0,329 m).
- **Blocos:** meio pixel da cabeça (a mesma grade 16 x 16 do rosto), por fora dela. Só as faces de fora viram malha.
- **Cor:** uma paleta de 8 marrons, mais clara em cima e mais escura embaixo e nas pontas, variando por tufo de 2 x 2
  casas, com algumas mechas mais claras. No jogo vale só o tom (`T_Cabelo_Tons`, linear): o `M_BlockHair` faz
  `Tint x tom x 2`, e o Tint é a cor `hair_color` do herói.
- **Para o jogo:** `blender -b --factory-startup -P Tools/Blender/MakeBlockHair.py -- jogo <estilo>` faz o cabelo no
  esqueleto do `Hero_humano.glb` e sai em `Data/Heroes/humano_Mixamo/Hair/Hair_<estilo>.glb`.
  `Tools/ImportBlockHair.py` (editor fechado) leva para `/Game/Vortex/Characters/Heroes/humano/Hair/Hair_<estilo>`.
  Também importa a `T_Cabelo_Tons`, o `M_BlockHair` e a `T_humano_Pixel_F`.
- **Para conferir no Blender:** os arquivos ficam em `modelando ainda/cabelo_<estilo>.fbx` e `.blend`.

As peças do gerador:
- `cap`: o topo;
- `wrap`: em volta da cabeça, com o fim de cada coluna;
- `puff`: tufos;
- `curls`: cachos;
- `fan_spike`: os espetos do moicano;
- `tube`: rabos e marias-chiquinhas;
- `tube(braid=True)`: tranças;
- `bun`: coques;
- `band`: prendedores;
- `waves`: ondas;
- `THIN`: o raspado rente.

As correntes de física são `chain`, `long_chains` (nuca, esquerda e direita) e as dos tubos.

**Masculinos** (corpo masculino no menu):

| Estilo | Física |
|---|---|
| Liso curto | não |
| Longo desfiado | leve (nuca e lados) |
| Moicano punk (leque de espetos) | não |
| Risca lateral | não |
| Meio dividido | não |
| Penteado para trás | não |
| Undercut | não |
| Mullet | leve (a parte comprida) |
| Raspado (rente, `THIN` 0,28) | não |
| Espetado curto | não |
| Degradê cacheado | não |

**Femininos** (corpo feminino no menu). O 1 a 4 saíram pelos nomes da folha, porque as figuras vieram cortadas:

| Estilo | Física |
|---|---|
| Liso longo | leve |
| Ondulado longo | leve |
| Cacheado longo | leve |
| Meio preso (coque atrás) | leve |
| Rabo de cavalo alto | solta (o rabo) |
| Rabo de cavalo baixo (por cima do ombro) | solta |
| Maria-chiquinha | solta (as duas) |
| Bob curto | não |
| Camadas médias | leve |
| Franja longa | leve |
| Curto bagunçado | não |
| Coque despojado | não |
| Trança lateral | nas tranças, 0,7 |
| Dois coques | leve |
| Tranças duplas | nas tranças, 0,7 |

Careca aparece nos dois corpos.

**No jogo:** `FVortexLookCatalog::BlockHairs()` lista os cabelos. A escolha fica em `Look.Pieces["block_hair"]`, que
é salva com o herói e vai pela rede junto com o resto da aparência. Cada cabelo é de um corpo (`Body`). Ao trocar de
corpo, um cabelo do outro volta para o padrão: o liso curto no masculino e o rabo de cavalo alto no feminino. `AVortexCharacter::ApplyBlockHair` veste o `UVortexHairComponent`.

**Física (`UVortexHairComponent`):**
- Sem física, o cabelo segue o corpo como as roupas (`SetLeaderPoseComponent`). Com física, todo quadro, depois da
  animação do corpo, ele copia a pose final dele osso a osso (os nomes são os mesmos).
- Nos cabelos soltos, cada corrente de ossos `Hair_<nome>_NN` vira pontos com inércia e gravidade. Elas são filhas de
  `mixamorig:Head`, feitas no Blender, com pesos pela altura. A simulação é Verlet em passos fixos de 1/60 s.
- Cada ponto fica no comprimento do osso e é puxado de volta para a forma do modelo. O quanto depende da `Looseness`:
  0 é preso à cabeça, 0,5 é leve, 1 é solto.
- O cabelo é empurrado para fora do corpo por esferas na cabeça, no pescoço, na coluna e nos ombros. Com armadura no peito ou
  capacete, as esferas ganham 4 cm (sem armadura, 1,5 cm).
- Se o herói teleportar, a simulação volta para a forma do modelo.

**Correções de 01/11 (depois do primeiro teste do usuário):**
- **Cabelo saindo da cabeça:** o cabelo estava um quadro atrasado. A pose copiada só era aplicada no quadro seguinte,
  então nas trocas de animação e nos movimentos rápidos ele se soltava. Medido no editor com o moicano: até 64 cm e 97°
  num quadro, e de 2 a 7 cm na corrida. Agora a pose é aplicada no mesmo quadro (`RefreshBoneTransforms` logo depois
  da cópia).
- **Cor voltando ao padrão:** ao trocar o corpo, o `Normalized` escolhia de novo a peça `hair` antiga do Godot, e o
  `ChoosePiece` punha a cor padrão dela por cima. Agora a `hair_color` escolhida é mantida.

**Segundo teste do usuário (01/11, à noite):**
- **Cabelos cinza:** os 26 cabelos tinham sido importados com o `WorldGridMaterial`, porque o `ImportBlockHair.py`
  mudava cópias dos slots de material. Os slots foram refeitos e salvos com o `M_BlockHair`, e o script foi corrigido.
  O `ApplyBlockHair` agora cria o material a partir do `M_BlockHair` direto, sem depender do material da malha.
- **Moicano solto ao lado do ombro depois de bater, fazer combo e girar rápido:** não foi reproduzido no editor. A
  correção troca o jeito de prender o cabelo:
  - os cabelos sem física seguem o corpo como as roupas (`SetLeaderPoseComponent`), na mesma pose que vai para a tela;
  - os cabelos com física copiam a pose final do corpo (o espaço do componente), e não mais os ossos locais da
    animação (`CopyPoseFromSkeletalComponent`).

**Menu (criação, depois de escolher a raça):** um painel "Aparência" à direita com:
- **Corpo:** Masculino ou Feminino.
- **Cabelo:** os estilos.
- **Cor:** as cores prontas da paleta `hair_color`, 7 por linha, com a escolhida marcada em dourado.
- **Cor livre:** matiz (com o arco-íris por trás), saturação e brilho (com a faixa da cor atual por trás), e o código
  `#RRGGBB`, que vale ao apertar Enter.

O seletor livre guarda a matiz, a saturação e o brilho em vez de recalcular do RGB. Antes ele pulava no cinza e no
preto, porque o cinza não tem matiz.

## Versão feminina (01/11)

- **Corpo:** o mesmo corpo, o mesmo esqueleto e as mesmas animações (os cabelos servem nos dois).
- **Cor:** `Tools/MakeBlockFemale.py` gera a `humano_cor_f.png`, que é a cor do corpo com um top claro no peito, do pano
  da cueca. Na Unreal é a `T_humano_Pixel_F`.
- **Rosto:** `Tools/MakeBlockFace.py humano --feminino` gera os mesmos olhos com cílios para fora (`eyes_f.png`). Na
  Unreal é a `T_humano_Eyes_F`, importada pelo `ImportHeroFace.py`.
- **No jogo:** quando a aparência é `female`, o `ApplyHeroModel` troca o "Base" e o "Eyes" do material do rosto.
