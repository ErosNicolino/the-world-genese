# Crítico que parece crítico e níveis do começo (23/10)

## Pedidos

1. O crítico precisa parecer crítico: um símbolo, letra maior e vermelha.
2. Perto da base, os monstros apareciam já no nível 5. O começo tem que ser nível 1 e o jogo, bem balanceado.

## O que estava errado

- **Crítico:** o aviso já chegava certo do servidor (`MulticastConfirmedHit` com `bCrit`), mas o número era só um pouco maior (112 contra 80), laranja, com um "CRÍTICO" pequeno em cima. Parecia um golpe comum de outra cor.
- **Níveis perto da vila:**
  - **De onde vinham:** os postos fixos do vale (`Data/Population/creature_posts.json`) são do Godot, de antes das muralhas. Os de nível 1–3 (gotas e cogumelos a 30–67 m do centro) ficavam dentro da cidade, e o jogo os apagava (a regra "dentro das muralhas e 15 m em volta não nasce bicho").
  - **Resultado:** os primeiros que sobravam estavam a 78–110 m e já eram nível 4–5.
  - **A curva:** era "1 + 1 a cada 42 m desde o centro", e também chegava a ~3–4 logo nos portões.

## O que mudou

### Crítico

O crítico agora tem quatro marcas:
- **Número:** vermelho vivo, 136 (era 112), que nasce piscando em branco e treme mais forte.
- **Símbolo:** uma explosão de 12 pontas atrás do número, em amarelo e laranja com borda escura. Ela estoura junto com o número, gira devagar e some antes dele.
- **"CRÍTICO!"** em amarelo, maior, em cima.
- **"!"** no fim do número.

O golpe comum continua amarelo, 80, e o dano no herói continua vermelho escuro, 52 (`AVortexAimHUD::DrawDamage`).

### Níveis

**Curva nova** (`FVortexTerrainData::AreaLevel`): a vila e o anel logo fora das muralhas (até ~110 m do centro) são nível 1. Depois, +1 a cada 40 m.

| Distância do centro da vila | Antes | Agora |
|---|---|---|
| portões (~80–110 m) | 3–4 | 1 |
| 150 m | 5 | 3 |
| 250 m | 7 | 5 |
| borda do vale (~300 m) | 8 | 6–7 |
| 1 km | 25 | 24 |

- **Fora do vale:** o mapa continua como antes. A faixa de cada região (Aurivale 10–22, Solcânion 20–35, Nivaluz 45–60...) é o piso, e longe da vila as duas curvas quase se encontram.

**Postos fixos do vale** (`UVortexCreatureSubsystem::LoadFixed`):
- **Os que caíam dentro das muralhas** (gotas e cogumelos) não somem mais. Saem para o anel de fora, a 80–100 m do centro na mesma direção. Se ali for água, giram até achar chão seco. São os bichos do começo, perto dos portões.
- **O nível de cada posto** segue a curva nova. Quem era mais forte que os vizinhos no Godot (elite) continua com a mesma diferença.
- **Chefes:** não mudam (o Colosso Rúnico continua nível 12).

**Como fica o vale** (simulação dos 43 postos):

| Distância | Antes | Agora |
|---|---|---|
| até 100 m | nenhum bicho até 78 m; depois nível 4–5 | gotas e cogumelos nível 1–2; lobos e morcegos nível 2–3 |
| 100–140 m | 5–6 | 3–4 |
| 160–200 m | 6–8 | 4–6 |
| 200–270 m | 8–10 | 6–8 |

## Fotos

| | Foto |
|---|---|
| Golpe comum (nível novo) | [foto](../Images/critico_niveis/golpe_comum.png) |
| Crítico novo | [foto](../Images/critico_niveis/depois_critico.png) |

Não há foto do crítico antigo: o teste de fotos não tinha crítico. Agora o `RunHudShot` dá um crítico de 47 no lobo da foto "combate".

## Validação

- `VortexEditor` compilou.
- **Passaram:** `RunHudShot` (com o crítico na foto), `RunPopulationSmoke`, `RunCreatureSmoke`, `RunRegionSmoke` e `RunBanditSmoke`.
- **Níveis:** conferidos por simulação dos 43 postos com a regra nova (tabela acima). O lugar exato dos postos movidos depende da água em volta; confira jogando.

## Riscos

- **Começo mais fácil:** o primeiro nível do herói vem mais devagar (bicho de nível 1 dá menos XP). Se ficar lento, o ajuste é o `70.f` (raio do nível 1) ou o `40.f` (metros por nível) em `AreaLevel`.
- **Postos movidos:** podem cair numa encosta ou num caminho perto dos portões.

## Para você testar jogando

- **Crítico:** dar críticos (o contra-ataque depois de um aparo perfeito sempre é crítico).
- **Níveis:** sair pelos portões e andar até a borda do vale, olhando os níveis nas placas. Perto da vila devem ser 1–2.

## Reversão

- **Curva:** em `AreaLevel`, volte para `RoundToInt(1 + distância / 42)`.
- **Postos:** em `LoadFixed`, volte para `if (!Post.bBoss && Walls.IsInside(Post.At)) continue;`.
