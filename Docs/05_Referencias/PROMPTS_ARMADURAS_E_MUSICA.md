# Prompts: armaduras (Gemini) e músicas (Suno), 02/11

Os prompts estão em inglês: as duas ferramentas entendem melhor assim. A explicação em volta está em português.

## Armaduras (Gemini)

**Como usar:**
- anexe ao Gemini 2 ou 3 fotos do herói do jogo (de frente, de lado e de costas; por exemplo `Saved/Screenshots/perto_*`
  ou o `personagem principal.glb` aberto no Blender);
- cole o **prompt base** e, logo depois, **uma raridade por vez** (pedir as 7 juntas sai pior).

As raridades são as do jogo (`Data/Rpg/rpg.json`). Cada raridade tem 3 conjuntos: um pesado (guerreiro), um médio
(arqueiro ou ladino) e um leve (mago).

### Prompt base (cole sempre primeiro)

```
You are a concept artist for "The World Gêneses", a cozy-but-epic voxel fantasy action RPG (Unreal Engine 5, multiplayer).
Use the attached images as the exact reference for the player character: a blocky, Minecraft/Hytale-like humanoid made of
boxes (big cubic head, rectangular torso, separate upper/lower arm and leg blocks), with hand-painted PIXEL ART textures
(about 16 px per block face, crisp, no gradients smoothing, soft cel shading painted into the pixels).

Design ARMOR SETS that this exact character can wear. Rules:
- Keep the character's proportions and block shapes; armor is extra boxes and plates that hug the existing blocks
  (shells 1-3 px thick in voxel terms), never smooth organic sculpting.
- Every set has 4 pieces: HELMET or HOOD, CHEST (with shoulder pieces), LEGS (with belt), BOOTS. Optional: gloves/bracers.
- Readable silhouette from a third-person camera: one clear signature shape per set (horns, crest, hood point, pauldron spikes...).
- Pixel-art texture details: rivets, stitches, rune engravings, fur trims, gems, all painted at block resolution.
- Higher rarity = more complex shapes, richer materials, small emissive details (glowing runes/gems) — but never noisy.
- World flavor: ancient Seals that hold the Vortex, a fallen star (violet crystal), a volcanic Caldera, mossy forests,
  a sanctuary city with stone streets.

For EACH set produce one clean concept sheet on a neutral light-grey background:
front view, back view, side view of the character wearing the full set, plus the 4 pieces shown separately,
a 6-8 color palette swatch, the set name and a one-line description. Orthographic-style, no perspective distortion,
no cast shadows on the background, consistent lighting from the upper left.
```

### Uma raridade por vez (cole depois do base)

**Comum (cinza e marrom: ferro, couro, lã)**
```
Rarity: COMMON (palette: worn iron grey, brown leather, undyed wool). Simple, practical starter gear, slightly used.
Make 3 sets:
1. "Iron Watch" (heavy/warrior): city-guard iron cap, riveted iron breastplate over a padded gambeson, simple greaves.
2. "Trailwalker Leather" (medium/ranger): hooded leather jerkin, quiver strap across the chest, laced leather boots.
3. "Apprentice Wool" (light/mage): wool hood with a rope tie, long tunic with a cloth sash and a small ink pouch.
```

**Incomum (verde: bronze, couro tingido, folhas)**
```
Rarity: UNCOMMON (palette: forest green accents, bronze, dyed leather). Better crafted, small decorative motifs.
Make 3 sets:
1. "Bronze Sentinel" (heavy): bronze half-helm with a short crest, scale mail, bronze knee guards.
2. "Mossrunner" (medium): green hood with leaf-shaped cape tips, layered leather with moss trims.
3. "Herbalist's Robe" (light): green robe with many pouches, dried herbs tied to the belt, wide-brim hat.
```

**Raro (azul: aço polido, prata, tecido azul)**
```
Rarity: RARE (palette: royal blue cloth, polished steel, silver trims). Clearly heroic, clean engraved lines.
Make 3 sets:
1. "Sanctuary Knight" (heavy): steel great-helm with a blue plume, plate armor with the sanctuary emblem, blue tabard.
2. "Tidecaller" (medium): blue scale coat like fish scales, silver bracers, a short cape with wave patterns.
3. "Starweave Mantle" (light): deep-blue mantle with tiny silver star embroidery, high collar, silver circlet.
```

**Épico (roxo: obsidiana, ametista, runas acesas)**
```
Rarity: EPIC (palette: violet, obsidian black, amethyst, faint glowing purple runes). Dramatic shapes, small emissive runes.
Make 3 sets:
1. "Vortex Breaker" (heavy): obsidian plate with violet crystal shards growing from the pauldrons, horned helm.
2. "Shadow of the Seal" (medium): dark leather with purple rune stitching, a mask-hood, crystal-tipped bracers.
3. "Fallen Star Regalia" (light): robe made from night-sky cloth, a floating crystal fragment above the shoulder.
```

**Lendário (laranja e dourado: ouro, magma, couro de dragão)**
```
Rarity: LEGENDARY (palette: gold, deep orange, magma glow, dark basalt). Mythic, unique silhouettes, glowing magma cracks.
Make 3 sets:
1. "Heart of Azhûr" (heavy): basalt plate armor with glowing magma cracks and a core gem in the chest (inspired by the
   volcanic golem boss), crown-like helm of black stone spikes.
2. "Sunfang Hunter" (medium): golden-trimmed hunter armor with a lion/sun mane hood and glowing gold arrows on the back.
3. "Phoenix Ascendant" (light): flowing feathered robe in orange and gold, wing-shaped shoulder pieces, ember particles
   painted as pixel sparks.
```

**Único (vermelho: peças de um dono só, com história)**
```
Rarity: UNIQUE (palette: crimson red with black and bone). Each set belongs to a named legend of the world, very distinctive.
Make 3 sets:
1. "Warden of the Caldera" (heavy): the armor of the last guardian of the volcanic Caldera, chained, scorched, heroic.
2. "The Crimson Duelist" (medium): an elegant red coat-armor with a single pauldron and a long scarf.
3. "Bonehollow Seer" (light): a bone-and-red-cloth shaman set with a skull mask and floating red talismans.
```

**Divino (branco e dourado: luz, os Selos)**
```
Rarity: DIVINE (palette: white marble, pale gold, soft cyan light of the Seals). Holy, rare, almost floating parts.
Make 3 sets:
1. "Seal Paladin" (heavy): white-gold plate with floating halo-like rings behind the helm and cyan seal runes.
2. "Dawn Windwalker" (medium): light white-and-gold armor with wind ribbons that hover around the arms.
3. "Oracle of the First Seal" (light): marble-white robes with a floating cyan seal disc behind the back.
```

**Dica:** se o Gemini fugir do estilo de blocos, responda: *"Keep it strictly blocky voxel like the reference: every
piece must be made of boxes with pixel-art textures, no smooth curves."*

## Músicas (Suno)

**Como usar:**
- no Suno, ligue **Custom** e **Instrumental**;
- cole o estilo em "Style of Music" e as marcações em "Lyrics" (servem só de estrutura; com Instrumental, ninguém
  canta);
- gere umas 4 versões e escolha a que dá laço melhor (o fim emendando no começo).

### Batalha contra o chefe (Azhûr, o golem vulcânico)

**Style of Music:**
```
epic orchestral boss battle, volcanic and heavy, 150 bpm, D minor, massive taiko drums and low brass ostinato,
distorted low cello and contrabass, stone-like metallic percussion hits, dark choir chants without words, tense string
spiccato, menacing horn melody, cinematic trailer hybrid, deep sub booms on downbeats, rising tension, final phase
intensifies with faster drums and higher choir, loopable, no vocals lyrics, fantasy RPG
```

**Lyrics (estrutura):**
```
[Intro: low rumble, single taiko hits, lava atmosphere]
[Main Theme: heavy brass ostinato, full drums]
[Build: strings spiccato, choir enters]
[Climax: full orchestra and choir, horn melody]
[Phase 2: tempo feels faster, double-time drums, higher choir, distorted cellos]
[Climax 2: everything at maximum intensity]
[Loop Point: return to main theme groove]
```

### Batalhas normais (monstros do mundo)

**Style of Music:**
```
adventurous fantasy battle music, 140 bpm, E minor, energetic orchestral folk, driving frame drums and snare,
bold french horns, fast pizzicato and spiccato strings, celtic fiddle and tin whistle melody, light choir pads,
heroic but not too dark, short and punchy, loopable 2 minute track for a voxel action RPG, no vocals lyrics
```

**Lyrics (estrutura):**
```
[Intro: drums and pizzicato]
[Theme A: fiddle melody over horns]
[Theme B: tin whistle answer, choir pad]
[Bridge: drums only, building]
[Theme A reprise: full ensemble]
[Loop Point]
```

**Dicas:**
- **Música de exploração:** troque "battle" por "exploration" e baixe para 90 bpm; sai uma de exploração no mesmo
  estilo.
- **Nos créditos:** guarde o link de cada música gerada. Veja também se o seu plano do Suno permite uso comercial
  (é o caso dos planos pagos).
