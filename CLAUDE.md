# The World Gêneses (Vortex) — instruções para o Claude

Jogo em Unreal Engine 5.8 (C++), migrado do projeto Godot. Comece lendo [Docs/CONTINUAR_AQUI.md](Docs/CONTINUAR_AQUI.md),
que diz onde o trabalho parou, e o painel [Docs/README.md](Docs/README.md).

## Regras de trabalho (pedidas pelo usuário)

- **Git:** commit e push só quando o usuário pedir (12/11: repositório "leve" no GitHub, privado; o `.gitignore`
  deixa só código, Config, Tools, os JSON, os textos dos Docs e `Docs/Videos`). Nunca a arte pesada nem arquivos >100 MB.
- **Idioma:** responder sempre em português. O usuário acompanha pelo Remote Control (celular).
- **Antes de mudanças grandes ou difíceis de desfazer:** explicar o plano e esperar o OK.
- **Testes:** o usuário testa o jogo. Não rodar a suíte inteira (`Tools/RunAllTests.ps1`, ~25 min) sem ele pedir.
- **Compilar:** só com o editor da Unreal fechado.
- **Compilar menos (01/11):**
  - Evitar mexer nos `.h` muito incluídos (`VortexCharacter.h`, `VortexLook.h`, `VortexGameMode.h`...): cada mudança
    neles recompila o projeto quase inteiro (mais de 10 minutos). Quando der, a mudança vai no `.cpp`; quando não der,
    juntar as mudanças de cabeçalho numa compilação só.
  - Módulos (02/11): o código está em camadas. `Source/VortexUI` (a interface) fica em cima do `Source/Vortex` (o
    jogo), que fica em cima do `Source/VortexWorld` (o mundo) e do `Source/VortexData` (os catálogos dos JSON). Um
    módulo de baixo nunca inclui o de cima: o que ele precisa de lá vai por ganchos (`World/VortexWorldHooks.h`,
    `Framework/VortexUiHooks.h`, este gerado por `Tools/GenUiHooks.py`). Ver
    [Docs/01_Base/MODULOS.md](Docs/01_Base/MODULOS.md) antes de mover classes (precisa de `[CoreRedirects]`).
  - Listas e números de ajuste ficam em arquivos de dados (`Data/**/*.json`, lidos pelo jogo), não no código: ajustar
    não precisa compilar. Ao mexer num sistema, passar os ajustes dele para dados aos poucos.
- **Fotos e docs:** tirar fotos quando ajudar, atualizar os Docs e relatar cada passo.
- **Godot:** o projeto Godot é só leitura.
- **Segredo:** `Data/Online/eos_credentials.json` nunca vai para o git.
- **Modelagem:** o usuário prefere que a modelagem e o visual sejam feitos no Blender pelo MCP (o Blender aberto).
- **Quando o usuário pedir para parar ("pare tudo", "pause"):** parar na hora.
- **PC de 16 GB:**
  - Blender e Unreal pesados rodam um de cada vez.
  - Depois de parar uma tarefa, conferir se não ficou processo órfão (bash, Blender, Unreal).
  - Nas conversões longas, usar um vigia de memória.

## Caminhos e comandos

- **Blender:** `/c/Program Files/Blender Foundation/Blender 5.2/blender.exe` (sem janela: `-b --factory-startup -P <script> -- <args>`).
- **Commandlet da Unreal (editor fechado):**
  `"/c/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "<raiz>/Vortex.uproject" -run=pythonscript "-script=<raiz>/Tools/X.py" -unattended -nullrhi`
  (caminhos absolutos; a pasta tem espaços: "4 - Projetos").
- **Compilar:**
  `powershell -Command "& 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' VortexEditor Win64 Development '-Project=<raiz>\Vortex.uproject' -WaitMutex"`
- **MaterialCraft (03/11):** materiais procedurais por nós (como o Substance Designer), em
  `C:\Program Files\MaterialCraft\materialcraft-cli.exe`. O MCP `materialcraft` está na configuração geral do Claude
  Code, com leitura e escrita só nesta pasta.
  - Pelo CLI: `new <modelo> <arq.mcraft> --size 512`. Os modelos são stone_tiles, bricks, wood, metal_plates,
    rusty_metal, lava, marble, wicker, fabric e ground.
  - Depois: `export <arq> <pasta> --format png` (cor, normal, rugosidade, altura, metal e oclusão) e
    `render <arq> <png> --hq`.
  - Para o nosso estilo, reduzir para 32 ou 64 px com poucas cores e pixel nítido. Teste em
    `modelando ainda/materialcraft_teste`.
- **SoundCraft (03/11):** estúdio de áudio, em `C:\Program Files\ArtCraft\SoundCraft\soundcraft-cli.exe`. O MCP
  `soundcraft` também está na configuração geral, só nesta pasta. Tem sessões com faixas, importar áudio e
  `bounce_mix`, que devolve o pico e o LUFS.
  - Pelo CLI: `info <arquivo>` e `convert <entra> <sai.wav> --bit-depth 16 --rate 44100 --normalize`, que leva o pico
    a -0,1 dBFS.
  - Bom para igualar o volume dos efeitos, fazer variações, montar sons de ambiente e emendar o loop das músicas.
- **Os outros apps do ArtCraft (03/11, todos com MCP na configuração geral, presos a esta pasta):**
  - TextureCraft: pintar textura no modelo 3D, como o Substance Painter;
  - PhotoCraft: editar imagem (fichas de referência, ícones, cartas);
  - EffectCraft: efeitos e animação 2D (sequências de quadros para partículas e animações de menu);
  - VectorCraft: vetor (logo, ícones);
  - FigCraft: desenho de telas, HUD e menus;
  - GridCraft: planilhas, para o balanceamento.

  Os executáveis ficam em `C:\Program Files\ArtCraft\<App>\<app>-cli.exe` (o TextureCraft em
  `C:\Program Files\TextureCraft`). São versões alpha ou em desenvolvimento: a base continua sendo a Unreal e o Blender.

  O plano do usuário é usar tudo junto:
  - Blender: modelos e animação;
  - MaterialCraft e TextureCraft: materiais e pintura;
  - SoundCraft: som;
  - PhotoCraft, EffectCraft, VectorCraft e FigCraft: a interface e os efeitos;
  - Unreal: junta tudo.
- **Arquivos de modelagem em andamento:** `modelando ainda/` (o personagem de blocos, os cabelos, as animações do Mixamo).
