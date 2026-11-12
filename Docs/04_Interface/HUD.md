# HUD do jogo

Status: **aguardando teste no editor**.

A HUD nova usa a arte do Godot (molduras douradas e azul-noite de `ui/art/hud` e ícones de `ui/art/icons`), que só foi lida. Tudo é desenhado no Canvas e escala com a altura da tela, com 1080p como base.

## O que mostra

**Herói (canto superior esquerdo):**
- retrato redondo com o rosto do próprio herói, capturado ao vivo (troca de roupa aparece);
- nome e selo de nível;
- vida, que escorre devagar e pisca abaixo de 30%;
- mana e vigor, com os números.

**Minimapa (canto superior direito):**
- O mapa é feito do próprio terreno do vale: cor do chão, sombra de relevo, contorno nos degraus, água pela profundidade e copas das árvores.
- Norte fica para cima.
- A seta dourada mostra para onde a câmera olha.
- Marcas: lobos em vermelho, NPCs em dourado, outros jogadores em azul.
- Na moldura: dia, nome da área ("Vale do Santuário") e relógio.
- A tecla **M** troca o alcance entre 40, 70 e 120 m, e a escolha fica gravada.

**Alvo (centro, em cima):**
- Aparece o lobo que está lutando com você, ou o último que você acertou.
- Mostra retrato, nível, vida e o que ele está fazendo: Caçando, Rondando, Mordida!, Bote!, Recuando…
- Em mordida, bote ou frenesi, a barra roxa pulsa.
- Some suavemente quando a luta acaba.

**Barra de ações (embaixo):** (refeita depois: ver `../02_Combate/ESPECIAL_ESPADA_AGUA_PARADA.md`)

| Ação | Tecla | Mostra |
|---|---|---|
| Espada | clique esquerdo | — |
| Defesa | clique direito | — |
| Esquiva | Ctrl | cargas, véu de recarga e fio da próxima carga |
| Recuperar | H | custo de 10 de mana; apagado sem mana ou com vida cheia |
| Correr | Shift | — |
| Pular | Espaço | — |

Cada ação acende enquanto está em uso.

**Números de dano:**
- dourados sobre quem você acerta;
- vermelhos sobre o herói quando ele perde vida.

**Outros:**
- A mira continua no centro.
- Avisos de sensibilidade do mouse e de alcance do minimapa.
- Console `Vortex.HUD 0` ou `1` esconde e mostra a HUD, e fica gravado.

## Arquivos

- `Source/Vortex/Private/Framework/VortexAimHUD.cpp` e `.h`: reescritos.
- `Tools/ImportHud.py`: importa as molduras e ícones para `/Game/Vortex/UI/Hud` e cria `M_HudPortrait` e `M_HudMinimap`.
- `VortexPlayerSettings`: `MinimapRadius`, `bShowHud` e o comando `Vortex.HUD`.
- `VortexCharacter`: tecla M.
- `VortexWolf`: o nível passa a ser replicado, para o cliente mostrar o nível certo.
- `Vortex.Build.cs`: módulos Slate e SlateCore, para medir o texto.
- Testes:
  - `-VortexHudShot` com `Tools/RunHudShot.ps1`: fotos de dia, em combate e à noite, com os retratos exportados;
  - `RunTerrainBench.ps1 -Hud`: mede o desempenho com a HUD na tela.

## Testes

- `RunHudShot.ps1`:
  - minimapa montado (1464×1464 células);
  - em combate, o alvo é o lobo, a vida caiu de 100 para 65 e a mana de 50 para 30;
  - número de dano 18 sobre o lobo;
  - retratos do herói e do lobo corretos.
- `RunWolfNetworkSmoke.ps1`: passou.
- `RunCombatSmoke.ps1`: passou.

## Desempenho

`RunTerrainBench -Profile alto -Hour 12`:

| | FPS | 1% low |
|---|---|---|
| Sem HUD | 65,8 | 51,2 |
| Com HUD | 64,8 | 48,6 |

- Os retratos são fotografados 2 a 2,5 vezes por segundo, em 256×256, só com o próprio boneco.
- O minimapa é montado uma vez, numa linha de processamento separada.

## Riscos e pendências

- **Fonte:** é a Roboto da engine. As de serifa do Windows (Georgia e outras) não podem ir junto com o jogo. Para ficar igual ao desenho do Godot, dá para usar a Cinzel, que é livre (OFL); basta trocar a fonte em `AVortexAimHUD::Label`.
- **Dados ainda inexistentes:** nome, raça, classe e nível do herói ainda não existem no Unreal. Por enquanto aparecem "Aventureiro", "Humano • Aventureiro" e nível 1.
- **Vigor:** a barra fina mostra o Vigor (Stamina), que por enquanto não muda.
- **Retratos sem luz:** mostram a cor do modelo, sem a luz da cena, para ficarem iguais de dia e de noite.
- **Partes do Godot que faltam:** lista de missões, grupo, buffs e botões de menu entram junto com esses sistemas.

## Como voltar atrás

Reverta este commit. A HUD antiga era só a mira, o relógio e o aviso de sensibilidade.
