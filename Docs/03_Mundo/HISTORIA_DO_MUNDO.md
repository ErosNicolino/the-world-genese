# A história do mundo de Vortex

Esta é a base das falas, das missões e dos chefes. Ela parte do que o Godot já tinha: a Caldeira do Meteoro e Azhûr, os cristais que brilham à noite, a Fonte de Jade, o Estádio do Despertar, a Yggdrasil, as nove regiões do continente, Seryth nas dunas e Thorun na mata. A partir disso, ela amarra tudo numa história só. Os dados ficam em `Data/Story/vortex_story.json` (lore, capítulos e missões do Vortex). As missões e os diálogos do Godot são copiados pelo `Tools/BuildStoryData.py`.

## O mundo

**Gênese** é o continente: cerca de 7,5 × 7,5 km cercados de mar. No centro dele está a **Yggdrasil**, a árvore que segura o mundo. As raízes dela correm por baixo das nove regiões e carregam a *seiva da gênese*, que é o que faz a grama crescer, a água correr e os bichos nascerem mansos.

Há muito tempo, o céu sobre a Yggdrasil se abriu num redemoinho de luz: o **Vórtice**. Ninguém sabe se ele é uma ferida ou uma porta. Ele puxa para si tudo o que é vivo, devagar, como a maré. Para conter esse puxão, os antigos ergueram **nove Selos**, um em cada região. Cada Selo tem um **Guardião**, uma criatura enorme ligada à terra daquele lugar. Enquanto os Guardiões dormem em paz, o Vórtice fica fechado.

## O que aconteceu há vinte anos

Uma estrela caiu a leste do Vale do Santuário: a **Estrela Partida**. Na verdade era um pedaço do próprio Vórtice, que tinha se soltado. Ela abriu a **Caldeira do Meteoro** e caiu exatamente sobre o primeiro Selo. O Guardião daquele Selo, **Azhûr**, um colosso de basalto e runas, acordou queimando por dentro. Desde então:

- os **cristais** do vale brilham à noite, porque a seiva está sendo puxada para cima;
- os bichos ficaram **agressivos**: as gotas de jade, os cogumelos errantes, os lobos da bruma e os golens musgosos;
- os outros Guardiões começaram a sonhar mal, um por um. O Vórtice está acordando.

## Os Despertos

Quando o Vórtice puxa, ele também **devolve**. Às vezes uma alma atravessa e acorda no **Estádio do Despertar**, um coliseu antigo construído em volta de um dos poços de seiva. São os **Despertos**: o jogador e os amigos dele. Eles chegam sem memória e escolhem a arma que a seiva lhes oferece nos pedestais. Por terem atravessado o Vórtice, eles são os únicos que conseguem **tocar um Guardião sem enlouquecer**.

A **Fonte de Jade**, no Santuário, é um broto da própria Yggdrasil. Ela cura e é para lá que um Desperto caído volta. É por isso que a vila foi construída em volta dela.

## As nove regiões e os seus Guardiões

| Região | Níveis | O lugar | Guardião (Selo) | Situação |
|---|---|---|---|---|
| **Valdora** (o Vale do Santuário) | 1–15 | Campos, lagos, mata e a vila | **Azhûr, o Colosso da Caldeira** | Desperto e ferido pela Estrela Partida: o primeiro inimigo |
| **Aurivale** | 10–22 | Planícies douradas de trigo e colmeias | **Vespera, a Rainha do Enxame** | Inquieta; enxames atacam as fazendas |
| **Verdalume** | 15–30 | Mata fechada e luminosa | **Thorun, o Titã das Raízes** | A raiz da Yggdrasil apodrece: Thorun ataca quem entra |
| **Solcânion** | 20–35 | Dunas e cânions de vidro | **Seryth, o Escorpião de Vidro** | Mergulha na areia e caça caravanas |
| **Lodonegro** | 25–40 | Pântano escuro, névoa e sapos-gigantes | **A Mãe-Lodo** | Dorme; o brejo cresce sobre as estradas |
| **Picos de Raikor** | 30–45 | Montanhas de tempestade | **Raikor, a Ave-Trovão** | Os raios caem sem nuvem |
| **Brasafenda** | 40–55 | Rachaduras de lava e o vulcão | **O Forjador Rubro** | Forja armas para o Vórtice |
| **Nivaluz** | 45–60 | Gelo e auroras | **Aurora, a Que Congela a Luz** | O céu do norte parou de mudar |
| **Morthúria** | 55–70 | Terra dos mortos-vivos e das ruínas | **O Rei Sem Sombra** | Foi o primeiro a ouvir o Vórtice e quer abri-lo |

No fim de tudo, com os nove Selos reacendidos, os Despertos sobem pela Yggdrasil até o **Vórtice**.

## A jornada (capítulos)

1. **O Despertar** (Valdora, jogável agora): acordar no Estádio, chegar ao Santuário, provar o valor à Capitã Rhea, descobrir com a Ilsa e a Nicolly por que os cristais brilham, ver a Caldeira e derrotar **Azhûr**. Termina com o **Coração de Azhûr** reacendendo o primeiro Selo. A Nicolly abre o mapa do continente.
2. **Ouro e Enxame** (Aurivale): as colmeias e a Rainha Vespera. *(o continente ainda não está no Vortex)*
3. **O Coração da Mata** (Verdalume): a Rastreadora Lyra e **Thorun**.
4. **Areia de Vidro** (Solcânion): a Guia Amal, o mercador Sahil e **Seryth**.
5. **O Brejo Que Respira** (Lodonegro).
6. **O Trovão sem Nuvem** (Picos de Raikor).
7. **A Forja do Vórtice** (Brasafenda).
8. **A Luz Parada** (Nivaluz).
9. **O Rei Sem Sombra** (Morthúria).
10. **O Vórtice** (a copa da Yggdrasil).

## A abertura em quadrinhos

A HQ de abertura (5 páginas de 4 quadros, com a narração e os prompts para gerar as páginas) está em [ABERTURA_QUADRINHOS](ABERTURA_QUADRINHOS.md).

## Quem é quem no Santuário

- **Rhea**, Capitã da Guarda: dura e justa. É ela quem dá a linha principal até a Caldeira.
- **Ilsa**, herbalista: cuida das ervas e da Fonte, e entende a seiva melhor que ninguém.
- **Bram**, ferreiro: quer sucata e obsidiana para forjar armas que aguentem o Vórtice.
- **Nicolly**, cartógrafa: mapeia os cristais e, depois de Azhûr, o continente inteiro.
- **Lary** e **Juh**, moradoras: o coração da vila, com pedidos pequenos que valem muito.
- **Ducci**, mercador mago ambulante: compra amostras encantadas e vende o que ninguém mais tem.
- **Garrick** (armeiro), **Rosa** (alfaiate) e **Marta** (taverneira da Taverna do Javali Dourado).
- **Julia**, moradora da praça, com o cachorrinho que a segue para todo lado.
