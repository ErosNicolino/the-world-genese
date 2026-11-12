# Água estilizada (lagos e poças)

**Pedido:** água menos realista e com o fundo mais opaco quanto mais fundo.

## O que mudou

- Novo material próprio, `/Game/Vortex/Materials/M_VortexWater`, criado por `Tools/CreateVortexWater.py`. O `VortexTerrainStreamer` usa ele em todos os lagos.
- A realista `MI_VortexLake` (`Tools/CreateVortexLake.py`) continua de reserva. Para comparar: `-VortexWaterMaterial=/Game/Vortex/Materials/MI_VortexLake.MI_VortexLake`.
- A espessura de água entre a superfície e o chão (profundidade da cena menos a do pixel) decide tudo:
  - **cor:** turquesa vivo no raso, que fecha num azul-petróleo no fundo;
  - **opacidade:** 0,62 no raso, 0,985 no fundo. O fundo some com a profundidade, e a borda some nos primeiros 1,5 cm, sem serrilhado;
  - **espuma:** uma faixa clara (≈12 cm) onde a água encosta na margem, com o contorno mexendo devagar.
- **Superfície:** quase lisa, com marolinhas por fórmula (sem textura, então não tremem ao girar a câmera). Fosca: rugosidade 0,22, especular 0,2.
- **Luz:** por pixel (Surface ForwardShading).

## Parâmetros

Ajustáveis no material ou numa instância:

| Parâmetro | Padrão |
|---|---|
| `Shallow` | turquesa |
| `Deep` | azul-petróleo |
| `Foam` | claro |
| `DepthScale` | 45 cm até o fundo |
| `OpacityShallow` | 0,62 |
| `OpacityDeep` | 0,985 |
| `FoamWidth` | 12 cm |
| `Ripple` | 1 |

## Notas técnicas

- As ondulações usam o UV da água, que já é a posição no mundo dividida por 10 m, e não a `WorldPosition`. Com `WorldPosition` (LWC) ligada no Custom, a água ficava invisível sem erro no log.
- Os nomes `Gold`, `Cream` e `Muted` do HUD viraram `HudGold`, `HudCream` e `HudMuted`, e o `Night` sem uso do menu saiu. No unity build eles colidiam com os do menu.

## Como testar

- No editor: chegar perto do lago do vale. A beirada fica clara e transparente, o meio escuro e opaco, sem brilho espelhado forte.
- `Tools/RunDayNightShot.ps1 -Only lago` tira as fotos `daynight_lago_1030` e `_1718`.
