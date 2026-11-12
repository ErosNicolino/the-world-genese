# Comandos de teste (chat do jogo)

Abra o chat com a tecla **T** (a barra / já vem escrita), digite o comando e aperte **Enter**. Os nomes em português e os antigos em inglês
funcionam igual. `/ajuda` mostra a lista dentro do jogo. Atualizado em 12/11/2026.

## Herói

| Comando | O que faz | Exemplo |
|---|---|---|
| `/deus` (ou `/god`) | liga e desliga: a vida não desce | `/deus` |
| `/voar` (ou `/fly`) | liga e desliga o voo: W vai para onde a câmera olha | `/voar` |
| `/curar` (ou `/heal`) | vida e mana cheias; também levanta do chão | `/curar` |
| `/nivel N` (ou `/level`) | sobe até o nível N (1 a 100; só sobe) | `/nivel 20` |
| `/xp N` | ganha N de experiência | `/xp 5000` |
| `/moedas N` (ou `/coins`) | ganha N moedas | `/moedas 10000` |
| `/item id [qtd]` (ou `/give`) | põe o item na mochila | `/item pocao_de_vida 5` |
| `/itens [palavra]` | procura o id dos itens (mostra até 40) | `/itens espada` |

## Lugar

| Comando | O que faz | Exemplo |
|---|---|---|
| `/ir X Y` (ou `/tp`) | vai para X, Y em metros | `/ir 480 -300` |
| `/ir 1..9` ou `/ir nome` | vai para uma região ou lugar marcante | `/ir deserto`, `/ir yggdrasil` |
| `/ir` | lista os lugares | `/ir` |
| `/onde` (ou `/pos`) | onde você está e a região | `/onde` |
| `/poi tipo` | vai às ruínas, torre, santuário, mina, cemitério, caravana ou cais mais perto | `/poi mina` |
| `/portal [E-S]` | abre um portal de dungeon na sua frente | `/portal C` |

## Criaturas

| Comando | O que faz | Exemplo |
|---|---|---|
| `/spawn nome [nível] [qtd]` (ou `/criar`) | cria bichos na sua frente; o nome é o do jogo, sem acento, e um pedaço basta | `/spawn gota de brasa 5 3` |
| `/spawn lista` | todos os nomes | `/spawn lista` |
| `/spawn fada [qtd]` | chama fadas na sua frente | `/spawn fada 3` |
| `/matar [metros]` (ou `/kill`) | vence os hostis em volta (30 m sem número) | `/matar 50` |

## Tempo e clima

| Comando | O que faz | Exemplo |
|---|---|---|
| `/hora H` (ou `/time`) | muda a hora (6 nascer, 12 meio-dia, 18 pôr do sol) | `/hora 20` |
| `/manha` `/meiodia` `/tarde` `/noite` | atalhos: 8, 12, 17 e 22 horas | `/noite` |
| `/relogio N` | velocidade do relógio do dia (1 normal, 0 parado, 60 bem rápido) | `/relogio 60` |
| `/dia N` | vai para o dia N (a estação muda a cada 3 dias) | `/dia 10` |
| `/estacao nome` | primavera, verao, outono ou inverno | `/estacao inverno` |
| `/clima tipo` | chuva, tempestade, neve, nevasca, neblina, nublado ou limpo, na hora | `/clima tempestade` |
| `/clima normal` | volta ao clima sorteado | `/clima normal` |
| `/velocidade N` | o jogo todo mais lento ou mais rápido (0,1 a 5; 1 normal) | `/velocidade 0.3` |

## Tela

| Comando | O que faz |
|---|---|
| `/fps` | mostra o FPS |
| `/hud` | esconde e mostra a HUD (para fotos) |
| `/limpar` (ou `/clear`) | limpa o chat |

## Observações

- `/clima` muda só no seu PC: o clima é desenhado por cada máquina. Num jogo em grupo, cada um digita o seu.
- `/velocidade` e `/relogio` valem para todos da partida.
- No inverno, a chuva sorteada vira neve.
- Fora do jogo, o clima também pode ser forçado ao abrir: `-VortexWeather=Rain` (ou `Storm`, `Snow`, `Blizzard`, `Fog`,
  `Cloudy`, `Clear`) no atalho; `-VortexNoWeather` desliga.
