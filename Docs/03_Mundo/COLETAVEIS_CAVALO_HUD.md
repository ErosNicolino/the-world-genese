# Coletáveis, cavalo, bússola, marcações e primeira pessoa (10/10/2026)

Situação: 🟡 pronto, com os testes automáticos passando; espera o seu teste no editor.

Esta etapa cobre a segunda lista de 10/10, menos o mundo inteiro (o continente, sem borda para cair), que é a próxima etapa.

## O que mudou

### Interface
- **Mochila (I) piscando:** corrigido. A lista de missões avisava "mudou" a cada atualização, e a janela se redesenhava sem parar.
- **Retrato da HUD:** agora é uma foto parada, tirada de frente, do rosto, quando o herói é criado (fica em `Saved/Portraits`). Ela não muda mais com o tamanho do corpo. Heróis antigos ganham a foto uma vez, na primeira vez que ficarem parados.
- **Bússola** no alto da tela, com:
  - N, L, S e O;
  - a cidade, a Caldeira e o objetivo da missão;
  - os amigos e as marcações do Z.
- **Z marca o ponto da mira para o grupo:**
  - num lugar, aparece "Aqui";
  - numa criatura hostil, aparece **"PERIGO"** em vermelho;
  - a marca dura 8 s e, fora da tela, fica presa na borda.
- **Gráficos:** duas opções novas, filtragem das texturas (anisotrópica, 16× por padrão) e qualidade das bordas (TSR). Elas tiram o serrilhado e as quinas tremidas. As de suavização (TSR, TAA, FXAA) já existiam.
- **Criação do herói:** ele não nasce mais embaixo de árvore. O palco procura uma clareira sem árvore a 12 m.

### Azhûr com mais detalhe
- **Segunda passada no modelo** (`Tools/Blender/BuildAzhur.py`):
  - cada pedra grande ganhou lascas de basalto e obsidiana, mais claras e mais escuras;
  - rachaduras de lava em zigue-zague, com galhos.
- **Peças novas:**
  - runas acesas nos punhos e nas canelas;
  - espinhos nos nós dos dedos e ao longo dos antebraços;
  - garras nos pés;
  - correntes da ombreira ao cinto;
  - placas de pedra na cintura;
  - crista de espinhos na coluna;
  - lava pingando do queixo;
  - respiro maior no vulcão das costas.
- **A cor de cada bloco agora chega à Unreal.** Antes o Blender não exportava a cor dos vértices, e o corpo inteiro tinha um tom só.
- **A pedra ganhou uma borda de calor laranja fraca.** Contra o céu vermelho, ele não vira mais só silhueta.
- São cerca de 28 mil faces, e os 11 clipes continuam iguais.

### Mapa (M) descoberto aos poucos
- **Névoa:** o mapa começa coberto, como um pergaminho escuro. Ele se abre num raio de 70 m em volta de onde você anda, com a borda suave e queimada.
- O que você descobriu fica no save do herói, com a origem de cada pedaço. Quando o mundo crescer (o continente), o que já foi visto continua no lugar certo.
- O minimapa usa o mesmo mapa, então também mostra só o descoberto.
- **Mais detalhe:**
  - grade de 200 m com letras e números na borda;
  - rosa dos ventos e barra de escala (50 a 1000 m, conforme o zoom);
  - "Explorado: N%";
  - as ruínas do Godot (menires, arcos, estátuas, altares, pilares) aparecem quando descobertas, com nome no zoom.
- **Roda do mouse com o mapa aberto:** aproxima até 6×, centrado no herói.
- A Caldeira e as criaturas vistas só aparecem nas partes descobertas; a cidade aparece sempre.

### Armas
- **Primeira pessoa (C):** a arma aparece na tela, presa à câmera, com:
  - uma pose por tipo (manoplas nas duas mãos, espada, machado, lança, cajado, arco...);
  - balanço ao andar e inércia ao girar;
  - golpes, socos e pancadas animados;
  - o coice do disparo e a defesa.
- **Golpe para trás ou sem efeito:**
  - sem alvo, o golpe sai para onde a câmera olha, então andando para trás o herói não bate de costas;
  - todo golpe de perto ganhou um rastro em arco, que muda de ângulo a cada golpe do combo;
  - os martelos ganharam uma onda de choque no chão.

### Coletáveis (como no Godot)
- **Moitas de amora** no campo aberto. Metade dá **Frutas silvestres** (2 por moita); a outra metade dá **Ervas curativas**.
- **Cogumelos vermelhos e marrons** ao pé das árvores (uma árvore em quatro), que dão **Chapéu de cogumelo**.
- A posição sai da semente do lugar, então todos os jogadores veem as mesmas.
- A até 14 m, estrelinhas verdes e douradas piscam sobre cada coletável.
- Perto, aparece **"[E] Colher …"**.
- Quem colhe pede ao servidor. A moita some para todos e volta em 5 minutos.
- Não nascem na água, na cidade nem na Caldeira.

### Domar e montar o cavalo
- Os cavalos selvagens pastam em manadas nos campos (cinco pelagens).
- **Para domar:** chegue perto com Frutas silvestres e aperte **E**.
  - O cavalo para e come uma fruta por vez.
  - Ele pede de 2 a 4 frutas, e o mesmo cavalo pede sempre o mesmo número.
  - A HUD mostra "Dar frutas silvestres (1 de 3)".
- **Domado:**
  - o cavalo fica seu ("SEU CAVALO" na barra do alvo);
  - segue você de perto e pasta quando você para;
  - ninguém consegue bater nele.
- **Montar e desmontar:** **E**.
  - Montado, o herói senta na sela (pose nova "Ride"), e o cavalo trota e galopa com o andar.
  - **Shift galopa**, a cerca de 1,75× a corrida a pé.
  - Montado, não se ataca, esquiva nem usa habilidades.
  - Quem cai sem vida desce do cavalo.
- **Assobio (X):**
  - o cavalo vem correndo; se estiver longe, aparece a 9 m atrás de você;
  - a pelagem fica no save do herói, então numa sessão nova o X chama o mesmo cavalo.
- Um cavalo por herói: domar outro troca o antigo.

## Teclas novas

| Ação | Tecla |
|---|---|
| Colher / dar fruta ao cavalo / montar / desmontar | E |
| Assobiar para o cavalo | X |
| Marcar ponto (perigo numa criatura) | Z |

## Testes

- `Tools/RunMountShots.ps1` (novo) roda no `L_TerrainProto`, com fotos em `Saved/Screenshots/montaria_*.png`. Confere:
  - 90 coletáveis a até 90 m;
  - colher uma moita (2 frutas na mochila; a moita some);
  - domar um cavalo selvagem com frutas;
  - a pelagem salva;
  - montar;
  - o galope a 1260 cm/s (a pé, a corrida é 720);
  - desmontar;
  - o assobio (de 38 m para 4 m);
  - o mapa: onde o herói está saiu da névoa, o canto longe segue coberto, e o explorado fica entre 0 e 50%.
- `Tools/RunFeatureShots.ps1` passou a fotografar também a espada em primeira pessoa.

| | |
|---|---|
| ![Moita com brilho e o E](../Images/etapa_1010b/montaria_coletavel.jpg) | ![Colhida: a moita some](../Images/etapa_1010b/montaria_colhido.jpg) |
| ![Dando frutas ao cavalo](../Images/etapa_1010b/montaria_cavalo_comendo.jpg) | ![Montado](../Images/etapa_1010b/montaria_montado.jpg) |
| ![Galope](../Images/etapa_1010b/montaria_galope.jpg) | ![Assobio](../Images/etapa_1010b/montaria_assobio.jpg) |
| ![Azhûr na fase 2](../Images/etapa_1010b/chefe_fase2_detalhe.jpg) | ![Azhûr de perto](../Images/etapa_1010b/chefe_fase2_luta_detalhe.jpg) |
| ![Mapa com a névoa](../Images/etapa_1010b/montaria_mapa.jpg) | ![Mapa com zoom](../Images/etapa_1010b/montaria_mapa_zoom.jpg) |
| ![Manoplas em primeira pessoa](../Images/etapa_1010b/novo_primeira_pessoa.jpg) | ![Espada em primeira pessoa](../Images/etapa_1010b/novo_primeira_pessoa_espada.jpg) |

## O que conferir no editor

1. Abrir a mochila (I) e ver se parou de piscar.
2. Criar um herói e ver a foto do rosto na HUD. Ela não muda ao trocar de raça.
3. Andar com a bússola: a cidade, a Caldeira e o objetivo.
4. Apertar Z num lugar e num monstro ("PERIGO"). Em dois jogadores, o outro vê.
5. Primeira pessoa (C) com manoplas, espada e arco.
6. Colher moitas e cogumelos.
7. Domar um cavalo com frutas, montar, galopar no Shift, desmontar e assobiar no X.
8. Opções de gráficos: suavização, filtragem e bordas.
9. Abrir o mapa (M), andar e ver a névoa abrindo; a roda do mouse dá zoom. Sair e entrar no mundo: o descoberto continua.

## Conhecido

- Montado, o herói ainda usa a cápsula de colisão dele, não a do cavalo. Em passagens estreitas o cavalo pode atravessar a quina de uma pedra.
- A câmera pode entrar numa moita grande (já era assim antes desta etapa).
- O cavalo não pula junto quando o herói pula montado: ele sobe junto, sem animação de pulo.
