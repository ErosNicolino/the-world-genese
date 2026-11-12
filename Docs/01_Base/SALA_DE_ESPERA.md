# Sala de espera (até 8 jogadores) e herói do convidado

**Pedido:** "ja arrume o heroi do convidado e a sala de espera maneira para ate 8 pessoas".

## Como fica

1. **Anfitrião:** Meus mundos → **Rede local** ou **Online** → escolhe o herói → **Abrir a sala de espera**.
   - O mapa do menu reabre como servidor: pela rede local (IP) ou pela sala da Epic (código de 5 letras).
2. **Convidado:** Jogar com amigos → escolhe ou cria um **herói viajante** → **Escolher e conectar** → digita o código ou o IP.
3. **Na sala:**
   - Todos ficam no mundinho do menu. Os heróis aparecem lado a lado num arco, o do anfitrião no centro.
   - À esquerda, um painel com 8 vagas mostra o nome, a raça, o nível e o estado de cada um (ANFITRIÃO / PRONTO / ESPERANDO).
   - Também mostra o código da sala ou o IP para os amigos.
   - O convidado clica **Estou pronto**. O herói dele dá uma volta e um golpe.
   - O anfitrião clica **Começar aventura**. Só funciona com todos prontos.
4. Todos viajam juntos para o mundo do anfitrião. Cada um entra com o próprio herói: visual, nível, mochila e arma.
5. **Sair da sala** volta ao menu. Se o anfitrião sai, a sala acaba.

![sala](../../Saved/Screenshots/sala_espera.png) *(gerada por `Tools/RunLobbySmoke.ps1`)*

## Herói viajante

- **O que é:** um herói do próprio computador que não pertence a nenhum mundo de lá. Ele visita o mundo de um amigo.
- **Onde fica:** os viajantes ficam num "mundo" fixo, `UVortexGameInstance::TravellerWorld()`.
- **Criação:** o criador de sempre (raça, estilo, cores, nome). O limite é de 3 viajantes por computador.
- **Progresso:** XP, mochila e arma ficam salvos no próprio herói, no computador do convidado. É o mesmo save dos heróis normais.
  - Um viajante novo acorda no Estádio do Despertar e escolhe a arma lá.
- **Visual no mundo:** o convidado só recebe o boneco depois de conectar. O jogo tenta aplicar o visual a cada 0,25 s até o herói existir, e então o envia ao servidor.

## Por dentro

| Peça | O que faz |
|---|---|
| `AVortexMenuGameMode` | Usa `AVortexPlayerState`. Recusa a partir do 9º jogador e marca quem é o anfitrião (o controle local do servidor). |
| `AVortexPlayerState` | Replica `LobbyHero`, `LobbyLevel`, `LobbyLook` (`FVortexNetLook`), `bLobbyReady` e `bLobbyHost`. RPCs: `ServerSetLobbyHero` (com validação) e `ServerSetLobbyReady`. |
| `UVortexGameInstance` | `OpenLobby` abre `L_Menu` com `listen` (Epic) ou `listen?bIsLanMatch` (rede local). `InLobby` / `IsLobbyHost` / `GetLobbyAddress`. Um timer manda o herói local à sala até o servidor confirmar. `StartAdventure` faz `ServerTravel` para `L_TerrainProto` (com `listen`), levando todos. |
| `SVortexFrontEnd` | Página `Lobby`, com 8 vagas que se atualizam sozinhas e os botões. A tela de heróis tem o modo viajante, e a de entrar exige um herói escolhido. |
| `AVortexMenuStage` | Modo `Lobby`: até 8 heróis num arco, de frente para a câmera, com uma volta e um golpe ao ficar pronto. A câmera se afasta conforme chega gente. Os heróis do palco não vão pela rede (`SetReplicates(false)`): cada computador monta os seus. |
| `UVortexOnline` | A sala da Epic passou a aceitar até 8 jogadores. |

## Testes

- **`Tools/RunLobbySmoke.ps1 [-Guests 3]`:**
  - 1 anfitrião (com tela) e N convidados (sem tela) no mesmo PC, pela rede local;
  - o anfitrião cria um mundo e um herói de teste e abre a sala;
  - cada convidado cria um viajante de outra raça, entra e fica pronto;
  - o anfitrião tira a foto e começa a aventura;
  - no mundo, cada um confere o próprio herói e o anfitrião confere que chegaram N+1 heróis, cada um com a sua raça;
  - tudo o que o teste cria é apagado no fim, inclusive o mundo, que se salva de novo ao fechar.
  - **Resultado:** `VORTEX_LOBBY_SMOKE_PASS`, com 3 e com 2 convidados.
- **Continuam passando:** `RunMenuNetworkSmoke`, `RunHeroSmoke` e `RunMenuFlowSmoke`.

## Limites

- **Pela Epic:** a sala e a viagem precisam de dois computadores para testar, como antes.
  - A viagem é "não contínua": cada convidado reconecta ao anfitrião no mapa novo.
  - Pela rede local isso foi testado. Pelo P2P da Epic, ainda não.
- **Pronto:** o anfitrião não tem botão de pronto (ele é quem começa).
- **Mundo do anfitrião:** fica salvo no computador do anfitrião, como sempre.

## Sem IP: só o código

**Pedido:** "no multiplayer faça não precisar colocar o IPv4, teria que ser automático a criação da sala e só mandar o código pros amigos".

- **No mundo:** os botões ficaram **Jogar** (sozinho) e **Jogar com amigos**. Este último cria a sala sozinho:
  - **Com a Epic** (há as credenciais): sala online com código de 5 letras, para amigos em qualquer lugar.
  - **Sem a Epic** (sem credenciais, ou a Epic não respondeu): a sala abre na rede local e também ganha um código.
    - O código começa com **L**. O IP e a porta vão escondidos nele, em base 32: `L` + 7 letras na porta 7777.
    - Ninguém digita IP. Quando a Epic não responde, o menu avisa.
- **Na sala de espera:** o **código** aparece grande, com o botão **Copiar**, que o põe na área de transferência.
- **Para quem entra:** Jogar com amigos → herói viajante → **um campo só, o código**:
  - código com **L**: vai direto ao endereço da rede local;
  - os outros: procura a sala da Epic.
  - O campo de IP saiu.
- **Teste:** `RunLobbySmoke` agora entra pelo código. Por exemplo, `LAD9AAAACUMD` vira `127.0.0.1:18787`, e passou com 2 convidados.
