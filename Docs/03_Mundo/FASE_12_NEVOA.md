# Fase 12 — névoa de distância e limites do vale (aguardando teste no editor)

## O que mudou e por quê

- **Névoa que fecha o horizonte.** O terreno só é montado até 6 pedaços (~110–126 m) em volta do jogador. Sem névoa, a borda aparece como uma faixa preta no horizonte, com árvores soltas sobre o vazio ([sem névoa](../Images/fase12_borda_sem.png)). A primeira tentativa (densidade 0,028) quase não aparecia perto do herói e deixava a linha onde as árvores terminam ([antes do ajuste](../Images/fase12_borda_antes_ajuste.png)). Os valores finais do `Terrain_Fog` no `L_TerrainProto` são: densidade **0,32**, início a **45 m**, queda de altura **0,35** e opacidade máxima **1,0**. A cor azul-acinzentada foi mantida. O resultado deixa nítidos os primeiros ~45 m, esfumaça a mata a partir de ~70 m e fecha a névoa antes da borda. A queda de altura afina a névoa para cima, e por isso o céu acima do horizonte mantém algum azul.
- **Referência FogArea.** O pacote `VortexReferencia/Content/FogArea` (volume local com ruído, sombras e feixes) serviu só de referência para o véu suave e frio. **Nenhum asset dele foi copiado.** A névoa usa a Exponential Height Fog nativa, que é mais barata e não exige material próprio.
- **Árvores até 150 m (eram 300 m).** Além do chão montado, as copas ficavam soltas no vazio, e a névoa já as escondia. O `TreeRange` passou para 15 000 cm. Esse corte também explica o ganho de desempenho medido abaixo.
- **Limites reais do vale.** `FVortexTerrainData::PrototypeBounds()` e `ContainsWorldXY()` informam a área exportada: X −90 000..19 800 cm, Y −54 000..55 800 cm, em 61×61 pedaços de 18 m. O carregador agora recusa um `terrain.bin` cujas dimensões ou cujo tamanho não batam com o cabeçalho, para não ler fora do arquivo.
- **Fotos automáticas esperam o chão.** `RunCameraShot` e `RunCameraSequenceShot` mantêm o herói parado no ar até a colisão do ponto existir. Antes, uma captura saiu com o herói caindo no vazio.

## Fotos (mesmos pontos, com e sem névoa)

| Vista | Com névoa | Sem névoa |
|---|---|---|
| Vale, altura do herói | [foto](../Images/fase12_vale_com.png) | [foto](../Images/fase12_vale_sem.png) |
| Início do passeio olhando a borda oeste, 15 m de altura | [foto](../Images/fase12_borda_com.png) | [foto](../Images/fase12_borda_sem.png) |
| Ponto mais alto do protótipo (−750, −480) m, 22 m de altura | [foto](../Images/fase12_alto_com.png) | [foto](../Images/fase12_alto_sem.png) |

Câmera de jogo perto do lago: [depois](../Images/fase12_neblina_depois.png), comparada com [antes da névoa](../Images/camera_after_idle.png).

## Testes

- Compilação `VortexEditor Win64 Development`: passou.
- `RunRegionSmoke.ps1`: passou (limites, 61×61 pedaços, chão na origem).
- `RunFogShot.ps1` (novo): grava as três vistas com e sem névoa. Os parâmetros `-Density`, `-Start`, `-Opacity` e `-Falloff` testam outros valores sem regravar o mapa. Os valores finais ficam em `TuneDistanceFog.py`, que foi executado e gravou o mapa.
- `RunCameraShot.ps1`, `RunCameraSmoke.ps1` e `RunSolidWalkSmoke.ps1` (nove obstáculos): passaram.

## Desempenho (`RunTerrainBench.ps1 -Profile alto`, aquecido, 1280×720, DX12)

| Situação | FPS médio | 1% pior | pior quadro |
|---|---|---|---|
| Antes da névoa | ~71,4 | ~48,8 | — |
| Névoa 0,028, árvores 300 m | 69,2 | 47,2 | — |
| **Final (3 execuções)** | **74,5–74,9** | **53,1–53,4** | 24,4–24,6 ms, nenhum > 25 ms |

Lumen, reflexos Lumen, sombras virtuais e Nanite continuaram ativos (`gi=1 reflexos=1 sombras_virtuais=1 nanite=1`). O custo da névoa de altura é desprezível. O ganho vem de deixar de desenhar árvores entre 150 e 300 m.

## Riscos

- O céu fica mais cinzento que antes, em clima de dia enevoado. Se o jogador preferir um céu mais limpo, há duas opções: reduzir a densidade, o que mostra mais a borda, ou aumentar o raio do terreno, o que custa desempenho. O ajuste é feito em `TuneDistanceFog.py`.
- O alcance de montagem é quadrado; na diagonal o chão vai além de 126 m, então a névoa cobre mais do que o necessário nessas direções. Mudar `StreamRadius` exige revisar a névoa.
- A troca de nível das árvores aos 150 m é instantânea, mas fica dentro da névoa fechada. No editor, convém conferir em movimento rápido.
- Em pontos muito altos, a névoa fica mais fina, por causa da queda de altura. O ponto mais alto do protótipo, a 16,75 m, foi conferido.

## Reversão

Reverter o commit desta fase restaura o mapa com a névoa anterior, o `TreeRange` de 300 m e o carregador sem a validação de tamanho. Para mudar só a névoa, edite as constantes de `Tools/TuneDistanceFog.py` e rode o script com o editor fechado:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' "<caminho>\Vortex.uproject" -run=pythonscript "-script=<caminho>\Tools\TuneDistanceFog.py" -unattended -nullrhi
```

## Avaliação pendente no editor

Confira se a distância nítida (~45 m) e o tom do céu agradam, se a borda do terreno ou alguma árvore aparece ao correr e girar a câmera, e repita o teste em cima de um morro. **A fase não está declarada concluída antes desse teste.**
