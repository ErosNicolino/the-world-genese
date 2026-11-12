# Auditoria de FPS (11/11/2026)

Onde o tempo de cada quadro vai e o que dá para fazer, por ordem de ganho.

**Atualização (11/11, noite):** os itens 1, 2, 4, 5 e parte do 3 e do 6 foram feitos (ver `CONTINUAR_AQUI.md`, "Melhorias de FPS"); o 7 (travadas das criaturas) e juntar os materiais das casas não. Não foi medido de novo.

## Como foi medido

- PC: RTX 2060 SUPER, 16 GB. Editor em modo jogo (`-game`), 1920x1080, `Tools/RunRoamBench.ps1`.
- Cena principal: parado na praça da vila nova, girando a câmera (`-VortexRoamTown`), ao meio-dia.
- Ajustes salvos do jogador: qualidade Alta, DLSS em Desempenho (desenha em 960x540 e amplia), todos os shaders ligados (luz global Lumen, reflexos, neblina, bloom, motion blur), sombras em cascata, grama no máximo.
- 23 execuções, desligando uma coisa por vez, mais a captura por etapa do profiler da Unreal (`csvprofile` com `r.GPUCsvStatsEnabled 1`).
- **Ruído:** duas execuções iguais variam uns 10% (uma deu 76 FPS, as outras de 60 a 64). Há navegador, Discord, Steam e outros usando a placa ao mesmo tempo. Diferenças menores que 5 FPS não contam; os milissegundos por etapa são mais confiáveis que o FPS.

## O retrato de hoje (praça, 60 a 64 FPS)

O quadro leva 16,4 ms. Três coisas estão quase empatadas, e o FPS só sobe de verdade mexendo nas três:

| Limite | Tempo | O que pesa |
|---|---|---|
| Linha de desenho (CPU) | 16,4 ms | Lumen 4,7 ms; 1.870 chamadas de desenho |
| Placa de vídeo | 14,0 ms | Lumen 5,1 ms; DLSS 3,55 ms; sombras 1,0 ms |
| Linha do jogo (CPU) | 13,8 ms | moradores 4,4 ms; HUD 1,8 ms |

Placa de vídeo, etapa por etapa (ms): DLSS 3,55 · Lumen: luz na tela 2,07, luz da cena 1,66, atualização 0,54, reflexos 0,45, campo de distância 0,35 · sombras 1,0 · passe base 0,56 · iluminação 0,49 · motion blur 0,28 · pós 0,27 · nuvens 0,18 · neblina 0,11.

Chamadas de desenho: sombras 531, testes de oclusão 437, passe base 317, pré-passe 273.

## Testes desligando uma coisa por vez (praça)

| Teste | FPS | Jogo ms | Desenho ms | Placa ms |
|---|---|---|---|---|
| Base | 63,7 | 13,6 | 15,7 | 13,7 |
| Sem moradores | 65,5 | 9,2 | 15,3 | 13,5 |
| Sem a vila | 74,7 | 8,9 | 13,4 | 12,7 |
| Shaders desligados | 72,2 | 13,9 | 11,0 | 8,4 |
| Só a luz global Lumen desligada | 71,8 | 13,8 | 11,7 | 8,75 |
| DLSS com o modelo antigo (E) | 60,4 | 14,0 | 16,5 | 11,6 |
| DLSS desligado | 57,1 | 15,3 | 17,2 | 14,7 |
| Sem sombras | 67,2 | 13,4 | 14,7 | 11,8 |
| Sombras virtuais | 59,8 | 13,9 | 16,7 | 13,7 |
| Qualidade Baixa | 64,2 | 13,9 | 15,5 | 13,4 |
| Qualidade Épica | 61,1 | 14,1 | 16,3 | 14,2 |
| Sem luz de dentro das casas | 61,5 | 14,4 | 16,2 | 14,3 |
| Sem vidro nas janelas | 62,1 | 14,4 | 15,9 | 14,0 |
| Sem enfeites do mundo (nuvens, névoa, folhas, vento, fadas, bichos, horizonte) | 61,8 | 14,1 | 15,7 | 13,6 |
| Grama no mínimo (`-VortexGrassDensity=0`) | 47,9 | 19,0 | 16,4 | 13,8 |

Fora da vila (passeio padrão, a galope): base 63,3; shaders desligados 90,8; sem criaturas 61,9 (linha do jogo de 9,95 para 8,1 ms); grama no mínimo 64,2.

## O que fazer, por ordem

### 1. A qualidade Baixa e a Média desligarem o Lumen (ganho grande, mudança pequena)

- Hoje a Baixa dá 64 FPS e a Épica 61: a predefinição não mexe no que mais pesa. A luz global e os reflexos ficam numa aba separada (Shaders) e continuam ligados.
- Só a luz global Lumen custa 5,1 ms de placa e 4,7 ms de linha de desenho. Desligada: 72 FPS na praça (aí quem limita é a linha do jogo) e 91 fora da vila.
- Onde: `SetPreset` em `Source/Vortex/Private/Framework/VortexGraphics.cpp` (os níveis 0 e 1 desligarem `bShaderGI` e `bShaderReflections`).

### 2. DLSS com o modelo mais leve nas placas RTX 20 e 30 (2,8 ms de placa)

- O plugin (8.8, NGX 310.9) usa por padrão o modelo novo, que custa 3,55 ms nesta placa. Com `r.NGX.DLSS.Preset 5` (modelo E) custa 0,79 ms.
- Sozinho não sobe o FPS na praça, porque a linha de desenho segura em 16,4 ms; vira ganho junto com os outros itens e fora da vila.
- Risco: o modelo E é menos nítido em movimento. Precisa de uma foto lado a lado antes de virar padrão; o ideal é uma opção no menu.

### 3. Lumen mais barato na qualidade Alta (a medir)

- Tirar da cena do Lumen o que não precisa rebater luz: as 900 placas de chão, o meio-fio, os enfeites e os móveis (como já foi feito com a grama em 11/11).
- Baixar a taxa de atualização e a resolução do Lumen. Não medi: é o próximo teste.

### 4. Moradores: 4,4 ms da linha do jogo e metade dos triângulos

- Sem eles a linha do jogo cai de 13,6 para 9,2 ms e os triângulos de 1,57 para 0,79 milhão (contando as sombras).
- É o que vai limitar a praça em 72 FPS depois dos itens 1 a 3.
- Ideias: animar menos vezes por segundo quem está longe, não rodar o movimento de quem está parado no posto, pensar a rotina com intervalo maior, e quem está longe não fazer sombra.

### 5. HUD: 1,8 ms por quadro

- Atualizar o minimapa e a bússola algumas vezes por segundo, não a cada quadro.

### 6. Chamadas de desenho da vila (linha de desenho)

- A vila soma umas 680 chamadas. Cada casa tem 10 materiais, e cada material é uma chamada por tipo de casa em cada passe (pré-passe, base e cada cascata de sombra).
- Sombras: os móveis de dentro das casas, o meio-fio e os enfeites pequenos não precisam fazer sombra (as sombras são 531 chamadas e 810 mil triângulos).
- Juntar os materiais das casas num só é o maior ganho aqui, mas é trabalho grande.

### 7. Travadas (o pior 1%)

- Fora da vila o pior 1% fica em 13 a 20 FPS por travadas de 150 a 400 ms quando nascem criaturas: `Criaturas.Pensar` chegou a 329 ms e o Colosso nascendo a 91 ms. Pesa mais na sensação do que a média.

## O que não adianta mexer

- Triângulos das casas novas: o passe base custa 0,56 ms de placa. Os LODs ficam para depois.
- Luz de dentro das casas, vidro das janelas e os enfeites do mundo: desligados, o FPS não mudou.
- Sombras virtuais: ficaram um pouco piores que as em cascata.

## Para conferir

- **Grama no mínimo piorou** (47,9 FPS, linha do jogo em 19 ms, 57 quadros acima de 50 ms) e fora da vila não tirou triângulos. Foi uma execução só, pela chave de teste; pode ser erro da chave ou da opção do menu.
- Não medido: o jogo empacotado, a noite (os lampiões e as janelas acendem umas 100 luzes), combate com efeitos e outras placas de vídeo.

## Como repetir

```powershell
.\Tools\RunRoamBench.ps1 -ResX 1920 -ResY 1080 -Extra '-VortexRoamTown -VortexRoamSeconds=30' -Console 'r.GPUCsvStatsEnabled 1,csvprofile frames=2200'
```

O CSV sai em `Saved/Profiling/CSV`. Chaves do jogo para testar: `-VortexPreset=0..3`, `-VortexShaders=0/1`, `-VortexShadowMode=0/1`, `-VortexUpscaler=0..4`, `-VortexGrassDensity=0..3`, `-VortexNoPopulation`, `-VortexNoTown`, `-VortexNoCreatures`.
