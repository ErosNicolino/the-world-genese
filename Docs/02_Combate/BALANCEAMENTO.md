# Balanceamento dos atributos e das criaturas (02/11)

Os números ficam em [Data/Rpg/balance.json](../../Data/Rpg/balance.json), lido ao abrir o jogo
(`FVortexRpgCatalog::LoadBalance`). Mudar o arquivo não precisa compilar.

## Por que mudou

- **O problema:** com 7 pontos por nível, cada ponto somava um valor fixo (1 ponto = +1 de ataque). Pôr tudo num
  atributo deixava o herói desproporcional aos bichos do mesmo nível.
- **A pesquisa:** o usuário pediu para fazer como a maioria dos RPGs. A solução junta duas ideias:
  - **WoW (rating por nível):** o valor de cada ponto depende do nível do herói;
  - **League of Legends (armadura):** a defesa divide o dano em vez de subtrair.

## Os pontos: retorno decrescente que acompanha o nível

```
valor = (max + maxPerLevel × nível) × pontos / (pontos + mid × nível)
```

- **Os primeiros pontos rendem muito;** cada ponto a mais no mesmo atributo rende menos.
- **O meio da curva cresce com o nível:** pôr a mesma parte dos pontos num atributo vale o mesmo no nível 5 e no 100.
  Exemplo: tudo em ataque dá ~64% do teto, metade dá ~47%.
- **O dano base sobe com o nível** (`levelDamage`, +1 por nível, somado ao da arma). É isso que deixa o herói mais
  forte com o tempo; os pontos decidem o estilo.
- **Equipamentos e classe continuam somando direto** (a progressão pelos itens).
- **A janela do herói** mostra no aviso de cada atributo quanto o **próximo** ponto rende.

| Atributo | Curva | Tudo nele (7 pontos/nível) |
|---|---|---|
| Ataque, Poder de magia | 3 × nível, meio 4 | +1,9 × nível |
| Defesa, Resistência | 10 × nível, meio 4 | +6,4 × nível |
| Vida | 50 × nível, meio 3 | +35 × nível |
| Mana | 30 × nível, meio 3 | +21 × nível |
| Crítico | 30%, meio 2 | ~23% |
| Dano crítico | 150%, meio 2 | ~117% |
| Velocidade de ataque | 60%, meio 1,5 | ~49% |
| Recarga | 50%, meio 1,5 | ~41% |
| Roubo de vida | 12%, meio 3 | ~8% |

## A defesa divide o dano

```
dano recebido = golpe × K / (K + defesa)      K = 20 + 6 × nível do herói
```

- **Defesa igual a K corta metade do dano.**
- **Nunca fica imune e nunca morre de um golpe só** por causa dos números. Antes, a Defesa subtraía direto do golpe.
- **A Resistência (contra magia) usa a mesma conta.**

## As criaturas escalam com a região

- **O nível vem da região:** cada criatura tem o nível da área onde nasce (`VortexTerrainData::AreaLevel`; cada região
  tem a sua faixa).
- **Vida:** golpes × (`monsterHitBase` + `monsterHitPerLevel` × (nível - 1)).
- **Dano:** sobe `monsterDamagePerLevel` (14%) por nível.
- **Diferença de nível:** o `VortexRpg::GapScale` continua aumentando ou cortando o dano quando o bicho está bem acima
  ou bem abaixo do herói.

## Onde mexer

- **Os números:** `Data/Rpg/balance.json`.
- **As contas:**
  - os pontos: `FVortexRpgCatalog::PointValue`;
  - o herói: `UVortexHeroSheet::ComputeStats`, `OutgoingDamage` e `IncomingDamage`;
  - as criaturas: `AVortexMonster::BeginPlay` e `AVortexWolf`.
- **Sem o arquivo**, os pontos voltam ao linear antigo (`officialPerPoint` do `rpg.json`).
