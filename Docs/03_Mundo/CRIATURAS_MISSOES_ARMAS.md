# Criaturas, missões, atributos e armas novas (09/10/2026)

Situação: 🟡 pronto, com os testes automáticos passando; espera o seu teste no editor.

Esta etapa cobre a lista pedida depois da revisão do jogo:

1. tirar o vigor e usar os atributos do Godot;
2. atordoamento e contra-ataque na defesa perfeita;
3. o travamento do primeiro poder;
4. texturas e modelos novos das armas;
5. abas Classes, Alquimia e Missões;
6. história do mundo, missões, diálogo e os bichos do Godot;
7. desvio de obstáculos;
8. save automático;
9. volumes de áudio;
10. HUD de missão, grupo e efeitos;
11. sons de combate;
12. o mapa no M.

## O que mudou

### Atributos (sem vigor)
- O GAS agora só tem **vida e mana**. O vigor saiu do conjunto de atributos; a barra que existia na HUD já era a de XP.
- Como no Godot (Compêndio v1.2), os **2 pontos por nível** vão para os **12 atributos oficiais**: Ataque, Defesa, Resistência, Vida máx., Mana máx., % Crítico, % Dano crítico, % Velocidade de ataque, % Pressa, Poder de magia, % Roubo de vida e Recuperação. Ponto e equipamento somam igual.
- Os oito antigos (Força, Destreza, Vitalidade, Inteligência, Mana, Espírito, Recuperação, Sorte) viraram o vetor da raça, mostrado na aba Herói.
- Cada atributo vale no combate:
  - Força, Destreza e Inteligência entram no golpe corpo a corpo, à distância e de magia, respectivamente;
  - a Defesa tira do golpe físico;
  - a Resistência corta a magia (veneno, cristais, runas);
  - a Velocidade acelera golpes e animações;
  - a Pressa encurta a recarga do especial;
  - o Roubo de vida cura.
- Base do Godot: vida 100, mana 60, crítico 2% e regeneração de mana de 1,1/s.
- Os saves antigos devolvem os pontos (o schema passou para 4).

### Defesa perfeita
- A criatura aparada fica **atordoada por 0,65 s**: o bote é cancelado e aparecem estrelinhas, faíscas e um tranco na câmera.
- O golpe seguinte, dentro de 1,6 s, é um **contra-ataque**: crítico garantido e +50% de dano. A HUD mostra "Contra-ataque" com o tempo restante.
- Criatura atordoada leva +25% de dano.
- O campo `Stun` dos golpes das armas agora atordoa (chefes não).

### O travamento do primeiro poder
- Os pacotes de efeitos (ArrowTrail, Basic_VFX, FreeParticle_SoftTofu) foram salvos numa versão antiga. A engine **recompilava cada efeito toda vez que ele era carregado**: o log mostrava "Compiling System… 0,2 a 0,65 s" a cada flecha ou magia.
- Os 39 efeitos foram re-salvos (`Tools/ResaveFx.py`) e ficam na memória depois do primeiro uso. Agora a compilação acontece uma vez, no aquecimento ao entrar no mundo.

### Criaturas do Godot
- **16 modelos**:
  - 11 espécies: gota de jade, cogumelo errante, golem musgoso, morcego de ametista, sapo-gigante, goblin, coelho, cervo e os chefes Seryth, Thorun e Azhûr;
  - 5 pelagens de cavalo selvagem.
- Os modelos foram convertidos do Godot com todas as animações (`Tools/ConvertGodotMonsters.py`, `Tools/ImportMonsters.py`).
- Uma classe só, `AVortexMonster`, guiada pela ficha da espécie (`VortexMonsterSpecies.cpp`). Os golpes, tempos, alcances e recargas são os do Godot:
  - **gota**: trombada e salto esmagador;
  - **cogumelo**: cospe veneno, pula para trás e solta esporos;
  - **golem**: soco, pancada no chão e pedra arremessada;
  - **morcego**: picada, mergulho e guincho;
  - **sapo**: língua de 4,6 m e salto;
  - **goblin**: porrete e pedras, em bando com chefe maior;
  - **animais**: pastam e fogem; o cavalo só foge se apanhar.
- **Chefes**: marcam o chão antes de cada golpe (círculos e anéis que enchem). Ficam mais fortes a 2/3 e a 1/3 da vida e voltam ao começo se todos fogem da arena. O Azhûr tem 6 dos 18 golpes do Godot.
- **Onde nascem**:
  - os postos fixos do vale;
  - o Azhûr na Caldeira (nível 12);
  - fora da área do vale, quadras de 72 m sorteadas pela posição, sempre iguais (gotas, lobos, cogumelos, morcegos, bandos de goblins, golens, e sapos perto da água);
  - animais em toda parte.
- Cada criatura aparece a 70 m e some a 85 m. Vencida, volta depois de 1 a 2 minutos (os chefes, depois de 5).
- Ninguém ataca dentro da cidade.
- XP e saque usam as tabelas do Godot para cada espécie.

### História, missões e diálogo
- **História do mundo**: [HISTORIA_DO_MUNDO.md](HISTORIA_DO_MUNDO.md).
  - O Vórtice sobre a Yggdrasil e os nove Selos com seus Guardiões.
  - A Estrela Partida, que acordou Azhûr, e os Despertos.
  - Dez capítulos da jornada; o capítulo 1, em Valdora, é jogável agora.
- **31 missões** (`Data/Story`, `Tools/BuildStoryData.py`):
  - as 21 do Godot (Rhea, Ilsa, Bram, Nicolly, Lary, Juh, Ducci, Amal, Lyra);
  - a linha principal nova: O Despertado → Recruta do Vale → Os Cristais que Cantam → Onde a Seiva Some → … → O Coração de Azhûr → Os Nove Selos;
  - missões novas da Julia, do Garrick, da Marta e da Rosa.
- As missões de fora do vale aparecem travadas até o continente chegar.
- **Diário no herói**: objetivos derrotar, chefe, coletar, chegar a e falar com. As missões automáticas entram sozinhas. O servidor decide tudo e o diário é salvo.
- **Conversa no E**:
  - as falas do Godot aparecem escritas aos poucos;
  - as missões entram como escolhas ([Missão], Sobre, [Entregar]);
  - presentes e marcas das falas funcionam;
  - o NPC para e olha para você.

### Interface
- **Janela do herói**: seis abas (Herói, Mochila, Técnicas, Classes, Alquimia, Missões).
  - **Classes**: o Despertar no nível 10 tira 6 cartas, pela chance da raridade (58/27/11/4%) e pela afinidade da raça. A classe soma seus bônus aos atributos oficiais. Há também um catálogo das 52 classes sorteáveis.
  - **Alquimia**: as 20 receitas do Godot (poções e equipamento de couro, guarda, casca e cratera).
  - **Missões**: o capítulo atual, as missões em andamento, disponíveis, concluídas e travadas, os detalhes, as recompensas e o botão "acompanhar".
- **HUD**:
  - a missão acompanhada embaixo do minimapa;
  - nome, título, falas e o ! ou ? sobre os NPCs, e o "[E] Falar com …";
  - os efeitos ativos (contra-ataque, cura da Fonte, cidade protegida);
  - os quadros do grupo, mais nome e seta de cada companheiro na tela (na borda quando ele está fora dela);
  - qualquer criatura na moldura de alvo (os chefes com moldura maior);
  - o aviso "Jogo salvo".
- **Minimapa**: mostra a cidade, o objetivo da missão e **só as criaturas que você viu** (na tela, sem morro ou árvore no caminho).
- **M**: abre o mapa do vale inteiro, com a cidade, a Caldeira, os objetivos, os moradores (com as marcas de missão), o grupo e as criaturas vistas. O alcance do minimapa foi para as opções (Interface).

### Áudio
- Os 87 sons do Godot que faltavam foram importados: criaturas, chefes, magias, missões, diálogo, moedas e ambientes das regiões.
- Quatro categorias (Música, Efeitos, Ambiente, Interface) e a aba **Áudio** nas opções, com o volume geral e o de cada categoria, de 10 em 10%.
- A música de luta toca com qualquer criatura brigando perto.

### Desvio de obstáculos
- Lobos, criaturas e moradores **sentem o caminho**: varrem a cápsula alguns metros à frente e escolhem o ângulo livre mais perto, mantendo o lado escolhido. Se ficam presos por 1,2 s, dão um passo firme de lado. Também não se empilham (`VortexSteering`).
- Não usei NavMesh: o terreno é gerado e entra e sai com o streaming.

### Save automático
- A ficha e o diário já gravavam a cada mudança.
- Agora **o mundo também grava a cada 45 s**: posição do anfitrião, moradores e hora do dia. Não grava durante a volta ao último lugar, com o herói caído ou no Estádio.

### Armas novas
- As 25 armas foram **refeitas no Blender** (`Tools/Blender/vortex_armory.py`, `BuildVortexWeapons.py`):
  - peças chanfradas e lâminas com fio de outro material;
  - braços de arco e laços contínuos;
  - gemas que brilham;
  - textura própria por material: aço escovado, ouro, bronze, madeiras com veio, couro trançado com costura, tecido, gemas.
- Lâminas mais largas e grossas, para combinar com o herói voxel.
- Elas usam **o mesmo referencial de mão das do Godot** (`Tools/WeaponFrames.py`), então a pegada é a do Godot. A espada padrão passou para o soquete da mão, como as outras.
- A **Espada e Escudo** ganhou um escudo redondo na mão esquerda.
- `Tools/ImportVortexWeapons.py` importa e aplica os materiais.
- Fotos: `Tools/RunWeaponCloseUp.ps1` (parada, dos dois lados, no golpe e no especial) e prévias em `Saved/WeaponPreviews`.

## Testes

| Teste | Resultado |
|---|---|
| `RunCreatureSmoke` (12 espécies: nascem, aparecem, atacam, morrem, dão XP; O Despertado e Recruta do Vale até a entrega; fotos da interface) | passa |
| `RunCombatSmoke`, `RunSwordSmoke`, `RunWolfSmoke`, `RunPackSmoke` | passam |
| `RunSaveSmoke`, `RunHeroSmoke`, `RunMenuFlowSmoke`, `RunPopulationSmoke` | passam |
| `RunHudShot` (inclui o teste de RPG: pontos nos oficiais, poção, equipamento) | passa |
| `RunWeaponShot`, `RunWeaponCloseUp` (25 armas) | passam |

## O que ainda não está aqui

- **Técnicas de classe (1 a 4)** e as Ascensões do nível 50: a classe dá os bônus, mas as habilidades ainda não.
- **Lojas** (Ilsa, Ducci, Garrick): o diálogo avisa que chegam em breve.
- **Golpes das armas**: os modelos e a pegada são novos, mas as animações dos golpes continuam as do Godot. Se algum golpe parecer errado, me diga qual arma.
- **Grupo**: a HUD mostra todos os jogadores da sessão; ainda não há convite nem grupo separado.
- **Efeitos**: são os estados que existem hoje (contra-ataque, Fonte, cidade). Os buffs de verdade chegam com as técnicas.
- **Bandidos e domar cavalos** (o Godot tem os dois): ainda não.
- **Seryth e Thorun** ficam nas regiões deles: aparecem quando o continente chegar; hoje só no teste.
- O Azhûr tem 6 dos 18 golpes do Godot.

## Como voltar atrás

Cada parte tem um commit próprio:

| Commit | O que contém |
|---|---|
| `c1d14df` | atributos, aparo e efeitos |
| `039f46e` | criaturas, missões, HUD, abas e áudio |
| `c655495` | armas, save automático e desvio de obstáculos |
| `c48d358` | ajustes da janela do herói |

`git revert <commit>` desfaz cada um.
