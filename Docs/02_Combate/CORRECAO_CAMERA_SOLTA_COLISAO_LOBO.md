# Câmera "solta" e herói entrando no lobo (aguardando teste no editor)

## Câmera que deixava de acompanhar

- **Sintoma:** às vezes a câmera parecia virar câmera livre: o herói andava e ela ficava para trás.
- **Causa:** depois de bater num obstáculo (um tronco, por exemplo), a câmera voltava à distância normal com uma interpolação de **posição no mundo** (`UVortexCameraBoom::BlendLocations`). Com o herói correndo, a posição alvo mudava a cada quadro; a interpolação nunca chegava, e a câmera ficava atrás até ele parar.
- **Correção:** a volta suave passou a ser só no **comprimento do braço**, na linha que sai do herói. A câmera continua sempre presa ao personagem; só a distância aumenta aos poucos. A resposta a um obstáculo continua imediata.
- **Teste:** `RunCameraSmoke` agora faz o herói saltar 30 m logo depois de a câmera bater no obstáculo.

| | Distância câmera–herói 0,1 s depois do salto |
|---|---|
| Antes | 1 317 cm (falhou) |
| Depois | 179 cm (passou) |

Se a câmera "sair" do herói de vez em quando no Play do editor, pode ser também a tecla **F8**, que solta a câmera no Play (ejetar); F8 de novo volta.

## Herói entrando no lobo, ou o lobo no herói

- **Causa:** o corpo do lobo (caixa de ~2,7 m de comprimento) é bem maior que a cápsula de movimento (raio de 72 cm). A cabeça e o salto passavam da cápsula e entravam no herói, e o corpo não bloqueava o herói.
- **Correção (`AVortexWolf`):**
  - a caixa do corpo **bloqueia** personagens: o herói não atravessa mais o lobo;
  - perto do herói, o lobo para de avançar na direção dele e perde o embalo naquela direção, e o salto não passa mais do ponto;
  - se mesmo assim ficar dentro, ele recua.
- **Medição (`RunWolfMotionProbe`, novo: quanto o herói entra no corpo do lobo, em perseguição, mordida e salto):**

| | 60 FPS | 120 FPS |
|---|---|---|
| Antes | 63 cm | 64 cm |
| Depois | 0 cm | 0 cm |

## Testes

- Compilação: passou.
- `RunCameraSmoke` (agora com o salto): passou.
- `RunWolfSmoke` (o lobo morde e o herói o mata), `RunPackSmoke` (5 lobos), `RunWolfNetworkSmoke`, `RunCombatSmoke`, `RunSwordSmoke` e `RunSolidWalkSmoke`: passaram.

## Riscos e reversão

- Lobos da matilha também se bloqueiam pelo corpo. Isso pode deixá-los menos amontoados, mas pode prendê-los em passagens estreitas; vale observar.
- Ao encostar, o lobo para de repente: isso é proposital, mas pode parecer brusco no salto.
- Reversão: reverter o commit.
