# Desempenho profissional, menus no estilo da HUD e mapa em desenho (11/10/2026)

Situação: 🟡 pronto, com os testes automáticos passando; espera o seu teste no jogo instalado.

> **Importante:** o jogo instalado no seu PC ainda era a **primeira** versão do instalador (10/10, 16:06), com as sombras virtuais pesadas e nenhuma correção. Os 10 fps vinham dela. Instale o setup novo por cima.

## 1. Medir antes de mexer
- **O passeio de desempenho** (`Tools\RunRoamBench.ps1`) agora conta também os pedidos de desenho por quadro, os triângulos e quantos objetos de cada tipo estão montados.
  - Roda em qualquer resolução (`-ResX`/`-ResY`) e com cada preset (`-VortexPreset=0..3`), sem gravar nada.
- **Perfil com o Unreal Insights** da thread de renderização (preset Alto, 1080p), que mostrou para onde iam os ~11 ms do quadro:

| Parte | ms por quadro |
|---|---|
| Sombras (camadas do sol) | 2,3 |
| **Nanite** | 1,9 |
| TSR (suavização das bordas) | 1,6 |
| Desenho da cena | 1,8 |
| Luz, oclusão, nuvens e o resto | ~3 |

## 2. O que mudou, e quanto cada coisa valeu
Medido no passeio a 1080p, preset Alto:

| Mudança | FPS |
|---|---|
| Antes | 86 |
| **Sem Nanite nas malhas de cubos** | 114 |
| + sombras clássicas com 2–3 camadas, TAA, objetos agrupados por região, animação econômica | **116** |

- **Nanite desligado nas 117 malhas** (plantas, pedras, peças das vilas, a Yggdrasil, as espadas): `Tools\DisableNanite.py`.
  - O Nanite foi feito para modelos de milhões de triângulos. Nas nossas malhas de cubos, só o custo fixo dele comia um quarto do quadro.
  - Agora as malhas são desenhadas inteiras, do jeito normal, com a mesma aparência.
- **Plantas, pedras e grama agrupadas por região:** um grupo por tipo em cada região de 72 m, e não um por pedaço de 18 m. Os grupos na cena caíram de **2.955 para 673**.
  - O pedaço que sai tira só as suas cópias, sem refazer o grupo, e o carregador do terreno ficou mais leve: pior quadro de 13 para 8 ms.
- **Sombras clássicas:** 2 camadas nas qualidades baixas e 3 nas altas, com alcance um pouco menor. As sombras virtuais continuam como opção ("Detalhadas").
- **Suavização das bordas pelo preset:** FXAA no Baixo, TAA no Médio, TSR no Alto e no Épico.
- **Resolução dinâmica, ligada por padrão:** mira 60 fps e, quando o FPS cai, a resolução interna baixa sozinha (até 50%) e é reconstruída. Fica em **Gráficos → Resolução dinâmica / Alvo de FPS**.
- **Detecção do PC na primeira vez:** o jogo roda o teste de hardware da Unreal e escolhe o preset. Nunca começa no Épico.
- **Criaturas, lobos e moradores:** de longe, a animação atualiza menos vezes por segundo.
- **DLSS (autorizado e instalado em 11/10):** o plugin oficial da NVIDIA v8.8.0 (DLSS 4.5, para a UE 5.8), baixado de developer.nvidia.com/rtx/dlss.
  - Fica em `Plugins/DLSS` e `Plugins/StreamlineNGXCommon` e não vai para o git (1,3 GB, e a licença da NVIDIA não permite publicar). O jogo compila sem ele.
  - Nova opção **Gráficos → Super resolução:** TSR, DLSS Qualidade, DLSS Equilibrado, DLSS Desempenho ou DLAA. Só aparece em placas RTX.
  - Em placa RTX, o jogo começa no DLSS Qualidade. Com DLSS, a resolução dinâmica fica desligada, porque o modo do DLSS já fixa a resolução interna.
  - No passeio a 1080p (Alto): TSR 111 fps, **DLSS Qualidade 119 fps**, com o pior 1% indo de 26 para 35 fps.

## 3. Menus no estilo da HUD
O miolo das janelas era de painel azul liso com fio fino, com cara de site. Agora usa as peças da HUD (`Tools\MakeUiKit.py` ganhou peças novas):
- **Títulos** na faixa da HUD: placa com pontas, gemas e o título dourado no meio.
- **Linhas com relevo** nos atributos, estatísticas, missões, classes e opções. Nas opções, a linha acende com borda dourada sob o mouse.
- **Gemas coloridas** no lugar dos risquinhos dos atributos e das missões, e o selo de pontos dourado.
- **Letras com contorno escuro**, como na HUD, e uma divisória dourada com losango sob o cabeçalho.
- **Corrigido:** a coluna da raça transbordava a janela; agora o retrato é menor e a coluna rola.

| | |
|---|---|
| ![Atributos](../Images/alpha_1_1/atributos.jpg) | ![Mochila](../Images/alpha_1_1/mochila.jpg) |
| ![Opções](../Images/alpha_1_1/opcoes.jpg) | ![Grama e plantas sem Nanite](../Images/alpha_1_1/sem_nanite.jpg) |

## 4. O mapa (M) em desenho
- **Menor e centrado**, numa moldura no estilo da HUD, com o nome da região numa faixa e uma rosa dos ventos pequena no canto.
- **Sem a grade de letras e números.** Nomes das regiões só com o mapa inteiro; ruínas, pontes e cabanas só como ícone, com o nome aparecendo no zoom.
- **Lugares importantes** (cidade, vilas, covis, Yggdrasil) em selos redondos. O nome da cidade aparece sempre; os outros só com zoom ou perto de você.
- **Só o que importa:** moradores com missão (! e ?), objetivos piscando, chefes vistos, o grupo e você. Sumiram os pontinhos de todos os moradores e criaturas.
- **Visual de desenho:**
  - chão com a cor suavizada e chapada em três faixas de luz;
  - patamares de altitude a cada 25 m, com fio escuro na borda;
  - água em dois tons (raso turquesa, fundo azul), com espuma e contorno de tinta na costa;
  - o não explorado é um pergaminho claro hachurado.
- **Minimapa:** usa a mesma imagem, então também ficou em desenho.
- **Rodapé curto:** "Explorado X% • roda: zoom • M fecha".

| | |
|---|---|
| ![Mapa](../Images/alpha_1_1/mapa.jpg) | ![Mapa com zoom](../Images/alpha_1_1/mapa_zoom.jpg) |

## Opções com "Aplicar"
- **Mudanças pendentes:** com as opções abertas, elas ficam pendentes. Os valores aparecem na hora, mas nada muda no jogo até **APLICAR**.
- **Aplicar:** grava tudo e mostra a tela de carregamento (anel de runas) até o chão em volta remontar; aí ela some.
- **Descartar, ou fechar sem aplicar:** volta ao que estava.
- **Corrigido:** mudar a densidade da grama fazia o chão e a grama sumirem até andar. O carregador soltava os pedaços para remontar, mas só recalculava quando você mudava de pedaço; agora remonta na hora.
- **As telas não se refazem mais inteiras:**
  - nas opções, mudar um valor só atualiza aquela linha (a rolagem e o mouse ficam);
  - na janela do herói, a rolagem de cada lista volta para onde estava quando algo muda (um item, XP, um ponto gasto).
- **Teste novo:** `Tools\RunApplySmoke.ps1`. Mudança pendente sem tirar o chão, a tela de carregamento, o chão remontando com o herói parado e o DLSS ligando.

## O que conferir
1. Instale o setup novo e entre no mundo. Ligue **Interface → Mostrar FPS**: no Alto deve passar de 100 em 1080p, com picos só nos primeiros segundos.
2. As janelas (I/Tab), a pausa e as opções.
3. O mapa (M) e o minimapa.
4. Se algo ficou pior visualmente sem o Nanite: não deveria, porque as malhas são as mesmas, só desenhadas de outro jeito.
