# Vortex Unreal — Fase 4: IDs, save e base de rede

**Data:** 03/10/2026

## WHAT / O que foi alterado

- Introduzidos `WorldId`, `PlayerId` e `CharacterId` como GUIDs. O `SessionSlot` da Fase 3 continua apenas como identificador temporário da conexão.
- Criados schemas `UVortexWorldSave` e `UVortexPlayerSave`, ambos na versão 1, com versão do jogo. O mundo guarda seed, versão do gerador e deltas persistentes; o jogador guarda IDs próprios e estruturas iniciais de itens e etapas de quests.
- Criado `UVortexSaveSubsystem` com slots separados para mundo hospedado e perfil local. Clientes remotos não podem usar `SaveHostedWorld` ou `LoadHostedWorld`.
- A escrita serializa em memória, envolve o payload em cabeçalho próprio com tamanho e CRC, valida um slot de preparação, preserva a cópia anterior válida e confirma a escrita principal. Ao carregar, tenta o principal e depois o backup validado.
- `GameState` replica apenas o descriptor da sessão (`WorldId`, seed e versão do gerador). O `GameMode` cria um mundo temporário ao iniciar; ele não o salva automaticamente.
- Incluído teste de salvamento acionado por `-VortexSaveSmoke` em builds que não sejam Shipping. Ele cria dados de teste, grava, lê, corrompe o principal, recupera o backup e remove seus próprios slots.

## WHY / Por quê

O save local do jogador precisa permanecer distinto do estado recebido ao visitar um mundo alheio. IDs persistentes também não podem depender do número da conexão. O mundo procedural será representado por seed, versão do gerador e mudanças duradouras, evitando salvar cada objeto gerado.

## DEPENDENCIES / Dependências

APIs nativas `USaveGame`, `UGameplayStatics` e `UGameInstanceSubsystem` da UE 5.8. O arquivo ainda usa o armazenamento por slots da plataforma Unreal. Não existe dependência de save Godot ou serviço online.

## VALIDATION / Validação

| Verificação | Resultado |
|---|---|
| Build `VortexEditor Win64 Development` | Passou |
| Save de mundo e jogador, leitura e IDs | Passou no teste runtime |
| Principal corrompido e recuperação do backup | Passou; log `SAVE_SMOKE_PASS` |
| Servidor e cliente recebendo o descriptor de mundo | Passou no teste de rede local |
| Build `Vortex Win64 Development` | Passou com opções de memória reduzida documentadas no README |

O primeiro teste de corrupção revelou que bytes arbitrários enviados diretamente ao desserializador da Unreal podem causar uma falha fatal. O cabeçalho próprio e o CRC agora rejeitam esse caso antes da desserialização; o mesmo teste passou após a correção. Para repetir: `Tools/RunSaveSmoke.ps1` e `Tools/RunFrameworkNetworkSmoke.ps1`.

## RISK / Limites atuais

- A escrita por slots não é uma transação atômica garantida pela plataforma. O backup validado reduz perda de dados; uma falha de disco em vários arquivos ainda pode exigir recuperação manual.
- O CRC detecta corrupção acidental. Ele não autentica dados nem protege contra edição maliciosa.
- O schema versão 1 rejeita versões incompatíveis. Não existe migração de schemas antigos da Unreal porque esta é a primeira versão; rotinas de migração terão de ser criadas quando o schema mudar. Saves Godot não são lidos nem convertidos.
- A sessão ainda não oferece seleção/reabertura de mundo salvo, autenticação de jogador ou associação segura entre uma conexão remota e `PlayerId`. Isso antecede qualquer persistência multiplayer real.
- `PersistentDelta`, itens e quests são contratos iniciais. As regras específicas de cada domínio serão acrescentadas nas fases correspondentes.
- O teste de corrupção confirma um arquivo truncado/inválido; não é uma campanha completa de fuzzing do parser Unreal.

## PERFORMANCE / Medição

Os saves são síncronos, com limite de 64 MiB por arquivo. Nenhum save é feito por frame ou automaticamente ao iniciar a sessão. O custo de gravação e leitura em conteúdo real ainda precisa ser medido; os dados de teste são pequenos e não representam um mundo completo.

## ROLLBACK / Reversão

A Fase 4 será mantida em commit próprio. Reverter esse commit remove os novos schemas e subsistema sem tocar no projeto Godot ou em seus saves. Os slots usados pelo teste são temporários e o teste apaga apenas os IDs que gerou.

## Próxima fase

Fase 5: protótipo mensurável de movimento, câmera e colisão suave sobre terreno visual blocky, com comparações objetivas ao comportamento Godot.
