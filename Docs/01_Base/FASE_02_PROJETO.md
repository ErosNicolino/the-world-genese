# Vortex Unreal — Fase 2: projeto C++

**Data:** 03/10/2026

**Engine usada:** Unreal Engine 5.8.3 (build 58210709)
**Estado:** concluída para revisão; a Fase 3 depende de aprovação.

## WHAT / O que foi alterado

- Criado `Vortex.uproject` como projeto Unreal 5.8 separado do Godot, em `Vortex-UE5-migracao/Vortex/`.
- Criado um único módulo runtime C++ (`Source/Vortex`) e dois alvos de build (`VortexEditor`, `Vortex`). Ainda não existem GameMode, personagens, assets importados nem lógica de gameplay.
- `Config/DefaultEngine.ini` usa o mapa `Template_Default` da engine somente para iniciar/verificar o projeto. Nenhuma escolha de terreno, renderer ou tecnologia de mundo foi congelada.
- Configurados nome, versão inicial `0.1.0`, README, regras de Git e repositório local na branch `main`. `Binaries`, `Intermediate`, `Saved` e cache ficam fora do histórico.
- Instalado o componente `Microsoft.Net.Component.4.8.SDK` no Visual Studio Build Tools 2022, necessário para a cadeia de build da Unreal nesta máquina. O instalador terminou com código 0; a presença do SDK foi confirmada.

## WHY / Por quê

A auditoria exige reconstrução nativa e modular. Esta fase estabelece um projeto compilável, reversível e versionado antes da implementação do framework. O módulo único reduz dependências prematuras; novos módulos só serão separados quando contratos e carga justificarem.

## DEPENDENCIES / Dependências verificadas

- Unreal Engine 5.8.3 instalada em `C:/Program Files/Epic Games/UE_5.8`.
- Visual Studio Build Tools 2022, MSVC 14.44 e Windows SDK 10.0.26100.
- .NET Framework SDK 4.8, instalado durante esta fase.
- Git local. Sem remoto configurado ou publicação.
- As duas cópias Godot da Fase 0 permanecem separadas; nenhum asset foi copiado para a Unreal.

## VALIDATION / Validação executada

| Verificação | Resultado |
|---|---|
| `Vortex.uproject` parseado como JSON e associação 5.8 conferida | Passou |
| `VortexEditor Win64 Development` | Passou; `UnrealEditor-Vortex.dll` criado |
| `Vortex Win64 Development` | Passou com `-NoDebugInfo -NoLinkerDebugInfo`; `Vortex.exe` criado |
| Carregamento do projeto por `UnrealEditor-Cmd.exe`, modo sem renderização | Passou; código 0, `Success - 0 error(s), 0 warning(s)` |
| Verificação de SDK Win64 durante a abertura | Passou |
| Abertura gráfica interativa e jogo empacotado/cooked | Ainda não verificados nesta fase |

Comando usado para a abertura sem interface:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'C:\Users\Valorant Niko\Documents\ChatGPT\Mangas\Vortex-UE5-migracao\Vortex\Vortex.uproject' -run=ResavePackages -NoSave -Unattended -NullRHI -NoSplash -ProjectOnly -Log
```

O comando não salva pacotes. A saída indica projeto carregado com sucesso; não prova apresentação visual ou gameplay.

## RISK / Riscos e decisões em aberto

- **Memória virtual:** a máquina tem ~16 GB de RAM e pagefile fixo de 16 GB. Uma build paralela atingiu o limite de commit; o linker de jogo com símbolos completos falhou com `LNK1102`. Uma build sequencial de editor passou. A build de jogo passou ao desabilitar informações de debug somente nesse comando. O projeto não fixa essa escolha; outra máquina pode compilar com símbolos.
- **Renderer e mapa:** o mapa de template é provisório. Não ativar Nanite, Lumen, World Partition, GAS, PCG, EOS ou plugins extras antes dos protótipos e decisões por fase.
- **Depuração de jogo:** a build de jogo validada não contém PDB completo. Depuração C++ deve usar por enquanto o alvo de editor, que compilou com símbolos.
- **Editor gráfico:** a abertura sem interface passou, mas a apresentação do projeto na janela interativa será conferida ao iniciar a Fase 3.

## PERFORMANCE / Impacto medido

- Primeiro build do alvo de editor com compilação serial: cerca de 144 s após o SDK estar instalado.
- Build de jogo com opções de memória reduzida: cerca de 133 s.
- Carregamento sem renderização: processo concluiu com pico físico de ~1,62 GB informado pela Unreal. Não é benchmark de FPS, GPU ou cena jogável.
- Não há regressão de gameplay mensurável: o projeto ainda é um esqueleto. O baseline real continuará sendo os cenários Godot documentados na auditoria.

## ROLLBACK / Como voltar

O marco inicial está em um commit local na branch `main`. Reverter futuras mudanças Unreal pode ser feito por commit; a versão Godot original e as cópias de código/saves da Fase 0 permanecem independentes. Nenhuma conversão dos saves foi executada.

## Próxima fase proposta — 3, framework

Implementar contratos mínimos de `GameInstance`, `GameMode`, `GameState`, `PlayerController`, `PlayerState`, `Character` e subsistemas estritamente necessários. Entrega exigida: mapa de teste próprio, sessão host+cliente, ownership e estado replicado observáveis, build de editor, testes de entrada/saída e comparação de custo básico. Definir API de IDs/estado antes de qualquer implementação de inventário ou combate. Parar ao final para revisão.
