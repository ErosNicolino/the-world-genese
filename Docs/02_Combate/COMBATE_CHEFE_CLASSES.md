# Combate de perto, matilha, chefe novo, classes e água (10/10/2026)

Situação: 🟡 pronto, com os testes automáticos passando; espera o seu teste no editor.

Esta etapa cobre a lista pedida depois da última:

1. golpe de perto sem mira e com alcance de verdade;
2. atordoamento só de vez em quando;
3. o chefe inicial refeito, com barra em 3 fases, céu vermelho e lava;
4. matilha de lobos inteligente;
5. conferir o multiplayer;
6. excluir mundos (até 5);
7. o Soco Sério das manoplas;
8. as classes do Godot com a cena das 6 cartas e os poderes;
9. primeira pessoa no C;
10. pulo na água e nado.

## O que mudou

### Golpe de perto sem mira
- Com espada, manoplas, machado, lança e as outras armas de perto, a mira some da tela.
- **O golpe vai no que está à frente do herói.** Ao clicar, ele vira sozinho para o inimigo mais perto num leque de ~80° para cada lado de onde você anda (ou para onde está virado). Se o inimigo está um pouco longe, ele dá um passo curto até o alcance.
- **O alcance agora conta até a borda do corpo do monstro**, não até o centro. Um golem largo é acertado de onde se acerta um coelho, e você não precisa grudar nele. O leque também abre para bichos grandes.
- Segurando F (defesa e mira) e em primeira pessoa, volta a mirar pela câmera.
- Os especiais (o corte à distância da espada, o Soco Sério) seguem a direção da câmera.

### Atordoamento e "encolher"
- Antes, quase todo golpe atordoava e todo golpe fazia o bicho encolher, então ele nunca atacava.
- Agora:
  - só golpes pesados (0,3 s ou mais) atordoam;
  - depois de um atordoamento, a criatura fica 3 s firme;
  - ela encolhe no máximo a cada 2,2 s;
  - o empurrão só acontece no crítico ou quando atordoa (antes afastava o bicho a cada golpe).
- **A defesa perfeita sempre atordoa**, como antes.

### Matilha de lobos
- **Cerco:** o alfa encara o herói pela frente, chamando a atenção, e os outros se espalham pelos lados e por trás. Cada um tem o seu ângulo e a roda vai fechando enquanto eles esperam a vez.
- **Oportunistas:** quem está nas costas do herói ataca logo; os outros esperam (2 a 3 s). Com 4 ou mais lobos (ou no frenesi do uivo), **dois atacam juntos**, em pinça.
- **Bate e corre:** depois da mordida, o lobo sai de lado.
- **Esquiva:** quando você começa um golpe com um lobo na frente, às vezes ele pula para trás.
- **Finta:** o alfa avança meio passo e recua, para você errar o tempo.
- **Matilha quebrada:** com o alfa morto e menos de 40% de pé, os que sobram fogem cada um para um lado e voltam para a toca.
- Os outros monstros também ficaram mais espertos: rondam de lado esperando a recarga, em vez de ficar parados, e em bando (goblins) cada um chega por um lado.

### Azhûr, o chefe da Caldeira, refeito no Blender
- **Modelo novo** (`Tools/Blender/BuildAzhur.py`), um colosso de 7 m em blocos, no estilo do jogo:
  - basalto escuro com placas de obsidiana que refletem roxo;
  - veios de lava pulsando entre as placas;
  - no peito, **o núcleo da Estrela Partida** preso numa grade de rocha (pulsa e cresce nos golpes fortes);
  - chifres grossos e uma coroa de espinhos com pontas em brasa;
  - **um vulcão nas costas**, com cratera de lava;
  - respiros de lava nos ombros e punhos enormes com lava entre os nós dos dedos;
  - quatro pedras em brasa girando em volta dele.
- **11 animações feitas para ele:** parado, andar, despertar (levanta e ruge), soco, pisão, arremesso, onda sísmica, investida, cataclismo, dor e morte (as pedras caem).
- Importado por `Tools/ImportAzhur.py`, com materiais próprios: pedra, obsidiana, lava e núcleo.
- **Golpe novo, Chuva de Meteoros:** rochas em brasa caem do céu nas marcas em volta de você, e onde cada uma bate fica lava por 12 s.
- **Barra de vida em 3 fases:**
  - marcas nos terços da barra;
  - cor por fase: dourada, vermelha e violeta pulsando;
  - selos I, II e III;
  - o nome da fase embaixo (O Colosso / A Fúria da Caldeira / O Núcleo Exposto).
- **A cada fase, a lava sobe em volta dele:** mais poças, e mais duradouras na última.

### A Caldeira: céu vermelho e lava
- Na zona do chefe, **o céu, a névoa e a luz ficam vermelhos**, mais escuros nas fases finais. Brasas sobem e cinzas caem em volta da câmera.
- **Poças de lava** espalhadas pela arena (18 fixas, mais as dos meteoros), com crosta de basalto na borda, chamas e brilho no chão. O miolo, onde ele luta, fica livre.
- **Pisar na lava fere** (6% da vida a cada meio segundo) e **põe fogo no herói**: perde vida por 3 s, com chamas no corpo (todos veem) e o aviso "Queimando" na HUD.

### Classes do Godot e o Despertar
- **As 264 habilidades das 66 classes do Godot** estão no jogo (`data/class_skills.json`), nas teclas **1 a 4**, com mana, recarga e o ícone na barra.
- Os 13 tipos funcionam:
  - **ofensivas:** projétil, onda, corrente e raio;
  - **área:** corte em cone, explosão em volta, golpe do céu (marca no chão; fogo, terra e estrelas caem como meteoro), zona que fica e invocação de espíritos;
  - **mobilidade:** investida;
  - **suporte:** bênção (dano, proteção, velocidade, vida que volta, pressa), cura e escudo (absorve dano).
- **Efeitos nos monstros** (servidor): lentidão e prisão, enfraquecer, expor, dano contínuo, empurrão, execução (mais dano com pouca vida) e golpe pelas costas.
- Bênçãos, cura e escudo valem para os companheiros perto. O Último Fôlego não deixa cair abaixo de 1 de vida.
- **A cena das cartas (nível 10):**
  - as seis cartas voam para a tela de costas (com o redemoinho do Vórtice) e **viram uma de cada vez**, da comum para a lendária;
  - **comum é bronze, rara é prata, épica é ouro, lendária é diamante** (furta-cor);
  - a épica e a lendária tremem antes de virar; a lendária clareia a tela e fica com raios de luz girando atrás;
  - cada raridade tem um som;
  - clique numa carta para ver a classe inteira: papel, passiva, bônus, armas e as 4 habilidades. "Despertar como ..." escolhe, e é para sempre;
  - Esc fecha sem escolher; a aba Classes abre de novo, com as cartas já viradas;
  - clique ou Espaço revela tudo de uma vez.
- **O sorteio leva em conta a arma inicial e os pontos investidos:**
  - classe que luta com a sua arma pesa 3x (as outras ficam raras);
  - classe cujos bônus combinam com onde você pôs os pontos pesa até 3,5x;
  - a raça continua pesando como no Godot;
  - a carta marca "★ Luta com a sua arma".
- A **aba Classes** mostra as 4 habilidades da sua classe com a descrição.
- As molduras foram desenhadas por `Tools/BuildCardArt.py`. Os sons de classe (conjuração por elemento e impacto por tipo) vieram do Godot.

### Soco Sério (manoplas, botão direito)
- O herói junta força por meio segundo e solta **um punho gigante** que voa 26 m, atravessa e **arremessa** tudo num corredor largo (chefes ficam).
- Na saída: **quadro de impacto de anime** em preto e branco (Vefects), onda de choque no chão, tranco forte na câmera e poeira. Pelo caminho: anéis de ar a cada 5 m. No fim: explosão.
- Custa 20 de mana e tem 9 s de recarga.

### Primeira pessoa (C)
- A câmera vai para os olhos do herói, com transição suave e um balanço leve ao andar.
- A cabeça some só na sua tela (para a câmera não ficar dentro dela). O corpo, as mãos e a arma aparecem ao olhar para baixo.
- O corpo vira junto com o olhar, a mira aparece e o golpe sai para onde você olha.
- C de novo volta para a terceira pessoa.

### Água
- **Animações de nado novas** (o Godot não tinha):
  - crawl com o corpo deitado, braçadas alternadas, o tronco rolando e pernadas;
  - boiar parado, com braços remando e pernas pedalando.
- As animações são escritas direto no arquivo do herói por `Tools/AddSwimClips.py`.
- **Ao cair na água:** respingo em coroa de gotinhas (cubos, no estilo do jogo), anéis abrindo na superfície e o som. Quanto maior a queda, maior o respingo.
- **Nadando:** ondinhas e gotas a cada braçada e um rastro atrás; parado, um anel de vez em quando. Ao sair, escorre um pouco.

### Menu: mundos
- Cada mundo tem **Excluir**, com confirmação. Apaga o mundo e os heróis dele, e não dá para desfazer.
- **Até 5 mundos.** O título mostra "Meus mundos (3 de 5)" e, cheio, o botão vira "Limite de mundos" e avisa para excluir um.

### Multiplayer
- Testes de rede passando:
  - sessão e estado do jogo;
  - movimento do anfitrião e do cliente;
  - entrada do cliente (com correções de rede sob controle);
  - lobo em rede;
  - menu na rede local.
- O que é novo também vai pela rede:
  - habilidades: o servidor decide e todos veem o efeito;
  - bênçãos e queimadura: replicadas;
  - meteoros e lava do chefe: avisados a todas as máquinas;
  - a cena das cartas: cada jogador vê a sua.
- **Ainda falta o seu teste com 2 jogadores de verdade**, principalmente com as habilidades e o chefe.

## Testes

| Teste | Resultado |
|---|---|
| `RunFeatureShots`: golpe de perto acerta um golem de lado sem mirar; Soco Sério acerta 3 de 3 na fila; primeira pessoa liga e desliga; lago, queda, nado e boiar | passa |
| `RunBossShots`: céu vermelho, 18 poças, desperta na fase 1, fase 2 a 2/3, fase 3 a 1/3, a lava fere e queima | passa |
| `RunClassShots`: nível 10 → 6 cartas → a cena abre → escolhe a mais rara; as 4 habilidades de 5 classes (os 13 tipos) | passa |
| Combate, espada, lobo, matilha (5), criaturas e missões | passam |
| Save, heróis, menu, rede (sessão, movimento, entrada, lobo) | passam |

Fotos em `Saved/Screenshots` (`novo_*`, `chefe_*`, `classe_*`) e algumas em [Images/etapa_1010](../Images/etapa_1010).

## O que ainda não está aqui

- **Ascensões (nível 50)** e as classes únicas: as habilidades alternativas existem nos dados, mas a escolha ainda não.
- Alguns efeitos raros das habilidades do Godot ficaram de fora: armadilha, torreta, eco, marcas que acumulam, Fúria e formas de elemento. Nessas a habilidade funciona sem o efeito extra.
- **Projéteis de classe** não aplicam lentidão nem enfraquecer (o dano e o atordoamento sim).
- Os espíritos invocados são luzes girando, ainda não bichos com modelo.
- O especial antigo das manoplas (Contra-Golpe do Godot) saiu para dar lugar ao Soco Sério.
- O respingo na água fica escuro contra o sol forte (é a luz de contraluz nos cubinhos).
- **Continente completo** (rios inteiros, montanhas, vales, as 9 regiões): é a próxima etapa.

## Como voltar atrás

| Commit | O que contém |
|---|---|
| `17398dd` | golpe de perto, atordoamento, matilha, mundos, Soco Sério, primeira pessoa, nado |
| `3363bf2` | Azhûr novo, Caldeira, lava, céu vermelho, barra em 3 fases |
| `347d37d` | classes, habilidades e a cena das cartas |

`git revert <commit>` desfaz cada um.

## Os poderes de classe refeitos, um por um (07/11, à noite)

O usuário: "recrie os poderes de cada classe do mesmo jeito que fez com as espadas, efeitos únicos para cada,
animações e etc".

- **Antes:** os gestos eram do esqueleto antigo (Adventurer) e não serviam no herói de blocos. Todo poder caía no golpe
  forte da arma, com os mesmos Niagara prontos (onda, explosão, círculo) só trocando a cor do elemento.
- **As animações:** as 264 são feitas no Blender, no esqueleto do herói de blocos.
  - Estão em `Tools/Blender/class_anims_hand.py`:
    - 25 movimentos base (golpe de cima, varrida, estocada, combo, giro, salto com golpe, investida, salto para trás,
      pisão, grito, reza, invocar, empurrar, mãos ao céu, canalizar, apontar, arremessar, guarda, dança, meditar, asas,
      socos, tiro, golpe ajoelhado, braços cruzados que abrem, giro mágico);
    - uma linha por poder, escolhida a dedo: o movimento, o ritmo, o lado, a força e os detalhes (salta, ajoelha,
      gira, flutua, abre as asas, recua, avança, treme).
  - Saem com `blender ... MakePowerAnims.py -- <raiz> --classes`, em `Classes_humano.glb` e `classes.json` (o instante
    do golpe de cada um).
  - Entram com `VORTEX_ANIMS=classes` + `Tools/ImportPowerAnims.py`, como `MX_humano_Poder_classe_<id>_Full`.
- **O golpe na hora certa:** o poder sai no instante do golpe da animação (`ServerCast` espera o "hit"). A mana e a
  recarga contam no clique.
- **Os efeitos:** os 264 são montados pelo `AVortexPowerFx` (as mesmas peças das espadas).
  - Estão em `Tools/Weapons/class_fx_hand.py`: uma linha de peças por poder, com as cores da paleta do elemento (18) e
    os tempos contados do golpe. Os números usam o raio, a duração, o alcance e o atraso de cada poder.
  - Viram `classe_<id>` no `Data/Rpg/power_fx.json` pelo `Tools/Weapons/BuildPowerFx.py`.
  - Nascem com a animação, em cada máquina.
  - Os Niagara prontos saíram desses poderes. Ficaram só os desenhos que mostram o estado: a zona que dura, o escudo,
    os espíritos, a corrente entre os alvos, o aviso no chão e o meteoro.
- Foto das animações: `Docs/Images/poderes/classes_amostra.png`.
