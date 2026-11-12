# Lava, níveis, morte, multiplayer e janelas no estilo da HUD (10/10/2026)

Situação: 🟡 pronto, com os testes automáticos passando; espera o seu teste no editor.

## O que mudou

### Lava do chefe em desenho, com correnteza e fumaça
- **Material novo `M_LavaToon`**, a partir de uma textura de ruído que repete sem emenda (`Tools/MakeLavaNoise.py`, `Tools/BuildLava.py`):
  - **correnteza:** cada poça corre numa direção. Uma corrente larga e lenta torce outra fina e rápida (os veios), esticadas ao longo da corrente;
  - **crosta:** placas escuras derivam devagar, e as frestas entre elas brilham;
  - **cores em faixas de desenho:** crosta, vermelho, laranja, amarelo. O miolo da poça é mais quente e a borda vira crosta.
- **As poças** agora são manchas irregulares, com borda de crosta e pedras de basalto.
- **Bolhas** incham devagar, tremem e estouram, soltando brasa.
- **Fumaça** sobe de cada poça, no lugar da chama pequena.
- **O vulcão de Brasafenda tem uma coluna de fumaça** que se vê de longe (`AVortexSmokeColumn`).
- **A névoa ao longe ficou um pouco mais grossa**, para não ver o mundo sendo montado.

### Criaturas: nível, força e a placa sobre a cabeça
- **O nível sobe com a distância da vila inicial em todo lugar** (1 + 1 a cada 42 m, como no Godot). No continente, a faixa de nível da região é o piso.
- **A diferença de nível pesa no dano, nos dois sentidos:**
  - a criatura acima bate mais forte (+9% por nível; com 15 níveis ou mais, mata com um golpe) e apanha menos (−4,5% por nível, até 15% do dano);
  - a criatura abaixo, o contrário.
- **Placa sobre a cabeça.** Aparece de perto, em luta, ferida ou como alvo, e some devagar. Tem:
  - moldura dourada e a vida (vermelha nas hostis, âmbar nos animais) com o rastro do dano;
  - o selo do nível;
  - o nome na cor da dificuldade: **cinza** (bem abaixo), **branco** (abaixo), **verde** (seu nível), **amarelo**, **laranja**, **vermelho** (muito difícil) e **preto com caveira** (mata com um golpe).
- A barra do alvo no alto usa as mesmas cores.
- **Corrigido:** nasciam monstros dentro da cidade. Os postos do Godot eram de antes das muralhas, e agora nenhum fica dentro delas nem a 15 m.

### Morte e carregamento
- **Morte:**
  - a animação de morte do herói segura o último quadro, e ele não anda nem ataca;
  - a câmera perde a cor e escurece nas bordas;
  - a tela de morte tem véu vermelho, faixas de cinema, **"VOCÊ CAIU"** com filetes dourados, quem venceu e a contagem para renascer na Fonte;
  - **Espaço** renasce depois de 2 s.
- **Tela de carregamento ao sair do Estádio do Despertar:** anel de runas girando, o nome da região, a barra pelos pedaços que faltam montar e uma dica. Ela fica até o chão em volta estar pronto e então some, sem travar nem aparecer bugado.

### Multiplayer: grupo, troca, duelo e amizade (até 8)
- Perto de outro jogador (3,2 m) e olhando para ele: **"[E] Interagir com Nome"**. O menu oferece:
  - **Grupo** (até 8): convidar, aceitar, sair. O grupo aparece embaixo do seu quadro em cartões pequenos, com nível, nome, vida, mana e distância (e caveira para quem caiu). Os companheiros têm nome e vida verdes na cabeça e uma seta na borda da tela.
  - **Troca:** a janela abre nos dois. Clique na mochila para pôr na mesa e na oferta para tirar; as moedas mudam de 1 em 1 ou de 10 em 10. Qualquer mudança desfaz o "pronto" dos dois. Com os dois prontos, o servidor confere as mochilas e troca.
  - **Duelo** (como no Godot): contagem 3, 2, 1 e a luta. Quem chegar a 15% da vida perde, e os dois voltam à vida de antes. Afastar-se 45 m cancela. Na tela aparecem a contagem grande e a faixa com as duas vidas e a marca dos 15%.
  - **Amizade:** pedir, aceitar, desfazer. Os amigos ficam no save do herói.
- **Até 8 jogadores no mundo**, como a sala.
- **Sincronia, conferida:**
  - as criaturas e a IA rodam no servidor, e o terreno, as árvores, os coletáveis, os rios e as pontes saem iguais em todas as máquinas;
  - **corrigido:** os efeitos das habilidades de classe e as poças de lava do chefe agora chegam com garantia (antes podiam se perder com a rede cheia);
  - **corrigido:** as moitas já colhidas ficam no estado do jogo, então quem entra depois vê as mesmas.

### Janelas e menus no estilo da HUD
- **Um kit gerado no estilo da HUD** (`Tools/MakeUiKit.py`, `/Game/Vortex/UI/Kit`): azul-noite com moldura dourada em relevo e gemas azuis.
- **Peças:** janela, painel, espaço de item (normal, com o mouse, escolhido), botões (comum, dourado, apagado), abas e a faixa do título.
- **Aplicado em:**
  - janela do herói (moldura, painéis, abas, mochila e equipamento);
  - menu inicial e as páginas dele (painéis, botões, cartões dos heróis, painéis da criação);
  - pausa e opções;
  - janelas de jogador com jogador.

## Respostas
- **"Já fez o mundo dos elfos?"** Sim. A **Yggdrasil** (a Árvore do Mundo, cerca de 150 m) fica no coração de Verdalume, com a **aldeia élfica** em volta: 8 casas em anel, o arco de entrada, o Poço da Lua e as lanternas. Ela aparece no mapa quando você chega perto. A Vila dos Caçadores e o covil de Thorun ficam ao lado.

## Testes
- **`RunBossShots`:** fotos novas da lava de perto (`chefe_lava_perto*.png`).
- **`RunMountShots`:** ganhou a morte (cai, tela de morte, renasce em 5 s) e a tela de carregamento (abre e fecha quando o chão monta).
- **`RunSocialSmoke` (novo, anfitrião + cliente, no `RunAllTests`):**
  - convite e grupo;
  - duelo: contagem, luta, o golpe para em 15%, a vida volta e, fora do duelo, ninguém se fere;
  - troca de 2 poções e 20 moedas;
  - amizade;
  - sair do grupo;
  - fotos da HUD do anfitrião.

| | |
|---|---|
| ![Lava](../Images/etapa_1010c/lava.jpg) | ![Fumaça do vulcão](../Images/etapa_1010c/vulcao.jpg) |
| ![Placas das criaturas](../Images/etapa_1010c/placas.jpg) | ![Tela de morte](../Images/etapa_1010c/morte.jpg) |
| ![Carregando](../Images/etapa_1010c/carregando.jpg) | ![Grupo](../Images/etapa_1010c/grupo.jpg) |
| ![Duelo: contagem](../Images/etapa_1010c/duelo_contagem.jpg) | ![Duelo: luta](../Images/etapa_1010c/duelo_luta.jpg) |
| ![Mochila](../Images/etapa_1010c/mochila.jpg) | ![Pausa](../Images/etapa_1010c/pausa.jpg) |
| ![Menu inicial](../Images/etapa_1010c/menu.jpg) | ![Criação](../Images/etapa_1010c/criacao.jpg) |

## O que conferir no editor
1. A lava da Caldeira (correnteza, bolhas, fumaça) e a fumaça do vulcão ao longe.
2. Andar para longe da vila: o nível das criaturas sobe, e a cor do nome muda com o seu nível.
3. Morrer: a animação, a tela e o Espaço para renascer.
4. Sair do Estádio do Despertar: a tela de carregamento e o mundo já pronto quando ela some.
5. Em dois jogadores: E no outro jogador para abrir o menu, entrar no grupo, trocar, duelar e pedir amizade.
6. As janelas (I), o menu inicial, a pausa e as opções.

## Conhecido
- Na troca, o "pronto" fica no botão da janela; ainda não há um som próprio de troca.
- A amizade é por nome do herói. Dois heróis com o mesmo nome contam como o mesmo amigo.
