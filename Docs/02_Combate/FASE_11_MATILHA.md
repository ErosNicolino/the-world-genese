# Fase 11 — coordenação de matilha

## O que mudou e por quê

O `UVortexGroupCoordinator` é um subsistema do mundo que mantém membros, papéis, alvo compartilhado e um único espaço de ataque por grupo. O servidor concede o espaço a um membro, e o recupera após o impacto, morte ou saída. Há um intervalo curto antes do próximo ataque. Isso substitui o turno fixo por relógio da referência Godot e impede que vários lobos iniciem golpes juntos só por estarem à distância correta.

Cada `AVortexWolf` pode receber `PackId` e `PackSlot`. O slot zero atua como líder e uiva; os demais assumem papéis alternados de flanqueador e âncora. Quando o alvo está perto, os membros sem espaço de ataque caminham para posições distribuídas em torno dele. Lobos sem `PackId` continuam a lógica individual da Fase 10. As decisões e concessões ficam exclusivamente no servidor; estado, animação e movimento continuam replicados.

`RunPackSmoke.ps1` cria grupos de 3, 5 ou 7 em um mapa de prova, enquanto `BeginPackSmoke` confere a quantidade de membros, o limite de um espaço de ataque e o dano ao jogador.

## Validação

- Build `VortexEditor Win64 Development`: passou.
- Grupos de 3, 5 e 7 passaram no teste. Nos grupos de 3 e 7, três golpes chegaram em sequência durante 2,8 s; a vida do jogador terminou em 58, não em zero. O registro indicou no máximo um membro com o espaço de ataque em cada amostra.
- `RunWolfSmoke.ps1` individual passou após a integração. `RunMovementSmoke.ps1 -Matrix` passou em rampas, escada e piscina.
- **Pendente:** inspeção de matilha no terreno e teste host/cliente com 3–7 lobos. O teste local confirma a regra autoritativa, mas ainda não mede a apresentação e a latência no cliente. A fase não está declarada concluída sem teste do usuário no editor.

## Desempenho, risco e reversão

O grupo mantém referências fracas e a lógica dos lobos segue a 10 Hz. Com sete lobos, as consultas de visão ainda são feitas por indivíduo; não há orçamento dinâmico de percepção nem navegação em malha. Em floresta densa, os pontos de circulação podem cair atrás de árvores ou rochas. É preciso medir e resolver isso antes de usar matilhas no mundo aberto. Para reverter, volte o commit da Fase 11; o lobo individual da Fase 10 continua independente.
