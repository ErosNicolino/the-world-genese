# Correções iniciais antes da revisão do lobo e da câmera

## O que mudou e por quê

- A configuração de DirectX 12 SM6 feita pelo usuário em `Config/DefaultEngine.ini` foi incluída. O passeio de medição usa `-dx12` para testar Nanite e sombras virtuais nesse caminho.
- O lobo colocado no mapa de terreno aguarda a colisão do pedaço em que nasceu. Até então fica oculto, sem colisão nem movimento. Ao encontrar o chão, é posicionado sobre ele e inicia a lógica de perseguição. A distância de salto e o tempo de recuo foram corrigidos para permitir o acerto observado no protótipo.
- A importação do lobo aplica os materiais de pelo e olhos à malha usada no jogo; o material branco importado deixou de ser a única opção da malha. A revisão de cor e animação terá um commit próprio.
- O terreno usa o atlas `terrain_tiles.png` do projeto Godot, copiado sem alterar a origem. A malha gera UVs por célula, e a cor regional exportada é recuperada sem multiplicar duas vezes a cor média da peça.
- O sol, a luz do céu, a névoa e a exposição do mapa de teste foram ajustados para reduzir o excesso de claridade.
- Os golpes consecutivos da espada fazem uma transição direta entre poses do tronco, sem voltar à pose de caminhada entre dois clipes do combo.

## Validação e imagens

- Build `VortexEditor Win64 Development`: passou.
- O registro do jogo mostrou `VORTEX_WOLF_GROUND_READY z=857.0` e ataques com dano após o ajuste do alcance.
- [Luz anterior](../Images/lighting_before.png) e [terreno e lobo após a primeira correção](../Images/wolf_before_correction.png). A segunda imagem também documenta que o lobo ainda está escuro.
- Passeio DX12 SM6, primeira execução: 63,6 FPS, 1% piores 5,4, pior quadro 937,5 ms; o carregamento inicial causou travadas.
- Passeio DX12 SM6, cache aquecido: **71,4 FPS, 1% piores 48,8**, pior quadro 26 ms, nenhum acima de 40 ms. A referência anterior era **121 FPS, 1% piores 66**. A perda aquecida é de 41,0% no FPS médio e 26,1% no 1% pior. O registro confirma Lumen GI/reflexos, sombras virtuais e Nanite ativos (`gi=1 reflexos=1 sombras_virtuais=1 nanite=1`). A comparação inclui tanto a mudança de SM6 quanto a nova textura/luz, portanto não isola o custo de cada uma.
- Validação do ângulo, cor e animação do lobo no editor: pendente.

## Risco, desempenho e reversão

O atlas acrescenta uma amostra de textura por pixel do chão; o SM6 e as sombras virtuais podem alterar o custo de renderização. O lobo só inicia a lógica de 10 Hz após o chão estar pronto. Se a colisão do pedaço nunca carregar, ele permanece oculto; a revisão do lobo verificará também anfitrião e cliente. Para reverter este conjunto, use `git revert` no commit desta etapa; o projeto Godot e os saves não foram alterados.
