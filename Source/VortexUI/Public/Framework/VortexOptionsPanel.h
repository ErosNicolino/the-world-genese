#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWidget.h"

/**
 * Painel de opções em abas (Tela, Gráficos, Shaders, Controles, Interface), o mesmo no menu inicial
 * e no menu de pausa. Cada linha tem nome, uma frase do que muda, "<  valor  >" e marcadores do
 * nível; passar o mouse acende a linha, o valor pisca ao mudar e a aba entra deslizando.
 * As opções vêm de VortexGraphics::Options.
 */
VORTEXUI_API TSharedRef<SWidget> MakeVortexOptionsPanel(TFunction<void()> OnClick = nullptr, float BodyHeight = 560.f, int32 InitialTab = 0);
