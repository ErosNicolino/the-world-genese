# Vortex Unreal — Fase 3: framework

**Data:** 03/10/2026
**Estado:** concluída; continuidade das fases autorizada pelo usuário.

## WHAT / O que foi alterado

- Criados `GameInstance`, `GameMode`, `GameState`, `PlayerController`, `PlayerState` e `Character` nativos em C++.
- O `GameMode` decide entrada/saída e atribui um slot de sessão; o `GameState` replica a contagem de conectados; o `PlayerState` replica o slot; o `Character` usa movimento replicado.
- Criado `L_Framework`, mapa de teste com piso, iluminação e dois pontos de entrada. A malha do personagem é uma forma provisória da própria engine.
- Configurados o mapa e as classes padrão. O conteúdo está em `Content/Vortex/`.
- Criados scripts para gerar o mapa e repetir o teste de rede local. O script de geração só deve ser usado em projeto novo, pois impede sobrescrever o mapa existente.

## WHY / Por quê

A estrutura mínima fixa as responsabilidades Unreal antes da migração de sistemas. O estado global de sessão nasce no servidor e chega ao cliente por replicação. Nenhum estado persistente foi ligado à entrada de outro mundo.

## DEPENDENCIES / Dependências

UE 5.8.3, toolchain da Fase 2 e `InputCore`. Os plugins Python/Editor Scripting são exclusivos do editor para criar o mapa; `AndroidFileServer` foi desativado no projeto por não ser usado. O jogo não depende de assets da cópia Godot.

## VALIDATION / Validação

| Verificação | Resultado |
|---|---|
| Compilação `VortexEditor Win64 Development` | Passou |
| Compilação `Vortex Win64 Development` | Passou com opções de memória reduzida documentadas no README |
| Criação e salvamento de `L_Framework` por Unreal Editor | Passou |
| Servidor local e cliente conectados ao mapa | Passou |
| `GameState.ConnectedPlayers=1` no cliente | Passou |
| `PlayerState.SessionSlot=0` no cliente | Passou |
| Pawn criado e reconhecido pelo cliente | Passou nos logs |

O teste de rede foi executado por `Tools/RunFrameworkNetworkSmoke.ps1`. Ele inicia servidor e cliente temporários, consulta logs e encerra ambos. Os logs de teste ficam em `Saved/Logs/`, fora do Git.

## RISK / Limitações

- `SessionSlot` é temporário e **não** é identidade persistente. A Fase 4 define IDs estáveis e schema de salvamento.
- O teste cobriu um cliente. Reentrada, desconexão, latência real, oito jogadores e pacote de jogo ainda não foram validados.
- O mapa e o personagem são instrumentos de teste, sem arte ou navegação final. Apresentação interativa na janela gráfica ainda precisa de inspeção.
- Não há garantia de gameplay, movimento sobre terreno blocky, save ou performance de cena nesta fase.

## PERFORMANCE / Medição

A build de jogo levou cerca de 113 s em modo serial; não há baseline de FPS ou frame time de conteúdo Unreal porque ainda não existe cenário equivalente ao Godot. O framework não faz busca global nem trabalho de gameplay a cada frame. Medição CPU/GPU virá nos protótipos correspondentes.

## ROLLBACK / Reversão

Este marco será mantido em commit próprio. Reverter o commit remove o framework e o mapa sem afetar as cópias Godot. Não houve leitura, conversão ou sobrescrita de saves Godot.

## Próxima fase

Fase 4: separar `WorldID`, `PlayerID` e `CharacterID`; definir autoridade e contrato de replicação; criar schema de save com versões e validação; garantir que mundo remoto não seja persistido como mundo local.
