# Revisão das armas — lista do que está errado e do que fazer (11/10/2026)

Situação: 🟡 **feito, esperando o seu teste no jogo**. Os prints são seus (Estádio do Despertar). São 23 armas.
O que foi feito, como testar e o que ainda falta estão no fim, em **[Feito (11/10/2026)](#feito-11102026)**.

Para cada arma:
- **Errado:** o que aparece no print ou o que você notou jogando.
- **Pedido:** o golpe normal (clique esquerdo) e o secundário (clique direito), como você descreveu.
- **Fazer:** como eu resolveria, com números sugeridos onde você pediu para eu decidir.

---

## Problemas que se repetem em quase todas

| # | Problema | Onde aparece | O que fazer |
|---|---|---|---|
| G1 | **Braço "quebrado"**: o antebraço dobra para o lado errado ou atravessa o corpo e a arma | Quase todas (instrumento, orbe, grimório, varinha, cajado, alabarda, espadão, catalisador, corrente, chicote, foice) | A pose de segurar cada família vem de clipes de outra arma, girados. Fazer uma **pose de segurar por família** (onde fica cada mão, o cotovelo e o pulso), com as mãos ajustadas à arma (IK de duas mãos onde a arma é de duas mãos). |
| G2 | **Arma atravessando o corpo** | Adagas, manoplas (um braço), escudo dentro do braço, espadão | Mesma correção do G1, mais um afastamento mínimo do corpo em cada pose. |
| G3 | **Golpes que não condizem com a arma**: todos parecem o corte da espada | Lança, maça, espadão, espada e escudo, manoplas (o rastro) | Clipes próprios por família: estocada para lança, pancada para maça e martelo, corte largo e pesado para espadão, e o rastro do golpe com a cor e o formato de cada arma. |
| G4 | **O corpo não acompanha o golpe**: só o tronco gira e as pernas ficam paradas | Foice (giro), alabarda (coluna torta) | O giro precisa mover o corpo inteiro (raiz do esqueleto), não só a coluna. |
| G5 | **Primeira pessoa**: arma solta do corpo, olhando para baixo dá para ver a separação; golpes "indo para a frente" sem gesto de verdade | Manoplas, alabarda, martelo, machado, espadão | Braços e mãos em primeira pessoa presos à câmera, com a arma na mão, e uma animação de primeira pessoa para cada família em vez do deslocamento genérico de hoje. |
| G6 | **Mana**: toda magia e toda arma mágica deve gastar mana | Varinha, cajado, orbe, grimório, instrumento, catalisador | Custo de mana em todo golpe mágico: **pouco no golpe normal** (2–4) e **mais no secundário** (12–25), conforme a arma. |
| G7 | **Animação da arma** na mão | Grimório (abrir e passar páginas), chicote e corrente (balançar) | Peças com movimento próprio: livro que abre, corrente e chicote que balançam como corda. |
| G8 | **Projéteis saem do centro da tela**, e não da ponta da arma (1ª e 3ª pessoa) | Pistola rúnica, besta, arco, varinha, cajado, orbe, instrumento | Todo tiro sai **da ponta da arma**: boca da pistola, corda do arco, ponta do cajado. |
| G9 | **A mira não acerta onde você mira**: com a câmera por cima do ombro, a flecha sai da frente do personagem em linha reta e vai parar à direita da mira | Arco, besta, pistola e todos os projéteis | Primeiro acha **o ponto que a mira enxerga** (um raio da câmera até onde bate); depois o projétil sai da ponta da arma **em direção a esse ponto**. Assim ele sai da arma e chega exatamente na mira. |

---

## Armas mágicas

### 1. Instrumento arcano
![Instrumento arcano](../Images/revisao_armas/instrumento.png)

- **Errado:** segurado de um jeito bugado; o braço atravessa o instrumento (G1).
- **Golpe normal:** **notas musicais** saem e voam até o inimigo.
- **Secundário (com recarga): uma melodia que fortalece** o herói e os aliados.
  - Enquanto você **segura** o botão, ele continua tocando e o fortalecimento continua: **mais dano e mais resistência**.
  - Ao soltar, o fortalecimento dura **15 s no próprio herói** (para quem joga sozinho) e **3 s nos aliados**.
- **Fazer:**
  - pose de tocar (G1);
  - projétil de notas com brilho e um som curto;
  - aura em volta dele enquanto toca, que alcança os aliados num raio;
  - mana gasta aos poucos enquanto toca (G6).
- **Números sugeridos:**
  - fortalecimento de **+15% de dano** e **+15% de resistência**;
  - raio de **8 m** para os aliados;
  - gasto de mana enquanto toca: **4/s**;
  - recarga de **20 s**, contada a partir de quando ele para de tocar.

### 2. Orbe arcano
![Orbe arcano](../Images/revisao_armas/orbe.png)

- **Errado:** o braço parece quebrado (G1).
- **Golpe normal:** **esferas de energia**. O padrão é de **luz**, que tem vantagem contra mortos-vivos. No futuro haverá orbes de fogo, eletricidade etc.
- **Secundário:** invoca **2 orbes que giram em volta dele** e atacam de tempos em tempos. Com ele atacando junto, são 3 golpes de energia.
- **Fazer:**
  - elemento por orbe (luz agora; fogo e eletricidade depois) com bônus por tipo de criatura, e luz com bônus contra mortos-vivos;
  - os 2 orbes giram em volta do herói e miram o inimigo mais perto ou o alvo travado.
- **Números sugeridos** (para não ficar roubado):
  - os orbes duram **12 s** e cada um ataca **a cada 1,5 s**, com **40% do dano** do golpe normal;
  - o secundário custa **20 de mana**, com recarga de **25 s**;
  - luz: **+50% de dano em mortos-vivos**.

### 3. Grimório (o inicial: Grimório de Fogo)
![Grimório](../Images/revisao_armas/grimorio.png)

- **Errado:** o braço entra dentro do livro (G1).
- **Golpe normal:** não é um projétil como nas outras armas. Um **círculo mágico aparece em cima do inimigo mirado** (ou no chão, se errar) e invoca a magia do grimório. No de Fogo é um **tornado de fogo**.
- **Secundário: parede de fogo** onde você mirar.
  - Os monstros **reconhecem a parede e não passam**.
  - **Exceção:** monstros de pedra e de fogo (que você vai fazer no futuro) atravessam.
- **Animação:** o livro **abre e passa as páginas rapidinho** ao usar, **também em primeira pessoa** (G7).
- **Fazer:**
  - mira no chão ou no inimigo, com o círculo mostrando onde cai;
  - tornado com dano em área por alguns segundos;
  - parede que bloqueia a passagem dos monstros (eles desviam), com uma lista de "imunes à parede" (pedra, fogo);
  - um grimório para cada elemento no futuro.
- **Números sugeridos:**
  - tornado: dura **2,5 s**, dano em **raio de 2,5 m**, **3 de mana**;
  - parede: **8 m de largura**, dura **6 s**, dano a quem encosta, **25 de mana**, recarga de **18 s**.

### 4. Varinha
![Varinha](../Images/revisao_armas/varinha.webp)

- **Errado:** o braço está todo quebrado, mas a varinha está segurada certa (G1).
- **Golpe normal:** como está hoje, mas com um **efeito de magia melhor** e **gasto de mana** em cada ataque (G6).
- **Secundário:** uma magia do **elemento da varinha**; sem elemento, **um disparo mais grosso e maior** que o normal, **com carregamento curto** (não instantâneo, mas rápido, porque é uma varinha pequena).
- **Números sugeridos:**
  - normal: **2 de mana**;
  - secundário: carrega em **0,6 s**, **2,5×** o dano do normal, **12 de mana**, recarga de **6 s**.

### 5. Cajado
![Cajado](../Images/revisao_armas/cajado.webp)

- **Errado:** segurado certo, mas com o braço quebrado (G1).
- **Golpe normal:** **disparos de pequenas lanças de vinha**, como tiros.
- **Secundário: prisão de vinhas.**
  - Mira um inimigo e o prende com vinhas por um tempo.
  - **Demora para conjurar** e tem **animação de conjuração**.
- **Números sugeridos:**
  - normal: **3 de mana**;
  - prisão: conjuração de **1,2 s** (dá para cancelar), prende por **3 s** (**1,5 s em chefes**), **20 de mana**, recarga de **16 s**.

### 6. Catalisador químico
![Catalisador químico](../Images/revisao_armas/catalisador.png)

- **Errado:** a mão está bem quebrada, e o jeito de segurar também (G1).
- **Ideia:** a arma é uma **garrafa**.
  - **Golpe normal:** ele **arremessa garrafas** que **quebram no local** e causam efeitos ou dano, conforme a garrafa ou o catalisador.
  - **Secundário:** uma **poção de fortalecimento geral** que cai onde você mirar. Ela **também fortalece os inimigos** se cair neles.
- **Fazer (proposta):**
  - catalisador inicial "Ácido": dano com corrosão (**−10% de defesa por 4 s**);
  - outros catalisadores no futuro: gelo (lentidão), fogo (queimadura), veneno;
  - animação de arremesso (arco no ar), garrafa que gira e estilhaça, com respingo e poça curta;
  - poção de fortalecimento: área de **4 m**, **+15% de dano e de velocidade por 10 s** para todos dentro (inclusive inimigos), **20 de mana**, recarga de **20 s**.

---

## Armas de perto

### 7. Maça sagrada
![Maça sagrada](../Images/revisao_armas/maca.webp)

- **Errado:** golpes esquisitos e tortos em 1ª e 3ª pessoa, nada realistas (G3, G5).
- **Golpe normal:** combo de 3 golpes **realistas de maça** (pancadas de cima e de lado, sem corte de espada).
- **Secundário: pancada no chão** que abre um **rastro para a frente**, na direção do golpe.
  - Quem é atingido fica **atordoado por 2 a 3 s**. Se o monstro for muito forte ou resistente, o atordoamento **não funciona, mas o dano entra**.
  - Ao mesmo tempo **cura uma pequena parte da vida** do herói, com um **efeito sagrado** em volta.
- **Números sugeridos:**
  - rastro de **7 m**;
  - atordoamento de **2,5 s**, ignorado se o monstro estiver **5 níveis ou mais acima**, ou se for chefe;
  - cura de **8% da vida máxima**;
  - **15 de mana** e recarga de **14 s**.

### 8. Manoplas
![Manoplas](../Images/revisao_armas/manoplas.png) ![Manoplas em primeira pessoa](../Images/revisao_armas/manoplas_1p.webp)

- **O que você gostou:** o jeito de segurar e os golpes, que estão perfeitos.
- **Errado:**
  - um braço parece entrar no corpo (G2);
  - o efeito do golpe normal parece de espada (G3);
  - **em primeira pessoa** ficam soltas do corpo (dá para ver a separação olhando para baixo), e o soco é só a manopla indo para a frente (G5).
- **Fazer:**
  - rastro de soco (impacto curto, sem arco de lâmina);
  - em primeira pessoa, braços presos à câmera, com soco de verdade: recuo, giro do ombro e retorno.

### 9. Adagas duplas
![Adagas duplas](../Images/revisao_armas/adagas.png)

- **O que você gostou:** ele leva as adagas para trás ao correr.
- **Errado:** entram no corpo (G2); nos golpes normais o movimento quebra os braços (G1).
- **Secundário:** um **avanço rápido bem para a frente**, cortando os inimigos no caminho como uma **tesourada**.
- **Números sugeridos:** avanço de **6 m em 0,25 s**, dano a todos no caminho, recarga de **8 s**. Não gasta mana, porque não é magia.

### 10. Alabarda
![Alabarda](../Images/revisao_armas/alabarda.png) ![Alabarda em primeira pessoa, no golpe](../Images/revisao_armas/alabarda_1p.webp)

- **Errado:**
  - a coluna e os braços quebram, e o golpe sai todo torto (G1, G4);
  - ele **bate com a parte de trás** da alabarda, inclusive em primeira pessoa (print do golpe acima).
- **Golpe normal:** **alcance bem maior** que o das outras armas.
- **Secundário:** um **corte vertical forte** que solta uma **onda de impacto**.
- **Números sugeridos:**
  - alcance de **4,5 m** (a espada tem ~3 m);
  - onda de **8 m** para a frente, **2×** o dano, recarga de **12 s**.

### 11. Lança
![Lança](../Images/revisao_armas/lanca.webp)

- **Errado:** o golpe parece de espada, e ele mexe as duas mãos ao golpear (G3).
- **Golpe normal:** entra em **posição de lança, de lado**, segurando com as duas mãos, e **estoca**.
- **Secundário:** uma **rajada de estocadas**.
- **Referência:** a pasta `VortexReferencia/Content/SpearAnimationsV2` tem animações de lança (posição, estocada, rajada). Mesmo sendo de outro esqueleto, servem de base para a pose e o ritmo.
- **Números sugeridos:** rajada de **6 estocadas em 1,2 s**, cada uma com **45% do dano**, recarga de **10 s**.

### 12. Martelo (sem print)
- **Certo** em terceira pessoa.
- **Errado:** **em primeira pessoa** os golpes e o jeito de segurar ficam bem bugados (G5).
- **Secundário:** **pancada no chão com dano em área**.
- **Números sugeridos:** raio de **4 m**, **1,8×** o dano, recarga de **12 s**.

### 13. Machados duplos
![Machados duplos](../Images/revisao_armas/machados_duplos.webp)

- **Errado:** a textura é feia, e **um deles está de cabeça para baixo** na mão.
- **Golpe normal:** **mais lento** que o das lâminas duplas.
- **Secundário:** ele **arremessa os machados**, com recarga antes de poder arremessar de novo.
- **Fazer:**
  - textura nova (lâmina de metal, cabo de madeira);
  - virar o machado invertido;
  - arremesso que vai e volta como bumerangue, ou que cai no chão e é recolhido. Sugiro **ir e voltar**.
- **Números sugeridos:** **1,6×** o dano de cada machado, alcance de **12 m**, recarga de **10 s**. Enquanto os machados não voltam, ele fica sem atacar.

### 14. Machado
![Machado](../Images/revisao_armas/machado.png)

- **Errado:**
  - segurado errado, e a textura feia nem parece machado;
  - golpes bugados, indo para trás do corpo, e em primeira pessoa também (G1, G5).
- **Golpe normal:** **lento, mas com mais dano**.
- **Secundário:** um **giro**: ele fica girando um pouco, dando dano em tudo em volta.
- **Números sugeridos:** giro de **2 s**, dano **4 vezes** em **raio de 2,5 m**, recarga de **14 s**.

### 15. Espadão
![Espadão](../Images/revisao_armas/espadao.webp) ![Espadão em primeira pessoa](../Images/revisao_armas/espadao_1p.webp)

- **Errado:**
  - o tronco quebra, e o jeito de segurar também (G1);
  - parece que ele **passa a lâmina** nos inimigos em vez de cortar (G3).
- **Golpe normal:** golpes **bem lentos** e pesados.
- **Secundário:** um **corte lateral** que **empurra todos os inimigos para longe**, menos os chefes.
- **Números sugeridos:** arco de **180°**, empurrão de **6 m**, recarga de **12 s**.

### 16. Espada e escudo
![Espada e escudo](../Images/revisao_armas/espada_escudo.webp)

- **Errado:**
  - jeito de segurar bugado, com o mesmo defeito do espadão: não parece cortar com a lâmina (G1, G3);
  - **o escudo entra no braço** (G2).
- **Secundário:** **usar o escudo**, não uma habilidade. A defesa com o escudo é **muito melhor** que a de qualquer outra arma.
- **Números sugeridos:**
  - escudo levantado bloqueia **80%** do dano de frente (as outras armas bloqueiam ~40%);
  - a defesa perfeita **devolve o atordoamento** ao inimigo.

### 17. Corrente com lâmina
![Corrente com lâmina](../Images/revisao_armas/corrente.webp)

- **Errado:** o braço está quebrado (G1), e a arma parece algo duro, sem movimento de corrente (G7).
- **Golpes:** animação e golpes que façam sentido para uma corrente: girar, chicotear e puxar.
- **Secundário:** **3 golpes rápidos** com a corrente.
- **Fazer:**
  - corrente como uma corda de elos com física simples (balança e se estica no golpe);
  - lâmina na ponta;
  - alcance médio.
- **Números sugeridos:** alcance de **5 m**; secundário com **3 golpes em 0,8 s**, **70%** do dano cada, recarga de **9 s**.

### 18. Chicote
![Chicote](../Images/revisao_armas/chicote.png)

- **Errado:** o braço está quebrado (G1); o chicote parece minúsculo na mão, como uma mola, e não balança.
- **Fazer:** um **chicote de verdade**, comprido, com animação de corda (G7).
- **Secundário:**
  - **grudar nas coisas** (uma árvore, por exemplo) e se **puxar até lá**, como os ganchos de Attack on Titan;
  - se acertar um **inimigo**, ele **afasta o inimigo** com uma animação que faça sentido e dá dano.
- **Números sugeridos:** alcance de **15 m**, puxada a **25 m/s**; no inimigo, empurrão de **4 m** e **1,2×** o dano; recarga de **6 s**.

### 19. Foice (sem print)
- **O que você gostou:** os golpes.
- **Errado:** os dois braços e o quadril quebram; no giro, **só o tronco gira** e as pernas ficam paradas (G1, G4).
- **Secundário:** um **golpe giratório** também.
- **Números sugeridos:** giro de **360° em 0,6 s**, raio de **3 m**, **1,5×** o dano, recarga de **10 s**.

---

## Armas de distância

### 20. Chakrams
![Chakrams](../Images/revisao_armas/chakrams.png)

- **Errado:** segurados errado (G1); sem animação, o ataque é só uma luz esquisita.
- **Golpe normal:** o **modelo do chakram voa** até o alvo, gira, corta e **volta para a mão**.
- **Secundário (sua ideia, como a shuriken gigante do Naruto):** um **chakram gigante** arremessado como **bumerangue**, cortando tudo na ida e na volta.
- **Números sugeridos:**
  - normal: alcance de **14 m**, volta em ~0,6 s, alternando as mãos;
  - secundário: chakram de **2 m de diâmetro**, ida de **16 m**, dano em tudo que atravessa (ida e volta), **2×** o dano, recarga de **12 s**.

### 21. Pistola rúnica
![Pistola rúnica](../Images/revisao_armas/pistola.webp) ![Pistola rúnica em primeira pessoa](../Images/revisao_armas/pistola_1p.webp)

- **Errado:**
  - na mão parece uma varinha; não dá para entender que é uma pistola (G1);
  - em primeira pessoa fica de lado (G5);
  - o tiro sai do centro da tela, e não da boca da pistola (G8).
- **Fazer:** segurar como pistola (cabo na mão, cano para a frente, braço estendido para mirar), com clarão de runa na boca a cada tiro.
- **Golpe normal:** tiros rúnicos rápidos, gastando pouca mana (G6).
- **Secundário (sugestão, já que você pediu algo único): "Selo Rúnico".**
  - A pistola dispara uma **runa que gruda** no inimigo ou no chão.
  - Depois de **1,5 s** ela **explode** em área.
  - Os inimigos marcados recebem **+25% de dano dos tiros normais** por 5 s, o que combina o secundário com o ataque básico.
  - **Números sugeridos:** explosão em **raio de 3 m**, **2×** o dano, **15 de mana**, recarga de **12 s**.

### 22. Besta
![Besta em primeira pessoa](../Images/revisao_armas/besta_1p.webp) ![Besta](../Images/revisao_armas/besta.png)

- **Errado:** fica **de lado**, tanto em primeira quanto em terceira pessoa (G1, G5).
- **Golpe normal:** o tiro vai **mais longe** e é **mais forte** que o do arco.
- **Secundário:** **3 tiros seguidos, rápidos**, também com mais alcance.
- **Números sugeridos:**
  - alcance de **40 m** (o arco tem ~25 m);
  - normal com **1,3×** o dano do arco e recarga de **0,8 s** entre tiros;
  - secundário: **3 tiros em 0,45 s**, recarga de **9 s**.

### 23. Arco
![Arco](../Images/revisao_armas/arco.png) ![Arco em primeira pessoa](../Images/revisao_armas/arco_1p.webp)

- **Errado:**
  - o arco entra no corpo (G2);
  - em primeira pessoa fica feio (G5);
  - a flecha não vai para onde você mira (G9).
- **Golpe normal:** **carregar antes de atirar**, como no Godot: segurar estica a corda, e quanto mais tempo, mais dano e alcance.
- **Flechas presas:** ficam **cravadas onde acertam** (chão, árvores, paredes e inimigos) por até **1 minuto**, em vez de sumir na hora.
- **Secundário:** **chuva de flechas** no alvo ou onde você estiver mirando.
- **Números sugeridos:**
  - carga cheia em **0,8 s** (de 0,5× a 1,5× do dano);
  - no máximo **40 flechas cravadas** no mundo (as mais antigas somem primeiro);
  - chuva: **raio de 4 m**, **2,5 s**, ~**20 flechas**, **20 de mana**, recarga de **16 s**.

---

## Ordem sugerida para fazer
1. **Poses de segurar e braços (G1, G2)** de todas as famílias de uma vez. É o que mais aparece, e as outras correções dependem disso.
2. **Golpes próprios por família (G3, G4)**: lança, maça, espadão, espada e escudo, alabarda, machados, foice.
3. **Primeira pessoa (G5)** com braços presos à câmera, e **os tiros saindo da ponta da arma em direção à mira (G8, G9)**.
4. **Os secundários novos**, por grupo: mágicos (instrumento, orbe, grimório, varinha, cajado, catalisador), depois os de perto.
5. **Mana em toda magia (G6)** e as peças animadas (G7: grimório, corrente, chicote).
6. **Texturas novas** (machados).
7. Um **teste com fotos de cada arma**: pose parada, golpe em 3ª e 1ª pessoa e o secundário, para conferir tudo junto.

## Faltam
- Prints do martelo e da foice, se quiser que eu confira a pose deles também.

---

## Feito (11/10/2026)

### Problemas gerais
| # | Situação | Como ficou |
|---|---|---|
| G1, G2 | ✅ | **104 clipes novos**, feitos pose a pose no Blender para 21 famílias (`Tools/Blender/BuildWeaponClips.py` + `weapon_families.py`, com o posador `vortex_poses.py`). As mãos vão onde a pose manda e o braço é resolvido por IK de dois ossos quadro a quadro; armas de duas mãos têm a esquerda presa no cabo. A **pose de segurar** (`Vx_<Família>_Hold`) manda só no tronco e nos braços: parado, andando e correndo as pernas continuam as do movimento. |
| G3 | ✅ | Golpes próprios: estocadas na lança, pancadas (cima, lado e chão) na maça, cortes lentos com o **fio na frente** no espadão, na alabarda, no machado e na foice. A orientação do fio vem da medida de cada modelo (o lado largo da lâmina), então a alabarda não bate mais com as costas. Rastro por família: soco curto nas manoplas (sem arco de lâmina), traço reto de estocada na lança, arco na cor de cada arma nas outras. |
| G4 | ✅ | Clipes `*_Full` (giros, investidas, pancadas no chão) mexem o corpo inteiro (quadril, pernas e raiz), não só o tronco. |
| G5 | ✅ | Primeira pessoa refeita: um **corpo-cópia preso à câmera** com a mesma animação do herói, mostrando só **antebraços, mãos e a arma** (o resto encolhe a zero). Cada família tem o gesto de verdade em 1ª pessoa e não existe mais separação ao olhar para baixo (o corpo de verdade some só para você; a sombra continua). Ajuste fino: `ViewForward` e `ViewDrop` em `VortexCharacter.h`. |
| G6 | ✅ | Mana: golpe normal de 1 a 3, secundário de 12 a 25, conforme a tabela de `VortexWeaponKit.cpp`. Sem mana o golpe não sai e aparece o aviso. |
| G7 | ✅ | Grimório em peças: a **capa abre e três folhas passam** a cada golpe, e fecha parado. Chicote e corrente viram **cordas simuladas** (balançam com o movimento e esticam até o alcance no golpe; a corrente leva a lâmina na ponta). |
| G8 | ✅ | O tiro sai da **ponta da arma** (boca da pistola, ponta do cajado e da varinha, corpo do instrumento, mão no arco, na garrafa e no chakram). |
| G9 | ✅ | Primeiro o ponto que a mira enxerga; depois o tiro sai da arma **em direção a esse ponto**. A flecha calcula o arco da gravidade para cair exatamente na mira. |

### Cada arma (números da lista acima)
| Arma | Golpe normal | Secundário |
|---|---|---|
| Instrumento | Notas musicais (partículas de nota) | Segurar: toca, aura em volta, +15% de dano e resistência num raio de 8 m, 4 de mana/s; ao soltar fica 15 s em você e 3 s nos aliados; recarga de 20 s |
| Orbe | Esferas de **luz** (+50% em mortos-vivos, para quando existirem) | 2 orbes giram em volta e atiram no inimigo mais perto a cada 1,5 s por 12 s |
| Grimório de Fogo | Círculo no alvo (ou no chão) e **tornado de fogo** de 2,5 s | **Parede de fogo** de 8 m por 6 s; as criaturas desviam (Golem e Colosso de pedra atravessam) |
| Varinha | Disparo com brilho novo, 2 de mana | Carga de 0,6 s e disparo grosso de 2,5× |
| Cajado | **Lanças de vinha** | Prisão de vinhas: conjura 1,2 s (apertar de novo cancela e devolve a mana), prende 3 s (1,5 s em chefes) |
| Catalisador | Garrafa em arco que **estilhaça**: dano em volta e ácido (−10% de defesa por 4 s) | Poção de fortalecimento: +15% de dano e velocidade por 10 s para **todos** no raio, inimigos também |
| Maça | Pancada de cima, de lado e no chão | Pancada sagrada: rastro de 7 m, atordoa 2,5 s (não em chefes nem 5 níveis acima), cura 8% |
| Manoplas | Clipes que você aprovou; rastro de soco | Soco Sério (como antes) |
| Adagas | Cortes alternados e tesoura | Avanço de 6 m cortando quem estiver no caminho |
| Alabarda | Alcance de 4,5 m, com o fio | Corte vertical e onda de impacto de 8 m (2×) |
| Lança | Estocadas de lado | Rajada de 6 estocadas em 1,2 s (45% cada) |
| Martelo | Clipes do Godot (estavam certos) | Pancada no chão, raio de 4 m (1,8×) |
| Machados duplos | Mais lentos que as adagas; o machado da esquerda na posição certa | Arremessa os dois, vão e voltam (bumerangue); sem golpe até voltarem |
| Machado | Lento e forte, segurado certo, cabeça nova | Giro de 2 s, 4 acertos em volta |
| Espadão | Muito lento, cortes com o fio | Varrida de 180° que empurra 6 m (menos chefes) |
| ~~Espada e escudo~~ | **Tirada do jogo** a seu pedido (o escudo do jogo vai ser outro) | — |
| Corrente | Chicotadas e giro, alcance de 5 m | 3 golpes rápidos |
| Chicote | Chicote comprido de verdade | Gancho: gruda (até 15 m) e puxa você até lá; num inimigo, empurra e dá 1,2× |
| Foice | Colheitas largas | Giro de 360° em 0,6 s com o corpo inteiro |
| Chakrams | O **chakram voa**, corta e volta para a mão (alternando) | Chakram gigante de 2 m, vai e volta cortando tudo |
| Pistola rúnica | Modelo novo de pistola (cano, tambor rúnico, cabo para baixo); tiros rúnicos com 1 de mana | **Selo Rúnico**: gruda, explode em 1,5 s (raio de 3 m, 2×) e o marcado leva +25% de dano por 5 s |
| Besta | 40 m de alcance, 1,3× o dano do arco | 3 virotes em 0,45 s |
| Arco | **Segurar estica a corda** (0,8 s para a carga cheia, de 0,5× a 1,5×); as flechas ficam **cravadas por 1 min** (no máximo 40) | Chuva de ~20 flechas num raio de 4 m |

### Sons (97 novos)
Cada família tem golpe (3 variações), acerto e tiro próprios, e cada secundário tem o seu (`wfx_*` em `/Game/Vortex/Audio/Sfx`).
Saem dos pacotes do VortexReferencia (magia modular, Free_Sounds_Pack, SmallSoundKit), dos sons do Godot e de síntese
(cordas do alaúde, vidro estilhaçando, elos de corrente, estalo do chicote, lufadas com o peso de cada arma).
Os sons dos secundários agora tocam para todos os jogadores (antes alguns tocavam só no servidor).

### Modelos e texturas
- Machado e machados duplos: cabeças novas e metais mais claros (o aço 100% metálico ficava quase preto no estádio). Os metais de todas as armas ficaram mais claros.
- Pistola rúnica nova; grimório em 3 peças (livro, capa, folha); chicote e corrente com cabo + corda animada (a lâmina da corrente é uma peça).

### Como refazer
1. Blender: `exec(open("Tools/Blender/BuildWeaponClips.py").read()); build(); export()` (clipes) e
   `blender --background --python Tools/Blender/BuildVortexWeapons.py -- <tipos>` (modelos).
2. `python Tools/BuildWeaponSfx.py` (sons; antes, uma vez, `Tools/ExportReferenceSounds.py` no projeto VortexReferencia).
3. Editor fechado: `Tools/ImportVortexWeapons.py`, `Tools/ImportWeaponClips.py`, `Tools/ImportWeaponSfx.py`.

### Como testar
- **No jogo:** pegue cada arma no Estádio do Despertar; teste o clique esquerdo, o direito e a 1ª pessoa (C).
- **Automático:** `Tools/RunWeaponCloseUp.ps1` fotografa cada arma parada (dos dois lados), no golpe, no secundário e em 1ª pessoa (`Saved/Screenshots/perto_*`). Rodado com as 23 armas: todos os secundários saíram.

### Segunda rodada (depois do seu teste)
- **Punhos:** a torção da mão agora vai para o antebraço (como o braço de verdade gira); no pulso fica só a dobra. As mãos não aparecem mais viradas em relação ao braço.
- **Espada e Escudo** fora do jogo: Estádio, itens e armas das classes (`Data/Rpg`); os geradores (`BuildRpgData.py`, `BuildWeaponData.py`) também tiram.
- **1ª pessoa:** manoplas mais longe; alabarda, lança e foice mais para a direita e para baixo (o cabo não corta mais o meio da tela); ajuste por família em `VortexWeaponKit::ViewOffset`.
- **Projéteis de magia invisíveis:** o material de brilho era procurado numa pasta errada; agora todos os tiros mágicos e os orbes do secundário brilham (com faíscas e luz).
- **Grimório:** a capa e as folhas não carregavam (o importador põe as peças numa subpasta), por isso o livro parecia aberto e quebrado. Agora fica fechado parado e abre e folheia ao usar.
- **Tornado:** refeito como uma espiral de chamas girando em cima do círculo mágico.
- **Chakram gigante** (e o normal): gira deitado, como disco.
- **Chuva de flechas:** flechas de verdade caindo do céu, inclinadas, que ficam cravadas; som novo (assobios descendo e as flechas cravando).

### Terceira rodada
- **Meio da tela livre em 1ª pessoa**, em todas as armas: se a mão ou a arma entram num círculo de ~15° em volta da mira (soco, arco puxado, cabo da alabarda), os braços deslizam para o lado e para baixo.
- **3ª pessoa com algo atrás:** quando a parede empurra a câmera para dentro do herói, ele some só para você (a sombra fica), em vez de a tela ficar cheia dele por dentro.
- **Secundários mais fortes:** de 2 a 5 vezes o golpe normal (por acerto), pela recarga e pela demora. Os números estão em `VortexWeaponKit.cpp`.
- **Arco sem metralhadora:** a corda precisa esticar pelo menos 0,3 s (soltar antes faz a flecha sair quando chega lá) e há 0,45 s entre uma flecha e outra.
- **Giros:** o corpo gira duas vezes mais rápido (machado ~760°/s por 2 s; foice uma volta em 0,45 s) e o efeito agora são lâminas de luz em crescente girando junto, no lugar dos arcos soltos.
- **Garrafa e poção do catalisador:** mirando num inimigo, a garrafa vai nele (e cai onde ele estiver quando chegar); mirando no chão, cai onde você mirou.
- **Coletáveis:** o teste com a tecla E de verdade colhe e põe na mochila. Se a mochila (30 espaços) estiver cheia, agora o aviso diz isso claramente; cada recusa fica registrada no log (`VORTEX_HARVEST`).

### Ainda falta / para você conferir
- **Ouvir os sons**: eu não consigo ouvir; conferi duração e volume de cada um, mas o gosto é seu.
- O ajuste fino das poses é a olho: se alguma família ainda estiver estranha, diga qual e o que incomoda.
- Manoplas e martelo continuam com os clipes do Godot (você aprovou); o braço entrando no corpo das manoplas (G2) não foi mexido para não estragar o que você gostou.
- Saves antigos com uma Espada e Escudo na mochila continuam funcionando (a arma ainda existe por dentro), só não aparece mais em lugar nenhum novo.
- Mortos-vivos e monstros de fogo ainda não existem; o bônus da luz e a exceção da parede de fogo já esperam por eles (por nome da espécie).
