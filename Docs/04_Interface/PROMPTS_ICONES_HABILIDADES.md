# Prompts dos ícones de habilidades e de classes (folhas de 8)

Gerado junto com `Docs/Roteiro_Proximas_Etapas.pdf`. Hoje as 264 habilidades usam só 161 ícones (muitas repetem o mesmo desenho, recortado das folhas do Godot). Aqui cada habilidade e cada classe ganha o seu, **8 por imagem**, para não gastar o gerador: 33 folhas de habilidades (2 classes por folha) e 9 de classes.

## Situação (27/10)

- **Todas as 42 folhas foram geradas** e estão em `Data/UI/SkillIcons/Folhas/`. Os 330 ícones cortados estão em `Data/UI/SkillIcons/Novos/`.
- **No jogo:** o `rpg.json` aponta cada habilidade para `skill_<id>` e cada classe para `class_<id>` (`python Tools/BuildRpgData.py --icons`). O `Tools/ImportItemIcons.py` importa os novos em `/Game/Vortex/UI/Icons`.
- **Para refazer um ícone:** gere a folha de novo, salve por cima e rode `python Tools/SliceIconSheet.py H07` (o nome da folha) e depois o `ImportItemIcons.py` com o editor fechado.

## Como usar

1. Abra uma conversa nova no ChatGPT e cole o **bloco de estilo** abaixo. Depois cole uma folha por vez, na mesma conversa.
2. Se a conversa ficar longa e o estilo começar a mudar, abra outra e cole o bloco de novo.
3. Salve cada imagem com o nome da folha (`H01.png` … `H33.png`, `C01.png` … `C09.png`) em `Data/UI/SkillIcons/Folhas/`.
4. Rode `python Tools/SliceIconSheet.py`. Ele corta cada folha nos 8 ícones (`Data/UI/SkillIcons/Novos/`) e faz uma folha de conferência com os nomes.
5. Quando tiver as folhas, me avise: falta apontar o `rpg.json` para os ícones novos e importar no editor.

## Bloco de estilo (colar primeiro)

```
I'm making the ability and class icons for a cozy fantasy voxel RPG called "The World Gêneses" (Minecraft-like blocky world; the UI is warm gold with dark navy panels and emerald accents). Every image in this conversation is a SHEET of 8 icons, and all of them must share one style:
- Layout: landscape 1536x1024 canvas, 4 columns x 2 rows, 8 square tiles of the same size, evenly spaced, separated by a flat dark navy (#101826) gap. No numbers, no labels, no text anywhere.
- Each tile: a full-bleed square icon with a soft radial background in the element color, NO frame or border (the game draws its own frame).
- Style: chunky voxel / blocky 3D look with subtle pixel texture, one bold central subject with a simple silhouette that still reads at 48 px, strong contrast, soft lighting, a small magic glow where it fits.
- Element colors: fire orange-red, holy gold-white, arcane violet, steel silver-blue, blood crimson, star deep blue with white stars, earth ochre, nature leaf green, shadow black-purple, wind pale cyan, time bronze-teal with clock motifs, jade jade-green, moon silver-blue, spirit turquoise, lightning electric yellow, bone ivory, water blue, poison toxic green.
Answer only with the image.
```

## H01 · Habilidades: Guerreiro e Cavaleiro

```
Sheet H01. Ability icons, left to right, top row (1-4) then bottom row (5-8). Tiles 1-4 are the class "Guerreiro", tiles 5-8 the class "Cavaleiro". Draw what each ability does (the description is in Portuguese):
1. Golpe Pesado (steel/silver) - Golpe frontal poderoso: dano alto e quebra a defesa do alvo (+20% de dano recebido por 6 s).
2. Investida de Combate (steel/silver) - Avança 7 m até o inimigo, derrubando quem estiver no caminho e desequilibrando o alvo (atordoa 0,8 s).
3. Grito de Guerra (fire/orange-red) - Grito que aumenta em 25% o dano seu e dos aliados próximos (8 m) por 8 s.
4. Último Fôlego (blood/crimson) - Último Fôlego: por 6 s recebe 45% menos dano e não cai abaixo de 1 de vida.
5. Golpe de Escudo (holy/gold-white) - Golpe de Escudo: dano e atordoamento de 1,4 s, empurrando o alvo.
6. Intervenção (holy/gold-white) - Intervenção: avança 8 m e ergue um escudo de 40 que protege você e os aliados próximos por 6 s.
7. Postura Defensiva (steel/silver) - Postura Defensiva: -35% de dano recebido por 8 s, mas anda mais devagar; provoca criaturas próximas.
8. Juramento de Proteção (holy/gold-white) - Juramento de Proteção: aura de 5 m que reduz em 25% o dano recebido pelo grupo por 8 s.
```

## H02 · Habilidades: Guardião e Patrulheiro

```
Sheet H02. Ability icons, left to right, top row (1-4) then bottom row (5-8). Tiles 1-4 are the class "Guardião", tiles 5-8 the class "Patrulheiro". Draw what each ability does (the description is in Portuguese):
1. Provocação (earth/ochre) - Provocação: todas as criaturas a até 8 m vão atrás de você por 6 s e causam 15% menos dano.
2. Muralha Viva (earth/ochre) - Muralha Viva: escudo de 70 e -20% de dano recebido por 6 s.
3. Impacto Protetor (earth/ochre) - Impacto Protetor: golpeia o chão: dano em volta (4 m) e lentidão de 50% por 4 s.
4. Ninguém Passa (earth/ochre) - Ninguém Passa: zona de 5 m por 8 s: aliados dentro recebem -30% de dano e inimigos atravessam a 35% da velocidade.
5. Flecha Perfurante (wind/pale cyan) - Flecha Perfurante: atravessa todos os inimigos alinhados (26 m).
6. Marca da Presa (nature/leaf green) - Marca da Presa: marca o alvo por 10 s; ele recebe +25% de dano de todo o grupo.
7. Salto de Retirada (wind/pale cyan) - Salto de Retirada: salta 6 m para trás disparando; quem estava perto fica 40% mais lento por 3 s.
8. Chuva de Flechas (nature/leaf green) - Chuva de Flechas: bombardeia uma área de 4,5 m na mira com 8 ondas de flechas.
```

## H03 · Habilidades: Duelista e Assassino

```
Sheet H03. Ability icons, left to right, top row (1-4) then bottom row (5-8). Tiles 1-4 are the class "Duelista", tiles 5-8 the class "Assassino". Draw what each ability does (the description is in Portuguese):
1. Estocada (steel/silver) - Estocada: avança 4 m com uma estocada precisa (+25% de chance de crítico).
2. Aparar (steel/silver) - Aparar: por 1,2 s, o próximo golpe que você receberia é anulado e devolvido.
3. Passo Lateral (wind/pale cyan) - Passo Lateral: esquiva curta para o lado e +30% de velocidade por 2 s.
4. Duelo Mortal (blood/crimson) - Duelo Mortal: desafia um inimigo por 12 s: ele só ataca você e recebe +35% de dano.
5. Punhalada Sombria (shadow/black-purple) - Punhalada Sombria: dano alto; pelas costas causa 2,5x.
6. Bomba de Fumaça (shadow/black-purple) - Bomba de Fumaça: nuvem de 4 m por 5 s; criaturas dentro perdem você de vista, erram mais e ficam lentas.
7. Marca do Executor (blood/crimson) - Marca do Executor: marca o alvo por 10 s (+15% de dano); quanto menos vida ele tiver, mais seus golpes ferem.
8. Execução (blood/crimson) - Execução: golpe brutal que causa até 2,5x de dano em alvos quase derrotados.
```

## H04 · Habilidades: Mago Elemental e Arcanista

```
Sheet H04. Ability icons, left to right, top row (1-4) then bottom row (5-8). Tiles 1-4 are the class "Mago Elemental", tiles 5-8 the class "Arcanista". Draw what each ability does (the description is in Portuguese):
1. Projétil Elemental (fire/orange-red) - Projétil Elemental: fogo (explode), gelo (deixa lento) ou raio (salta) — alterna a cada uso.
2. Explosão Elemental (fire/orange-red) - Explosão Elemental: onda de 4,5 m em volta do elemento atual.
3. Convergência (arcane/violet) - Convergência: +20% de dano e recargas 30% mais rápidas por 8 s.
4. Tempestade Elemental (fire/orange-red) - Tempestade Elemental: 9 impactos de fogo, gelo e raio numa área de 5 m na mira.
5. Projétil Arcano (arcane/violet) - Projétil Arcano: disparo estável; cada uso seguido em 4 s soma +15% de dano (até 5).
6. Runa de Contenção (arcane/violet) - Runa de Contenção: armadilha na mira (10 s); a primeira criatura que pisar fica presa por 3 s.
7. Amplificação Arcana (arcane/violet) - Amplificação Arcana: +30% de poder para você e aliados próximos por 10 s.
8. Campo Arcano (arcane/violet) - Campo Arcano: área de 5 m por 8 s: aliados recarregam 25% mais rápido e criaturas dentro causam 20% menos dano.
```

## H05 · Habilidades: Alquimista e Clérigo

```
Sheet H05. Ability icons, left to right, top row (1-4) then bottom row (5-8). Tiles 1-4 are the class "Alquimista", tiles 5-8 the class "Clérigo". Draw what each ability does (the description is in Portuguese):
1. Frasco Explosivo (fire/orange-red) - Frasco Explosivo: arremesso em arco que explode em 3 m.
2. Elixir Restaurador (jade/jade green) - Elixir Restaurador: cura 55 de vida sua e dos aliados próximos.
3. Composto Instável (poison/toxic green) - Composto Instável: veneno por 6 s, -15% de defesa e lentidão em 3 m.
4. Laboratório de Campo (jade/jade green) - Laboratório de Campo: estação por 12 s: cura aos poucos e +10% de dano para quem estiver nela.
5. Luz Restauradora (holy/gold-white) - Luz Restauradora: cura 60 de vida sua e do aliado mais ferido por perto.
6. Purificação (holy/gold-white) - Purificação: remove lentidões e enfraquecimentos e cura 20.
7. Símbolo Sagrado (holy/gold-white) - Símbolo Sagrado: área de 5 m por 10 s que regenera os aliados dentro.
8. Milagre (holy/gold-white) - Milagre: grande cura (120) no grupo e proteção contra a morte por 4 s.
```

## H06 · Habilidades: Druida e Xamã

```
Sheet H06. Ability icons, left to right, top row (1-4) then bottom row (5-8). Tiles 1-4 are the class "Druida", tiles 5-8 the class "Xamã". Draw what each ability does (the description is in Portuguese):
1. Raízes Aprisionadoras (nature/leaf green) - Raízes Aprisionadoras: prende as criaturas numa área de 3,5 m por 3 s.
2. Regeneração Natural (nature/leaf green) - Regeneração Natural: você e aliados próximos recuperam 4 de vida por segundo por 10 s.
3. Forma Feral (nature/leaf green) - Forma Feral: por 12 s, +30% de dano, +20% de velocidade e -15% de dano recebido.
4. Floresta Viva (nature/leaf green) - Floresta Viva: bosque de 6 m por 10 s que cura aliados e deixa inimigos 55% mais lentos.
5. Totem de Guerra (fire/orange-red) - Totem de Guerra: totem por 12 s: +20% de dano para aliados a até 6 m.
6. Totem de Cura (water/blue) - Totem de Cura: totem por 12 s que cura quem estiver a até 5 m.
7. Chamado Espiritual (spirit/turquoise) - Chamado Espiritual: um espírito luta ao seu lado por 14 s e cura um pouco a cada ataque.
8. Dança dos Ancestrais (spirit/turquoise) - Dança dos Ancestrais: por 10 s, totens e espíritos ficam 50% mais fortes e o grupo ganha dano, velocidade e cura.
```

## H07 · Habilidades: Feiticeiro e Lâmina Arcana

```
Sheet H07. Ability icons, left to right, top row (1-4) then bottom row (5-8). Tiles 1-4 are the class "Feiticeiro", tiles 5-8 the class "Lâmina Arcana". Draw what each ability does (the description is in Portuguese):
1. Rajada Mística (arcane/violet) - Rajada Mística: raio concentrado de 14 m com dano alto.
2. Familiar (shadow/black-purple) - Familiar: criatura mágica que ataca o inimigo mais próximo por 40 s.
3. Maldição (shadow/black-purple) - Maldição: o alvo causa 25% menos dano e recebe 20% mais por 8 s.
4. Grande Conjuração (star/deep blue) - Grande Conjuração: invoca uma entidade poderosa que ataca até 3 inimigos por 15 s.
5. Corte Arcano (arcane/violet) - Corte Arcano: o golpe lança uma lâmina de energia que avança 8 m.
6. Encantar Arma (arcane/violet) - Encantar Arma: +25% de dano e golpes com faíscas arcanas por 12 s.
7. Passo Etéreo (arcane/violet) - Passo Etéreo: atravessa os inimigos numa investida de 6 m sem ser atingido.
8. Lâmina Transcendente (star/deep blue) - Lâmina Transcendente: por 10 s, +35% de dano, +25% de velocidade e +40% de alcance da arma.
```

## H08 · Habilidades: Monge e Lanceiro

```
Sheet H08. Ability icons, left to right, top row (1-4) then bottom row (5-8). Tiles 1-4 are the class "Monge", tiles 5-8 the class "Lanceiro". Draw what each ability does (the description is in Portuguese):
1. Sequência de Punhos (wind/pale cyan) - Sequência de Punhos: quatro golpes rápidos à frente.
2. Palma Repulsora (wind/pale cyan) - Palma Repulsora: empurra os inimigos 4 m e interrompe o que estavam fazendo.
3. Fluxo de Chi (jade/jade green) - Fluxo de Chi: recupera 40 de mana e 30 de vida; aliados próximos recebem uma parte.
4. Mil Golpes (wind/pale cyan) - Mil Golpes: sequência extremamente rápida em volta (8 pulsos de 3 m).
5. Perfuração (steel/silver) - Perfuração: estocada longa (6 m) que atravessa e ignora parte da defesa.
6. Varredura (steel/silver) - Varredura: giro de 4 m que derruba os inimigos em volta.
7. Salto da Lança (wind/pale cyan) - Salto da Lança: salta 9 m e crava a lança ao aterrissar (dano em 3,2 m).
8. Formação de Lanças (steel/silver) - Formação de Lanças: por 12 s, você e aliados a até 6 m ganham +15% de dano e -15% de dano recebido.
```

## H09 · Habilidades: Bardo e Caçador de Monstros

```
Sheet H09. Ability icons, left to right, top row (1-4) then bottom row (5-8). Tiles 1-4 are the class "Bardo", tiles 5-8 the class "Caçador de Monstros". Draw what each ability does (the description is in Portuguese):
1. Canção de Guerra (fire/orange-red) - Canção de Guerra: por 10 s, +20% de dano para quem estiver a até 7 m.
2. Balada Restauradora (water/blue) - Balada Restauradora: por 10 s, cura contínua para quem estiver a até 7 m.
3. Nota Dissonante (arcane/violet) - Nota Dissonante: onda sonora que interrompe (1 s) e enfraquece em 20% por 5 s.
4. Grande Concerto (star/deep blue) - Grande Concerto: por 12 s, todo o grupo ganha dano, velocidade, proteção e cura.
5. Estudar Presa (nature/leaf green) - Estudar Presa: revela as fraquezas do alvo: +20% de dano do grupo nele por 15 s.
6. Bomba de Caça (fire/orange-red) - Bomba de Caça: explosivo que causa dano alto em 3,2 m e atordoa.
7. Esquiva Preparada (wind/pale cyan) - Esquiva Preparada: por 6 s, -30% de dano recebido e +25% de velocidade.
8. Abate Preparado (blood/crimson) - Abate Preparado: golpe forte; +60% em alvos estudados e mais dano quanto menos vida tiverem.
```

## H10 · Habilidades: Bárbaro da Bruma e Martelo Rúnico

```
Sheet H10. Ability icons, left to right, top row (1-4) then bottom row (5-8). Tiles 1-4 are the class "Bárbaro da Bruma", tiles 5-8 the class "Martelo Rúnico". Draw what each ability does (the description is in Portuguese):
1. Machado Brutal (blood/crimson) - Machado Brutal: golpe pesado em 3,2 m; acertar gera 2 de Fúria (até 5, some após 8 s sem lutar).
2. Bruma de Guerra (wind/pale cyan) - Bruma de Guerra: névoa de 4,5 m que acompanha você por 7 s; as criaturas dentro erram e causam 25% menos dano.
3. Rugido Tribal (fire/orange-red) - Rugido Tribal: +20% de dano e -15% de dano recebido para você e aliados a até 8 m por 10 s.
4. Frenesi da Bruma (blood/crimson) - Frenesi da Bruma: consome a Fúria (cada ponto soma +12% aos bônus) e livra você de lentidões: dano, velocidade e proteção por 8 s.
5. Martelo Sísmico (earth/ochre) - Martelo Sísmico: impacto frontal de 3,4 m que atordoa 1,2 s e empurra.
6. Runa de Peso (arcane/violet) - Runa de Peso: runa de 3,5 m na frente por 8 s; quem passa por ela anda a 40% da velocidade.
7. Runa Protetora (arcane/violet) - Runa Protetora: escudo de 55 por 8 s em você e nos aliados a até 8 m.
8. Julgamento Rúnico (earth/ochre) - Julgamento Rúnico: golpeia o chão (5 m) e detona de uma vez as suas runas por perto, que explodem com força.
```

## H11 · Habilidades: Guardião de Musgo e Andarilho do Vento

```
Sheet H11. Ability icons, left to right, top row (1-4) then bottom row (5-8). Tiles 1-4 are the class "Guardião de Musgo", tiles 5-8 the class "Andarilho do Vento". Draw what each ability does (the description is in Portuguese):
1. Pele de Casca (nature/leaf green) - Pele de Casca: -30% de dano recebido e +2 de vida por segundo por 10 s.
2. Espinhos Vivos (nature/leaf green) - Espinhos Vivos: por 8 s, devolve 45% do dano corpo a corpo a quem bateu em você.
3. Raízes Protetoras (nature/leaf green) - Raízes Protetoras: raízes envolvem você e os aliados a até 10 m: escudo de 45 por 8 s.
4. Coração da Floresta (nature/leaf green) - Coração da Floresta: quase imóvel por 10 s, vira um bosque de 6 m: cura forte, -35% de dano recebido para o grupo e inimigos lentos.
5. Corte do Vento (wind/pale cyan) - Corte do Vento: o golpe solta uma lâmina de ar muito rápida.
6. Passo Celeste (wind/pale cyan) - Passo Celeste: investida de 7 m em que nada encosta em você, cortando quem estiver no caminho.
7. Corrente Ascendente (wind/pale cyan) - Corrente Ascendente: redemoinho de 3,5 m que tira as criaturas do chão (1,4 s sem agir; chefes, menos).
8. Dança da Tempestade (wind/pale cyan) - Dança da Tempestade: salta entre até 5 inimigos cortando cada um e termina ao lado do último.
```

## H12 · Habilidades: Tecelã de Jade e Maga da Lua

```
Sheet H12. Ability icons, left to right, top row (1-4) then bottom row (5-8). Tiles 1-4 are the class "Tecelã de Jade", tiles 5-8 the class "Maga da Lua". Draw what each ability does (the description is in Portuguese):
1. Fragmento de Jade (jade/jade green) - Fragmento de Jade: marca o alvo (+10% de dano por 8 s) e o acerto cura 14 você e os aliados por perto.
2. Flor de Jade (jade/jade green) - Flor de Jade: cura 50 e dá um escudo de 25 (6 s) a você e aos aliados por perto.
3. Ligação Vital (jade/jade green) - Ligação Vital: conecta você aos aliados a até 10 m: cura 15 e todos regeneram 4 de vida por segundo por 10 s.
4. Palácio de Jade (jade/jade green) - Palácio de Jade: área cristalina de 6 m por 10 s que cura e reduz em 25% o dano recebido pelo grupo.
5. Raio Lunar (moon/silver-blue) - Raio Lunar: feixe de 14 m que aplica a Marca Lunar (+15% de dano recebido por 8 s).
6. Véu Noturno (moon/silver-blue) - Véu Noturno: por 4 s as criaturas perdem você de vista e o grupo próximo fica 20% mais rápido.
7. Gravidade Lunar (moon/silver-blue) - Gravidade Lunar: puxa as criaturas de 4,5 m para o centro e as deixa 50% mais lentas por 3 s.
8. Eclipse (moon/silver-blue) - Eclipse: escurece 7 m por 10 s: aliados dentro causam +20% de dano e criaturas dentro, 25% menos.
```

## H13 · Habilidades: Domador de Feras e Engenheiro Arcano

```
Sheet H13. Ability icons, left to right, top row (1-4) then bottom row (5-8). Tiles 1-4 are the class "Domador de Feras", tiles 5-8 the class "Engenheiro Arcano". Draw what each ability does (the description is in Portuguese):
1. Companheiro Feral (nature/leaf green) - Companheiro Feral: uma fera luta ao seu lado por 60 s, atacando o inimigo mais próximo.
2. Comando: Atacar (nature/leaf green) - Comando: Atacar: aponta um alvo; se uma fera sua estiver ativa, ela ataca junto (dano dobrado) e o derruba.
3. Elo de Sangue (blood/crimson) - Elo de Sangue: por 10 s, você ganha +20% de dano e velocidade e suas feras causam 40% a mais.
4. Chamado da Alcateia (nature/leaf green) - Chamado da Alcateia: uma alcateia ataca até 3 inimigos de uma vez por 15 s.
5. Torreta Arcana (arcane/violet) - Torreta Arcana: instala uma torre por 15 s que atira no inimigo mais próximo (10 m) a cada segundo.
6. Mina Rúnica (fire/orange-red) - Mina Rúnica: armadilha na frente (20 s) que explode forte quando uma criatura se aproxima.
7. Drone Reparador (arcane/violet) - Drone Reparador: por 10 s, escudo de 30 e +3 de vida por segundo para você e aliados próximos.
8. Sobrecarga Mecânica (lightning/yellow) - Sobrecarga Mecânica: por 10 s, torres, minas e invocações causam 60% a mais e você recarrega 20% mais rápido.
```

## H14 · Habilidades: Corsário e Pugilista

```
Sheet H14. Ability icons, left to right, top row (1-4) then bottom row (5-8). Tiles 1-4 are the class "Corsário", tiles 5-8 the class "Pugilista". Draw what each ability does (the description is in Portuguese):
1. Tiro de Pistola (fire/orange-red) - Tiro de Pistola: disparo rápido e reto, bom entre golpes de lâmina.
2. Corte Corsário (steel/silver) - Corte Corsário: corte de 2,8 m; +50% de dano se você estava em movimento ou acabou de investir.
3. Gancho (steel/silver) - Gancho: lança o gancho no inimigo mirado (até 12 m) e se puxa até ele, desequilibrando-o.
4. Bombardeio (fire/orange-red) - Bombardeio: marca uma área de 5 m na mira que recebe 10 impactos explosivos.
5. Jab Relâmpago (lightning/yellow) - Jab Relâmpago: soco muito rápido; cada jab seguido em 4 s soma +10% de dano (até 5).
6. Quebra-Defesa (steel/silver) - Quebra-Defesa: golpe que deixa o alvo recebendo +25% de dano por 6 s.
7. Esquiva Ofensiva (wind/pale cyan) - Esquiva Ofensiva: esquiva lateral; por 2 s, +25% de dano e +50% de chance de crítico.
8. Combo Implacável (lightning/yellow) - Combo Implacável: seis golpes seguidos; cada um que conecta bate 15% mais forte que o anterior.
```

## H15 · Habilidades: Caçador de Demônios e Místico do Véu

```
Sheet H15. Ability icons, left to right, top row (1-4) then bottom row (5-8). Tiles 1-4 are the class "Caçador de Demônios", tiles 5-8 the class "Místico do Véu". Draw what each ability does (the description is in Portuguese):
1. Marca Profana (holy/gold-white) - Marca Profana: marca o alvo por 15 s: +25% de dano de todo o grupo nele.
2. Corrente Consagrada (holy/gold-white) - Corrente Consagrada: prende o alvo no lugar por 3 s (chefes ficam só lentos).
3. Purga Demoníaca (holy/gold-white) - Purga Demoníaca: onda sagrada que interrompe e enfraquece em 30% por 6 s.
4. Sentença Infernal (holy/gold-white) - Sentença Infernal: raio devastador; +60% em alvos marcados e mais dano quanto menos vida tiverem.
5. Duplicata (arcane/violet) - Duplicata: deixa uma cópia ilusória onde estava e salta para o lado; as criaturas perdem você de vista por 4 s.
6. Véu (arcane/violet) - Véu: por 5 s você fica fora da vista das criaturas e o grupo próximo recebe 20% menos dano.
7. Pesadelo (shadow/black-purple) - Pesadelo: até 3 inimigos ficam paralisados de medo por 2 s e causam 30% menos dano por 6 s.
8. Mundo dos Sonhos (arcane/violet) - Mundo dos Sonhos: região de 6,5 m por 9 s em que as criaturas andam a 45% e causam 25% menos dano.
```

## H16 · Habilidades: Arauto de Guerra e Espadachim Carmesim

```
Sheet H16. Ability icons, left to right, top row (1-4) then bottom row (5-8). Tiles 1-4 are the class "Arauto de Guerra", tiles 5-8 the class "Espadachim Carmesim". Draw what each ability does (the description is in Portuguese):
1. Estandarte de Guerra (fire/orange-red) - Estandarte de Guerra: planta um estandarte por 14 s: +20% de dano para quem estiver a até 7 m.
2. Ordem de Avanço (wind/pale cyan) - Ordem de Avanço: +30% de velocidade no grupo próximo por 8 s e livra você de lentidões.
3. Ordem de Defesa (steel/silver) - Ordem de Defesa: -30% de dano recebido para o grupo próximo por 8 s.
4. Comando Supremo (star/deep blue) - Comando Supremo: +25% de dano no grupo próximo por 8 s e corta 30% da recarga que falta das suas outras habilidades.
5. Corte Rubro (blood/crimson) - Corte Rubro: corte de 2,8 m que faz sangrar (dano contínuo por 6 s).
6. Dança Carmesim (blood/crimson) - Dança Carmesim: por 8 s, +20% de velocidade, +10% de dano e recargas 25% mais rápidas.
7. Sangue por Sangue (blood/crimson) - Sangue por Sangue: sacrifica 15% da vida atual (nunca mata) por +40% de dano por 8 s.
8. Rosa Sangrenta (blood/crimson) - Rosa Sangrenta: explode em 5 m e detona os sangramentos: cada alvo sofre na hora 1,5x o que ainda iria sangrar.
```

## H17 · Habilidades: Destruidor e Senhor da Guerra

```
Sheet H17. Ability icons, left to right, top row (1-4) then bottom row (5-8). Tiles 1-4 are the class "Destruidor", tiles 5-8 the class "Senhor da Guerra". Draw what each ability does (the description is in Portuguese):
1. Demolição (earth/ochre) - Demolição: golpe brutal de 3,2 m que estilhaça a armadura (+30% de dano recebido por 6 s).
2. Onda de Choque (earth/ochre) - Onda de Choque: onda em linha de 8 m que derruba (1 s) e empurra as criaturas.
3. Ímpeto Destrutivo (fire/orange-red) - Ímpeto Destrutivo: por 10 s, cada golpe que você recebe soma +6% de dano (até +30%).
4. Impacto de Meteoro (fire/orange-red) - Impacto de Meteoro: salta 9 m e cai como um meteoro: explosão de 4,2 m que atordoa 1 s.
5. Golpe do Comandante (steel/silver) - Golpe do Comandante: corte de 3,2 m que marca os atingidos (+15% de dano do grupo por 8 s).
6. Reagrupar (steel/silver) - Reagrupar: chama o grupo à formação: escudo de 40 e -20% de dano recebido por 8 s para quem estiver a até 10 m.
7. Presença Dominante (blood/crimson) - Presença Dominante: as criaturas a até 7 m vão atrás de você por 6 s e causam 25% menos dano.
8. Campo de Batalha (fire/orange-red) - Campo de Batalha: declara 9 m como território aliado por 12 s: +20% de dano e -15% de dano recebido dentro.
```

## H18 · Habilidades: Colosso e Imortal

```
Sheet H18. Ability icons, left to right, top row (1-4) then bottom row (5-8). Tiles 1-4 are the class "Colosso", tiles 5-8 the class "Imortal". Draw what each ability does (the description is in Portuguese):
1. Pisoteio (earth/ochre) - Pisoteio: derruba as criaturas a até 4,5 m (1 s) e as faz vir atrás de você por 6 s.
2. Corpo de Pedra (earth/ochre) - Corpo de Pedra: -55% de dano recebido por 5 s, andando mais devagar.
3. Interceptar (steel/silver) - Interceptar: avança 8 m, atrai as criaturas do caminho e cobre você e os aliados próximos com um escudo de 50.
4. Forma Colossal (earth/ochre) - Forma Colossal: por 12 s, -30% de dano recebido, +15% de dano, regeneração e +40% de alcance; provoca tudo a 6 m.
5. Sangue Persistente (blood/crimson) - Sangue Persistente: recupera 55 de vida na hora e mais 3 por segundo por 6 s.
6. Recusar a Morte (blood/crimson) - Recusar a Morte: por 8 s, nenhum golpe derruba você (fica com 1 de vida no mínimo).
7. Marca da Eternidade (holy/gold-white) - Marca da Eternidade: marca o alvo por 12 s; cada golpe seu nele cura você em 20% do dano.
8. Renascimento (holy/gold-white) - Renascimento: por 30 s, se cair, volta na hora com 40% da vida (uma vez).
```

## H19 · Habilidades: Fantasma e Cronomante

```
Sheet H19. Ability icons, left to right, top row (1-4) then bottom row (5-8). Tiles 1-4 are the class "Fantasma", tiles 5-8 the class "Cronomante". Draw what each ability does (the description is in Portuguese):
1. Passagem Espectral (spirit/turquoise) - Passagem Espectral: atravessa 7 m sem ser tocado; quem você atravessa fica com a defesa aberta (+12% por 4 s).
2. Lâmina Fantasma (spirit/turquoise) - Lâmina Fantasma: corte que passa pela defesa; pelas costas causa o dobro.
3. Assombrar (spirit/turquoise) - Assombrar: o alvo fica 45% mais lento e recebe +15% de dano por 8 s — não há como fugir.
4. Mil Lâminas (spirit/turquoise) - Mil Lâminas: sete ondas de cortes espectrais em volta de você (3,2 m).
5. Atraso Temporal (time/bronze-teal) - Atraso Temporal: até 2 inimigos andam a 40% e agem mais devagar (-20% de dano) por 6 s.
6. Aceleração (time/bronze-teal) - Aceleração: por 8 s, +30% de velocidade no grupo próximo e suas recargas 30% mais rápidas.
7. Eco Temporal (time/bronze-teal) - Eco Temporal: repete a última habilidade que você lançou, com 60% do poder.
8. Parar o Tempo (time/bronze-teal) - Parar o Tempo: congela as criaturas numa região de 6 m por 4 s (chefes ficam muito lentos).
```

## H20 · Habilidades: Oráculo e Avatar Elemental

```
Sheet H20. Ability icons, left to right, top row (1-4) then bottom row (5-8). Tiles 1-4 are the class "Oráculo", tiles 5-8 the class "Avatar Elemental". Draw what each ability does (the description is in Portuguese):
1. Presságio (star/deep blue) - Presságio: prevê os golpes: -20% de dano recebido e +4% de crítico no grupo próximo por 8 s.
2. Destino Favorável (star/deep blue) - Destino Favorável: por 8 s, +25% de crítico para você e +10% de dano e velocidade no grupo.
3. Destino Sombrio (shadow/black-purple) - Destino Sombrio: amaldiçoa o alvo: +30% de dano de todo o grupo nele por 10 s.
4. Reescrever Destino (time/bronze-teal) - Reescrever Destino: grava sua vida agora; daqui a 4 s, se tiver caído, ela volta ao valor gravado. Cura 20 o grupo.
5. Forma Elemental (fire/orange-red) - Forma Elemental: troca o elemento (fogo > gelo > raio > terra); fogo queima, gelo deixa lento, raio salta e terra faz tropeçar.
6. Explosão Primordial (fire/orange-red) - Explosão Primordial: explosão de 4,5 m com o efeito da forma atual.
7. Absorção Elemental (arcane/violet) - Absorção Elemental: absorve energia: recupera 35 de mana e recebe 20% menos dano por 6 s.
8. Manifestação Elemental (fire/orange-red) - Manifestação Elemental: por 12 s, forma elemental completa: +30% de dano, velocidade, proteção e golpes com o elemento atual.
```

## H21 · Habilidades: Cavaleiro Abissal e Sangromante

```
Sheet H21. Ability icons, left to right, top row (1-4) then bottom row (5-8). Tiles 1-4 are the class "Cavaleiro Abissal", tiles 5-8 the class "Sangromante". Draw what each ability does (the description is in Portuguese):
1. Corte Abissal (shadow/black-purple) - Corte Abissal: golpe de aço e abismo em 3 m que devolve 15% do dano como vida.
2. Armadura do Vazio (shadow/black-purple) - Armadura do Vazio: escudo de 50 por 8 s que converte energia: recupera 20 de mana.
3. Correntes do Abismo (shadow/black-purple) - Correntes do Abismo: prende até 3 inimigos por 2 s e os faz vir atrás de você por 6 s.
4. Portão Abissal (shadow/black-purple) - Portão Abissal: fenda de 4,5 m por 10 s que fere quem está dentro e dá +20% de dano a você enquanto estiver nela.
5. Lança de Sangue (blood/crimson) - Lança de Sangue: sacrifica 6% da vida atual para disparar uma lança que atravessa tudo.
6. Drenagem (blood/crimson) - Drenagem: feixe de 12 m que drena 5 vezes; metade do dano volta como vida.
7. Laço Carmesim (blood/crimson) - Laço Carmesim: liga até 4 inimigos por 8 s: todos recebem +20% de dano.
8. Mar de Sangue (blood/crimson) - Mar de Sangue: área de 6,5 m por 10 s que drena as criaturas e cura o grupo dentro dela.
```

## H22 · Habilidades: Invocador Astral e Mestre das Correntes

```
Sheet H22. Ability icons, left to right, top row (1-4) then bottom row (5-8). Tiles 1-4 are the class "Invocador Astral", tiles 5-8 the class "Mestre das Correntes". Draw what each ability does (the description is in Portuguese):
1. Fragmento Estelar (star/deep blue) - Fragmento Estelar: um espírito astral luta ao seu lado por 30 s.
2. Constelação Protetora (star/deep blue) - Constelação Protetora: barreira estelar de 50 por 8 s em você e nos aliados a até 10 m.
3. Cometa (star/deep blue) - Cometa: marca uma área de 4 m; depois de 1,2 s um cometa cai e atordoa 1 s.
4. Portal Zodiacal (star/deep blue) - Portal Zodiacal: uma criatura astral poderosa ataca até 3 inimigos por 15 s.
5. Corrente Puxadora (steel/silver) - Corrente Puxadora: puxa o inimigo até você (chefes não se movem) e ele causa 6% menos dano por 5 s.
6. Grilhões (steel/silver) - Grilhões: o alvo anda a 20% da velocidade por 5 s.
7. Elo Protetor (steel/silver) - Elo Protetor: liga você aos aliados próximos: escudo de 40 e -15% de dano recebido por 8 s.
8. Prisão de Correntes (steel/silver) - Prisão de Correntes: arena de 6 m por 8 s que arrasta as criaturas para o centro e quase as impede de andar.
```

## H23 · Habilidades: Arquimago e Cavaleiro do Dragão

```
Sheet H23. Ability icons, left to right, top row (1-4) then bottom row (5-8). Tiles 1-4 are the class "Arquimago", tiles 5-8 the class "Cavaleiro do Dragão". Draw what each ability does (the description is in Portuguese):
1. Magia Superior (fire/orange-red) - Magia Superior: projétil que alterna fogo (explode e queima), gelo (deixa lento) e raio (salta para mais 2).
2. Contrafeitiço (arcane/violet) - Contrafeitiço: pulso de 6 m que interrompe as criaturas (1 s), as enfraquece e devolve 25 de mana.
3. Dobra Arcana (arcane/violet) - Dobra Arcana: teleporta 9 m à frente sem ser tocado no caminho.
4. Grande Ritual (arcane/violet) - Grande Ritual: canaliza por 2,2 s (quase parado) e explode uma área de 7 m com dano enorme; proteja o mago.
5. Investida Dracônica (fire/orange-red) - Investida Dracônica: atravessa 10 m da linha inimiga com a lança, derrubando todos no caminho.
6. Escamas Dracônicas (fire/orange-red) - Escamas Dracônicas: -50% de dano recebido por 5 s; quem bate de perto se queima (20% de volta).
7. Sopro Herdado (fire/orange-red) - Sopro Herdado: cone de fogo dracônico de 5,5 m que deixa as criaturas queimando por 5 s.
8. Despertar do Dragão (fire/orange-red) - Despertar do Dragão: asas e poder dracônico por 12 s: +25% de dano, +30% de velocidade, -20% de dano recebido e +30% de alcance.
```

## H24 · Habilidades: Soberano Rúnico e Caminhante do Vazio

```
Sheet H24. Ability icons, left to right, top row (1-4) then bottom row (5-8). Tiles 1-4 are the class "Soberano Rúnico", tiles 5-8 the class "Caminhante do Vazio". Draw what each ability does (the description is in Portuguese):
1. Runa de Poder (fire/orange-red) - Runa de Poder: runa de 5 m por 12 s: +25% de dano para quem estiver nela.
2. Runa de Proteção (arcane/violet) - Runa de Proteção: runa de 5 m por 12 s: -30% de dano recebido e cura leve dentro dela.
3. Runa de Prisão (earth/ochre) - Runa de Prisão: runa de 3,2 m por 10 s que prende no lugar quem estiver nela.
4. Palavra Primordial (star/deep blue) - Palavra Primordial: ativa na hora todas as suas runas por perto, que explodem com potência ampliada.
5. Passo no Vazio (shadow/black-purple) - Passo no Vazio: teleporta 8 m atravessando tudo; por 2 s, +25% de crítico.
6. Corte Dimensional (shadow/black-purple) - Corte Dimensional: corta através da armadura (+30% de dano recebido por 4 s); pelas costas, +50%.
7. Portal Vinculado (arcane/violet) - Portal Vinculado: abre um portal e sai 10 m à frente; você e o grupo próximo ficam 30% mais rápidos por 6 s.
8. Colapso Espacial (shadow/black-purple) - Colapso Espacial: comprime uma área de 5 m, arrastando as criaturas para o centro, e a faz implodir.
```

## H25 · Habilidades: Santo Guerreiro e Senhor das Tempestades

```
Sheet H25. Ability icons, left to right, top row (1-4) then bottom row (5-8). Tiles 1-4 are the class "Santo Guerreiro", tiles 5-8 the class "Senhor das Tempestades". Draw what each ability does (the description is in Portuguese):
1. Golpe Radiante (holy/gold-white) - Golpe Radiante: golpe de 3 m que, ao acertar, cura 20 você e os aliados por perto.
2. Escudo da Fé (holy/gold-white) - Escudo da Fé: barreira poderosa de 80 por 8 s em você e nos aliados a até 10 m.
3. Martírio (holy/gold-white) - Martírio: por 8 s, atrai para si os golpes: -35% de dano recebido e escudo de 30 no grupo próximo.
4. Intervenção Celestial (holy/gold-white) - Intervenção Celestial: cura 80, escudo de 40 e purificação no grupo; ninguém cai pelos próximos 3 s (você).
5. Relâmpago Encadeado (lightning/yellow) - Relâmpago Encadeado: raio que salta entre até 6 inimigos.
6. Rajada de Vento (wind/pale cyan) - Rajada de Vento: onda de 10 m que empurra as criaturas 4 m e desfaz a formação.
7. Nuvem de Tempestade (lightning/yellow) - Nuvem de Tempestade: nuvem de 4 m por 8 s na mira que dispara raios em quem estiver embaixo.
8. Ira dos Céus (lightning/yellow) - Ira dos Céus: tempestade enorme: 14 raios numa região de 7 m na mira.
```

## H26 · Habilidades: Arconte Solar e Devorador de Magia

```
Sheet H26. Ability icons, left to right, top row (1-4) then bottom row (5-8). Tiles 1-4 are the class "Arconte Solar", tiles 5-8 the class "Devorador de Magia". Draw what each ability does (the description is in Portuguese):
1. Corte Solar (holy/gold-white) - Corte Solar: corte luminoso de 3 m que deixa queimadura solar por 5 s.
2. Clarão (holy/gold-white) - Clarão: luz cegante a 6 m: criaturas ficam 1,2 s sem agir e erram mais (-30% de dano) por 5 s.
3. Aura Solar (holy/gold-white) - Aura Solar: por 12 s, aliados a até 7 m ganham +15% de dano e criaturas nela causam 15% menos.
4. Sol Descendente (holy/gold-white) - Sol Descendente: concentra energia por 1,5 s e explode uma região de 7,5 m, deixando-a em chamas.
5. Drenar Mana (arcane/violet) - Drenar Mana: suga a energia do alvo (-15% de dano por 6 s) e recupera 25 de mana.
6. Silenciamento (arcane/violet) - Silenciamento: o alvo fica 2 s sem agir e enfraquecido (-30%) por 4 s.
7. Absorção Arcana (arcane/violet) - Absorção Arcana: escudo de 50 por 6 s e +30% de dano enquanto durar.
8. Zona Nula (arcane/violet) - Zona Nula: região de 6 m por 10 s: criaturas dentro causam 40% menos dano e aliados recebem 20% menos.
```

## H27 · Habilidades: Necromante e Herdeiro do Colosso

```
Sheet H27. Ability icons, left to right, top row (1-4) then bottom row (5-8). Tiles 1-4 are the class "Necromante", tiles 5-8 the class "Herdeiro do Colosso". Draw what each ability does (the description is in Portuguese):
1. Erguer Morto (bone/ivory) - Erguer Morto: levanta um servo morto-vivo por 30 s; cada Essência guardada o deixa 15% mais forte (e é gasta).
2. Explosão Cadavérica (bone/ivory) - Explosão Cadavérica: detona os restos na mira: explosão de ossos de 4 m.
3. Colheita de Almas (spirit/turquoise) - Colheita de Almas: arranca almas a até 6 m: cada criatura atingida gera 1 Essência (até 5) e você se cura.
4. Exército dos Mortos (bone/ivory) - Exército dos Mortos: levanta mortos que atacam até 4 inimigos por 20 s; consome a Essência (+20% de força por ponto).
5. Punho Titânico (earth/ochre) - Punho Titânico: impacto massivo de 3,2 m que atordoa, empurra e atrai as criaturas para você.
6. Sangue de Gigante (earth/ochre) - Sangue de Gigante: cura 70 e, por 10 s, -30% de dano recebido e +3 de vida por segundo.
7. Passo do Titã (earth/ochre) - Passo do Titã: tremor de 5,5 m que derruba (1,4 s) e deixa as criaturas lentas.
8. Despertar Ancestral (earth/ochre) - Despertar Ancestral: forma colossal por 15 s: -40% de dano recebido, +25% de dano, regeneração e +50% de alcance.
```

## H28 · Habilidades: Senhor da Matilha e Cartógrafo do Vale

```
Sheet H28. Ability icons, left to right, top row (1-4) then bottom row (5-8). Tiles 1-4 are the class "Senhor da Matilha", tiles 5-8 the class "Cartógrafo do Vale". Draw what each ability does (the description is in Portuguese):
1. Chamado da Matilha (nature/leaf green) - Chamado da Matilha: dois lobos lutam ao seu lado por 40 s.
2. Marca do Alfa (blood/crimson) - Marca do Alfa: marca o alvo (+20% de dano por 10 s) e a matilha avança nele (dano +80% se houver lobos).
3. Uivo de Caça (nature/leaf green) - Uivo de Caça: +20% de dano e velocidade no grupo próximo e lobos 40% mais fortes por 10 s.
4. Grande Caçada (blood/crimson) - Grande Caçada: a matilha inteira ataca até 4 alvos; +50% nos marcados e muito mais forte com lobos ativos.
5. Olho do Cartógrafo (nature/leaf green) - Olho do Cartógrafo: revela e marca todas as criaturas a até 25 m (+10% de dano nelas por 15 s).
6. Rota Segura (wind/pale cyan) - Rota Segura: +35% de velocidade para o grupo próximo por 15 s e livra você de lentidões.
7. Marcar Terreno (nature/leaf green) - Marcar Terreno: estuda 8 m de terreno por 15 s: quem luta dentro ganha +15% de dano e -15% de dano recebido.
8. Mapa Impossível (star/deep blue) - Mapa Impossível: revela tudo a 35 m: criaturas marcadas (+20% por 20 s) e mais lentas; o grupo ganha velocidade.
```

## H29 · Habilidades: Mestre Forjador e Guardião do Santuário

```
Sheet H29. Ability icons, left to right, top row (1-4) then bottom row (5-8). Tiles 1-4 are the class "Mestre Forjador", tiles 5-8 the class "Guardião do Santuário". Draw what each ability does (the description is in Portuguese):
1. Afiar (steel/silver) - Afiar: +25% de dano para você e aliados próximos por 12 s; seus golpes cortam com faíscas.
2. Reforçar Armadura (earth/ochre) - Reforçar Armadura: -30% de dano recebido para você e aliados próximos por 12 s.
3. Forja de Campo (fire/orange-red) - Forja de Campo: monta uma forja por 15 s: quem fica perto se recompõe (cura) e ganha +10% de dano e proteção.
4. Obra-Prima (star/deep blue) - Obra-Prima: por 12 s, equipamento lendário: +30% de dano, +20% de velocidade e proteção no grupo e +20% de crítico.
5. Selo do Santuário (holy/gold-white) - Selo do Santuário: área de 6 m por 10 s em que o grupo recebe 30% menos dano.
6. Proteção Absoluta (holy/gold-white) - Proteção Absoluta: escudo excepcional de 120 por 8 s em você e nos aliados a até 10 m.
7. Julgamento do Invasor (holy/gold-white) - Julgamento do Invasor: por 10 s, quem bate em você recebe 60% de volta; o grupo próximo recebe 10% menos dano.
8. Santuário Vivo (holy/gold-white) - Santuário Vivo: grande zona de 8 m por 12 s: cura forte, -30% de dano recebido e purificação ao abrir.
```

## H30 · Habilidades: Portador da Lua e Rei sem Coroa

```
Sheet H30. Ability icons, left to right, top row (1-4) then bottom row (5-8). Tiles 1-4 are the class "Portador da Lua", tiles 5-8 the class "Rei sem Coroa". Draw what each ability does (the description is in Portuguese):
1. Crescente Lunar (moon/silver-blue) - Crescente Lunar: o golpe solta um corte em meia-lua que voa 12 m.
2. Bênção da Lua (moon/silver-blue) - Bênção da Lua: dano, proteção e regeneração por 12 s; à noite, a bênção fica 60% mais forte.
3. Passo Lunar (moon/silver-blue) - Passo Lunar: teleporte de 8 m guiado pela lua, atravessando os inimigos.
4. Lua Manifestada (moon/silver-blue) - Lua Manifestada: lua artificial sobre 8 m por 12 s: aliados +25% de dano, criaturas mais fracas e suas invocações mais fortes.
5. Comando Real (star/deep blue) - Comando Real: +30% de dano para você e o grupo próximo por 10 s.
6. Autoridade (steel/silver) - Autoridade: as criaturas a até 8 m perdem a coragem: -30% de dano e mais lentas por 8 s.
7. Reunir Seguidores (fire/orange-red) - Reunir Seguidores: o grupo a até 12 m ganha dano, proteção e velocidade por 12 s; invocações aliadas 30% mais fortes.
8. Meu Reino (star/deep blue) - Meu Reino: declara 10 m como domínio por 15 s: +20% de dano, -20% de dano recebido e cura leve para o grupo.
```

## H31 · Habilidades: Serafim da Aurora e Arauto do Fim

```
Sheet H31. Ability icons, left to right, top row (1-4) then bottom row (5-8). Tiles 1-4 are the class "Serafim da Aurora", tiles 5-8 the class "Arauto do Fim". Draw what each ability does (the description is in Portuguese):
1. Luz da Aurora (holy/gold-white) - Luz da Aurora: onda de luz de 12 m que fere as criaturas e, ao acertar, cura 30 você e o grupo próximo.
2. Asas Celestiais (holy/gold-white) - Asas Celestiais: voo curto de 12 m por cima de tudo, pousando com luz (3 m).
3. Graça Divina (holy/gold-white) - Graça Divina: escudo de 60 no grupo próximo; por 6 s você não pode ser abatido.
4. Primeiro Amanhecer (holy/gold-white) - Primeiro Amanhecer: onda enorme de 9 m que cura 90 e purifica o grupo e fere e atordoa as criaturas.
5. Primeiro Selo (shadow/black-purple) - Primeiro Selo: marca acumulável: cada selo soma +12% de dano recebido (até +60%) por 12 s.
6. Trombeta (shadow/black-purple) - Trombeta: medo a até 8 m: criaturas paralisadas por 2 s, enfraquecidas e mais vulneráveis.
7. Presságio do Fim (shadow/black-purple) - Presságio do Fim: até 5 alvos causam 30% menos dano e definham (dano contínuo) por 10 s.
8. Apocalipse (shadow/black-purple) - Apocalipse: explosão de 7 m que consome as marcas: quanto mais marcado o alvo, mais dano (até 3,4x).
```

## H32 · Habilidades: Vaso do Éter e Soberano Primordial

```
Sheet H32. Ability icons, left to right, top row (1-4) then bottom row (5-8). Tiles 1-4 are the class "Vaso do Éter", tiles 5-8 the class "Soberano Primordial". Draw what each ability does (the description is in Portuguese):
1. Éter Condensado (star/deep blue) - Éter Condensado: disparo de energia pura que explode em 1,4 m.
2. Corpo Astral (star/deep blue) - Corpo Astral: por 5 s, recebe 40% menos dano; nos primeiros 2 s as criaturas perdem você de vista.
3. Transferência Etérea (arcane/violet) - Transferência Etérea: redistribui energia: o grupo próximo cura 30 e você recupera 30 de mana.
4. Transcendência (star/deep blue) - Transcendência: forma astral por 12 s: +40% de dano, +20% de velocidade e proteção e recargas 30% mais rápidas.
5. Criar (earth/ochre) - Criar: ergue uma barreira primordial de 4 m por 10 s: quem fica atrás dela recebe 35% menos dano.
6. Destruir (fire/orange-red) - Destruir: rompe uma área de 5 m na mira com dano pesado e quebra a defesa (+30% por 6 s).
7. Equilibrar (jade/jade green) - Equilibrar: redistribui forças: cura 40 o grupo próximo, você recupera 25 de mana e se purifica.
8. Lei Primordial (star/deep blue) - Lei Primordial: impõe uma regra a 12 m por 12 s: o grupo +20% de dano e proteção; criaturas mais fracas e lentas.
```

## H33 · Habilidades: Herdeiro do Caos e Avatar do Destino

```
Sheet H33. Ability icons, left to right, top row (1-4) then bottom row (5-8). Tiles 1-4 are the class "Herdeiro do Caos", tiles 5-8 the class "Avatar do Destino". Draw what each ability does (the description is in Portuguese):
1. Fragmento Caótico (arcane/violet) - Fragmento Caótico: projétil com um elemento sorteado (fogo queima, gelo deixa lento, raio salta, terra faz tropeçar...).
2. Ruptura (arcane/violet) - Ruptura: distorce 5 m por 6 s: criaturas dentro andam mal e sofrem dano irregular de um elemento sorteado.
3. Entropia Crescente (arcane/violet) - Entropia Crescente: +25% de dano e recargas 25% mais rápidas por 8 s.
4. Caos Encarnado (shadow/black-purple) - Caos Encarnado: por 12 s, +35% de dano e velocidade; cada golpe sai com um elemento diferente.
5. Fio Favorável (time/bronze-teal) - Fio Favorável: por 10 s, o grupo próximo ganha dano, proteção e regeneração; você, +10% de crítico.
6. Fio Cortado (time/bronze-teal) - Fio Cortado: o alvo causa 35% menos dano e recebe 15% mais por 8 s.
7. Destino Vinculado (time/bronze-teal) - Destino Vinculado: une o grupo próximo: cura 30 e escudo de 40 por 8 s em todos.
8. Causalidade (time/bronze-teal) - Causalidade: grava sua vida agora; daqui a 4 s, se tiver caído, ela volta. Cura 25 o grupo.
```

## C01 · Classes: Guerreiro, Cavaleiro, Guardião, Patrulheiro, Duelista, Assassino, Mago Elemental, Arcanista

```
Sheet C01. CLASS emblems (not abilities): each tile is the emblem of a hero class, a heraldic object that sums up the class, with the background tint by rarity. 8 tiles:
1. Guerreiro - DPS corpo a corpo / bruiser; weapons: espada de cinzas, espadao, machado; background tint: gray
2. Cavaleiro - Tank / suporte; weapons: espada de cinzas, lanca do patrulheiro, maca sagrada; background tint: gray
3. Guardião - Tank puro; weapons: martelo de bram, maca sagrada, lanca do patrulheiro; background tint: gray
4. Patrulheiro - DPS à distância / exploração; weapons: arco de betula, besta, lanca do patrulheiro; background tint: gray
5. Duelista - DPS técnico; weapons: espada de cinzas, adagas gemeas; background tint: gray
6. Assassino - Burst / eliminação; weapons: adagas gemeas, espada de cinzas, chakrams; background tint: gray
7. Mago Elemental - DPS mágico; weapons: cajado de ilsa, varinha, livro da aurora; background tint: gray
8. Arcanista - Controle / magia utilitária; weapons: cajado de ilsa, varinha, livro da aurora; background tint: gray
```

## C02 · Classes: Alquimista, Clérigo, Druida, Xamã, Feiticeiro, Lâmina Arcana, Monge, Lanceiro

```
Sheet C02. CLASS emblems (not abilities): each tile is the emblem of a hero class, a heraldic object that sums up the class, with the background tint by rarity. 8 tiles:
1. Alquimista - Suporte / debuff; weapons: catalisador; background tint: gray
2. Clérigo - Healer; weapons: maca sagrada, cajado de ilsa, livro da aurora; background tint: gray
3. Druida - Suporte / híbrido; weapons: cajado de ilsa, orbe, foice; background tint: gray
4. Xamã - Suporte / buffs; weapons: cajado de ilsa, orbe, instrumento; background tint: gray
5. Feiticeiro - Caster / invocação; weapons: cajado de ilsa, varinha, livro da aurora; background tint: gray
6. Lâmina Arcana - Melee mágico; weapons: espada de cinzas, espadao, lanca do patrulheiro; background tint: gray
7. Monge - DPS / suporte; weapons: manoplas de ferro; background tint: gray
8. Lanceiro - Melee de alcance médio; weapons: lanca do patrulheiro, alabarda; background tint: gray
```

## C03 · Classes: Bardo, Caçador de Monstros, Bárbaro da Bruma, Martelo Rúnico, Guardião de Musgo, Andarilho do Vento, Tecelã de Jade, Maga da Lua

```
Sheet C03. CLASS emblems (not abilities): each tile is the emblem of a hero class, a heraldic object that sums up the class, with the background tint by rarity. 8 tiles:
1. Bardo - Suporte; weapons: instrumento; background tint: gray
2. Caçador de Monstros - DPS especializado; weapons: espada de cinzas, espadao, machado; background tint: gray
3. Bárbaro da Bruma - Bruiser; weapons: espadao, machado, machados duplos; background tint: blue
4. Martelo Rúnico - Tank / controle; weapons: martelo de bram; background tint: blue
5. Guardião de Musgo - Tank / regeneração; weapons: martelo de bram, cajado de ilsa, foice; background tint: blue
6. Andarilho do Vento - Mobilidade / DPS; weapons: espada de cinzas, adagas gemeas, chakrams; background tint: blue
7. Tecelã de Jade - Healer / suporte; weapons: orbe, cajado de ilsa, livro da aurora; background tint: blue
8. Maga da Lua - Controle / suporte mágico; weapons: orbe, cajado de ilsa, varinha; background tint: blue
```

## C04 · Classes: Domador de Feras, Engenheiro Arcano, Corsário, Pugilista, Caçador de Demônios, Místico do Véu, Arauto de Guerra, Espadachim Carmesim

```
Sheet C04. CLASS emblems (not abilities): each tile is the emblem of a hero class, a heraldic object that sums up the class, with the background tint by rarity. 8 tiles:
1. Domador de Feras - Pet DPS; weapons: arco de betula, lanca do patrulheiro, chicote; background tint: blue
2. Engenheiro Arcano - Utilidade / DPS; weapons: pistola runica, besta, catalisador; background tint: blue
3. Corsário - DPS / mobilidade; weapons: pistola runica, espada de cinzas, adagas gemeas; background tint: blue
4. Pugilista - Combo DPS; weapons: manoplas de ferro; background tint: blue
5. Caçador de Demônios - DPS anti-magia; weapons: besta, espada de cinzas, adagas gemeas; background tint: blue
6. Místico do Véu - Controle / ilusão; weapons: livro da aurora, orbe, varinha; background tint: blue
7. Arauto de Guerra - Suporte ofensivo; weapons: lanca do patrulheiro, alabarda; background tint: blue
8. Espadachim Carmesim - DPS / risco e recompensa; weapons: espada de cinzas, espadao, adagas gemeas; background tint: blue
```

## C05 · Classes: Destruidor, Senhor da Guerra, Colosso, Imortal, Fantasma, Cronomante, Oráculo, Avatar Elemental

```
Sheet C05. CLASS emblems (not abilities): each tile is the emblem of a hero class, a heraldic object that sums up the class, with the background tint by rarity. 8 tiles:
1. Destruidor - DPS pesado; weapons: espadao, machado, martelo de bram; background tint: purple
2. Senhor da Guerra - Comandante / bruiser; weapons: espada de cinzas, espadao, machado; background tint: purple
3. Colosso - Super tank; weapons: martelo de bram, espadao, manoplas de ferro; background tint: purple
4. Imortal - Tank de sobrevivência; weapons: espadao, foice, maca sagrada; background tint: purple
5. Fantasma - Assassino sobrenatural; weapons: adagas gemeas, espada de cinzas, foice; background tint: purple
6. Cronomante - Controle; weapons: cajado de ilsa, varinha, livro da aurora; background tint: purple
7. Oráculo - Suporte / informação; weapons: livro da aurora, orbe, cajado de ilsa; background tint: purple
8. Avatar Elemental - DPS adaptativo; weapons: cajado de ilsa, varinha, orbe; background tint: purple
```

## C06 · Classes: Cavaleiro Abissal, Sangromante, Invocador Astral, Mestre das Correntes, Arquimago, Cavaleiro do Dragão, Soberano Rúnico, Caminhante do Vazio

```
Sheet C06. CLASS emblems (not abilities): each tile is the emblem of a hero class, a heraldic object that sums up the class, with the background tint by rarity. 8 tiles:
1. Cavaleiro Abissal - Tank ofensivo; weapons: espadao, foice, correntes; background tint: purple
2. Sangromante - Mage sustain; weapons: foice, livro da aurora, cajado de ilsa; background tint: purple
3. Invocador Astral - Summons / suporte; weapons: livro da aurora, cajado de ilsa, orbe; background tint: purple
4. Mestre das Correntes - Controle; weapons: correntes, chicote; background tint: purple
5. Arquimago - Mage completo; weapons: cajado de ilsa, varinha, livro da aurora; background tint: gold
6. Cavaleiro do Dragão - Bruiser; weapons: lanca do patrulheiro, espadao; background tint: gold
7. Soberano Rúnico - Controle / suporte; weapons: martelo de bram, livro da aurora, cajado de ilsa; background tint: gold
8. Caminhante do Vazio - Mobilidade / assassinato; weapons: espada de cinzas, adagas gemeas, foice; background tint: gold
```

## C07 · Classes: Santo Guerreiro, Senhor das Tempestades, Arconte Solar, Devorador de Magia, Necromante, Herdeiro do Colosso, Senhor da Matilha, Cartógrafo do Vale

```
Sheet C07. CLASS emblems (not abilities): each tile is the emblem of a hero class, a heraldic object that sums up the class, with the background tint by rarity. 8 tiles:
1. Santo Guerreiro - Tank / healer; weapons: maca sagrada, espada de cinzas, lanca do patrulheiro; background tint: gold
2. Senhor das Tempestades - AoE DPS; weapons: cajado de ilsa, orbe, lanca do patrulheiro; background tint: gold
3. Arconte Solar - DPS / suporte; weapons: espada de cinzas, maca sagrada, cajado de ilsa; background tint: gold
4. Devorador de Magia - Anti-caster; weapons: espada de cinzas, espadao, foice; background tint: gold
5. Necromante - Summons / controle; weapons: livro da aurora, foice, cajado de ilsa; background tint: crimson
6. Herdeiro do Colosso - Super tank / bruiser; weapons: martelo de bram, manoplas de ferro, espadao; background tint: crimson
7. Senhor da Matilha - Summoner / melee; weapons: machado, adagas gemeas, lanca do patrulheiro; background tint: crimson
8. Cartógrafo do Vale - Exploração / suporte estratégico; weapons: besta, arco de betula, espada de cinzas; background tint: crimson
```

## C08 · Classes: Mestre Forjador, Guardião do Santuário, Portador da Lua, Rei sem Coroa, Serafim da Aurora, Arauto do Fim, Vaso do Éter, Soberano Primordial

```
Sheet C08. CLASS emblems (not abilities): each tile is the emblem of a hero class, a heraldic object that sums up the class, with the background tint by rarity. 8 tiles:
1. Mestre Forjador - Suporte / equipamento; weapons: martelo de bram, machado, espadao; background tint: crimson
2. Guardião do Santuário - Tank / suporte; weapons: maca sagrada, lanca do patrulheiro; background tint: crimson
3. Portador da Lua - Híbrido; weapons: foice, chakrams, espada de cinzas; background tint: crimson
4. Rei sem Coroa - Comandante; weapons: espada de cinzas, lanca do patrulheiro, alabarda; background tint: crimson
5. Serafim da Aurora - Healer / DPS celestial; weapons: maca sagrada, espada de cinzas, cajado de ilsa; background tint: white-gold
6. Arauto do Fim - Debuff / AoE; weapons: foice, livro da aurora, correntes; background tint: white-gold
7. Vaso do Éter - Caster transcendental; weapons: orbe, cajado de ilsa, livro da aurora; background tint: white-gold
8. Soberano Primordial - Adaptativo; weapons: cajado de ilsa, livro da aurora, orbe; background tint: white-gold
```

## C09 · Classes: Herdeiro do Caos, Avatar do Destino

```
Sheet C09. CLASS emblems (not abilities): each tile is the emblem of a hero class, a heraldic object that sums up the class, with the background tint by rarity. 2 tiles only (leave the other tiles as empty dark navy):
1. Herdeiro do Caos - DPS imprevisível; weapons: foice, chakrams, correntes; background tint: white-gold
2. Avatar do Destino - Suporte / controle; weapons: livro da aurora, orbe, cajado de ilsa; background tint: white-gold
```
