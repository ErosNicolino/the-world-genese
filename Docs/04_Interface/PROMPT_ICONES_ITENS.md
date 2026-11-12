# Prompt único dos ícones dos itens (08/11)

São 185 ícones em 24 folhas de 8: 35 espadas, 35 arcos, 84 peças de armadura e 31 itens (materiais, poções, amuletos).
Uma imagem só não comporta todos de forma legível, por isso o prompt pede uma folha por vez.

## Como usar

1. Abra uma conversa nova no ChatGPT e cole o prompt abaixo inteiro. Ele gera a folha I01.
2. Para cada folha seguinte, escreva só `next`.
3. Salve cada imagem com o nome da folha (`I01.png`, `I02.png`...) em `Data/UI/SkillIcons/Folhas/`.
4. Se o estilo começar a mudar, abra outra conversa, cole o prompt e peça a folha em que parou (por exemplo: `start at sheet I12`).
5. Me avise quando tiver as folhas: eu corto (`python Tools/SliceIconSheet.py I01 I02 ...`), ligo no jogo e, a partir delas, faço os modelos 3D.

## O prompt

```
I'm making the item icons for a cozy fantasy voxel RPG called "The World Gêneses" (Minecraft-like blocky world). Below are 24 sheets of 8 icons each. Generate ONE sheet per image, starting with sheet I01. After each image, stop and wait. When I say "next", generate the following sheet. Keep the exact same style in all of them.

Rules for every sheet:
- Layout: landscape 1536x1024 canvas, 4 columns x 2 rows, 8 square tiles of the same size, evenly spaced, separated by a flat dark navy (#101826) gap, in the order listed (left to right, top row first).
- No numbers, labels, letters or text anywhere, and no frame or border (the game draws its own).
- Each tile: full-bleed square with a soft radial background in the item's rarity color (the color in parentheses).
- Style: chunky voxel / blocky 3D look with subtle pixel texture, one bold centered object with a simple silhouette that still reads at 48 px, strong contrast, soft lighting. The object is shown alone: no character, no hands.
- Rarity shows in the material: common is plain, and higher rarities get richer materials, gold, gems and a small magic glow. An element, when listed, sets the glow and details.
- The names are in Portuguese: use them as inspiration for the look, and never write them on the image.

Sheet I01:
1. sword named "Lâmina do Recruta", common (gray)
2. sword named "Espada de Treino", common (gray)
3. sword named "Agulha do Viajante", common (gray)
4. sword named "Velha Companheira", common (gray)
5. sword named "Cutelo do Lenhador", common (gray)
6. sword named "Foice-Verde do Bosque", uncommon (green), nature
7. sword named "Espada de Cobre Batido", uncommon (green)
8. sword named "Lâmina da Maré Baixa", uncommon (green), water

Sheet I02:
1. sword named "Marco do Guardião", uncommon (green)
2. sword named "Ferrão da Noite", uncommon (green), poison
3. sword named "Safira Gélida", rare (blue), ice
4. sword named "Lua Crescente", rare (blue)
5. sword named "Trovão Partido", rare (blue), lightning
6. sword named "Presa da Serpente", rare (blue), poison
7. sword named "Coração de Ametista", rare (blue), shadow
8. sword named "Monolito Desperto", epic (purple)

Sheet I03:
1. sword named "Escritura do Eclipse", epic (purple), shadow
2. sword named "Lâmina Sanguínea", epic (purple), shadow
3. sword named "Fragmento do Vórtice", epic (purple), lightning
4. sword named "Ceifadora do Vazio", epic (purple), shadow
5. sword named "Suspiro de Azhûr", unique (crimson), fire
6. sword named "Aurora Eterna", legendary (orange), holy light
7. sword named "Asa da Fênix", legendary (orange), fire
8. sword named "Sopro de Valdora", legendary (orange), nature

Sheet I04:
1. sword named "Coroa do Sol", legendary (orange), holy light
2. sword named "Fenda Esmeralda", unique (crimson), nature
3. sword named "Lágrima do Inverno", unique (crimson), ice
4. sword named "Raiz da Primeira Árvore", unique (crimson), nature
5. sword named "Silêncio Branco", unique (crimson), holy light
6. sword named "Sucata do Andarilho", unique (crimson), lightning
7. sword named "Juramento Celeste", divine (white-gold), holy light
8. sword named "Tridente do Primeiro Selo", divine (white-gold), water

Sheet I05:
1. sword named "Estrela Guia", divine (white-gold), holy light
2. sword named "Voz do Santuário", divine (white-gold), holy light
3. sword named "Gênese", divine (white-gold), holy light
4. bow named "Arco Curto", common (gray)
5. bow named "Arco do Batedor", common (gray)
6. bow named "Arco de Galho Seco", common (gray)
7. bow named "Arco de Bétula", common (gray)
8. bow named "Arco Remendado", common (gray)

Sheet I06:
1. bow named "Arco do Salgueiro", uncommon (green), nature
2. bow named "Arco de Espinheiro", uncommon (green), poison
3. bow named "Arco de Cobre Rubro", uncommon (green), fire
4. bow named "Arco da Correnteza", uncommon (green), water
5. bow named "Arco do Caçador", uncommon (green)
6. bow named "Arco da Névoa Azul", rare (blue), water
7. bow named "Arco da Lua Prateada", rare (blue), holy light
8. bow named "Arco do Relâmpago", rare (blue), lightning

Sheet I07:
1. bow named "Arco da Geada", rare (blue), ice
2. bow named "Arco de Jade", rare (blue), nature
3. bow named "Arco Ametista", epic (purple), shadow
4. bow named "Arco da Noite Violeta", epic (purple), shadow
5. bow named "Sussurro da Lua Sangrenta", unique (crimson), shadow
6. bow named "Arco Fragmentado", epic (purple), lightning
7. bow named "Arco do Eclipse", epic (purple), shadow
8. bow named "Arco de Magma Rúnico", legendary (orange), fire

Sheet I08:
1. bow named "Arco do Grifo Dourado", legendary (orange)
2. bow named "Arco da Fênix", legendary (orange), fire
3. bow named "Arco do Sol Nascente", legendary (orange), holy light
4. bow named "Arco do Bosque Ancestral", legendary (orange), nature
5. bow named "Arco de Braços Gêmeos", unique (crimson), lightning
6. bow named "Arco do Espelho", unique (crimson), shadow
7. bow named "Arco da Raiz Primeva", unique (crimson), nature
8. bow named "Arco Sem Sombra", unique (crimson), holy light

Sheet I09:
1. bow named "Arco do Viajante Perdido", unique (crimson), ice
2. bow named "Asas da Aurora", divine (white-gold), holy light
3. bow named "Arco da Árvore do Mundo", divine (white-gold), nature
4. bow named "Arco das Estrelas", divine (white-gold), holy light
5. bow named "Arco do Primeiro Rio", divine (white-gold), water
6. bow named "Arco do Vendaval Eterno", divine (white-gold), nature
7. head piece named "Capuz do Viajante", common (gray)
8. chestplate named "Colete do Viajante", common (gray)

Sheet I10:
1. leg armor named "Calças do Viajante", common (gray)
2. pair of boots named "Botas do Viajante", common (gray)
3. head piece named "Chapéu de Ferro do Recruta", common (gray)
4. chestplate named "Gibão Acolchoado", common (gray)
5. leg armor named "Calças Acolchoadas", common (gray)
6. pair of boots named "Botas Enfaixadas", common (gray)
7. head piece named "Gorro de Pele", common (gray)
8. chestplate named "Casaco de Peles", common (gray)

Sheet I11:
1. leg armor named "Calças de Peles", common (gray)
2. pair of boots named "Botas de Pele", common (gray)
3. head piece named "Elmo de Nasal", uncommon (green)
4. chestplate named "Cota de Malha Reforçada", uncommon (green)
5. leg armor named "Perneiras de Malha", uncommon (green)
6. pair of boots named "Botas de Ferro", uncommon (green)
7. head piece named "Capuz do Batedor", uncommon (green)
8. chestplate named "Gibão Tachonado", uncommon (green)

Sheet I12:
1. leg armor named "Calças Tachonadas", uncommon (green)
2. pair of boots named "Botas do Batedor", uncommon (green)
3. head piece named "Elmo de Escamas", uncommon (green)
4. chestplate named "Couraça de Escamas", uncommon (green)
5. leg armor named "Saiote de Escamas", uncommon (green)
6. pair of boots named "Botas de Bronze", uncommon (green)
7. head piece named "Elmo de Penacho Azul", rare (blue)
8. chestplate named "Peitoral com Tabardo", rare (blue)

Sheet I13:
1. leg armor named "Coxotes do Guarda", rare (blue)
2. pair of boots named "Grevas do Guarda", rare (blue)
3. head piece named "Elmo de Cristal de Gelo", rare (blue), ice
4. chestplate named "Couraça de Gelo", rare (blue), ice
5. leg armor named "Calças de Gelo e Pele", rare (blue), ice
6. pair of boots named "Botas de Neve", rare (blue), ice
7. head piece named "Elmo de Conchas", rare (blue), water
8. chestplate named "Couraça de Coral", rare (blue), water

Sheet I14:
1. leg armor named "Calças de Escama Marinha", rare (blue), water
2. pair of boots named "Botas da Maré", rare (blue), water
3. head piece named "Elmo Chifrudo da Noite", epic (purple), shadow
4. chestplate named "Peitoral Rúnico da Noite", epic (purple), shadow
5. leg armor named "Perneiras Rúnicas", epic (purple), shadow
6. pair of boots named "Botas Rúnicas", epic (purple), shadow
7. head piece named "Elmo do Ferrão", epic (purple), poison
8. chestplate named "Carapaça de Quitina", epic (purple), poison

Sheet I15:
1. leg armor named "Perneiras de Quitina", epic (purple), poison
2. pair of boots named "Botas de Garra", epic (purple), poison
3. head piece named "Elmo do Dragão", epic (purple), fire
4. chestplate named "Peitoral de Escamas Rubras", epic (purple), fire
5. leg armor named "Perneiras Rubras", epic (purple), fire
6. pair of boots named "Garras do Dragão", epic (purple), fire
7. head piece named "Elmo Alado do Grifo", legendary (orange)
8. chestplate named "Peitoral do Grifo", legendary (orange)

Sheet I16:
1. leg armor named "Coxotes Dourados", legendary (orange)
2. pair of boots named "Garras do Grifo", legendary (orange)
3. head piece named "Elmo de Obsidiana", legendary (orange), fire
4. chestplate named "Couraça de Magma", legendary (orange), fire
5. leg armor named "Perneiras de Magma", legendary (orange), fire
6. pair of boots named "Botas de Lava", legendary (orange), fire
7. head piece named "Elmo de Galhos", legendary (orange), nature
8. chestplate named "Couraça de Casca", legendary (orange), nature

Sheet I17:
1. leg armor named "Saiote de Folhas", legendary (orange), nature
2. pair of boots named "Botas de Raiz", legendary (orange), nature
3. head piece named "Elmo Espelhado", unique (crimson), shadow
4. chestplate named "Peitoral Espelhado", unique (crimson), shadow
5. leg armor named "Coxotes Espelhados", unique (crimson), shadow
6. pair of boots named "Botas Espelhadas", unique (crimson), shadow
7. head piece named "Elmo Sem Sombra", unique (crimson), holy light
8. chestplate named "Peitoral Sem Sombra", unique (crimson), holy light

Sheet I18:
1. leg armor named "Calças Sem Sombra", unique (crimson), holy light
2. pair of boots named "Botas Sem Sombra", unique (crimson), holy light
3. head piece named "Gorro Remendado", unique (crimson)
4. chestplate named "Colete Remendado", unique (crimson)
5. leg armor named "Calças Remendadas", unique (crimson)
6. pair of boots named "Botas Desparelhadas", unique (crimson)
7. head piece named "Elmo Alado da Aurora", divine (white-gold), holy light
8. chestplate named "Peitoral da Aurora", divine (white-gold), holy light

Sheet I19:
1. leg armor named "Saiote de Penas", divine (white-gold), holy light
2. pair of boots named "Botas Aladas", divine (white-gold), holy light
3. head piece named "Elmo de Madeira Viva", divine (white-gold), nature
4. chestplate named "Couraça da Árvore", divine (white-gold), nature
5. leg armor named "Perneiras de Raízes", divine (white-gold), nature
6. pair of boots named "Botas de Raiz Antiga", divine (white-gold), nature
7. head piece named "Elmo Estelar", divine (white-gold), holy light
8. chestplate named "Peitoral das Constelações", divine (white-gold), holy light

Sheet I20:
1. leg armor named "Coxotes Estelares", divine (white-gold), holy light
2. pair of boots named "Botas Estelares", divine (white-gold), holy light
3. glowing runic stone core with an orange diamond heart, legendary (orange)
4. broken gray stone shard with a glowing orange crack, rare (blue)
5. amulet with an orange diamond gem on a gold chain, epic (purple)
6. charm made of a pale blue crystal scorpion stinger, epic (purple)
7. charm of green glowing heartwood set in dark wood, epic (purple)
8. gold ring with a pointed jade stone, rare (blue)

Sheet I21:
1. dark leather necklace with a small blue moon pendant, epic (purple)
2. round glass flask of red healing potion with a cork, common (gray)
3. round glass flask of blue mana potion with a cork, common (gray)
4. round glass flask of green herbal potion with a cork, uncommon (green)
5. golden loaf of bread, common (gray)
6. drop of translucent teal jelly, common (gray)
7. gray wolf pelt with the head and paws spread out, common (gray)
8. sprig of green healing leaves, common (gray)

Sheet I22:
1. red mushroom with white spots, common (gray)
2. round mossy stone with a glowing green center, rare (blue)
3. purple amethyst-toned bat wing, common (gray)
4. round oak shield with an iron rim, common (gray)
5. pile of rusty iron scraps and bolts, common (gray)
6. coiled ring of rough bark rope, common (gray)
7. sharp black obsidian shard with a red ember glow, rare (blue)
8. small green antidote vial with a white cross label shape, no letters, uncommon (green)

Sheet I23:
1. brown leather travel satchel stuffed with food, common (gray)
2. folded parchment map with a red dotted route, uncommon (green)
3. rolled letter map of distant lands with a red wax seal, rare (blue)
4. old rolled treasure map with torn golden edges, uncommon (green)
5. roasted leg of game meat with a bone, common (gray)
6. brown rabbit pelt with long ears, common (gray)
7. pale branching deer antler, uncommon (green)
8. red bandana with white dots, folded in a triangle, common (gray)

Sheet I24:
1. bunch of dark purple wild berries with a leaf, common (gray)
```
