# As 21 armaduras (07/11)

Das pranchas do usuário (`modelando ainda/Futuras modelagens/Armaduras/Armaduras - <Raridade>.jpg`, 3 conjuntos por
raridade). 84 peças (capacete, peitoral, calças, botas), cada uma com atributos e uma passiva própria; cada conjunto
completo (4 peças) tem mais uma passiva forte. Fotos: `Docs/Images/armaduras/`.

## O visual

- **Feitas no Blender** pelo mesmo gerador das roupas (`Tools/Blender/MakeBlockOutfit.py -- jogo A_<conjunto>_<peça>`,
  com os desenhos em `Tools/Blender/armor_designs.py`): cascas de bloco em volta do corpo, presas ao esqueleto, em
  camadas (gibão + placa do peito + faixa; ombreira em 3 lâminas; coxote, caneleira, saiote em 2 lâminas; sola, bico e
  cano da bota; no elmo faixa, bochechas, crista e protetor da nuca) e os enfeites (chifres, asas, penacho, galhos,
  cauda de escorpião, coroa de estrelas, garras).
- **A pintura é a da prancha:** cada peça recortada (`Tools/Armor/SliceArmorSheets.py` -> `Data/VortexArmor/Pieces`) é
  projetada nos blocos (a frente na imagem inteira, as costas espelhadas, os lados na borda). As partes claras e
  saturadas (gemas, lava, runas) brilham: a máscara `T_A_<...>_mask` (vermelho = brilha, verde = franja).
- **Capacetes:** o nosso rosto só tem os olhos grandes; os elmos fechados têm uma janela larga na altura dos olhos e o
  queixo fechado (sem boca). Todo capacete esconde o cabelo; os capuzes e chapéus têm uma franja que pega a cor do
  cabelo do herói (`M_ArmorBlock`: `HairTint`). Na primeira pessoa o capacete some para quem joga.
- **Trocam a roupa:** os 6 trajes do herói saem em 4 materiais (cabeça, tronco, pernas, pés); a peça vestida esconde
  a região que cobre (`AVortexCharacter`: `HideCoveredOutfit`, `ApplyArmorParts`). Nada fica por cima atravessando.
- **As antigas** (capuz de couro, cota de malha, lobo, guarda, casca, cratera, dunas, sombra, sábio, Azhûr, Yggdrasil)
  vestem o visual do conjunto novo parecido (`visual` no json), com os atributos delas.

**Faces faltando (07/11, à noite):** cada face de bloco nasce solta e o `recalc_face_normals` do Blender chutava o lado de
fora (várias ficavam para dentro e o jogo não desenha): agora o `MakeBlockOutfit.py` vira cada face pela direção dela
na caixa (vale para os trajes também). A conferência (`PreviewArmor.py`) usa o mesmo corte de faces do jogo.

## Refazer

1. `python Tools/Armor/SliceArmorSheets.py` (as peças recortadas e a conferência).
2. `python Tools/Armor/BuildArmors.py` (nomes, atributos, passivas, ícones `armor_*`) e `python Tools/BuildArsenal.py`.
3. `bash Tools/Armor/MakeAllArmor.sh [conjunto ...]` (Blender, ~20 min as 84) e, para conferir,
   `blender -b --factory-startup -P Tools/Blender/PreviewArmor.py -- <raiz> <saida.png> <conjunto ...>`.
4. Os trajes divididos: `MakeBlockOutfit.py -- jogo <traje>` para os 6 do herói.
5. Editor fechado, pelo bash: `ImportBlockArmor.py`, `ImportBlockOutfits.py`, `ImportItemIcons.py`.
6. Foto no jogo: `Tools/RunWeaponCloseUp.ps1 -Types "set:guarda_real,set:magma"`.

## As passivas (`Source/Vortex/Private/Rpg/VortexArmor.cpp`)

Os números ficam no `armors.json` (ajustar não compila). Peças com a mesma passiva somam. Bloqueio, espinhos, gelo,
brasa e lava em quem bate, guarda alta, última defesa e renascer quando o herói apanha; veneno, brasa, roubo de vida e
a cura da vitória quando ele bate; regeneração, mana, abrigo e chuva de estrelas com o tempo; velocidade, esquiva,
moedas e XP nos atributos. Onde achar: as comuns e incomuns na Rosa (couro) e no Garrick (metal); das raras para
cima no saque (bandidos, goblins, golens, Colosso de Runas); as divinas só no Golem e no Colosso.

| Conjunto | Raridade | 2 peças | 4 peças | As peças (passiva) |
|---|---|---|---|---|
| Couro do Viajante | Normal | 3 haste | **Pé na Estrada**: +6% de velocidade ao andar | Capuz do Viajante (+4% de moedas); Colete do Viajante (recupera 1% da vida a cada 5 s); Calças do Viajante (a esquiva volta 5% mais rápido); Botas do Viajante (+3% de velocidade ao andar) |
| Gibão do Recruta | Normal | 1 defense, 10 max_health | **Disciplina**: o primeiro golpe a cada 10 s tira 50% menos | Chapéu de Ferro do Recruta (3% de chance de anular um golpe); Gibão Acolchoado (o primeiro golpe a cada 12 s tira 25% menos); Calças Acolchoadas (com menos de 30% da vida, leva 8% menos de dano); Botas Enfaixadas (cada inimigo vencido devolve 1% da vida) |
| Peles do Caçador | Normal | 2 crit | **Instinto do Caçador**: +12% de dano contra feras | Gorro de Pele (+4% de dano contra feras); Casaco de Peles (+5% de dano contra feras); Calças de Peles (+3% de velocidade ao andar); Botas de Pele (cada inimigo vencido devolve 1% da vida) |
| Malha de Ferro | Incomum | 2 defense, 2 resistance | **Muralha de Ferro**: 8% de chance de anular um golpe | Elmo de Nasal (3% de chance de anular um golpe); Cota de Malha Reforçada (4% de chance de anular um golpe); Perneiras de Malha (3% de chance de anular um golpe); Botas de Ferro (com menos de 30% da vida, leva 8% menos de dano) |
| Couro Batido do Batedor | Incomum | 3 crit, 3 speed | **Pés Leves**: a esquiva volta 20% mais rápido | Capuz do Batedor (a esquiva volta 6% mais rápido); Gibão Tachonado (depois de esquivar, o próximo golpe em 2 s é crítico); Calças Tachonadas (a esquiva volta 6% mais rápido); Botas do Batedor (+4% de velocidade ao andar) |
| Escamas de Bronze | Incomum | 2 defense, 15 max_health | **Escamas Afiadas**: devolve 12% do dano recebido a quem bateu | Elmo de Escamas (devolve 3% do dano recebido a quem bateu); Couraça de Escamas (devolve 5% do dano recebido a quem bateu); Saiote de Escamas (devolve 4% do dano recebido a quem bateu); Botas de Bronze (devolve 3% do dano recebido a quem bateu) |
| Placas do Guarda Real | Raro | 3 defense, 3 resistance | **Juramento do Guarda**: com menos de 30% da vida, leva 25% menos de dano | Elmo de Penacho Azul (4% de chance de anular um golpe); Peitoral com Tabardo (o primeiro golpe a cada 10 s tira 30% menos); Coxotes do Guarda (com menos de 30% da vida, leva 10% menos de dano); Grevas do Guarda (3% de chance de anular um golpe) |
| Couraça de Gelo | Raro | 5 resistance | **Coração Gelado**: 12% de chance de congelar quem te acerta por 1,2 s | Elmo de Cristal de Gelo (8% de chance de deixar lento quem te acerta); Couraça de Gelo (10% de chance de deixar lento quem te acerta); Calças de Gelo e Pele (8% de chance de deixar lento quem te acerta); Botas de Neve (+4% de velocidade ao andar) |
| Armadura da Maré | Raro | 2 hp_regen, 10 max_mana | **Maré Cheia**: recupera 2,5% da vida a cada 5 s | Elmo de Conchas (recupera 2% da mana a cada 5 s); Couraça de Coral (recupera 1,2% da vida a cada 5 s); Calças de Escama Marinha (recupera 1% da vida a cada 5 s); Botas da Maré (recupera 2% da mana a cada 5 s) |
| Placas da Noite Violeta | Épico | 3 attack, 2 lifesteal | **Fome da Noite**: rouba 5% do dano como vida (10% à noite) | Elmo Chifrudo da Noite (rouba 1% do dano como vida (2% à noite)); Peitoral Rúnico da Noite (rouba 1,5% do dano como vida (3% à noite)); Perneiras Rúnicas (rouba 1% do dano como vida (2% à noite)); Botas Rúnicas (+4% de velocidade ao andar) |
| Carapaça do Escorpião | Épico | 4 crit, 2 attack | **Veneno do Deserto**: 35% de chance de envenenar o alvo | Elmo do Ferrão (8% de chance de envenenar o alvo); Carapaça de Quitina (10% de chance de envenenar o alvo); Perneiras de Quitina (devolve 5% do dano recebido a quem bateu); Botas de Garra (8% de chance de envenenar o alvo) |
| Escamas do Dragão Rubro | Épico | 3 attack, 20 max_health | **Sangue de Dragão**: 35% de chance de queimar o alvo | Elmo do Dragão (não queima); Peitoral de Escamas Rubras (12% de chance de queimar quem te acerta); Perneiras Rubras (8% de chance de queimar o alvo); Garras do Dragão (8% de chance de queimar o alvo) |
| Placas do Grifo Dourado | Lendário | 5 crit, 6 speed | **Voo do Grifo**: a esquiva volta 25% mais rápido | Elmo Alado do Grifo (a esquiva volta 8% mais rápido); Peitoral do Grifo (com a vida cheia, +8% de dano); Coxotes Dourados (+5% de velocidade ao andar); Garras do Grifo (+5% de velocidade ao andar) |
| Armadura de Magma | Lendário | 4 attack, 6 resistance | **Erupção**: 20% de chance de explodir em lava em volta ao ser atingido | Elmo de Obsidiana (não queima); Couraça de Magma (8% de chance de explodir em lava em volta ao ser atingido); Perneiras de Magma (12% de chance de queimar quem te acerta); Botas de Lava (6% de chance de explodir em lava em volta ao ser atingido) |
| Couraça do Bosque Ancestral | Lendário | 3 hp_regen, 30 max_health | **Seiva Ancestral**: recupera 3,5% da vida a cada 5 s | Elmo de Galhos (recupera 1,2% da vida a cada 5 s); Couraça de Casca (recupera 1,5% da vida a cada 5 s); Saiote de Folhas (devolve 6% do dano recebido a quem bateu); Botas de Raiz (com menos de 30% da vida, leva 12% menos de dano) |
| Armadura Espelhada | Único | 8 resistance, 4 skill | **Reflexo Perfeito**: devolve 25% do dano recebido a quem bateu | Elmo Espelhado (devolve 6% do dano recebido a quem bateu); Peitoral Espelhado (devolve 8% do dano recebido a quem bateu); Coxotes Espelhados (5% de chance de anular um golpe); Botas Espelhadas (devolve 6% do dano recebido a quem bateu) |
| Armadura Sem Sombra | Único | 6 crit, 20 crit_dmg | **Golpe Sem Sombra**: depois de esquivar, o próximo golpe em 3 s é crítico | Elmo Sem Sombra (a esquiva volta 10% mais rápido); Peitoral Sem Sombra (com a vida cheia, +10% de dano); Calças Sem Sombra (a esquiva volta 10% mais rápido); Botas Sem Sombra (+6% de velocidade ao andar) |
| Vestes do Andarilho Perdido | Único | 6 haste, 30 max_health | **Sorte de Viajante**: +15% de experiência | Gorro Remendado (+8% de moedas); Colete Remendado (+6% de experiência); Calças Remendadas (+8% de moedas); Botas Desparelhadas (+6% de velocidade ao andar) |
| Armadura da Aurora | Divino | 50 max_health, 8 resistance | **Renascer na Aurora**: ao cair, levanta com 30% da vida (a cada 5 min) | Elmo Alado da Aurora (recupera 1,5% da vida a cada 5 s); Peitoral da Aurora (com menos de 30% da vida, leva 15% menos de dano); Saiote de Penas (recupera 1,5% da vida a cada 5 s); Botas Aladas (+7% de velocidade ao andar) |
| Couraça da Árvore do Mundo | Divino | 4 hp_regen, 50 max_health | **Abrigo da Árvore**: cura você e os aliados a até 10 m em 1,2% da vida por segundo | Elmo de Madeira Viva (recupera 1,5% da vida a cada 5 s); Couraça da Árvore (cura você e os aliados a até 9 m em 0,4% da vida por segundo); Perneiras de Raízes (devolve 8% do dano recebido a quem bateu); Botas de Raiz Antiga (com menos de 30% da vida, leva 15% menos de dano) |
| Manto das Estrelas | Divino | 8 crit, 8 skill | **Chuva de Estrelas**: a cada 8 s uma estrela cai num inimigo a até 16 m | Elmo Estelar (recupera 3% da mana a cada 5 s); Peitoral das Constelações (com a vida cheia, +12% de dano); Coxotes Estelares (a esquiva volta 10% mais rápido); Botas Estelares (+7% de velocidade ao andar) |
