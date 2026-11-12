# Prompts para gerar os ícones e imagens do menu no ChatGPT (23/10)

## Motivo

O usuário achou feios os ícones do menu. Os de hoje são uma mistura:
- **Jogar, Multiplayer, Configurações e Sair:** pequenos modelos de voxel tirados do jogo;
- **Conquistas:** um troféu branco e chapado.

Ficam pequenos, escuros e sem um estilo comum. Este arquivo traz os prompts prontos para gerar ícones novos, todos no mesmo estilo, e a lista do que ainda falta de imagem no jogo.

## Situação (23/10, tarde)

- **Gerados e importados:** os 12 ícones do menu, gerados pelo usuário no ChatGPT com estes prompts.
  - **Onde ficam:** `Data/UI/Menu/` (reduzidos a 256 px para não serrilhar sem mipmaps); os originais de 1024 px em `Data/UI/Menu/Originais/`.
  - **Importados para:** `/Game/Vortex/UI/Menu/Art`.
- **Onde cada um aparece:**
  - **início:** Jogar, Multiplayer, Configurações, Conquistas, Sair;
  - **Criar sala:** fogueira;
  - **Entrar na sala:** chave, também na página do código;
  - **Criar mundo:** a carta "Novo mundo" e o botão;
  - **Criar herói:** a carta "Novo herói" e o botão;
  - **Excluir:** herói, mundo e as confirmações;
  - **Voltar:** o "Voltar" do alto de todas as páginas, o da escolha de raça e o Cancelar;
  - **Pronto:** "Estou pronto" e "Pronto! Começar aventura".
- **Botões com ícone:** os botões do menu (`SVortexButton`) agora aceitam um ícone à esquerda do texto, que cresce um pouco com o mouse em cima.
- **Para refazer um ícone:** gere de novo, salve com o mesmo nome em `Data/UI/Menu/` (em 256 px), feche o editor e rode `Tools/ImportMenuIcons.py`.
- **Ainda faltam:** as imagens da seção "O que mais falta".

## Como usar

1. Cole primeiro o **bloco de estilo** no ChatGPT (gerador de imagens). Depois, um prompt de ícone por vez, na mesma conversa, para manter o estilo.
2. Peça sempre **PNG com fundo transparente, 1024×1024**.
3. Salve com o nome indicado em `Data/UI/Menu/` (ex.: `Data/UI/Menu/T_MenuIcon_Play.png`).
4. Com o editor fechado, rode `Tools/ImportMenuIcons.py`. Ele importa todo `T_MenuIcon_*.png` dessa pasta para `/Game/Vortex/UI/Menu/Art` e substitui o ícone de mesmo nome. O menu já usa esses nomes, então não precisa mexer no código.
5. Se o ícone não ficar bom num tamanho pequeno (o menu desenha a ~72 px), peça: "mesma imagem, formas mais grossas e menos detalhes".

## Bloco de estilo (colar primeiro)

> I'm making a set of game menu icons for a cozy fantasy voxel RPG called "The World Gêneses" (Minecraft-like blocky world, warm golden UI with dark navy-blue panels and emerald-green gem accents). Style for EVERY icon in this set: chunky voxel / blocky 3D look, slightly isometric 3/4 view, soft cartoon lighting from the top-left, thick dark outline around the silhouette, warm saturated colors, small emerald-green glowing gem accent, centered object filling about 80% of the canvas, transparent background, no text, no letters, no frame, no shadow on the ground. 1024×1024 PNG with transparent background. Keep the same style, lighting and outline thickness for all icons I ask next.

## Ícones do menu inicial

| Arquivo | Botão | Prompt |
|---|---|---|
| `T_MenuIcon_Play.png` | Jogar | Icon for "PLAY / single player": a short voxel steel sword with a golden cross-guard and an emerald gem in the pommel, crossed over a small round wooden shield with a gold rim, slight heroic tilt. |
| `T_MenuIcon_Join.png` | Multiplayer | Icon for "MULTIPLAYER": three small cute voxel adventurer heads side by side (one with a blue hood, one with a red bandana, one with an elf ear and a green leaf), the middle one slightly in front, with a tiny golden banner arch behind them. |
| `T_MenuIcon_Options.png` | Configurações | Icon for "SETTINGS": a chunky golden voxel gear with an emerald gem in the center, a smaller iron gear interlocked behind it, small sparkle on the gold. |
| `T_MenuIcon_Trophy.png` | Conquistas | Icon for "ACHIEVEMENTS": a chunky golden voxel trophy cup with two round handles, an emerald star on the front, on a small dark stone base, three tiny sparkles around it. |
| `T_MenuIcon_Quit.png` | Sair | Icon for "QUIT": a heavy wooden voxel door with iron bands, slightly open, warm light coming out of the gap, a small hanging lantern beside it. |
| `T_MenuIcon_Key.png` | Entrar na sala (código) | Icon for "JOIN A FRIEND'S ROOM": a golden voxel key with an emerald gem in the bow, turning inside a small stone portal ring glowing green. |
| `T_MenuIcon_Host.png` | Criar sala | Icon for "HOST A ROOM": a small voxel campfire with three log seats around it and a little flag pole with a golden pennant, cozy glow. |
| `T_MenuIcon_NewWorld.png` | Criar mundo | Icon for "NEW WORLD": a tiny floating voxel island with grass, one round tree, a waterfall falling off the edge, and a golden plus sign sparkle above it. |
| `T_MenuIcon_NewHero.png` | Criar herói | Icon for "NEW HERO": a cute voxel adventurer silhouette (blank face, simple tunic) with a golden plus badge on the corner, standing on a small round pedestal. |
| `T_MenuIcon_Delete.png` | Apagar mundo ou herói | Icon for "DELETE": a small voxel stone tombstone cracked in the middle with a red gem, a few falling pebbles. |
| `T_MenuIcon_Back.png` | Voltar | Icon for "BACK": a chunky golden voxel arrow curving to the left, with an emerald gem in its tail. |
| `T_MenuIcon_Ready.png` | Estou pronto (sala) | Icon for "READY": a round green voxel shield with a big white check mark, small golden rim. |

> **Atenção:** `Key`, `Host`, `NewWorld`, `NewHero`, `Delete`, `Back` e `Ready` ainda não têm lugar no código. Depois de gerar, me avise que eu ligo cada um no botão certo.

## Arte grande do menu

| Arquivo | Onde | Prompt |
|---|---|---|
| `T_MenuWorldArt.png` (1024×1024) | Cartão "Continuar" e lista de mundos | A small floating voxel world island seen from 3/4 view: grassy top with round trees, a tiny village with orange roofs, a waterfall falling into clouds, a giant glowing tree (Yggdrasil) on one side, emerald crystals sticking out of the dirt below, soft sunset light, transparent background, cozy fantasy voxel style, no text. |
| `T_MenuBackground.png` (1920×1080) | Fundo do menu (se trocar a cena 3D por imagem) | Wide cozy fantasy voxel landscape at golden hour: rolling blocky green hills, a winding river, a walled medieval voxel town in the middle distance, a gigantic glowing tree on the far horizon, light fog in the valleys, warm sky with soft clouds, the left third of the image darker and calmer (room for menu buttons), no characters, no text. |
| `T_Emblem.png` (1024×1024) | Emblema / ícone do jogo | Game emblem for "The World Gêneses": a round golden voxel medallion with a stylized blocky tree in the middle whose roots form a circle, an emerald gem at the top, dark navy enamel background inside the ring, transparent outside, no text. |

## O que mais falta de imagem no jogo

Hoje **todos os 257 ícones de itens e técnicas existem** (`Content/Vortex/UI/Icons`, 296 arquivos). O que falta:

1. **Medalhas das conquistas:** são 14, e todas usam o mesmo troféu. Uma medalha por conquista (`T_Badge_<id>.png`, 512×512):
   - **Prompt base** (depois do bloco de estilo): "Round voxel achievement badge, golden rim with a small emerald gem at the top, dark navy enamel center, and in the center: ___".
   - Troque `___` por:
     - **criar herói:** "a tiny voxel hero with a sword raised";
     - **nível 5 / 25 / 50:** "a bronze / silver / gold star with the number 5 / 25 / 50 made of blocks";
     - **Despertar:** "a glowing crystal heart cracking open with light";
     - **5 / 15 / 31 missões:** "a rolled parchment scroll with a red wax seal (1 / 2 / 3 seals)";
     - **domar cavalo:** "a voxel horse head with a golden bridle";
     - **5% / 25% do mapa:** "a folded treasure map with a compass on top";
     - **fazer um amigo:** "two voxel hands shaking";
     - **criar mundo de multiplayer:** "three tiny voxel tents around a campfire";
     - **1.000 moedas:** "a pile of golden voxel coins with one gem".
2. **Cartões das 9 regiões** (para o letreiro de entrada, o mapa e a tela de carregamento; `T_Region_<nome>.png`, 1024×512):
   - **Prompt base:** "Wide banner illustration of a voxel fantasy region, cozy cartoon voxel style, no text: ___".
   - Troque `___` por:
     - **Valdora:** "green fields, small forests, a walled town";
     - **Brasafenda:** "volcano land, ash ground, lava rivers, dark rocks glowing orange";
     - **Raikor:** "stormy mountains, fir forests, snowy peaks, lightning in the clouds";
     - **Nivaluz:** "snow plains, ice spikes, snowy pines, aurora in the sky";
     - **Solcânion:** "red sand desert canyon, dunes, glass crystals, a hot sun";
     - **Aurivale:** "golden grass hills, apple and autumn trees, a calm lake";
     - **Lodonegro:** "dark swamp, mud pools, twisted trees, fireflies";
     - **Morthúria:** "purple cursed land, dead trees, ruins, ghostly purple light";
     - **Verdalume:** "elven forest, glowing green grass, white stone arches, moon wells".
3. **Tela de carregamento:** 3 ou 4 imagens de 1920×1080 no estilo do fundo do menu (vila de dia, floresta à noite, deserto, neve) para alternar com as dicas.
4. **Retratos dos chefes** para a barra de vida do alto (que agora só aparece em chefe): Escorpião de Vidro e Titã das Raízes (`T_Boss_<id>.png`, 512×512):
   - **Prompt:** "Round portrait of a voxel boss monster: a giant glass scorpion with crystal pincers / a giant tree titan made of roots and moss with glowing green eyes, dark navy background, golden ring frame".
5. **Cursor do mouse** do menu e da janela do herói: uma luva de voxel dourada ou uma pequena espada (`T_Cursor.png`, 64×64).

Os itens 1 a 5 também pedem uma pequena mudança no código para aparecerem. Quando as imagens estiverem prontas, é só colocar em `Data/UI/Menu/` e me avisar.
