# Barras de vida, troca de foco, chat de comandos (T) e letreiro das regiões (23/10)

## Pedidos

1. Tirar a barra de vida grande do alto da HUD; deixá-la só para os chefes.
2. A placa de vida sobre os monstros mais bonita, animada e fluida.
3. Com a marcação de foco, trocar de alvo virando o mouse rápido e forte para outro inimigo.
4. Arco e armas de longe: sem foco, sempre mira livre.
5. Tecla **T**: um chat de comandos de teste (/god, /level, /fly e outros); depois, `/tp` para as regiões (área 1, 2, 3, deserto...).
6. Ao entrar numa região ou cidade, o nome aparecer no alto com uma animação fofa, de desenho animado.
7. Prompts para o ChatGPT gerar ícones melhores para o menu e a lista do que ainda falta de imagem: ver [PROMPTS_ICONES_CHATGPT](PROMPTS_ICONES_CHATGPT.md).

## O que mudou

### 1. Barra do alto só nos chefes

- **`AVortexAimHUD::DrawTarget`:** a barra grande só aparece se o alvo for chefe (`IsBoss`): Escorpião de Vidro, Titã das Raízes e os outros chefes.
- **Os demais:** só a placa sobre a cabeça.

### 2. Placa de vida dos monstros

- **Formato:** cápsula com pontas redondas, aro dourado, sombra e brilho em cima. Vermelha nos hostis, âmbar nos animais.
- **Animações:**
  - **ao aparecer:** dá um pulinho (cresce passando do tamanho e volta);
  - **no golpe:** treme e pisca em branco;
  - **rastro claro do dano:** espera 0,35 s e escorre atrás da vida;
  - **com menos de 30% de vida:** pulsa.
- **Movimento:** a placa segue o monstro suavizada, sem tremer com a câmera e os passos.

### 3. Foco que troca com o mouse

- **Foco preso:** o inimigo marcado fica marcado enquanto estiver vivo e perto (até ~5,5 m da borda dele). Antes, a marcação pulava sozinha para o mais perto.
- **Virada rápida e forte do mouse** (~28° em ~0,1 s): o foco passa para o inimigo que ficou na frente da câmera, que não seja o atual. Depois de uma troca há uma espera de 0,35 s.
- **Detalhe técnico:** a virada é medida em `DrawFocus`; a troca é `AVortexCharacter::FlickFocus`.

### 4. Armas de longe: mira livre

- **Como já era:** a marcação e o "virar para o alvo" só existem com arma de perto (`IsMeleeWeapon`), na terceira pessoa e sem segurar a mira. Arco, besta, cajado, varinha, pistola, orbe, grimório e as outras de longe usam sempre a mira do meio da tela.
- **Nesta rodada:** confirmei no código; não precisou mudar nada.

### 5. Chat de comandos (T)

**Funcionamento** (`Framework/VortexDevChat`):
- **T** abre uma caixa embaixo à esquerda, já com `/`.
- **Enter** manda o comando, **Esc** fecha, e as **setas ↑ ↓** voltam aos comandos já usados.
- As respostas ficam na tela por 8 s, mesmo com a caixa fechada.
- Só existe fora do jogo final: a tecla e o servidor recusam no build Shipping.

| Comando | O que faz |
|---|---|
| `/help` | lista os comandos |
| `/god` | liga/desliga: a vida não desce (os números de dano continuam aparecendo) |
| `/fly` | liga/desliga: voa a 18 m/s; W vai para onde a câmera olha (olhe para cima para subir) |
| `/level N` | sobe até o nível N (1 a 100; o nível só sobe) |
| `/xp N`, `/coins N` | ganha XP ou moedas |
| `/give item [qtd]` | põe um item na mochila (id do catálogo, ex.: `/give pocao_de_vida 5`, `/give potion_red 3`) |
| `/heal` | vida e mana cheias; também levanta o herói caído |
| `/time H` | hora do dia (6 nascer, 18 pôr do sol) |
| `/tp X Y` | vai para X, Y em metros |
| `/tp 1` a `/tp 9` | vai ao centro da região: 1 Valdora, 2 Brasafenda, 3 Raikor, 4 Nivaluz, 5 Solcânion, 6 Aurivale, 7 Lodonegro, 8 Morthúria, 9 Verdalume |
| `/tp nome` | pelo nome ou pelo tipo: `deserto`, `neve`, `gelo`, `vulcao`, `lava`, `montanha`, `pantano`, `floresta`, `elfos`, `ouro`, `sombrio`, `vila`... e pelos lugares: `yggdrasil`, `dunas`, `cacadores`, `seryth`, `thorun`, `escorpiao`, `titan` |
| `/tp` | lista os lugares |
| `/pos` | onde o herói está (metros) e a região |
| `/spawn bicho [nível] [qtd]` | faz nascer criaturas a ~5 m na frente (ex.: `/spawn Goblin 5 3`) |
| `/kill [metros]` | vence os hostis em volta (30 m) |
| `/fps`, `/hud`, `/clear` | mostra o FPS, esconde a HUD, limpa o chat |

**Detalhes:**
- **Onde roda:** `/help`, `/fps`, `/hud`, `/pos` e `/clear` rodam no próprio PC. O resto vai ao servidor (`ServerDevCommand`), que responde só para quem pediu (`ClientDevReply`). No multiplayer, cada um usa os comandos no próprio herói.
- **Depois do `/tp`:** o herói flutua 4 s antes de cair, enquanto o chão de lá ganha colisão. Assim ele não atravessa o terreno.
- **`/god`:** é uma trava na vida (`UVortexAttributes::bGodMode`), não um corte no dano. Vale para todo tipo de dano (golpes, fogo, queda).

### 6. Letreiro do lugar

**`AVortexAimHUD::DrawPlaceBanner`.** Ao entrar num lugar com nome, aparece no alto da tela:
- uma faixa escura e macia que abre do meio;
- o nome em letras de desenho animado (Luckiest Guy, a mesma fonte dos números de dano), que **caem uma a uma e quicam**;
- depois de pousar, a palavra balança devagar como bandeirinha;
- estrelinhas estouram dos lados;
- embaixo vem o tipo do lugar, por exemplo "REGIÃO · CRIATURAS NÍVEL 12", "CIDADE" ou "LUGAR LENDÁRIO";
- depois de 4,6 s tudo some subindo.

**De onde vem o nome:**
- a Cidade do Santuário (dentro das muralhas);
- os lugares marcados: Yggdrasil, Acampamento das Dunas, Vila dos Caçadores, Seryth, Thorun;
- senão, a região.

**Cor:** cada região tem a sua (vermelho na Brasafenda, azul-gelo na Nivaluz, laranja no Solcânion, roxo na Morthúria...).

**Na divisa:** o nome não pisca. O lugar novo precisa durar 1 s, e o mesmo nome não volta antes de 40 s.

## Fotos

| | Antes | Depois |
|---|---|---|
| HUD em combate | [folha do dia 23 (em baixo)](../Images/nevoa_ia_dano/folha_hud.png) | [foto](../Images/barras_chat_letreiro/depois_combate.png) |
| Letreiro do lugar | não existia | [foto](../Images/barras_chat_letreiro/depois_lugar.png) |
| Chat de comandos (respostas) | não existia | [foto](../Images/barras_chat_letreiro/depois_chat.png) |
| Ícones do menu | [início antes](../Images/menu_icones/antes_inicio.png) | [início](../Images/menu_icones/depois_inicio.png), [heróis](../Images/menu_icones/depois_herois.png), [os 12 ícones](../Images/menu_icones/folha_icones.png) |

## Validação

Ver "Resultados" no fim.

## Riscos

- **Troca pelo mouse:** 28° em 0,1 s é uma virada forte; quem gira a câmera rápido de propósito, sem querer trocar, pode trocar. O ajuste é o `28.f` em `DrawFocus`.
- **Foco preso:** se o jogador quiser bater em outro que chegou mais perto, precisa virar o mouse para ele (ou se afastar 5,5 m do atual).
- **`/fly`:** a animação usada é a de cair. Encostar em água não faz nadar enquanto voa.
- **`/kill` e `/spawn`** valem para todos os jogadores do mundo; use só em teste.

## Para você testar jogando

- **Foco:** 3 goblins (`/spawn Goblin 3 3`). Bata em um, vire o mouse rápido para outro e veja a marcação pular. Com arco, não deve haver marcação.
- **Placas:** bater e ver o tremor, o brilho e o rastro; deixar um com pouca vida e ver o pulso.
- **Chefe:** `/tp escorpiao` (com `/god`) para ver a barra grande do alto.
- **Chat:** T, `/help`, `/fly`, `/tp deserto`, `/tp 4`, `/time 20`, setas ↑ ↓.
- **Letreiro:** andar da cidade para fora e entre regiões (ou usar `/tp`).

## Resultados

- `VortexEditor` compilou.
- **Suíte completa:** 27 de 27 testes passaram, entre eles combate, espada, lobo, matilha, criaturas, golpe de perto (`RunFeatureShots`), câmera e rede (entrada do cliente, movimento, lobo).
- **`RunHudShot`:** passou.
  - O letreiro "Planícies de Valdora · Região · criaturas nível 8" caiu no alto ao chegar ao vale.
  - **Chat:** `/god`, `/heal`, `/give potion_red 2` e `/nada` responderam (o último com erro em vermelho).
  - **`/god`:** a vida ficou cheia mesmo quando o teste a pôs em 1. Com o `/god` desligado, ela voltou a descer.
  - **Barra do alto:** saiu no lobo; ficou só a placa.
- **Teste da poção, que estava velho:** desde o commit `ddf4d70`, beber leva um instante, e o `RunHudShot` conferia na hora. Agora ele aceita "começou a beber".
- **Não testado automaticamente:**
  - a troca de foco pela virada do mouse;
  - `/fly`, `/tp`, `/spawn` e `/kill` em jogo;
  - a caixa de digitar do chat aparecendo. Na foto ela não sai, porque o teste roda numa janela sem foco e a caixa se fecha ao perder o foco; as respostas aparecem.
  - Teste jogando.
