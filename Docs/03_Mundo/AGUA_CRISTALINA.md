# Água cristalina dos lagos (aguardando teste no editor)

Pedido do jogador: água cristalina e reluzente, realista, que combine com o jogo e não pese.

## O que mudou

- **Base:** o material `M_Lake` do pacote **WaterMaterials**, que o jogador pôs em `VortexReferencia`. O pacote foi trazido para `Content/WaterMaterials`, com materiais, texturas, malhas e partículas; os mapas de demonstração ficaram de fora. É um material translúcido com ondas por textura (normais grande, média e fina), espuma e reflexo de Fresnel.
- **`MI_VortexLake`** (`Tools/CreateVortexLake.py`): versão própria, sem mexer no original. O `M_Lake` é quase incolor, feito para lagos fundos, e os lagos do vale são rasos (5–75 cm). A versão do Vortex tem:
  - tom turquesa no raso e azul-petróleo no fundo;
  - um pouco mais de presença;
  - mais brilho do sol nas ondinhas;
  - superfície mais lisa.
- **Reflexo do céu, das nuvens e das margens:** `r.Lumen.TranslucencyReflections.FrontLayer.Enable=1` em `DefaultEngine.ini`. Na medição, ligar o reflexo não teve custo mensurável.
- **Malha da água:** cada vértice ganhou normal e UV na escala do mundo (uma repetição a cada 10 m), então as ondas têm o mesmo tamanho em todo lago, sem emenda entre células e pedaços. Para testar outra água: `-VortexWaterMaterial=<caminho>`, por exemplo `/Game/WaterMaterials/Materials/M_Lake_Cheaper.M_Lake_Cheaper`.
- Uma primeira tentativa com o modelo Single Layer Water, feita do zero, não compilou nesta máquina e foi descartada.

## Fotos

| | |
|---|---|
| Margem, 10:30 | [foto](../Images/agua_lago_1030.png) |
| Margem, 17:18 | [foto](../Images/agua_lago_1718.png) |
| Vale, 17:42 | [foto](../Images/agua_1742.png) |
| Noite, 01:00 | [foto](../Images/agua_0100.png) |

## Desempenho (`RunTerrainBench -Profile alto -Hour 12`)

| | FPS | 1% pior |
|---|---|---|
| Água simples (antes) | 70,2–70,4 | 53,2–55,3 |
| Água cristalina | 66,9 | 53,9 |

A água custa ~3,5 FPS no modo alto. A primeira execução teve um pico de 45 ms, provavelmente da compilação inicial do shader novo; a segunda saiu limpa. Se pesar, o `M_Lake_Cheaper` do mesmo pacote usa luz por vértice e um mapa de reflexo fixo, mais barato, mas sem refletir as árvores.

## Testes

- `RunMovementSmoke -Matrix` (inclui nado e saída na plataforma): passou.
- `RunDayNightShot`: ganhou duas vistas do lago de perto (`lago_1030`, `lago_1718`).

## Riscos e reversão

- A licença de uso é a do Fab, do pacote do jogador.
- Rios, cachoeiras e espuma de margem por partícula do pacote ainda não foram usados.
- Reversão: reverter o commit. A água volta ao `M_Water` simples, e o pacote sai de `Content`.
