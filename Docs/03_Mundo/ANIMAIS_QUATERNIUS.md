# Animações novas no lobo, no veado e nos cavalos (Quaternius, 30/10)

Situação: 🟡 lobo, veado e cavalos no jogo, com os testes passando; espera o seu teste.

## Pedido

"Os lobos e os animais diferentes: no Mixamo não dá." O Mixamo só aceita humanoides. Você escolheu:

- usar o pacote gratuito de animais animados;
- animar no Blender os bichos que ficarem sem par.

## De onde vieram

- **Pacote:** Quaternius "Ultimate Animated Animals" (CC0, domínio público, pode usar em jogo comercial).
- **Página:** quaternius.com/packs/ultimateanimatedanimals.html; os arquivos ficam no Google Drive do autor.
- **No projeto:** `Data/Creatures/Quaternius_Animais/`, com Wolf, Deer, Stag, Horse, Fox e License.txt.
- **Clipes de cada animal:** Idle, Idle_2, Walk, Gallop, Gallop_Jump, Attack (ou Headbutt e Kick), Eating,
  Idle_HitReact, Death e Jump_ToIdle.

Os modelos continuam os nossos: o lobo ametista e o veado e os cavalos voxel. Só o movimento vem do pacote.

## Como passa para os nossos bichos

O conversor é `Tools/Blender/QuadrupedTransfer.py <bicho>`.

- **O que cada osso copia:** o quanto o osso dela gira a partir da pose de pé, e não a direção.
  - A pose de pé é o primeiro quadro do Idle de cada um.
  - Copiar a direção, como no Mixamo, só serve para corpos iguais. A canela da Quaternius aponta para trás e a nossa
    para baixo, então copiar a direção encolhia as patas.
- **Quadril:** sobe e desce na proporção das alturas. O deslocamento para a frente sai, porque quem anda é o jogo.
- **Chão:** se alguma parte da malha passa abaixo do chão de pé, o corpo sobe só o que falta. No ar ele continua solto,
  então o salto do galope fica.
- **Tempo dos ataques:** o lobo morde em 0,3 s e dá o bote em 0,55 s, os instantes do dano em `AVortexWolf`. Os clipes
  são esticados ou encolhidos para o golpe cair aí.
- **Cavalo espelhado:** a pata esquerda dele fica em -X, então os lados são ligados pela posição.

| Bicho | Clipe novo ← clipe da Quaternius | Continuam os nossos |
|---|---|---|
| Lobo ametista | Idle, Walk, Run ← Gallop, Attack ← Attack, Pounce ← Gallop_Jump, Hit, Death | Howl, StrafeL, StrafeR |
| Veado | Idle, Walk, Run ← Gallop, Eat ← Eating, Hit, Death | Alert |
| Cavalos (5 pelagens) | Idle, Walk, Run ← Gallop, Eat ← Eating, Hit, Death | Rear |

Na Unreal:

- **Lobo:** `Tools/ImportAmethystWolf.py`.
- **Veado e cavalos:** `Tools/ImportMonsters.py`.

Os originais de antes ficaram guardados fora do projeto, para comparar.

## Cavalo novo (Meshy)

O cavalo do Meshy (`Data/Modelos/Cavalo/Cavalo_Meshy.glb`) substituiu o de blocos nas cinco pelagens.

- **Animações:** as da Quaternius (`QuadrupedTransfer.py horse_meshy` → `Data/Modelos/Cavalo/Horse_Skinned.glb`).
  - A pose de pé é a de descanso dele.
  - Empinar usa o coice (Attack_Kick).
- **Sela:** o osso "Saddle" no lombo (1,12 m), onde o herói senta.
- **Textura:** 2K.
- **Na Unreal:** `Tools/ImportHorse.py` importa o mesmo modelo como Horse_Baio, Negro, Tordilho, Palomino e Malhado. Cada
  pelagem tem o seu tom (`M_HorseCoat`: a textura desbotada e tingida).
- **O import dos monstros** (`ImportMonsters.py`) não mexe mais nas pelagens.

## Correção: clipes que saíam parados

O conversor dava aos clipes novos os nomes do jogo e depois apagava pelo nome os clipes que sobravam. Quando o clipe da
Quaternius tinha o mesmo nome (Idle, Walk...), o novo nascia "Idle.001" e era o apagado. Por isso o lobo, o veado e os
cavalos ficaram com clipes parados.

Agora o conversor apaga pela referência, antes de dar os nomes, e cada faixa ganha o encaixe do clipe (Blender 5).
O lobo, o veado e o cavalo novo foram refeitos a partir dos originais. Conferido no próprio arquivo: o andar e o galope
giram as patas de 30° a 80°.

## Testes

- `Tools/RunWolfVisualShot.ps1`: o lobo com os clipes novos (fotos `wolf_*.png`).
- `Tools/RunCreatureSmoke.ps1 -Species Deer,Horse_Baio,Horse_Negro`: nascem, aparecem, apanham, morrem e dão XP.
  Use o nome da pelagem do cavalo (Horse_Baio...): o nome "Horse" sozinho não tem malha.
- `Tools/RunBossShots.ps1`: PASS.
  - **O que aconteceu:** o `ImportMonsters.py` reimporta todas as espécies e tinha trazido de volta o Azhur antigo do
    Godot.
  - **Correção:** o Azhur foi refeito com o `ImportAzhur.py`, e agora o `ImportMonsters.py` pula as espécies que têm
    import próprio.

## Os bichos sem par no pacote

Sapo, coelho, goblin, golem, aranha, morcego, escorpião, cogumelo e os chefes ficam como estão, até você escolher o
plano (veja a resposta no chat).
