# Cidade do Santuário (a vila inicial do Godot) e a Fonte de Jade

**Pedidos:**
- "já faça a cidade inicial também igual do Godot";
- "a fonte refaça inteira com a nossa água bonitinha etc., funcional".

## A cidade

Ela é a mesma do Godot (`ValleyWorld.Town.cs` + `Village.cs`) e fica no mesmo lugar do vale:

| Parte | O que tem |
|---|---|
| Muralha | Quadrado de 80 m (x de −40 a 40, z de −24 a 56) com torres nos cantos e no meio dos lados leste e oeste |
| Portões | Sul ("CIDADE DO SANTUÁRIO") e norte |
| Construções | Taverna do Javali Dourado, sobrados, lojas (Alfaiate, Padaria, Armaria), chalés e a torre do sino |
| Praça | Fonte no centro e, em volta, as três oficinas: Ervas de Ilsa, Forja de Bram e Mapas de Nicolly |
| Enfeites | Torre de vigia, carroça-bazar do Ducci, barracas, poços, quadro de missões, bancos, jardineiras, cercas e caixotes |
| Ruas | Lampiões (acendem ao entardecer e apagam de manhã) e pedras na praça e nos caminhos |
| Árvores | Cerejeiras, carvalhos, bétula e outono, com tronco que bloqueia |

**Como foi montada:**
- **Modelos:** os `.glb` de voxel do Godot (`Art/World/town_*`, `vil_*`, `house_*`), importados por `Tools/ImportGodotProps.py only=...` com o atlas de cores de cada um (Nanite).
- **Planta:** `Tools/BuildTownLayout.py` lê os `.json` do Godot (somente leitura), copia as listas de posições e grava `Data/GodotTown/town.json`:
  - 135 peças;
  - 27 modelos com as caixas de colisão, placas, lampiões e alicerces;
  - 11 árvores;
  - 2.058 pedras de 75 cm.
- **`AVortexTown`** (`World/VortexTown`), criado pelo streamer do terreno nos mapas de jogo:
  - monta tudo em lotes por modelo (34 lotes);
  - usa as 269 caixas de colisão do Godot;
  - põe as placas com os nomes e os 69 lampiões;
  - tem um alicerce de pedra onde o chão desce.
  - Não vai pela rede: cada máquina monta a sua, como o estádio.
  - `-VortexNoTown` desliga a cidade.
- **Conversão:** X = x do Godot, Y = z, giro = −giro (a mesma regra das plantas do terreno).
- **Grama:** não nasce nas pedras da praça e das ruas nem dentro das construções (`AVortexTown::BlocksGrass`, lido pelas tarefas do terreno).
- **NPCs:** Lary, Juh, Ilsa, Bram, Nicolly, Ducci e Rhea já tinham os postos na praça (`Data/Population/npc_posts.json`).

## A Fonte de Jade (refeita na Unreal)

O modelo de voxel da fonte foi trocado por `AVortexFountain` (`World/VortexFountain`):
- **Pedra:** bacia octogonal de travertino com borda de mármore e degrau de basalto, coluna de mármore com anéis de bronze, taça de cima com borda de 16 pedras e oito bicos de bronze. As texturas são as do estádio.
- **Cristal:** jade no topo, girando devagar, com luz verde que pulsa.
- **Água do Vortex nas duas bacias:** é a mesma do terreno e segue **Opções → Qualidade da água** (baixa, média, alta, épica). A fonte confere a cada segundo.
- **Movimento, como o `FountainWater.cs` do Godot:**
  - oito jatos de gotas em arco, que saem dos bicos e caem na bacia de baixo;
  - respingos onde as gotas batem;
  - borbulhar em volta da coluna.
  - São 150 gotinhas de voxel que brilham em azul-claro. Elas só se mexem com alguém a até 60 m.
- **Som:** água corrente, que some com a distância.
- **Funcional (é o santuário):**
  - **Cura:** perto da fonte (4,2 m), vida e mana voltam a 12% por segundo.
  - **Renascimento:** quem cai renasce na fonte depois de 5 s, com vida e mana cheias. Antes, o herói caído ficava caído para sempre. No Estádio do Despertar continua como antes.

## Correção que veio junto: materiais em lote

Os materiais de pedra e de brilho do estádio (`M_StadiumStone`, `M_StadiumGlow`) e as águas não tinham o uso "Instanced Static Meshes".
- No jogo, a Unreal trocava esses materiais pelo material padrão, quadriculado.
- Isso afetava as pedras do estádio e da cidade e as gotas da fonte.
- `Tools/FixInstancedMaterials.py` marca esses materiais. O caixote reimportado voltou ao `MI_vil_crates`.

## Teste

`Tools/RunTownShot.ps1` gera as fotos `Saved/Screenshots/cidade_*.png` e confere o santuário:
- portão, praça, taverna, oficinas, vista de cima, praça à noite, a fonte de dia e à noite;
- o herói cai longe e tem de renascer na fonte.

Resultado: `VORTEX_TOWN_SHOT_DONE PASS santuario vida=100 distancia_da_fonte=297`.
