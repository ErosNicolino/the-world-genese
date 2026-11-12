# Sons do jogo e músicas

Status: **aguardando teste no editor**. Os testes automáticos rodam sem som, então os sons precisam ser ouvidos.

Os sons e as músicas vêm do Godot (só lidos) e são importados por `Tools/ImportSfx.py` para `/Game/Vortex/Audio/Sfx` e `/Game/Vortex/Audio/Music`. Nas máquinas que desenham, os sons tocam com posição e somem com a distância (cerca de 25 m). O servidor dedicado não toca nada. Tudo sai do que já é replicado, então os outros jogadores ouvem o mesmo.

## Herói

| Momento | Som |
|---|---|
| Andar e correr | Passos conforme o chão embaixo (grama, areia, pedra ou lama, pela cor da célula do vale). Correndo, os passos são mais rápidos e mais altos; o anão dá passos mais curtos e o bárbaro mais longos |
| Nadar | Braçadas |
| Pular | Pulo |
| Cair de altura | Queda, mais forte quanto maior a queda |
| Esquiva | Rolamento |
| Combo da espada | Três sons de golpe, um para cada golpe do combo |
| Acerto | Espada; no lobo, também o som de criatura atingida |
| Levar dano | Dor (com a defesa erguida, som de armadura) |
| Recuperar vida | Cura |
| Morrer | Morte |
| Especial | O corte à distância já tinha o próprio som |

## Lobo

- Uiva ao uivar.
- Rosna ao observar, caçar ou rondar (no máximo a cada 5 s).
- Faz um som no bote e morde quando acerta.
- Ao correr, as patas fazem passos baixinhos conforme o chão.

## Música (como o `GameMusic.cs` do Godot)

| Situação | Música |
|---|---|
| Menu | "Echo des Montagnes" (o tema) |
| Exploração de dia | O tema |
| Noite | "Vale Adormecido" |
| Perto da vila, até 45 m da praça | "Praça do Santuário" |
| Lobo lutando a até 25 m | "Fúria de Azhur", que continua 6 s depois da luta |

- A troca é feita com fade de 2,5 s.
- Para desligar a música: `-VortexNoMusic`.

## Arquivos

- `Framework/VortexAudio.h/.cpp`: o ajudante de som e o `UVortexMusicSubsystem`.
- Ganchos de som em `VortexCharacter` (`UpdateSounds`, `Landed`, `OnJumped`), `VortexWolf` (`UpdateSounds`) e `VortexCombatComponent` (acerto).

## Testes

Combate, espada, lobo, matilha, movimento, entrada do cliente, lobo em rede, heróis, fluxo do menu e fotos da HUD passaram depois da mudança.

## Riscos

- O chão de cada passo é adivinhado pela cor da célula. Em cores de transição, pode sair grama onde é areia.
- Os volumes ainda são fixos. Falta a opção de volume no menu de opções.
