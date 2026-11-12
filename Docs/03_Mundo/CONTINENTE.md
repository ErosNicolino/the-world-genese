# O continente (10/10/2026)

Situação: 🟡 primeira parte pronta, com os testes automáticos passando; espera o seu teste no editor.

Pedido: "faça o mundo inteiro, nesse tem limite que eu caio do mundo", igual ao do Godot, com rios completos, montanhas bonitas e vales.

## O que mudou

### Não há mais borda
- Antes, o chão existia só na região exportada do Godot, de cerca de 1,1 × 1,1 km. Fora dela não havia nada, e o herói caía.
- **Agora o continente inteiro do Godot existe em volta do vale.** São cerca de 7,5 × 7,5 km, com o mar em volta. O mar não acaba: em qualquer direção há fundo para nadar.
- **Ele é gerado na hora**, como no Godot (`WorldMap` + `ValleyWorld.Ground/Scatter`):
  - o relevo vem do mapa de 6 m por ponto (`Data/GodotWorld`, copiado de `data/world` do Godot), com o mesmo ruído fino por cima;
  - a 300 m da vila começa a mistura do vale com o continente, até 460 m, como no Godot;
  - na borda da região exportada as duas alturas se misturam em 40 m, sem degrau.
- Cada pedaço de 18 m é montado em outra thread e guardado. São até cerca de 16 mil pedaços na memória; depois, os mais antigos saem e voltam iguais quando você passa de novo.
- `-VortexNoWorld` volta ao vale sozinho, como antes.

### As nove regiões
Cada região tem o chão, o tom, as árvores, os enfeites e as plantas do Godot:

| Região | Chão | Árvores | Enfeites e plantas |
|---|---|---|---|
| Planícies de Valdora | campo, flores, mata | carvalho, bétula, outono, cerejeira, pinheiro no alto | moitas, pedras, troncos caídos, flores |
| Brasafenda | cinza vulcânica, pedra no alto | árvore em brasa, pinheiro de cinza | agulhas de basalto, rochas, flores de brasa |
| Picos de Raikor | grama, pedra, neve acima de 105 m | abeto da tempestade, árvore do penhasco | agulhas de ardósia, cristais, flores alpinas |
| Nivaluz | neve, gelo | pinheiro nevado, árvore de gelo | moitas de neve, espinhos de gelo, flores de geada |
| Solcânion | areia vermelha | cacto, acácia | cactos-barril, arbustos secos, ossos, arenito |
| Aurivale | grama dourada | árvore dourada, macieira | feno, abóboras, trigo, girassóis |
| Lodonegro | lama, mata | mangue, salgueiro, árvore morta | cogumelos do brejo, lanternas, juncos |
| Morthúria | solo roxo | árvore assombrada, árvore de ossos | lápides, espinheiros, flores de alma |
| Verdalume | grama élfica | árvore élfica, folha-da-lua | flores élficas, samambaia-da-lua, cristais |

- Os 25 modelos novos foram importados do `Art/World` do Godot.
- Árvores, agulhas, cactos, feno e lápides têm colisão.
- O gelo e a lava do mapa do Godot também valem: neles não nasce planta.
- Os rios e lagos do mapa do Godot viram água de verdade (dá para nadar).
- **Criaturas:**
  - fora do vale, cada região tem as criaturas do Godot (`WorldMap.Species`) e a faixa de nível dela (Valdora 1–15 … Morthúria 55–70), que sobe com a distância da vila;
  - no vale, continua tudo como antes.
- **Coletáveis:** moitas e cogumelos nascem no continente também.

### Rios completos, vales e montanhas (`Tools/World/BakeWorld.py`)
- O mapa do mundo do Vortex agora sai do mesmo processo do Godot (a arte do continente, lida da pasta do Godot sem mexer nela), com três mudanças:
  - **rios da nascente ao mar:** o riacho começa com uma bacia cerca de 3 vezes menor que a do Godot, então nasce perto das cabeceiras, nas montanhas, recebe afluentes, engrossa e sempre chega ao mar. Eram poucos trechos; agora são 67 redes de rios, e os maiores ficam mais largos;
  - **vales:** em volta de cada rio o chão desce em U largo (22% até 70 m do rio) antes da encosta. Nas montanhas isso vira cânion;
  - **montanhas:** cristas finas por cima das grandes em Raikor, Nivaluz e Brasafenda.
- A água só existe numa altura (a linha d'água do lago), então o rio corre no fundo do vale, que é cavado até ela.
- As regiões, os tons, a lava e o vulcão continuam iguais aos do Godot. As 60 pontes e 70 cabanas foram recalculadas sobre os rios novos.

| Rios do Godot | Rios do Vortex |
|---|---|
| ![Godot](../Images/continente/rios_godot.jpg) | ![Vortex](../Images/continente/rios_vortex.jpg) |

### O horizonte (`AVortexFarTerrain`)
- Uma malha de relevo distante, de 18 m por ponto, cobre até 2,6 km em volta de quem joga, com a cor do chão de cada região, a neve dos picos e o mar.
- Montanhas, o vulcão de Brasafenda, vales e a costa aparecem ao longe.
- Ela tem um buraco onde o terreno de verdade está montado e fica 2,5 m abaixo dele. Cada ponto usa a menor altura em volta, então não fura um vale perto.
- É refeita em outra thread a cada 40 m andados.
- **A névoa ficou bem mais leve** (densidade 0,07 a partir de 60 m, era 0,32 a partir de 45 m) e vai até o fim do relevo distante. O céu continua limpo.
- `-VortexNoFar` desliga o horizonte. `-VortexFarFogDensity=` e `-VortexFarFogStart=` testam outra névoa.

### Pontes, cabanas e baús (`AVortexWorldFeatures`)
- As 60 pontes e as 70 cabanas abandonadas do mapa são montadas quando alguém chega a 200 m e saem quando todos se afastam. É o mesmo desenho do Godot (`ValleyWorld.Features`), em blocos de madeira e pedra:
  - **ponte:** tábuas em arco com cravos de ferro, vigas, postes, corrimão e pilares dentro do rio;
  - **cabana:** alicerce de pedra, paredes de troncos (algumas meio caídas), janela, telhado com buraco, mesa quebrada, barril e teia.
- Têm colisão: dá para atravessar a ponte por cima do rio e entrar na cabana. Plantas e pedras não nascem em cima delas.
- **Baú em cada cabana (raro em 15%):**
  - **E** abre; a tampa gira na dobradiça e sai um brilho dourado;
  - o servidor sorteia o saque como no Godot: uma poção, materiais da região (obsidiana e ferro em Brasafenda e Solcânion; ferro e pele em Raikor e Nivaluz; musgo, asa de morcego e cogumelo em Lodonegro e Morthúria; fibra, ervas, frutas e ração no resto), às vezes um equipamento (comum 45%, incomum 32%, raro 17%, épico 6%; o baú raro sobe um degrau) e moedas;
  - aberto fica aberto no save do herói. Em grupo, cada um abre o seu.

### Marcos: Yggdrasil, vilas e covis (`AVortexLandmarks`)
- **Yggdrasil, a Árvore do Mundo** (cerca de 150 m), no coração de Verdalume:
  - o tronco e a copa são os modelos do Godot; o tronco tem colisão pela própria malha (dá para subir nas raízes);
  - ela fica sempre montada e aparece de longe;
  - em volta, a **aldeia élfica**: 8 casas em anel de frente para a árvore, o arco de entrada, o Poço da Lua e as lanternas pelo caminho e em roda;
  - uma clareira de 128 m e o caminho do arco ficam sem árvores.
- **Acampamento das Dunas** (Solcânion): carroças, tendas de mercado, torre, poço, caixotes, cercas, lampiões, praça de pedra e fogueira.
- **Vila dos Caçadores** (Verdalume): três chalés, a loja, o arco élfico, poço, banco, lanternas, praça e fogueira.
- **Covis dos chefes regionais:** mosaico em anel e oito obeliscos com runas acesas (com colisão). **Seryth, o Escorpião de Vidro** (nível 35) mora nas dunas de vidro de Solcânion e **Thorun, o Titã das Raízes** (nível 30), no bosque antigo de Verdalume. Os dois acordam quando você entra no círculo.
- As vilas e os covis montam quando alguém chega a 400 m. As peças usam as caixas de colisão do Godot.
- Tudo aparece no mapa (M) quando descoberto.

### Mapa (M)
- **O mapa agora é o continente inteiro**, a 3 m por ponto. Ele é montado em várias threads quando o jogo começa.
- O mapa ganhou:
  - curvas de nível a cada 10 m (montanhas e vales aparecem);
  - grade de 500 m;
  - o nome de cada região quando você chega perto do centro dela;
  - as 130 pontes e cabanas do Godot como marcas.
- O título mostra a região onde você está.
- A névoa cobre o continente e se abre 110 m em volta de onde você anda. O "explorado" conta só a terra.

## Testes

`Tools/RunWorldShots.ps1` (novo, também no `RunAllTests`) visita:
- as duas emendas do vale exportado com o continente (leste a 230 m e oeste a −920 m);
- o centro de cada uma das nove regiões;
- três rios e vales;
- três vistas do horizonte;
- uma ponte (o herói de pé no meio, por cima da água);
- uma cabana (abre o baú: o saque e as moedas chegam, e o baú fica marcado no save);
- a Yggdrasil, a aldeia élfica, as duas vilas e o covil de Seryth.

Em cada lugar, o herói espera o chão montar, fica de pé e tira a foto (`Saved/Screenshots/mundo_*.png`). Resultado: os 22 lugares de pé, com a região e o nível certos, e o mapa com a névoa.

| | |
|---|---|
| ![Emenda leste](../Images/continente/mundo_emenda_leste.jpg) | ![Valdora](../Images/continente/mundo_valdora.jpg) |
| ![Brasafenda](../Images/continente/mundo_brasafenda.jpg) | ![Raikor](../Images/continente/mundo_raikor.jpg) |
| ![Nivaluz](../Images/continente/mundo_nivaluz.jpg) | ![Solcânion](../Images/continente/mundo_solcanion.jpg) |
| ![Aurivale](../Images/continente/mundo_aurivale.jpg) | ![Morthúria](../Images/continente/mundo_morthuria.jpg) |
| ![Verdalume](../Images/continente/mundo_verdalume.jpg) | ![O mapa do continente](../Images/continente/mundo_mapa.jpg) |
| ![O vulcão no horizonte](../Images/continente/mundo_horizonte_vulcao.jpg) | ![Rio no vale de Raikor](../Images/continente/mundo_vale_raikor.jpg) |
| ![Na ponte](../Images/continente/mundo_ponte.jpg) | ![O baú aberto](../Images/continente/mundo_bau.jpg) |
| ![Yggdrasil e a aldeia élfica](../Images/continente/mundo_aldeia_elfica.jpg) | ![Acampamento das Dunas](../Images/continente/mundo_acampamento_dunas.jpg) |
| ![Vila dos Caçadores](../Images/continente/mundo_vila_cacadores.jpg) | ![O covil de Seryth](../Images/continente/mundo_covil_seryth.jpg) |

## O que conferir no editor

1. Sair do vale para qualquer lado e andar (ou galopar no cavalo) até outra região. Não pode haver borda nem buraco.
2. As emendas: a leste da cidade (uns 200 m) e a oeste (uns 900 m). O chão sobe ou desce em rampa, sem paredão.
3. Cada região com o seu chão, as suas árvores e as suas criaturas.
4. O mapa (M): a névoa abrindo e os nomes das regiões aparecendo.
5. Nadar no mar e nos rios. Seguir um rio do alto das montanhas até o mar.
6. O horizonte: as montanhas e o vulcão ao longe, a névoa mais leve. Ver se em algum lugar a malha distante aparece por cima do chão perto (não deveria).
7. Uma ponte (atravessar a pé e a cavalo) e uma cabana (abrir o baú com E).
8. A Yggdrasil (subir nas raízes), a aldeia élfica, as duas vilas e os covis de Seryth e Thorun.

## Falta (próximas partes)

- **Detalhes:** a fumaça do vulcão (o cone já está no relevo), a versão de longe da Yggdrasil (a silhueta a quilômetros, como no Godot) e os moradores das vilas regionais.
