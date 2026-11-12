# Sensibilidade do mouse e distância de aparição do cenário (aguardando teste no editor)

Pedidos do jogador: a câmera girava rápido demais; plantas e grama surgiam perto, e dava para ver o mundo se montando. A névoa da Fase 12 foi aprovada e ficou como estava.

## Sensibilidade do mouse

- `UVortexPlayerSettings` guarda as preferências de cada jogador na máquina, em `Saved/Config/<plataforma>/GameUserSettings.ini`, na seção `/Script/Vortex.VortexPlayerSettings`. O `DefaultInput.ini` não foi alterado.
- **Sensibilidade:** padrão **0,6** (1 = velocidade antiga), de 0,1 a 3.
- **No jogo:** **`[`** diminui e **`]`** aumenta, em passos de 0,1; o valor aparece na tela por 2 s.
- **Ao mirar** (botão direito), a sensibilidade é multiplicada por **0,7**.
- **Eixo vertical** invertível.
- **Console:** `Vortex.Mouse.Sensibilidade 0.5`, `Vortex.Mouse.Mira 0.7`, `Vortex.Mouse.InverterY 1`. Sem número, o comando mostra o valor atual.
- **Conferido:** o comando gravou 0,5 no `GameUserSettings.ini` e leu o valor de volta; depois o padrão 0,6 foi restaurado.

## Aparição do cenário

| | Antes | Agora |
|---|---|---|
| Grama com vento | some a 30 m (começa a sumir a 24 m) | **45 m** (36 m) |
| Plantas pequenas, flores, trigo | 32 m | **60 m** |
| Moitas e pedras | 58 m | **90 m** |

A névoa engrossa a partir de ~70 m, então as plantas passam a surgir já veladas.

**Entrada no jogo:** quem joga nesta máquina vê a tela escura até não faltar nenhum pedaço da área. Nesse período, o streamer monta 6 pedaços por quadro em vez de 1; depois a tela clareia em 0,6 s. A espera medida foi de **1,15 s**, com limite de segurança de 15 s. O servidor dedicado não é afetado. Testes que precisam da imagem desde o início usam `-VortexNoInitialFade`.

## Desempenho

`RunTerrainBench -Profile alto`, executado sozinho:

| | FPS | 1% pior | pior quadro |
|---|---|---|---|
| Antes | 74,5–74,8 | 56,8–57,4 | 22,1 ms |
| Agora | 73,8–74,0 | 56,5–56,7 | 22,0–22,5 ms |

O custo é de ~0,8 FPS, sem quadro acima de 25 ms. A memória do processo ficou igual (pico privado ~6,15 GB).

## Testes

- Compilação: passou.
- `RunCameraShot`: a foto sai normal; a tela escura acaba antes.
- A troca da sensibilidade pelo console foi gravada e lida.

## Riscos e reversão

- As teclas `[` e `]` e a sensação da sensibilidade precisam do teste do jogador; o HUD só mostra o aviso para o herói local.
- Mais grama na tela custa um pouco de GPU. Em máquinas mais fracas, reduza `GrassRange`, `SmallPropRange` e `PropRange` no ator do streamer.
- Um teletransporte longe, depois da entrada, ainda mostra a área se montando: a tela escura vale só para a primeira entrada.
- Reversão: reverter este commit volta à sensibilidade fixa e às distâncias antigas.
