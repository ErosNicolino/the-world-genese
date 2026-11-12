# Os 35 arcos (07/11)

Das pranchas do usuário (`modelando ainda/Futuras modelagens/Armas/Arcos`), do Normal ao Divino, no mesmo estilo de
blocos do arco do jogo. O modelo não tem corda: a corda é feita na Unreal (`VortexWeaponParts::TickBowString`), de
ponta a ponta do modelo, e estica com a animação de puxar. Cada arco tem a sua flecha (a ponta no formato do elemento,
as cores do arco) e um poder no botão direito.

## Como refazer

1. `python Tools/Weapons/SliceBowSheets.py`: recorta as pranchas em grades de pixels (`Data/VortexWeapons/Bows`),
   tira a corda (o traço reto, comprido e fino), a sombra desenhada dos Incomuns e os nomes escritos dos Únicos, e vira
   todos para o mesmo lado. Conferência: `Data/VortexWeapons/Bows/_conferencia.png`.
2. `python Tools/Weapons/BuildBows.py`: nomes, poderes e ícones -> `Data/Rpg/bows.json`; depois `python Tools/BuildArsenal.py`.
3. Blender: `blender -b --factory-startup -P Tools/Blender/MakePixelBows.py -- <raiz>` (os arcos e as flechas;
   conferência das flechas em `Data/VortexWeapons/Bows/_flechas.png`).
4. `python Tools/MakeSwordSounds.py` (os sons `bw_<chave>`).
5. Editor fechado (pelo bash, caminhos com `pwd -W`): `ImportBlockSword.py` (com `VORTEX_ONLY=arco,flecha`),
   `ImportItemIcons.py`, `ImportExtraSfx.py`.
6. Foto: `Tools/RunWeaponCloseUp.ps1 -Types "item:arco_da_fenix,item:asas_da_aurora"`.

## No jogo

- O jogo lê o `bows.json` pelo `VortexSwords` (o mesmo das espadas): o modelo (`PixelSwordFor` / `ForVisual`), a flecha
  (`VortexSwords::ArrowMesh`, na flecha em voo e na encaixada na corda) e o poder (`SpecialKit`). O poder usa a animação
  do próprio arco (`anim: self`).
- Poderes novos de arco (`VortexWeaponSpecials.cpp`): **volley** (leque de flechas, `count` e `spread`), **pierce** (a
  flecha grande que atravessa, `size`) e **explosive** (explode no alvo ou no chão, `radius`, com a explosão do
  elemento). Os outros são os das outras armas, com o elemento do arco.
- O Arco de Bétula (o inicial) fica com a chuva de flechas. Os arcos do Godot (Curto, Caçador, Névoa Azul, Noite
  Violeta, Grifo Dourado, Lua Sangrenta) ganharam o modelo que combina; o id do item é o mesmo (os saves valem).
- As variantes do arsenal (`arco_de_betula_fire_rare`...) usam um dos 5 arcos da raridade delas.
- Os divinos só caem do Colosso de Runas e do Golem (2%).

| Arco | Raridade | Elemento | Poder | Recarga | Mana |
|---|---|---|---|---|---|
| Arco Curto | Normal | — | **Três Seguidas**: Solta três flechas rápidas, uma atrás da outra. | 10.3 s | — |
| Arco do Batedor | Normal | — | **Leque do Batedor**: Abre um leque de três flechas. | 10.3 s | — |
| Arco de Galho Seco | Normal | — | **Flecha Seca**: Uma flecha forte que atravessa os inimigos em linha. | 11.5 s | — |
| Arco de Bétula | Normal | — | **Chuva de Flechas**: Uma chuva de flechas cai onde você mira. | — | — |
| Arco Remendado | Normal | — | **Chuvisco de Flechas**: Uma chuva de flechas pequena onde você mira. | 18.4 s | — |
| Arco do Salgueiro | Incomum | Natureza | **Folhas ao Vento**: Um leque de quatro flechas de folha. | 9.5 s | — |
| Arco de Espinheiro | Incomum | Veneno | **Espinhos em Fila**: Três flechas de espinho envenenadas, uma atrás da outra. | 9.5 s | — |
| Arco de Cobre Rubro | Incomum | Fogo | **Flecha de Brasa**: Uma flecha que explode em fogo onde acerta. | 11.6 s | 10 |
| Arco da Correnteza | Incomum | Água | **Flecha da Correnteza**: Atravessa os inimigos em linha, como a água. | 10.5 s | — |
| Arco do Caçador | Incomum | — | **Chuva do Caçador**: Uma chuva de flechas onde você mira. | 16.8 s | — |
| Arco da Névoa Azul | Raro | Água | **Selo da Névoa**: Marca o chão com uma runa de água que explode logo depois. | 12.0 s | 15 |
| Arco da Lua Prateada | Raro | Luz | **Crescente**: Um leque de cinco flechas de luz em meia-lua. | 9.0 s | — |
| Arco do Relâmpago | Raro | Raio | **Flecha-Trovão**: Segura e solta: um raio carregado que explode no alvo. | 6.0 s | 12 |
| Arco da Geada | Raro | Gelo | **Lança de Gelo**: Uma flecha de gelo que atravessa todos em linha. | 10.0 s | — |
| Arco de Jade | Raro | Natureza | **Três Jades**: Três flechas de jade seguidas. | 9.0 s | — |
| Arco Ametista | Épico | Sombra | **Estilhaços Flutuantes**: Cristais de ametista giram em volta e atiram nos inimigos. | 23.8 s | 20 |
| Arco da Noite Violeta | Épico | Sombra | **Runa da Noite**: Uma runa de sombra que explode e marca os inimigos. | 11.4 s | 15 |
| Sussurro da Lua Sangrenta | Único | Sombra | **Estrela Sangrenta**: Uma flecha que explode em estrelas de sangue. | 9.3 s | 10 |
| Arco Fragmentado | Épico | Raio | **Fragmentos**: Cinco disparos rápidos de fragmentos do Vórtice. | 8.5 s | — |
| Arco do Eclipse | Épico | Sombra | **Flecha do Eclipse**: Uma flecha escura enorme que atravessa tudo. | 9.5 s | — |
| Arco de Magma Rúnico | Lendário | Fogo | **Muralha de Magma**: Ergue uma muralha de magma que queima quem atravessa. | 16.2 s | 25 |
| Arco do Grifo Dourado | Lendário | — | **Asas do Grifo**: Um leque de sete flechas douradas. | 8.1 s | — |
| Arco da Fênix | Lendário | Fogo | **Sopro da Fênix**: Lança uma bola de fogo que explode em 3 m. | 9.0 s | 15 |
| Arco do Sol Nascente | Lendário | Luz | **Explosão Solar**: Uma flecha de sol que explode em luz. | 9.9 s | 10 |
| Arco do Bosque Ancestral | Lendário | Natureza | **Chuva de Galhos**: Uma chuva de flechas enorme onde você mira. | 14.4 s | — |
| Arco de Braços Gêmeos | Único | Raio | **Disparo Gêmeo**: Seis flechas em pares, bem rápidas. | 7.6 s | — |
| Arco do Espelho | Único | Sombra | **Reflexo**: Um leque largo de seis flechas, para os dois lados da mira. | 7.6 s | — |
| Arco da Raiz Primeva | Único | Natureza | **Raiz que Explode**: Uma raiz rompe o chão onde você mira e explode. | 10.2 s | 15 |
| Arco Sem Sombra | Único | Luz | **Flecha Sem Sombra**: Uma flecha de luz que atravessa tudo e vai muito longe. | 8.5 s | — |
| Arco do Viajante Perdido | Único | Gelo | **Fôlego do Viajante**: Cura você e os aliados em volta. | 13.6 s | 20 |
| Asas da Aurora | Divino | Luz | **Aurora**: Nove flechas de luz em leque. | 7.2 s | — |
| Arco da Árvore do Mundo | Divino | Natureza | **Chuva da Árvore do Mundo**: Uma chuva de flechas imensa onde você mira. | 12.8 s | — |
| Arco das Estrelas | Divino | Luz | **Constelação Viva**: Estrelas giram em volta e atiram nos inimigos sem parar. | 20.0 s | 20 |
| Arco do Primeiro Rio | Divino | Água | **Rio Primordial**: Uma flecha de água imensa que atravessa tudo à frente. | 8.0 s | — |
| Arco do Vendaval Eterno | Divino | Natureza | **Vendaval Eterno**: Uma flecha que estoura num vendaval enorme. | 8.8 s | 10 |

Animações e efeitos próprios de cada arco (07/11, à noite): os mesmos das espadas (ver ESPADAS.md, "Animações e efeitos
próprios"): `Poder_arco_<chave>_Full` e a receita de efeito do arco em `Data/Rpg/power_fx.json`.

## Poderes feitos à mão (07/11, à noite)

Cada arco tem a sua animação (`Tools/Blender/power_anims_hand.py`, "arco_<chave>") e o seu efeito
(`Tools/Weapons/power_fx_hand.py`), como as espadas (ver ESPADAS.md, "Poderes feitos à mão, um por um"). As rajadas
soltam uma flecha a cada puxada da animação; a chuva sobe ao céu e cai onde a mira bate; o Arco do Relâmpago tem a carga
(o raio cai no arco enquanto segura) e explode no alvo.
