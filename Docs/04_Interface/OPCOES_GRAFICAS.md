# Opções gráficas: shaders, água e gráficos avançados

**Pedido:** "tire o shaders do jogo mas coloque uma opção nas configurações que permite colocar ele, e coloque bem mais opções nas opções gráficas como a gráficas avançadas com tudo possível, ter níveis diferentes de água também".

O "sem shader" foi combinado como o visual simples, tipo Minecraft sem shader.

## Onde

- **Menu inicial:** Opções.
- **No jogo:** Esc → Opções.

As duas telas usam a mesma lista (`Framework/VortexGraphics`). Cada mudança é aplicada e gravada na hora. Os valores ficam em `GameUserSettings.ini`: as escolhas do Vortex na seção do `UVortexPlayerSettings` e as da Unreal na seção dela.

## Shaders (desligados por padrão)

**Shaders** liga ou desliga de uma vez estes seis efeitos:

| Efeito | Variável |
|---|---|
| Luz global do Lumen | `r.Lumen.DiffuseIndirect.Allow` |
| Reflexos do Lumen | `r.Lumen.Reflections.Allow` |
| Raios de sol | `r.LightShaftQuality` |
| Desfoque de movimento | `r.MotionBlurQuality` |
| Brilho das luzes (bloom) | `r.BloomQuality` |
| Névoa volumétrica | `r.VolumetricFog` |

- Na aba **Shaders → Cada efeito**, cada um liga à parte. Com uma mistura, a linha mostra "Personalizados".
- **Sem shaders**, os cantos ficam sombreados pela oclusão de ambiente (SSAO e oclusão do céu por campo de distância).
- As variáveis são postas "pelo código", acima dos grupos de qualidade da Unreal. Por isso continuam valendo quando a qualidade geral muda.

**Medido** no mesmo passeio do teste de desempenho, em 1280×720:

| | FPS | GPU | Thread de render |
|---|---|---|---|
| Sem shaders | **107** | 6,5 ms | 8,8 ms |
| Com shaders | 80 | 8,8 ms | 11,9 ms |

Fotos: `Saved/Screenshots/graficos_sem_shaders.png` e `graficos_com_shaders.png`.

## Qualidade da água

Os materiais vêm de `Tools/CreateVortexWater.py`:

| Nível | Material | Como fica |
|---|---|---|
| Baixa | `M_VortexWaterSimple` | Opaca, só a cor (raso/fundo em manchas que andam devagar) e marolinhas. A mais leve: 110 FPS no passeio. |
| Média | `M_VortexWater` com `Ripple=0` | Transparente pela profundidade, com espuma na margem, parada. |
| Alta (padrão) | `M_VortexWater` | A de antes, com marolinhas. |
| Épica | `M_VortexWaterEpic` | Refração do fundo, ondas em três camadas, brilho do sol mais forte. |

- Na alta e na épica, com os reflexos dos shaders ligados, a água também reflete pelo Lumen (camada da frente).
- A troca vale também para os lagos que já estão na tela.

## O painel de opções (segunda versão)

**Pedido:** "nas opções o gráfico baixo, médio etc. não diminuir só a geração de distância, isso continuar o mesmo e ter opção para qual a distância de visualização você quer, mas no baixo tipo menos densidade de vegetação, efeitos etc. ... e melhore o menu de opções, faça bem detalhado e organizado e fluido".

**O painel** (`Framework/VortexOptionsPanel`) é o mesmo no menu inicial e no Esc:
- **5 abas:** Tela, Gráficos, Shaders, Controles, Interface.
- **Cada linha tem:**
  - nome;
  - uma frase do que muda e quanto pesa;
  - `‹  valor  ›` (clicar no valor também avança);
  - marcadores do nível.
- **Animações:**
  - a linha sob o mouse acende, com um filete dourado;
  - o valor pisca e cresce um pouco ao mudar;
  - a aba nova entra deslizando, com o sublinhado crescendo.
- **Rolagem:** a posição se mantém ao mudar um valor.
- **Rodapé:** "Restaurar padrões desta aba".

**Qualidade geral** agora é um pacote do Vortex. Ela **não mexe na distância**: o mundo gerado é sempre o mesmo.

| Nível | Sombras, luz, reflexos, texturas, efeitos, pós, sombreamento, folhagem, AA | Vegetação | Água | Oclusão |
|---|---|---|---|---|
| Baixa | Baixa (sem sombras do sol) | 35% | Baixa | Desligada |
| Média | Média | 60% | Média | Ligada |
| Alta | Alta | Toda | Alta | Ligada |
| Épica | Épica | Toda | Épica | Ligada |

Se algo for mudado à mão, a linha mostra "Personalizada".

**Distância de visualização** (Curta / Média / Longa / Máxima) é à parte:
- Encurta até onde aparecem árvores, plantas e pedras (×0,45 / 0,65 / 0,85 / 1).
- Também ajusta o `sg.ViewDistanceQuality` (detalhe de longe da Unreal).
- A máxima é a de sempre, porque o chão gerado vai até ~110–126 m.

**Densidade da vegetação** (Baixa 35% / Média 60% / Alta 80% / Máxima):
- Cada tufo de grama entra ou sai sempre igual, decidido pelo lugar, então não fica piscando.
- Ao mudar, os pedaços do terreno à vista são remontados.

**Medido** no mesmo passeio em 720p, com a qualidade toda em Baixa, vegetação 35%, água baixa e sem shaders: **166 FPS** (GPU 4,0 ms). Em Alta, sem shaders, eram 107 FPS. Foto: `Saved/Screenshots/graficos_baixo.png`.

### O que tem em cada aba

| Aba | Opções |
|---|---|
| **Tela** | Resolução da tela, modo de tela, resolução interna (50–100%), limite de FPS (sem limite, 30–240), sincronia vertical, campo de visão (60°–110°) |
| **Gráficos** | Qualidade geral, distância de visualização, distância da grama, densidade da vegetação, qualidade da água, sombras, texturas, efeitos, folhagem, sombreamento, pós-processamento, iluminação global, reflexos, oclusão de ambiente, anti-aliasing (TSR/TAA/FXAA/desligado), qualidade do anti-aliasing, nitidez |
| **Shaders** | O pacote e cada efeito (luz global, reflexos, raios de sol, desfoque, bloom, névoa volumétrica) |
| **Controles** | Sensibilidade do mouse, sensibilidade mirando, inverter eixo vertical |
| **Interface** | Mostrar HUD, mostrar FPS, alcance do minimapa |

## Testes

- **Fotos com os shaders e a água forçados só naquela execução, sem gravar:**
  `-VortexShaders=0/1` e `-VortexWaterQuality=0..3`, por exemplo
  `$env:VORTEX_BENCH_ARGS='-VortexShaders=1 -VortexWaterQuality=3'; Tools/RunTerrainBench.ps1 -Shot -Hour 15`.
- **`-VortexOptionsShot`:** no `L_Menu`, tira uma foto de cada aba: `frontend_opcoes_0..4.png`.
- **`-VortexGrassDensity=0..3`:** força a densidade da vegetação só naquela execução.
- **Continuam passando:** `RunCameraSmoke`, `RunMenuFlowSmoke` e `RunCombatSmoke`.
