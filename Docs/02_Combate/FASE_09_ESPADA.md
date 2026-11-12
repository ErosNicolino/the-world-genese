# Fase 9 — primeira arma: espada

## O que mudou

- A espada `Sword_Default.glb` e os clipes `Attack_01`, `Attack_02`, `Attack_03` e `Skill_Guard` vieram dos arquivos próprios do jogo Godot. O script `ImportSword.py` liga as animações ao esqueleto já usado pelo herói.
- `DA_DefaultSword` é a definição editável da arma. Guarda ID, malha, habilidade primária e secundária, clipes e tempos de cada golpe. `CreateSwordDefinition.py` recria os dados a partir de `Data/GodotWeapons/combo_profile.json` sem alterar o Godot.
- `UVortexWeaponComponent` cuida do equipamento em tempo de jogo, concede e retira as habilidades pelo GAS, replica a arma equipada e o estado da animação. A malha visual não possui colisão de dano.
- Clique esquerdo ativa o combo de três golpes. `UVortexSwordAbility` mantém a janela ativa de cada golpe no servidor, aceita uma entrada guardada para o próximo e controla um conjunto de alvos já acertados por golpe. Os tempos de 0,62 / 0,62 / 0,94 s, janelas ativas e dano 1 / 1 / 2 vêm do perfil original; alcance efetivo 4,30 m e cone 0,1 seguem `PlayerController.Melee.cs`.
- Segurar o botão direito ativa a guarda por `UVortexGuardAbility`. Nos primeiros 0,22 s, o aparo frontal anula golpes até 15 de dano; depois, a guarda deixa passar 35% (mínimo 1). Soltar encerra a guarda. Um aparo emite `Event.Combat.Parry`.
- Após o primeiro teste do usuário, os golpes passaram a se misturar apenas a partir do `Torso`, mantendo as pernas em movimento. A correção posterior do encaixe e da câmera, motivada por uma foto do usuário, está em [CORRECAO_CAMERA_ESPADA.md](CORRECAO_CAMERA_ESPADA.md).

## Por quê

A arma reúne definição, uso, visual, conjunto de habilidades e conjunto de animações, sem codificar seus tempos dentro do fluxo de combate. O servidor decide dano e defesa; a animação apenas apresenta o estado replicado.

## Validação

- `VortexEditor` compilou após as correções visuais.
- `RunSwordSmoke.ps1` passou após as últimas mudanças: três golpes reduziram a vida de 100 para 96 (1 + 1 + 2); aparo perfeito manteve 96; bloqueio tardio reduziu para 92; sem guarda, para 82. Registro: `VORTEX_SWORD_SMOKE_PASS combo=96 parry=96 block=92 release=82.0`.
- `RunCombatSmoke.ps1` passou. `RunMovementSmoke.ps1 -Matrix` passou em RAMP20, RAMP40, RAMP50, STAIRS e POOL (`MATRIX_SMOKE_PASS failed=0`).
- `RunSwordShot.ps1` gerou as capturas iniciais no `L_TerrainProto`. A foto posterior do usuário revelou que a espada ainda cruzava o pescoço em algumas poses; as novas capturas e a correção estão em [CORRECAO_CAMERA_ESPADA.md](CORRECAO_CAMERA_ESPADA.md). Capturas iniciais: [parado](../Images/phase9_sword_idle.png), [golpe parado](../Images/phase9_sword_still.png), [golpe andando](../Images/phase9_sword_walking.png).
- **Pendente:** o usuário precisa testar no editor o ângulo, o encaixe da espada em movimento e a sensação de atacar/defender. A fase ainda não está declarada concluída. A entrada da espada em cliente remoto também não foi medida diretamente nesta etapa.

## Riscos e desempenho

Uma habilidade ativa amostra alvos a cada 40 ms somente durante a janela de contato; cada candidato usa uma consulta de linha de visão. O encaixe visual importado do glTF ainda precisa de avaliação no editor, inclusive em outros ângulos e roupas. Golpes não têm som/VFX finais. O aparo emite evento para um futuro contra-ataque, mas o bônus e o atordoamento do inimigo do Godot ainda não foram portados. A escolha persistente de equipamento e a interface de inventário são de etapas posteriores.

## Reversão

Reverter o commit da Fase 9 remove a espada, seu código e seus assets; os componentes básicos de GAS e combate das Fases 7 e 8 continuam. `Config/DefaultInput.ini` local do editor não foi alterado por esta etapa.
