# Janelas cartoon: visual, animações e sons

O kit em pixel-art ("Aventura", em [JANELAS_AVENTURA](JANELAS_AVENTURA.md)) não combinava com a letra de desenho animado. Este kit o substitui nas 6 abas do herói e na loja.

## Visual
- **Peças do kit:**
  - Geradas por `Tools/MakeUiKitCartoon.py`, em `Data/UI/KitCartoon`.
  - Importadas por `Tools/ImportUiKitCartoon.py` para `/Game/Vortex/UI/KitCartoon`.
  - No código, ficam em `VortexUiKit::Cartoon`.
- **Estilo:**
  - janela azul viva, com cantos redondos e contorno grosso azul-escuro;
  - botões coloridos com a parte de baixo mais escura;
  - aba ativa amarela;
  - nome da página numa fita laranja saindo pela borda de cima;
  - "X" vermelho redondo no canto;
  - números em blocos coloridos (vida vermelha, mana azul…).
- **Personagem ao vivo:** aparece sem o fundo preto. A câmera grava a cor e a profundidade, e o material `M_UiDoll` deixa transparente onde não há nada.
- **Texto centrado:** a letra cartoon desenha no alto da linha. `CartoonNudge` desce o texto para o meio das caixas.
- **Voltar ao visual antigo:** em `VortexHeroWindow.cpp`, troque `Av = VortexUiKit::Cartoon` por `Av = VortexUiKit::Aventura`.

## Animações
- **Botões de borracha (`SJuicyButton`):**
  - com o mouse, crescem e balançam;
  - apertados, achatam;
  - soltos, quicam.
  - O estado fica guardado por nome (`Id`), então a animação continua mesmo quando a página é refeita depois do clique.
- **Estrelinhas (`SCartoonFx`):** cada clique solta um anel, estrelas e faíscas. A cor depende do clique: raridade do item, dourado na compra, verde no ponto.
- **Janela:** abre pulando (passa do tamanho e assenta) e fecha encolhendo. A loja também abre pulando.
- **Troca de aba:** a página entra deslizando com quique, a aba nova pula e a fita do título dá um soco.
- **Gastar ponto:** "+1" verde sobe, o número dá um soco e fica verde por um instante.
- **Avisos que pulsam:** o selo "N PONTOS" e a bolinha vermelha das abas.
- **Moedas e itens:**
  - o número das moedas rola até o valor novo, e a moeda pula;
  - itens de cor forte (raros para cima) têm o brilho pulsando.
- **Ainda não feito:** o item "voando" até o lugar no corpo ao vestir. Hoje, vestir dá o estouro de estrelas e o som.

## Sons
São criados por `Tools/MakeUiSoundsCartoon.py`, que gera `Data/UI/SonsCartoon/*.wav`; a importação é a mesma do kit, para `/Game/Vortex/Audio/UI`, na categoria Interface.

| Som | Quando | Origem |
|---|---|---|
| `ui_c_hover` | mouse passa no botão | Kenney UI Audio (rollover2) |
| `ui_c_pop` | clique comum | Kenney Interface (pluck_001) |
| `ui_c_select` | escolher item, missão | Kenney Interface (select_002) |
| `ui_c_tab` | trocar aba | Kenney Interface (switch_003) |
| `ui_c_open` / `ui_c_close` | abrir / fechar | Kenney Interface (maximize_006 / minimize_006) |
| `ui_c_point` | gastar ponto, despertar | mola sintetizada + Kenney (confirmation_002) |
| `ui_c_coin` | vender | Kenney RPG Audio (handleCoins) |
| `ui_c_buy` | comprar | Kenney RPG (handleCoins2) + Interface (confirmation_001) + sininho |
| `ui_c_equip` | vestir | Kenney RPG (handleSmallLeather) + Interface (glass_002) |
| `ui_c_use` | poção | bolhas sintetizadas ("glub glub") |
| `ui_c_error` | (reservado) | Kenney Interface (error_004) |

**Licença:** os pacotes da Kenney (Interface Sounds, UI Audio e RPG Audio, de kenney.nl) são **CC0**. São livres para uso comercial e não exigem crédito; dar crédito à Kenney é opcional, mas gentil. Os arquivos originais e as licenças estão em `Data/UI/SonsKenney`. Os sons sintetizados são do projeto.

## O jogo todo no estilo cartoon (24/10)
- **Um lugar só:** os efeitos (sons, estrelinhas, botão de borracha, soco dos números e texto centrado) saíram da janela do herói. Agora ficam em `Framework/VortexUiJuice.h`, e todas as telas usam.
- **Kit antigo trocado por baixo:** as funções do kit antigo (`VortexUiKit::Window/Panel/Row/Banner/Header/ItemSlot/Button`) passaram a devolver as peças cartoon. Assim, o menu, a pausa, as opções e as janelas de jogador trocaram de visual sem reescrever cada tela.
- **Sons antigos viram cartoon:** os sons antigos da interface (`ui_click`, `ui_open`, `ui_close`, `ui_error`) viram os cartoon em `VortexAudio::Play2D`. O clique baixinho de "passar o mouse" vira o tique.
  - Cada som de interface é contado, e o botão de borracha não toca o "pop" por cima de outro som.
- **Menu inicial:**
  - a letra Cinzel deu lugar à cartoon;
  - os botões e as entradas grandes viraram cartões azuis, e o "Jogar" ficou amarelo;
  - todos são de borracha, com estrelinhas no clique.
- **Pausa e opções:** a janela é azul; os títulos ficam na letra cartoon; os botões e setas são de borracha; os valores aparecem em branco com contorno.
- **Conversa com os NPCs:**
  - caixa azul, com o nome numa fita laranja;
  - as respostas são botões de borracha;
  - escolher uma resposta faz o som de "escolher".
- **Janelas de jogador (interagir, trocar):** mesma janela azul e botões de borracha.
- **Ainda no estilo antigo:** a HUD em jogo (barras, barra de habilidades, minimapa), porque é desenhada por outro sistema (Canvas), e a tela do Despertar.

| Menu | Mundos | Criação | Pausa |
|---|---|---|---|
| ![](../Images/janelas/cartoon_menu_inicio.png) | ![](../Images/janelas/cartoon_menu_mundos.png) | ![](../Images/janelas/cartoon_menu_criacao.png) | ![](../Images/janelas/cartoon_pausa.png) |

## Fotos
| Herói | Mochila | Técnicas |
|---|---|---|
| ![](../Images/janelas/cartoon_personagem.png) | ![](../Images/janelas/cartoon_mochila.png) | ![](../Images/janelas/cartoon_habilidades.png) |

| Classe | Alquimia | Missões |
|---|---|---|
| ![](../Images/janelas/cartoon_classe.png) | ![](../Images/janelas/cartoon_alquimia.png) | ![](../Images/janelas/cartoon_missoes.png) |

A prévia desenhada antes de mexer no jogo está em `Images/janelas/proposta_cartoon.png`.

## Testes
- `RunHudShot`: passou e fotografou as 6 abas.
- `RunShopSmoke`: passou.
- A suíte completa não foi rodada.
- As animações e os sons não aparecem nas fotos. Precisam ser vistos e ouvidos no jogo.

## Para testar no jogo
1. Abra com I ou Tab e veja a janela pular.
2. Passe o mouse nos botões: eles balançam e fazem um tique.
3. Troque de aba: ouça o som e veja a página deslizar.
4. Gaste um ponto: deve aparecer "+1", estrelas e o som de mola.
5. Na mochila, clique e dê duplo clique numa poção (som de bolhas) e num equipamento (couro + brilho).
6. Na loja, compre (moedas + sininho) e venda (moedas).
7. Feche no "X" e veja a janela encolher.
