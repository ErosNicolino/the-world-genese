# Continuar depois: economia e monstros

Atualizado em 11/10/2026 (parado a pedido do usuário). O resumo da etapa está em [ARSENAL_E_LOJAS.md](ARSENAL_E_LOJAS.md).

## 1. Já feito

- **Arsenal:** compilado e aprovado pelo usuário no jogo.
- **Painel do item:** elemento, conjunto e valor.
- **Lojas:** os 9 comerciantes do Godot.
- **Mapas das caravanas:** revelam rotas na carta (commit `ddf4d70`).
- **Licenças da Fab:** o usuário confirmou (`Docs/05_Referencias/CREDITOS_FAB.md`).
- **Bandidos e acampamentos (commit seguinte ao `dfc2fbe`):** os testes automáticos passam, mas ainda falta a revisão visual (ver a seção 2).
  - **Tipos:** 4 espécies novas em `VortexMonsterSpecies.cpp`, todas com o boneco do aventureiro (`bHumanoid`) e a arma do tipo (`Weapon` = DA_<tipo>).
    - `Bandit`: espada, combo de 3 e estocada. O primeiro do bando é o CAPITÃO BANDIDO, com 1,6 vez a vida e malha cor de vinho.
    - `BanditShield`: espada e escudo. Em guarda, golpes pela frente passam só 40% (`UVortexCombatComponent::bShieldFront`).
    - `BanditBrute`: marreta, 1,45 vez maior, pancada no chão com aviso.
    - `BanditArcher`: arco, guarda distância (`KeepAway`) e atira flechas de verdade (`EVortexMoveKind::Shoot` com `AVortexProjectile`).
  - **Roupa:** `UVortexDressComponent` (`Creatures/VortexDress.*`) veste com o guarda-roupa do herói. `BanditLook` é a roupa escura do Godot (`TownFolk.BanditLook`), sorteada pela semente do posto (`LookSeed`, replicada).
  - **Animação:** `VortexMonsterAnimInstance` pega as posturas e os golpes (Swing0..2) do DA da arma; Hit e Death vêm do aventureiro.
  - **Para o jogo, todos são "Bandit"** (`GetSpecies`): saque `bandit_bandana`, XP e missões.
  - **Acampamentos:** `UVortexCreatureSubsystem::BanditCampAt`, ~1 a cada 4 quadras de 72 m, longe da cidade, com bando de 3 a 4 e nível do lugar (mínimo 2).
  - **Decoração:** `AVortexLandmarks::UpdateCamps/BuildCamp` monta a fogueira acesa, os troncos de assento, os caixotes e às vezes uma carroça quando alguém chega a 200 m.
  - **Teste:** `Tools/RunBanditSmoke.ps1` passou todos os passos.
    - os 4 nascem;
    - o bando fere o herói;
    - o arqueiro atira flecha;
    - o escudo segura: 5,5 de dano pela frente contra 10,9 pelas costas;
    - vencer dá XP;
    - o acampamento é montado com o bando em volta.

## 2. Onde parou (fazer primeiro)

> **24/10: feito.** As roupas foram conferidas e corrigidas (as mangas eram verdes), o teste está na suíte, o relatório está em [BANDIDOS](BANDIDOS.md), e o baú guardado do acampamento (item da seção 3) foi feito. Falta só conferir no editor a pegada do escudo e da flecha.

1. **Conferir as roupas dos bandidos na foto `Saved/Screenshots/bandido_quatro.png`** (a câmera foi refeita para mostrar os 4 de frente; essa foto ainda não foi olhada). Na foto anterior pareciam vestir a roupa padrão do herói (túnica verde). A causa provável foi corrigida (`ApplyHumanoid` agora roda durante o `BeginPlay`: `IsActorBeginningPlay`), mas falta confirmar na imagem que estão de roupa escura, tapa-olho etc.
2. **Conferir no editor:**
   - a pegada da arma, o escudo no braço esquerdo (`Hand_L`) e se os golpes tocam;
   - o tamanho do brutamontes;
   - o arqueiro recuando;
   - a flecha saindo da mão.
3. **Pôr `RunBanditSmoke.ps1` no `Tools/RunAllTests.ps1`** e rodar só os testes ligados: bandidos, criaturas e lobo/matilha.
4. **Escrever o relatório** `Docs/02_Combate/BANDIDOS.md` e pôr a linha no `Docs/README.md`.

## 3. Ainda falta (pedidos do usuário)

- **Baús guardados:** baús espalhados, protegidos pelo bando do acampamento ou por monstros.
  - Os caixotes do acampamento podem virar um baú de verdade (reaproveitar o baú das cabanas: `VortexWorldFeatures`, saque da região rolado no servidor, abre com E).
- **Monstros:**
  - IA mais inteligente e com comportamento próprio por espécie;
  - variantes: goblin mago, lobo alfa com chifres, golem de fogo, morcego cuspidor;
  - monstros das outras regiões;
  - bestiário revelado ao derrotar cada um.
- **Forja:** fica com o usuário.

## Avisos

- O usuário prefere poucos testes demorados: rodar só os ligados ao que mudou; a bateria completa (`Tools/RunAllTests.ps1`, ~35 min) só no fim de uma etapa. Na última, "Movimento" e "Rede: movimento" falharam uma vez e passaram de novo (intermitentes).
- A compilação é lenta (um núcleo, `-MaxParallelActions=1`). Mudar `VortexRpg.h` ou `VortexMonster.h` recompila muita coisa.
- **AccuRig:** o usuário instalou. Só tem interface de mouse; se precisar, passe os passos ao usuário.
- **Blender:** o MCP do Blender não conectou nesta sessão.
