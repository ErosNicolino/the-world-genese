# A economia (07/11)

O usuário: "como está a economia do jogo?". Depois: "faça tudo isso".

## O diagnóstico

- **O que entra cresce com o nível:**
  - os bichos dão (7 + nível) / 2 moedas em média (o elite, 5x): uns 1.400 por hora no nível 5 e uns 10.000 no 80;
  - os baús dão de 45 a 140 moedas + até 120 de sorte + 5 a 8 por nível;
  - o equipamento vendido vale mais conforme o nível do item;
  - as missões dão 8.060 moedas no total.
- **Para onde ia:** só as lojas, e nelas nada crescia. As poções custavam ~12 e curavam 40 de vida fixa (nada no nível
  50), e o equipamento à venda vinha no nível natural da raridade. Do meio do jogo para frente as moedas sobravam sem
  uso.

## O que mudou

1. **A forja do Bram** (o maior gasto do fim do jogo, como a melhoria dos MMOs):
   - na loja do Bram, "Forja: melhorar o que você veste", sobe 1 nível do item vestido (até o nível do herói);
   - custa um quarto do preço do item no nível novo, mais o material da raridade (1 + nível / 30): sucata de ferro (do
     Normal ao Raro), fragmento de obsidiana (Épico e Lendário) ou fragmento de colosso (Único e Divino);
   - fica em `VortexShop::ForgeCost` e `UVortexHeroSheet::ServerForge`;
   - exemplos, do nível natural ao 80: a Rara, ~14 mil moedas; a Lendária, ~27 mil; a Divina, ~76 mil. Um nível perto
     do 50 custa uns 200 (Rara), 500 (Lendária) e 2.000 (Divina);
   - assim a arma de que você gosta acompanha você.
2. **A loja no seu nível:**
   - o equipamento à venda (Ducci, Bram, Garrick, Rosa e os viajantes) vem no nível do herói, com o preço junto
     (`VortexShop::Stock(..., HeroLevel)`);
   - o equipamento sem preço escrito usa o preço da raridade, que agora também cresce com o nível do item, e a venda
     acompanha.
3. **As poções curam uma parte da vida:**
   - a vermelha (40) cura 30% da vida, a verde (30) ~23%, a carne (12) ~9%;
   - a azul (30) devolve 30% da mana;
   - o preço sobe 10% por nível do herói a partir do 2: no 50, a poção que custava 12 custa ~71.

Durabilidade e conserto ficaram de fora (incomodam mais do que ajudam).

**Conferido:**
- **A forja**, na conferência do Arsenal: o Arco das Estrelas foi do 40 para o 41 por 1.690 moedas e 2 fragmentos de
  colosso. A Divina pedia sucata de ferro (o `RarityIndex` da loja não conhece a Divina); a forja agora conta as 7
  raridades.
- **As lojas:** o `Tools/RunShopSmoke.ps1` passou (21 de 21), com o Ducci vendendo no nível do herói
  (`martelo_de_guerra@1`).
- **As poções e a loja dentro do jogo** não foram testadas jogando.
