# Fase 8 — fluxo de combate autoritativo

## O que mudou e por quê

`UVortexCombatComponent` recebe um pedido de golpe feito por uma habilidade, seleciona alvos na área e valida no servidor facção, vida, invulnerabilidade durante esquiva, alcance, direção e linha de visão. O componente aplica `UVortexDamageEffect` pelo GAS com magnitude definida pelo servidor. Depois do dano, envia os eventos `Event.Combat.Hit` e, quando cabível, `Event.Combat.Death`, e transmite o resultado confirmado para apresentação local (`OnConfirmedHit`). O conjunto de alvos já atingidos é controlado pelo golpe chamador.

A animação e futuros VFX, som, câmera e interface devem observar esses eventos; não calculam dano. O fluxo já é utilizável por jogadores e futuros inimigos que implementem `IAbilitySystemInterface` e tenham facção.

## Validação

- `VortexEditor` compilou em 04/10/2026.
- `Tools/RunCombatSmoke.ps1` passou (`VORTEX_COMBAT_SMOKE_PASS`): rejeitou facção igual, dano e alcance/ângulo inválidos; aceitou alvo hostil frontal, verificou vida 100→80 e exercitou a recuperação da Fase 7 (vida 70→90, mana 50→40).
- `RunMovementSmoke.ps1` passou (`MOVEMENT_SMOKE_PASS`), pois a verificação adicionou um gancho no GameMode.
- Teste do usuário no editor ainda obrigatório antes de declarar a fase concluída.

## Riscos e desempenho

O golpe faz uma consulta espacial e uma linha de visão por candidato. O limite de 10 m e dano máximo 10000 são guardas técnicas; os valores efetivos vêm da definição de cada arma. A facção simples Player/Hostile/Neutral será ampliada quando houver grupos, aliados e regras PvP. A comunicação visual é uma interface de evento: efeitos finais serão ligados em outra etapa.

## Reversão

Reverter apenas o commit da Fase 8 remove componente, efeito de dano e teste de combate. A Fase 7 permanece independente.
