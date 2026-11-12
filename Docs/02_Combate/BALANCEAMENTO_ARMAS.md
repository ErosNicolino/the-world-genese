# Balanceamento das armas: a conferência (07/11)

O usuário: "veja os danos e atributos de cada arma, se estão relevantes para cada tipo, balanceados, comparado a outros
jogos". A conta está em `Tools/Balance/WeaponDps.py` (só lê os dados e imprime). Nada foi mudado ainda.

## Como o jogo calcula hoje

```
golpe = (dano do golpe + 1 + Força acima da base + Ataque + nível - 1) x escala do tipo
```

- O **dano do golpe** vem do `DA_<tipo>`: espada 1, 1, 2; martelo 2, 2, 4; adagas 1, 1, 2; lança 1, 1, 3; arco 1 (x1,5
  carregado); cajado 1, 1, 3.
- O **Ataque** é o atributo da arma: na espada, 2 na Normal e 14 na Divina.
- O **nível** soma 1 por nível (`levelDamage`).
- Esses dois entram **somados**. No nível 30 o bônus já vale uns 40, e o "1, 2 ou 4" de cada golpe some no meio.
- A **escala do tipo** (`VortexWeaponKit::PrimaryDamageScale`) é martelo 0,62, adagas 0,61 e lança 0,66. Ela multiplica
  o golpe inteiro. No arco (1,5) e no cajado (1,4), multiplica só o "1" do golpe.
- Os **poderes** multiplicam só o dano do 1º golpe ("scale" x 1) e depois somam o bônus. Um poder de força 4,5 dá
  4,5 + 40, quase igual a um golpe comum de 1 + 40.

## O que a conta mostra

**1. Os poderes de um golpe só valem um golpe comum.** No nível em que cada raridade aparece:

| Poder | Quanto vale (em golpes de espada) |
|---|---|
| Asa da Fênix, Gênese, Coroa do Sol, Juramento, Monolito, Raiz... (avanço, golpe no chão, onda, golpe de luz) | **1,1** |
| Safira, Tridente, Fenda, Trovão, Maré Baixa (runa, raio, onda de água) | **0,9** |
| Todos os leques de flechas e as flechas que atravessam ou explodem | **1,0 a 1,2** |
| Presa da Serpente (7 botes), Lágrima do Inverno (6 cortes), Ceifadora (5) | 5 a 7 |
| Estrela Guia e Arco das Estrelas (estrelas por 16 s) | **12,7** |
| Árvore do Mundo (chuva de 40 flechas) | 9,9 |

Ou seja, o poder "grande" da Divina é um golpe normal com recarga de 10 s, e só os de muitos golpes valem a pena.

**2. O tipo de arma quase não muda o golpe.** Dano por segundo num alvo, comparado à espada:

| Raridade | espada | adagas | martelo | lança | arco | cajado |
|---|---|---|---|---|---|---|
| Normal | 100% | 71% | 65% | 54% | 65% | 89% |
| Raro | 100% | 92% | 57% | 62% | 70% | 99% |
| Lendário | 100% | 91% | **47%** | 56% | 70% | 83% |
| Divino | 100% | 91% | **44%** | 54% | 70% | 80% |

- **O martelo bate pela metade da espada**, e o "golpe pesado" (dano 4) quase não pesa mais que o leve (2).
- **As adagas**, de alcance curto, ficam abaixo da espada. Deviam ficar acima: é o risco delas.
- **O cajado** bate quase como a espada, de 34 m de distância.
- **A causa:** as escalas do tipo (0,62 no martelo) foram feitas para uma conta que **multiplica** (a do Godot). Com o
  bônus somado, elas só cortam o dano. Em 02 e 03/11 o arco e o cajado subiram para 1,5 e 1,4 para compensar o mesmo
  problema.

**3. Coisas que não fazem efeito:**
- O "Ataque" dos cajados não faz nada: a magia usa o "Poder de magia" (skill).
- Os poderes mágicos das espadas e dos arcos (onda, runa, raio, estrelas, muralha, bola de fogo) não usam o Ataque da
  arma (eles contam o Poder de magia, que espada e arco não dão). Quanto mais rara a arma, mais fracos eles ficam perto
  dos outros.

**4. A raridade pesa pouco no fim do jogo.**
- No nível 1, a Divina bate 3,8 vezes a Normal; no 50, só 1,25 vez (o nível domina).
- Nos jogos parecidos, a raridade alta no mesmo nível rende +50% a +100%.

**5. Faltam armas em alguns tipos:**
- Lança: só a Normal.
- Adagas: até a Épica.
- Martelo e cajado: até a Única.

Sem arma alta nesses tipos, não dá para jogar com eles até o fim.

## Como os outros jogos fazem

| Jogo | Como calcula |
|---|---|
| Monster Hunter | ataque da arma x "valor do movimento" de cada golpe (o golpe pesado do martelo vale 3 a 4 leves) |
| Dark Souls / Elden Ring | (dano da arma + atributos) x valor do movimento |
| Diablo / Path of Exile | o poder é "X% do dano da arma" |
| Genshin | o talento é "X% do ATQ" |
| League of Legends | base + Y% do dano de ataque |

O que todos têm em comum:
- O que é **somado** é pequeno, e o **golpe e o poder multiplicam** o total.
- Por isso:
  - a arma pesada bate pesado, e a rápida bate mais vezes;
  - o dano por segundo dos tipos fica perto (dentro de uns ±20%);
  - o poder grande vale de 3 a 6 golpes comuns, com recarga de 8 a 15 s;
  - as armas de longe dão de 70 a 85% do dano das de perto.

## A proposta (precisa do OK)

1. **O golpe e o poder multiplicam** (uma linha na `OutgoingDamage`):
   ```
   golpe = dano do golpe x (1 + Força acima da base + Ataque + nível - 1) x escala do tipo
   ```
   - O 1º golpe da espada (dano 1) fica igual ao de hoje, e a vida dos bichos foi feita em cima dele.
   - Os números de cada golpe voltam a valer: o pesado do martelo vale o dobro do leve.
   - Os poderes passam a valer o que o "scale" diz: Gênese 7,2 golpes, avanço 3,5, botes 1,1 cada.
2. **As escalas do tipo voltam ao que eram para essa conta**, calculadas para:
   - adagas 110% da espada e martelo 100% (lento, mas o pesado acerta em área e dá defesa);
   - lança 95% (alcance) e arco e cajado 75% (de longe).

   | Tipo | Hoje | Proposta |
   |---|---|---|
   | martelo | 0,62 | 0,62 (fica) |
   | adagas | 0,61 | 0,71 |
   | lança | 0,66 | 0,79 |
   | arco | 1,5 | 0,9 |
   | cajado | 1,4 | 0,7 |

   Vão para um arquivo de dados (`Data/Rpg/weapons.json`), para ajustar sem compilar.
3. **Os poderes das armas contam o dano da arma.** A espada conta o golpe de perto e o arco o de longe, mesmo quando o
   poder é de fogo, água ou raio. O elemento continua no efeito: queimar, congelar, o raio que salta.
4. **Os poderes de muitos golpes**, que passam a valer muito, descem um pouco:
   - estrelas 0,9 → 0,5 por estrela;
   - chuva 0,3 → 0,2 por flecha.

   Com isso, a recarga longa (20 s) deles casa com o valor.
5. **Opcional:** o Ataque por raridade subir mais nas altas (Divina 14 → 20) e as armas que faltam (lança, adagas, martelo
   e cajado altos) serem feitas no arsenal.

**Cuidados:**
- Com a conta que multiplica, o 3º golpe dobra, e a espada mata uns 25% mais rápido. Para manter o ritmo,
  `monsterHitBase` 3 → 3,75 (dados).
- As magias de classe também passam pela `OutgoingDamage`: precisam ser conferidas na mesma rodada.

## Aplicado (07/11, à noite)

O usuário: "faça o que achar melhor, e faça as mesmas armas com níveis diferentes como os jogos fazem, com restrição de
uso por nível que trava a arma, aumentando o dano, bem balanceado com base em outros jogos".

- **O golpe da arma multiplica**
  - `golpe = dano do golpe x (1 + 1 + Força + Ataque + nível - 1) x escala do tipo`.
  - Vale para todas as fontes de dano das armas:
    - os golpes normais;
    - os secundários e os poderes;
    - as flechas e os tiros (marcados "arma");
    - os desenhos que ferem (`AVortexWeaponFx`);
    - a onda do corte e o Soco Sério.
  - É feito pelo `DamageScale` do `ApplyHit`: a `OutgoingDamage` continua igual.
  - As magias de classe (números absolutos), os venenos e as passivas da armadura continuam somando, como antes.
- **A escala de cada tipo** ficou no `Data/Rpg/balance.json` (`weaponDamageScale`):
  - espada 1, adagas 0,71, martelo 0,62, lança 0,79, arco 0,9, cajado 0,7;
  - o dano por segundo de cada um, perto da espada: adagas 109%, martelo 100%, lança 96%, arco 74%, cajado 78%.
- **Os poderes contam o dano da arma:**
  - a espada conta o golpe de perto e o arco o de longe, mesmo nos poderes de elemento (`VortexSwords::PowerKind`);
  - a Maré Baixa conta o golpe de perto.
- **Os poderes de muitos acertos desceram:** estrelas 0,9 → 0,5, muralha 1,2 → 0,6, chuva de flechas 0,3 → 0,2.
- **O Ataque dos cajados saiu:** ele não fazia nada, e o Poder de magia cresce no lugar dele.
- **Nível de item** (como Diablo e WoW):
  - **Formato:** a mesma arma ou armadura existe em qualquer nível, `"<id>@<nível>"`, montada pelo catálogo
    (`FVortexRpgCatalog::Item`). O save não muda de formato.
  - **Ataque:**
    - `Ataque = (base + por nível x nível) x fator da arma`, com os números no `balance.json`, `itemLevel`;
    - no mesmo nível, a Divina dá uns 1,8x o golpe da Normal;
    - 10 níveis a mais dão uns +9%.
  - **Outros atributos:** defesa, vida e mana crescem 10% por nível; crítico, velocidade e roubo de vida não crescem.
  - **Trava:** só veste quem tem o nível. A mochila recusa, e o detalhe do item mostra em vermelho
    "precisa do nível 78 (você está no 40)".
  - **Saque:** o equipamento que os bichos derrubam cai no nível do bicho (de 2 abaixo a 1 acima).
  - **Itens sem "@"** (os da loja, os iniciais e os do save antigo) ficam no nível natural da raridade:
    Normal 1, Incomum 5, Raro 12, Épico 20, Lendário 30, Único 40, Divino 50.
- **Os bichos aguentam um pouco mais:** `monsterHitBase` 3 → 3,5 e `monsterHitPerLevel` 1,3 → 1,6. Com arma rara do
  mesmo nível, o golpe leve casa com o golpe de referência da vida deles; o 3º golpe e os poderes encurtam a luta.
- **A conta nova:** `Tools/Balance/WeaponLevels.py`.
- **No Arsenal:**
  - as placas "Herói -10/+10" e "Armas -10/+10" na frente mudam o nível do herói e o das armas e armaduras;
  - o E dá o item no nível escolhido e trava como no jogo.
