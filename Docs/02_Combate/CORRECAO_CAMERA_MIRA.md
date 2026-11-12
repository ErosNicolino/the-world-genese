# Câmera de ombro e mira — aguardando teste no editor

## O que mudou e por quê

- Segurar o botão direito aproxima suavemente a câmera para até 2,10 m, reduz o campo de visão de 75° para 68° e mantém o herói voltado para a direção da mira. Ao soltar, volta ao enquadramento e ao giro livre de caminhada. Durante um golpe, o herói também acompanha a direção da câmera pelo tempo do ataque.
- A tecla **V** troca o ombro direito pelo esquerdo. O scroll continua alterando o zoom em 60 cm por passo, entre 1,50 m e 8 m; o ombro, altura e campo de visão acompanham a transição.
- Removi o atraso de posição e rotação do braço da câmera que produzia balanço ao girar. A colisão continua imediata quando algo se interpõe; o retorno à distância desejada é suavizado. A esfera de colisão passou de 18 para 24 cm para reduzir a aproximação excessiva da cabeça.
- O teste de direção do golpe usa o ponto obtido por `TraceAim` no servidor para filtrar o arco de acerto do jogador. A checagem de alcance e de obstrução continua no servidor. Monstros não usam esta mudança.
- A câmera das medições (`UseGodotBenchCamera`) continua com 7 m, alvo de 1,35 m, raio de colisão de 18 cm, sem lag, sem a suavização nova de obstáculos e FOV 64°. A tecla F, a esquiva, a corrida e a grama não tiveram seus comandos alterados.

## Fotos na mesma posição do L_TerrainProto

| Situação | Antes | Depois |
|---|---|---|
| Parado | [foto](../Images/camera_before_idle.png) | [foto](../Images/camera_after_idle.png) |
| Correndo | [foto](../Images/camera_before_run.png) | [foto](../Images/camera_after_run.png) |

Sequência na mata: [giro](../Images/camera_sequence_turn.png), [corrida](../Images/camera_sequence_run.png), [golpe](../Images/camera_sequence_attack.png), [mira](../Images/camera_sequence_aim.png), [ombro esquerdo](../Images/camera_sequence_left.png). No enquadramento padrão da foto nova, o corpo e os pés aparecem por inteiro. O enquadramento de mira é mais fechado de propósito e pode cortar os pés.

## Validação

- Compilação `VortexEditor Win64 Development`: passou.
- `RunCameraSmoke.ps1`: passou, inclusive na execução ampliada (colisão, limites de zoom, traço de mira, enquadramento ao mirar e troca de ombro).
- `RunCameraSequenceShot.ps1`: capturou os cinco estados no mesmo trecho de mata.
- `RunDodgeDirectionSmoke.ps1`: passou nos quatro clipes.
- `RunSolidWalkSmoke.ps1`: passou nas nove posições com árvores e rochas.
- `RunMovementSmoke.ps1 -Matrix`: passou nas rampas, escadas e água.
- `RunSwordSmoke.ps1`: passou (sequência de golpes, bloqueio e contra golpe).
- `RunCombatSmoke.ps1`: passou (acerto e dano).

## Risco, desempenho e reversão

O traço de mira é feito no momento do acerto, não a cada quadro. A recuperação de colisão interpola apenas depois de um bloqueio; a câmera pode parecer diferente em corredores de árvores muito estreitos e precisa do teste visual no editor. O giro de mira é replicado por comando do cliente ao servidor; a sensação com latência ainda precisa de avaliação jogando como cliente. Reversão: restaurar este commit, preservando o commit separado do lobo parcial.

## Avaliação pendente no editor

O jogador deve conferir se o ângulo padrão, a distância da mira, a troca de ombro, o giro durante corrida e golpe e o retorno da câmera entre troncos ficaram bons. **Esta correção não está declarada concluída antes desse teste.**
