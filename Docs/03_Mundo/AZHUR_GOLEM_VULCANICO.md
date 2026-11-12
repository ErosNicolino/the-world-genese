# Azhûr, o golem vulcânico (02/11)

Situação: 🟡 feito e testado no jogo de um computador (`Tools/RunBossShots.ps1` PASS, com fotos). Ainda não foi testado
com dois jogadores.

O documento de design pedia um chefe completo: 12 habilidades, duas fases, IA e efeitos vulcânicos, sem mudar o
modelo. O golem vulcânico do projeto é o **Azhûr, o Colosso da Caldeira**. O modelo é o do Meshy, com o núcleo no
peito. O pacote "Giant Golem AnimSet" do Fab serviu só de referência, como o usuário pediu.

## Animações

As animações são escritas em código, em `Tools/Blender/azhur_moves.py`. O `BuildAzhur.py` as aplica no Azhûr de
blocos, o `MeshyBiped.py` as passa para o modelo do Meshy e o `ImportAzhur.py` as importa na Unreal.

Todas foram refeitas para ficar mais naturais:
- antecipação clara;
- o quadril vai antes do peito, e o peito antes do braço;
- impacto seco;
- o braço continua o movimento depois do golpe;
- a volta é lenta e ele assenta;
- o peso passa de uma perna para a outra;
- base larga com os joelhos dobrados.

**As mãos tortas:** as duas mãos entortavam e puxavam para o meio, para baixo. O `MeshyBiped.py` agora copia só 35% do
giro do pulso (`HAND_KEEP`), e a mão acompanha o antebraço.

### Matriz: habilidade e animação

| # | Habilidade | Clipe | Situação | Impacto (s) |
|---|---|---|---|---|
| 1 | Punho Devastador | `Hook` | animação nova | 1.0 |
| 2 | Martelo Colossal | `Seismic` | existente, refeita | 1.46 |
| 3 | Pisada Sísmica | `Stomp` | existente, refeita | 1.07 |
| 4 | Varredura de Pedra | `Sweep` | animação nova | 0.95 a 1.38 |
| 5 | Combo Sísmico | `Combo` | adaptação: soco direito, soco esquerdo, dois punhos | 0.78, 1.68, 2.95 |
| 6 | Coração Incandescente | `CoreBurst` | animação nova: fecha, carrega e abre | 1.75 |
| 7 | Ruptura Magmática | `Punch` | existente: o punho bate no chão e a fenda sai dali | 0.8 |
| 8 | Meteoro Vulcânico | `Throw` | existente, refeita | solta em 1.52 |
| 9 | Raio do Núcleo | `Beam` | animação nova: firma o corpo, segura e vira | 1.0 a 3.4 |
| 10 | Erupção Vulcânica | `Eruption` | animação nova: braços erguidos e uma pisada | 1.0 a 2.0 |
| 11 | Armadura Derretida | `Shed` | animação nova: encolhe, treme e explode | 1.38 |
| 12 | Despertar do Núcleo | `Ultimate` | animação nova: joelho no chão, carga, rugido e os dois punhos | 4.45 |

**Os estados:**
- Idle: `Idle`;
- Chase: `Walk`;
- Detection e PhaseTransition: `Awaken` (ajoelhado, levanta e ruge);
- Stunned: `Stun`, que é novo e fica em laço;
- Death: `Death`, refeita;
- `Hit` e `Charge` também foram refeitas.

**Animações que ainda faltariam:** nenhuma das 12 ficou sem animação própria. Como no pacote de referência, ainda
não há andar de lado nem as três partes do atordoamento (começo, laço e fim); o laço `Stun` entra direto.

### Montage e notificações

O projeto não usa Montage para monstros. A animação vai pelo `Act(clipe)` do `AVortexMonster`, replicado para
todos. Os instantes de impacto ficam no `Data/Creatures/azhur.json` (os mesmos de `azhur_moves.py`), e o servidor
marca e fere nesses instantes. Faz o papel das Animation Notifies e funciona igual em rede.

## Componentes e dados

- **`UVortexVolcanicBoss`** (`Source/Vortex/.../Creatures/VortexVolcanicBoss.h/.cpp`):
  - é o cérebro do chefe e nasce com o Azhûr em todas as máquinas;
  - o `AVortexMonster` continua sendo o corpo (vida, movimento, animação, morte, saque);
  - o `ThinkBoss` passa para ele.
- **`Data/Creatures/azhur.json`:** todos os números (dano, alcance, raio, recarga, preparação, duração, recuperação,
  velocidade, resistência, atordoamento, fases, emendas e a escala dos efeitos). Mudar ali não precisa compilar.
- **Material:** `M_AzhurMeshy` ganhou o parâmetro `CoreHeat` (`Tools/ImportAzhur.py`). A fase 2 aquece o núcleo e
  as rachaduras; na morte ele apaga.
- **HUD:** a barra do Azhûr mostra 2 fases, com a divisão em 40%.

### Hitboxes

Nada fica com colisão ligada o tempo todo:
- cada golpe fere por marcas no chão (`AVortexTelegraph`), que o servidor resolve só no instante do impacto;
- o feixe é a exceção: o servidor confere a linha a cada 0,2 s, enquanto ele dura.

Formatos:
- arco de marcas: Punho Devastador e Varredura (a varredura estoura da direita para a esquerda, junto com o braço);
- círculos: Martelo e Pisada;
- anel: Martelo, Pisada e o Despertar (o miolo é seguro);
- linha que avança: Ruptura;
- círculos espalhados: Erupção e Meteoros;
- pedras arremessadas: os fragmentos da Armadura.

## Sistemas Niagara

`Tools/MakeAzhurFx.py` faz cópias dos efeitos dos pacotes gratuitos do projeto em `/Game/Vortex/Bosses/Azhur/FX`:

| Sistema | Base |
|---|---|
| NS_MagmaCore_Charge | Free Spells Aura Fire |
| NS_MagmaCore_Explosion | Vefects Fire Big |
| NS_Volcanic_Fissure | Vefects Fire Floor 02 |
| NS_Lava_Pillar | Vefects Fire Big Simple |
| NS_Molten_Meteor | ArrowTrail Fire 02 |
| NS_Core_Beam | Vefects Fire Medium (mais um cilindro brilhante, o feixe) |
| NS_Stone_Impact | Vefects Fire Floor 02 Smoke |
| NS_Rock_Debris | Vefects Fire Floor 01 Smoke |
| NS_Volcanic_Death | Vefects Fire Big Smoke |

**Parâmetros:** escala, duração e `User.Color` (quando o sistema tem esse parâmetro); a escala de todos juntos é
`fxScale` no `azhur.json`. As bases do Free Spells com runas douradas ficaram de fora: em cada golpe elas cobriam a
tela. Os avisos do Azhûr também não soltam mais a onda e a explosão douradas de cada marca.

## Sons

`Tools/MakeAzhurSfx.py` monta 15 sons em camadas, cada um com 3 variações (`Data/Audio/azhur_*_v1..3.wav`; na Unreal,
`Tools/ImportExtraSfx.py`):
- rugido, passo, whoosh do braço, punho no chão;
- carga do núcleo, explosão, fenda de lava;
- meteoro (saída e queda), raio, coluna de lava;
- armadura estalando, atordoado, morte, golpe recebido.

As gravações são da Sonniss GDC 2024 (`Data/Sfx/AzhurSonniss`) e do OpenGameArt CC0 (`Data/Sfx/AzhurCC0`: rugido
grave, bolhas e fogo), mais graves (é um colosso de 7 m) e com um seno grave por baixo dos impactos. O
`UVortexVolcanicBoss` toca cada som no instante da animação, no lugar onde acontece (`MulticastSfx`). Os créditos
estão em `Data/Sfx/CREDITOS_SONS.txt`.

## IA

- **Estados:** Idle, Detection, Chase, Attack (físicas), Ability (vulcânicas), Recovery, Stunned, PhaseTransition e
  Death.
- **Escolha do golpe:**
  - pela distância (mínimo e máximo de cada habilidade), pela recarga e pela fase;
  - por sorteio com peso;
  - a última habilidade não se repete, e a penúltima vale a metade.
- **A final (Despertar do Núcleo):** a primeira vez quando a vida passa de 75%; na fase 2 volta quando recarrega.
- **Atordoamento:**
  - perder 9% da vida em pouco tempo quebra a guarda: 4 s atordoado, levando 30% a mais de dano;
  - pode acontecer de novo depois de 14 s;
  - nunca durante a final nem na troca de fase.
- **Fase 2 (40% da vida):**
  - a transição própria: ajoelha, levanta, ruge, sobe lava em volta e o núcleo esquenta;
  - golpes 20% mais fortes, recargas 30% menores e pausas menores;
  - mais rápido;
  - emendas (pisada e fenda, punho e combo, martelo e erupção, meteoro e feixe);
  - a Armadura Derretida só aparece na fase 2;
  - o Despertar aprimorado: 10 fendas e meteoros.

## Multiplayer

Tudo é decidido no servidor: a escolha, os estados, o dano, a vida e a troca de fase.
- **Os clientes recebem:**
  - a animação (o `Act` replicado);
  - a fase (replicada; o calor do núcleo segue ela);
  - o estado (replicado no componente);
  - as marcas (atores replicados);
  - os efeitos e o feixe, por multicast do componente.
- **O componente:** tem o mesmo nome em todas as máquinas (`SetNetAddressable`).
- **Grupo:** com mais heróis perto, cada golpe deles fere menos (o sistema de grupo do jogo, `groupHealthPerHero`).

## A cratera, mais limpa

O usuário achou o chão "muito poluído de fogo". Agora:
- 7 rachaduras finas, com menos galhos (eram 12 largas);
- 8 poças (eram 18), com a luz mais fraca;
- veios de lava em 12% dos paredões (eram 35%);
- o brilho da lava no chão em 3,2 (era 9).

## Testes

`Tools/RunBossShots.ps1` PASS. Fotos em `Saved/Screenshots/chefe_*.png`, e `chefe_habilidades.png` junta as 12. O
teste confere:
- o Azhûr na Caldeira, o céu vermelho e a arena;
- o despertar na fase 1;
- as 12 habilidades, uma de cada vez, cada uma no estado certo, com a foto no impacto;
- ainda na fase 1 antes dos 40%;
- a fase 2 com 40%, na transição;
- o atordoamento depois de um tranco de dano;
- a lava fere e queima.

## Pendências

- **Rede:** testar com dois jogadores (não há teste automático de chefe em rede).
- **Equilíbrio:** ajustar os números jogando (estão todos no `azhur.json`).
- **Câmera:** o Azhûr de 7 m fica escuro contra os paredões em algumas fotos; talvez uma luz de recorte nele.
- **Animações que faltam:** andar de lado e o atordoamento em três partes, como no pacote de referência.
