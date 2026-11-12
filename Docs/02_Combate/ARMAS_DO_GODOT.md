# As 25 armas do Godot no Vortex

**Pedido:** "precisa dos golpes de verdade das outras famílias… já coloque as outras, pode fazer uma por uma".

## De onde vem cada coisa (o Godot é só lido)

| O quê | Fonte no Godot | No Vortex |
|---|---|---|
| Os 24 tipos novos (golpes, especial, ala, magia) | `data/weapon_types.json` | `Data/Rpg/weapons.json` (`Tools/BuildWeaponData.py`) |
| Golpes das armas iniciais (arco, lança, martelo, cajado) | `Game/Player/WeaponStyles.cs` | copiados no `BuildWeaponData.py` |
| Pegada na mão (inclinação, armas de par) | `Art/Adventurer/WeaponDefinition.cs`, `AdventurerEquipment.cs` | `Tools/ConvertGodotWeapons.py` |
| Modelos | `Assets/Equipment/Weapons/<item>.glb` (o item comum de cada tipo) | `/Game/Vortex/Weapons/Models/W_<tipo>` |
| Animações (152 clipes: ataques, postura, carga, especiais) | `Art/Adventurer/Adventurer.glb` | `/Game/Vortex/Weapons/Clips` (o mesmo conversor da espada) |

**`Tools/ImportWeapons.py`** cria no editor:
- uma definição por tipo (`/Game/Vortex/Weapons/Types/DA_<tipo>`);
- o material das armas (`M_WeaponVertexColor`: cor por peça no vértice, runas e cristais brilham);
- o brilho dos projéteis (`M_ProjectileGlow`).

A espada continua no `DA_DefaultSword`.

## Como cada golpe acerta (modos do Godot)

| Modo | O que faz | Exemplos |
|---|---|---|
| `melee` | corte no alcance e no cone à frente durante a janela do golpe | espada, lança, machado, katana, adagas |
| `slam` | pancada no chão: todos no raio em volta | 3º golpe do martelo, das manoplas e da maça |
| `arrow` | flecha que voa reto até o alcance e para no primeiro alvo | arco, besta |
| `bolt` | dardo de magia | cajado, varinha, pistola, livro, orbe, chakrams |
| `orb` | esfera que explode no raio ao bater | 3º golpe do cajado, catalisador |
| `burst` | explosão num ponto à frente | 3º golpe do livro e do alaúde |

- **Projétil:** `AVortexProjectile` (`Source/Vortex/*/Combat/VortexProjectile.*`). O núcleo brilha na cor do elemento (natureza, jade, sagrado, arcano, aço…) e o rastro é do pacote **ArrowTrail**, um por elemento.
- **Rede:** o servidor decide os acertos; cada máquina desenha a sua cópia.
- **Postura:** cada família tem parado, andar e correr próprios (lança, cajado, livro, orbe…). As armas de par levam a cópia espelhada na mão esquerda.
- **Velocidade do clipe:** clipes reaproveitados de outras armas tocam mais rápido (`ClipSpeed` do Godot).

## Especial do botão direito

| Tipo | Situação |
|---|---|
| **charge** (segurar e soltar), igual ao Godot. A carga cheia leva 1,2 s; o dano vai do mínimo ao máximo do Godot. | ✅ |
| • arco: flecha carregada maior e mais rápida | ✅ |
| • besta e pistola: atravessam a fila | ✅ |
| • martelo: impacto sísmico em volta | ✅ |
| • lança: investida que avança perfurando | ✅ |
| • espadão: corte largo | ✅ |
| • livro: círculo na mira | ✅ |
| • foice: giro em volta | ✅ |
| **espada**: corte à distância | ✅ |
| **Os outros** (bloqueio, execução, redemoinho, gancho, passo assassino, contra-golpe, consagrar, ricochete, canalização, sobrecarga, égide, acorde, aprisionar, névoa) | 🟡 **provisório**: um golpe forte da própria arma (o mais forte do combo, maior e com dano dobrado). Entram um por um. |

Também valem para todas as armas:
- **F:** defesa, como na espada.
- **Anão:** não equipa armas de magia.
- **Ficha:** equipar a arma na mochila troca a arma na mão do herói.

## Como testar

- **No jogo:** equipe uma arma na mochila (I). O Despertar (coliseu) também dá as armas.
- **Automático:** `Tools/RunWeaponShot.ps1 -Types arco_de_betula,besta,...`. Sem `-Types`, testa arco, lança, martelo e cajado.
  - Para cada arma: equipa, fotografa a pose, ataca um lobo de treino com o combo e solta o especial.
  - Mostra `VORTEX_WEAPON_SHOT <tipo> vida_do_lobo=500->N ok`.
  - Rodado com as 24 armas: **todas acertaram**.
