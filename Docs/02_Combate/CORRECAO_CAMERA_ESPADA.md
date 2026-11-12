# Correção do encaixe da espada e da câmera de jogo

## Motivo e dependências

O usuário mostrou a espada cruzando o braço e o pescoço durante os golpes e pediu uma câmera mais baixa sobre o ombro direito. Esta correção parte da Fase 9 (`c7986a2`), do esqueleto do aventureiro e da malha/colisão do terreno da Fase 6. O projeto Godot e os saves continuam intocados.

## O que mudou

- A espada visual anterior usava `Sword_Default.glb` com o transform de repouso do osso `Sword` copiado para `Socket_Weapon_R`. Esse conjunto tinha origem/orientação diferente do visual embutido no herói. `ExtractGodotEmbeddedSword.py` extrai apenas a geometria da espada que já está no modelo original, na escala do herói. O visual agora segue o próprio osso `Sword`; a escala absoluta do componente impede que a animação que oculta a arma embutida também o esconda. `DA_DefaultSword` usa a nova malha.
- A câmera normal saiu de 600 cm, FOV 64° e `SocketOffset=(0,70,156)` para 380 cm, FOV 75°, pivô 45 cm acima do centro da cápsula e deslocamento de aproximadamente 85 cm à direita e 13 cm acima. O corpo inteiro e os pés aparecem na distância padrão.
- A roda do mouse altera o alvo entre 150 e 800 cm, com interpolação suave. O deslocamento lateral e a altura acompanham a distância. O tick do personagem só fica ativo enquanto a câmera faz a transição; o traço da mira é calculado apenas sob demanda por `TraceAim`.
- Uma mira discreta ocupa o centro da tela. O braço de câmera usa o canal `Camera` e uma sonda de 18 cm para não atravessar obstáculos.
- `UseGodotBenchCamera` mantém o enquadramento e FOV 64° das medições/fotos antigas; também oculta a mira. A tecla F, corrida e esquiva mantêm suas entradas.

## Fotos na mesma posição de `L_TerrainProto`

| Situação | Antes | Depois |
|---|---|---|
| Parado | [foto](../Images/camera_before_idle.png) | [foto](../Images/camera_after_idle.png) |
| Correndo | [foto](../Images/camera_before_run.png) | [foto](../Images/camera_after_run.png) |

Espada na câmera nova: [parado](../Images/sword_normal_idle.png), [golpe parado](../Images/sword_normal_still.png), [terceiro golpe andando](../Images/sword_normal_walking.png).

## Validação

- Build `VortexEditor Win64 Development`: passou.
- `RunCameraSmoke.ps1`: passou com obstáculo no caminho da câmera, zoom suave nos limites de 1,5 e 8 m e traço da mira (`collision=1 near=1 far=1 aim=1`). No mapa, os troncos, rochas e o chão usam `BlockAll` e são atingidos pelo mesmo canal `Camera`.
- `RunSwordShot.ps1 -Normal`: capturou parado, golpe parado e terceiro golpe andando. O visual permaneceu com escala 1. Na amostragem dos três golpes, a menor distância da lâmina ao centro da cabeça foi 34,7 cm e ao centro do tronco 26,6 cm. Essa é uma medida geométrica, não substitui a inspeção visual de cada roupa e ângulo.
- `RunSwordSmoke.ps1`: passou (`combo=96 parry=96 block=92 release=82`).
- `RunDodgeDirectionSmoke.ps1`: passou nas quatro direções. `RunSolidWalkSmoke.ps1`: passou nos seis troncos e três rochas. `RunMovementSmoke.ps1 -Matrix`: passou em três rampas, escada e piscina.

## Risco, desempenho e reversão

O braço de câmera usa a colisão padrão da Unreal; locais muito apertados podem aproximá-la abruptamente, o que ainda precisa de teste manual na mata e nos degraus. A mira soma poucas linhas ao HUD, e o traço não roda a cada quadro. O usuário ainda precisa decidir no editor se o ângulo e a animação da espada ficaram bons. Para reverter, voltar este commit preserva as Fases 7–9; a referência Godot segue separada.
