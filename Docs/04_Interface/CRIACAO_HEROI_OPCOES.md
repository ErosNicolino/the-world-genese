# Criação de herói e opções — aguardando avaliação no editor

## Referência lida no Godot

`Game/Interface/GameHud.Menu.cs`, `GameHud.Races.cs`, `GameHud.Creator.cs`, `GameHud.Options.cs`, `Art/Adventurer/AdventurerCustomization.cs`, `AdventurerWardrobe.cs` e `Systems/Player/RaceCatalog.cs`. O projeto Godot e seus saves foram apenas lidos.

## O que mudou e por quê

O menu Unreal agora segue mundo → lista de heróis → escolha de raça → aparência → criar → escolher herói → jogo. Há limite de três heróis locais por mundo e exclusão com confirmação. Nome, raça, visual masculino/feminino e peças importadas são gravados em saves individuais validados. Ao entrar no mundo, as peças escolhidas são aplicadas no herói local. Foram importados rosto e dois cabelos femininos do aventureiro original, com o esqueleto já usado pelas outras peças.

As opções iniciais agora permitem aplicar e salvar sensibilidade, inversão vertical, HUD, VSync, qualidade geral e modo janela/tela cheia. O fluxo foi inspirado nas seis abas do Godot, mas mostra apenas ajustes que já existem no Unreal.

## Validação

`VortexEditor Win64 Development` compilou. `RunHeroSmoke.ps1` verifica criação, releitura, vínculo com mundo, aparência/peça salva, limite de três e exclusão. `RunSaveSmoke.ps1` passou, assim como `RunMenuFlowSmoke.ps1` para abrir, salvar, voltar e retomar o mundo. As malhas femininas passaram pela importação do Unreal com o esqueleto compartilhado. O usuário ainda deve testar aparência, seleção e opções no editor.

## Limites, riscos e desempenho

O catálogo Godot contém centenas de peças; esta etapa apresenta somente as alternativas efetivamente importadas em sete categorias. Cores livres, traços raciais, acessórios, prévia 3D no criador e efeitos de atributos/tamanho das raças ainda não foram portados. A raça é salva, mas ainda não altera o combate. O visual feminino usa rosto e cabelos próprios com o mesmo corpo e animações.

Cada herói é carregado sob demanda. As peças com `Leader Pose` só são criadas quando uma escolha modular é aplicada. O sistema de save mantém leitura dos perfis de esquema 1; os novos usam esquema 2. Não houve medição de FPS específica para o menu.

## Reversão

Reverta este commit para retirar a criação de herói e as opções. Saves já gerados ficam em `Saved/SaveGames` e não são apagados pela reversão; faça backup deles antes de experimentar versões antigas.
