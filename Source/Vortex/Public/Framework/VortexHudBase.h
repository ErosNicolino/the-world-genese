#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "VortexHudBase.generated.h"

class AVortexWolf;

/**
 * O que o jogo pergunta ao HUD (02/11, a separação em módulos): o HUD de verdade (AVortexAimHUD) mora no módulo
 * VortexUI, em cima deste; o herói, o controle, as opções e os testes só conhecem esta base. Sem o HUD (servidor),
 * as perguntas respondem "não".
 */
UCLASS(Abstract)
class VORTEX_API AVortexHudBase : public AHUD
{
    GENERATED_BODY()
public:
    // medições/testes
    virtual bool HasMinimap() const { return false; }
    virtual const AVortexWolf* GetTargetWolf() const { return nullptr; }
    /** M: o mapa do mundo em tela cheia. */
    virtual void ToggleWorldMap() {}
    virtual bool IsWorldMapOpen() const { return false; }
    /** Roda do mouse com o mapa aberto. */
    virtual void ZoomWorldMap(float Steps) {}
    /** O marcador do jogador (clique direito no mapa). */
    virtual bool HasMapMarker() const { return false; }
    virtual FVector2D GetMapMarker() const { return FVector2D::ZeroVector; }
    virtual void SetMapMarker(const FVector2D& At) {}
    /** Testes: o mapa local do minimapa já foi montado. */
    virtual bool HasLocalMap() const { return false; }
    virtual bool HasDetailMap() const { return false; }
    virtual float GetMapZoom() const { return 1.f; }
    virtual void CenterWorldMap(const FVector2D& At) {}
    /** Parte do mapa já descoberta (0 a 1). */
    virtual float ExploredFraction() const { return 0.f; }
    /** Mapa de viajante: devolve quantos pedaços eram novos. */
    virtual int32 RevealTravelerRoute(int32 Hundreds, int32 Seed, const FVector& Hero) { return 0; }
    /** Testes: o pedaço daquele ponto já saiu da névoa. */
    virtual bool IsDiscovered(const FVector& At) const { return false; }
    /** Testes: abre a tela de carregamento como na saída do Estádio. */
    virtual void DebugBeginLoading() {}
    /** O Aplicar das opções: a tela de carregamento cobre a troca. */
    virtual void BeginApplyLoading() {}
    virtual bool IsLoadingScreen() const { return false; }
    virtual void ExportPortraits(const FString& Prefix) const {}
};
