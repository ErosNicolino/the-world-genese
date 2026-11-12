# Cliques nas janelas, habilidades por nível e água rasa (23/10, noite)

## Pedidos

1. "Não consigo interagir com os balões de fala dos NPC nem no inventário; fica atacando quando clico com o mouse."
2. "As skills, quando chego no nível 10, já ficam todas ativas: queria que só a primeira aparecesse, o restante com cadeado, liberando de 5 em 5 níveis."
3. "A água não tem física: quando entro ou pulo nela não tem toque."
4. Uma opinião sincera sobre o inventário e as outras telas.

## 1. Cliques que viravam ataque

**Causa:**
- O chat de comandos (tecla T) fica na tela depois da primeira vez que é aberto, para mostrar as respostas.
- Ele estava **clicável na tela inteira** e por cima das janelas (inventário e diálogo).
- Assim, os cliques caíam nele, não chegavam à janela e iam parar no jogo, virando golpe.

**Correção:**
- **O chat:** é invisível ao mouse; só a caixa de digitar, quando aberta, pega clique. Ele também passou para trás das janelas, porque as linhas dele cobriam o inventário.
- **Proteção extra** (`AVortexCharacter::IsMouseOnWindow`): com **qualquer** janela aberta, o clique não ataca e o mouse não gira a câmera. Antes isso valia só para o inventário; agora vale também para diálogo, loja, janela de outro jogador e chat.

## 2. Habilidades da classe por nível

| Tecla | Libera no nível |
|---|---|
| 1 | 10 (o Despertar) |
| 2 | 15 |
| 3 | 20 |
| 4 | 25 |

- **Barra:** as trancadas aparecem bem apagadas, com um **cadeado** dourado e "Nv 15", "Nv 20" ou "Nv 25".
- **Janela, aba Classes:** "TRANCADA, libera no nível N", com o ícone apagado.
- **Apertando a tecla de uma trancada:** aviso "Nome: libera no nível N".
- **Servidor:** confere o nível também, então não dá para lançar uma trancada.
- **Detalhe técnico:** `UVortexClassSkills::UnlockLevel` e `IsUnlocked`.
- **Teste:** `RunClassShots` agora confere que no nível 10 só a 1ª está liberada e que no nível 25 as 4 estão. Depois lança as 4 de todas as classes. Passou.
- **Foto:** [aba Classes no nível 10, com a barra trancada](../Images/cliques_habilidades/aba_classes_nivel10.png).

## 3. Água rasa

**Antes:** respingo, ondas e som só existiam **nadando** (água com mais de ~1 m). Na água rasa das margens, onde se anda quase sempre, não acontecia nada.

**Agora** (`AVortexCharacter::UpdateSounds` e `Landed`, `UVortexCharacterMovementComponent::GetMaxSpeed`):
- **Entrar andando:** um respingo pequeno.
- **Cada passo:** uma onda no pé, gotinhas e o som de água. Correndo, mais gotas.
- **Parado:** um anel de onda de vez em quando.
- **Pulo que cai na água:** respingo do tamanho da queda (o "tchibum"), no lugar do som de chão.
- **Velocidade:**
  - com a água pela canela (mais de 30 cm), anda mais devagar: de 85% a 65% da velocidade, até a água chegar no nado;
  - o cálculo é igual no cliente e no servidor, sem tranco na rede.

**Testes:**
- `RunFeatureShots` ("entrou nadando depois da queda") e `RunMovementSmoke`: passaram.
- **Não foi testado automaticamente:** o efeito visual da água rasa. Teste jogando.

## 4. Opinião sobre as telas

Fotos: [mochila](../Images/cliques_habilidades/mochila.png), [herói](../Images/cliques_habilidades/heroi.png), [técnicas](../Images/cliques_habilidades/tecnicas.png).

**O que está bom:**
- **Estilo coerente:** azul-noite e dourado.
- **Organização:** as abas estão bem separadas, o rodapé ensina os atalhos e as informações de combate são completas.
- **Técnicas:** é a melhor tela. Clara, com cartões, e mostra quando cada técnica libera.

**O que não está bom (sincero):**
- **Mochila:**
  - **Grade vazia:** os 30 espaços vazios dominam a tela, e o painel do item fica vazio até clicar.
  - **Sem o herói:** não aparece o herói vestido, que é o centro do inventário na maioria dos RPGs. Os espaços do corpo usam todos o mesmo ícone de camisa.
  - **Itens sem destaque:** a raridade não aparece na moldura dos itens (comum, raro, épico).
  - **Sem comparação:** não compara o item com o equipado.
- **Herói:**
  - **Texto demais e letra pequena:** a 720p as explicações dos atributos ficam ilegíveis.
  - **Rolagem:** 12 atributos numa lista com rolagem cansam. Daria para agrupar em 3 blocos (ataque, defesa, magia).
  - **Retrato da raça:** bonito, mas não é o seu herói.
- **Classes:**
  - **Pequena:** a coluna da sua classe fica espremida, e as habilidades aparecem quase no fim, com rolagem.
  - **Muitos nomes juntos:** a lista "Classes do mundo" é um paredão de nomes.
- **Geral:**
  - **Desperdício de espaço:** a janela ocupa o centro da tela, mas a letra é pequena e quase tudo tem o mesmo peso visual.
  - **Moldura demais:** tudo tem moldura dourada, e isso fica "carregado".
  - **Estilo misturado:** as janelas não usam a letra de desenho animado nova (títulos e números), que a HUD, o estádio e o letreiro já usam.

**O que eu faria** (precisa do seu OK, é uma mudança grande):
- **Mochila:**
  - o herói 3D no meio (o retrato que gira), com os espaços do corpo em volta;
  - grade menor, com molduras na cor da raridade;
  - o item mostrado ao passar o mouse, já comparando com o equipado.
- **Herói:**
  - os atributos em 3 grupos com letra maior;
  - os números do combate num resumo curto;
  - a explicação só ao passar o mouse.
- **Classes:** sua classe em destaque com as 4 habilidades grandes (o cadeado no lugar); as outras classes numa aba própria.
- **Todas:** títulos e números na letra de desenho animado, menos molduras, mais espaço.
