# Desempenho (FPS): o que foi otimizado

**Pedido:** "veja a questão do fps, veja o que já dá para otimizar".

## Como foi medido

`Tools/RunTerrainBench.ps1` faz o passeio na mata do protótipo, o mesmo trajeto do `--bench-wild` do Godot. Usa a qualidade do projeto (Épica) e o HUD ligado.

- **Perfil detalhado:** com `$env:VORTEX_BENCH_PROFILE=1`, no meio do passeio o `ProfileGPU` e a média dos contadores de render (`stat dumpave`) vão para `Saved/Logs/TerrainBench_alto.log`.
- **Outras resoluções e opções:** `-ResX 1920 -ResY 1080` muda a resolução; `-Extra 'r.ScreenPercentage 100'` compara opções.

## O que o perfil mostrou

| Onde | Custo | Causa |
|---|---|---|
| Render | 1,6 ms | O **sol era recriado na cena a cada quadro**: raios, disco, ângulo e brilho eram reaplicados todo quadro. Cada recriação também joga fora o cache das sombras virtuais (2,4 ms de GPU). |
| Render | 2,3 ms | A **pisada da grama** mandava 8 posições de pé, todo quadro, para o material compartilhado por toda a grama. A Unreal refazia os comandos de desenho de cada moita. |
| GPU, em 720p | ~5 ms | Lumen (luz indireta) |
| GPU, em 720p | 2,4 ms | Sombras virtuais |
| GPU, em 720p | resto | Base, nuvens, pós-processo |

A GPU cresce com o número de pixels: em 1080p ela era o gargalo.

## O que mudou

1. **Sol:** gira em passos de 0,3° (já era assim). Raios, disco, ângulo e brilho do sol, luminância do céu e cor da névoa agora só são aplicados junto com o giro, no nascer/pôr do sol ou quando o valor muda de verdade. `UVortexDayNightSubsystem::Apply`.
2. **Pisada da grama:**
   - as posições dos pés passam por uma **Material Parameter Collection** (`/Game/Vortex/Materials/MPC_GrassStep`, criada por `Tools/CreateGrassMaterial.py`);
   - só são mandadas quando um pé anda mais de 1 cm;
   - a grama usa o `M_GrassWind` direto (sem material dinâmico).
3. **Resolução interna com TSR:**
   - na primeira vez, o jogo renderiza em **75%** da resolução da tela e o TSR reconstrói a imagem, que fica quase tão nítida;
   - em **Opções → Vídeo → Resolução interna (TSR)** dá para escolher 100 / 85 / 75 / 67 / 50%.
4. **Contador de FPS:** em **Opções → Interface → Mostrar FPS** (canto inferior direito do HUD).

## Resultado (mesmo trajeto, HUD ligado, qualidade Épica)

| Resolução | Antes | Depois |
|---|---|---|
| 1280×720 | 66 FPS (1% pior 51) | **79 FPS** (1% pior 55) |
| 1920×1080 | 61 FPS (GPU 15,2 ms) | **76 FPS** (GPU 11,6 ms) |

Em 720p a resolução interna quase não muda o FPS, porque lá quem limita é o render. O ganho dela aparece em 1080p e acima.

## O que ainda dá para ganhar (próximos passos)

- **A thread de render** fica em ~12 ms: é o custo base do motor neste executável de editor. Num jogo **empacotado** (Development/Shipping) ela cai bastante. **No editor (PIE) o FPS é sempre mais baixo**: para medir de verdade, use "Standalone Game".
- **Lumen:** "Qualidade geral: Alta" em vez de "Épica" reduz a luz indireta e as sombras. Medido: GPU de 13,8 → 9,7 ms em 720p.
- **Testes já feitos e descartados:** a floresta viva (borboletas, vaga-lumes) e o HUD pesam pouco (≈0,3 ms cada).

## Segunda rodada: Alta e Épica

**Pedido:** "veja oq mais consegue melhorar o fps no alto e epico".

### Onde o tempo ia

Perfil da GPU no Épico em 1080p (`VORTEX_BENCH_PROFILE=1`):

| Item | GPU |
|---|---|
| Pós-processo | 3,6 ms |
| ↳ TSR | 2,8 ms |
| ↳ ↳ histórico do TSR, guardado em **3840×2160** (o padrão do Épico é 200%) | 1,3 ms |
| Luz indireta do Lumen | 1,8 ms |
| Sombras virtuais | 1,1 ms |
| Projeção das sombras | 0,5 ms |
| Campo de distância ("Update Movable") | 0,5 ms |

### O que mudou

1. **`Config/DefaultScalability.ini`**, novo. Contém só as diferenças em relação à Unreal:
   - **Épica:** o histórico do TSR fica em 100%, como no Alto (`r.TSR.History.ScreenPercentage`). No estilo de blocos não se nota diferença.
   - **Épica:** as sombras do sol ficam com detalhe −0,5 em vez de −1,5 (`r.Shadow.Virtual.ResolutionLodBiasDirectional`). Ainda são mais nítidas que no Alto (0) e precisam de menos páginas de sombra.
2. **Arma na mão** (`WeaponVisual` e `WeaponVisualLeft`): saiu do campo de distância e da luz indireta. É pequena e se mexe o tempo todo, e fazia o campo ser atualizado a cada quadro, multiplicado por cada herói na sala.

### Resultado (mesmo passeio, 1920×1080, HUD ligado)

| | Antes | Depois |
|---|---|---|
| Épica, GPU | 11,6 ms | **10,2 ms** (−12%) |
| Épica, FPS | 76 | 77 |
| Alta, GPU | — | 10,3 ms |

### Por que o FPS quase não subiu nesta máquina

- **O limite aqui é a thread de render**, não a GPU. Ela fica em ~13 ms em qualquer qualidade, o que dá cerca de 77 FPS no máximo.
- **Pelo perfil:**
  - o próprio renderizador gasta ~3 ms;
  - a atualização de primitivas gasta 2,4 ms (1,8 ms disso é espera);
  - a montagem do RDG gasta 1,4 ms;
  - o Slate e o Canvas gastam ~3,5 ms.
- Grande parte disso é o executável de **editor** (`UnrealEditor -game`). Num jogo **empacotado** essa thread cai bastante.
- **Quem ganha com a mudança:** placas de vídeo mais fracas (onde a GPU é o gargalo) e resoluções acima de 1080p.
- **Próximo passo** para medir de verdade: um pacote Development fora do editor. O HUD foi medido de novo: 77,5 FPS sem ele contra 76,3 com ele, ou seja, pesa pouco.
