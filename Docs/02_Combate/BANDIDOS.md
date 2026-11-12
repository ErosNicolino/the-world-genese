# Bandidos, acampamentos e baú guardado

Este documento fecha o trabalho dos bandidos, que estava "em andamento" (commit `5431b64`), e a pendência dos baús guardados que constava em [CONTINUAR_ARSENAL](CONTINUAR_ARSENAL.md).

## Os 4 tipos
Todos usam o boneco do aventureiro com roupa sorteada (`UVortexDressComponent::BanditLook`) e a arma do tipo.

| Tipo | Arma | Como luta |
|---|---|---|
| `Bandit` (o primeiro do bando é o CAPITÃO) | espada | combo de 3 e estocada; o capitão tem 1,6× vida e malha cor de vinho |
| `BanditShield` | espada e escudo | em guarda, golpes pela frente passam só 40% |
| `BanditBrute` | marreta | 1,45× maior, pancada no chão com aviso |
| `BanditArcher` | arco | recua e atira flecha de verdade |

- **Roupa corrigida:** as mangas ficavam no verde padrão do herói e pareciam a túnica do aventureiro. Agora seguem a cor da camisa, sempre escura.

![os quatro](../Images/bandidos/bandidos_quatro.png)

## Acampamentos
- **Onde aparecem:** cerca de 1 a cada 4 quadras de 72 m, longe da cidade (`UVortexCreatureSubsystem::BanditCampAt`).
- **O bando:** de 3 a 4 bandidos, no nível do lugar (mínimo 2).
- **O que é montado:** fogueira acesa, troncos de assento, caixotes e às vezes uma carroça (`AVortexLandmarks::BuildCamp`). Tudo é montado quando alguém chega a 200 m.

## Baú guardado (novo)
- **Onde fica:** cada acampamento tem um baú entre os caixotes, de frente para a fogueira. É o mesmo baú das cabanas: `AVortexWorldFeatures::AddCampChest`, com o desenho em `ChestBoxes`. Em 25% dos acampamentos ele é o baú raro, cor de vinho com ouro.
- **Só abre com o bando vencido:**
  - Com bandidos vivos a menos de 22 m, o E responde "O bando ainda guarda o baú! (N de pé)" (`GuardsAlive`).
  - O aviso na tela diz "Baú dos bandidos: vença o bando para abrir".
- **O saque:**
  - sempre dá um equipamento;
  - dá mais moedas que o baú das cabanas (40 + nível × 4);
  - também dá poção e materiais da região.
- **Aberto fica aberto:** fica marcado no save de cada herói, como os baús das cabanas.

![baú no acampamento](../Images/bandidos/bandidos_bau.png)

## Testes
`Tools/RunBanditSmoke.ps1` passou e agora também está no `RunAllTests.ps1`. Ele confere:
- os 4 tipos nascem;
- o bando fere o herói;
- o batedor atira uma flecha;
- o escudo segura: 5,5 de dano pela frente contra 10,9 pelas costas;
- vencer o bando dá XP;
- o acampamento é montado com o bando em volta;
- o baú não abre com o bando de pé e abre depois, dando moedas (de 38 para 100 no teste).

## Lobos em matilha (pedido junto)
- **O problema:** no vale do começo os lobos vinham dos postos fixos do Godot, um por posto, por isso apareciam sempre sozinhos.
- **Agora:** em todo o mundo, a matilha tem de 1 a 3 lobos, sorteados. Isso vale para os postos fixos, as quadras do continente e os lobos da noite.
- **O alfa:** na matilha de 3, o líder é sempre o ALFA (`AVortexWolf::bAlpha`). Ele é 1,35× maior, tem 2,2× a vida e a mordida 1,5× mais forte. A placa mostra "ALFA · LOBO DA BRUMA" com o efeito de elite.
- **Teste:** `RunWolfSmoke` passou.
