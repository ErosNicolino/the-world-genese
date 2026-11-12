# Alpha 1.0: instalador, FPS, neblina, claridade e manoplas (10/10/2026)

Situação: 🟡 instalador pronto; espera o seu teste no jogo instalado.

## O instalador
- **Arquivo:** `Build\Installer\TheWorldGeneses_Alpha_1.0_Setup.exe` (~550 MB; ~0,73 GB instalado).
- **O que ele faz:** instala em português, com o brasão como ícone e atalho na área de trabalho. Também instala sozinho as bibliotecas do Windows que a Unreal precisa.
- **Para gerar de novo** (com o editor fechado): `Tools\PackageAlpha.ps1`.
  - Compila em Shipping, cozinha o conteúdo e copia os dados de `Data` (os `.json`, `.bin` e `.txt`).
  - Gera o instalador com o Inno Setup.
  - Roda em prioridade baixa, compila só para DirectX 12 e deixa 4 núcleos livres, então não trava mais o PC.
- **Saves do jogo instalado:** ficam em `%LOCALAPPDATA%\Vortex\Saved`, separados dos do editor.
- **Corrigido no caminho:**
  - o mundo não entrava no pacote: a lista de mapas estava em `DefaultEngine.ini`, mas o empacotador lê `DefaultGame.ini`;
  - o material das armas falhava fora do editor.

## Por que o jogo instalado rodava a 30–60 FPS
Sem configuração salva, a Unreal começa tudo no **Épico**. O mais caro disso são as **sombras virtuais** (a sombra nova da Unreal 5). No editor, as sombras estavam no mínimo; por isso lá rodava a ~100 FPS e parecia mais claro.

Medido no próprio jogo empacotado, a 1920×1080, num passeio de 60 s a galope:

| Sombras | FPS médio |
|---|---|
| Virtuais (o padrão antigo, níveis 1 a 3 iguais) | 62–79 |
| **Clássicas (o padrão novo)** | **89–92** |
| Sem sombras | 102 |

- **Padrão novo:** sombras clássicas. A opção **Gráficos → Tipo de sombra** troca entre "Leves" e "Detalhadas" (as virtuais).
- **Instalações que já existem:** passam para as leves sozinhas na próxima vez que o jogo abrir.
- **O que ainda pesa:** é a thread de renderização, que prepara os desenhos para a placa de vídeo (11 ms por quadro). A placa de vídeo usa só 9 ms.

## Mais claro
Sem a luz global (Lumen, desligada por padrão), a parte na sombra só recebia a luz do céu e ficava escura demais. Agora a luz do céu fica **1,6× mais forte** quando a luz global está desligada. Com ela ligada, nada muda.

## Neblina nova (pacote FogArea, do VortexReferencia)
- **Névoa de distância:** a névoa grossa de antes ficou bem mais leve, só para dar profundidade (densidade 0,11 → 0,045).
- **Neblina baixa:** manchas de névoa com ruído do **FogArea** (`/Game/FogArea`, copiado do VortexReferencia), em volta de quem joga (`AVortexMist`).
  - Aparecem mais nos vales, na beira da água e nas matas.
  - Cada quadra de 90 m sorteia se tem neblina, e o sorteio é sempre o mesmo para o mesmo lugar.
  - Aparecem a até ~210 m; as que ficam para trás somem.
- **Por que precisou da névoa volumétrica:** o FogArea é um material de volume, que só aparece com a névoa volumétrica da Unreal ligada. A neblina liga essa névoa numa resolução mais barata e sem engrossar a névoa geral.
  - Também aumenta o alcance do volume de 60 m (o padrão da Unreal) para 220 m.
- **Opção:** **Gráficos → Neblina nos vales** (ligada por padrão). Custo medido: menos de 1 ms.

## Manoplas em primeira pessoa
- **Invertidas:** o código apontava para a frente o lado mais distante do centro do modelo. Na manopla, esse lado é o punho largo, não os dedos.
  - Agora os dedos apontam para a frente, com os espetos dos nós dos dedos para cima, nas duas mãos.
- **Sem golpes:** a pose era escolhida pelo tipo de arma da ficha, não pela arma na mão. A manopla caía na pose da espada em par, e o soco andava na direção do punho e quase não aparecia.
  - Agora o soco vai para a frente e para o centro da tela, alternando as mãos.

| | |
|---|---|
| ![Manoplas](../Images/alpha_1_0/manoplas.jpg) | ![Soco](../Images/alpha_1_0/soco.jpg) |

## O que conferir
1. Instale por cima com o setup novo e entre no mundo. O FPS deve estar perto de 90 em 1080p; ligue o contador em **Interface → Mostrar FPS**.
2. Se a claridade agradou.
3. A neblina nos vales e na beira da água. Se ficou fraca ou forte demais, diga que eu ajusto.
4. As manoplas em primeira pessoa (C), parado e socando.
