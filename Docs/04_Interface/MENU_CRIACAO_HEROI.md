# Menu novo, seleção de raça e criação de herói

Status: **aguardando teste no editor**.

O menu deixou de ser uma lista de botões sobre uma foto e virou um **palco 3D ao vivo**. O herói de verdade aparece animado, com a espada, em cima do palco de pedra do vale pintado. O menu é transparente por cima.

A base é o fluxo que você fez (mundos, heróis por mundo, LAN, opções e saves). Visual, raças e criação foram refeitos com um estilo próprio, sem copiar o Godot.

## Palco (`AVortexMenuStage`)

- **Cenário:** o vale pintado do Godot (`character-select-background-v3.png`) fica na frente da câmera, sempre cobrindo a tela. O herói é posicionado para pisar exatamente no palco de pedra da pintura.
- **Luzes:** sol de fim de tarde vindo da direita, luz fria de preenchimento e contraluz dourada, com exposição fixa como no jogo.
- **Círculo de runas:** fica sob os pés, na cor da raça. Gira, respira, pulsa na revelação da raça e explode em luz ao criar o herói.
- **Câmera:** balança devagar. Na criação dá para girar o herói arrastando, aproximar com a roda do mouse, focar o rosto e centralizar.
- **Raças grandes:** o bárbaro fica mais para trás no palco para caber inteiro.
- **Som:** a música tema "Echo des Montagnes" (do Godot) entra em fade, e a interface usa os sons de clique, abrir, erro, equipar e subir de nível do Godot.
- **Abertura:** uma cortina escura se abre quando o mundo do menu começa a rodar. Antes disso o palco ainda não estava montado e aparecia cinza.

## Telas (`SVortexFrontEnd`)

- **Visual:**
  - vidro azul-noite com borda dourada;
  - brasas quadradas (o mundo é de blocos) subindo pela tela;
  - cada tela entra com um fade subindo.
- **Botões:** acendem, deslizam e mostram uma barra dourada. O botão principal é dourado e "respira".
- **Início:** brasão flutuando, "VORTEX — A LENDA DO VALE", "Continuar • nome do último mundo" e os demais botões. No palco aparece um herói sorteado de qualquer raça.
- **Mundos, novo mundo, LAN, opções e excluir herói:** painéis de vidro. As opções mostram valores legíveis (qualidade Baixa/Média/Alta/Épica, Ligada/Desligada…).
- **Seus heróis:** até 3 cartas com a arte da raça. Clicar mostra aquele herói no palco, com a borda pulsando na cor da raça; o espaço vazio vira "+ Novo herói". Os botões são "Entrar no mundo", "Excluir herói" e "Voltar".
- **Escolha sua raça:** 6 cartas com:
  - a arte da raça;
  - o nome e o epíteto (Filhos do Vale, Guardiões da Floresta, Herdeiros da Cratera, Clãs da Pedra, Gigantes do Gelo, Erguidos pelos Cristais);
  - a frase da raça;
  - os 8 atributos em barras que enchem, verdes ou vermelhas em relação ao humano;
  - os traços (+ e −).
- **Animação das cartas:** elas sobem uma a uma. No hover a carta sobe, a arte dá zoom e a borda pulsa na cor da raça. Ao escolher, a carta vai para o centro e cresce, as outras caem e somem, e vem um clarão na cor da raça com "VOCÊ ESCOLHEU — MORTO-VIVO — Erguidos pelos Cristais". Depois disso abre a criação.
- **Criação:**
  - nome da raça grande na cor dela, epíteto e altura (anão 0,93 m, bárbaro 2,10 m);
  - **Estilo:** Aventureiro/Aventureira e uma linha por categoria, com o nome da peça e a posição na lista (< Trança da floresta 2/10 >);
  - **Cores:** amostras para pele (paleta da raça), cabelo, camisa, mangas, calça, botas, luvas, cinto e alça; a camisa leva as mangas junto; botão para sortear só as cores;
  - nome com **sorteio por raça** (sílabas próprias: "Thorgrim" para anão, "Velthulan" para elfo…);
  - "Surpreenda-me" sorteia o visual inteiro.
- **Criar herói:** o herói dá uma volta e golpeia com a espada, o círculo explode em luz, toca o som de subir de nível e aparece "NASCE UM HERÓI — nome". Em seguida abre a lista de heróis com ele selecionado.

## Peças e raças diferentes de verdade

- **Peças do Unreal:** de 22 para **392**. Vieram as 153 peças comuns do Godot (cabelos, olhos, bocas, 15 roupas, 16 calças, 15 calçados, 15 luvas, 16 acessórios e os traços de raça) e as **240 peças próprias das raças**, 40 por raça (10 cabelos, 10 roupas, 10 olhos e 10 acessórios; no morto-vivo, 10 crânios no lugar dos olhos).
- **Regras próprias do Vortex** (o Godot misturava as comuns com todas as raças):

| Raça | Traço | Regra |
|---|---|---|
| Morto-vivo | Crânio (limpo, olhos azuis, rachado, coroa de runas, rei caído…) | Sempre careca; sem olhos nem boca comuns |
| Elfo | Orelhas | Cabelos e olhos só élficos |
| Diabo | Chifres e cauda | Cabelos e olhos só da raça |
| Anão | Barba | Cabelos e olhos só da raça |
| Bárbaro | Marcas de guerra | Cabelos e olhos só da raça |
| Humano | — | Usa as peças comuns e as próprias |

  Roupas e acessórios mostram primeiro os da raça e depois os comuns.
- **Cores por paleta**, como no Godot: cada tom da textura sabe a que cor pertence, e a cor nova mantém sombras e detalhes. A paleta das peças de raça contém a do herói, então é uma textura só por herói, gerada em tempo de jogo.
- **Tamanho e proporção da raça** (ajuste pedido depois: "anão pequeno e parrudo"), no palco e **no jogo**. O corpo tem altura e largura separadas, e cada parte tem proporção própria (`shape` em `look_catalog.json`; os valores ficam em `Tools/BuildLookCatalog.py`):

  | Raça | Altura | Proporção |
  |---|---|---|
  | Anão | 0,98 m | Baixo e parrudo: pernas curtas e grossas, tronco largo, braços grossos, cabeça grande |
  | Bárbaro | 2,05 m | Gigante largo, braços grossos, cabeça proporcionalmente menor |
  | Elfo | 1,52 m | Esguio, pernas longas |
  | Diabo | 1,45 m | Tronco um pouco mais largo |
  | Morto-vivo | 1,42 m | Magro: pernas e braços finos |
  | Humano | 1,40 m | Referência |

  - **Pose:** a proporção entra na animação de cada quadro (`FVortexAnimInstanceProxy::ApplyBody`). As pernas encolhem e o quadril desce junto, para os pés continuarem no chão.
  - **Jogo:** a cápsula de colisão e a altura da câmera seguem a altura da raça. O anão passa por lugares mais baixos e o bárbaro fica mais largo.
  - **Rede:** o visual inteiro (raça, peças e cores) agora é replicado (`FVortexNetLook`), então os outros jogadores veem a sua aparência e o servidor usa o mesmo tamanho na colisão.
  - **Testes:** movimento, rede, entrada do cliente, câmera, colisão com obstáculos e heróis passaram depois da mudança.
- **No jogo:** o herói entra no mundo com raça, peças, cores e tamanho salvos. A HUD mostra o nome e a raça de verdade, e o retrato acompanha.

## Arquivos

- **Novos:**
  - `Framework/VortexLook.h/.cpp`: catálogo, regras das raças, nomes e paleta.
  - `Framework/VortexMenuStage.h/.cpp`: o palco.
- **Reescritos ou alterados:**
  - `VortexFrontEnd.h/.cpp`: reescrito.
  - `VortexCharacter`: `ApplyLook`, espaços de traço e acessório.
  - `VortexGameInstance`: cria o palco, salva as cores, `GetHeroLook` e aplica o visual no jogo.
  - `VortexAimHUD`: nome e raça.
  - `Vortex.Build.cs`: módulo `ImageCore`.
- **Ferramentas:**
  - `Tools/ConvertGodotRacePieces.py`: peças de raça a partir do `RacePieces.glb`.
  - `Tools/BuildLookCatalog.py`: gera `Data/GodotCharacter/look_catalog.json`, `palette.png` e `palette_map.json`.
  - `Tools/CreateFrontEndArt.py`: artes das raças, brasão, cenário, sons, música e os materiais `M_StageBackdrop` e `M_StageRing`.
  - `ConvertGodotWardrobe.py --all-local` e `ImportWardrobe.py`: as suas, só rodadas com mais peças.
- **Teste:** `Tools/RunFrontEndShot.ps1` tira fotos do início, das raças, da revelação, da criação (morto-vivo, elfa, close do rosto, bárbaro e anão), da comemoração e dos heróis, e apaga o mundo de teste no fim.

## Testes

- `RunFrontEndShot.ps1`: todas as fotos conferidas.
- `RunHeroSmoke`, `RunMenuFlowSmoke` e `RunSaveSmoke`: passaram.
- `RunHudShot`: o herói entra no jogo vestido com um visual sorteado (uma diaba).

## Riscos e pendências

- O repositório cresce cerca de 400 MB: 370 peças novas em `Content` e os `.glb` intermediários em `Data`. Vale considerar Git LFS antes de enviar ao GitHub.
- A primeira abertura do menu numa máquina nova compila os materiais; a cortina cobre isso.
- Os números das raças (atributos, dano, regeneração) aparecem nas cartas, mas ainda não valem no combate. Entram com a próxima etapa: níveis, atributos e inventário.
- A fonte continua sendo a Roboto da engine.

## Como voltar atrás

Reverta o commit. Os saves dos heróis guardam peças e cores em campos que já existiam, então não quebram.

## Revisão depois do primeiro teste no editor

Pedidos do jogador: luz estourada; "mundo fake tranquilo" estilo Cube World com câmera livre; cores que não mudavam; ficar preso ao trocar de raça; elfo "bugado"; cor livre com HEX.

**Mundinho de verdade:** o cenário pintado e as três luzes improvisadas saíram.
- O menu agora mostra um canto do vale perto do lago, com terreno, grama, girassóis, árvores, céu, nuvens, borboletas e sons.
- A luz é a mesma do jogo: sol, céu, névoa e a exposição já calibrada, com o dia parado às 10h30.
- O sol fica atrás da câmera, um pouco de lado, para iluminar o herói de frente.
- O motion blur está desligado no menu (borrava o herói ao mover a câmera).
- O jogador do menu é espectador, e o terreno passou a ser montado em volta do espectador (`AVortexTerrainStreamer::GatherViewers`).

**Câmera livre:** arrastar com o botão esquerdo gira a câmera em volta do herói, o direito sobe e desce, a roda aproxima e "Rosto" dá close. Raças pequenas não ficam coladas na câmera; as grandes ficam mais longe.

**Cores que não mudavam:** o material do boneco veio da importação do glTF e usa a paleta como `BaseColorTexture`; o código trocava um parâmetro "Palette" que esse material não tem. Agora troca os dois.

**Peças próprias da raça** (cabelos, roupas, olhos e acessórios de elfo, diabo…): têm cores fixas na paleta, como no Godot. Agora são tingidas pela cor escolhida dividida pela cor original da peça (`BaseColorFactor`, o mesmo cálculo do Godot), então o cabelo e a roupa do elfo também mudam de cor. A aparência "bugada" do elfo vinha principalmente da luz estourada, que deixava tudo branco.

**Trocar raça:** vindo da criação, "Voltar à criação" retorna sem perder nada. Escolher outra raça mantém o nome, as cores e as peças que a nova raça aceita; só a pele vai para a da raça.

**Cor livre:** cada cor tem um botão "+" ao fim das amostras, que abre ao lado um quadrado de saturação e brilho, uma faixa de matiz e um campo HEX (#RRGGBB).

**Correção de travamento:** as imagens das cartas e o brasão podiam ser apagados da memória pela coleta de lixo enquanto o menu as desenhava, o que travava o jogo. Agora o `UVortexGameInstance::KeepForMenu` segura essas imagens.

**Teste de peças:** `-VortexLookProbe=elfo` veste a raça com cada peça dela e tira uma foto de cada em `Saved/Screenshots/probe`.

## Correção — cards de raça e cancelar (2026-10-06)

- Os cards de raça sumiam: eram escondidos (`Hidden`) enquanto ainda faziam o fade de entrada, e um widget escondido para de atualizar, então nunca voltavam. Agora só ficam escondidos depois de uma raça ser escolhida.
- A criação ganhou **Cancelar** (canto inferior esquerdo): volta à lista de heróis sem salvar. Na seleção de raça, **Voltar** volta aos heróis (ou "Voltar à criação" quando veio de "Trocar raça").

## Cosméticos em três lugares (2026-10-06)

- **Antes:** o "Acessório" único do Godot, com 77 peças misturadas.
- **Agora:** três categorias que se combinam (um chapéu, uns óculos e uma capa ao mesmo tempo), cada uma com a própria peça no corpo:
  - **Chapéu e cabeça** (25): chapéus, coroas, tiaras, auréolas, penas, flores, faixas. No herói: `Accessory`.
  - **Rosto** (16): óculos, máscaras, monóculos, tapa-olhos, brincos. No herói: `AccessoryFace`.
  - **Pescoço e costas** (35): capas, cachecóis, colares, ombreiras, aljava, lanternas, bandoleiras. No herói: `AccessoryBack`.
- Cada raça continua com os próprios cosméticos (os `rw_<raça>_accessory_*`) além dos comuns.
- "Nenhum" vem primeiro: o herói começa limpo. O "Surpreenda-me" põe cada cosmético só às vezes (45%).
- Saves com o `accessory` antigo migram sozinhos para o lugar certo (`FVortexLookCatalog::Normalized`).
- A classificação é por palavras do nome, em `Tools/BuildLookCatalog.py` (`HEAD_WORDS`, `FACE_WORDS`; o resto vai para costas).
- **Como testar:** na criação, as três linhas novas no painel ESTILO. Na foto `frontend_criacao_anao` o anão aparece com chapéu de viajante, óculos de forja e capa.

## Correção — rosto do morto-vivo (2026-10-06)

- Os 13 crânios do morto-vivo são **placas** na frente do rosto (4,6 cm de espessura), feitas para ficar sobre a cabeça, como no Godot.
- A regra "sem rosto" do Vortex tirava a cabeça de baixo, e o esqueleto ficava com um rosto chato, sem o cubo dos outros.
- Agora a cabeça fica, na cor da pele (osso), e o crânio por cima.
- Cabelo, olhos e boca continuam escondidos.
- Mudança em `Tools/BuildLookCatalog.py` (`faceless: False`).
