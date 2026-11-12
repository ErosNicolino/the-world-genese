# O saque no chão (07/11)

O usuário: "fazer o sistema de drop no chão: jogar o item no chão, ou quando matar um monstro ou abrir uma caixa, igual
ao Fortnite, o item ficar rodando no chão com um efeitinho".

- **O item no chão** (`AVortexItemDrop`):
  - sai pulando em arco de onde nasceu e fica girando e flutuando em cima de um anel;
  - tem um facho de luz para cima e uma luz, na cor da raridade (o facho é maior nas raridades e nos níveis altos);
  - o modelo é o da própria arma ou peça de armadura; o resto vira um saquinho;
  - some sozinho depois de 5 minutos.
- **Pegar:** perto aparece "[E] pegar <nome> (nv. N)", e o E põe na mochila (`UVortexHeroSheet::ServerPickup`). Com a
  mochila cheia, avisa e o item fica no chão.
- **De onde cai:**
  - os bichos: o saque deles, com o equipamento no nível do bicho;
  - os baús: equipamento outra vez, no nível do herói. O raro dá do Raro ao Lendário, o do acampamento do Incomum ao
    Épico, e o abandonado às vezes um Normal ou Incomum, ou uma poção;
  - a mochila: o botão "Jogar no chão" no detalhe do item (`ServerDropSlot`).
- Em grupo, qualquer um pega (o primeiro que apertar E). As moedas continuam indo direto para quem venceu.
