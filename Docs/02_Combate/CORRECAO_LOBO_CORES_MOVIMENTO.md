# Lobo — cores, olhos e movimento (aguardando teste no editor)

Retoma `CORRECAO_LOBO_PARCIAL.md`.

## Cores: a causa e a correção

- **Causa.** O GLB convertido (`Data/GodotWolf/Wolf_Skinned.glb`) tem todas as cores por voxel do Godot. A conferência foi feita no Blender pelo MCP, com as mesmas cores do `Wolf.glb` original. O material antigo também lia a cor de vértice corretamente. O que falhava era o caminho das cores de vértice até a Unreal: o lobo inteiro chegava num cinza só.
- **Correção.** `Tools/ConvertGodotWolf.py` agora grava as 581 cores do lobo numa paleta de 64×64 (`Data/GodotWolf/Wolf_Palette.png`) e dá a cada vértice um UV que aponta para a sua cor.
  - A paleta guarda em sRGB a mesma cor que o Godot usa como albedo linear (`vertex_color_is_srgb = false`).
  - `Tools/ImportWolf.py` reimporta a malha e os 8 clipes no mesmo caminho e cria `T_WolfPalette`, sem filtro, sem mipmaps e sem compressão, o que mantém o voxel nítido.
  - Também cria `M_WolfFur_Palette` (rugosidade 0,9, como o corpo no Godot) e `M_WolfEyes_Palette`.
- **Olhos.** No Godot, a parte "Eyes" brilha em ciano `#7ff5ff` com força 1,6 (`ValleyMonster.Model.cs`). No Unreal o brilho é `#7ff5ff` × 6: a exposição da Unreal pede mais força para o mesmo efeito na tela.
- **Conferência no Blender:** [original em cima, paleta embaixo](../Images/wolf_blender_paleta.png), com as mesmas cores.
- **Unreal:** [quatro lados](../Images/wolf_paleta_quatro_lados.png) e [olhos de perto](../Images/wolf_paleta_olhos.png). Aparecem a juba e o peito brancos, o dorso cinza-azulado, o focinho e as orelhas escuros com o interior rosa, a ponta da cauda branca e os olhos ciano brilhando.

## Movimento: patas duras e andar aos trancos

- **Causa.** A IA do lobo decide 10 vezes por segundo e impunha a velocidade diretamente. Entre duas decisões, o freio padrão do `CharacterMovement` zerava essa velocidade. Em FPS alto, o lobo andava alguns quadros e parava outros, e a animação, que escolhe Walk/Run pela velocidade real, caía em Idle no meio da perseguição: patas paradas deslizando.
- **Correção (`AVortexWolf`).** O `Think` agora só escolhe direção e velocidade. A cada quadro, no servidor, o lobo pede esse movimento ao `CharacterMovement` (`AddMovementInput`, com `MaxWalkSpeed` igual à velocidade escolhida). A aceleração é de 2 600 cm/s² e o freio de 1 800 cm/s². Ataques, desvio de obstáculos e matilha continuam iguais.
- **Medição (`Tools/RunWolfMotionProbe.ps1 -Fps 120|60`, novo).** O herói fica a 14 m; o teste mede a velocidade e a animação do lobo de 3 a 4 s, depois do uivo:

| | Antes (120 / 60 FPS) | Depois (120 / 60 FPS) |
|---|---|---|
| Quadros perseguindo com pose parada | 20/121 / 10/61 | **0/43 / 0/16** |
| Velocidade média | 271 / 283 cm/s | **553 / 558 cm/s** |
| Maior salto de velocidade | 74 017 / 36 780 cm/s² | **9 600 cm/s²** |

Depois da correção, o lobo também alcança o herói e salta (`Pounce`) dentro do tempo do teste. A sonda confirmou ainda que a pele acompanha os ossos (os vértices se movem com a pata).

## Testes

- Compilação: passou.
- `RunWolfSmoke`, `RunPackSmoke` (5 lobos) e `RunWolfNetworkSmoke`: passaram.
- `RunWolfVisualShot`: as fotos acima.

## Riscos e reversão

- O lobo não usa mais a cor de vértice, e sim a paleta. Se o modelo do Godot mudar, rode `ConvertGodotWolf.py` e depois `ImportWolf.py` com o editor fechado.
- O brilho dos olhos à noite e com a exposição automática precisa de avaliação no editor (`EYE_GLOW` em `ImportWolf.py`).
- A perseguição ficou mais rápida de verdade, porque o lobo agora atinge a velocidade pedida. Se ficar difícil demais, ajuste as velocidades no `Think`, não o freio.
- Reversão: reverter este commit restaura o import anterior (cinza) e o movimento antigo.

## Avaliação pendente no editor

Olhe o lobo parado, andando, correndo, uivando e atacando, de frente, dos lados e de costas, perto do herói. Confira as patas no chão em degraus e o brilho dos olhos. **A correção não está declarada concluída antes desse teste.**
