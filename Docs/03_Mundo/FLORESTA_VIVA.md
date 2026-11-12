# Floresta viva: sons de ambiente, borboletas, sementes e vaga-lumes

Status: **aguardando teste no editor**.

## O que mudou

**Sons de ambiente** (porte do `Systems/Audio/Ambience.cs` do Godot, que foi só lido):

| Loop | Origem | Volume |
|---|---|---|
| Pássaros (dia) | `amb_day.wav` do Godot | 0,55 × claridade |
| Grilos (noite) | `amb_night.wav` do Godot | 0,5 × (1 − claridade) |
| Vento | `amb_wind.wav` do Godot | 0,40 (o valor de campo; a cidade vai baixar isso na Fase 16) |
| Riacho | `Ambient_Water_Stream_Calm_3` do Free_Sounds_Pack (VortexReferencia) | 0,7 × proximidade da água |

- A proximidade da água é medida 2 vezes por segundo, numa grade de 2,5 m até 25 m em volta da câmera: cheio até 3 m, some aos 22 m (a mesma curva da fonte no Godot).
- Os volumes andam em linha reta até o alvo a 0,5 por segundo, como no Godot.
- `amb_town` também foi importado e fica pronto para a cidade (Fase 16).
- O `.uasset` do riacho foi copiado tal como está para `Content/Free_Sounds_Pack/wav/`, o mesmo caminho do pacote.

**Bichinhos em volta da câmera**, criados por `AVortexForestLife`:
- 20 borboletas (eram 64; ficou muita) (monarca, limão, branca e azul, com borda escura e pintas) e 90 sementes de dente-de-leão levadas pelo vento, só de dia.
- 90 vaga-lumes piscando cada um no seu ritmo, só à noite.
- Na troca dia/noite eles aparecem e somem um a um, em pontos sorteados, não todos juntos.
- Ficam sobre o chão (nunca sobre a água), num raio de 14 a 18 m.
- Quem fica longe da câmera reaparece numa faixa afastada, então ninguém surge na sua cara.
- Todo o movimento é calculado na placa de vídeo pelo material: bater de asas com planadas, voo errante, vento e piscar. O processador só reposiciona quem ficou longe, 4 vezes por segundo.
- Nenhum deles faz sombra, colisão ou reflexo.

## Arquivos

- `Source/Vortex/Public/World/VortexForestLife.h` e `Private/World/VortexForestLife.cpp` (novos). As malhas são montadas no código.
- `VortexDayNightSubsystem.cpp`: cria a floresta viva em cada máquina que desenha (nunca no servidor dedicado).
- `Tools/ImportAmbience.py`: importa os sons em `/Game/Vortex/Audio`, todos em loop.
- `Tools/CreateForestLife.py`: cria `M_Butterfly`, `M_Seed` e `M_Firefly` em `/Game/Vortex/Life`.
- `VortexGameMode.cpp` e `Tools/RunDayNightShot.ps1`:
  - novas vistas `vida_1100` e `vida_2200`;
  - `-Only vida` para tirar só essas vistas;
  - `-Extra "-VortexLifeRadius=0.3"` para juntar os bichos perto da câmera nas fotos.

## Testes

- `RunDayNightShot.ps1 -Only vida`:
  - 11h: 138 visíveis (borboletas e sementes), dia 0,55, noite 0, riacho 0,70 (ao lado do lago).
  - 22h: 90 vaga-lumes, dia 0, noite 0,5.
- `RunDayNightShot.ps1 -Only vida_1100 -Extra "-VortexLifeRadius=0.3"`: as borboletas aparecem coloridas e batendo as asas.
- `RunDayNightSmoke.ps1` (dia/noite em rede): passou.

## Desempenho

`RunTerrainBench -Profile alto`:

| Momento | Antes | Agora |
|---|---|---|
| Meio-dia | 66,9 fps | 69,8 fps (1% low 54,6) |
| 22h | — | 73,3 fps |

A diferença do meio-dia está dentro do ruído: os 244 bichos custam 3 desenhos por quadro e quase nada de processador.

## Riscos

- Na densidade normal as borboletas são poucas na tela; é de propósito, para não virar enxame. Se quiser mais, aumente as quantidades em `AVortexForestLife::BeginPlay`.
- Borboletas muito perto da câmera ficam um pouco borradas pelo motion blur.
- Com `-VortexNoDayNight` a floresta viva não é criada, porque nasce junto com o dia e noite.
- No teste automático o som fica desligado (`-nosound`): só os volumes calculados foram verificados. O som em si precisa ser ouvido no editor.

## Como desligar ou voltar atrás

- `-VortexNoForestLife` desliga tudo, sons e bichos.
- Para remover de vez, reverta este commit; os assets ficam em `Content/Vortex/Audio`, `Content/Vortex/Life` e `Content/Free_Sounds_Pack`.
