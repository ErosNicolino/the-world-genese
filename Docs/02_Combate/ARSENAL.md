# Arsenal: o modo de teste (07/11)

O usuário: "um modo de jogo à parte, mais leve, só com os personagens e um mapa de teste com todas as armas
enfileiradas para eu testar uma por uma, de armas a armaduras e arcos".

- **Como entrar:** no menu principal, "Arsenal". Vai com o herói do último mundo (a aparência e a raça dele). Sem
  nenhum herói, entra um herói de teste. Esc e "voltar ao menu" saem.
- **Só no editor (08/11):** é uma ferramenta do criador. O jogo empacotado e o instalador não mostram a entrada, e o
  `UVortexGameInstance::OpenArsenal` não abre o mapa fora do editor (`#if WITH_EDITOR`); o mapa `L_Arsenal` também não
  vai no pacote (o `PackageAlpha.ps1` só cozinha o menu e o mundo).
- **Nada grava:** no mapa `L_Arsenal`, o `UVortexSaveSubsystem::SaveLocalPlayer` e a `SaveLocal` da ficha não gravam
  (`AVortexArsenal::IsArsenal`). O mundo é provisório e o Estádio do Despertar não abre.
- **O mapa:** `Tools/MakeArsenalMap.py` copia o `L_TerrainProto` (a mesma luz, céu, neblina e pós-processo), tira o
  terreno e os bichos e põe o `AVortexArsenal` no centro. É leve: perto de 90 FPS.
- **O que tem** (montado pelo `AVortexArsenal` a partir dos catálogos, então arma ou armadura nova aparece sozinha):
  - à frente, as espadas (esquerda) e os arcos (direita) da tabela em grades, uma fileira por raridade, 5 em cada;
  - mais à frente, as outras armas (uma de cada tipo: martelo, adagas, lança, cajado);
  - atrás de onde o herói nasce, os 21 conjuntos de armadura vestidos em manequins e a placa "Tirar a armadura";
  - lá no fundo, o campo de treino: a linha branca e os bonecos a 5, 10 e 20 m dela.
- **Como usar:** chegar perto de um pedestal ou manequim (aparece "[E] equipar" em cima) e apertar E. A arma vai para a
  mão e o conjunto inteiro vai para o corpo. Mana infinita e poder sem recarga.
- **Conferência:** `-VortexArsenalShots` (abrindo o `L_Arsenal` com `-game`): vista de cima, arco e armadura
  equipados pelo E, fotos em `Saved/Screenshots/arsenal_*.png`. Foto: `Docs/Images/arsenal_teste.png`.
- **Ainda aparece** a HUD do mundo (a missão "O Despertado" e o mapa da Cidade do Santuário no canto): não atrapalha.
