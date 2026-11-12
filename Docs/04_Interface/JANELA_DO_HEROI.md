# Janela do herói: níveis, atributos e inventário

**Pedido:** "faça o inventário já com I e Tab", com os níveis e os atributos funcionando (os números da raça valendo no combate) e as outras abas do Godot, no estilo do próprio Vortex.

## Como abrir

- **I** ou **Tab** abre e fecha a janela.
- **Esc**, **I** ou **Tab** fecham. As teclas **1**, **2** e **3** trocam de aba.
- Com a janela aberta, o mouse fica livre e a câmera e os golpes param. Andar continua funcionando.
- **Q** usa uma poção de vida e **R** uma de mana. O contador do HUD vem da mochila.

## As abas

| Aba | O que tem |
|---|---|
| **Personagem** | Arte e traços da raça. Os 8 atributos com o que cada um faz e o **+** para gastar pontos. Nível e barra de XP. Os números que valem no combate: vida, mana, ataque, defesa, crítico, regeneração e afinidades da raça. |
| **Mochila** | Equipamento no corpo (arma, cabeça, peito, pernas, botas, joia). 30 espaços com filtros (tudo, equipamento, armas, uso, materiais, joias). Moedas. Painel do item com descrição, atributos e o botão usar / equipar / desequipar. Clique duplo ou botão direito usa ou equipa. |
| **Habilidades** | As 4 técnicas da arma na mão (níveis 1, 4, 7 e 10), com mana, recarga e se já estão prontas. Também a lista de comandos. |

Do Godot ficaram de fora por enquanto: **Classes**, **Alquimia** e **Missões**. Elas dependem de sistemas que ainda não existem no Vortex.

## Regras (vindas do Godot)

Os dados ficam em `Data/Rpg/rpg.json`, gerado por `Tools/BuildRpgData.py` a partir de `data/items.json`, `data/progression.json`, `RaceCatalog.cs` e `StarterWeaponCatalog.cs`. O Godot é só lido.

### Nível e XP

- XP total para um nível: 100 × (nível − 1)^1,8. O nível 2 pede 100, o 3 pede 348, o 10 pede 5.229.
- Cada nível dá **2 pontos**. Subir de nível enche a vida e a mana, toca o som e mostra "NÍVEL N" no meio da tela.
- XP por criatura, como no Godot: 65% da tabela (o lobo vale 45 × 0,65 ≈ 29), +8% por nível da criatura.
- Diferença de nível: até 3 níveis abaixo do herói a criatura vale o XP cheio. Mais abaixo, cai 8% por nível. Acima do herói, vale +4% por nível.
- Todos os heróis vivos a até 40 m ganham o XP.

### Atributos

Valor do atributo = número inicial da raça + pontos gastos.

| Atributo | Efeito |
|---|---|
| **Força** | +1 de dano em todo golpe por ponto acima de 6 |
| **Destreza** | +0,75% de crítico por ponto |
| **Vitalidade** | +6 de vida máxima por ponto acima de 8; regeneração |
| **Inteligência** | Curas 3% mais fortes por ponto acima de 4 |
| **Mana** | +5 de mana máxima por ponto acima de 6 |
| **Espírito** | Mana volta mais rápido |
| **Recuperação** | Vida volta mais rápido fora de combate (6 s sem apanhar) |
| **Sorte** | Mais saque, mais moedas e +0,5% de crítico por ponto |

- **Base:** vida 100 e mana 50 (o humano). O dano crítico multiplica por 1,8.
- **Equipamento** soma os atributos oficiais dos itens: ataque, defesa, vida máx., mana máx., crítico, dano crítico, recuperação.
- **Defesa** reduz o dano recebido: × 25 / (25 + defesa).
- **Afinidades da raça:**

  | Raça | Corpo a corpo | Dano recebido | Outros |
  |---|---|---|---|
  | Elfo | −20% | — | — |
  | Diabo | +10% | +10% | — |
  | Anão | +20% | −10% | não usa magia |
  | Bárbaro | +35% | −20% | — |
  | Morto-vivo | — | — | regenera 2× |

  Todas aparecem na aba Personagem e entram no `UVortexCombatComponent::ApplyHit`.
- Os golpes críticos aparecem maiores e laranja, com "!".

### Saque e mochila

- **Lobo:** 1 ou 2 peles de lobo. Moedas: 2 a 5 + o nível do lobo, com a Sorte aumentando.
- Saque e moedas vão direto para a mochila de quem venceu. Os avisos sobem à direita do HUD.
- **Herói novo:** a mochila inicial do Godot (3 poções de vida, 2 de mana, 1 de ervas, 4 pães) e a Espada de Cinzas na mão.
- **Armas:** só a família da espada tem golpes no Vortex por enquanto. Equipar outra família avisa que os golpes dela ainda não chegaram. Elas vêm com a escolha de arma (o Despertar).

## Rede e save

`UVortexHeroSheet` (`Source/Vortex/*/Rpg/`) é um componente do herói:

- O servidor é o dono dos números.
- O dono do herói recebe tudo replicado e os outros jogadores só veem o nível.
- O cliente só **pede**: gastar ponto, usar, equipar, desequipar, mover, poção rápida.
- Ao entrar, o dono manda a ficha salva (`ServerLoad`). O servidor confere tudo: pontos dentro do que o nível deu, itens que existem, pilhas dentro do limite, cada peça no lugar certo.
- Cada máquina grava o próprio herói (`UVortexPlayerSave` schema 3: `Experience`, `AttributePoints`, `Bag`, `Equipped`, `Coins`), 1,5 s depois da última mudança. Saves antigos (schema 1–2) começam como herói novo.

## Arquivos

| Arquivo | Para que serve |
|---|---|
| `Source/Vortex/Public/Rpg/VortexRpg.h`, `Private/Rpg/VortexRpg.cpp` | Catálogo: itens, saque, XP, efeitos, raças, técnicas e ícones |
| `Source/Vortex/Public/Rpg/VortexHeroSheet.h`, `Private/Rpg/VortexHeroSheet.cpp` | Ficha do herói (rede, save, regeneração, XP e saque) |
| `Source/Vortex/Public/Framework/VortexHeroWindow.h`, `Private/Framework/VortexHeroWindow.cpp` | A janela em Slate |
| `Tools/BuildRpgData.py` | Gera `Data/Rpg/rpg.json` e `icons.txt` |
| `Tools/ImportItemIcons.py` | Importa 144 ícones para `/Game/Vortex/UI/Icons` (sem filtro, sempre residentes) |

## Como testar

- **No editor:** jogar, vencer um lobo (ganha XP, pele e moedas) e apertar I. Depois:
  - gastar pontos com o **+**;
  - na Mochila, clicar duas vezes numa poção;
  - equipar e desequipar;
  - apertar Q e R no jogo.
- **Automático:** `Tools/RunHudShot.ps1` vence um lobo, sobe ao nível 3, gasta pontos, usa poção e equipa o capuz de lobo. Mostra `VORTEX_RPG_SMOKE PASS` e tira as fotos `hud_rpg_nivel`, `hud_janela_personagem`, `hud_janela_mochila` e `hud_janela_habilidades`.
