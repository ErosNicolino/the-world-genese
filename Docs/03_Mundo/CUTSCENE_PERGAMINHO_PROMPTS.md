# Abertura "Acorde": o pergaminho pintado (prompts, 04/11)

A abertura passa a ser **um pergaminho antigo que se desenrola para os lados**, como um rolo grego pintado com contos.
A câmera anda pelo pergaminho da esquerda para a direita, no tempo da música, e cada cena chega quando o verso dela é
cantado. As cenas ficam separadas por colunas pintadas.

Os prompts estão em inglês, porque os geradores de imagem entendem melhor. Cada um é **completo**: o estilo, o mundo e
as cenas estão dentro do texto, sem precisar de nenhuma imagem de referência. É só copiar e colar.

## Como gerar

- **O pergaminho:** são **8 trechos**, com **3 cenas cada** (24 cenas, os 24 versos da música), mais a imagem de
  abertura (o rolo fechado).
- **Formato de cada trecho:** bem largo, **4:1** (por exemplo, 4096 × 1024).
  - No Midjourney: acrescente `--ar 4:1` no fim.
  - Se o gerador só faz 16:9 ou 3:2: gere as 3 cenas de cada trecho uma por uma (há um prompt de cena solta no fim) e
    me mande. Eu monto o pergaminho.
- **Mesma cara em todos:** use a mesma "seed" (se o gerador tiver) e gere todos no mesmo dia, com o mesmo modelo.
- **Nomes dos arquivos:** `P0.png` (o rolo fechado) e `P1.png` a `P8.png` (os trechos).
- **O que conferir em cada imagem:**
  - nenhuma letra ou texto (os geradores às vezes inventam);
  - as 3 cenas separadas pelas colunas;
  - o que importa no meio da altura (as bordas de cima e de baixo levam a faixa grega).
- **No jogo:** o pergaminho que se desenrola precisa de uma mudança no código da abertura. Eu faço quando as imagens
  estiverem prontas. Até lá, a abertura atual continua funcionando.

**Se quiser manter os personagens de blocos do jogo**, acrescente esta linha no fim de qualquer prompt:
`All figures, creatures, trees and buildings are stylized as chunky blocky voxel shapes, like carved wooden toys, but painted in the same antique style.`

## O texto que se repete (já está dentro de cada prompt)

**Estilo:**
> An ancient illustrated scroll unrolled horizontally, painted like an old Greek and Byzantine storybook of myths and
> fables. Egg tempera and gouache on aged cream parchment with visible fibers, soft creases, small cracks and faded
> pigments, fine hand-drawn sepia ink outlines, touches of worn gold leaf. A thin painted Greek key (meander) border in
> faded terracotta and gold runs along the top and bottom edges. Three scenes painted from left to right, separated by
> slender painted marble columns wrapped in olive vines, read as one continuous tale. Classical fresco composition with
> gentle depth, flowing robes, expressive silhouettes. Earthy palette of ochre, umber, terracotta, moss green and deep
> lapis blue; magic appears only as glowing jade green and violet. No text, no letters, no captions, no signatures,
> no modern elements.

**O mundo:**
> World of the legend "The World Gêneses": a single island-continent with a three-peaked mountain ridge in the north.
> Yggdrasil, a colossal world tree whose trunk forks into three great branches under a broad flat oak-like crown. Its
> sap glows jade green. The Vortex: a giant spiral wound in the sky, a black eye at the center, a violet spiral turning
> counterclockwise with two jade-green arcs, veiled by dark clouds. Azhûr: a colossus of black basalt rock with glowing
> orange runes and a fire core in its chest. The hero, one of the Awakened: hooded, green cloak, face hidden in shadow,
> glowing jade eyes.

## P0: o rolo fechado (a abertura, 0:00 a 0:14, enquanto aparece o título)

```
An ancient rolled scroll of aged cream parchment lying on a dark wooden table, tied with a faded red cord and closed
with a wax seal stamped with a spiral symbol, two carved wooden scroll handles with worn gold caps. Soft candlelight
from one side, deep shadows, a few dried olive leaves and an old quill nearby. Painted like a classical still life in
egg tempera and gouache, earthy ochre and umber tones, warm and mysterious. The center of the image is calm and dark
enough for a title to appear over it. No text, no letters, no captions, no signatures. Wide format 16:9.
```

## P1: Gênese, a árvore e as raízes (0:14 a 0:55)

Os versos: "Antes do nome…" / "E em seu coração…" / "Suas raízes cruzavam…"

```
An ancient illustrated scroll unrolled horizontally, painted like an old Greek and Byzantine storybook of myths and fables. Egg tempera and gouache on aged cream parchment with visible fibers, soft creases, small cracks and faded pigments, fine hand-drawn sepia ink outlines, touches of worn gold leaf. A thin painted Greek key (meander) border in faded terracotta and gold runs along the top and bottom edges. Three scenes painted from left to right, separated by slender painted marble columns wrapped in olive vines, read as one continuous tale. Classical fresco composition with gentle depth, flowing robes, expressive silhouettes. Earthy palette of ochre, umber, terracotta, moss green and deep lapis blue; magic appears only as glowing jade green and violet. No text, no letters, no captions, no signatures, no modern elements.
World of the legend "The World Gêneses": a single island-continent with a three-peaked mountain ridge in the north. Yggdrasil, a colossal world tree whose trunk forks into three great branches under a broad flat oak-like crown. Its sap glows jade green. The Vortex: a giant spiral wound in the sky, a black eye at the center, a violet spiral turning counterclockwise with two jade-green arcs, veiled by dark clouds. Azhûr: a colossus of black basalt rock with glowing orange runes and a fire core in its chest. The hero, one of the Awakened: hooded, green cloak, face hidden in shadow, glowing jade eyes.
SCENE 1 (left): dawn over an endless calm primordial ocean, a single great island-continent rising from the mist, the first golden light touching the three-peaked ridge; no tree, no people, no buildings yet.
SCENE 2 (center): the colossal world tree Yggdrasil growing at the heart of the continent under rays of primordial golden light, rivers and valleys spreading around it, birds circling its crown.
SCENE 3 (right): a cutaway view beneath the earth under the same tree, its three great roots crossing layered soil and stone, jade-green sap flowing through them like rivers of light toward distant lands.
Wide panoramic format 4:1.
```

## P2: a vida, o céu que se retorce, o Vórtice (0:55 a 1:47)

Os versos: "Onde ela fluía…" / "Mas sob a copa…" / "Chamaram de Vórtice…"

```
An ancient illustrated scroll unrolled horizontally, painted like an old Greek and Byzantine storybook of myths and fables. Egg tempera and gouache on aged cream parchment with visible fibers, soft creases, small cracks and faded pigments, fine hand-drawn sepia ink outlines, touches of worn gold leaf. A thin painted Greek key (meander) border in faded terracotta and gold runs along the top and bottom edges. Three scenes painted from left to right, separated by slender painted marble columns wrapped in olive vines, read as one continuous tale. Classical fresco composition with gentle depth, flowing robes, expressive silhouettes. Earthy palette of ochre, umber, terracotta, moss green and deep lapis blue; magic appears only as glowing jade green and violet. No text, no letters, no captions, no signatures, no modern elements.
World of the legend "The World Gêneses": a single island-continent with a three-peaked mountain ridge in the north. Yggdrasil, a colossal world tree whose trunk forks into three great branches under a broad flat oak-like crown. Its sap glows jade green. The Vortex: a giant spiral wound in the sky, a black eye at the center, a violet spiral turning counterclockwise with two jade-green arcs, veiled by dark clouds. Azhûr: a colossus of black basalt rock with glowing orange runes and a fire core in its chest. The hero, one of the Awakened: hooded, green cloak, face hidden in shadow, glowing jade eyes.
SCENE 1 (left): a lush valley in golden afternoon light along the river that flows from the world tree, deer and rabbits drinking, wildflowers and soft mist, perfect harmony.
SCENE 2 (center): the same valley and tree at night, the stars beginning to bend into a spiral above the crown, a dark shadow rising between them, leaves lifting upward; the Vortex is not fully formed yet.
SCENE 3 (right): the Vortex fully revealed above the world tree, a giant wound in the sky with its black eye, violet spiral and jade arcs, pulling leaves, birds, water and glowing sap upward like a tide, a few ancient villagers clinging to rocks below.
Wide panoramic format 4:1.
```

## P3: os antigos, os nove Selos, Valdora (1:47 a 2:37)

Os versos: "Então os antigos…" / "Nove Selos… nove destinos…" / "Mil anos se passaram…"

```
An ancient illustrated scroll unrolled horizontally, painted like an old Greek and Byzantine storybook of myths and fables. Egg tempera and gouache on aged cream parchment with visible fibers, soft creases, small cracks and faded pigments, fine hand-drawn sepia ink outlines, touches of worn gold leaf. A thin painted Greek key (meander) border in faded terracotta and gold runs along the top and bottom edges. Three scenes painted from left to right, separated by slender painted marble columns wrapped in olive vines, read as one continuous tale. Classical fresco composition with gentle depth, flowing robes, expressive silhouettes. Earthy palette of ochre, umber, terracotta, moss green and deep lapis blue; magic appears only as glowing jade green and violet. No text, no letters, no captions, no signatures, no modern elements.
World of the legend "The World Gêneses": a single island-continent with a three-peaked mountain ridge in the north. Yggdrasil, a colossal world tree whose trunk forks into three great branches under a broad flat oak-like crown. Its sap glows jade green. The Vortex: a giant spiral wound in the sky, a black eye at the center, a violet spiral turning counterclockwise with two jade-green arcs, veiled by dark clouds. Azhûr: a colossus of black basalt rock with glowing orange runes and a fire core in its chest. The hero, one of the Awakened: hooded, green cloak, face hidden in shadow, glowing jade eyes.
SCENE 1 (left): hooded ancient sages in a torch-lit circle of standing stones, carving glowing jade runes into nine tall stones, sparks of green light, solemn and secret.
SCENE 2 (center): nine colossal guardians sleeping as stone statues, each curled inside its own glowing rune circle under the stars: a basalt colossus, a titan of roots, a glass scorpion, a giant toad, a thunderbird, a crimson blacksmith giant, an ice dragon, a lich king and a wasp queen.
SCENE 3 (right): a thousand years later, the small walled village of Valdora asleep under snow at night, warm lit windows, a softly glowing jade fountain in the square, a silent starry sky.
Wide panoramic format 4:1.
```

## P4: a estrela, o impacto, Azhûr desperta (2:37 a 3:13)

Os versos: "Até que, vinte invernos atrás…" / "Rasgou a terra…" / "Azhûr! O Colosso despertou!"

```
An ancient illustrated scroll unrolled horizontally, painted like an old Greek and Byzantine storybook of myths and fables. Egg tempera and gouache on aged cream parchment with visible fibers, soft creases, small cracks and faded pigments, fine hand-drawn sepia ink outlines, touches of worn gold leaf. A thin painted Greek key (meander) border in faded terracotta and gold runs along the top and bottom edges. Three scenes painted from left to right, separated by slender painted marble columns wrapped in olive vines, read as one continuous tale. Classical fresco composition with gentle depth, flowing robes, expressive silhouettes. Earthy palette of ochre, umber, terracotta, moss green and deep lapis blue; magic appears only as glowing jade green and violet. No text, no letters, no captions, no signatures, no modern elements.
World of the legend "The World Gêneses": a single island-continent with a three-peaked mountain ridge in the north. Yggdrasil, a colossal world tree whose trunk forks into three great branches under a broad flat oak-like crown. Its sap glows jade green. The Vortex: a giant spiral wound in the sky, a black eye at the center, a violet spiral turning counterclockwise with two jade-green arcs, veiled by dark clouds. Azhûr: a colossus of black basalt rock with glowing orange runes and a fire core in its chest. The hero, one of the Awakened: hooded, green cloak, face hidden in shadow, glowing jade eyes.
SCENE 1 (left): an old shepherd with his sheep on a snowy hill at night looking up at a bright falling star; up close the star is a jagged violet crystal shard of the Vortex wrapped in green fire, tearing through the clouds.
SCENE 2 (center): the impact, the earth torn open by a blinding blast, a crater of fire and lava being born, the stones of a rune circle cracking and flying apart; the brightest point of the whole scroll is the center of this scene.
SCENE 3 (right): Azhûr, the black basalt colossus with glowing orange runes, rising from the burning crater, rocks falling from his shoulders, under a red sky.
Wide panoramic format 4:1.
```

## P5: o fogo no peito, a seiva que foge, as feras (3:13 a 3:39)

Os versos: "Azhûr! E em seu peito…" / "Desde então, a seiva foge…" / "As feras já não conhecem…"

```
An ancient illustrated scroll unrolled horizontally, painted like an old Greek and Byzantine storybook of myths and fables. Egg tempera and gouache on aged cream parchment with visible fibers, soft creases, small cracks and faded pigments, fine hand-drawn sepia ink outlines, touches of worn gold leaf. A thin painted Greek key (meander) border in faded terracotta and gold runs along the top and bottom edges. Three scenes painted from left to right, separated by slender painted marble columns wrapped in olive vines, read as one continuous tale. Classical fresco composition with gentle depth, flowing robes, expressive silhouettes. Earthy palette of ochre, umber, terracotta, moss green and deep lapis blue; magic appears only as glowing jade green and violet. No text, no letters, no captions, no signatures, no modern elements.
World of the legend "The World Gêneses": a single island-continent with a three-peaked mountain ridge in the north. Yggdrasil, a colossal world tree whose trunk forks into three great branches under a broad flat oak-like crown. Its sap glows jade green. The Vortex: a giant spiral wound in the sky, a black eye at the center, a violet spiral turning counterclockwise with two jade-green arcs, veiled by dark clouds. Azhûr: a colossus of black basalt rock with glowing orange runes and a fire core in its chest. The hero, one of the Awakened: hooded, green cloak, face hidden in shadow, glowing jade eyes.
SCENE 1 (left): a close view of Azhûr's chest, a core of fire igniting inside the cracked black basalt, runes burning orange, embers swirling.
SCENE 2 (center): a cold valley at night, thin threads of jade-green sap rising from the ground toward the Vortex in the sky, green crystals glowing among the rocks and grass.
SCENE 3 (right): corrupted beasts at the edge of a dark forest, wolves with glowing violet eyes snarling, a frightened traveler with a lantern stepping back.
Wide panoramic format 4:1.
```

## P6: as muralhas, o sonho, a alma devolvida (3:39 a 4:34)

Os versos: "Nas muralhas, a guarda se ergueu…" / "Sonham com o fogo…" / "Mas nem tudo o que atravessa…"

```
An ancient illustrated scroll unrolled horizontally, painted like an old Greek and Byzantine storybook of myths and fables. Egg tempera and gouache on aged cream parchment with visible fibers, soft creases, small cracks and faded pigments, fine hand-drawn sepia ink outlines, touches of worn gold leaf. A thin painted Greek key (meander) border in faded terracotta and gold runs along the top and bottom edges. Three scenes painted from left to right, separated by slender painted marble columns wrapped in olive vines, read as one continuous tale. Classical fresco composition with gentle depth, flowing robes, expressive silhouettes. Earthy palette of ochre, umber, terracotta, moss green and deep lapis blue; magic appears only as glowing jade green and violet. No text, no letters, no captions, no signatures, no modern elements.
World of the legend "The World Gêneses": a single island-continent with a three-peaked mountain ridge in the north. Yggdrasil, a colossal world tree whose trunk forks into three great branches under a broad flat oak-like crown. Its sap glows jade green. The Vortex: a giant spiral wound in the sky, a black eye at the center, a violet spiral turning counterclockwise with two jade-green arcs, veiled by dark clouds. Azhûr: a colossus of black basalt rock with glowing orange runes and a fire core in its chest. The hero, one of the Awakened: hooded, green cloak, face hidden in shadow, glowing jade eyes.
SCENE 1 (left): Captain Rhea, a stern woman in worn steel armor and a red cape, standing on the western city walls at sunset with spear-bearing guards and torches; above, in the clouds, faint ghostly shapes of the sleeping guardians beginning to stir.
SCENE 2 (center): a dreamlike scene, a lone hooded figure in a field of fire beneath the Vortex, which bends down toward him like a calling hand.
SCENE 3 (right): from the dark abyss of the Vortex, one small warm glowing soul resists the pull and descends slowly toward the land, like a falling star in reverse, peaceful and bright.
Wide panoramic format 4:1.
```

## P7: o Estádio, as armas, os Despertos (4:34 a 5:18)

Os versos: "Uma luz atravessa o véu…" / "Sem nome, sem memória…" / "Os antigos os chamaram Despertos…"

```
An ancient illustrated scroll unrolled horizontally, painted like an old Greek and Byzantine storybook of myths and fables. Egg tempera and gouache on aged cream parchment with visible fibers, soft creases, small cracks and faded pigments, fine hand-drawn sepia ink outlines, touches of worn gold leaf. A thin painted Greek key (meander) border in faded terracotta and gold runs along the top and bottom edges. Three scenes painted from left to right, separated by slender painted marble columns wrapped in olive vines, read as one continuous tale. Classical fresco composition with gentle depth, flowing robes, expressive silhouettes. Earthy palette of ochre, umber, terracotta, moss green and deep lapis blue; magic appears only as glowing jade green and violet. No text, no letters, no captions, no signatures, no modern elements.
World of the legend "The World Gêneses": a single island-continent with a three-peaked mountain ridge in the north. Yggdrasil, a colossal world tree whose trunk forks into three great branches under a broad flat oak-like crown. Its sap glows jade green. The Vortex: a giant spiral wound in the sky, a black eye at the center, a violet spiral turning counterclockwise with two jade-green arcs, veiled by dark clouds. Azhûr: a colossus of black basalt rock with glowing orange runes and a fire core in its chest. The hero, one of the Awakened: hooded, green cloak, face hidden in shadow, glowing jade eyes.
SCENE 1 (left): a single beam of jade light piercing the black cloud veil at night and falling straight down into an ancient overgrown coliseum, the Stadium of Awakening, where the hooded hero lies unconscious in a fading circle of light.
SCENE 2 (center): the hero kneeling in the arena, lost and confused, looking at his own hands; around him glowing weapons float above stone pedestals: a sword, twin daggers, a war hammer, a staff and a bow, with jade sap light pouring from the ground.
SCENE 3 (right): an ancient temple mural coming alive, carved hooded figures with glowing jade eyes walking toward a giant sleeping guardian, while ordinary people kneel far away.
Wide panoramic format 4:1.
```

## P8: a corrupção, o penhasco, os olhos (5:18 a 6:14, o fim)

Os versos: "sem se perder na corrupção." / "Você é um deles…" / "Acorde."

```
An ancient illustrated scroll unrolled horizontally, painted like an old Greek and Byzantine storybook of myths and fables. Egg tempera and gouache on aged cream parchment with visible fibers, soft creases, small cracks and faded pigments, fine hand-drawn sepia ink outlines, touches of worn gold leaf. A thin painted Greek key (meander) border in faded terracotta and gold runs along the top and bottom edges. Three scenes painted from left to right, separated by slender painted marble columns wrapped in olive vines, read as one continuous tale. Classical fresco composition with gentle depth, flowing robes, expressive silhouettes. Earthy palette of ochre, umber, terracotta, moss green and deep lapis blue; magic appears only as glowing jade green and violet. No text, no letters, no captions, no signatures, no modern elements.
World of the legend "The World Gêneses": a single island-continent with a three-peaked mountain ridge in the north. Yggdrasil, a colossal world tree whose trunk forks into three great branches under a broad flat oak-like crown. Its sap glows jade green. The Vortex: a giant spiral wound in the sky, a black eye at the center, a violet spiral turning counterclockwise with two jade-green arcs, veiled by dark clouds. Azhûr: a colossus of black basalt rock with glowing orange runes and a fire core in its chest. The hero, one of the Awakened: hooded, green cloak, face hidden in shadow, glowing jade eyes.
SCENE 1 (left): the hooded hero walking calmly through a swirling violet mist of corruption, unharmed, corrupted vines and shadows recoiling from him.
SCENE 2 (center, the widest scene): the hero seen from behind on a cliff at dawn, cloak blowing, overlooking the whole continent: the world tree in the distance, the Vortex above it, rivers, a walled village and the smoke of a volcanic crater, hopeful golden light.
SCENE 3 (right): an extreme close-up of the hero's eyes opening for the first time, glowing jade irises under the shadow of the hood, a single tear of light; both eyes centered and symmetrical.
Wide panoramic format 4:1.
```

## Cena solta (se o gerador não fizer 4:1)

Troque `<CENA>` pelo texto de uma cena acima (o que vem depois de "SCENE 1 (left):", por exemplo). Nomeie os
arquivos `P1a.png`, `P1b.png` e `P1c.png` (trecho 1, cenas da esquerda, do meio e da direita), e assim por diante.

```
One scene from an ancient illustrated scroll, painted like an old Greek and Byzantine storybook of myths and fables. Egg tempera and gouache on aged cream parchment with visible fibers, soft creases, small cracks and faded pigments, fine hand-drawn sepia ink outlines, touches of worn gold leaf. A thin painted Greek key (meander) border in faded terracotta and gold along the top and bottom edges only. Classical fresco composition with gentle depth, flowing robes, expressive silhouettes. Earthy palette of ochre, umber, terracotta, moss green and deep lapis blue; magic appears only as glowing jade green and violet. No text, no letters, no captions, no signatures, no modern elements.
World of the legend "The World Gêneses": a single island-continent with a three-peaked mountain ridge in the north. Yggdrasil, a colossal world tree whose trunk forks into three great branches under a broad flat oak-like crown. Its sap glows jade green. The Vortex: a giant spiral wound in the sky, a black eye at the center, a violet spiral turning counterclockwise with two jade-green arcs, veiled by dark clouds. Azhûr: a colossus of black basalt rock with glowing orange runes and a fire core in its chest. The hero, one of the Awakened: hooded, green cloak, face hidden in shadow, glowing jade eyes.
SCENE: <CENA>
```
