# Menu inicial, mundos locais e LAN — aguardando teste no editor

## O que mudou e por quê

O jogo agora inicia em `L_Menu`, com menu funcional para criar/listar mundos, continuar o último, entrar em LAN por IPv4 e voltar ao menu. O mundo escolhido é salvo e reaberto com hora, moradores e posição do anfitrião. `L_TerrainProto` continua como mapa inicial do editor para testes. A tela usa uma captura do próprio Vortex como fundo provisório.

## Validação

Compilação `VortexEditor Win64 Development` concluída. Rodaram com êxito o fluxo menu → mundo → menu → mundo, entrada cliente LAN, movimento em matriz, rede do framework e smoke de saves. A última mudança pequena no menu foi recompilada; os smokes de fluxo serão repetidos na próxima rodada de integração.

## Riscos e limites

LAN aceita IP local e não faz descoberta automática de partidas. A semente é preservada, mas o terreno ainda é o protótipo, sem geração de continente. O visual é provisório. O usuário ainda precisa validar o fluxo no editor.

## Desempenho e reversão

O menu Slate é criado ao entrar em `L_Menu` e removido ao viajar; não permanece ativo durante a partida. Para reverter, restaure este commit e o mapa inicial anterior em `Config/DefaultEngine.ini`.
