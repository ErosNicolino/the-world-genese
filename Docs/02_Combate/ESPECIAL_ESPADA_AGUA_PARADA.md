# Especial da espada, barra nova, água parada e menos borboletas

Status: **aguardando teste no editor**.

## O que mudou

### Controles

Toda arma passa a ter o combo no **botão esquerdo** e um **especial no botão direito**.

| Ação | Tecla |
|---|---|
| Golpes normais (combo) | Botão esquerdo |
| Especial da arma | Botão direito |
| Defesa (e mira por cima do ombro) | Segurar **F** (antes era o botão direito) |
| Habilidades | 1, 2, 3, 4 |
| Poção de cura | Q |
| Poção de mana | R |
| Teste de roupas (só no desenvolvimento) | Teclado numérico (antes 1 a 7) |
| Olhar o rosto (só no desenvolvimento) | F9 (antes F) |

### Especial da espada: corte à distância

- O herói faz o golpe mais forte do combo e solta uma **lua crescente de energia azul**.
  - Ela corre reto para onde a mira aponta: cerca de 26 m a 26 m/s.
  - Atravessa inimigos, e cada um leva 16 de dano uma vez.
  - Passa pela vegetação e se desfaz ao bater no terreno ou numa construção.
- **Visual:** a lua tem a borda da frente quase branca e o corpo azul, com fiapos de energia correndo pelo arco (ruído do pacote Vefects das referências). Dois rastros vêm atrás, e uma luz azul ilumina o chão e as plantas na passagem.
- **Som:** `SW_MagicWeapon_Swing_1`, do FreeModularMagicSFX das referências.
- **Recarga:** 3,5 s.
- **Rede:** o servidor decide os acertos, e cada máquina desenha a onda pelo mesmo caminho.
- **Outras armas:** cada uma configura o seu especial em `UVortexWeaponDefinition`:

  | Campo | O que define |
  |---|---|
  | `SpecialSwingIndex` | qual golpe do combo é animado |
  | `SpecialRelease` | o instante em que solta |
  | `SpecialDamage` | o dano |
  | `SpecialCooldown` | a recarga |
  | `SpecialProjectile` | o que é lançado |

### Barra de ações nova

- Seis espaços: golpe, especial (com véu e contagem da recarga) e as habilidades 1 a 4. As habilidades ainda estão vazias, só com o número.
- Ao lado ficam as duas poções na moldura do Godot: Q (cura, vermelha) e R (mana, azul). Ficam apagadas e com 0 até existir uma bolsa.
- Em cima da barra ficam as cargas de esquiva (Ctrl) em losangos. A carga que está voltando enche aos poucos.

### Água

- Os lagos e poças agora ficam **parados**.
- O `M_Lake` mexia a superfície com três ondas grandes e usava refração de tela, que borrava e "rasgava" ao girar a câmera. As duas coisas foram desligadas no `MI_VortexLake`.
- Ficam só marolinhas finas e lentas, e o brilho do sol ficou mais suave.
- O reflexo do céu e das margens continua.

### Borboletas

Caíram de 64 para 20 em volta da câmera.

## Arquivos

- **Novos:**
  - `Combat/VortexSlashWave.h` e `.cpp`
  - `Tools/CreateSlashWave.py` (gera `/Game/Vortex/Combat/M_SlashWave`)
- **Alterados:**
  - `VortexCombatComponent`: `CanHarm` e `ApplyHit` separados do golpe corpo a corpo.
  - `VortexWeaponComponent` e `VortexWeaponDefinition`: o especial.
  - `VortexCharacter`: teclas.
  - `VortexAimHUD`: a barra.
  - `Tools/ImportHud.py`: novos ícones `ability_flying_slash`, `potion_red`, `potion_blue` e a moldura `potion_slot`; os ícones antigos de defesa, esquiva, recuperar, correr e pular saíram.
  - `Tools/CreateVortexLake.py`
  - `VortexForestLife.cpp`
- **Copiados das referências, tal como estão e no mesmo caminho do pacote:**
  - `Content/Vefects/Easy_Impact_Frames/VFX/Extras/Textures/Shockwaves/T_VFX_Easy_Shockwaves_Noise_01`
  - `Content/FreeModularMagicSFX/D1_Spell/Mono/SW_MagicWeapon_Swing_1`

## Testes

- `RunHudShot.ps1`:
  - a foto `hud_especial` mostra o corte voando;
  - `VORTEX_SLASH_HIT VortexWolf_1 dano=16`;
  - a recarga aparece na barra.
- `RunDayNightShot.ps1 -Only lago`: o lago sai liso, com reflexo limpo.
- `RunCombatSmoke`, `RunSwordSmoke` (combo, defesa perfeita, bloqueio), `RunWolfSmoke` e `RunFrameworkNetworkSmoke`: todos passaram.

## Riscos

- O especial ainda não foi testado num cliente em rede de verdade, só no servidor ou anfitrião. Ele usa a mesma chamada ao servidor que o golpe normal.
- As habilidades 1 a 4 e as poções ainda não fazem nada, porque esses sistemas ainda não existem.
- O especial sai do golpe mais forte do combo. Se a animação desse golpe for curta, o corte pode sair um pouco cedo; dá para ajustar com `SpecialRelease`.

## Como voltar atrás

Reverta este commit. A água volta a ter ondas e o botão direito volta a ser a defesa.
