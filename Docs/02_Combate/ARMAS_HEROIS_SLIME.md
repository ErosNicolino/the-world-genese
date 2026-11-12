# Armas novas, heróis fixos por raça, slime de gelatina e golpes que não pegavam (30/10)

Situação: 🟡 pronto e testado por testes automáticos e fotos; espera o seu teste no jogo.

## Pedidos

- Trocar as 5 armas pelos modelos novos do Docs (espada longa, martelo, adagas duplas, cajado, arco); o arco sem corda, com a corda feita na Unreal.
- Heróis novos: um modelo fixo por raça (sem personalização por enquanto), polidos e animados sem quebrar. O elfo tinha uma parede grudada. A raça "Diabo" se chama **Demônio**.
- Slime refeito do zero na Unreal: bem gelatinoso, o ataque estica o corpo até o inimigo e volta. Quatro tipos: normal, fogo, veneno e ferro.
- A espada muitas vezes não dava dano mesmo colada no inimigo.
- O herói ficava preso no corpo do inimigo morto.
- Tirar a marca de alvo da tela.

## 1. Armas novas

- **Origem:** os zips do Docs (Meshy) extraídos em `Data/Modelos/Armas/<tipo>/`.
- **Encaixe:** `Tools/Blender/FitNewWeapons.py` põe cada arma no eixo da arma antiga, com o mesmo comprimento e a pegada no mesmo ponto da mão. Assim as posturas, os golpes e o alcance continuam iguais.
  - As antigas ficaram guardadas em `Data/VortexWeapons/Models/_antigas/`.
- **Texturas:** de 8192 para 2048 (cor) e 1024 (normais, rugosidade).
- **Unreal:** `Tools/ImportNewWeapons.py` importa no mesmo caminho das antigas. As definições `DA_<tipo>` não mudam.
- **Espada do começo:** o `DA_DefaultSword` voltou a ser a espada (era a adaga inicial desde 22/10). Como o alcance do golpe sai do tamanho do modelo, a adaga curta encurtava o golpe da espada.
- **Adagas duplas:** o arquivo traz uma adaga só; ela vai nas duas mãos (`W_adagas_gemeas` e `_L`).
- **Arco de ametista e flecha:** o zip "separar os 2" virou:
  - `W_flecha`: a flecha que voa quando se atira, no lugar das formas básicas;
  - `W_arco_ametista`: guardado para um arco raro futuro.
- **Corda do arco** (`UVortexWeaponParts::TickBowString`):
  - presa nas duas pontas do arco, achadas nos vértices do modelo;
  - segurando o clique, vai até a outra mão;
  - ao soltar, volta e vibra um instante;
  - material `M_BowString`: fio trançado creme e castanho.

## 2. Heróis fixos por raça

- **Modelos:** personagem principal (humano), anão, elfo, bárbaro gigante (bárbaro), Demonio (Demônio) e esqueleto (morto-vivo).
- **Esqueleto e animação** (`Tools/Blender/RigHero.py`):
  - cada modelo usa o **esqueleto do aventureiro**, então servem todas as animações atuais: andar, correr, esquiva, nadar, montar, golpes de todas as armas e os gestos das habilidades;
  - os pesos de pele que vieram do site são reaproveitados: cada osso do site vira o osso do aventureiro que faz o mesmo papel;
  - o corpo é dobrado até a postura de descanso do aventureiro (braços caídos);
  - as juntas vão para o corpo novo;
  - na Unreal a translação dos ossos vem da malha, e cada herói anima com as próprias proporções;
  - fotos de conferência no Blender: `Saved/Previews/heroes_grid.png` (parado, correndo, golpeando e caindo).
- **Elfo:** a parede saiu (66.904 faces).
- **Texturas:** de 8192 para 2048.
- **Unreal:**
  - `Tools/ImportHeroes.py` põe os modelos em `/Game/Vortex/Characters/Heroes/<raça>/`;
  - no jogo, `AVortexCharacter::ApplyHeroModel` troca o corpo pelo modelo da raça e esvazia as peças de roupa. O tamanho vem da raça (anão menor, bárbaro maior). Pela rede, todos veem o modelo da raça de cada um.
- **Demônio:** o nome mudou só na tela; o código interno continua `diabo`, para os saves antigos abrirem.
- **Fotos no jogo:** `Saved/Screenshots/heroi_<raça>.png` e `heroi_<raça>_golpe.png` (teste `Tools/RunHeroShots.ps1`).

## 3. Slime de gelatina (4 tipos)

- **Malha** (`Tools/Blender/BuildSlime.py`): uma gota um pouco quadrada (combina com os blocos), com miolo dentro, olhos grandes com brilho e uma boquinha.
- **Materiais** (`Tools/ImportSlime.py`):
  - `M_SlimeJelly`: translúcido, a borda mais opaca e o brilho por dentro;
  - `M_SlimeSolid`: olhos, miolo e o de ferro;
  - o próprio material deforma a malha.
- **Movimento** (`UVortexSlimeLook`):
  - anda aos pulinhos (estica no ar, amassa ao cair e balança);
  - parado, respira;
  - **o bote estica um braço de gelatina até o alvo** (alcance ~2,6 m) e volta tremendo;
  - no salto, encolhe antes;
  - apanhando, treme;
  - morto, derrete;
  - pisca de vez em quando.

| Tipo | Onde aparece | Diferença |
|---|---|---|
| **Gota de Jade** (normal) | vale e a maioria das regiões | verde; foge com pouca vida |
| **Gota de Brasa** (fogo) | Brasafenda, dungeon de vulcão | laranja brilhante, faíscas; o bote **queima** (3 s) |
| **Gota Peçonhenta** (veneno) | Lodonegro, Morthúria, dungeon de pântano | roxa com miolo verde; o bote **envenena** (5 s, não mata) e cospe veneno de longe |
| **Gota de Ferro** | Picos de Raikor, Solcânion, mina | metálica, lenta, o dobro de vida, pancada no chão em volta |

- **Contagem:** todas contam como "gota" para XP, saque e missões ("Derrote gotas de jade").
- **Veneno no herói:** aviso "Envenenado!" e um selo verde na HUD, como a queimadura.

## 4. Golpe que não pegava colado no inimigo

- **A causa:** o herói "prendia o foco" num inimigo vivo a até 5,5 m. Com outro bicho colado nele, o golpe virava para o primeiro, longe, e cortava o ar.
  - A marca na tela atualizava esse foco o tempo todo, e o problema aparecia menos. Sem ela, piorou.
- **Agora** (`AVortexCharacter::PreviewMeleeTarget`): se o foco está fora do alcance da arma e outro inimigo está ao alcance, o golpe vai no de perto.
- **Cajado:** colado no inimigo, o tiro nascia depois dele e passava reto. Agora sai do peito direto no inimigo.
- **Teste novo** (`Tools/RunMeleeSweep.ps1`): espada, martelo, adagas e cajado contra gota, goblin, golem e cogumelo:
  - colado, perto, no limite do alcance, de lado e atrás;
  - um caso com um sapo preso no foco a 4 m e o inimigo colado atrás.
  - Resultado: **101 acertos, 0 erros**. O cajado atira para a mira, então só os da frente contam.

## 5. Corpo morto prendendo o herói

- **O lobo:** a caixa do corpo dele (2,7 m) continuava bloqueando depois de morto.
- **Os outros monstros:** o bloqueio só saía no servidor, então quem entrava pela sala esbarrava no corpo até ele sumir.
- **Agora:** morto, o corpo deixa de bloquear em todas as máquinas.

## 6. Marca de alvo

Saiu a marca e a opção do menu. A virada rápida do mouse ainda troca o foco.

## O que testar

1. **Heróis:** crie um herói de cada raça; ande, corra, pule, nade, monte e golpeie com as 5 armas. Algum braço ou perna dobra estranho? Na lista de raças aparece "Demônio"?
2. **Armas:** a espada longa, o martelo, as adagas (uma em cada mão), o cajado e o arco, na mão e no golpe. No arco, segure o clique: a corda vai até a mão? Ao soltar, ela vibra e a flecha nova voa?
3. **Golpe colado:** lute com vários bichos em volta; o golpe deve ir no que está colado.
4. **Corpo morto:** mate um lobo e passe por cima.
5. **Slimes:**
   - no vale, a de jade;
   - a de brasa (comando `/spawn SlimeFire`, ou Brasafenda);
   - a peçonhenta (`/spawn SlimePoison`);
   - a de ferro (`/spawn SlimeIron`).
   - Veja o bote esticando e o tremido, a queimadura e o veneno.
6. **Multiplayer:** cada um vê o herói da raça do outro.

## Testes rodados

- `Tools/RunMeleeSweep.ps1`: PASS (101/0).
- `Tools/RunCreatureSmoke.ps1 -Species Slime,SlimeFire,SlimePoison,SlimeIron`: PASS. Fotos `Saved/Screenshots/criatura_Slime*_perto.png`.
- `Tools/RunHeroShots.ps1`: PASS (as 6 raças com o modelo certo).
- `Tools/RunOnlineLobbySmoke.ps1`: PASS. O anfitrião humano e o convidado elfo se veem cada um com o modelo da sua raça. A recusa "ECONNREFUSED" no log é a tentativa enquanto o anfitrião carrega; na segunda tentativa entra.

## Ferramenta nova para testar com o editor

- `Vortex.Cmd <comando>` no console roda o chat de testes, sem a barra. Exemplo: `Vortex.Cmd spawn SlimeFire 5 2`.

## Segunda rodada (30/10, depois do seu teste)

| Problema | Causa | Correção |
|---|---|---|
| Braço do anão e do esqueleto entrando no corpo; adagas grudadas na frente | os ombros dos modelos novos são mais estreitos que os do aventureiro, e as animações dele trazem as mãos para perto do corpo | os braços abrem para fora por raça, em toda animação: anão 20°, esqueleto 10°, bárbaro 8°, humano 6°, elfo e Demônio 4°. Medido no Blender com a animação parada |
| Ombro colado e para dentro da roupa (anão) | a junta do ombro ficava funda dentro do colete | o ombro sai do tronco e o braço abre um pouco, por raça (ombro / abertura): humano 2 cm / 6°, anão 8 cm / 9° (com os pesos do ombro corrigidos), elfo 1,5 cm / 4°, bárbaro 3,5 cm / 6°, Demônio 2 cm / 4°, esqueleto 7 cm / 6° (`FVortexBodyShape::ShoulderOut` e `ArmSpread`) |
| Armas com uma textura feia (cinza) | a reimportação manteve o material antigo `MI_W_aco_escuro` | `Tools/ImportNewWeapons.py` põe o material novo (`MI_WN_<tipo>`) em todos os espaços de cada arma |
| Primeira pessoa bugada (lona de roupa esticada, braços gigantes) | a cópia flutuante dos braços encolhia o resto do corpo e esticava a roupa | **primeira pessoa de corpo inteiro, como no Fortnite**, descrita abaixo |
| Andar e correr iguais | o herói anda a 460, e a animação de correr já entrava a partir de 420; com Shift era a mesma, só mais rápida | correndo com Shift, o tronco e o quadril inclinam para a frente |
| Slime de veneno com efeito de cura | a aura pronta tinha cruzes verdes | bolhas roxas feitas no código (sobem, crescem e estouram); corpo e miolo roxos; respingo, marca no chão e o aviso "Envenenado" também roxos |

**Primeira pessoa de corpo inteiro (como no Fortnite):**
- os olhos ficam no próprio corpo;
- só quem joga vê um corpo sem a cabeça (`Hero_<raça>_FP`), que copia a pose do corpo de verdade; os outros jogadores e a sombra continuam com o corpo inteiro;
- olhando para a frente, os braços quase não aparecem;
- olhando para baixo, a câmera avança até a frente do peito, e aparecem o tronco, as pernas, as mãos e a arma;
- no golpe, o braço e a arma de verdade passam na frente.

**Fotos:**
- `Saved/Screenshots/heroi_<raça>.png`: parado;
- `_golpe`: golpeando;
- `_corrida`: de lado, inclinado;
- `_1p`: primeira pessoa olhando para a frente;
- `_1p_baixo`: olhando para baixo;
- `_1p_golpe`: golpeando em primeira pessoa.

**Testes:** `RunHeroShots` PASS, `RunMeleeSweep` PASS (101/0), `RunCreatureSmoke -Species SlimePoison` PASS.

## Ainda falta (próximos passos)

- Andar e correr de verdade: o projeto `VortexReferencia` tem pacotes de animação humanoide (Locodrome, FreeAnimationLibrary, FreeAnimationsPack, SampleAnimationPack). Dá para passar para o esqueleto dos heróis com o redirecionador da Unreal (IK Retargeter).

- Piscar e expressões nos heróis: uma versão do rosto com os olhos fechados, trocada por um instante.
- Monstros (golem, morcego, sapo, cogumelo corrompido), chefes (Azhûr, Seryth, Thorun), cavalo e ovelha.
- A névoa roxa saindo dos monstros.
- Iluminação: noite mais clara e lâmpadas mais fracas.
