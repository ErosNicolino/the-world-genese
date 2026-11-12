# Fase 10 — primeiro lobo hostil

## Objetivo e origem

Introduzir um inimigo completo no ciclo mínimo de combate, depois da base de atributos, combate e espada das Fases 7–9. A referência de movimentos e do modelo foi lida de `ValleyMonster.*` e `Art/Creatures/Wolf.glb` no projeto Godot. Nenhum arquivo Godot ou save foi alterado.

## O que mudou

- `ConvertGodotWolf.py` transforma as partes rígidas do lobo original em uma malha com esqueleto e pele, na escala da espécie usada pelo Godot. O GLB de trabalho fica em `Data/GodotWolf/`; `ImportWolf.py` cria a malha, oito clipes e materiais de cor por vértice em `Content/Vortex/Creatures/Wolf/`.
- `AVortexWolf` usa `ACharacter`, cápsula e movimento da Unreal, atributos do GAS e o componente de combate da Fase 8. A vida e o dano são calculados pelo nível. O servidor decide alvo, visão, estado e impacto em intervalos de 0,1 s. Movimento e estado são replicados para os clientes.
- Há estados de observação, perseguição, uivo, mordida, salto, recuo, retorno, reação a golpe e morte. O salto avança durante a preparação: sem esse avanço, o lobo começava fora do alcance e o impacto não podia acertar. A morte interrompe decisões e impactos pendentes.
- `UVortexWolfAnimInstance` mistura os oito clipes importados diretamente em C++, com transições curtas, inclusive movimento e morte. `PlaceWolfInTerrain.py` colocou um lobo próximo da entrada de `L_TerrainProto` para teste manual. O diretório foi incluído nos caminhos de cook.

## Validação

- Build `VortexEditor Win64 Development`: passou.
- `RunWolfSmoke.ps1` colocou o lobo a 4 m do jogador, confirmou o início de `Pounce` nessa distância, o acerto de 14 pontos de dano, o contra-ataque do jogador e o estado `Dead` com vida zero. Registro: `VORTEX_WOLF_SMOKE_PASS playerHealth=86.0 wolfHealth=0.0 state=10`.
- **Pendente no editor:** conferir aparência, orientação, pés no chão, transições dos clipes, resposta a golpes, posição no terreno, host e cliente. Esta etapa ainda aguarda o teste visual do usuário e não está declarada concluída.

## Desempenho, riscos e reversão

As decisões rodam a 10 Hz no servidor; cada lobo examina os jogadores em até 25 m e traça a visão do candidato mais próximo. Uma matilha ainda precisará de coordenação e medição com vários lobos na Fase 11. O deslocamento usa velocidade direta do `CharacterMovement`, não navegação por caminhos; portanto pedras e árvores podem interromper a perseguição. Áudio, efeitos finais, loot e persistência do lobo ficam para fases próprias.

Para reverter, volte apenas o commit da Fase 10; a espada e os sistemas das Fases 7–9 permanecem. O projeto Godot original não foi modificado.
