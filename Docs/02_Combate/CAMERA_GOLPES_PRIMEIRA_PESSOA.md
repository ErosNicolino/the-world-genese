# Câmera de terceira pessoa, alcance dos golpes e primeira pessoa (23/10)

## Motivo

O usuário achou a câmera e os acertos "estranhos, meio feios", principalmente os golpes acertando de longe. Ele pediu que fossem refeitos olhando como os jogos de ação fazem. Pediu também uma primeira pessoa realista: arma e mão embaixo ou de lado, aparecendo pouco, conforme a arma.

## O que estava errado

- **Alcance dos golpes de perto:** vinha do Godot em metros do Godot (a espada: `3 m × 1,3 + 0,4 m = 4,3 m`), sem a escala do herói (0,367, a mesma das armas em `Data/VortexWeapons/frames.json`). Os golpes acertavam a ~2,7 vezes a distância em que a lâmina chega. Valia para todas as armas de perto.
- **Leque do golpe:** ~95° para cada lado, o que pegava inimigos ao lado e até um pouco atrás.
- **Passo até o alvo:** o herói "andava" na direção do inimigo durante 0,3 s (uns 50 cm). Na prática, o golpe acertava de longe, sem chegar perto.
- **Câmera:** de 70 a 140 cm à direita e na altura do peito (estilo jogo de tiro por cima do ombro), sem nenhuma suavização ao seguir o herói.
- **Primeira pessoa:** braços com as animações da terceira pessoa a uma distância fixa da câmera, com um ajuste à mão por família. A espada sumia (~42° abaixo do meio da tela), e katana, manoplas, grimório e orbe ficavam no meio, cobrindo a mira.

## O que mudou

### Golpes (como nos jogos de ação)

- **Alcance físico** (`UVortexWeaponComponent::MeleeReach`): corpo do herói + braço (55 cm) + comprimento real do modelo da arma (o canto mais longe da pegada) + folga de 35 cm (70 cm nas pancadas no chão). Nunca passa do alcance antigo.
  - Adaga inicial: ~1,9 m do centro do herói até a borda do alvo (era 4,3 m).
  - Lança e alabarda, maiores, alcançam mais; manoplas, menos.
- **Leque do corte:** ~70° para cada lado (era ~95°). O herói já se vira para o alvo antes de bater.
- **Busca do alvo:** até 4,5 m (era 6 m, e o herói se virava para um bicho longe e cortava o ar), no semicírculo da frente. Um inimigo ao lado conta: o herói se vira para ele.
- **[Removido no mesmo dia, a pedido: o herói parecia "andar sozinho"; no lugar entrou a marcação de foco, ver [FOCO_NEVOA_MONSTROS_MENU](../04_Interface/FOCO_NEVOA_MONSTROS_MENU.md)]** **Avanço até o alvo** (`AVortexCharacter::LungeToward`): se o alvo está um pouco fora do alcance (até ~2,5 m além), o golpe leva o herói até a borda dele em 0,16 s. É feito com uma força de root motion, que o movimento do cliente prevê e o servidor confere, sem puxões na rede.
- **Recuo no acerto** (`VortexHitFx::Recoil`): o corpo desenhado do alvo vai 9 cm para trás e volta em 0,18 s (15 cm no crítico; quase nada nos chefes). É só visual: posição, rede e IA não mudam.
  - Já existiam: brilho no corpo, pausa de impacto de 65 ms, tremor da câmera e respingos.
  - O empurrão de verdade continua só no crítico e no atordoamento, para o bicho não fugir do herói a cada golpe.

### Câmera de terceira pessoa

- **Andando e lutando:** quase atrás do herói, 28 a 45 cm para o lado e 12 a 60 cm acima do pivô (cresce com o zoom), olhando um pouco para baixo. É o enquadramento dos jogos de ação com espada.
- **Atraso ao seguir** (`CameraLagSpeed 12`, no máximo 60 cm): suaviza paradas, pulos e degraus. A rotação continua direta no mouse.
- **Mira (segurar F):** volta o enquadramento por cima do ombro, como antes. Trocar o ombro com V continua valendo.
- **Primeira pessoa e câmera dos testes:** o atraso desliga.

### Primeira pessoa

- **Âncora por família de arma** (`VortexWeaponKit::ViewAnchor`). Com o herói parado ou andando, o jogo mede onde fica a mão que segura a arma e desliza os braços até ela cair num ponto da tela. Durante golpes, defesa e poção, a medida congela e o golpe atravessa a tela a partir desse ponto.

| Família | Onde a mão fica (frente, direita, altura em cm da câmera) |
|---|---|
| Uma mão (espada, adagas, maça, machados, varinha, pistola, chicote, corrente, chakram, frasco) | (42, 18, −17): embaixo à direita |
| Duas mãos (espadão, martelo, katana) | meio das duas mãos em (46, 9, −23) |
| Cabo comprido (alabarda, lança, foice) | meio das duas mãos em (50, 19, −28): a mão da frente fica fora do meio |
| Manoplas | meio das duas em (50, 2, −24): uma luva em cada canto |
| Grimório, orbe, alaúde | meio das duas em (48, 6, −25) |
| Arco | (48, 17, −11): o arco em pé à direita |
| Besta e cajado | (44, 13, −18) |

- A regra de deixar o meio da tela livre continua: se a arma entra num círculo de ~15° em volta da mira, os braços deslizam para fora.

## Fotos

- **Primeira pessoa, as 24 armas paradas, mesma posição** (`RunWeaponCloseUp`), 8 por folha:

| Armas | Antes | Depois |
|---|---|---|
| espada, katana, adagas, maça, machado, machados duplos, chicote, correntes | [folha](../Images/primeira_pessoa/folha_antes_1.png) | [folha](../Images/primeira_pessoa/folha_depois_1.png) |
| chakrams, pistola, varinha, frasco, espadão, martelo, alabarda, lança | [folha](../Images/primeira_pessoa/folha_antes_2.png) | [folha](../Images/primeira_pessoa/folha_depois_2.png) |
| foice, cajado, arco, besta, manoplas, grimório, orbe, alaúde | [folha](../Images/primeira_pessoa/folha_antes_3.png) | [folha](../Images/primeira_pessoa/folha_depois_3.png) |

  Cada arma também está sozinha em `Docs/Images/primeira_pessoa/antes_<arma>_1p.png` e `depois_<arma>_1p.png`, e no golpe em `_1p_golpe`.
- **Câmera:**

| Situação | Antes | Depois |
|---|---|---|
| Parado | [foto](../Images/camera_golpes/antes_idle.png) | [foto](../Images/camera_golpes/depois_idle.png) |
| Golpe parado | [foto](../Images/camera_golpes/antes_still.png) | [foto](../Images/camera_golpes/depois_still.png) |
| Golpe andando | [foto](../Images/camera_golpes/antes_walking.png) | [foto](../Images/camera_golpes/depois_walking.png) |

- **Golpe num golem ao lado** (o herói vira e avança): [foto](../Images/camera_golpes/depois_golpe_de_lado.png).

## Validação

- Compilou.
- `RunFeatureShots`, golpe de perto:
  - **antes:** acertava o golem a 2,4 m da borda sem sair do lugar;
  - **com o alcance novo e sem o avanço:** errou, como esperado;
  - **com o avanço:** o herói se vira, avança de 2,6 m para 1,06 m e acerta. Passou.
- Também passaram `RunSwordSmoke` (combo, aparo e defesa com os mesmos números de antes), `RunCombatSmoke`, `RunCameraSmoke` (colisão, zoom, mira e troca de ombro), `RunWolfSmoke`, `RunPackSmoke`, `RunClientInputSmoke`, `RunMovementNetworkSmoke` e `RunWolfNetworkSmoke`.

## Riscos

- **Alcance:** as criaturas e os bandidos não mudaram (usam os próprios números). Um golpe de herói que antes acertava de longe agora pede chegar perto. Se ficar difícil demais, a folga (`Slack` em `MeleeReach`) é o ajuste.
- **Avanço:** pode empurrar o herói para a beira de um barranco ao bater num bicho logo abaixo. O avanço só acontece no chão.
- **Câmera:** com o enquadramento mais centrado, o herói fica mais perto da mira na tela.
- **Primeira pessoa:** a âncora é medida no boneco desta máquina; raças muito diferentes (anão, bárbaro) usam a mesma tabela.

## Para você testar jogando

- **Combo nos bichos:** que o golpe só acerte quando a lâmina chega e que o herói "puxe" até o inimigo; inimigo ao lado deve funcionar.
- **Câmera andando, correndo, pulando e no zoom:** se o atraso agrada (`CameraLagSpeed`) e se a altura está boa.
- **Primeira pessoa (C)** com várias armas, parado, andando e golpeando.

## Reversão

- **Golpes:** em `VortexSwordAbility.cpp`, troque `Weapon->MeleeReach(Swing)` por `Swing.ReachCm`.
- **Avanço:** tire a chamada a `LungeToward`.
- **Câmera:** em `CameraOffsetFor`, volte para os números da mira e desligue `bEnableCameraLag`.
- **Primeira pessoa:** `ViewAnchor`; a função antiga `ViewOffset` está no git.
