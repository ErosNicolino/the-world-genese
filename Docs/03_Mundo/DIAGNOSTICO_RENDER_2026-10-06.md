# Diagnóstico do limite de FPS — 06/10/2026

## O que foi verificado e por quê

O pedido foi identificar o maior custo atual antes de reduzir grama, sombras ou Lumen. Repeti o passeio da mata em 1920×1080, DX12, qualidade Épica, sol às 12h e HUD ligado, com os mesmos parâmetros e sem limite de FPS. Uma execução ativou `ProfileGPU` e estatísticas de render; a outra deixou esses instrumentos desligados.

| Execução | FPS médio | 1% pior | Jogo | Render | GPU | pior quadro |
|---|---:|---:|---:|---:|---:|---:|
| Diagnóstico detalhado | 64,6 | 22,9 | 12,61 ms | 6,79 ms | 10,34 ms | 170,7 ms |
| Passeio normal | **79,5** | **51,9** | **3,98 ms** | **12,54 ms** | **9,97 ms** | **26,2 ms** |

O diagnóstico detalhado altera o resultado: os contadores tornam a thread do jogo cerca de 8,6 ms mais lenta e geram 34 quadros acima de 25 ms, contra um no passeio normal. Os FPS dessa execução servem para localizar trabalhos, não para comparar qualidade. `RunTerrainBench.ps1` agora grava o perfil em `TerrainBench_alto_profile.log`, preserva o log do passeio normal e avisa sobre o custo dos contadores.

## Conclusão prática

No passeio normal, o render é o limite: 12,54 ms, contra 9,97 ms de GPU. A GPU tem cerca de 2,6 ms de folga. O perfil anterior de 1080p já havia localizado pós-processamento/TSR (3,6/2,8 ms), Lumen (1,8 ms) e sombras virtuais (1,1 ms), mas a otimização de GPU daquela rodada pouco mudou os FPS. Portanto, reduzir visual agora tem baixa chance de ganho perceptível nesta máquina. O HUD isolado já foi medido em cerca de 1 FPS e o streamer em 0,54 ms por quadro em média nesta rodada.

## Limites, desempenho e próximos testes

Esses números são do executável `UnrealEditor -game`, não de um pacote final. A compilação do alvo `Vortex Win64 Development` passou. A tentativa de cozinhar um pacote foi interrompida durante a compilação longa de shaders globais, inclusive Vulkan; portanto não há comparação válida com executável empacotado. Antes de mudar a arquitetura dos pedaços ou cortar qualidade, conclua uma medição empacotada com o mesmo mapa e dados externos incluídos, e verifique visualmente a equivalência. O passeio atual usa região fixa de teste, sem medir uma cidade cheia nem oito jogadores.

## Reversão

Reverter este commit restaura apenas o nome do log e a mensagem do script de medição. Nenhum ajuste de render do jogo foi alterado.
