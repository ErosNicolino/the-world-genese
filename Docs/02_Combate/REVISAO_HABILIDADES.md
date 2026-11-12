# Revisão das habilidades de classe

> "Dê uma olhada em todas as skills, porque muitas estão bugadas ou feias ou nem funcionando ou com textura diferente."
> "Veja tudo: se a distância, o uso da habilidade se encaixa com o nome etc."

São 66 classes com 4 habilidades cada: **264 habilidades**. Um teste novo lança todas, uma por uma, tira uma foto de cada e mede o efeito (dano nos alvos, bênçãos, cura e quanto o herói andou).

## O que a revisão achou

| Problema | Quantas | Nesta etapa |
|---|---|---|
| A pressa ("recargas 30% mais rápidas") deixava **todas** as recargas em 0,5 s: o 30 do Godot (porcentagem) era lido como 3000% | 8 | ✅ corrigido |
| **Alcance maior que o do texto**: o jogo multiplicava o alcance por 1,3 a 3 ("Perfuração: estocada de 6 m" acertava a 20 m; "Flecha Perfurante (26 m)" ia a 57 m) | 51 | ✅ corrigido |
| Golpe do céu de vários impactos se espalhava além da área escrita (Chuva de Flechas: "4,5 m", caía a até 9 m) | 4 | ✅ corrigido |
| Zona "na mira" caía sempre a uma distância fixa à frente (Nuvem de Tempestade, Runa de Contenção, Mina Rúnica...) | 42 (15 com alcance escrito, 27 com o padrão de 4 m) | ✅ corrigido |
| Bênção "só sua" ia também para os aliados a 8 m (Último Fôlego, Pele de Casca, Corpo de Pedra...) | 34 | ✅ corrigido |
| Dois nomes quebrados: "Duelo Mortal da Ascensão:" e "Reescrever Destino da Ascensão:" | 2 | ✅ corrigido |
| Ícones **recortados tortos** da folha (moldura deslocada, pedaço do ícone vizinho embaixo) e serrilhados (filtro de pixel art numa pintura) | todos | ✅ corrigido |
| Ícones que não combinavam (Tiro de Pistola com orbe arcano, Chamado da Matilha com corte de fogo, Familiar com raio): só 57 ícones para 264 habilidades | ~190 | ✅ trocados |
| 207 habilidades (gritar, rezar, invocar, apontar...) só **erguiam a arma**: os 9 gestos do herói do Godot nunca tinham vindo | 207 | ✅ trazidos |
| Efeitos visuais: explosão sempre de fogo (até a de vento), escudo que esconde o herói, invocações que são cubos, colunas brancas | — | ⏳ etapa 5 (depois) |
| Partes da descrição que ainda não têm efeito: puxar, esquiva para o lado, totens, armadilhas, sumir da vista, purificar, devolver dano, Fúria e Essência, eco, forma elemental... | 103 | ⏳ etapa 6 (depois) |

## O que mudou

### Bugs e distâncias (código)
- **Pressa** em porcentagem, como no Godot: "recargas 30% mais rápidas" agora é 30% (`VortexRpg.cpp`).
- **Alcance = o do texto.** `FVortexClassSkill::Reach` devolve o alcance escrito, sem multiplicar. Vale para projéteis, raios, ondas, correntes e golpes do céu, e também para a mira (até onde dá para mirar).
- **Golpe do céu**: as marcas caem dentro do raio escrito, em volta do ponto mirado.
- **Zonas com alcance** caem no ponto mirado (até o alcance). As sem alcance continuam em volta do herói.
- **Bênção, cura e escudo sem raio** valem só para o herói. Com raio (por exemplo "aliados a até 8 m"), valem para o grupo perto.
- **Nomes**: "Duelo Mortal" e "Reescrever Destino".

### Ícones
- `Tools/MakeSkillIcons.py` recorta de novo as 4 folhas do Godot (A classes, B ascensões, C golpes de arma, D habilidades). Cada ícone é achado pela própria moldura. As folhas não são retas, e a última linha da folha A tem 9 molduras em vez de 8: o recorte antigo, em grade fixa, saía torto. Saída: `Data/UI/SkillIcons` (192 x 192), com folhas de conferência.
- `Data/Rpg/class_skill_fixes.json`: o ícone escolhido para cada uma das 264 habilidades e os 2 nomes. O `Tools/BuildRpgData.py` aplica isso sempre que o rpg.json é refeito do Godot.
- `Tools/ImportItemIcons.py` traz esses ícones com suavização e mipmaps (são pintura, não pixel art). Os ícones de itens continuam como eram.
- Os ícones das classes (folha A) também saem do recorte novo.

### Animações
- Os 8 gestos do Godot (**Cast, Shout, Pray, Throw, Stomp, Summon, Guard, Point**) e a variante de cada um para 21 famílias de arma (cajado, orbe, besta, alaúde...), 176 clipes de 0,75 s: `Data/GodotWeapons/Adventurer_SkillClips.glb`, convertido pelo `Tools/ConvertGodotAdventurer.py`. `Tools/ImportSkillClips.py` os importa para `/Game/Vortex/Weapons/SkillClips`.
- `UVortexClassSkills::CastAnimation`: os golpes de arma (finalização, combo, estocada, investida) continuam com o golpe da arma. Os outros tocam o gesto: a variante da arma na mão (pela postura dela em `Data/Rpg/weapons.json`) ou o de mãos livres (espada, martelo, arco).

## Testes

- **`Tools/RunSkillAudit.ps1`** (novo; ~15 min, fora da suíte rápida). Lança as 264 habilidades contra alvos novos a cada uma. Fotos em `Saved/Screenshots/auditoria_<classe>_<n>.png`; resultado em `Saved/Logs/SkillAudit.txt`.
  - Depois das correções: **264 lançadas, 0 falhas**.
  - **216 tocam um gesto** do Godot (0 sem animação); as outras 48 tocam o golpe da arma.
  - 18 aparecem "sem efeito medido", mas é limitação do teste, não das habilidades:
    - os golpes de perto (2,4 a 3,4 m) não alcançam alvos que nascem a 5,5 m;
    - os golpes do céu caem onde a cruz da mira aponta, e no teste ela fica além dos alvos;
    - as curas pequenas ficam escondidas pelo dano que as gotas fazem no herói.
- **`Tools/RunClassShots.ps1`** (Despertar e as habilidades de cinco classes): passou, 0 falhas.
- **Não testado por máquina:** a sensação do alcance novo em luta real e os gestos com cada uma das 21 armas (o teste usa a espada).

## Fotos

| Ícones antes (recorte torto) | Barra antes / depois |
|---|---|
| ![](../Images/habilidades/icones_antes.png) | ![](../Images/habilidades/barra_antes_depois.png) |

| Gestos antes / depois: o herói aponta (Marca da Presa, Destino Sombrio) em vez de erguer a espada | Os efeitos visuais hoje (etapa 5) |
|---|---|
| ![](../Images/habilidades/gestos_antes_depois.png) | ![](../Images/habilidades/tipos_visual_hoje.png) |

As 264 habilidades com o ícone novo: [parte 1](../Images/habilidades/icones_novos_1.jpg) e [parte 2](../Images/habilidades/icones_novos_2.jpg).

O herói às vezes fica rosa-claro nas fotos: é o brilho de "levou golpe" (0,11 s). No teste, as gotas batem nele o tempo todo.

## Para testar no jogo

1. Pegue uma classe com pressa (Mago Elemental, Convergência). Lance e veja que as outras habilidades ainda têm recarga, só que mais curta.
2. Use a Perfuração (Lanceiro) e a Flecha Perfurante (Patrulheiro): elas param na distância escrita (6 m e 26 m).
3. Lance a Nuvem de Tempestade ou a Runa de Contenção mirando longe: a zona cai na mira.
4. Abra a janela do herói, aba Classes, e olhe a barra: os ícones estão nítidos e combinam com o nome.
5. Lance um grito, uma prece e uma invocação, e troque a arma para cajado ou orbe: o gesto muda conforme a arma.

## Próximas etapas (combinadas para depois)

- **5. Visual dos efeitos:**
  - explosão de cada elemento;
  - escudo transparente;
  - invocações com corpo (lobo, esqueleto, espírito);
  - golpe do céu de flechas e de luz com flechas e luz de verdade.
- **6. Efeitos que faltam:** as 103 habilidades com partes sem efeito, começando pelas mais comuns (puxar, esquiva lateral, totens e armadilhas, sumir da vista, purificar).

## Como refazer

```
python Tools/MakeSkillIcons.py
python Tools/ConvertGodotAdventurer.py <Adventurer.glb do Godot> Data/GodotCharacter/default_look.json Data/GodotWeapons/Adventurer_SkillClips.glb <Skill_<gesto>, Skill_<gesto>_<família>>
UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho>/Tools/ImportItemIcons.py -unattended -nullrhi
UnrealEditor-Cmd.exe Vortex.uproject -run=pythonscript -script=<caminho>/Tools/ImportSkillClips.py -unattended -nullrhi
```
