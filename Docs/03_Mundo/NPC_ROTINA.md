# Moradores com rotina, ofícios, reações, passantes e voz (08/11)

O pedido: deixar a vila viva, como Stardew Valley, Skyrim, Zelda e Red Dead. Antes, só a Julia andava; os outros ficavam
o dia inteiro parados no posto, sem animação de trabalho, sem reagir ao herói e falando só por texto.

![O dia da vila: 9h na praça, 9h na forja, 12h30 na taverna, 17h30 na praça, 23h vazia](../Images/npc_rotina_dia.png)

## Rotina de dia e noite

Cada morador tem horários no `Data/Population/npc_schedule.json` (ajustar não precisa compilar).

| Papel | Quem | O dia |
|---|---|---|
| loja | Bram, Garrick, Ilsa, Tomás, Edda, João | 6h no serviço, 12h almoço na taverna, 13h serviço, 19h taverna, 21h casa |
| loja_tarde | Rosa, Nicolly, Ducci, Lexy | 8h serviço, 13h banco, 14h serviço, 20h praça, 22h casa |
| taverneira | Marta | 7h à 1h na taverna |
| guarda | os 6 guardas | sempre no posto, de vigia (não dormem) |
| capita | Rhea | vigia, almoço na taverna, casa às 22h |
| capela | Irmã Célia | reza na capela, banco ao meio-dia, casa às 21h |
| morador | Lary, Bento | praça, banco, taverna, passeio, banco, taverna, casa |
| julia | Julia | a volta dela pela praça, banco, praça à noite, casa às 22h |
| passante | 10 moradores sorteados | passeio das 7h às 21h |
| crianca | 6 crianças | passeio e praça das 8h às 19h |

- **Casa:** cada morador vai até a porta da casa mais perto do posto dele (os passantes têm a sua) e entra: some até a
  hora de sair. Quem está dormindo não aparece, não conversa e não vende: as lojas fecham à noite.
- **Banco:** ele senta de verdade (a animação de sentar). São 12 lugares em 6 bancos; se o lugar estiver ocupado, fica
  em pé ali perto conversando.
- **Passeio:** troca de lugar a cada 40 a 90 s (a praça, os bancos, o quadro de missões, as portas das lojas).
- **Quando aparece:** o morador que estava longe do herói surge já onde a hora manda (sem atravessar a vila na frente
  dele).

## Caminhos

`Tools/Town/BuildNpcNav.py` gera a malha `Data/Population/vila_nav.json`:
- uma grade de 0,5 m com onde dá para andar (as caixas de colisão de cada peça na altura de uma pessoa, um pouco mais
  largas, e as portas fechadas das casas);
- os lugares de cada rotina (portas das casas, o salão da taverna e da capela, os bancos, a roda da fonte).

O morador acha o caminho nessa grade (A*), corta as curvas onde dá e só abre espaço para os outros moradores.
- **Longe** (atravessar a vila): anda mais rápido.
- **Preso:** refaz o caminho; preso de novo várias vezes, aparece no lugar.
- **Lugar ocupado:** chega a até 2,5 m quando o ponto exato tem gente.

![A malha de caminhos: cinza anda; vermelho, as portas das casas; azul e verde, porta e salão das construções; amarelo, os bancos](../Images/npc_malha_caminhos.png)

## Ofícios (animações novas)

Feitas no Blender por camadas, em laço, com a mesma ferramenta dos poderes (`Tools/Blender/MakePowerAnims.py --npc`,
lista em `Tools/Blender/npc_anims_hand.py`; os cotovelos dobram agora). Importadas com
`VORTEX_ANIMS=npc Tools/ImportPowerAnims.py`.

| Clipe | Quem |
|---|---|
| Npc_Martelar (bate o martelo na bigorna) | Bram |
| Npc_Balcao (apoiado no balcão, olha em volta) | Garrick, Marta |
| Npc_Costurar | Rosa, Lexy |
| Npc_Mexer (o caldeirão) | Ilsa |
| Npc_Ler (o livro, vira a página) | Nicolly |
| Npc_Conversar (gesticula) | Ducci e todos na praça e na taverna |
| Npc_Sovar (a massa) | Tomás |
| Npc_Serrar | Edda |
| Npc_Escovar (o cavalo) | João |
| Npc_Rezar | Irmã Célia |
| Npc_Vigiar (olha para um lado e para o outro) | os guardas e a Rhea |

Também usa os clipes que já existiam: sentar no banco, beber (no almoço), acenar, saudar e comemorar.

![Algumas poses dos ofícios no Blender](../Images/npc_oficios_poses.png)

## Reação ao herói

- **A 4,5 m:** quem está parado vira para o herói.
- **A 3 m:** para o serviço (o guarda não para) e volta depois que ele se afasta.
- **Cumprimento:** uma vez a cada 90 s, com o som de voz. O guarda bate continência, a criança comemora, os outros acenam.
- **Falas acima da cabeça:** mudam com a hora (manhã, tarde, noite) e com o tipo (todos, guarda, criança, passante),
  além das falas próprias de cada um. Em `npc_schedule.json`, "falas".

## Passantes

10 adultos e 6 crianças, sorteados sempre do mesmo jeito (a semente no arquivo):
- **Visual:** nome, cabelo, roupa e cor do cabelo saem do catálogo do herói.
- **Crianças:** menores (72%) e correm quando passeiam.
- **Na tela:** aparecem como "Morador" ou "Criança da vila". Não têm diálogo próprio: uma fala curta.

## Voz

`Tools/MakeNpcVoices.py` sintetiza murmúrios de vogal em quatro tons (grave, médio, agudo e criança), quatro de cada:
`Data/Audio/npc_voz_*.wav`, importados com `Tools/ImportExtraSfx.py`. Tocam:
- no diálogo, a cada 6 letras, no tom do morador (no lugar do bip de antes);
- no cumprimento.

O tom de cada um está em `npc_schedule.json` ("voz").

## Teste

`Tools/RunNpcShots.ps1` (cerca de 6 min) fotografa e confere quatro horas:

| Hora | Resultado |
|---|---|
| 9h | 35 de 38 chegaram |
| 12h30 | 34 de 36, todos na taverna |
| 17h30 | 37 de 38, 2 sentados |
| 23h | 31 dormindo (só os guardas e a taverneira acordados) |

Também confere os cumprimentos. Fotos em `Saved/Screenshots/npc_*.png`.

`-VortexNoRoutines` desliga tudo isso (os moradores voltam a ficar só no posto, como antes).

## Não conferido

- **Animações de perto:** a forja é escura e as fotos são do alto; de perto, no jogo, falta ver.
- **Voz:** não ouvi.
- **Lojas fechadas à noite:** um herói que chega de madrugada não acha o lojista. Se não quiser assim, é só mudar o
  horário de "casa" no arquivo.
- **Testes antigos dos moradores** (a Lary no `RunCreatureSmoke`): não rodei; eles podem estranhar a Lary andando.
