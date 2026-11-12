# Prompts das imagens dos monstros para virar 3D (23/10)

## Para que serve

Gerar no ChatGPT uma imagem de cada criatura, já na pose que dá o melhor 3D no site e o melhor rig depois. A ideia é a mesma dos heróis: a imagem vira modelo no site; eu passo para voxel no Blender, ponho o esqueleto e reaproveito ou refaço as animações.

## As regras de pose (o que faz o rig ficar bom)

- **Pose neutra e simétrica:** nada cruzado, nada encostado. Uma perna não toca a outra, o braço não toca o corpo, o rabo não toca as costas, a asa não toca o braço. Partes encostadas viram uma peça só no 3D, e o rig não consegue separar.
- **Juntas retas:** joelhos e cotovelos quase esticados. Dobras grandes na imagem viram dobras "moldadas" no modelo, e o rig deforma feio.
- **Boca fechada, olhos abertos.** A boca abrindo vem do rig, não da modelagem.
- **Sem arma, sem escudo, sem efeito** (fogo, magia, poeira). As armas dos goblins e bandidos são as do jogo, presas na mão depois.
- **Câmera:**
  - **Bípedes** (de pé, como gente): de frente, ortográfica, braços em A (~40° para baixo).
  - **Quadrúpedes e bichos compridos:** de **lado** (perfil), ortográfico, as quatro patas visíveis e separadas. Se o site aceitar várias imagens, faça também a de frente (prompt da folha de vistas no fim).
- **Fundo cinza-claro liso, luz uniforme, sem sombra, sem chão,** o bicho inteiro com folga em volta.
- **Uma criatura por imagem.**

## Bloco de estilo (colar primeiro, numa conversa só)

> I'm creating the creature 3D models for a cozy fantasy voxel RPG ("The World Gêneses", Minecraft-like blocky world). Each image will be converted into a 3D model by an image-to-3D tool and then rigged and animated, so follow these rules for every image in this conversation:
> - One single creature, full body, centered, orthographic camera, no perspective distortion, whole creature visible with margin around it.
> - Neutral rig-friendly pose: symmetrical, joints almost straight, limbs clearly separated from each other and from the body (no limb touching another part), mouth closed, eyes open.
> - Style: chunky voxel / blocky 3D look, cute but readable game-creature proportions, clean flat-shaded colored blocks with subtle pixel texture, soft even studio lighting, no strong shadows.
> - No weapons, no held items, no magic effects, no particles, no ground, no text.
> - Plain light-gray background.
> - High resolution, sharp edges.

## Os prompts (um por vez, depois do bloco)

A altura é a do bicho no jogo, para referência de tamanho.

### Bichos do começo

| Criatura (no jogo) | Câmera | Prompt |
|---|---|---|
| **Gota de Jade** (Slime, ~1,1 m) | frente, 3/4 de cima leve | "Jade slime creature: a rounded blocky jelly cube-blob, translucent jade-green with a darker green core visible inside, two big cute black eyes and a small smile, slightly wider at the bottom, no limbs. Front view." |
| **Cogumelo Errante** (Mushroom, ~1,3 m) | frente | "Walking mushroom creature: big round red mushroom cap with white blocky spots, thick cream-colored stem body with a cute grumpy face, two short stubby legs slightly apart and two small stubby arms angled 40 degrees down away from the body. Front view, A-pose." |
| **Morcego de Ametista** (Bat, ~1,0 m) | frente | "Amethyst bat creature: small round dark-purple furry body, big pointed ears, glowing violet eyes, small fangs visible with the mouth closed, small amethyst crystals on the head. Wings fully spread out flat and horizontal to both sides like a T-pose, wing membrane dark violet with visible finger bones, legs hanging straight down and apart. Front view." |
| **Sapo-Gigante do Brejo** (Frog, ~1,0 m) | **lado** | "Giant swamp frog creature: chunky green-brown frog with darker spots, yellow belly, big round eyes on top of the head, wide closed mouth. Standing on all four legs (not crouched): front legs straight down, back legs only slightly bent with the feet flat, legs separated from the body. Side profile view." |
| **Lobo** (Wolf) | **lado** | "Gray forest wolf creature: blocky gray wolf with lighter chest and muzzle, amber eyes, ears up, mouth closed. Standing straight on all four legs, legs straight and evenly apart, tail straight back horizontally (not touching the legs). Side profile view." |
| **Coelho** (Rabbit, ~0,8 m) | **lado** | "Cute wild rabbit: blocky light-brown rabbit with a white belly and white tail, long ears straight up and apart. Standing on all four paws (not sitting), legs visible and separated. Side profile view." |
| **Cervo** (Deer, ~1,6 m) | **lado** | "Forest deer (stag): blocky brown deer with a white chest and spots on the back, small branching antlers that do not touch each other, mouth closed. Standing straight on four long thin legs, evenly apart, short tail. Side profile view." |
| **Cavalo Selvagem** (Horse, ~2,1 m) | **lado** | "Wild horse: blocky bay-brown horse with black mane and tail, mane short and close to the neck, tail hanging straight back and down, NOT touching the legs, no saddle. Standing straight on four legs, evenly apart. Side profile view." (As 5 pelagens do jogo são o mesmo modelo com cores diferentes; gere só esta.) |

### Inimigos que andam de pé (mesmo esqueleto de gente)

| Criatura | Câmera | Prompt |
|---|---|---|
| **Goblin** (~1,0 m) | frente | "Goblin creature: small green-skinned goblin with big pointed ears angled sideways, large yellow eyes, big nose, mischievous closed-mouth grin, ragged brown leather vest and shorts (tight to the body), barefoot. A-pose: arms straight and angled 40 degrees down, hands open, legs slightly apart. Front view." |
| **Ladrão Emboscador** (Bandit, ~1,4 m) | frente | "Bandit thief: lean human bandit with a dark hood (hood down on the head, not covering the shoulders), black cloth mask over the mouth and nose, dark-brown leather jerkin, gray pants, soft boots. A-pose, hands open, no weapon. Front view." |
| **Guerreiro de Escudo** (BanditShield, ~1,4 m) | frente | "Bandit shield warrior: sturdy human bandit with a dented iron half-helmet, red scarf, patched leather armor with metal shoulder plates (close to the body), dark pants, boots. A-pose, hands open, no shield, no weapon. Front view." |
| **Saqueador Brutamontes** (BanditBrute, ~2,0 m) | frente | "Huge brute bandit: very big muscular human raider, bald with a black beard, scar over one eye, fur strap across the chest (tight), studded leather belt, ragged pants, heavy boots. A-pose, hands open, no weapon. Front view." |
| **Batedor Bandido** (BanditArcher, ~1,4 m) | frente | "Bandit scout archer: slim human with a green hood down on the head, leather bracers, light-green tunic with a brown belt, quiver strapped flat on the back (not sticking out to the sides), brown pants, boots. A-pose, hands open, no bow. Front view." |
| **Golem Musgoso** (Golem, ~2,4 m) | frente | "Mossy stone golem: big blocky gray-stone body covered with patches of green moss and small ferns, glowing green eyes in a small head, huge long arms with big stone fists, short thick legs. A-pose: arms angled 40 degrees down and away from the body (fists not touching the legs), legs apart. Front view." |

### Chefes

| Chefe | Câmera | Prompt |
|---|---|---|
| **Seryth, o Escorpião de Vidro** (~2,4 m de altura, bem largo) | **3/4 de cima** e depois de lado | "Giant glass scorpion boss: big scorpion made of translucent amber and teal glass blocks with a dark sand-colored inner core, glowing teal eyes. Pose for rigging: eight legs spread evenly out to the sides and straight, none touching each other; two big pincers stretched forward and slightly open, apart from each other; the tail raised up in a big curve above the body, the stinger high above the back and NOT touching the body. Three-quarter top view." |
| **Thorun, o Titã das Raízes** (~3,5 m) | frente | "Root titan boss: giant humanoid made of twisted tree roots and dark bark, mossy shoulders with small trees growing on them, glowing green eyes in a face of bark, long arms ending in root-fingered hands, thick legs ending in spreading root feet. A-pose: arms angled 40 degrees down, away from the body, legs apart, roots on the feet not touching each other. Front view." |
| **Azhûr, o Colosso da Caldeira** (~7 m) | frente | "Volcanic rune colossus boss: enormous humanoid made of black basalt blocks with glowing orange lava cracks and carved glowing blue runes, small head with a glowing furnace face, massive shoulders, huge arms with big blocky fists, thick legs. A-pose: arms angled 40 degrees down, away from the body, legs apart. Front view." |

### Thorun enraizado (decidido em 23/10, para quando o modelo novo chegar)

O usuário quer o Thorun **gigante e parado, preso nas raízes**. Assim não há pernas andando no rig, só tronco, braços e cabeça.

**Prompt novo** (substitui o de cima):

> "Rooted titan boss: gigantic tree titan whose lower body is a massive trunk merged into a wide base of thick roots spreading out on the ground in all directions (no legs), upper body made of twisted roots and dark bark, mossy shoulders with small trees growing on them, glowing green eyes in a face of bark, glowing green knots on some roots, two long arms ending in big root-fingered hands. Pose: arms angled 40 degrees down and clearly away from the body, fingers apart. Front view."

**A luta, só depois que o modelo chegar** (pedido do usuário: esperar o modelo):
- Velocidade 0; ele só gira para o jogador.
- **Ficam como estão:** Golpe de Raízes, Vinhas e Esporos.
- **Investida do Titã** sai. Entra **Raízes que correm**: uma linha de raízes saindo do chão até o jogador, com aviso.
- **Novo:** Chuva de sementes, que explodem longe dele (contra quem fica só de longe).
- **Fase 2** (metade da vida): os nós brilhantes viram pontos fracos e ele chama Duendes-Musgo.

## Folha de vistas (se o site aceitar várias imagens)

Depois de aprovar a imagem principal, na mesma conversa:

> "Now make a turnaround sheet of exactly this same creature, same colors, proportions and pose: front view, side view and back view, side by side, same scale, aligned at the bottom, plain light-gray background, orthographic camera, no text."

## Dicas do site (Meshy, Tripo e parecidos)

- Use **"image to 3D"** com a imagem principal e, se tiver, as vistas extras em **"multi-view"**.
- Peça o modelo **sem rig** (ou ignore o rig automático). Eu ponho o esqueleto no Blender, no mesmo padrão dos bichos atuais, para as animações de agora continuarem servindo onde der.
- Exporte em **GLB** (ou FBX), com textura.
- Salve em `Data/Creatures/Novos/<nome>.glb` e me avise. Comece por **um** (o lobo ou a gota) para eu testar o caminho inteiro antes dos outros.

## O que muda no jogo quando os modelos chegarem

- **Bichos de 4 patas (lobo, cavalo, cervo, coelho, sapo):** precisam de esqueleto de quadrúpede e das animações de andar, correr, atacar e morrer. As de hoje vieram do Godot; vejo se dá para reaproveitar caso a caso.
- **Goblin e bandidos:** podem usar o esqueleto dos heróis, então ganham todas as animações de golpe que os heróis já têm.
- **Morcego:** precisa só de bater asas, picar, guinchar e morrer.
- **Gota:** não precisa de esqueleto; o "pula e amassa" é feito em código.
- **Chefes:** os golpes deles são animações próprias, e vão ser o maior trabalho.

---

# Bestiário novo: monstros de cada região, noturnos, semi-chefes, chefes e dungeons de portal

**Pedido:** monstros novos para cada bioma, monstros noturnos, chefes e semi-chefes, e monstros para as dungeons de portal que o usuário vai fazer.

**O que está pronto e o que falta:**
- **Já feito:** este bestiário com os prompts, para gerar as imagens e os modelos.
- **Ainda não está no jogo:** cada monstro só entra quando o modelo chegar. Aí eu crio a espécie (vida, golpes, nível, onde nasce, se é de noite) e as animações.
- **Chefes das regiões:** são os da história (`HISTORIA_DO_MUNDO.md`). Seryth, Thorun e Azhûr já existem; os outros 6 são novos.

**Como ler as tabelas:**
- **Papel:**
  - **Comum:** nasce de dia e de noite.
  - **Noturno:** só nasce das 19h às 5h, perto do jogador, e some de manhã.
  - **Semi-chefe:** um por região, num lugar fixo; mais forte, com barra de vida no alto e saque melhor.
  - **Chefe:** o da história; arena e fases.
- **Câmera e pose:** seguem as regras do começo deste arquivo (frente em A para quem anda de pé, lado para quadrúpedes). Tipos de corpo novos:
  - **Cobra ou verme (sem patas):** corpo **reto na horizontal**, de lado, sem enrolar.
  - **Várias cabeças:** pescoços **abertos em leque**, sem um tocar o outro.
  - **Quadrúpede com asas** (dragão): de **frente em 3/4 de cima**, asas abertas retas para os lados.
  - **Fantasma flutuante:** de frente, braços em A, a "cauda" de fumaça reta para baixo, separada.
  - **Inseto ou aranha:** **3/4 de cima**, patas abertas por igual, nenhuma tocando outra.
- Cole o **bloco de estilo** do começo do arquivo antes de cada conversa.

## Valdora: campos, lagos e a mata do vale (nível 1–15)

| Monstro | Papel | Câmera | Prompt |
|---|---|---|---|
| **Javali-Espinho** | comum, 3–10, investe em linha reta | lado | "Thorn boar: chunky brown wild boar with a ridge of short wooden thorns along the back, two small tusks, mouth closed, standing straight on four legs evenly apart, short tail straight back. Side profile view." |
| **Vespa-Lanceira** | comum, 5–12, voa e pica | frente | "Giant lance wasp: yellow and black striped blocky wasp with a long stinger pointing straight down, big glossy eyes, two pairs of transparent wings spread flat to the sides like a T-pose, six thin legs hanging down and apart. Front view." |
| **Espantalho Vivo** | **noturno**, 4–12 | frente, A | "Living scarecrow: straw-stuffed body in a patched brown shirt and pants, burlap sack head with stitched glowing orange eyes and a stitched smile, wide straw hat, straw sticking out of the cuffs. A-pose, arms angled 40 degrees down, legs apart. Front view." |
| **Rei Javali** | **semi-chefe**, 12 | lado | "Boar king: huge dark-brown boar with large curved ivory tusks, a crown of thorny branches on the head, scars, thick fur mane, mouth closed, standing on four legs evenly apart. Side profile view." |
| **Azhûr, o Colosso da Caldeira** | chefe (já existe) | — | (prompt na seção Chefes, mais acima) |

## Aurivale: planícies douradas, trigo e colmeias (nível 10–22)

| Monstro | Papel | Câmera | Prompt |
|---|---|---|---|
| **Raposa-Dourada** | comum, 10–18, bate e foge | lado | "Golden fox: slim blocky fox with golden-orange fur, white chest and tail tip, big fluffy tail held straight back, ears up, mouth closed, standing on four legs evenly apart. Side profile view." |
| **Abóbora-Guerreira** | comum, 12–20, rola e morde | frente, A | "Pumpkin warrior: round orange pumpkin head with a carved angry face glowing yellow inside, small body made of twisted green vines, leaf shoulder pads, vine arms angled 40 degrees down, short vine legs apart. Front view." |
| **Mariposa Lunar** | **noturno**, 12–20, solta pó que adormece | frente | "Moon moth: big fluffy pale-blue moth with glowing silver-white wing patterns, feathery antennae apart, black eyes, four large wings spread flat and fully open to the sides, six small legs hanging down and apart. Front view." |
| **Urso-do-Mel** | **semi-chefe**, 20 | lado | "Honey bear: big round golden-brown bear with honey on the muzzle (mouth closed), a broken wooden beehive stuck on one shoulder, standing on all four legs evenly apart. Side profile view." |
| **Vespera, a Rainha do Enxame** | **chefe** (história) | frente | "Swarm queen boss: giant regal wasp queen, golden and black armored body, crown-shaped head crest, four big translucent amber wings spread flat to the sides, upper pair of thin arms angled down and apart, long stinger pointing straight down, six legs apart. Front view." |

## Verdalume: mata fechada e luminosa (nível 15–30)

| Monstro | Papel | Câmera | Prompt |
|---|---|---|---|
| **Aranha-Folha** | comum, 15–25, teia que prende | 3/4 de cima | "Leaf spider: green spider whose body is covered in overlapping leaves, eight long legs spread evenly around the body and straight, none touching, small glowing green eyes. Three-quarter top view." |
| **Duende-Musgo** | comum, 15–25, joga sementes | frente, A | "Moss sprite: small chubby forest creature made of moss and bark, mushroom cap hat, big round glowing yellow eyes, twig arms angled 40 degrees down, short legs apart. Front view." |
| **Lobo-Espectral** | **noturno**, 18–28, some e reaparece | lado | "Spectral wolf: translucent glowing teal ghost wolf with darker blue core, wisps on the tail end, glowing white eyes, mouth closed, standing on four legs evenly apart, tail straight back. Side profile view." |
| **Sentinela de Pedra-Lua** | **semi-chefe**, 28 | frente, A | "Moonstone sentinel: tall elven stone guardian statue made of white moonstone blocks with glowing pale-blue elven runes, leafy stone crown, arms angled 40 degrees down, legs apart. Front view." |
| **Thorun, o Titã das Raízes** | chefe (já existe) | — | (seção Chefes) |

## Solcânion: dunas e cânions de vidro (nível 20–35)

| Monstro | Papel | Câmera | Prompt |
|---|---|---|---|
| **Escaravelho de Vidro** | comum, 20–30, carapaça que reflete magia | 3/4 de cima | "Glass scarab: big beetle with a shiny shell of translucent amber glass, sand-colored body, six legs spread evenly and straight, two small horns forward, wings closed under the shell. Three-quarter top view." |
| **Lagarto-Duna** | comum, 22–32, cospe areia | lado | "Dune lizard: long sandy-orange lizard with darker stripes, frilled neck folded flat, mouth closed, four legs out to the sides and straight, long tail straight back on the ground. Side profile view." |
| **Múmia de Areia** | **noturno**, 24–34 | frente, A | "Sand mummy: thin undead wrapped in tight beige bandages (no loose strips), glowing yellow eyes through the wraps, small golden jewelry on the neck, arms angled 40 degrees down, legs apart. Front view." |
| **Verme-das-Dunas** | **semi-chefe**, 34, sai da areia | lado | "Dune worm: huge segmented sand worm with armored sandy plates, round mouth closed at the front with small teeth around it, body completely straight and horizontal (not coiled), no legs. Side profile view." |
| **Seryth, o Escorpião de Vidro** | chefe (já existe) | — | (seção Chefes) |

## Lodonegro: pântano escuro e névoa (nível 25–40)

| Monstro | Papel | Câmera | Prompt |
|---|---|---|---|
| **Crocodilo-do-Lodo** | comum, 25–35, agarra e arrasta | lado | "Mud crocodile: dark green-brown crocodile covered in dried mud and small swamp plants, mouth closed, four short legs out to the sides and straight, long tail straight back. Side profile view." |
| **Bruxa-do-Brejo** | comum, 28–38, maldições à distância | frente, A | "Swamp hag: hunched old witch with green skin, long nose, wild dark hair close to the head, ragged moss-green dress that ends at the knees (legs visible), arms angled 40 degrees down, no staff. Front view." |
| **Fogo-Fátuo** | **noturno**, 26–38, atrai e explode | frente | "Wisp spirit: small floating spirit made of a glowing pale-green flame core with a cute blank face, two tiny flame arms out to the sides, a straight flame tail pointing down. Front view." |
| **Hidra do Brejo** | **semi-chefe**, 38 | 3/4 de frente | "Swamp hydra: three-headed lizard beast, dark teal scales, the three long necks spread apart in a fan (none touching), heads with mouths closed, four thick legs evenly apart, tail straight back. Three-quarter front view." |
| **A Mãe-Lodo** | **chefe** (história) | frente | "Mud mother boss: gigantic swamp toad-mother made of mud and roots, mossy back with small trees and lily pads, big glowing yellow eyes, wide closed mouth, front legs straight and apart, back legs slightly bent with feet flat. Front view." |

## Picos de Raikor: montanhas de tempestade (nível 30–45)

| Monstro | Papel | Câmera | Prompt |
|---|---|---|---|
| **Cabra-Trovão** | comum, 30–40, chifrada com raio | lado | "Thunder goat: sturdy mountain goat with gray-blue fur, big curled horns crackling with small blue lightning lines, mouth closed, standing on four legs evenly apart. Side profile view." |
| **Harpia da Tempestade** | comum, 32–42, mergulha | frente | "Storm harpy: bird-woman with dark-blue feathers, yellow eyes, wild feathered hair close to the head, large wings as arms spread straight out to the sides like a T-pose, bird talon feet apart. Front view." |
| **Gárgula** | **noturno**, 32–44, de dia é estátua | frente | "Gargoyle: stone gargoyle with gray cracked rock skin, small horns, glowing blue eyes, bat-like stone wings spread straight out to the sides, arms angled 40 degrees down, clawed feet apart. Front view." |
| **Ogro da Montanha** | **semi-chefe**, 44 | frente, A | "Mountain ogre: huge gray-skinned ogre with a fur cloak tied tight on the shoulders, one big tusk, small eyes, round belly, arms angled 40 degrees down with open hands, legs apart, no club. Front view." |
| **Raikor, a Ave-Trovão** | **chefe** (história) | frente | "Thunderbird boss: giant storm eagle with dark-blue and white feathers, glowing electric-yellow feather tips and eyes, wings fully spread straight out to the sides, legs with talons hanging straight down and apart, tail feathers spread down. Front view." |

## Brasafenda: lava, cinzas e o vulcão (nível 40–55)

| Monstro | Papel | Câmera | Prompt |
|---|---|---|---|
| **Salamandra de Brasa** | comum, 40–50, rastro de fogo | lado | "Ember salamander: black salamander with glowing orange lava cracks, small flame crest on the back, mouth closed, four short legs out to the sides and straight, tail straight back. Side profile view." |
| **Diabrete de Cinzas** | comum, 42–52, voa e lança brasas | frente | "Ash imp: small dark-gray imp with glowing ember eyes, little horns, pointed ears, small bat wings spread straight out to the sides, thin arms angled 40 degrees down, legs apart, short tail straight down. Front view." |
| **Cão Infernal** | **noturno**, 44–54 | lado | "Hellhound: black blocky hound with glowing magma cracks, red eyes, flame mane close to the neck, mouth closed, standing on four legs evenly apart, tail straight back. Side profile view." |
| **Ferreiro de Magma** | **semi-chefe**, 54 | frente, A | "Magma smith: big blacksmith golem made of dark iron plates and glowing magma, leather apron, chimney-like vents on the shoulders, arms angled 40 degrees down, open hands, legs apart, no hammer. Front view." |
| **O Forjador Rubro** | **chefe** (história) | frente, A | "Crimson forger boss: towering demon blacksmith in crimson and black armor, horned helmet with a glowing visor, molten metal veins, huge arms angled 40 degrees down with open gauntlets, thick legs apart, no weapon. Front view." |

## Nivaluz: gelo e auroras (nível 45–60)

| Monstro | Papel | Câmera | Prompt |
|---|---|---|---|
| **Urso-de-Gelo** | comum, 45–55 | lado | "Ice bear: big white polar bear with ice crystals growing on the back and shoulders, light-blue eyes, mouth closed, standing on four legs evenly apart. Side profile view." |
| **Yeti** | comum, 47–57, joga bolas de neve | frente, A | "Yeti: tall shaggy white-furred yeti with a blue face, small horns, arms angled 40 degrees down with open hands, big feet apart. Front view." |
| **Espírito da Nevasca** | **noturno**, 48–58 | frente | "Blizzard wraith: floating ghostly figure made of swirling snow and pale-blue light, hooded head with glowing white eyes, arms angled 40 degrees down, a straight trail of snow pointing down instead of legs. Front view." |
| **Mamute de Gelo** | **semi-chefe**, 58 | lado | "Ice mammoth: huge woolly mammoth with frosted white-blue fur, long curved tusks that do not touch each other or the trunk, trunk hanging straight down, standing on four thick legs evenly apart. Side profile view." |
| **Aurora, a Que Congela a Luz** | **chefe** (história) | 3/4 de cima | "Aurora ice dragon boss: elegant ice dragon with translucent crystal scales shimmering with aurora green and purple, wings fully spread straight out to the sides, four legs evenly apart, long neck forward, mouth closed, long tail straight back. Three-quarter top view." |

## Morthúria: terra dos mortos-vivos e ruínas (nível 55–70)

| Monstro | Papel | Câmera | Prompt |
|---|---|---|---|
| **Esqueleto Guerreiro** | comum, 55–65 (esqueleto de gente) | frente, A | "Skeleton warrior: blocky bone skeleton with glowing purple eyes, rusty iron helmet and chest plate close to the body, arms angled 40 degrees down with open hands, legs apart, no weapon. Front view." |
| **Carniçal** | comum, 57–67, rápido | frente, A | "Ghoul: thin hunched undead with gray-purple skin, long claws, glowing yellow eyes, torn dark rags tight to the body, arms angled 40 degrees down, legs apart. Front view." |
| **Banshee** | **noturno**, 58–68, grito que atordoa | frente | "Banshee: floating ghost woman with long pale hair flowing straight down the back, glowing purple eyes, translucent violet dress that becomes a straight smoky tail pointing down, arms angled 40 degrees down. Front view." |
| **Cavaleiro da Morte** | **semi-chefe**, 68 | frente, A | "Death knight: tall undead knight in black and dark-purple plate armor, horned helmet with glowing purple eyes, tattered cape short and tight to the back, arms angled 40 degrees down, legs apart, no weapon. Front view." |
| **O Rei Sem Sombra** | **chefe** (história) | frente, A | "Shadowless king boss: tall lich king with a pale skull face and glowing white eyes, tall jagged crown, regal dark robe that ends at the knees (legs visible), purple Vortex crystals floating close to the shoulders, arms angled 40 degrees down with open hands, no staff. Front view." |

## Noturnos de todo o continente

Aparecem de noite em qualquer região, além do noturno de cada uma:

| Monstro | Nível | Câmera | Prompt |
|---|---|---|---|
| **Zumbi** | o da região | frente, A | "Zombie: blocky green-gray zombie in torn villager clothes tight to the body, glowing pale eyes, arms angled 40 degrees down, legs apart. Front view." |
| **Sombra do Vórtice** | o da região +3 | frente | "Vortex shade: humanoid made of swirling dark-purple smoke with tiny star sparkles inside, glowing white eyes, arms angled 40 degrees down, the lower body a straight smoky tail pointing down. Front view." |

## Dungeons de portal: as fendas do Vórtice

Monstros pensados para dungeons por tema. O nível acompanha o portal.

### Cripta

| Monstro | Papel | Câmera | Prompt |
|---|---|---|---|
| **Rato-da-Cripta** | comum, em bando | lado | "Crypt rat: big gray rat with red eyes, bald tail held straight back, mouth closed, standing on four legs evenly apart. Side profile view." |
| **Esqueleto Arqueiro** | comum, à distância | frente, A | "Skeleton archer: blocky skeleton with a tattered green hood down on the skull, leather bracers, glowing teal eyes, arms angled 40 degrees down, legs apart, no bow. Front view." |
| **Mímico** | armadilha (parece baú) | 3/4 de frente | "Mimic chest monster: wooden treasure chest with iron bands, the lid slightly open showing teeth and a tongue inside, two big eyes on the front, four stubby legs evenly apart under it. Three-quarter front view." |
| **O Necromante da Cripta** | **chefe da dungeon** | frente, A | "Crypt necromancer boss: tall robed necromancer with a bone mask, glowing green eyes, robe ending at the knees (legs visible), floating skulls close to the shoulders, arms angled 40 degrees down, no staff. Front view." |

### Caverna de Cristal

| Monstro | Papel | Câmera | Prompt |
|---|---|---|---|
| **Golem de Cristal** | comum, resistente | frente, A | "Crystal golem: blocky golem made of purple and pink crystal clusters, glowing core in the chest, arms angled 40 degrees down, legs apart. Front view." |
| **Aranha de Cristal** | comum | 3/4 de cima | "Crystal spider: spider with a body of blue crystal shards, eight long legs spread evenly and straight, none touching, glowing white eyes. Three-quarter top view." |
| **Morcego-Gema** | comum, voa em bando | frente | "Gem bat: small bat with a shiny sapphire crystal body, glowing blue eyes, wings spread flat to the sides like a T-pose, legs hanging straight down and apart. Front view." |
| **A Rainha de Cristal** | **chefe da dungeon** | 3/4 de cima | "Crystal spider queen boss: giant spider queen made of glowing violet crystal, crown of crystal spikes on the head, eight long legs spread evenly and straight, none touching, a crystal egg sack on the back. Three-quarter top view." |

### Forja do Vórtice

| Monstro | Papel | Câmera | Prompt |
|---|---|---|---|
| **Autômato de Bronze** | comum | frente, A | "Bronze automaton: clockwork robot made of bronze plates and gears, one round glowing eye, arms angled 40 degrees down with open hands, legs apart. Front view." |
| **Cavaleiro de Lava** | comum | frente, A | "Lava knight: knight in black obsidian armor with glowing lava between the plates, flame plume on the helmet, arms angled 40 degrees down, legs apart, no weapon. Front view." |
| **Cria do Vórtice** | comum, rápida | lado | "Vortex spawn: four-legged dark-purple beast with a swirling vortex pattern on the back, many small glowing eyes, mouth closed, standing on four legs evenly apart, tail straight back. Side profile view." |
| **O Guardião da Fenda** | **chefe da dungeon** | frente, A | "Rift guardian boss: huge armored guardian with a body of floating purple stone pieces held together by Vortex energy, a swirling portal in the chest, arms angled 40 degrees down, legs apart. Front view." |

## Ordem sugerida para gerar

1. **Um de cada tipo de corpo** para testar o caminho inteiro (imagem → site → Blender → jogo): Javali-Espinho (quadrúpede), Espantalho Vivo (de pé), Vespa-Lanceira (com asas) e Verme-das-Dunas (sem patas).
2. **Valdora e Aurivale:** são as regiões do começo do jogo.
3. **O resto,** região por região, e as dungeons quando o sistema de portais existir.

---

# Fauna de cada região (animais que não atacam)

**Pedido:** "já vai melhorando a fauna de cada região também".

## Já no jogo: os animais de hoje, distribuídos por região

Antes, coelho, cervo e cavalo nasciam iguais no mapa todo: 42% das quadras, até no vulcão, na neve e na terra dos mortos. Agora cada região tem a sua mistura (`UVortexCreatureSubsystem`, tabela `Faunas`):

| Região | Quadras com animais | Mistura | Manada | Cavalos |
|---|---|---|---|---|
| Valdora (vale e campos) | 42% (igual a antes) | cavalo 15%, cervo 40%, coelho | normal | todas as pelagens |
| Aurivale (planícies douradas) | 55% | cavalo 40%, cervo 30%, coelho | **+2** (manadas grandes) | palomino, baio, malhado |
| Verdalume (mata) | 50% | cervo 70%, coelho | +1 | nenhum |
| Picos de Raikor | 28% | cavalo 20%, cervo 55%, coelho | normal | tordilho, negro |
| Nivaluz (neve) | 18% | cervo 50%, coelho | normal | nenhum |
| Lodonegro (pântano) | 20% | cervo 35%, coelho | normal | nenhum |
| Solcânion (deserto) | 10% | cavalo 30%, coelho | normal | palomino, baio |
| Brasafenda (vulcão) | 4% | só coelho | normal | nenhum |
| Morthúria (mortos) | nenhum | — | — | — |

O vale continua exatamente igual (as mesmas chances de antes, então os mesmos animais nos mesmos lugares).

## Animais novos para gerar (prompts)

As mesmas regras de pose dos monstros: quadrúpedes **de lado**, aves com **asas abertas retas** de frente, nada encostado. Cole o bloco de estilo antes.

| Região | Animal | Câmera | Prompt |
|---|---|---|---|
| Valdora | **Ovelha** | lado | "Fluffy sheep: blocky white woolly sheep with a black face and black legs, mouth closed, standing on four legs evenly apart. Side profile view." |
| Valdora | **Pato do lago** | lado | "Wild duck: blocky mallard duck with green head, brown chest and gray body, orange feet apart, wings closed against the body. Side profile view." |
| Valdora | **Esquilo** | lado | "Red squirrel: small blocky red squirrel with a big fluffy tail held straight up and back (not touching the back), standing on four paws. Side profile view." |
| Aurivale | **Bisão Dourado** | lado | "Golden bison: big blocky bison with golden-brown fur, darker shaggy front, small curved horns, mouth closed, standing on four legs evenly apart. Side profile view." |
| Aurivale | **Faisão** | lado | "Pheasant: blocky pheasant with copper-red body, green-purple head, long striped tail straight back, legs apart. Side profile view." |
| Aurivale | **Abelhão** (não ataca) | frente | "Fat bumblebee: round fuzzy yellow and black bumblebee, cute black eyes, small transparent wings spread flat to the sides, six tiny legs hanging down and apart. Front view." |
| Verdalume | **Cervo-Luz** | lado | "Light stag: elegant white stag with glowing pale-green spots and glowing branching antlers that do not touch each other, mouth closed, standing on four legs evenly apart. Side profile view." |
| Verdalume | **Coruja** | frente | "Forest owl: blocky brown owl with big round orange eyes, white face disc, wings spread straight out to the sides, legs hanging straight down and apart. Front view." |
| Verdalume | **Tartaruga-Musgo** | lado | "Moss turtle: blocky turtle with a big shell covered in green moss and tiny flowers, head out and forward, four short legs out to the sides. Side profile view." |
| Solcânion | **Camelo** | lado | "Desert camel: blocky sandy-tan camel with one hump, mouth closed, standing on four long legs evenly apart, short tail straight down. Side profile view." |
| Solcânion | **Feneco** (raposa do deserto) | lado | "Fennec fox: small cream-colored fox with huge ears straight up and apart, black eyes, bushy tail straight back, standing on four legs. Side profile view." |
| Lodonegro | **Garça** | lado | "Gray heron: tall blocky gray heron with a long straight neck, long beak forward, wings closed against the body, two long thin legs apart. Side profile view." |
| Lodonegro | **Capivara** | lado | "Capybara: chubby blocky brown capybara, calm face, mouth closed, standing on four short legs evenly apart. Side profile view." |
| Raikor | **Cabra-Montesa** | lado | "Mountain ibex: blocky gray-brown ibex with long curved horns that do not touch the back, mouth closed, standing on four legs evenly apart. Side profile view." |
| Raikor | **Águia** | frente | "Mountain eagle: blocky golden eagle with brown body and white head, wings fully spread straight out to the sides, talons hanging straight down and apart. Front view." |
| Raikor | **Marmota** | lado | "Marmot: small chubby blocky brown marmot standing on four paws, short tail straight back. Side profile view." |
| Brasafenda | **Besouro-Brasa** (não ataca) | 3/4 de cima | "Ember beetle: small black beetle with a glowing orange shell, six legs spread evenly and straight, none touching. Three-quarter top view." |
| Brasafenda | **Lagartixa de Lava** | lado | "Lava gecko: small black gecko with glowing orange dots, mouth closed, four legs out to the sides, tail straight back. Side profile view." |
| Nivaluz | **Rena** | lado | "Reindeer: blocky brown-gray reindeer with a white neck, big branching antlers that do not touch each other, mouth closed, standing on four legs evenly apart. Side profile view." |
| Nivaluz | **Raposa-do-Ártico** | lado | "Arctic fox: small pure-white blocky fox with black eyes and nose, fluffy tail straight back, standing on four legs. Side profile view." |
| Nivaluz | **Pinguim** | frente | "Penguin: blocky black and white penguin with a yellow neck patch, flippers angled 40 degrees down away from the body, feet apart. Front view." |
| Morthúria | **Corvo** | frente | "Raven: blocky black raven with glowing purple eyes, wings spread straight out to the sides, legs hanging straight down and apart. Front view." |
| Lagos e rios | **Peixe** (para pescar no futuro) | lado | "Lake fish: blocky silver-blue fish with darker back and orange fins, body straight, tail fin straight back. Side profile view." |

**No jogo:**
- **Comportamento:** cada animal novo entra com o mesmo comportamento dos de hoje. Foge quando apanha, anda em bando e dá couro, carne, pena ou lã quando vencido.
- **Aves:** voam baixo, como o morcego, e pousam.
- **Peixes:** ficam para quando houver pesca.
