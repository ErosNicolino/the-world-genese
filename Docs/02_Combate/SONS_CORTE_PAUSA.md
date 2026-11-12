# Sons do herói, corte à distância novo e opções na pausa

**Pedido:** "o som do herói não funciona (andar, rolar, atacar)", "o golpe especial parece um PNG voando reto, queria um efeito legal" e "coloque as opções no Esc dentro do jogo".

## Sons do herói

- **Causa:** o `Tick` do herói ficava **desligado**. Só ligava durante a transição da câmera (zoom, mira) e desligava de novo no fim. Passos, rolamento e golpes tocam no `UpdateSounds`, chamado pelo `Tick`, então quase nunca tocavam.
- **Correção:** o `Tick` fica sempre ligado (`PrimaryActorTick.bStartWithTickEnabled = true`). Os pontos que o desligavam foram removidos.

## Corte à distância

Antes eram três placas planas com a textura da lua. Agora:

- **Arco de energia em 3D:** uma fita curva gerada em código (`BuildArc` em `VortexSlashWave.cpp`), com o meio à frente e as pontas curvadas para trás, inclinada na diagonal do golpe.
- **Material `M_SlashArc`** (`Tools/CreateSlashArc.py`), aditivo:
  - borda da frente quase branca;
  - corpo azul com fiapos de energia correndo ao longo do arco;
  - pontas finas.
- **Camadas:**
  - **núcleo** fino e brilhante;
  - **brilho** largo em volta;
  - **4 cópias** que ficam para trás e se apagam (o rastro).
- **Faíscas:** `NS_Sparkling_Glow` do pacote `FreeParticle_SoftTofu`, copiado das referências.
- **Luz azul** acompanhando o corte.
- **No acerto:** o arco se abre num clarão e some.
- **Correção:** um `NaN` nas pontas do arco (cosseno negativo minúsculo numa potência fracionária) escondia a malha inteira.
- **Pacotes copiados das referências** (mesmo motor, 5.8): `ArrowTrail` (rastros de flecha por elemento), `Basic_VFX` e `FreeParticle_SoftTofu`.

## Opções no Esc

`Framework/VortexPauseMenu`: menu de pausa no estilo do Vortex.

- **Continuar**, **Salvar mundo** e **Voltar ao menu**.
- O código da sala online, quando há uma.
- **Opções**, aplicadas e salvas na hora:

  | Grupo | Opções |
  |---|---|
  | Controles | sensibilidade do mouse, inverter eixo |
  | Vídeo | qualidade geral, resolução interna (TSR), sincronia vertical, modo de tela |
  | Interface | HUD, FPS, raio do minimapa |

## Como testar

- **No jogo:** andar, rolar (Ctrl) e atacar com som; botão direito para o corte; Esc para as opções.
- **Automático:** `Tools/RunHudShot.ps1` tira as fotos `hud_especial` (o arco acertando o lobo) e `hud_pausa`.
