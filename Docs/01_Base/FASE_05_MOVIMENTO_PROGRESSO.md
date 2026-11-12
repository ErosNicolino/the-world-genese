# Vortex Unreal — Fase 5: movimento, trabalho em andamento

**Data:** 03/10/2026
**Estado:** protótipo de rampa e rede validado; Fase 5 ainda aberta.

## WHAT / O que foi alterado

- `AVortexCharacter` agora recebe movimento WASD, câmera por mouse, salto e corrida com Shift. A câmera segue o personagem com braço, colisão e suavização.
- `CharacterMovementComponent` foi configurado inicialmente com 4,6 m/s de caminhada, 7,2 m/s de corrida, salto 8 m/s, gravidade aproximada de 24 m/s², degrau máximo de 45 cm e piso caminhável até 45°. São valores de partida derivados do código Godot, não ajuste final de sensação.
- Criado `L_MovementProbe`: dez blocos visuais de 30 cm de incremento, rampa física suave separada, penhasco físico, obstáculo e uma pista de degraus físicos de 30 cm e 60 cm. Os blocos visuais da rampa têm colisão desabilitada; penhasco, obstáculo e degraus físicos mantêm colisão.
- Criados scripts para geração, inspeção e reparo do mapa, além dos testes locais e de servidor/cliente.

## WHY / Por quê

O problema prioritário do Godot é a oscilação e o travamento ao caminhar diretamente sobre degraus blocky. O teste prova que a malha visual pode ser independente da superfície física de locomoção, sem transformar o penhasco em uma rampa atravessável.

## DEPENDENCIES / Dependências

`ACharacter` e `UCharacterMovementComponent` nativos. O teste usa cubos simples da engine e nenhum asset Godot. O mapa não seleciona a tecnologia final de terreno (Landscape, mesh procedural ou híbrida).

## VALIDATION / Validação

| Verificação | Resultado |
|---|---|
| Build `VortexEditor Win64 Development` | Passou |
| Mapa salvo e reaberto, com colisão visual desligada e rampa no eixo correto | Passou |
| Percurso local de rampa, bloqueio no penhasco e salto/pouso | Passou |
| Degrau de 30 cm transposto e degrau de 60 cm bloqueando | Passou; posição final x=607,9 cm, z=128,0 cm |
| Deslocamento do servidor observado pelo cliente | Passou |
| Build `Vortex Win64 Development` | Passou com opções de memória reduzida documentadas no README |

O teste local registrou subida de **296,4 cm**, maior variação vertical de **2,8 cm por amostra**, posição final **x=1037,9 cm** diante do penhasco, salto até **z=504,0 cm** e pouso. A sonda manteve velocidade física constante para isolar colisão; esses números não são um teste de sensação ao teclado nem um benchmark de frame time. O teste de rede confirmou no cliente uma posição replicada acima de x=900 cm enquanto o servidor executava o percurso.

Na pista de degraus, `Tools/RunMovementSmoke.ps1 -StepProbe` registrou **x=607,9 cm, z=128,0 cm**: o personagem chegou à plataforma 30 cm acima do piso inicial e parou antes da face do degrau seguinte, de 60 cm. O teste da rampa foi repetido depois da alteração do mapa e continuou passando.

O mapa inicial falhou no primeiro teste porque a rotação da rampa foi aplicada no eixo errado e a colisão dos blocos visuais voltou a estar ativa após salvar. A inspeção do mapa reaberto identificou isso; o asset foi corrigido e a geração foi ajustada para criar a versão correta desde o início.

## RISK / O que falta para fechar a Fase 5

- Validar controles e câmera visualmente no editor; os testes sem renderização não provam sensação, enquadramento ou ausência de flick na câmera.
- A tentativa de captura gráfica adicional em 03/10/2026 falhou na inicialização por falta de memória virtual disponível (log da Unreal: menos de 6 MiB livres no limite de commit e falha ao reservar 0,5 MiB). O teste gráfico não chegou a avaliar o mapa. As validações locais e de rede sem renderização já registradas permanecem válidas. Não iniciar editor e captura simultâneos nesta máquina sem liberar memória virtual.
- Completar a matriz de inclinações, escadas, água e dash/rolamento. O mapa agora cobre uma rampa, um penhasco e o limiar de passo 30/60 cm.
- Testar salto e corrida por entradas reais de cliente, inclusive com latência e perda. O teste de rede atual move o pawn pela física do servidor; o salto foi validado somente em jogo local.
- Integrar o proxy a chunks/terreno gerado, com atualização de colisão e navegação. A tecnologia de terreno continua em avaliação.
- Medir frame time, correções de rede e custo de colisão em cenário representativo. Ainda não existe cena equivalente ao Godot para comparação honesta de FPS.
- A corrida local altera `MaxWalkSpeed` no cliente e envia RPC ao servidor. A predição deve ser refinada em um movimento salvo/customizado antes de gameplay competitivo.

## PERFORMANCE / Medição

O percurso curto não apresentou salto vertical maior que 2,8 cm entre amostras de 20 ms, sob a velocidade fixa da sonda. Isso mede continuidade geométrica, não FPS, GPU ou custo de streaming. O código normal de locomoção usa o componente nativo; os timers de sonda só são ativados por flags de desenvolvimento.

## ROLLBACK / Reversão

O protótipo será mantido em commit próprio. Remover o commit restaura o framework da Fase 4; o Godot e seus saves ficam intactos. O mapa de prova pode ser descartado sem afetar o mapa de framework.

## Próximo trabalho

Fechar a matriz de movimento e o teste visual antes de iniciar animação. A autorização do usuário permite avançar de fase sem novo pedido, mas os critérios técnicos da Fase 5 ainda precisam passar.
