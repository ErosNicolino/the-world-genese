# Noite mais clara, luzes mais fracas e névoa roxa nos monstros (30/10)

Situação: 🟡 pronto, com fotos; espera o seu teste no jogo.

## Pedidos

- "Quando escurece fica muito escuro, não dá para ver nada."
- "A luz das coisas está muito forte, como a das lâmpadas."
- "Os monstros com uma névoa roxa saindo deles naturalmente."

## O que mudou

### Noite (`UVortexDayNightSubsystem`)

| O quê | Antes | Agora |
|---|---|---|
| Luz ambiente da noite (o que ilumina a sombra) | 4,5× | 9× |
| Exposição da noite (o olho se acostumando ao escuro; menor = mais claro) | 0,55 | 0,36 |
| Lua | 0,32 do sol (lua nova a 25%) | 0,4 do sol (lua nova a 35%) |

- **Antes:** embaixo das árvores e perto das casas ficava preto puro, mesmo com lua.
- **Agora:** dá para ver os troncos, o caminho e o chão na sombra. Continua noite, com o céu escuro e o luar azulado.

### Luzes mais fracas

| Luz | Antes | Agora |
|---|---|---|
| Lampiões da cidade (`VortexTown`) | 90 cd, alcance 8,5 m | 30 cd, alcance 6,5 m |
| Tochas das dungeons (`VortexDungeons`) | 4500 (900 no modo escuro) | 2500 (1400 no modo escuro) |
| Runa dos santuários (`VortexPointsOfInterest`) | 3500–6000 | 1750–3000 |
| Portal (`VortexPortal`) | 7000 ± 3000 | 4200 ± 1800 |

As poças de lava do chefe da Caldeira ficaram como estavam, porque fazem parte da luta.

### Névoa roxa de corrupção

- **Quem tem:** todos os monstros hostis (`AVortexMonster::ApplyMist`) e os lobos. Os animais pacíficos não têm.
- **Como é:** sai dos pés, do tamanho de cada bicho, e some quando ele morre.
- **De onde vem:** é a aura "Soul" do Free_Spells, com cada parte pintada de roxo pelos parâmetros do próprio efeito (`Creatures/VortexMist.h`). Ficaram só a fumaça e umas faíscas; os raios, a espiral, a onda e a rachadura saíram, porque pareciam efeito de cura.

## Fotos

- `Saved/Screenshots/noite_comparar.png`: 21h, 22h na cidade e 1h; à esquerda antes, à direita agora.
- `Saved/Screenshots/nevoa_monstros.png`: goblin, golem e gota peçonhenta.

## Testes

- `Tools/RunDayNightShot.ps1`: DONE.
- `Tools/RunCreatureSmoke.ps1 -Species Goblin,Golem,SlimePoison`: PASS.

## O que testar

1. **Noite no mundo:** ande no campo, na mata e na cidade às 21h e às 2h (comando `/hora 21`). Está dá para ver sem perder o clima de noite? Se ainda estiver escuro, a opção **Brilho** em Opções > Gráficos também clareia.
2. **Lampiões da cidade e tochas das dungeons:** ainda estão fortes demais ou fracos demais?
3. **A névoa roxa:** está forte demais em monstros grandes (golem, chefes)?
