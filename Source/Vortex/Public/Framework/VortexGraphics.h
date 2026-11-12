#pragma once

#include "CoreMinimal.h"
#include "Framework/VortexViewScales.h"

class UWorld;

/**
 * Opções do Vortex (tela, gráficos, shaders, controles e interface), montadas pelo painel de opções
 * em abas (Framework/VortexOptionsPanel) no menu inicial e no menu de pausa. Cada passo já aplica e grava.
 *
 * - Qualidade geral é um pacote: sombras, efeitos, texturas, luz, densidade da vegetação, água e
 *   oclusão; NÃO mexe na distância (o terreno gerado é sempre o mesmo).
 * - Distância de visualização é à parte: até onde aparecem árvores, plantas e pedras.
 * - Shaders (desligados por padrão): luz global e reflexos do Lumen, raios de sol, desfoque de
 *   movimento, bloom e névoa volumétrica.
 * Ver Docs/04_Interface/OPCOES_GRAFICAS.md.
 */
namespace VortexGraphics
{
    struct FOption
    {
        FString Tab;
        FString Section;
        FString Name;
        /** Uma frase: o que muda e quanto pesa. */
        FString Description;
        FString Value;
        /** Posição na escala (para os marcadores) e quantos valores há; Count 0 = sem marcadores. */
        int32 Index = 0;
        int32 Count = 0;
        /** -1 / +1: valor anterior / seguinte (aplica e grava). */
        TFunction<void(int32)> Step;
    };

    VORTEX_API const TArray<FString>& Tabs();
    /** As opções de uma aba, na ordem da tela. */
    VORTEX_API TArray<FOption> Options(const FString& Tab);
    /** Volta tudo da aba ao padrão. */
    VORTEX_API void ResetTab(const FString& Tab);

    /** Aplica as opções do Vortex ao motor (variáveis de render) e ao mundo (água, grama, câmera). */
    VORTEX_API void Apply(UWorld* World = nullptr);
    /** Aplica e grava um preset (0 Baixa, 1 Média, 2 Alta, 3 Épica): qualidade da Unreal, grama, água, AO e anti-aliasing. */
    VORTEX_API void ApplyPreset(int32 Level);
    /**
     * Opções com "Aplicar": enquanto o painel está aberto ele edita uma cópia das opções (o jogo continua com as de
     * verdade); Aplicar passa a cópia para as de verdade, grava e aplica tudo de uma vez, Descartar volta.
     */
    VORTEX_API void BeginEditing();
    VORTEX_API void EndEditing();
    VORTEX_API bool HasPending();
    VORTEX_API void ApplyPending();
    VORTEX_API void DiscardPending();
    /** 11/11: do clique no Aplicar até o mundo terminar de remontar (o painel mostra "Aplicando..." e não aceita cliques). */
    VORTEX_API bool IsApplying();
    /** A placa e o jogo têm DLSS (RTX e o plugin da NVIDIA presente). */
    VORTEX_API bool DlssSupported();

    /** Todos os efeitos dos shaders ligados (1), desligados (0) ou misturados (-1). */
    VORTEX_API int32 ShadersState();
    VORTEX_API void SetShaders(bool bOn);
    /** 02/11: na primeira abertura (GameUserSettings.ini [Vortex.Graphics] AutoSetup), a qualidade recomendada do PC (o
     *  teste de hardware, Recommend) com a neblina ligada; o menu pergunta se liga os shaders (NeedsShaderChoice). */
    VORTEX_API void FirstRunSetup();
    VORTEX_API bool NeedsShaderChoice();
    VORTEX_API void ChooseShaders(bool bOn);

    // GrassDistanceScale, ViewDistanceScale e GrassDensity: Framework/VortexViewScales.h (módulo VortexWorld, o streamer usa)
    VORTEX_API float FieldOfView();
}
