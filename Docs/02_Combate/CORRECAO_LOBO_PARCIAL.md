# Lobo — correção parcial, aguardando retomada

## Estado

Este registro congela o trabalho no lobo para seguir à correção da câmera, conforme pedido do jogador. **O lobo não está concluído nem aprovado no editor.** A pelagem foi devolvida ao material cinza visível; a aparência ainda não reproduz os detalhes do Godot e os olhos precisam de avaliação.

## O que mudou e por quê

- Conferi os eixos do GLB original e convertido: a cabeça aponta no eixo local que a rotação atual da malha (`Yaw=-90`) leva à frente do ator. Mantive a orientação medida, sem escolher outro ângulo por tentativa.
- Ampliei a cápsula e acrescentei uma área de contato para o corpo, pois a antiga cápsula de 42 × 55 cm deixava o tronco fora da colisão. Preservei a cápsula como base do `CharacterMovement`.
- O lobo permanece oculto até o terreno local estar carregado, inclusive no cliente, para evitar a queda inicial no vazio. A pose anima mesmo fora da tela; o sistema agora carrega e confere os oito clipes.
- Acrescentei desvio simples de obstáculos à perseguição e ajustei as durações de uivo e reação a golpe para combinar com os clipes.
- Refiz materiais separados para pelo e olhos usando as cores de vértice do GLB. A importação contém cores, mas a reconstrução ainda resulta numa pelagem predominantemente cinza e pouco detalhada. Uma tentativa de aumentar contraste deixou o lobo preto; ela foi descartada.

## Validação feita

- Compilação do editor passou após as alterações em C++.
- `RunWolfSmoke.ps1`: passou; salto acertou o herói e o herói matou o lobo.
- `RunPackSmoke.ps1 -Count 5`: passou; cinco membros ativos.
- Conferi por script a existência dos oito clipes e dos dois materiais aplicados à malha.
- A referência visual lida sem modificar o Godot está em [wolf_godot_reference.png](../Images/wolf_godot_reference.png). As capturas do Unreal continuam em `Saved/Screenshots/wolf_*.png` para comparação, mas **não constituem aprovação visual**.

## Pendente ao voltar ao lobo

- Melhorar textura, contraste e olhos em comparação direta com o Godot, com fotos em sol pleno de frente, lados e costas, parado e andando.
- Confirmar em jogo que o focinho acompanha a direção da perseguição, que as oito animações tocam nas velocidades corretas e que as patas encostam no terreno em degraus.
- Testar colisão e desvio junto a árvores, pedras e herói; testar anfitrião e cliente, inclusive aparecimento depois da carga do terreno.
- Rodar a matilha nas demais contagens previstas e medir o custo da nova colisão/desvio.
- Pedir nova avaliação no editor antes de declarar a correção pronta.

## Risco, desempenho e reversão

A cápsula maior e a área extra podem alterar os caminhos estreitos; o desvio faz uma consulta de colisão adicional durante a perseguição. Não há medição de desempenho isolada do lobo nesta pausa. Para reverter este lote, restaure o commit anterior; a medição de terreno e o estado do projeto permanecem documentados em `CORRECAO_INICIAL_TERRENO_LOBO_ESPADA.md`.
