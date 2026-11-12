# NPC Julia, o cachorrinho dela e a aparência dos NPCs

**Pedido:** "crie esse NPC já exatamente assim e seu cachorrinho que segue, NPC Julia" (com a imagem de referência em voxel).

## Julia

Moradora da praça da Cidade do Santuário. Passeia em volta da Fonte de Jade, com paradas.

**Visual da referência:**
- cabelo preto liso até a cintura, com franja reta e **pontas azuis**;
- **óculos redondos** de aro preto;
- **gargantilha** com pingente de **estrela** prateada;
- **vestido preto** (corpete com decote, mangas bufantes curtas e saia rodada em camadas);
- **meias 7/8** pretas e **botas** pretas;
- pele clara.

**Peças novas do Vortex** (`Tools/BuildJuliaPieces.py`):
- `julia_hair`, `julia_glasses`, `julia_choker`, `julia_dress` e `julia_stockings`;
- `gloves_none` (mãos sem luva).

Como foram feitas:
- São caixas de voxel (1/32 do modelo), desenhadas no espaço do aventureiro do Godot e presas aos ossos (cabeça, tronco, quadril, braços, coxas e canelas).
- Passam pelo mesmo conversor das peças oficiais (`ConvertGodotAdventurer.convert`) e são importadas por `Tools/ImportWardrobe.py` (`VORTEX_WARDROBE_ONLY=...`).
- As cores vêm de células fixas da paleta (preto, azul e prata).
- O catálogo extra (`Data/GodotCharacter/look_catalog_extra.json`) entra no catálogo do jogo. Por isso os óculos, a gargantilha e o resto também aparecem no **criador de heróis**.
- As botas são as `boots_shadow` pintadas de preto. O coturno tinha cores fixas e saía caramelo.

**Dados:** `Data/Population/npc_extra.json` (NPCs só do Vortex), juntado ao `npc_posts.json` por `Tools/ExtractNpcPosts.py`.

## O cachorrinho (`AVortexPet`)

- **Modelo:** um bichon branco e fofo de voxels de 2,5 cm, montado em código (com tufos de três tons). Tem corpo e peito, cabeça redonda, focinho, olhos e nariz pretos, a linguinha de fora, orelhas caídas, quatro patas e o rabo em pompom.
- **Comportamento:** segue a Julia do lado direito, um pouco atrás. Anda devagar quando está perto e corre quando ela se afasta. Parado, olha para ela.
- **Animação por código:**
  - trote com as patas em diagonal;
  - o corpo balança;
  - o rabo abana (mais rápido parado);
  - a cabeça inclina de vez em quando.
- **Rede:** só visual, cada máquina tem o seu, seguindo a Julia (que vai pela rede).
- **Ainda simples:** o modelo da carinha pode ficar mais detalhado. É a primeira versão.

## Aparência de todos os NPCs

Antes, os NPCs eram o aventureiro padrão. Agora:
- `Tools/ExtractNpcPosts.py` copia a aparência de cada NPC do `npcs.json` do Godot (peças e cores).
- `AVortexNpc` veste essas peças com a mesma paleta, tingimento e tamanho de raça do herói.
- O visual vai pela rede uma vez, quando o NPC é criado.
- Isso vale para Lary, Juh, Ilsa, Bram, Nicolly, Ducci, Rhea e os outros.

## Teste

`Tools/RunTownShot.ps1` também gera `cidade_julia_0.png` (de corpo inteiro), `cidade_julia_1.png` (de perto) e `cidade_julia_2.png` (o cachorrinho).
