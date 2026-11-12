# Fase 7 — base do Gameplay Ability System

## O que mudou

- O personagem tem `AbilitySystemComponent` próprio, inicializado para autoridade e cliente, com replicação de efeitos em modo Mixed.
- `UVortexAttributes` define vida, mana e vigor, seus máximos, limites e notificações de rede.
- `UVortexRecoverAbility` é a primeira habilidade nativa. A tecla **H** pede recuperação ao servidor: custa 10 de mana, cura 20 de vida e emite `Event.Ability.Recover`. Ela não ativa com vida cheia, sem mana suficiente ou após a morte.
- O custo e a cura são modificadores de um `GameplayEffect` instantâneo, não alterações diretas no personagem. As tags `Ability.Recover` e `Event.Ability.Recover` ficam registradas no código.

## Por quê

Esta é uma ação pequena e verificável que atravessa input, habilidade, efeito, atributos e evento. A mesma base servirá às armas, aos inimigos e aos estados persistentes. O ator é dono do componente por ora; a futura troca de avatar com preservação de atributos exigirá decidir se ele migra para `PlayerState`.

## Validação

- `VortexEditor` compilou com um trabalho por vez em 04/10/2026.
- `RunMovementSmoke.ps1` passou: `MOVEMENT_SMOKE_PASS`, subida 294,2 cm, salto 503,1 cm e pouso confirmado.
- Testar em editor, inclusive com dois jogadores, H com vida menor que o máximo e mana suficiente. Confirmar cura +20 até o máximo e custo de 10, visíveis em ambos os lados. Confirmar que H com vida cheia não consome mana.
- Rodar a regressão de movimento por esta mudança tocar `AVortexCharacter`.

**Status:** implementação em validação técnica. A fase só será declarada concluída depois do teste do usuário no editor.

## Riscos e desempenho

O sistema de habilidades adiciona um componente replicado e seis atributos por personagem. O custo deve ser medido quando houver vários inimigos. A habilidade de recuperação ainda não tem interface, som ou animação final; a tecla H é temporária para teste.

## Reversão

Reverter o commit da Fase 7 restaura o personagem anterior. `Config/DefaultInput.ini` local do editor não foi alterado por esta etapa.
