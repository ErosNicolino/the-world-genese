# Grama, nuvens e céu realistas (aguardando teste no editor)

**Direção de arte** (pedido do jogador): o Vortex é "quadrado com meios realistas". Terreno, árvores, personagens e criaturas ficam em blocos; grama, céu, nuvens, luz e shaders são realistas. Por isso, as nuvens de voxel portadas do Godot foram trocadas pelas nuvens volumétricas da Unreal, e a grama foi refeita.

## Grama

**Antes** (fotos `grama_antes_*`): poucas folhas largas por tufo, só 6 formatos iguais e tufos espaçados sobre o chão verde-limão liso. A cor (oliva escuro e palha) não combinava com o chão, e o resultado parecia tufos colados num tapete.

**Agora:**
- **12 variantes de touceira, em três tipos:**
  - rasteira larga e cheia (64–82 folhas em ~60 cm);
  - de prado (30–40 folhas);
  - alta com espigas (14–20 folhas).
- **Folhas** finas, de 3 segmentos, curvas (a ponta tomba), afinando até a ponta, cada uma com altura, largura, giro e brilho próprios; algumas bem mais altas que as vizinhas.
- **A cor nasce do chão:** cada cópia recebe a cor do chão naquele ponto (dados por cópia do ISM). A raiz é escura e sombreada, o corpo tem a cor do chão e a ponta é mais clara e amarelada.
- **Sem repetição:**
  - manchas grandes no mundo, de dezenas de metros, e a "secura" de cada região puxam para palha;
  - cada touceira e cada folha variam de tom;
  - o vento tem fase própria por touceira.
- **Distribuição:** o campo decide os tipos (rasteira em todo lugar, de prado nas manchas densas, alta rara). A rasteira só aparece até ~20 m, onde a densidade importa; a de prado e a alta, até 45 m.
- **Custo controlado:** cada pedaço de terreno usa 6 das 12 variantes (2 rasteiras, 3 de prado e 1 alta), sorteadas pelo pedaço. Assim a variedade aparece entre pedaços, com o mesmo número de chamadas de desenho de antes.

| Vista | Antes | Depois |
|---|---|---|
| Rente ao chão | [foto](../Images/grama_antes_rente.png) | [foto](../Images/grama_depois_rente.png) |
| Câmera do jogo | [foto](../Images/grama_antes_jogo.png) | [foto](../Images/grama_depois_jogo.png) |
| Aberta | [foto](../Images/grama_antes_aberta.png) | [foto](../Images/grama_depois_aberta.png) |
| Fim da tarde | [foto](../Images/grama_antes_tarde.png) | [foto](../Images/grama_depois_tarde.png) |

O código está em `AVortexTerrainStreamer` (`AddGrassClump`, distribuição e dados por cópia) e em `Tools/CreateGrassMaterial.py` (cor e vento). As fotos são de `Tools/RunGrassShot.ps1` (novo).

## Nuvens, céu e horizonte

- **Nuvens:** volumétricas da Unreal (`VolumetricCloud`), iluminadas pelo mesmo sol e pela mesma lua do ciclo: brancas ao meio-dia, douradas no nascer e no pôr do sol, prateadas à noite com estrelas entre elas. As nuvens de voxel e o `M_VoxelCloud` foram removidos. As amostras por raio foram reduzidas (vista 0,5, reflexos 0,25), o que custa quase nada sem diferença visível.
- **Névoa só até 1 km:** o terreno continua sumindo na névoa como na Fase 12, mas o céu, as nuvens e as estrelas não recebem mais a névoa. Antes, a névoa deixava o céu branco e escondia as nuvens.
- **Névoa com a cor do céu:** a névoa é iluminada pelo próprio SkyAtmosphere, então a mata ao longe tem o tom do céu da hora; a cor fixa virou só um toque.
- **Saia do horizonte:** sem névoa além de 1 km, a faixa abaixo do horizonte, depois do fim do terreno, mostraria o chão escuro do céu. Um plano de 20 km sob o terreno acompanha a câmera, com a cor do próprio céu logo acima do horizonte (`Tools/CreateHorizonSkirt.py`, `M_HorizonSkirt`).
- **Fim da tarde natural:** o sol da Unreal já avermelha ao atravessar a atmosfera. O laranja manual do Godot dobrava o vermelho e foi removido do sol, da luz ambiente e dos raios. A exposição também abre até 2× na hora dourada, como o olho se adapta; meio-dia e noite ficam como estavam.

**Fotos:**
- **Céu:** [meio-dia](../Images/fase15_nuvens_1200.png), [17:36](../Images/fase15_nuvens_1736.png), [21:00](../Images/fase15_nuvens_2100.png), [lua cheia](../Images/fase15_lua_cheia.png).
- **Vale:** [12:00](../Images/fase15_1200.png), [17:42](../Images/fase15_1742.png), [06:12](../Images/fase15_0612.png).
- **Olhando o sol:** [07:00](../Images/fase15_sol_0700.png), [17:18](../Images/fase15_sol_1718.png).
- **Horizonte visto do alto:** [foto](../Images/ceu_horizonte_alto.png).

## Desempenho (`RunTerrainBench -Profile alto -Hour 12`)

| | FPS | 1% pior |
|---|---|---|
| Antes (grama antiga, nuvens de voxel) | 73,5 | 56,0 |
| Grama nova, sem nuvens | 72,0–72,1 | 55,7–56,0 |
| Grama nova + nuvens volumétricas (amostras reduzidas) | 70,2–70,4 | 53,2–55,3 |

À noite o custo é menor. Nenhum quadro passou de 25 ms nas medições finais.

## Testes

- Compilação: passou.
- `RunDayNightSmoke`, `RunWolfSmoke`, `RunCameraSmoke`, `RunStreamNetworkSmoke`, `RunSolidWalkSmoke` e `RunPopulationSmoke`: todos passaram.

## Riscos e o que avaliar

- **Horizonte visto do alto:** de morros ou de cima das árvores ainda se vê uma linha suave entre o chão distante e o céu, por causa das nuvens perto do horizonte. Ao nível do herói as árvores escondem.
- **Amanhecer cedo** (06:12) ainda fica escuro na mata.
- **Céu do meio-dia** levemente esverdeado sob a exposição fixa: avaliar no editor.
- **Grama:** as distâncias e densidades podem ser ajustadas no ator do streamer (`GrassRange`) e no código das touceiras, se ficar pesada em máquinas mais fracas.
- **Reversão:** reverta o commit. A grama e o céu antigos voltam juntos.
