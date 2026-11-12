# Arsenal, lojas e mapas das caravanas (11/10/2026)

Situação: 🟡 pronto, com os testes automáticos passando; espera o seu teste no editor. O arsenal (itens, elementos, conjuntos e armadura no herói) você já testou e aprovou ("testei eles tudo e tá certo").

## 1. Arsenal (continuação do `CONTINUAR_ARSENAL.md`)

O código que tinha ficado salvo sem compilar compilou sem erros. Você testou as armas com elemento e aprovou.

**O que o arsenal tem:**
- 392 itens no catálogo (`VORTEX_ARSENAL itens=392 conjuntos=9`): 200 novos e 191 ajustados;
- 9 conjuntos de armadura;
- elemento e peculiaridade ao acertar;
- tiros e rastros na cor do elemento;
- armadura aparecendo no herói.

**Novo na janela do herói (painel do item):**
- **Elemento e peculiaridade** numa linha na cor do elemento (por exemplo VENENO • SANGRAMENTO). O texto do efeito aparece embaixo só quando ainda não está na descrição.
- **Conjunto:** o nome, quantas peças você está vestindo (por exemplo 2/4) e os bônus de 2 e 4 peças. O bônus que já vale fica verde.
- **Valor** do item em moedas.

Fotos (do teste de criaturas e interface): `Saved/Screenshots/ui_item_elemento.png` e `ui_item_conjunto.png`.

## 2. Lojas (economia)

As lojas do Godot (`Main.Shop.cs` e `Main.Vendors.cs`) foram portadas. Os diálogos dos comerciantes já tinham a ação `open_shop`, que antes só mostrava "(As lojas chegam em breve.)". Agora a conversa fecha e a loja abre.

| Comerciante | Onde | Vende | Compra |
|---|---|---|---|
| **Ducci**, mercador mago | cidade | 8 armas de raridade sorteada (comum 38%, incomum 28%, rara 18%, épica 10%, lendária 5%, única 1%), 3 armaduras ou acessórios e as poções de sempre; uma de cada | tudo da mochila, por 30% (materiais pelo valor de mercado) |
| **Bram**, forja | cidade | 6 armas forjadas sorteadas e escudo de carvalho | armas (45%) e metais (150%) |
| **Garrick**, armaria | cidade | 5 armaduras de metal e escudo | armaduras de metal e escudos (45%), sucata e obsidiana (120%) |
| **Rosa**, alfaiataria | cidade | 5 roupas de pano, couro e pele | peles, fibras, galhadas e bandanas (140%), roupas usadas (40%) |
| **Ilsa**, ervas e poções | cidade | poções, antídoto, ervas e 2 cajados ou livros | ervas, cogumelos, gel, musgo, asas e frutas (150%) |
| **Marta**, taverna | cidade | pão, carne, rações e frutas | comida (130%) |
| **Caravana** das trilhas, **Sahil** (dunas), **Tavian** (mata) | trilhas e vilas | o básico de viagem e os mapas, sem acabar | os achados, como o Ducci |

**Regras:**
- **Estoque do dia:** é sorteado pelo dia do mundo, o mesmo para todos no mesmo mundo, e acaba. O que você comprou some até o dia seguinte, e cada herói tem a sua cota.
- **Servidor decide:** cada compra e venda é conferida no servidor (moedas, estoque, mochila cheia). A loja só funciona a até 8 m do comerciante.
- **Save:** o dia e o que foi comprado nesse dia ficam no save do herói, em dois campos novos. Saves antigos abrem com esses campos vazios.
- **Preços:**
  - itens do arsenal usam o preço do arsenal;
  - os outros, o preço do Godot. Agora o `rpg.json` traz o preço de 132 itens (`Tools/BuildRpgData.py` passou a copiar o campo);
  - sem preço, vale a tabela do Godot por raridade.

**A janela da loja:**
- No mesmo estilo da janela do herói, com o título e a fala do comerciante.
- **À esquerda, o estoque:** ícone, raridade, quantas restam, elemento, preço (vermelho quando falta moeda) e COMPRAR ou ESGOTADO.
- **À direita, o que ele compra da sua mochila e por quanto:** VENDER 1 e TUDO.
- Passe o mouse num item para ver a descrição. Esc ou E fecha.

**Arquivos:**
- `Source/Vortex/*/Rpg/VortexShop.*`: estoque, preços e quem compra o quê;
- `UVortexHeroSheet::ServerShopBuy/ServerShopSell`;
- `Source/Vortex/Public/Framework/VortexShopWindow.h` e o fim de `VortexHeroWindow.cpp`;
- o gancho no `VortexDialogue.cpp`.

## 3. Mapas das caravanas

O **Mapa de viajante** (70 moedas) e a **Carta de terras distantes** (220) agora podem ser usados da mochila (botão Usar), como no Godot (`Main.Maps.cs`):
- **Mapa de viajante:** copia para a carta uma rota sinuosa de 500 m, saindo das terras em volta da cidade.
- **Carta de terras distantes:** copia uma rota de 900 m que começa longe (500 m a 1,6 km) e termina num trecho grande aberto.
- **Como a rota é desenhada:** contorna o mar e a borda do mundo, com borda irregular e manchas do que se via dos lados. Cada mapa sorteia uma rota diferente.
- **Depois de usar:** a parte revelada é gravada na hora no save do herói. O aviso mostra quantos pedaços novos saíram da névoa (M para ver).

## Testes

- **`Tools/RunShopSmoke.ps1` (novo, também no `RunAllTests.ps1`):**
  - estoque estável no mesmo dia e novo no dia seguinte;
  - os 9 comerciantes com estoque;
  - sem moedas não compra;
  - comprar o pão da Marta até esgotar (8);
  - vender 3 de volta pelo preço dela;
  - o Bram recusa comida;
  - o Ducci vende uma de cada;
  - longe do comerciante não compra;
  - comprar e usar o mapa de viajante aumenta a parte descoberta da carta;
  - fotos `loja_bram.png`, `loja_ducci.png`, `loja_marta.png` e `loja_mapa_viajante.png`.
- **Teste de criaturas e interface:** fotos novas do painel do item (elemento e conjunto).
- **Bateria rápida (`RunAllTests.ps1`):** 25 de 27 passaram em 34 min. As duas que falharam, Movimento e Rede: movimento (conexão perdida), passaram quando rodadas de novo sozinhas. São intermitentes, não ligadas a estas mudanças.
- **Mapa:** o mapa de viajante revelou 0,07% do continente inteiro (uma rota de 500 m num mapa de vários quilômetros).

## Como testar no jogo

1. Na Cidade do Santuário, fale com o Bram, a Marta, a Ilsa, a Rosa, o Garrick ou o Ducci (E) e escolha a opção da loja.
2. Compre e venda. Repare no estoque que esgota e na mudança de estoque no dia seguinte.
3. Compre um mapa de viajante da caravana, use pela mochila (I) e abra o mapa (M).
4. Na mochila, escolha uma arma com elemento e uma peça de conjunto, e veja o painel do item.

## O que ainda falta (pedidos seus)

- **Forja:** fazer itens com materiais (você disse que vai fazer a forja depois).
- **Baús espalhados** pelo mundo, guardados por bandidos ou monstros.
- **Acampamentos de bandidos** (humanoides hostis).
- **Monstros:** IA mais esperta e com comportamento próprio por espécie, variantes (goblin mago, lobo alfa com chifres, golem de fogo, morcego cuspidor), monstros das outras regiões e o bestiário.
