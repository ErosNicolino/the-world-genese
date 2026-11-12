# Adaga inicial no lugar da Espada de Cinzas

## Motivo e dependências

O usuário pediu que a espada padrão do herói passasse a ser o modelo `Data/fantasy dagger 3d model.glb` (gerado no Tripo). A troca segue o caminho das armas refeitas da [REVISAO_ARMAS](REVISAO_ARMAS.md): o modelo é montado no referencial do item (pegada na origem, lâmina para +Z), recebe o referencial da mão da Espada de Cinzas (`Data/VortexWeapons/frames.json`) e vai para o `Socket_Weapon_R`. Golpes, tempos, dano e o especial continuam os da [Fase 9](FASE_09_ESPADA.md).

## O que mudou

- `Tools/Blender/BuildStarterDagger.py` (Blender em segundo plano) prepara o modelo:
  - acha o comprimento e a guarda pela forma (o modelo vinha deitado na diagonal, em centímetros);
  - põe a origem no meio da pegada, a lâmina para +Z e a guarda no eixo X;
  - usa o comprimento da Espada de Cinzas (2,3 unidades do item, ~84 cm na mão; a espada tinha 2,42);
  - afina o cabo (raio 0,12 → 0,075) e o pomo para a mão do herói fechar em volta;
  - reduz de ~98 mil para 12 mil triângulos.

  O resultado fica em `Data/VortexWeapons/StarterDagger/W_adaga_inicial.glb`, fora de `Models`, para o `ImportVortexWeapons.py` não reimportá-lo junto das 25 armas.
- `Tools/ImportStarterDagger.py` importa a adaga em `/Game/Vortex/Weapons/StarterDagger` com as três texturas do modelo (cor, metal/rugosidade e normal, limitadas a 1024 px). Também:
  - desliga o Nanite, como nas outras armas;
  - liga `DA_DefaultSword` à adaga;
  - baixa o metálico do material para 0,3 e clareia a cor em 1,2×, como no `ImportVortexWeapons.py`. Sem isso, sem Lumen, a lâmina só refletia o chão e ficava marrom-escura (ver a primeira foto de "depois" abaixo, que foi refeita).
- `ImportVortexWeapons.py`: se for rodado de novo, mantém a adaga na espada padrão em vez de voltar para `W_espada_de_cinzas`.

**Quem também passa a usar a adaga:** tudo que cai em `espada_de_cinzas`. Isso inclui as espadas do arsenal sem modelo próprio, `sword_iron` e `sword_jade`, e o capitão e o ladrão dos bandidos (`VortexMonsterSpecies.cpp`).

## Fotos (mesma posição, `RunWeaponCloseUp.ps1 -Types espada_de_cinzas`)

| Situação | Antes | Depois |
|---|---|---|
| Parado, de perto | [foto](../Images/adaga_inicial/antes_pose.png) | [foto](../Images/adaga_inicial/depois_pose.png) |
| Pelo lado esquerdo | [foto](../Images/adaga_inicial/antes_esquerda.png) | [foto](../Images/adaga_inicial/depois_esquerda.png) |
| Golpe | [foto](../Images/adaga_inicial/antes_golpe.png) | [foto](../Images/adaga_inicial/depois_golpe.png) |
| Especial | [foto](../Images/adaga_inicial/antes_especial.png) | [foto](../Images/adaga_inicial/depois_especial.png) |
| Primeira pessoa, golpe | [foto](../Images/adaga_inicial/antes_1p_golpe.png) | [foto](../Images/adaga_inicial/depois_1p_golpe.png) |

## Validação

- No editor, a malha da adaga ocupa no soquete quase o mesmo volume da espada anterior: de (−6, −12, −44) a (7, 53, 11) cm, contra (−5, −15, −46) a (5, 54, 12) cm.
- Os golpes foram conferidos fora do jogo. A pose de cada osso foi calculada na Unreal (`AnimationLibrary`) a cada 1/24 dos clipes `Attack_01`, `Attack_02`, `Attack_03` e `Skill_Guard`. A adaga foi posta no Blender casando os ossos dos dois programas (erro de 5×10⁻⁸ m).
  - Na pose parada e nos golpes 1 e 3, a lâmina não atravessa o corpo: fica a pelo menos 33 cm do tronco e 37 cm do centro da cabeça.
  - Os problemas encontrados estão logo abaixo.
- `RunSwordShot.ps1 -Normal` não serve para comparar: ele soma o tempo de carregamento e pede as três fotos no mesmo quadro (só a do golpe andando sai). Está na lista de bugs.

## Golpes refeitos (22/10, pedido do usuário)

Os problemas medidos acima foram corrigidos em clipes novos. Os do Godot continuam no projeto, sem mudança.

- `Tools/Blender/BuildSwordAttacks.py` lê os clipes direto do GLB do Godot, com a mesma interpolação da Unreal (slerp), e faz as correções no espaço de cada osso do arquivo original. Ele grava `Data/GodotWeapons/Adventurer_SwordAttacksV2.glb` com os mesmos nós, malha e esqueleto; só as animações mudam. O exportador glTF do Blender não foi usado porque ele gira os eixos dos ossos, e a Unreal aplicaria as chaves num esqueleto diferente. A conversão Blender ↔ glTF foi conferida com o clipe original: 0,00° de erro em todos os ossos.
- No Blender aberto, `preview()` cria as ações `VX_Attack_01`, `VX_Attack_02`, `VX_Attack_03` e `VX_Skill_Guard` para ver cada golpe com a adaga na mão.
- `Tools/ImportSwordAttacksV2.py` importa os clipes em `/Game/Vortex/Weapons/Sword/Adventurer_SwordAttacksV2` e os liga ao `DA_DefaultSword`. Tempos, janelas de dano, dano e alcance não mudam.

| Golpe | O que estava errado | O que mudou |
|---|---|---|
| 1º | O corte inteiro (de trás do ombro até a frente, ~2,5 m de arco na ponta) acontecia em 33 ms: a 60 FPS, dois quadros, e não se via o arco. | O corte dura 0,208 → 0,29 s; o resto do clipe encolhe para manter os 0,62 s. |
| 2º | Na preparação, a lâmina descia entre as pernas (ponta a 6 cm do chão). Depois o braço girava ~150° num quadro. | A lâmina prepara para trás e para fora da perna. O corte dura 3 quadros (0,167 → 0,25 s) e cruza na frente do corpo até o alto, do lado direito. |
| 3º | O clipe tem 1,12 s e o golpe acaba aos 0,94 s, então a volta era cortada e o braço pulava para a pose parada. | O corte (até 0,47 s) fica igual e a volta é comprimida para caber nos 0,94 s. |
| Defesa (F) | A lâmina ficava atravessada na frente do queixo. | Os dois braços vão 12° para a frente (a mão esquerda continua na lâmina). |

Medidas na adaga, a cada 1/60 s (menor distância da lâmina ao centro de cada osso):

| Clipe | Medida | Antes | Depois |
|---|---|---|---|
| 1º golpe | maior salto da ponta por quadro | 1,28 m | 0,75 m |
| 2º golpe | ponta mais baixa | 6 cm (16 cm na Unreal, depois) | 14 cm |
| 2º golpe | quadril | 21 cm | 31 cm |
| 2º golpe | tronco | 25 cm | 31 cm |
| 2º golpe | maior salto da ponta por quadro | 1,05 m | 0,59 m |
| Defesa | cabeça | 25,5 cm | 27,6 cm |
| 3º golpe | volta até a pose parada | cortada aos 0,94 s | inteira |

2º golpe quadro a quadro, no Blender e na mesma câmera: [antes](../Images/adaga_inicial/golpe2_quadros_antes.png) e [depois](../Images/adaga_inicial/golpe2_quadros_depois.png). 1º golpe novo: [quadros](../Images/adaga_inicial/golpe1_quadros_depois.png).

Fotos no jogo depois dos golpes novos (`RunWeaponCloseUp`; o teste tira só dois instantes de cada golpe):

| Situação | Antes | Depois |
|---|---|---|
| Golpe | [foto](../Images/adaga_inicial/golpes_antes_golpe.png) | [foto](../Images/adaga_inicial/golpes_depois_golpe.png) |
| Golpe (2º instante) | [foto](../Images/adaga_inicial/golpes_antes_golpe2.png) | [foto](../Images/adaga_inicial/golpes_depois_golpe2.png) |
| Especial (usa o 3º golpe) | [foto](../Images/adaga_inicial/golpes_antes_especial.png) | [foto](../Images/adaga_inicial/golpes_depois_especial.png) |

**Ainda falta:** em primeira pessoa a arma não aparece parada e, no golpe, só se vê o punho no canto. Isso já acontecia antes e vai para a lista de bugs.

**Para voltar só os golpes:** aponte os golpes e a `guard_animation` do `DA_DefaultSword` para `/Game/Vortex/Weapons/Sword/Adventurer_SwordAttacks/...` (os clipes do Godot), ou rode o `CreateSwordDefinition.py`.

## Risco, desempenho e reversão

São 12 mil triângulos (a Espada de Cinzas tinha ~3,6 mil) e três texturas de 1024 px. Para uma arma por personagem, o custo não aparece no FPS: nas fotos, 141–167 FPS antes e depois. O material usa o material de glTF da Interchange (`MI_Default_Opaque`), e não o `M_WeaponTextured`. Para voltar atrás, ponha `/Game/Vortex/Weapons/Models/W_espada_de_cinzas` no `visual_mesh` do `DA_DefaultSword`, ou apague `/Game/Vortex/Weapons/StarterDagger` e rode o `ImportVortexWeapons.py`.
