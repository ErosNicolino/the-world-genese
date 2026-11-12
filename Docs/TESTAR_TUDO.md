# Como testar tudo de uma vez

São duas partes:
1. Os **testes automáticos**: o computador joga sozinho e diz o que passou.
2. Um **passeio no editor** (uns 20 minutos), em que você confere com os olhos o que só uma pessoa avalia.

## 1. Testes automáticos

Feche o editor da Unreal e rode no PowerShell, na pasta do projeto:

```powershell
powershell -ExecutionPolicy Bypass -File Tools\RunAllTests.ps1
```

**O que roda (cerca de 20 a 30 min, 19 testes):**
- save, heróis, fluxo do menu;
- movimento (incluindo rampas, escada e piscina), esquiva nas 4 direções, colisão com árvores e pedras;
- câmera, combate, espada, lobo, matilha, região do vale;
- rede: sessão, movimento, entrada do cliente, lobo, dia e noite, LAN.

**Opções:**

| Opção | O que faz | Tempo |
|---|---|---|
| `-Completo` | Também roda rede com atraso, streaming, NPCs, as fotos (menu, HUD, dia e noite, grama) e a medição de FPS | cerca de 1 h |
| `-Compilar` | Compila o projeto antes de testar | — |
| `-Parar` | Para no primeiro teste que falhar | — |

**Como ler o resultado:** cada teste aparece como **PASSOU** (verde) ou **FALHOU** (vermelho), com a linha que explica. O resumo fica em `Saved\Logs\RunAllTests.txt` e as fotos em `Saved\Screenshots`.

## 2. Passeio no editor

Abra pelo `AbrirEditor_DX11.bat`.

**Para começar pelo menu (recomendado):**
1. Abra o mapa `Content/Vortex/Maps/L_Menu`.
2. Na seta ao lado do Play, escolha **Standalone Game**.

O mapa que abre por padrão no editor (`L_TerrainProto`) cai direto no vale, sem menu; serve para testar só o jogo.

**Menu e criação (5 min):**
- [ ] A cortina abre, a música começa e um herói sorteado aparece no palco de pedra.
- [ ] Novo mundo → dê um nome → Criar.
- [ ] "+ Novo herói" → passe o mouse nas cartas de raça (sobem e brilham) → escolha uma e veja a revelação.
- [ ] Na criação, para cada raça que quiser (o botão "Trocar raça" volta às cartas):
  - [ ] troque as peças com < >;
  - [ ] clique nas cores;
  - [ ] arraste para girar, use a roda do mouse e o botão "Rosto";
  - [ ] use "Surpreenda-me" e "Sortear nome";
  - [ ] confira o formato da raça: anão baixinho e parrudo, bárbaro gigante, morto-vivo careca com crânio, elfo com orelhas, diabo com chifres.
- [ ] Criar herói → comemoração → o herói aparece na lista → **Entrar no mundo**.

**No jogo (10 min):**
- [ ] O herói está com a roupa, as cores e o tamanho escolhidos. A HUD mostra o nome e a raça, e o retrato é o rosto dele.
- [ ] **Movimento:** WASD, Shift (correr), Espaço (pular), Ctrl (esquiva; os losangos em cima da barra gastam e voltam).
- [ ] **Combate:** clique esquerdo (combo), **botão direito** (corte à distância, com recarga na barra), **segurar F** (defesa), H (recuperar vida com mana).
- [ ] Ache lobos (marcas vermelhas no minimapa). A barra do lobo aparece no topo, com os números de dano.
- [ ] Minimapa: a tecla **M** troca o alcance.
- [ ] **Dia e noite:** console (tecla `) `Vortex.Tempo.Escala 60` para ver um dia em ~40 s; confira o pôr do sol, a noite com lua e estrelas, os vaga-lumes e os grilos. `Vortex.Tempo.Escala 1` volta ao normal.
- [ ] **Lago:** a água parada e limpa, sem tremer ao girar a câmera; perto da margem dá para ouvir o riacho.
- [ ] Praça dos NPCs, perto de (0, 15 m): os moradores aparecem e passeiam.
- [ ] Esc → voltar ao menu → o herói continua na lista → entrar de novo: ele volta onde estava.

**Com um amigo na rede local (5 min, opcional):**
- [ ] Um hospeda o mundo (Meus mundos → Hospedar) e o outro entra por "Jogar com amigos (LAN)" com o IP.
- [ ] Cada um vê o herói do outro com a raça e a roupa certas.
- [ ] Andem para longe um do outro: o chão aparece sem buracos.
- [ ] Lutem juntos com os lobos.

**Se algo estiver estranho:** anote o que fez e mande junto `Saved\Logs\Vortex.log`. Esse arquivo é o registro do jogo.
