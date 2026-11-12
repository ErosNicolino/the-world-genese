# Travamentos, miragem da Yggdrasil, nome do jogo e o coliseu (10/10/2026)

Situação: 🟡 pronto, com os testes automáticos passando; espera o seu teste no editor.

## Por que o jogo estava travando

A média de FPS já era boa (100 a 130). O problema eram **quadros longos**: o jogo parava de 0,1 s a 0,7 s de tempos em tempos. Para achar os culpados, criei um passeio de desempenho de verdade e um cronômetro por sistema.

- **O passeio:** `Tools\RunRoamBench.ps1` sai da cidade a galope para o leste por 60 s, atravessa a emenda do vale com o continente e passa perto da Caldeira, com criaturas, HUD e tudo ligado.
- **O cronômetro:** `VortexProbe` mede cada sistema. O passeio escreve o tempo total de cada um, o pior quadro e quantas vezes passou de 8 ms.

Os culpados, do maior para o menor:

| Sistema | Pior quadro antes | O que acontecia | O que mudou | Pior quadro agora |
|---|---|---|---|---|
| **Criaturas** | **677 ms** | Na entrada, os ~30 mil postos do continente eram sorteados de uma vez. Depois, todos eram conferidos a cada meio segundo. A malha e as animações de cada espécie carregavam do disco na hora em que o bicho nascia: o Colosso de Runas sozinho travava 620 ms. | Os postos são sorteados por quadra de 72 m quando alguém chega perto, e só as quadras em volta dos jogadores são conferidas. As malhas e as animações de todas as espécies, chefes incluídos, carregam em segundo plano na entrada. | **4,8 ms** |
| **Névoa do mapa** | 18 ms, todo segundo | Cada pedaço descoberto refazia o mapa inteiro (2514 × 2514) e o reenviava à placa de vídeo, 25 MB por segundo. | Só o retângulo que mudou é refeito e enviado. | **0,3 ms** |
| **Relevo distante** | 15 ms, a cada 40 m | A malha do horizonte (85 mil pontos) era trocada inteira. | A malha virou blocos fixos de 576 m, guardados. Andar só refaz os 2 a 4 blocos do buraco do terreno próximo e põe ou tira blocos na borda. | **0,9 ms** |
| **Lava** | 56 ms | As poças da Caldeira (malha, pedras, fumaça e luz) eram montadas todas no mesmo quadro. | Uma poça por quadro. | **15 ms** |
| **Sons** | pequeno | Cada efeito carregava do disco na primeira vez que tocava. | Todos carregam em segundo plano ao entrar no mundo. | — |

### Resultado do passeio

| | Antes | Agora |
|---|---|---|
| Pior quadro, depois da carga inicial | 686 ms | **55 ms** |
| Pior 1% dos quadros, depois da carga inicial | ~16 FPS | **32 FPS** |
| Quadros acima de 50 ms no passeio inteiro | 12 | 11 a 15, todos nos 6 primeiros segundos |
| Média | 115 FPS | ~100 FPS |

- **A média caiu um pouco no teste** porque agora o horizonte e as criaturas nascem enquanto se anda. A diferença é pequena e varia de uma rodada para outra.
- **Os primeiros 6 segundos** ainda têm paradas de 0,2 s a 2 s, e o tamanho muda de uma rodada para outra. É a carga inicial: as espécies e os sons carregando, e o editor preparando as malhas na primeira vez que as usa ("Preparing Animation Sequences"), com o aviso de pouca memória livre. No jogo de verdade, boa parte disso acontece **atrás da tela de carregamento**. Num jogo empacotado, essa preparação do editor nem existe.
- **O que ainda pesa:** o terreno perto de quem anda, quando se vai a galope (até ~15 ms em 40 quadros de 60 s). Fica para a etapa de desempenho com 8 jogadores.

## Miragem da Yggdrasil

Sim, tem como. Vista de longe, a Árvore do Mundo agora aparece como **miragem**:

- é uma cópia da árvore, maior, que paira um pouco acima do chão;
- é translúcida e brilha em tons de céu embaixo e ouro em cima;
- faixas de ar quente sobem pelo tronco, e a silhueta ondula de lado (mais no alto que na base) e sobe e desce devagar;
- a base some no ar, como uma miragem sobre o horizonte.

A miragem começa a aparecer a **800 m** e fica inteira a partir de **1,5 km**. Perto, ela se apaga e fica só a árvore de verdade. Depois de 2,3 km, só a miragem aparece: a névoa termina antes disso, e sem ela a árvore apareceria nítida demais. Ela não recebe névoa, então segue visível acima do relevo distante.

- Material: `M_YggMirage` (`Tools/BuildMirage.py`).
- Código: `AVortexLandmarks`, a parte da Yggdrasil.

## O nome do jogo: The World Gêneses

- **Menu inicial:** "THE WORLD" em dourado sobre **"GÊNESES"** grande, no lugar de "VORTEX".
- **Tela de carregamento:** o emblema do anel de runas agora é um **G**.
- **Janela do jogo:** o título do projeto agora é "The World Gêneses".
- **Textos:** as mensagens que diziam "ainda não chegou ao Vortex" foram trocadas.

**O que continua "Vortex":** os nomes internos do código, das pastas e dos saves (`Vortex_Player_…`). O jogador não os vê. Trocá-los apagaria os saves atuais e quebraria os testes. Se quiser, dá para renomear tudo depois.

## O coliseu cheio de plantas

O Estádio do Despertar fica longe, numa área separada do mapa (a 4 km da vila). O continente procedural, que agora vai até lá, plantava árvores e plantas élficas dentro da arena.

- Agora o continente não existe a menos de 160 m do estádio: sem chão, plantas nem criaturas.
- A foto nova da arena (`RunStadiumShot`) mostra a areia limpa.

## Testes

- **`RunRoamBench` (novo):** o passeio de desempenho, com o tempo de cada sistema e os números "depois de 8 s".
- **`RunWorldShots`:** ganhou a foto da miragem a 1,5 km (`mundo_miragem_15km.png`).
- **`RunStadiumShot`:** a arena e a arquibancada sem plantas.
- **`RunAllTests`:** 25 de 26 passaram na primeira rodada. "Rede: movimento" falhou: o cliente desse teste roda no mesmo PC e carregava as ~300 peças das espécies e dos sons, roubando processador do servidor. A carga em segundo plano agora só acontece nos mapas com o continente. Com isso, o teste voltou a passar.

| | |
|---|---|
| ![Miragem da Yggdrasil a 1,5 km](../Images/etapa_1010d/miragem.jpg) | ![Arena limpa](../Images/etapa_1010d/estadio.jpg) |
| ![Menu com o nome novo](../Images/etapa_1010d/menu.jpg) | ![Carregando](../Images/etapa_1010d/carregando.jpg) |

## O que conferir no editor

1. **Andar e galopar** pelo vale e pelo continente: as travadas de meio segundo devem ter sumido. O começo pode ainda engasgar um pouco atrás da tela de carregamento.
2. **Da Planície de Valdora ou de Morthúria, olhar para o leste:** a Yggdrasil como miragem no horizonte. Chegar perto: ela vira a árvore de verdade.
3. **O menu inicial e a tela de carregamento** com o nome novo.
4. **O Estádio do Despertar** sem plantas.

## Como voltar atrás

Tudo está no commit desta etapa. A miragem some se apagar `/Game/Vortex/Sky/M_YggMirage`: o código só a cria se o material existir.
