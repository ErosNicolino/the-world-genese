# Azhûr novo, a cratera do primeiro Selo, avisos dos golpes e o cavalo (30/10)

Situação: 🟡 pronto, com fotos e os testes passando; espera o seu teste no jogo.

## Pedidos

- Animações do Azhûr mais fluidas; ele não pode socar reto, porque é gigante.
- Usar o Azhûr novo da pasta Downloads (o modelo do Meshy).
- A cratera:
  - mais bonita, com texturas boas;
  - pensada junto com a história (um dos Selos);
  - bordas que não dá para escalar;
  - sem o chão em xadrez.
- Avisos dos golpes melhores que um círculo amarelo.
- O cavalo:
  - seguia o herói para sempre e atrapalhava os monstros e o herói;
  - deve vir no assobio, como no Red Dead, e sumir quando você se afasta;
  - o herói flutuava em cima dele.

## Azhûr

### O modelo

- **De onde veio:** o do Meshy (Downloads → `Data/Modelos/Azhur/Azhur_Meshy.glb`).
- **Como ganhou animação:** `Tools/Blender/MeshyBiped.py` acha no esqueleto do site, pela posição, o quadril, a coluna,
  o pescoço, a cabeça, os braços e as pernas. Ele põe o modelo com 7 m e copia, osso por osso, as animações do Azhûr
  de blocos (as duas têm o mesmo corpo).
- **Na Unreal:** `Tools/ImportAzhur.py` agora importa `Data/Modelos/Azhur/RuneColossus_Skinned.glb`.
- **Material:** `M_AzhurMeshy` usa a textura dele. As rachaduras de lava e o núcleo brilham laranja, e as runas
  brilham ciano, pela própria cor da textura.

### Animações mais fluidas

As animações saem do `Tools/Blender/BuildAzhur.py`, e todas ganharam uma camada de fluidez:

- a dobra do peito se espalha pela coluna e a da cabeça passa pelo pescoço;
- antebraço, mão e cabeça chegam um pouco depois do braço e do corpo;
- ele respira pesado;
- as curvas são mais suaves (sem tranco no começo e no fim);
- tem uma chave por quadro.

O andar ganhou peso: o quadril balança, os ombros vão ao contrário, o pé bate e a cabeça fica firme.

**O soco** agora vem de cima. Ele ergue o punho acima da cabeça, inclina o corpo e bate no chão na marca (a 4,8 m),
onde o herói de 1,4 m está.

## A cratera: o primeiro Selo

Pela história (`HISTORIA_DO_MUNDO.md`), a Estrela Partida caiu exatamente sobre o primeiro Selo e acordou o Azhûr. A
cratera (`AVortexLavaArena::BuildCrater`) ficou assim:

- **O Selo no meio:** um disco de pedra antiga rachado, com dois anéis de runas. Enquanto o Azhûr vive, as runas
  queimam em laranja (corrompidas). Quando ele cai, elas voltam ao **ciano**, porque o Coração de Azhûr reacende o Selo.
- **Oito pilares dos antigos** em volta do Selo: uns de pé e quebrados, com uma faixa de runas; outros caídos.
- **A Estrela Partida:** o pedaço do Vórtice cravado perto do Selo, com o núcleo brilhando e uma luz violeta.
- **As rachaduras de lava** saem do Selo e se ramificam até a borda.
- **O chão:** uma malha só (sem emenda) com basalto de verdade, a textura CC0 `dark_rock` da Poly Haven. A lava das
  rachaduras pulsa.
- **Os paredões:** de 13 a 23 m, inclinados para dentro, com o basalto `dark_rock_02` (projetado dos três lados) e veios
  de lava e espinhos de obsidiana.
  - **Não dá para escalar:** os paredões não têm degrau e há uma barreira invisível em cima.
  - **Entradas:** duas, no norte e no sul.
- **Sem floresta dentro:** não há árvore, grama nem pedrinha do terreno até 57 m do centro.
- **O xadrez** tinha três causas, todas corrigidas:
  - os materiais do Azhûr não estavam liberados para malhas instanciadas;
  - a função de projeção da Unreal não recebia a textura;
  - o terreno de degraus aparecia por baixo do chão e nas poças.

Materiais: `Tools/BuildCalderaMaterials.py` (texturas em `Data/Textures/Caldeira/`).

## Avisos dos golpes

`M_TelegraphRunes` (`Tools/BuildTelegraphMaterial.py`):

- **Borda e runas:** a borda é forte e tem um anel de runas girando.
- **Enchimento:** enche do meio até a borda no tempo do golpe, com listras correndo para dentro e uma frente clara.
- **Golpe em anel:** o miolo seguro fica escuro, com um tracejado.
- **Fim do tempo:** pisca rápido no último instante.
- **Impacto:** sai uma onda de choque. Nos avisos grandes, sai também uma explosão de fogo.

## Cavalo

- **Como no Red Dead:** o cavalo fica onde você deixou e não segue mais.
- **Assobio (X):** se ele estiver longe, surge a 35 m atrás da câmera e vem galopando até parar ao seu lado.
- **Longe:** a 70 m de você, ele vai embora; o assobio chama de novo.
- **Colisão:** ele não empurra mais heróis nem monstros (passam através dele).
- **Na sela:** o herói senta na sela de verdade, pelo osso "Saddle" do cavalo e pelo quadril do herói. Antes subia
  102 cm (62% da altura do cavalo); agora sobe 50 cm.

## Os outros modelos novos

Cavalo, golem musgoso, Thorun, Seryth e sapo do Meshy já estão em `Data/Modelos/`. Ainda faltam o Seryth e o sapo.

### Golem musgoso e Thorun (31/10)

Os dois passam pelo `Tools/Blender/MeshyBiped.py` (as animações dos antigos do Godot) e pelo
`Tools/ImportMeshyMonsters.py`. Os ombros saíam quebrados, por dois motivos diferentes:

- **Golem:** o primeiro osso do braço do esqueleto do site é o ombro (deitado, sai do peito para o lado). Ele era
  tratado como braço e girava para baixo com o bloco do ombro inteiro, e o braço saía da cintura. Agora esse osso fica
  com o peito e o braço começa no osso de baixo.
- **Thorun:** os braços copiavam a direção do antigo (retos para baixo) e afundavam dentro do tronco largo. Agora ficam
  na pose do próprio modelo (abertos) e só recebem o movimento do antigo (`own_arms`).

Teste: `RunCreatureSmoke.ps1 -Species Golem,RootTitan` PASS.

## Testes

- `Tools/RunBossShots.ps1`: PASS (o Azhûr novo, a cratera, as fases, a lava fere e queima).
- `Tools/RunMountShots.ps1`: PASS. Testa:
  - montar, galopar e desmontar;
  - o assobio traz o cavalo de 40 m;
  - ele fica onde chegou;
  - longe do herói ele vai embora.
