# Multiplayer online pela Epic (EOS)

**Pedido:** "o multiplayer era da Epic, olhe como era do Godot".

## Como o Godot fazia

O Godot usava `Systems/Network/eos_bridge.gd` com o plugin EOSG:

- **Entrada sem conta:** login anônimo (EOS Connect com o id do aparelho), usando o nome do jogador.
- **Anfitrião:** cria uma sala pública no "balde" da versão do jogo, com o atributo `codigo` (5 letras de `ABCDEFGHJKLMNPQRSTUVWXYZ23456789`). Sem migração de anfitrião: se ele sai, a sala acaba.
- **Convidado:** procura balde + código (6 tentativas, 2 s entre elas), entra e conecta no dono da sala por P2P da Epic, com relay. Ninguém precisa abrir porta.
- **Credenciais:** ficam em `eos_credentials.json` (produto, sandbox, deployment e o cliente).

## Como o Vortex faz (igual, na Unreal)

- **`UVortexOnline`** (`Source/Vortex/*/Online/`, subsistema do GameInstance) usa o SDK da EOS que vem com a Unreal (plugin EOSShared):
  - login com o id do aparelho (`EOS_ECT_DEVICEID_ACCESS_TOKEN`; na primeira vez cria o usuário);
  - a sala tem balde `vortex-1`, até 8 jogadores e o atributo `codigo`;
  - a busca faz 6 tentativas, 2 s entre elas.
  - O OnlineSubsystemEOS da Unreal **não** foi usado: ele não aceita esse login anônimo.
- **Transporte:** o P2P da Epic é registrado como o subsistema de sockets "EOS" (`FSocketSubsystemEOS`, plugin SocketSubsystemEOS). O `GameNetDriver` passou a ser o `NetDriverEOS` (`Config/DefaultEngine.ini`):
  - endereço `EOS:<id do anfitrião>` → P2P da Epic, com relay;
  - qualquer outro endereço, ou mundo aberto com `?bIsLanMatch` → IP normal, como antes. A rede local e os testes continuam iguais.
- **Credenciais:** `Data/Online/eos_credentials.json`, copiado do Godot por `Tools/CopyEosCredentials.py` (sem mostrar nada na tela).
  - Está no `.gitignore`: o segredo do cliente **nunca** vai para o git.
  - Num computador novo, rode o script de novo.

## No menu

| Onde | O que faz |
|---|---|
| **Meus mundos** → **Jogar com amigos** (antes "Online") | Escolhe o herói e clica **Abrir a sala de espera**. O jogo entra na Epic, cria a sala e abre a [sala de espera](SALA_DE_ESPERA.md), com o código. No mundo, o código aparece no alto da tela e no menu de pausa (Esc). |
| Sem a Epic | A sala abre sozinha na rede local, com um código "L..." que esconde o IP ([SALA_DE_ESPERA](SALA_DE_ESPERA.md)). |
| **Jogar com amigos** | Primeiro escolhe ou cria um **herói viajante**, depois o **código da sala online** (5 letras) + **Entrar**, ou o **IP** da rede local + **Conectar**. O progresso aparece embaixo ("Procurando a sala ABCDE..."). |
| **Voltar ao menu** | O anfitrião desfaz a sala; o convidado sai dela. |

## Limites de agora

- **Teste:** precisa de **dois computadores** (ou um amigo). No mesmo PC, os dois jogos teriam o mesmo id de aparelho na Epic.
- **Herói do convidado:** resolvido. Ele entra com um herói viajante do próprio computador (ver [SALA_DE_ESPERA](SALA_DE_ESPERA.md)).

## Como testar

1. Nos dois computadores, rode `python Tools/CopyEosCredentials.py` uma vez. O Godot precisa estar na pasta ao lado.
2. **Anfitrião:** Meus mundos → **Online** → herói → Jogar. Anote o código no alto da tela.
3. **Amigo:** Jogar com amigos → digite o código → **Entrar**.
4. No log (`Saved/Logs`), os passos aparecem como `VORTEX_ONLINE ...`:
   - entrou na Epic;
   - sala criada código=ABCDE;
   - entrou na sala;
   - viajando até o anfitrião.
