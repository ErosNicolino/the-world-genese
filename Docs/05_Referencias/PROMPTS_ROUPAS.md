# Prompts: roupas de customização (ChatGPT), 08/11

São roupas comuns de escolher no menu (não armaduras). Cada imagem traz 3 roupas, e eu monto cada uma em blocos no gerador
do jogo (`Tools/Blender/MakeBlockOutfit.py`), como foi feito com a ficha de trajes.

## Como usar

1. Abra uma conversa nova no ChatGPT.
2. Anexe as duas referências:
   - `modelando ainda/Ficha de Trajes RPG em Pixel Art.png` (o estilo da ficha);
   - `Docs/Midia/Referencia_Heroi/heroi_padrao_frente.png` (o corpo).
3. Cole o **prompt base** e, junto, a **folha R01**.
4. Para as próximas, mande só o bloco da folha seguinte, na mesma conversa.
5. Salve como `R01.png`, `R02.png`... na pasta `modelando ainda/Futuras modelagens/Roupas/` e me avise.

## Prompt base (cole sempre primeiro)

```
You are a costume designer for "The World Gêneses", a cozy fantasy voxel RPG. Use the attached images as exact references: the first shows the costume sheet layout and pixel-art style, the second shows the player's body.

The body is a blocky Minecraft-like humanoid: big cubic head, rectangular torso, separate box arms and legs. Clothes are thin shells around those boxes, painted in crisp pixel art with about 16 pixels per block face, with soft cel shading painted into the pixels and no smooth gradients.

Draw a costume sheet with exactly 3 outfits side by side on a plain light-gray background. For each outfit show the character from the FRONT and from the BACK, full body, straight-on, no perspective, arms slightly away from the body, same pose for all.

Rules:
- Everyday and adventuring clothes, not armor: no heavy plates.
- Each outfit has a clear silhouette and 4 to 6 colors.
- Sleeves and trousers are continuous, in one color across the elbow and knee, with no details placed exactly on those joints.
- Keep accessories close to the body: belts, pouches, scarves, short capes, hoods. Nothing long or floating.
- Face and hair stay plain and neutral, because hair and face are chosen separately in the game.
- No text, labels, logos, weapons or background scenery.
```

## Prompt único do herói: 5 roupas, masculino e feminino

Anexe **só** `Docs/Midia/Referencia_Heroi/heroi_padrao_frente.png` (a ficha de trajes puxava para pixel fino, diferente do herói) e cole este bloco, sem o prompt base. Segunda versão, 08/11: a primeira e as folhas R ficaram modernas demais.

```
You are a costume designer for "The World Gêneses", a medieval fantasy voxel RPG. The attached image is the game's hero: copy its exact style, body proportions, chunky large pixels, muted natural colors and worn leather and linen materials. The new clothes must look like they belong to this same character.

The body is a blocky Minecraft-like humanoid: big cubic head, rectangular torso, separate box arms and legs. Clothes are thin shells around those boxes, painted with large visible square pixels, about 16 pixels per block face, hard-edged shading and no smooth gradients or fine detail.

Setting: medieval European fantasy. Materials are only rough linen, wool, leather, fur, rope and dull iron or bronze. Nothing from after the Middle Ages: no knitwear patterns, stripes, plaid, suspenders, zippers, jeans, collared shirts, modern hats or modern shoes.

Draw one costume sheet on a plain light-gray background, arranged as a grid of 5 columns and 2 rows. Each column is one outfit. Top row: the masculine version. Bottom row: the feminine version of the same outfit, with the same colors and theme but its own cut. In every cell show the character from the FRONT and from the BACK, side by side, full body, straight-on, no perspective, arms slightly away from the body, same pose everywhere.

The 5 outfits, for a young adventurer hero:
1. Pilgrim: undyed linen tunic, faded teal wool hood and short shoulder cape, rope belt with a leather pouch, brown hose, wrapped leather boots. Feminine: longer tunic split at the sides over the hose.
2. Squire: quilted padded gambeson in dull crimson, wide leather belt with an iron buckle, dark wool hose, tall leather boots. Feminine: gambeson with a longer flared skirt section.
3. Hedge mage: deep violet wool robe with a plain darker border, leather belt with a small bound book, simple cloth shoes. Feminine: laced waist and wider hanging sleeves.
4. Forest hunter: moss-green hooded tunic, laced leather vest, fur shoulder wrap, brown hose, fur-topped boots. Feminine: shorter vest and a tunic with a split hem.
5. Knight's herald: long sleeveless surcoat in muted blue with a plain bronze-yellow cross band, over a linen shirt, leather belt, gray hose, brown boots. Feminine: fitted surcoat reaching the ankles, split at the front.

Rules:
- Adventuring clothes, not armor: no metal plates.
- Each outfit has a clear silhouette and 4 to 5 muted colors, slightly worn and dirty.
- Sleeves and hose are continuous, in one color across the elbow and knee, with no details placed exactly on those joints.
- Keep accessories close to the body. Nothing long or floating.
- Faces and hair stay plain and neutral, because hair and face are chosen separately in the game.
- No text, labels, logos, weapons or background scenery.
```

Salve como `H01.png` na mesma pasta das folhas R.

Avaliação das folhas R (08/11), para NPC. Servem: padeiro, taverneiro, nômade, patrulheiro, herbalista, jovem nobre, mercador, erudito, aprendiz estelar, dançarina da chama e druida da lua. Modernas demais: macacão, suéter, marinheiro, xadrez, festival de inverno, chapéu da colheita e pijama.

## Prompt único: as 20 peças numa imagem, só de frente

É o caminho mais curto (pedido do usuário, 08/11): as costas ficam por minha conta na hora de montar. Unissex, porque o corpo feminino usa a mesma malha. Anexe só `heroi_padrao_frente.png` e salve como `P_todas.png`.

```
You are a costume designer for "The World Gêneses", a medieval fantasy voxel RPG. The attached image is the game's hero: copy its exact style, body proportions, chunky large pixels, muted natural colors and worn leather and linen materials.

Draw one sheet on a plain light-gray background: a neat grid of 5 columns and 4 rows, 20 cells in total. Each cell shows one clothing piece ALONE, worn by a plain, bald, featureless light-gray blocky mannequin with the hero's proportions, seen from the FRONT only, full body, straight-on, no perspective, arms slightly away from the body, same pose everywhere. The mannequin wears nothing except that one piece. All pieces are unisex.

Painted with large visible square pixels, about 16 pixels per block face, hard-edged shading, no smooth gradients or fine detail. Medieval European fantasy only: rough linen, wool, leather, fur, rope, dull iron or bronze. No knitwear patterns, stripes, plaid, suspenders, zippers, jeans, collared shirts or modern shoes.

Each piece stays strictly inside its body zone. 3 to 4 muted colors per piece, slightly worn. Sleeves and legs are one continuous color across the elbow and knee. Nothing long or floating. No text, labels, logos, weapons or scenery.

Row 1, SHIRTS (torso and arms, neck to waist, down to the wrists): undyed linen shirt with a laced neck; dull crimson wool shirt with short sleeves; moss-green rough shirt with rolled sleeves; faded blue linen shirt with leather cuffs; dark brown quilted padded shirt.

Row 2, OUTER TOPS (torso and upper arms only, neck to waist, forearms bare): worn brown leather vest with lacing; dull crimson quilted sleeveless gambeson with an iron-buckled belt; faded teal wool hood down on the shoulders with a short cape; gray fur shoulder wrap tied with leather cord; muted blue sleeveless tabard with a plain bronze-yellow band.

Row 3, TROUSERS (hips and legs only, waist to ankles, no footwear): brown wool hose with a rope belt; dark gray trousers with a leather belt and small pouch; tan linen trousers with cloth wraps on the shins; dark leather riding trousers with side lacing; moss-green hose with a split knee-length skirt panel at the hips.

Row 4, FOOTWEAR (feet and lower shins only, bare legs above): wrapped leather boots tied with cord; tall brown leather boots with a folded top; fur-topped winter boots; simple cloth shoes with leather soles; dark leather boots with dull iron toe caps.
```

## Mais opções femininas: 40 peças em 2 imagens, só de frente

Anexe só `heroi_padrao_frente.png`. Salve como `P_fem1.png` e, depois do segundo bloco (na mesma conversa), `P_fem2.png`.

```
You are a costume designer for "The World Gêneses", a medieval fantasy voxel RPG. The attached image is the game's hero: copy its exact style, body proportions, chunky large pixels, muted natural colors and worn leather and linen materials.

Draw one sheet on a plain light-gray background: a neat grid of 5 columns and 4 rows, 20 cells in total. Each cell shows one feminine clothing piece ALONE, worn by a plain, bald, featureless light-gray blocky mannequin with the hero's proportions, seen from the FRONT only, full body, straight-on, no perspective, arms slightly away from the body, same pose everywhere. The mannequin wears nothing except that one piece.

Painted with large visible square pixels, about 16 pixels per block face, hard-edged shading, no smooth gradients or fine detail. Medieval European fantasy only: rough linen, wool, leather, fur, rope, dull iron or bronze, simple embroidery. No knitwear patterns, stripes, plaid, zippers, jeans, high heels or modern shoes. Modest clothing for a young adventurer.

Each piece stays strictly inside its body zone. 3 to 4 muted colors per piece, slightly worn. Sleeves and legs are one continuous color across the elbow and knee. Skirts are straight and close to the legs. Nothing floating. No text, labels, logos, weapons or scenery.

Row 1, BLOUSES (torso and arms, neck to waist, down to the wrists): cream linen blouse with a square laced neckline; soft rose wool blouse with wide cuffs; pale blue blouse with embroidered collar band; wine-red blouse with puffed upper sleeves; sage-green blouse with leather-tied sleeves.

Row 2, BODICES AND TOPS (torso and upper arms only, neck to waist, forearms bare): brown leather corset-laced bodice; deep green wool bodice with bronze clasps; plum short shoulder cape with a dark hem; white fur-collared short mantle; teal sleeveless laced vest with simple embroidery.

Row 3, SKIRTS AND LEGWEAR (hips and legs only, waist to ankles, no footwear): long earth-brown wool skirt with a rope belt; knee-length moss-green split skirt over dark hose; wine-red ankle skirt with an embroidered hem; slate-gray hose with a short leather skirt panel; cream linen skirt with a wide leather belt and pouch.

Row 4, FOOTWEAR (feet and lower shins only, bare legs above): soft brown laced ankle boots; tall dark leather boots with a cuff; cloth slippers with embroidered toes; fur-lined short boots; green leather boots with bronze buckles.
```

```
Now make a second sheet with the same style, layout and rules, and 20 new feminine pieces:

Row 1, BLOUSES: mustard linen blouse with a tied neck; dusty lilac blouse with long fitted sleeves; dark teal blouse with pale cuffs; ivory blouse with vine embroidery at the shoulders; rust-orange blouse with short sleeves.

Row 2, BODICES AND TOPS: dark violet laced bodice with silver thread; tan suede vest with fringe-free leather ties; forest-green hooded short cape; quilted cream sleeveless gambeson with a waist belt; crimson bodice with a bronze center clasp.

Row 3, SKIRTS AND LEGWEAR: long midnight-blue skirt with small silver embroidered stars at the hem; ochre layered skirt with a sash; black leather riding hose with side lacing; pale gray long skirt with a teal sash; brown short skirt over moss-green hose.

Row 4, FOOTWEAR: cream wrapped cloth boots; violet soft boots with folded tops; tall tan boots with leather straps; dark sandals with cloth wraps up the shin; gray fur-topped boots with iron buckles.
```

## Chapéus e acessórios: masculino e feminino, 2 imagens, só de frente

Anexe só `heroi_padrao_frente.png`. Salve como `P_acess_masc.png` e `P_acess_fem.png`.

```
You are a costume designer for "The World Gêneses", a medieval fantasy voxel RPG. The attached image is the game's hero: copy its exact style, body proportions, chunky large pixels, muted natural colors and worn leather and linen materials.

Draw one sheet on a plain light-gray background: a neat grid of 5 columns and 4 rows, 20 cells in total. Each cell shows one masculine hat or accessory ALONE, worn by a plain, bald, featureless light-gray blocky mannequin with the hero's proportions, seen from the FRONT only, upper body, straight-on, no perspective, same pose everywhere. The mannequin wears nothing except that one item.

Painted with large visible square pixels, about 16 pixels per block face, hard-edged shading, no smooth gradients or fine detail. Medieval European fantasy only: linen, wool, felt, leather, fur, feathers, rope, wood, bone, dull iron or bronze. No modern hats, glasses, watches, logos or modern jewelry. 3 to 4 muted colors per item, slightly worn.

Hats fit snugly around the big cubic head and leave the face visible, without covering the eyes. Items stay close to the body, with nothing floating. No hair, text, labels, weapons or scenery.

Row 1, HATS: brown felt hood with a pointed tail; green hunter's cap with a feather; wide-brimmed straw traveler's hat; gray wool coif; dark leather cap with fur trim.

Row 2, MORE HATS: blue pointed wizard hat with a bent tip; crimson soft beret with a bronze pin; bronze circlet with a small dark stone; horned leather helmet-cap; black traveler's hat with a rope band.

Row 3, NECK AND FACE: red cloth neck scarf; leather cord necklace with a wolf tooth; dark cloth half-mask over the mouth and nose; bronze chain with a round pendant; leather eyepatch.

Row 4, BELT AND ARMS: leather belt with two pouches; shoulder strap with a rolled map case; pair of leather wristbands; small side bag on a rope; fur-trimmed leather bracers.
```

```
Now make a second sheet with the same style, layout and rules, showing 20 feminine hats and accessories:

Row 1, HATS: cream linen head scarf tied at the nape; green huntress cap with a long feather; wide-brimmed straw hat with a ribbon; flower crown of small wild blossoms; wine-red velvet cap with a bronze pin.

Row 2, MORE HATS: violet pointed witch hat with silver stars; silver circlet with a pale blue stone; pale hooded veil that leaves the face visible; leaf-and-vine wreath; fur-trimmed white winter hood.

Row 3, NECK AND FACE: blue cloth neck ribbon; bronze necklace with a green stone; shell-and-bead cord necklace; pale cloth half-veil below the eyes; pair of small bronze earrings.

Row 4, BELT AND ARMS: braided leather belt with an herb pouch; narrow sash with a small book; pair of embroidered wrist cuffs; small round side bag with a leather strap; bronze arm bands with a vine pattern.
```

## Peças separadas do herói (camisa, parte de cima, calça, calçado)

Para o jogador montar a roupa peça por peça. Uma imagem por categoria: anexe só `heroi_padrao_frente.png`, cole o bloco base
e, junto, a primeira categoria. Depois mande só a categoria seguinte, na mesma conversa. Salve como `P_camisas.png`,
`P_cima.png`, `P_calcas.png` e `P_calcados.png`.

Falta no jogo: hoje cada roupa é uma malha inteira (`FVortexBlockOutfit`, `Data/Looks/roupas.json`). As peças precisam de
uma casa por parte no visual do herói e no salvamento.

```
You are a costume designer for "The World Gêneses", a medieval fantasy voxel RPG. The attached image is the game's hero: copy its exact style, body proportions, chunky large pixels, muted natural colors and worn leather and linen materials.

The player mixes clothing pieces freely, so every piece must be drawn ALONE. Draw each piece on a plain, bald, featureless light-gray blocky mannequin with the same proportions as the hero: big cubic head, rectangular torso, separate box arms and legs. The mannequin wears nothing except that one piece.

Clothes are thin shells around those boxes, painted with large visible square pixels, about 16 pixels per block face, hard-edged shading and no smooth gradients or fine detail.

Setting: medieval European fantasy. Materials are only rough linen, wool, leather, fur, rope and dull iron or bronze. Nothing from after the Middle Ages: no knitwear patterns, stripes, plaid, suspenders, zippers, jeans, collared shirts or modern shoes.

Layout: plain light-gray background, a grid of 5 columns and 2 rows. Each column is one piece. Top row: masculine version. Bottom row: feminine version, same colors and theme with its own cut. In every cell show the FRONT and the BACK side by side, full body, straight-on, no perspective, arms slightly away from the body, same pose everywhere.

Rules:
- Each piece stays strictly inside its body zone, given below, so that any piece fits with any other.
- 3 to 4 muted colors per piece, slightly worn.
- Sleeves and legs are continuous, in one color across the elbow and knee, with no details placed exactly on those joints.
- Nothing long or floating. No text, labels, logos, weapons or background scenery.
```

```
Category: SHIRTS. Body zone: torso and arms only, from the neck down to the waist and down to the wrists. Nothing below the waist.
1. Undyed linen shirt with a laced neck and long sleeves.
2. Dull crimson wool shirt with short sleeves over bare forearms.
3. Moss-green rough tunic-shirt with rolled sleeves.
4. Faded blue linen shirt with leather cuffs.
5. Dark brown quilted padded shirt.
```

```
Category: OUTER TOPS worn over a shirt. Body zone: torso and upper arms only, from the neck down to the waist. Forearms stay bare.
1. Worn brown leather vest with lacing.
2. Dull crimson quilted sleeveless gambeson with an iron-buckled belt at the waist.
3. Faded teal wool hood down on the shoulders with a short shoulder cape.
4. Gray fur shoulder wrap tied with leather cord.
5. Muted blue sleeveless tabard ending at the waist, with a plain bronze-yellow band.
```

```
Category: TROUSERS. Body zone: hips and legs only, from the waist down to the ankles. Nothing above the waist and no footwear.
1. Brown wool hose with a rope belt.
2. Dark gray trousers with a leather belt and a small pouch.
3. Tan linen trousers with cloth wraps on the shins.
4. Dark leather riding trousers with side lacing.
5. Moss-green hose with a split knee-length skirt panel at the hips.
```

```
Category: FOOTWEAR. Body zone: feet and lower shins only, from the sole up to mid-shin. Bare legs above.
1. Wrapped leather boots tied with cord.
2. Tall brown leather boots with a folded top.
3. Fur-topped winter boots.
4. Simple cloth shoes with leather soles.
5. Dark leather boots with dull iron toe caps.
```

## Elfos: 3 trajes completos e peças separadas (09/11)

Anexe só `Docs/Midia/Referencia_Heroi/heroi_padrao_frente.png`. Salve como `E_sets.png`, `E_pecas_masc.png` e
`E_pecas_fem.png` (os dois últimos na mesma conversa). As orelhas são escolhidas à parte no jogo, por isso as folhas usam
manequim careca ou rosto neutro.

```
You are a costume designer for "The World Gêneses", a medieval fantasy voxel RPG. The attached image is the game's hero: copy its exact body proportions and chunky large pixels. These are clothes for elves, using the same blocky body as the hero.

The body is a blocky humanoid: big cubic head, rectangular torso, separate box arms and legs. Clothes are thin shells around those boxes, painted with large visible square pixels, about 16 pixels per block face, hard-edged shading and no smooth gradients or fine detail.

Setting: medieval high fantasy, elegant and old. Materials are fine linen, silk, soft suede, leaf-shaped leather, silver and pale bronze. Motifs are simple leaves, vines, crescent moons and flowing lines, kept large enough to read in big pixels. Nothing modern: no zippers, jeans, collared shirts, stripes, plaid or modern shoes.

Draw one costume sheet on a plain light-gray background, arranged as a grid of 3 columns and 2 rows. Each column is one outfit. Top row: the masculine version. Bottom row: the feminine version of the same outfit, with the same colors and theme but its own cut. In every cell show the character from the FRONT and from the BACK, side by side, full body, straight-on, no perspective, arms slightly away from the body, same pose everywhere.

The 3 outfits:
1. Forest warden: layered moss-green and pale-gold tunic with a leaf-shaped hem, soft suede vest with vine lacing, slim brown hose, tall soft boots wrapped with cord. Feminine: fitted tunic split at the sides over the hose.
2. Moon court mage: long ivory and pale-blue robe with a silver crescent at the chest, narrow silver belt, long fitted sleeves, soft silver-gray shoes. Feminine: laced waist and a straight skirt close to the legs.
3. Shadow stalker, for dark elves: deep violet and charcoal tunic with sharp angular edges, black leather harness with silver clasps, dark fitted hose, tall black boots with folded tops. Feminine: fitted bodice and a split hem over the hose.

Rules:
- Each outfit has a clear silhouette and 4 to 5 colors.
- Sleeves and hose are one continuous color across the elbow and knee, with no details placed exactly on those joints.
- Keep everything close to the body. Nothing long or floating.
- Faces and hair stay plain and neutral, and the ears are not drawn, because they are chosen separately in the game.
- No text, labels, logos, weapons or background scenery.
```

```
Now make a new sheet with the same style and materials. It is a neat grid of 5 columns and 5 rows, 25 cells in total. Each cell shows one masculine elven piece ALONE, worn by a plain, bald, featureless light-gray blocky mannequin with the hero's proportions, seen from the FRONT only, full body, straight-on, no perspective, arms slightly away from the body, same pose everywhere. The mannequin wears nothing except that one piece.

Each piece stays strictly inside its body zone. 3 to 4 colors per piece. Sleeves and legs are one continuous color across the elbow and knee. Nothing floating. No text, labels, weapons or scenery.

Row 1, SHIRTS (torso and arms, neck to waist, down to the wrists): ivory silk shirt with a vine-embroidered neck; moss-green shirt with pale-gold cuffs; pale-blue shirt with a silver crescent; dark violet fitted shirt; charcoal shirt with sharp silver cuffs.

Row 2, OUTER TOPS (torso and upper arms only, forearms bare): leaf-scaled suede vest; pale-gold sleeveless tabard with a vine band; short forest-green shoulder cape; silver-clasped ivory mantle; black leather harness vest.

Row 3, TROUSERS (hips and legs only, no footwear): slim brown hose with a vine belt; moss-green trousers with a leaf-shaped hip panel; pale-gray silk trousers; dark violet hose; black leather trousers with silver side lacing.

Row 4, FOOTWEAR (feet and lower shins only): soft suede boots wrapped with cord; tall green boots with a leaf-shaped top; silver-gray cloth shoes; black boots with folded tops; dark violet boots with silver buckles.

Row 5, HATS AND ACCESSORIES: silver circlet with a pale green stone, on the forehead; leaf wreath; forest-green hood with open sides; silver crescent necklace; black half-mask over the mouth and nose. Headwear leaves the sides of the head clear, where the ears go.
```

```
Now make a second sheet with the same style, layout and rules, showing 25 feminine elven pieces:

Row 1, BLOUSES: ivory blouse with long fitted sleeves and a vine neckline; sage-green blouse with leaf cuffs; pale-blue blouse with a silver moon; deep violet blouse with pointed cuffs; charcoal blouse with silver thread at the shoulders.

Row 2, BODICES AND TOPS: leaf-scaled suede bodice; pale-gold laced bodice with vine embroidery; short moss-green shoulder cape; silver-clasped white mantle; black leather bodice with silver clasps.

Row 3, SKIRTS AND LEGWEAR: long straight moss-green skirt with a leaf hem; slim brown hose with a short suede skirt panel; long ivory skirt with a silver sash; dark violet hose with a split skirt panel; long black skirt with a sharp angular hem.

Row 4, FOOTWEAR: soft green laced boots; tall suede boots with a leaf-shaped top; silver cloth slippers; black boots with a cuff; violet soft boots with silver clasps.

Row 5, HATS AND ACCESSORIES: silver circlet with a pale blue stone, on the forehead; flower and leaf wreath; pale hooded veil with open sides that leaves the face visible; green stone necklace on a silver chain; dark half-veil below the eyes. Headwear leaves the sides of the head clear, where the ears go.
```

## As folhas

Cada bloco abaixo vai depois do prompt base. Dá para pedir a mesma folha no corpo feminino acrescentando no fim:
`Use a feminine version of the body.`

```
Sheet R01 — Village life:
1. Baker: cream shirt with rolled sleeves, flour-dusted white apron, brown trousers, soft cap.
2. Farmer: straw-yellow tunic, patched green overalls, rope belt, muddy boots.
3. Tavern keeper: burgundy vest over a linen shirt, dark trousers, short striped apron.
```

```
Sheet R02 — Travelers:
1. Desert nomad: sand-colored layered robes, wide red sash, cloth wrapped around the neck.
2. Mountain guide: thick blue wool sweater with a white zigzag band, gray trousers, fur-trimmed boots.
3. Sailor: navy-and-white striped shirt, dark blue trousers, red neckerchief, wide belt.
```

```
Sheet R03 — Forest:
1. Ranger: forest-green hooded tunic, brown leather belt with pouches, dark trousers.
2. Herbalist: moss-colored dress-tunic with leaf embroidery, herb pouches, soft boots.
3. Woodcutter: red-and-black checkered flannel shirt, sturdy brown trousers, leather suspenders.
```

```
Sheet R04 — City and nobility:
1. Young noble: royal-blue doublet with gold buttons, white collar, slim black trousers.
2. Merchant: rich purple coat with gold trim, coin pouch at the belt, tan trousers.
3. Scholar: long gray-blue academic robe, scroll pouch, ink-stained cuffs.
```

```
Sheet R05 — Fantasy:
1. Star apprentice: midnight-blue robe with small painted stars, silver cord belt.
2. Flame dancer: orange-and-crimson wrapped outfit with a flame hem, gold bands.
3. Moon druid: pale silver tunic with crescent patterns, teal sash.
```

```
Sheet R06 — Festive and cozy:
1. Winter festival: red coat with white fur trim, green scarf, black boots.
2. Harvest festival: orange tunic with autumn-leaf patterns, brown vest, felt hat.
3. Pajamas: soft sky-blue sleepwear with cloud patterns, fuzzy slippers.
```
